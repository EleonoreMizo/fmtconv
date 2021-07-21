/*****************************************************************************

        Bitdepth.cpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtc/Bitdepth.h"
#include "fmtc/SplFmtUtl.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fmtcl/ProxyRwSse2.h"
#endif
#include "fmtcl/VoidAndCluster.h"
#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"

#include <algorithm>
#include <stdexcept>

#include <cassert>
#include <cmath>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Bitdepth::Bitdepth (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "bitdepth", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, nullptr), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_processor (vsapi, *this, "bitdepth", true)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_splfmt_src (fmtcl::SplFmt_ILLEGAL)
,	_splfmt_dst (fmtcl::SplFmt_ILLEGAL)
,	_scale_info_arr ()
,	_upconv_flag (false)
,	_sse2_flag (false)
,	_avx2_flag (false)
,	_full_range_in_flag (false)
,	_full_range_out_flag (false)
,	_range_def_flag (false)
,	_dmode (get_arg_int (in, out, "dmode", DMode_FILTERLITE))
,	_pat_size (get_arg_int (in, out, "patsize", PAT_WIDTH))
,	_ampo (get_arg_flt (in, out, "ampo", 1.0))
,	_ampn (get_arg_flt (in, out, "ampn", 0.0))
,	_dyn_flag (get_arg_int (in, out, "dyn", 0) != 0)
,	_static_noise_flag (get_arg_int (in, out, "staticnoise", 0) != 0)
,	_correlated_planes_flag (get_arg_int (in, out, "corplane", 0) != 0)
,	_tpdfo_flag (get_arg_int (in, out, "tpdfo", 0) != 0)
,	_tpdfn_flag (get_arg_int (in, out, "tpdfn", 0) != 0)
,	_errdif_flag (false)
,	_simple_flag (false)
,	_dither_pat_arr ()
,	_amp ()
,	_buf_factory_uptr ()
,	_process_seg_int_int_ptr (nullptr)
,	_process_seg_flt_int_ptr (nullptr)
{
	fstb::unused (user_data_ptr);

	vsutl::CpuOpt  cpu_opt (*this, in, out);
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (_vi_in.format == nullptr)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	// Source colorspace
	const ::VSFormat &   fmt_src = *_vi_in.format;

	{
		const int            st  = fmt_src.sampleType;
		const int            bps = fmt_src.bytesPerSample;
		const int            res = fmt_src.bitsPerSample;
		if (! (   (st == ::stInteger && bps == 1 &&     res ==  8 )
		       || (st == ::stInteger && bps == 2 && (   (   res >=  9
		                                                 && res <= 12)
		                                             || res == 14
		                                             || res == 16))
		       || (st == ::stFloat   && bps == 4 &&     res == 32 )))
		{
			throw_inval_arg ("input pixel bitdepth not supported.");
		}
		if (fmt_src.colorFamily == ::cmCompat)
		{
			throw_inval_arg ("\"compat\" colorspace not supported.");
		}
	}

	_splfmt_src = SplFmtUtl::conv_from_vsformat (fmt_src);

	// Destination colorspace
	const ::VSFormat& fmt_dst = get_output_colorspace (in, out, core, fmt_src);

	if (   fmt_dst.colorFamily  != fmt_src.colorFamily
	    || fmt_dst.subSamplingW != fmt_src.subSamplingW
	    || fmt_dst.subSamplingH != fmt_src.subSamplingH
	    || fmt_dst.numPlanes    != fmt_src.numPlanes)
	{
		throw_inval_arg (
			"specified output colorspace is not compatible with input."
		);
	}

	{
		const int            st  = fmt_dst.sampleType;
		const int            bps = fmt_dst.bytesPerSample;
		const int            res = fmt_dst.bitsPerSample;
		if (! (   (st == ::stInteger && bps == 1 &&     res ==  8 )
		       || (st == ::stInteger && bps == 2 && (   res ==  9
		                                             || res == 10
		                                             || res == 12
		                                             || res == 16))
		       || (st == ::stFloat   && bps == 4 &&     res == 32 )))
		{
			throw_inval_arg ("output pixel bitdepth not supported.");
		}
	}

	// Format is validated
	_vi_out.format = &fmt_dst;
	_splfmt_dst = SplFmtUtl::conv_from_vsformat (fmt_dst);

	// Conversion-related things
	bool           range_def_src_flag = false;
	_full_range_in_flag  = (get_arg_int (
		in, out, "fulls" , vsutl::is_full_range_default (fmt_src) ? 1 : 0,
		0, &range_def_src_flag
	) != 0);
	bool           range_def_dst_flag = false;
	_full_range_out_flag = (get_arg_int (
		in, out, "fulld", (_full_range_in_flag) ? 1 : 0,
		0, &range_def_dst_flag
	) != 0);
	_range_def_flag = (range_def_src_flag || range_def_dst_flag);

	// No dithering required
	if (   (   fmt_src.sampleType == ::stInteger
	        && (    fmt_dst.sampleType == ::stFloat
	            || (   fmt_src.bitsPerSample <= fmt_dst.bitsPerSample
	                && ! _full_range_in_flag
	                && ! _full_range_out_flag)))
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_dst.sampleType == ::stFloat))
	{
		_upconv_flag = true;
	}

	for (int plane_index = 0; plane_index < fmt_dst.numPlanes; ++plane_index)
	{
		SclInf &       scl_inf = _scale_info_arr [plane_index];
		vsutl::compute_fmt_mac_cst (
			scl_inf._info._gain,
			scl_inf._info._add_cst,
			*_vi_out.format, _full_range_out_flag,
			fmt_src, _full_range_in_flag,
			plane_index
		);

		if (   _upconv_flag
		    && fmt_src.sampleType == ::stInteger
		    && fmt_dst.sampleType == ::stFloat)
		{
			scl_inf._ptr = &scl_inf._info;
		}
		else
		{
			scl_inf._ptr = nullptr;
		}
	}

	// Dithering parameters
	if (_dmode == DMode_ROUND_ALIAS)
	{
		_dmode = DMode_ROUND;
	}
	if (   _dmode <  0
	    || _dmode >= DMode_NBR_ELT)
	{
		throw_inval_arg ("invalid dmode.");
	}

	if (_ampo < 0)
	{
		throw_inval_arg ("ampo cannot be negative.");
	}
	if (_ampn < 0)
	{
		throw_inval_arg ("ampn cannot be negative.");
	}

	if (_pat_size < 4 || PAT_WIDTH % _pat_size != 0)
	{
		throw_inval_arg ("Wrong value for patsize.");
	}

	int            w = _vi_in.width;
	if (_vi_in.width <= 0)
	{
		w = MAX_UNK_WIDTH;
	}
	_buf_factory_uptr =
		std::unique_ptr <fmtcl::ErrDifBufFactory> (new fmtcl::ErrDifBufFactory (w));
	_buf_pool.set_factory (*_buf_factory_uptr);

	build_dither_pat ();

	// Amplitude precalculations

	// In case of TPDF, rescales the amplitude so the power is kept constant.
	// Sum of two noises (uncorrelated signals) -> +3 dB
	if (_tpdfo_flag)
	{
		_ampo *= fstb::SQRT2 * 0.5;
	}
	if (_tpdfn_flag)
	{
		_ampn *= fstb::SQRT2 * 0.5;
	}

	const int		amp_mul = 1 << AMP_BITS;
	const int      ampo_i_raw = fstb::round_int (_ampo * amp_mul);
	const int      ampn_i_raw = fstb::round_int (_ampn * amp_mul);
	_amp._o_i = std::min (ampo_i_raw, 127);
	_amp._n_i = std::min (ampn_i_raw, 127);
	_amp._n_f = float (_ampn * (1.0f / 256.0f));

	_simple_flag = (ampo_i_raw == amp_mul && ampn_i_raw == 0);

	if (_errdif_flag)
	{
		_amp._e_i = fstb::limit (
			fstb::round_int ((_ampo - 1) * (128 << AMP_BITS)),
			0,
			(2048 << AMP_BITS) - 1
		);
		_amp._e_f = fstb::limit (float (_ampo) - 1, 0.0f, 8.0f);
	}

	// Processing function initialisation
	if (_errdif_flag)
	{
		init_fnc_errdiff ();
	}
	else if (_dmode == DMode_QUASIRND)
	{
		init_fnc_quasirandom ();
	}
	else if (_dmode == DMode_FAST)
	{
		init_fnc_fast ();
	}
	else
	{
		init_fnc_ordered ();
	}
}



void	Bitdepth::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
	_plane_processor.set_filter (in, out, _vi_out, true);
}



const ::VSFrameRef *	Bitdepth::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);

	::VSFrameRef *    dst_ptr = nullptr;
	::VSNodeRef &     node    = *_clip_src_sptr;

	if (activation_reason == ::arInitial)
	{
		_vsapi.requestFrameFilter (n, &node, &frame_ctx);
	}

	else if (activation_reason == ::arAllFramesReady)
	{
		vsutl::FrameRefSPtr	src_sptr (
			_vsapi.getFrameFilter (n, &node, &frame_ctx),
			_vsapi
		);
		const ::VSFrameRef & src = *src_sptr;

		const int      w = _vsapi.getFrameWidth (&src, 0);
		const int      h = _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		const int      ret_val = _plane_processor.process_frame (
			*dst_ptr, n, frame_data_ptr, frame_ctx, core, _clip_src_sptr
		);
		if (ret_val != 0)
		{
			_vsapi.freeFrame (dst_ptr);
			dst_ptr = nullptr;
		}

		// Output frame properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));
		if (_range_def_flag)
		{
			const int      cr_val = (_full_range_out_flag) ? 0 : 1;
			_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);
		}
	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	Bitdepth::do_process_plane (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr)
{
	fstb::unused (frame_data_ptr, core, src_node2_sptr, src_node3_sptr);
	assert (src_node1_sptr.get () != nullptr);

	int            ret_val = 0;

	const vsutl::PlaneProcMode proc_mode =
		_plane_processor.get_mode (plane_index);

	if (proc_mode == vsutl::PlaneProcMode_PROCESS)
	{
		vsutl::FrameRefSPtr	src_sptr (
			_vsapi.getFrameFilter (n, src_node1_sptr.get (), &frame_ctx),
			_vsapi
		);
		const ::VSFrameRef & src = *src_sptr;

		const int      w = _vsapi.getFrameWidth (&src, plane_index);
		const int      h = _vsapi.getFrameHeight (&src, plane_index);

		const uint8_t* data_src_ptr = _vsapi.getReadPtr (&src, plane_index);
		const int      stride_src   = _vsapi.getStride (&src, plane_index);
		uint8_t *      data_dst_ptr = _vsapi.getWritePtr (&dst, plane_index);
		const int      stride_dst   = _vsapi.getStride (&dst, plane_index);

		try
		{
			if (_upconv_flag)
			{
				fmtcl::BitBltConv blitter (_sse2_flag, _avx2_flag);
				blitter.bitblt (
					_splfmt_dst, _vi_out.format->bitsPerSample,
					data_dst_ptr, nullptr, stride_dst,
					_splfmt_src, _vi_in.format->bitsPerSample,
					data_src_ptr, nullptr, stride_src,
					w, h,
					_scale_info_arr [plane_index]._ptr
				);
			}
			else
			{
				dither_plane (
					_splfmt_dst, _vi_out.format->bitsPerSample,
					data_dst_ptr, stride_dst,
					_splfmt_src, _vi_in.format->bitsPerSample,
					data_src_ptr, stride_src,
					w, h,
					_scale_info_arr [plane_index]._info,
					n, plane_index
				);
			}
		}

		catch (std::exception &e)
		{
			_vsapi.setFilterError (e.what (), &frame_ctx);
			ret_val = -1;
		}
		catch (...)
		{
			_vsapi.setFilterError ("bitdepth: exception.", &frame_ctx);
			ret_val = -1;
		}
	}

	return (ret_val);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat &	Bitdepth::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const
{
	const ::VSFormat *   fmt_dst_ptr = &fmt_src;

	const int      undef    = -666666666;
	const int      dst_csp  = get_arg_int (in, out, "csp" , undef);
	const int      dst_flt  = get_arg_int (in, out, "flt" , undef);
	const int      dst_bits = get_arg_int (in, out, "bits", undef);

	if ((dst_flt != undef || dst_bits != undef) && dst_csp != undef)
	{
		throw_inval_arg (
			"you cannot specify both a colorspace and a pixel format."
		);
	}

	// Full colorspace
	if (dst_csp != undef)
	{
		fmt_dst_ptr = _vsapi.getFormatPreset (dst_csp, &core);
		if (fmt_dst_ptr == nullptr)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
	}

	else
	{
		int            col_fam  = fmt_dst_ptr->colorFamily;
		int            spl_type = fmt_dst_ptr->sampleType;
		int            bits     = fmt_dst_ptr->bitsPerSample;
		int            ssh      = fmt_dst_ptr->subSamplingW;
		int            ssv      = fmt_dst_ptr->subSamplingH;

		// Data type
		if (dst_flt == 0)
		{
			spl_type = ::stInteger;
		}
		else if (dst_flt != undef)
		{
			spl_type = ::stFloat;
			if (dst_bits == undef)
			{
				bits = 32;
			}
		}

		// Bitdepth
		if (dst_bits != undef)
		{
			bits = dst_bits;
			if (dst_flt == undef)
			{
				if (bits < 32)
				{
					spl_type = ::stInteger;
				}
				else
				{
					spl_type = ::stFloat;
				}
			}
		}

		// Combines the modified parameters and validates the format
		try
		{
			fmt_dst_ptr = register_format (
				col_fam,
				spl_type,
				bits,
				ssh,
				ssv,
				core
			);
		}
		catch (...)
		{
			fmt_dst_ptr = nullptr;
		}
		if (fmt_dst_ptr == nullptr)
		{
			throw_rt_err (
				"couldn\'t get a pixel format identifier for the output clip."
			);
		}
	}

	return (*fmt_dst_ptr);
}



void	Bitdepth::build_dither_pat ()
{
	_errdif_flag = false;

	switch (_dmode)
	{
	case DMode_BAYER:
		build_dither_pat_bayer ();
		break;

	case DMode_FILTERLITE:
	case DMode_STUCKI:
	case DMode_ATKINSON:
	case DMode_FLOYD:
	case DMode_OSTRO:
		_errdif_flag = true;
		_tpdfo_flag  = false;
		break;

	case DMode_ROUND:
	case DMode_FAST:
	default:
		build_dither_pat_round ();
		break;

	case DMode_VOIDCLUST:
		build_dither_pat_void_and_cluster (_pat_size);
		break;

	case DMode_QUASIRND:
		// Nothing
		break;
	}
}



void	Bitdepth::build_dither_pat_round ()
{
	PatData &      pat_data = _dither_pat_arr [0];
	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			pat_data [y] [x] = 0;
		}
	}

	build_next_dither_pat ();
}



void	Bitdepth::build_dither_pat_bayer ()
{
	assert (fstb::is_pow_2 (int (PAT_WIDTH)));

	PatData &      pat_data = _dither_pat_arr [0];
	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			pat_data [y] [x] = -128;
		}
	}

	for (int dith_size = 2; dith_size <= PAT_WIDTH; dith_size <<= 1)
	{
		for (int y = 0; y < PAT_WIDTH; y += 2)
		{
			for (int x = 0; x < PAT_WIDTH; x += 2)
			{
				const int      xx = (x >> 1) + (PAT_WIDTH >> 1);
				const int      yy = (y >> 1) + (PAT_WIDTH >> 1);
				const int      val = (pat_data [yy] [xx] + 128) >> 2;
				pat_data [y    ] [x    ] = int16_t (val +   0-128);
				pat_data [y    ] [x + 1] = int16_t (val + 128-128);
				pat_data [y + 1] [x    ] = int16_t (val + 192-128);
				pat_data [y + 1] [x + 1] = int16_t (val +  64-128);
			}
		}
	}

	build_next_dither_pat ();
}



void	Bitdepth::build_dither_pat_void_and_cluster (int w)
{
	assert (PAT_WIDTH % w == 0);
	fmtcl::VoidAndCluster   vc_gen;
	fmtcl::MatrixWrap <uint16_t> pat_raw (w, w);
	vc_gen.create_matrix (pat_raw);

	PatData &      pat_data = _dither_pat_arr [0];
	const int      area = w * w;
	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			pat_data [y] [x] = int16_t (pat_raw (x, y) * 256 / area - 128);
		}
	}

	build_next_dither_pat ();
}



void	Bitdepth::build_next_dither_pat ()
{
	if (_tpdfo_flag)
	{
		for (int y = 0; y < PAT_WIDTH; ++y)
		{
			for (int x = 0; x < PAT_WIDTH; ++x)
			{
				const int      r = _dither_pat_arr [0] [y] [x];
				const int      t = remap_tpdf_scalar (r);
				_dither_pat_arr [0] [y] [x] = int16_t (t);
			}
		}
	}

	for (int seq = 1; seq < PAT_PERIOD; ++seq)
	{
		const int      angle = (_dyn_flag) ? seq & 3 : 0;
		copy_dither_pat_rotate (
			_dither_pat_arr [seq],
			_dither_pat_arr [0],
			angle
		);
	}
}



void	Bitdepth::copy_dither_pat_rotate (PatData &dst, const PatData &src, int angle) noexcept
{
	assert (angle >= 0);
	assert (angle < 4);

	static const int  sin_arr [4] = { 0, 1, 0, -1 };
	const int      s = sin_arr [ angle         ];
	const int      c = sin_arr [(angle + 1) & 3];

	assert (fstb::is_pow_2 (int (PAT_WIDTH)));
	const int		mask = PAT_WIDTH - 1;

	for (int y = 0; y < PAT_WIDTH; ++y)
	{
		for (int x = 0; x < PAT_WIDTH; ++x)
		{
			const int		xs = (x * c - y * s) & mask;
			const int		ys = (x * s + y * c) & mask;

			dst [y] [x] = src [ys] [xs];
		}
	}
}



// All possible combinations
#define fmtc_Bitdepth_SPAN_INT(SETP, NAMP, NAMF, simple_flag, tpdfo_flag, tpdfn_flag, dst_res, dst_fmt, src_res, src_fmt) \
	switch (  ((simple_flag) << 7) \
	        + ((tpdfo_flag) << 23) + ((tpdfn_flag) << 22) \
	        + ((dst_res) << 24) + ((dst_fmt) << 16) \
	        + ((src_res) <<  8) +  (src_fmt)) \
	{ \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t,  9) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 16) \
	}

// All possible combinations using float as intermediary data
#define fmtc_Bitdepth_SPAN_FLT(SETP, NAMP, NAMF, simple_flag, tpdfo_flag, tpdfn_flag, dst_res, dst_fmt, src_res, src_fmt) \
	switch (  ((simple_flag) << 7) \
	        + ((tpdfo_flag) << 23) + ((tpdfn_flag) << 22) \
	        + ((dst_res) << 24) + ((dst_fmt) << 16) \
	        + ((src_res) <<  8) +  (src_fmt)) \
	{ \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT8 , uint8_t ,  8) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t,  9) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT8 , uint8_t ,  8, fmtcl::SplFmt_FLOAT, float   , 32) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT8 , uint8_t ,  8) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t,  9) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t,  9, fmtcl::SplFmt_FLOAT, float   , 32) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT8 , uint8_t ,  8) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t,  9) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 10, fmtcl::SplFmt_FLOAT, float   , 32) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT8 , uint8_t ,  8) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t,  9) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 12, fmtcl::SplFmt_FLOAT, float   , 32) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT8 , uint8_t ,  8) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT16, uint16_t,  9) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT16, uint16_t, 10) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT16, uint16_t, 11) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT16, uint16_t, 12) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT16, uint16_t, 14) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_INT16, uint16_t, 16) \
	SETP (NAMP, NAMF, fmtcl::SplFmt_INT16, uint16_t, 16, fmtcl::SplFmt_FLOAT, float   , 32) \
	}



#define fmtc_Bitdepth_SET_FNC_MULTI(FCASE, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (false, false, false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (false, false, true , NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (false, true , false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (false, true , true , NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (true , false, false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (true , false, true , NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (true , true , false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	FCASE (true , true , true , NAMP, NAMF, DF, DT, DP, SF, ST, SP)

#define fmtc_Bitdepth_SET_FNC_INT_CASE(simple_flag, tpdfo_flag, tpdfn_flag, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	case   (simple_flag << 7) + (tpdfn_flag << 22) + (tpdfo_flag << 23) \
	     + (DP << 24) + (DF << 16) + (SP << 8) + SF: \
		_process_seg_int_int_ptr = &ThisType::process_seg_##NAMF##_int_int_cpp < \
			simple_flag, tpdfo_flag, tpdfn_flag, DT, DP, ST, SP \
		>; \
		break;

#define fmtc_Bitdepth_SET_FNC_INT(NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_MULTI (fmtc_Bitdepth_SET_FNC_INT_CASE, \
		NAMP, NAMF, DF, DT, DP, SF, ST, SP)

#define fmtc_Bitdepth_SET_FNC_FLT_CASE(simple_flag, tpdfo_flag, tpdfn_flag,NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	case   (simple_flag << 7) + (tpdfn_flag << 22) + (tpdfo_flag << 23) \
	     + (DP << 24) + (DF << 16) + (SP << 8) + SF: \
		_process_seg_flt_int_ptr = &ThisType::process_seg_##NAMF##_flt_int_cpp < \
			simple_flag, tpdfo_flag, tpdfn_flag, DT, DP, ST \
		>; \
		break;

#define fmtc_Bitdepth_SET_FNC_FLT(NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_MULTI (fmtc_Bitdepth_SET_FNC_FLT_CASE, \
		NAMP, NAMF, DF, DT, DP, SF, ST, SP)

#define fmtc_Bitdepth_SET_FNC_INT_SSE2_CASE(simple_flag, tpdfo_flag, tpdfn_flag, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	case   (simple_flag << 7) + (tpdfn_flag << 22) + (tpdfo_flag << 23) \
	     + (DP << 24) + (DF << 16) + (SP << 8) + SF: \
		_process_seg_int_int_ptr = &ThisType::process_seg_##NAMF##_int_int_sse2 < \
			simple_flag, tpdfo_flag, tpdfn_flag, DF, DP, SF, SP \
		>; \
		break;

#define fmtc_Bitdepth_SET_FNC_INT_SSE2(NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_MULTI (fmtc_Bitdepth_SET_FNC_INT_SSE2_CASE, \
		NAMP, NAMF, DF, DT, DP, SF, ST, SP)

#define fmtc_Bitdepth_SET_FNC_FLT_SSE2_CASE(simple_flag, tpdfo_flag, tpdfn_flag, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	case   (simple_flag << 7) + (tpdfn_flag << 22) + (tpdfo_flag << 23) \
	     + (DP << 24) + (DF << 16) + (SP << 8) + SF: \
		_process_seg_flt_int_ptr = &ThisType::process_seg_##NAMF##_flt_int_sse2 < \
			simple_flag, tpdfo_flag, tpdfn_flag, DF, DP, SF \
		>; \
		break;

#define fmtc_Bitdepth_SET_FNC_FLT_SSE2(NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_MULTI (fmtc_Bitdepth_SET_FNC_FLT_SSE2_CASE, \
		NAMP, NAMF, DF, DT, DP, SF, ST, SP)



void	Bitdepth::init_fnc_fast () noexcept
{
	const fmtcl::SplFmt  dst_fmt = _splfmt_dst;
	const int            dst_res = _vi_out.format->bitsPerSample;
	const fmtcl::SplFmt  src_fmt = _splfmt_src;
	const int            src_res = _vi_in.format->bitsPerSample;

	fmtc_Bitdepth_SPAN_INT (
		fmtc_Bitdepth_SET_FNC_INT, fast, fast, false, false, false,
		dst_res, dst_fmt, src_res, src_fmt
	)
	fmtc_Bitdepth_SPAN_FLT (
		fmtc_Bitdepth_SET_FNC_FLT, fast, fast, false, false, false,
		dst_res, dst_fmt, src_res, src_fmt
	)

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (_sse2_flag)
	{
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_INT_SSE2, fast, fast, false, false, false,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_FLT_SSE2, fast, fast, false, false, false,
			dst_res, dst_fmt, src_res, src_fmt
		)
	}
#endif
}



void	Bitdepth::init_fnc_ordered () noexcept
{
	assert (! _errdif_flag);

	const fmtcl::SplFmt  dst_fmt = _splfmt_dst;
	const int            dst_res = _vi_out.format->bitsPerSample;
	const fmtcl::SplFmt  src_fmt = _splfmt_src;
	const int            src_res = _vi_in.format->bitsPerSample;

	fmtc_Bitdepth_SPAN_INT (
		fmtc_Bitdepth_SET_FNC_INT,
		ord, ord, _simple_flag, _tpdfo_flag, _tpdfn_flag,
		dst_res, dst_fmt, src_res, src_fmt
	)
	fmtc_Bitdepth_SPAN_FLT (
		fmtc_Bitdepth_SET_FNC_FLT,
		ord, ord, _simple_flag, _tpdfo_flag, _tpdfn_flag,
		dst_res, dst_fmt, src_res, src_fmt
	)

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (_sse2_flag)
	{
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_INT_SSE2,
			ord, ord, _simple_flag, _tpdfo_flag, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_FLT_SSE2,
			ord, ord, _simple_flag, _tpdfo_flag, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
	}
#endif
}



void	Bitdepth::init_fnc_quasirandom () noexcept
{
	assert (! _errdif_flag);

	const fmtcl::SplFmt  dst_fmt = _splfmt_dst;
	const int            dst_res = _vi_out.format->bitsPerSample;
	const fmtcl::SplFmt  src_fmt = _splfmt_src;
	const int            src_res = _vi_in.format->bitsPerSample;

	fmtc_Bitdepth_SPAN_INT (
		fmtc_Bitdepth_SET_FNC_INT,
		qrs, qrs, _simple_flag, _tpdfo_flag, _tpdfn_flag,
		dst_res, dst_fmt, src_res, src_fmt
	)
	fmtc_Bitdepth_SPAN_FLT (
		fmtc_Bitdepth_SET_FNC_FLT,
		qrs, qrs, _simple_flag, _tpdfo_flag, _tpdfn_flag,
		dst_res, dst_fmt, src_res, src_fmt
	)

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (_sse2_flag)
	{
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_INT_SSE2,
			qrs, qrs, _simple_flag, _tpdfo_flag, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_FLT_SSE2,
			qrs, qrs, _simple_flag, _tpdfo_flag, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
	}
#endif
}



#undef fmtc_Bitdepth_SET_FNC_MULTI
#undef fmtc_Bitdepth_SET_FNC_INT_CASE
#undef fmtc_Bitdepth_SET_FNC_INT
#undef fmtc_Bitdepth_SET_FNC_FLT_CASE
#undef fmtc_Bitdepth_SET_FNC_FLT
#undef fmtc_Bitdepth_SET_FNC_INT_SSE2_CASE
#undef fmtc_Bitdepth_SET_FNC_INT_SSE2
#undef fmtc_Bitdepth_SET_FNC_FLT_SSE2_CASE
#undef fmtc_Bitdepth_SET_FNC_FLT_SSE2



#define fmtc_Bitdepth_SET_FNC_ERRDIF_INT_CASE(simple_flag, tpdfn_flag, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	case   (simple_flag << 7) + (tpdfn_flag << 22) \
	     + (DP << 24) + (DF << 16) + (SP << 8) + SF: \
		_process_seg_int_int_ptr = &ThisType::process_seg_errdif_int_int_cpp < \
			simple_flag, tpdfn_flag, Diffuse##NAMF <DT, DP, ST, SP> \
		>; \
		break;

#define fmtc_Bitdepth_SET_FNC_ERRDIF_INT(NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_INT_CASE (false, false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_INT_CASE (false, true , NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_INT_CASE (true , false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_INT_CASE (true , true , NAMP, NAMF, DF, DT, DP, SF, ST, SP)

#define fmtc_Bitdepth_SET_FNC_ERRDIF_FLT_CASE(simple_flag, tpdfn_flag, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	case   (simple_flag << 7) + (tpdfn_flag << 22) \
	     + (DP << 24) + (DF << 16) + (SP << 8) + SF: \
		_process_seg_flt_int_ptr = &ThisType::process_seg_errdif_flt_int_cpp < \
			simple_flag, tpdfn_flag, Diffuse##NAMF <DT, DP, ST, SP> \
		>; \
		break;

#define fmtc_Bitdepth_SET_FNC_ERRDIF_FLT(NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_FLT_CASE (false, false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_FLT_CASE (false, true , NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_FLT_CASE (true , false, NAMP, NAMF, DF, DT, DP, SF, ST, SP) \
	fmtc_Bitdepth_SET_FNC_ERRDIF_FLT_CASE (true , true , NAMP, NAMF, DF, DT, DP, SF, ST, SP)



void	Bitdepth::init_fnc_errdiff () noexcept
{
	assert (_errdif_flag);

	const fmtcl::SplFmt  dst_fmt = _splfmt_dst;
	const int            dst_res = _vi_out.format->bitsPerSample;
	const fmtcl::SplFmt  src_fmt = _splfmt_src;
	const int            src_res = _vi_in.format->bitsPerSample;

	switch (_dmode)
	{
	case DMode_FILTERLITE:
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_INT,
			errdif, FilterLite, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_FLT,
			errdif, FilterLite, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		break;

	case DMode_STUCKI:
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_INT,
			errdif, Stucki, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_FLT,
			errdif, Stucki, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		break;

	case DMode_ATKINSON:
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_INT,
			errdif, Atkinson, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_FLT,
			errdif, Atkinson, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		break;

	case DMode_FLOYD:
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_INT,
			errdif, FloydSteinberg, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_FLT,
			errdif, FloydSteinberg, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		break;

	case DMode_OSTRO:
		fmtc_Bitdepth_SPAN_INT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_INT,
			errdif, Ostromoukhov, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		fmtc_Bitdepth_SPAN_FLT (
			fmtc_Bitdepth_SET_FNC_ERRDIF_FLT,
			errdif, Ostromoukhov, _simple_flag, false, _tpdfn_flag,
			dst_res, dst_fmt, src_res, src_fmt
		)
		break;

	default:
		break;
	}
}



#undef fmtc_Bitdepth_SET_FNC_ERRDIF_INT_CASE
#undef fmtc_Bitdepth_SET_FNC_ERRDIF_INT
#undef fmtc_Bitdepth_SET_FNC_ERRDIF_FLT_CASE
#undef fmtc_Bitdepth_SET_FNC_ERRDIF_FLT



#undef fmtc_Bitdepth_SPAN_INT
#undef fmtc_Bitdepth_SPAN_FLT



void	Bitdepth::dither_plane (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, int src_stride, int w, int h, const fmtcl::BitBltConv::ScaleInfo &scale_info, int frame_index, int plane_index)
{
	fstb::unused (dst_fmt);
	assert (dst_fmt >= 0);
	assert (dst_fmt < fmtcl::SplFmt_NBR_ELT);
	assert (dst_res >= 8);
	assert (dst_ptr != nullptr);
	assert (src_fmt >= 0);
	assert (src_fmt < fmtcl::SplFmt_NBR_ELT);
	assert (src_res >= 8);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (h > 0);

	SegContext     ctx;
	ctx._scale_info_ptr = &scale_info;
	ctx._amp            = _amp;

	uint32_t       rnd_state = 0;
	if (! _correlated_planes_flag)
	{
		rnd_state += plane_index << 16;
	}
	if (_static_noise_flag)
	{
		rnd_state += 55555;
	}
	else
	{
		rnd_state += frame_index;
	}
	ctx._rnd_state = rnd_state;

	const bool     sc_flag =
		(   src_fmt == fmtcl::SplFmt_FLOAT
		 || ! fstb::is_eq (scale_info._gain * double ((uint64_t (1)) << (src_res - dst_res)), 1.0, 1e-6)
		 || ! fstb::is_null (scale_info._add_cst, 1e-6));

	void (* process_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, SegContext &ctx) =
		  (sc_flag)
		? _process_seg_flt_int_ptr
		: _process_seg_int_int_ptr;
	assert (process_ptr != nullptr);

	fmtcl::ErrDifBuf *   ed_buf_ptr = nullptr;
	if (_errdif_flag)
	{
		ed_buf_ptr = _buf_pool.take_obj ();
		if (ed_buf_ptr == nullptr)
		{
			throw_rt_err ("cannot allocate memory for temporary buffer.");
		}
		ed_buf_ptr->clear ((sc_flag) ? sizeof (float) : sizeof (int16_t));
	}

	switch (_dmode)
	{
	case DMode_BAYER:
	case DMode_ROUND:
	case DMode_VOIDCLUST:
		{
			int            pat_index = 0;
			if (! _correlated_planes_flag)
			{
				pat_index += plane_index;
			}
			if (_dyn_flag)
			{
				pat_index += frame_index;
			}
			pat_index &= PAT_PERIOD - 1;
			const PatData& pattern   = _dither_pat_arr [pat_index];
			ctx._pattern_ptr = &pattern;
		}
		break;

	case DMode_FAST:
		// Nothing
		break;

	case DMode_QUASIRND:
		ctx._qrs_seed = 0;
		if (_dyn_flag)
		{
			ctx._qrs_seed += uint32_t (frame_index * 73);
		}
		if (! _correlated_planes_flag)
		{
			ctx._qrs_seed += uint32_t (plane_index * 263);
		}
		break;

	case DMode_FILTERLITE:
	case DMode_STUCKI:
	case DMode_ATKINSON:
	case DMode_FLOYD:
	case DMode_OSTRO:
		ctx._ed_buf_ptr = ed_buf_ptr;
		break;

	default:
		assert (false);
		throw_logic_err ("unexpected dithering algorithm");
		break;
	}

	for (int y = 0; y < h; ++y)
	{
		ctx._y = y;

		(*process_ptr) (dst_ptr, src_ptr, w, ctx);

		src_ptr += src_stride;
		dst_ptr += dst_stride;
	}

	if (ed_buf_ptr != nullptr)
	{
		_buf_pool.return_obj (*ed_buf_ptr);
		ed_buf_ptr = nullptr;
	}
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
void	Bitdepth::process_seg_fast_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	fstb::unused (ctx);

	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);

	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;
	static_assert (DIF_BITS >= 0, "This function cannot increase bidepth.");

	const SRC_TYPE * fstb_RESTRICT src_n_ptr = reinterpret_cast <const SRC_TYPE *> (src_ptr);
	DST_TYPE * fstb_RESTRICT       dst_n_ptr = reinterpret_cast <      DST_TYPE *> (dst_ptr);

	for (int pos = 0; pos < w; ++pos)
	{
		const int      s   = src_n_ptr [pos];
		const int      pix = s >> DIF_BITS;
		dst_n_ptr [pos] = static_cast <DST_TYPE> (pix);
	}
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
void	Bitdepth::process_seg_fast_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (ctx._scale_info_ptr != nullptr);

	const SRC_TYPE * fstb_RESTRICT src_n_ptr = reinterpret_cast <const SRC_TYPE *> (src_ptr);
	DST_TYPE * fstb_RESTRICT       dst_n_ptr = reinterpret_cast <      DST_TYPE *> (dst_ptr);

	const float    mul  = float (ctx._scale_info_ptr->_gain);
	const float    add  = float (ctx._scale_info_ptr->_add_cst);
	const int      vmax = (1 << DST_BITS) - 1;

	for (int pos = 0; pos < w; ++pos)
	{
		float          s = float (src_n_ptr [pos]);
		s = s * mul + add;
		const int      quant = fstb::conv_int_fast (s);
		const int      pix   = fstb::limit (quant, 0, vmax);
		dst_n_ptr [pos] = static_cast <DST_TYPE> (pix);
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
void	Bitdepth::process_seg_fast_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	fstb::unused (ctx);
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);

	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;
	static_assert (DIF_BITS >= 0, "This function cannot increase bidepth.");

	typedef typename  fmtcl::ProxyRwSse2 <SRC_FMT>::PtrConst::Type SrcPtr;
	typedef typename  fmtcl::ProxyRwSse2 <DST_FMT>::Ptr::Type      DstPtr;
	SrcPtr         src_n_ptr = reinterpret_cast <SrcPtr> (src_ptr);
	DstPtr         dst_n_ptr = reinterpret_cast <DstPtr> (dst_ptr);
	const __m128i  zero      = _mm_setzero_si128 ();
	const __m128i  mask_lsb  = _mm_set1_epi16 (0x00FF);

	for (int pos = 0; pos < w; pos += 8)
	{
		const __m128i  s   =
			fmtcl::ProxyRwSse2 <SRC_FMT>::read_i16 (src_n_ptr + pos, zero);
		const __m128i  pix = _mm_srli_epi16 (s, DIF_BITS);
		fmtcl::ProxyRwSse2 <DST_FMT>::write_i16 (dst_n_ptr + pos, pix, mask_lsb);
	}
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
void	Bitdepth::process_seg_fast_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (ctx._scale_info_ptr != nullptr);

	typedef typename  fmtcl::ProxyRwSse2 <SRC_FMT>::PtrConst::Type  SrcPtr;
	typedef typename  fmtcl::ProxyRwSse2 <DST_FMT>::Ptr::Type       DstPtr;
	SrcPtr         src_n_ptr = reinterpret_cast <SrcPtr> (src_ptr);
	DstPtr         dst_n_ptr = reinterpret_cast <DstPtr> (dst_ptr);

	const __m128   mul      = _mm_set1_ps (float (ctx._scale_info_ptr->_gain));
	const __m128   add      = _mm_set1_ps (float (ctx._scale_info_ptr->_add_cst));
	const __m128   vmax     = _mm_set1_ps (float ((1 << DST_BITS) - 1));
	const __m128   zero_f   = _mm_setzero_ps ();
	const __m128i  zero_i   = _mm_setzero_si128 ();
	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit = _mm_set1_epi16 (-0x8000);
	const __m128   offset   = _mm_set1_ps (-32768);

	for (int pos = 0; pos < w; pos += 8)
	{
		__m128         s0;
		__m128         s1;
		fmtcl::ProxyRwSse2 <SRC_FMT>::read_flt (
			src_n_ptr + pos, s0, s1, zero_i
		);
		s0 = _mm_add_ps (_mm_mul_ps (s0, mul), add);
		s1 = _mm_add_ps (_mm_mul_ps (s1, mul), add);
		s0 = _mm_max_ps (_mm_min_ps (s0, vmax), zero_f);
		s1 = _mm_max_ps (_mm_min_ps (s1, vmax), zero_f);
		fmtcl::ProxyRwSse2 <DST_FMT>::write_flt (
			dst_n_ptr + pos, s0, s1, mask_lsb, sign_bit, offset
		);
	}
}



#endif



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
void	Bitdepth::process_seg_ord_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	const PatRow & fstb_RESTRICT  pattern = ctx.extract_pattern_row ();

	process_seg_common_int_int_cpp <
		S_FLAG, TN_FLAG, DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int pos)
		{
			return pattern [pos & (PAT_WIDTH - 1)];
		}
	);
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
void	Bitdepth::process_seg_ord_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	const PatRow & fstb_RESTRICT  pattern = ctx.extract_pattern_row ();

	process_seg_common_flt_int_cpp <
		S_FLAG, TN_FLAG, DST_TYPE, DST_BITS, SRC_TYPE
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int pos)
		{
			return pattern [pos & (PAT_WIDTH - 1)];
		}
	);
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
void	Bitdepth::process_seg_ord_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	const PatRow & fstb_RESTRICT  pattern = ctx.extract_pattern_row ();

	process_seg_common_int_int_sse2 <
		S_FLAG, TN_FLAG, DST_FMT, DST_BITS, SRC_FMT, SRC_BITS
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int pos)
		{
			return _mm_load_si128 (reinterpret_cast <const __m128i *> (
				&pattern [pos & (PAT_WIDTH - 1)]
			)); // 8 s16 [-128 ; +127]
		}
	);
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
void	Bitdepth::process_seg_ord_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	const PatRow & fstb_RESTRICT  pattern = ctx.extract_pattern_row ();

	process_seg_common_flt_int_sse2 <
		S_FLAG, TN_FLAG, DST_FMT, DST_BITS, SRC_FMT
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int pos)
		{
			return _mm_load_si128 (reinterpret_cast <const __m128i *> (
				&pattern [pos & (PAT_WIDTH - 1)]
			)); // 8 s16 [-128 ; +127]
		}
	);
}



#endif   // fstb_ARCHI_X86



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
void	Bitdepth::process_seg_qrs_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	// alpha1 = 1 / x, with x real solution of: x^3 - x - 1 = 0
	// Also:
	// alpha1 =   (curt (2) * sq (curt (3)))
	//          / (curt (9 - sqrt (69)) + curt (9 + sqrt (69)))
	constexpr double  alpha1  = 1.0 / 1.3247179572447460259609088544781;
	constexpr double  alpha2  = alpha1 * alpha1;
	constexpr int     sc_l2   = 16; // 16 bits of fractional values
	constexpr float   sc_mul  = float (1 << sc_l2);
	constexpr int     qrs_shf = sc_l2 - 9;
	constexpr int     qrs_inc = int (alpha1 * sc_mul + 0.5f);
	uint32_t          qrs_cnt = uint32_t (std::llrint (
		(alpha2 * double (ctx._y + ctx._qrs_seed)) * sc_mul
	));

	process_seg_common_int_int_cpp <
		S_FLAG, TN_FLAG, DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int /*pos*/)
		{
			const int      p      = (qrs_cnt >> qrs_shf) & 0x1FF;
			int            dith_o = (p > 255) ? 512 - 128 - p : p - 128; // s8
			qrs_cnt += qrs_inc;

			if (TO_FLAG)
			{
				dith_o = remap_tpdf_scalar (dith_o);
			}

			return dith_o;
		}
	);
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
void	Bitdepth::process_seg_qrs_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	// alpha1 = 1 / x, with x real solution of: x^3 - x - 1 = 0
	// Also:
	// alpha1 =   (curt (2) * sq (curt (3)))
	//          / (curt (9 - sqrt (69)) + curt (9 + sqrt (69)))
	constexpr double  alpha1  = 1.0 / 1.3247179572447460259609088544781;
	constexpr double  alpha2  = alpha1 * alpha1;
	constexpr int     sc_l2   = 16; // 16 bits of fractional values
	constexpr float   sc_mul  = float (1 << sc_l2);
	constexpr int     qrs_shf = sc_l2 - 9;
	constexpr int     qrs_inc = int (alpha1 * sc_mul + 0.5f);
	uint32_t          qrs_cnt = uint32_t (std::llrint (
		(alpha2 * double (ctx._y + ctx._qrs_seed)) * sc_mul
	));

	process_seg_common_flt_int_cpp <
		S_FLAG, TN_FLAG, DST_TYPE, DST_BITS, SRC_TYPE
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int /*pos*/)
		{
			const int      p      = (qrs_cnt >> qrs_shf) & 0x1FF;
			int            dith_o = (p > 255) ? 512 - 128 - p : p - 128; // s8
			qrs_cnt += qrs_inc;

			if (TO_FLAG)
			{
				dith_o = remap_tpdf_scalar (dith_o);
			}

			return dith_o;
		}
	);
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
void	Bitdepth::process_seg_qrs_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	// alpha1 = 1 / x, with x real solution of: x^3 - x - 1 = 0
	// Also:
	// alpha1 =   (curt (2) * sq (curt (3)))
	//          / (curt (9 - sqrt (69)) + curt (9 + sqrt (69)))
	constexpr double  alpha1  = 1.0 / 1.3247179572447460259609088544781;
	constexpr double  alpha2  = alpha1 * alpha1;
	constexpr int     sc_l2   = 16; // 16 bits of fractional values
	constexpr float   sc_mul  = float (1 << sc_l2);
	constexpr int     qrs_shf = sc_l2 - 9;
	constexpr int     qrs_inc = int (alpha1 * sc_mul + 0.5f);
	uint32_t          qrs_cnt = uint32_t (std::llrint (
		(alpha2 * double (ctx._y + ctx._qrs_seed)) * sc_mul
	));

	const __m128i     qrs_inc_4 = _mm_set1_epi32 (4 * qrs_inc);
	__m128i           qrs_cnt_4 = _mm_set1_epi32 (qrs_cnt);
	const __m128i     qrs_ofs   = _mm_set_epi32 (qrs_inc * 3, qrs_inc * 2, qrs_inc, 0);
	qrs_cnt_4 = _mm_add_epi32 (qrs_cnt_4, qrs_ofs);
	const __m128i     qrs_msk   = _mm_set1_epi32 (0x1FF);
	const __m128i     c128      = _mm_set1_epi16 (128);
	const __m128i     c256      = _mm_set1_epi16 (256);
	const __m128i     c384      = _mm_set1_epi16 (384);

	process_seg_common_int_int_sse2 <
		S_FLAG, TN_FLAG, DST_FMT, DST_BITS, SRC_FMT, SRC_BITS
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int /*pos*/)
		{
			auto           p03    = _mm_srli_epi32 (qrs_cnt_4, qrs_shf);
			p03 = _mm_and_si128 (p03, qrs_msk);
			qrs_cnt_4 = _mm_add_epi32 (qrs_cnt_4, qrs_inc_4);
			auto           p47    = _mm_srli_epi32 (qrs_cnt_4, qrs_shf);
			p47 = _mm_and_si128 (p47, qrs_msk);
			qrs_cnt_4 = _mm_add_epi32 (qrs_cnt_4, qrs_inc_4);
			const auto     p      = _mm_packs_epi32 (p03, p47);
			const auto     tri_a  = _mm_sub_epi16 (p, c128);
			const auto     tri_d  = _mm_sub_epi16 (c384, p);
			const auto     cond   = _mm_cmplt_epi16 (p, c256);
			auto           dith_o = _mm_or_si128 (
				_mm_and_si128 (cond, tri_a),
				_mm_andnot_si128 (cond, tri_d)
			);

			if (TO_FLAG)
			{
				dith_o = remap_tpdf_vec (dith_o);
			}

			return dith_o; // 8 s16 [-128 ; +127] or [-256 ; +255]
		}
	);
}



template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
void	Bitdepth::process_seg_qrs_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	// alpha1 = 1 / x, with x real solution of: x^3 - x - 1 = 0
	// Also:
	// alpha1 =   (curt (2) * sq (curt (3)))
	//          / (curt (9 - sqrt (69)) + curt (9 + sqrt (69)))
	constexpr double  alpha1  = 1.0 / 1.3247179572447460259609088544781;
	constexpr double  alpha2  = alpha1 * alpha1;
	constexpr int     sc_l2   = 16; // 16 bits of fractional values
	constexpr float   sc_mul  = float (1 << sc_l2);
	constexpr int     qrs_shf = sc_l2 - 9;
	constexpr int     qrs_inc = int (alpha1 * sc_mul + 0.5f);
	uint32_t          qrs_cnt = uint32_t (std::llrint (
		(alpha2 * double (ctx._y + ctx._qrs_seed)) * sc_mul
	));

	const __m128i     qrs_inc_4 = _mm_set1_epi32 (4 * qrs_inc);
	__m128i           qrs_cnt_4 = _mm_set1_epi32 (qrs_cnt);
	const __m128i     qrs_ofs   = _mm_set_epi32 (qrs_inc * 3, qrs_inc * 2, qrs_inc, 0);
	qrs_cnt_4 = _mm_add_epi32 (qrs_cnt_4, qrs_ofs);
	const __m128i     qrs_msk   = _mm_set1_epi32 (0x1FF);
	const __m128i     c128      = _mm_set1_epi16 (128);
	const __m128i     c256      = _mm_set1_epi16 (256);
	const __m128i     c384      = _mm_set1_epi16 (384);

	process_seg_common_flt_int_sse2 <
		S_FLAG, TN_FLAG, DST_FMT, DST_BITS, SRC_FMT
	> (dst_ptr, src_ptr, w, ctx,
		[&] (int /*pos*/)
		{
			auto           p03    = _mm_srli_epi32 (qrs_cnt_4, qrs_shf);
			p03 = _mm_and_si128 (p03, qrs_msk);
			qrs_cnt_4 = _mm_add_epi32 (qrs_cnt_4, qrs_inc_4);
			auto           p47    = _mm_srli_epi32 (qrs_cnt_4, qrs_shf);
			p47 = _mm_and_si128 (p47, qrs_msk);
			qrs_cnt_4 = _mm_add_epi32 (qrs_cnt_4, qrs_inc_4);
			const auto     p      = _mm_packs_epi32 (p03, p47);
			const auto     tri_a  = _mm_sub_epi16 (p, c128);
			const auto     tri_d  = _mm_sub_epi16 (c384, p);
			const auto     cond   = _mm_cmplt_epi16 (p, c256);
			auto           dith_o = _mm_or_si128 (
				_mm_and_si128 (cond, tri_a),
				_mm_andnot_si128 (cond, tri_d)
			);

			if (TO_FLAG)
			{
				dith_o = remap_tpdf_vec (dith_o);
			}

			return dith_o; // 8 s16 [-128 ; +127]
		}
	);
}



#endif   // fstb_ARCHI_X86



template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS, typename DFNC>
void	Bitdepth::process_seg_common_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);

	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;
	static_assert (DIF_BITS >= 1, "This function must reduce bidepth.");

	uint32_t &     rnd_state = ctx._rnd_state;

	const SRC_TYPE * fstb_RESTRICT src_n_ptr = reinterpret_cast <const SRC_TYPE *> (src_ptr);
	DST_TYPE * fstb_RESTRICT       dst_n_ptr = reinterpret_cast <      DST_TYPE *> (dst_ptr);

	const int      rcst = 1 << (DIF_BITS - 1);
	const int      vmax = (1 << DST_BITS) - 1;

	const int      ao   = ctx._amp._o_i; // s8
	const int      an   = ctx._amp._n_i; // s8

	for (int pos = 0; pos < w; ++pos)
	{
		const int      s = src_n_ptr [pos];

		const int      dith_o = dither_fnc (pos); // s8
		int            dither;
		if (S_FLAG)
		{
			constexpr int  DIT_SHFT = 8 - DIF_BITS;
			dither = fstb::sshift_r <int, DIT_SHFT> (dith_o);
		}
		else
		{
			const int      dith_n = generate_dith_n_scalar <TN_FLAG> (rnd_state); // s8

			constexpr int  DIT_SHFT = AMP_BITS + 8 - DIF_BITS;
			dither = fstb::sshift_r <int, DIT_SHFT> (dith_o * ao + dith_n * an);	// s16 = s8 * s8 // s16 = s16 >> cst
		}
		const int      sum   = s + dither;	// s16+
		const int      quant = (sum + rcst) >> DIF_BITS;	// s16

		const int      pix   = fstb::limit (quant, 0, vmax);
		dst_n_ptr [pos] = static_cast <DST_TYPE> (pix);
	}

	if (! S_FLAG)
	{
		generate_rnd_eol (rnd_state);
	}
}



// int dither_fnc (int pos) noexcept;
// Must provide the ordered dither value, in [-128 ; +127] nominal range
// (doubled for TPDF)
template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, typename DFNC>
void	Bitdepth::process_seg_common_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);

	const SRC_TYPE * fstb_RESTRICT src_n_ptr = reinterpret_cast <const SRC_TYPE *> (src_ptr);
	DST_TYPE * fstb_RESTRICT       dst_n_ptr = reinterpret_cast <      DST_TYPE *> (dst_ptr);

	uint32_t &     rnd_state = ctx._rnd_state;

	const int      ao   = ctx._amp._o_i; // s8
	const int      an   = ctx._amp._n_i; // s8

	const float    mul  = float (ctx._scale_info_ptr->_gain);
	const float    add  = float (ctx._scale_info_ptr->_add_cst);
	const float    qt   = 1.0f / (1 << ((S_FLAG ? 0 : AMP_BITS) + 8));
	const int      vmax = (1 << DST_BITS) - 1;

	for (int pos = 0; pos < w; ++pos)
	{
		float          s = float (src_n_ptr [pos]);
		s = s * mul + add;

		const int      dith_o = dither_fnc (pos); // s8

		float          dither;
		if (S_FLAG)
		{
			dither = float (dith_o) * qt;
		}
		else
		{
			const int      dith_n = generate_dith_n_scalar <TN_FLAG> (rnd_state); // s8
			dither = float (dith_o * ao + dith_n * an) * qt;
		}
		const float    sum    = s + dither;
		const int      quant  = fstb::round_int (sum);

		const int      pix = fstb::limit (quant, 0, vmax);
		dst_n_ptr [pos] = static_cast <DST_TYPE> (pix);
	}

	if (! S_FLAG)
	{
		generate_rnd_eol (rnd_state);
	}
}



template <bool T_FLAG>
int	Bitdepth::generate_dith_n_scalar (uint32_t &rnd_state) noexcept
{
	generate_rnd (rnd_state);
	int            dith_n = int8_t (rnd_state >> 24);
	if (T_FLAG)
	{
		generate_rnd (rnd_state);
		dith_n += int8_t (rnd_state >> 24);
	}

	return dith_n;
}



int	Bitdepth::remap_tpdf_scalar (int d) noexcept
{
	// [-128 ; 127] to [-32767 ; +32767], representing [-1 ; 1] (15-bit scale)
	auto           x2   = d * d;
	x2 += x2;
	x2 = std::min (x2, 0x7FFFF); // Saturated here because of -min * -min overflow
	auto           x4   = (x2  * x2 ) >> 15;
	auto           x8   = (x4  * x4 ) >> 15;
	auto           x16  = (x8  * x8 ) >> 15;
	auto           x32  = (x16 * x16) >> 15;

	// 15-bit scale
	constexpr int  c3  = 0x8000 * 5 / 8;
	constexpr int  c33 = 0x8000 * 3 / 8;

	// 15-bit scale
	auto           sum_s15 = (x2 * c3 + x32 * c33) >> 15;
	const auto     x_s15   = d << 8;
	const auto     sum_s7  = (sum_s15 * x_s15) >> (30 - 7);

	d += sum_s7;

	return d;
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



// __m128i dither_fnc (int pos) noexcept;
// Must provide the ordered dither values as a vector of 8 x int16_t,
// in [-128 ; +127] nominal range (doubled for TPDF)
template <bool S_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS, typename DFNC>
void	Bitdepth::process_seg_common_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);

	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;
	static_assert (DIF_BITS >= 0, "This function cannot increase bidepth.");

	uint32_t &     rnd_state = ctx._rnd_state;

	typedef typename  fmtcl::ProxyRwSse2 <SRC_FMT>::PtrConst::Type SrcPtr;
	typedef typename  fmtcl::ProxyRwSse2 <DST_FMT>::Ptr::Type      DstPtr;
	SrcPtr         src_n_ptr = reinterpret_cast <SrcPtr> (src_ptr);
	DstPtr         dst_n_ptr = reinterpret_cast <DstPtr> (dst_ptr);
	const __m128i  zero      = _mm_setzero_si128 ();
	const __m128i  mask_lsb  = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit  = _mm_set1_epi16 (-0x8000);
	const __m128i  rcst      = _mm_set1_epi16 (1 << (DIF_BITS - 1));
	const __m128i  vmax      = _mm_set1_epi16 ((1 << DST_BITS) - 1);

	const __m128i  ampo_i    = _mm_set1_epi16 (int16_t (ctx._amp._o_i)); // 8 ?16 [0 ; 255]
	const __m128i  ampn_i    = _mm_set1_epi16 (int16_t (ctx._amp._n_i)); // 8 ?16 [0 ; 255]

	for (int pos = 0; pos < w; pos += 8)
	{
		const __m128i  s =	// 8 u16
			fmtcl::ProxyRwSse2 <SRC_FMT>::read_i16 (src_n_ptr + pos, zero);

		// 8 s16 [-128 ; +127] or [-256 ; 255]
		__m128i        dith_o = dither_fnc (pos);

		__m128i        dither;
		if (S_FLAG)
		{
			constexpr int  DIT_SHFT = 8 - DIF_BITS;
			dither = _mm_srai_epi16 (dith_o, DIT_SHFT);
		}
		else
		{
			// Random generation. 8 s16 [-128 ; 127] or [-256 ; 255]
			__m128i			dith_n = generate_dith_n_vec <TN_FLAG> (rnd_state);

			dith_o = _mm_mullo_epi16 (dith_o, ampo_i);      // 8 s16 (full range)
			dith_n = _mm_mullo_epi16 (dith_n, ampn_i);      // 8 s16 (full range)
			dither = _mm_adds_epi16 (dith_o, dith_n);       // 8 s16 = s8 * s8

			constexpr int  DIT_SHFT = AMP_BITS + 8 - DIF_BITS;
			dither = _mm_srai_epi16 (dither, DIT_SHFT);     // 8 s16 = s16 >> cst
		}

		const __m128i  dith_rcst = _mm_adds_epi16 (dither, rcst);

		__m128i        quant;
		if (S_FLAG && SRC_BITS < 16)
		{
			__m128i        sum = _mm_adds_epi16 (s, dith_rcst);
			quant = _mm_srai_epi16 (sum, DIF_BITS);
		}
		else
		{
			__m128i        sum  = _mm_xor_si128 (s, sign_bit); // 8 s16
			sum   = _mm_adds_epi16 (sum, dith_rcst);
			sum   = _mm_xor_si128 (sum, sign_bit);          // 8 u16
			quant = _mm_srli_epi16 (sum, DIF_BITS);
		}

		__m128i        pix = quant;
		if (SRC_BITS < 16)
		{
			pix = _mm_max_epi16 (pix, zero);
			pix = _mm_min_epi16 (pix, vmax);
		}

		fmtcl::ProxyRwSse2 <DST_FMT>::write_i16 (dst_n_ptr + pos, pix, mask_lsb);
	}

	if (! S_FLAG)
	{
		generate_rnd_eol (rnd_state);
	}
}



template <bool S_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, typename DFNC>
void	Bitdepth::process_seg_common_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (((_mm_getcsr () >> 13) & 3) == 0);   // 00 = Round to nearest (even)

	uint32_t &     rnd_state = ctx._rnd_state;

	const float    qt_cst    = 1.0f / (
		65536.0f * float (1 << ((S_FLAG ? 0 : AMP_BITS) + 8))
	);

	typedef typename  fmtcl::ProxyRwSse2 <SRC_FMT>::PtrConst::Type SrcPtr;
	typedef typename  fmtcl::ProxyRwSse2 <DST_FMT>::Ptr::Type      DstPtr;
	SrcPtr         src_n_ptr = reinterpret_cast <SrcPtr> (src_ptr);
	DstPtr         dst_n_ptr = reinterpret_cast <DstPtr> (dst_ptr);
	const __m128   zero_f    = _mm_setzero_ps ();
	const __m128i  zero_i    = _mm_setzero_si128 ();
	const __m128   mul       = _mm_set1_ps (float (ctx._scale_info_ptr->_gain));
	const __m128   add       = _mm_set1_ps (float (ctx._scale_info_ptr->_add_cst));
	const __m128   qt        = _mm_set1_ps (qt_cst);
	const __m128   vmax      = _mm_set1_ps ((1 << DST_BITS) - 1);
	const __m128   offset    = _mm_set1_ps (-32768);
	const __m128i  mask_lsb  = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit  = _mm_set1_epi16 (-0x8000);

	const __m128i  ampo_i    = _mm_set1_epi16 (int16_t (ctx._amp._o_i)); // 8 ?16 [0 ; 255]
	const __m128i  ampn_i    = _mm_set1_epi16 (int16_t (ctx._amp._n_i)); // 8 ?16 [0 ; 255]

	for (int pos = 0; pos < w; pos += 8)
	{
		__m128         s0;
		__m128         s1;
		fmtcl::ProxyRwSse2 <SRC_FMT>::read_flt (
			src_n_ptr + pos, s0, s1, zero_i
		);
		s0 = _mm_add_ps (_mm_mul_ps (s0, mul), add);
		s1 = _mm_add_ps (_mm_mul_ps (s1, mul), add);

		// 8 s16 [-128 ; +127] or [-256 ; 255]
		__m128i        dith_o = dither_fnc (pos);

		__m128i        dither;
		if (S_FLAG)
		{
			dither = dith_o;
		}
		else
		{
			// Random generation. 8 s16 [-128 ; 127] or [-256 ; 255]
			__m128i			dith_n = generate_dith_n_vec <TN_FLAG> (rnd_state);

			dith_o = _mm_mullo_epi16 (dith_o, ampo_i);      // 8 s16 (full range)
			dith_n = _mm_mullo_epi16 (dith_n, ampn_i);      // 8 s16 (full range)
			dither = _mm_adds_epi16 (dith_o, dith_n);       // 8 s16 = s8 * s8
		}

		__m128i        dither_03i = _mm_unpacklo_epi16 (zero_i, dither);  // 4 s32 << 16
		__m128i        dither_47i = _mm_unpackhi_epi16 (zero_i, dither);  // 4 s32 << 16
		__m128         dither_03  = _mm_cvtepi32_ps (dither_03i);
		__m128         dither_47  = _mm_cvtepi32_ps (dither_47i);
		dither_03 = _mm_mul_ps (dither_03, qt);
		dither_47 = _mm_mul_ps (dither_47, qt);

		s0 = _mm_add_ps (s0, dither_03);
		s1 = _mm_add_ps (s1, dither_47);

		s0 = _mm_max_ps (_mm_min_ps (s0, vmax), zero_f);
		s1 = _mm_max_ps (_mm_min_ps (s1, vmax), zero_f);

		fmtcl::ProxyRwSse2 <DST_FMT>::write_flt (
			dst_n_ptr + pos, s0, s1, mask_lsb, sign_bit, offset
		);
	}

	if (! S_FLAG)
	{
		generate_rnd_eol (rnd_state);
	}
}



template <bool T_FLAG>
__m128i	Bitdepth::generate_dith_n_vec (uint32_t &rnd_state) noexcept
{
	generate_rnd (rnd_state);
	const uint32_t rnd_03  = rnd_state;
	generate_rnd (rnd_state);
	const uint32_t rnd_47  = rnd_state;
	const auto        zero = _mm_setzero_si128 ();

	if (T_FLAG)
	{
		generate_rnd (rnd_state);
		const uint32_t rnd_03x = rnd_state;
		generate_rnd (rnd_state);
		const uint32_t rnd_47x = rnd_state;
		const auto     rnd_val = _mm_set_epi32 (rnd_47x, rnd_03x, rnd_47, rnd_03);
		const auto     c256_16 = _mm_set1_epi16 (0x100);
		const auto     x0      = _mm_unpacklo_epi8 (rnd_val, zero);
		const auto     x1      = _mm_unpackhi_epi8 (rnd_val, zero);
		const auto     dith_n  = _mm_sub_epi16 (_mm_add_epi16 (x0, x1), c256_16);
		return dith_n; // 8 s16 [-256 ; 255]
	}

	else
	{
		const auto     rnd_val = _mm_set_epi32 (0, 0, rnd_47, rnd_03);
		const auto     c128_16 = _mm_set1_epi16 (0x80);
		const auto     x0      = _mm_unpacklo_epi8 (rnd_val, zero); // 8 ?16 [0 ; 255]
		const auto     dith_n  = _mm_sub_epi16 (x0, c128_16);       

		return dith_n; // 8 s16 [-128 ; 127]
	}
}



// d: 8 s16 [-128 ; 127]
// Returns: 8 s16 [-256 ; 255]
// Formula:
// f: [-1 ; +1] -> [-2 ; +2]
//            x -> x + 5/8 * x^3 + 3/8 * x^33
__m128i	Bitdepth::remap_tpdf_vec (__m128i d) noexcept
{
	// [-128 ; 127] to [-32767 ; +32767], representing [-1 ; 1] (15-bit scale)
	auto           x2   = _mm_mullo_epi16 (d  , d  );
	x2  = _mm_adds_epi16 (x2 , x2 ); // Saturated here because of -min * -min overflow
	auto           x4   = _mm_mulhi_epi16 (x2 , x2 );
	x4  = _mm_add_epi16 (x4 , x4 );
	auto           x8   = _mm_mulhi_epi16 (x4 , x4 );
	x8  = _mm_add_epi16 (x8 , x8 );
	auto           x16  = _mm_mulhi_epi16 (x8 , x8 );
	x16 = _mm_add_epi16 (x16, x16);
	auto           x32  = _mm_mulhi_epi16 (x16, x16);
	x32 = _mm_add_epi16 (x32, x32);

	// 15-bit scale
	const auto     c3  = _mm_set1_epi16 (0x8000 * 5 / 8);
	const auto     c33 = _mm_set1_epi16 (0x8000 * 3 / 8);

	// 14-bit scale
	auto           sum_s14 = _mm_mulhi_epi16 (x2, c3);
	sum_s14 = _mm_add_epi16 (sum_s14, _mm_mulhi_epi16 (x32, c33));

	const auto     x_s15 = _mm_slli_epi16 (d, 8);
	const auto     sum_s13 = _mm_mulhi_epi16 (sum_s14, x_s15);

	const auto     sum_s7  = _mm_srai_epi16 (sum_s13, 13 - 7);

	d = _mm_add_epi16 (d, sum_s7);

	return d;
}



#endif



template <bool S_FLAG, bool T_FLAG, class ERRDIF>
void	Bitdepth::process_seg_errdif_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (ctx._y >= 0);

	typedef typename ERRDIF::SrcType SRC_TYPE;
	typedef typename ERRDIF::DstType DST_TYPE;
	constexpr int  SRC_BITS = ERRDIF::SRC_BITS;
	constexpr int  DST_BITS = ERRDIF::DST_BITS;

	uint32_t &                       rnd_state =  ctx._rnd_state;
	fmtcl::ErrDifBuf & fstb_RESTRICT ed_buf    = *ctx._ed_buf_ptr;

	const SRC_TYPE * fstb_RESTRICT src_n_ptr = reinterpret_cast <const SRC_TYPE *> (src_ptr);
	DST_TYPE * fstb_RESTRICT       dst_n_ptr = reinterpret_cast <      DST_TYPE *> (dst_ptr);

	const int      ae = ctx._amp._e_i;

	// Makes e1 point on the default buffer line for single-line
	// error diffusor because we use it in prepare_next_line()
	int            e0 = 0;
	int            e1 = 0;
	if (ERRDIF::NBR_ERR_LINES == 2)
	{
		e0 =      ctx._y & 1 ;
		e1 = 1 - (ctx._y & 1);
	}
	int16_t *      err0_ptr = ed_buf.get_buf <int16_t> (e0);
	int16_t *      err1_ptr = ed_buf.get_buf <int16_t> (e1);

	int            err_nxt0 = ed_buf.use_mem <int16_t> (0);
	int            err_nxt1 = ed_buf.use_mem <int16_t> (1);

	// Forward
	if ((ctx._y & 1) == 0)
	{
		for (int x = 0; x < w; ++x)
		{
			int            err = err_nxt0;
			SRC_TYPE       src_raw;

			quantize_pix_int <
				S_FLAG, T_FLAG, DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS
			> (
				dst_n_ptr, src_n_ptr, src_raw, x, err, rnd_state, ae, ctx._amp._n_i
			);
			ERRDIF::template diffuse <1> (
				err, err_nxt0, err_nxt1,
				err0_ptr + x, err1_ptr + x, src_raw
			);
		}
		ERRDIF::prepare_next_line (err1_ptr + w);
	}

	// Backward
	else
	{
		for (int x = w - 1; x >= 0; --x)
		{
			int            err = err_nxt0;
			SRC_TYPE       src_raw;

			quantize_pix_int <
				S_FLAG, T_FLAG, DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS
			> (
				dst_n_ptr, src_n_ptr, src_raw, x, err, rnd_state, ae, ctx._amp._n_i
			);
			ERRDIF::template diffuse <-1> (
				err, err_nxt0, err_nxt1,
				err0_ptr + x, err1_ptr + x, src_raw
			);
		}
		ERRDIF::prepare_next_line (err1_ptr - 1);
	}

	ed_buf.use_mem <int16_t> (0) = int16_t (err_nxt0);
	ed_buf.use_mem <int16_t> (1) = int16_t (err_nxt1);

	if (! S_FLAG)
	{
		generate_rnd_eol (rnd_state);
	}
}



template <bool S_FLAG, bool T_FLAG, class ERRDIF>
void	Bitdepth::process_seg_errdif_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept
{
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (ctx._y >= 0);

	typedef typename ERRDIF::SrcType SRC_TYPE;
	typedef typename ERRDIF::DstType DST_TYPE;
	constexpr int  DST_BITS = ERRDIF::DST_BITS;

	uint32_t &                       rnd_state =  ctx._rnd_state;
	fmtcl::ErrDifBuf & fstb_RESTRICT ed_buf    = *ctx._ed_buf_ptr;

	const SRC_TYPE * fstb_RESTRICT src_n_ptr = reinterpret_cast <const SRC_TYPE *> (src_ptr);
	DST_TYPE * fstb_RESTRICT       dst_n_ptr = reinterpret_cast <      DST_TYPE *> (dst_ptr);

	const float    mul = float (ctx._scale_info_ptr->_gain);
	const float    add = float (ctx._scale_info_ptr->_add_cst);
	const float    ae  = float (ctx._amp._e_f);
	const float    an  = float (ctx._amp._n_f);

	// Makes e1 point on the default buffer line for single-line
	// error diffusor because we use it in prepare_next_line()
	int            e0 = 0;
	int            e1 = 0;
	if (ERRDIF::NBR_ERR_LINES == 2)
	{
		e0 =      ctx._y & 1 ;
		e1 = 1 - (ctx._y & 1);
	}
	float *        err0_ptr = ed_buf.get_buf <float> (e0);
	float *        err1_ptr = ed_buf.get_buf <float> (e1);

	float          err_nxt0 = ed_buf.use_mem <float> (0);
	float          err_nxt1 = ed_buf.use_mem <float> (1);

	// Forward
	if ((ctx._y & 1) == 0)
	{
		for (int x = 0; x < w; ++x)
		{
			float          err = err_nxt0;
			SRC_TYPE       src_raw;

			quantize_pix_flt <S_FLAG, T_FLAG, DST_TYPE, DST_BITS, SRC_TYPE> (
				dst_n_ptr, src_n_ptr, src_raw, x, err, rnd_state, ae, an, mul, add
			);
			ERRDIF::template diffuse <1> (
				err, err_nxt0, err_nxt1,
				err0_ptr + x, err1_ptr + x, src_raw
			);
		}
		ERRDIF::prepare_next_line (err1_ptr + w);
	}

	// Backward
	else
	{
		for (int x = w - 1; x >= 0; --x)
		{
			float          err = err_nxt0;
			SRC_TYPE       src_raw;

			quantize_pix_flt <S_FLAG, T_FLAG, DST_TYPE, DST_BITS, SRC_TYPE> (
				dst_n_ptr, src_n_ptr, src_raw, x, err, rnd_state, ae, an, mul, add
			);
			ERRDIF::template diffuse <-1> (
				err, err_nxt0, err_nxt1,
				err0_ptr + x, err1_ptr + x, src_raw
			);
		}
		ERRDIF::prepare_next_line (err1_ptr - 1);
	}

	ed_buf.use_mem <float> (0) = err_nxt0;
	ed_buf.use_mem <float> (1) = err_nxt1;

	if (! S_FLAG)
	{
		generate_rnd_eol (rnd_state);
	}
}



void	Bitdepth::generate_rnd (uint32_t &state) noexcept
{
	state = state * uint32_t (1664525) + 1013904223;
}



void	Bitdepth::generate_rnd_eol (uint32_t &state) noexcept
{
	state = state * uint32_t (1103515245) + 12345;
	if ((state & 0x2000000) != 0)
	{
		state = state * uint32_t (134775813) + 1;
	}
}



const Bitdepth::PatRow &	Bitdepth::SegContext::extract_pattern_row () const noexcept
{
	assert (_pattern_ptr != nullptr);
	assert (_y >= 0);

	return ((*_pattern_ptr) [_y & (PAT_WIDTH - 1)]);
}



template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
void	Bitdepth::quantize_pix_int (DST_TYPE * fstb_RESTRICT dst_ptr, const SRC_TYPE * fstb_RESTRICT src_ptr, SRC_TYPE &src_raw, int x, int & fstb_RESTRICT err, uint32_t &rnd_state, int ampe_i, int ampn_i) noexcept
{
	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;
	constexpr int  TMP_BITS =
		  (DIF_BITS < 6 && SRC_BITS < ERR_RES && DST_BITS < ERR_RES)
		? ERR_RES
		: SRC_BITS;
	constexpr int  TMP_SHFT = TMP_BITS - SRC_BITS;
	constexpr int  TMP_INVS = TMP_BITS - DST_BITS;

	const int      rcst     = 1 << (TMP_INVS - 1);
	const int      vmax     = (1 << DST_BITS) - 1;

	src_raw = src_ptr [x];
	const int		src     = src_raw << TMP_SHFT;
	const int      preq    = src + err;

	int            sum     = preq;
	if (! S_FLAG)
	{
		enum {         DIT_SHFT = AMP_BITS + 8 - TMP_INVS };  // May be negative

		const int      dith_n  = generate_dith_n_scalar <TN_FLAG> (rnd_state); // s8
		const int		err_add = (err < 0) ? -ampe_i : ampe_i;
		const int		noise   =
			fstb::sshift_r <int, DIT_SHFT> (dith_n * ampn_i + err_add);	// s16 = s8 * s8 // s16 = s16 >> cst

		sum += noise;
	}

	const int      quant   = (sum + rcst) >> TMP_INVS;

	err = preq - (quant << TMP_INVS);
	const int      pix     = fstb::limit (quant, 0, vmax);

	dst_ptr [x] = static_cast <DST_TYPE> (pix);
}



template <class SRC_TYPE>
static inline SRC_TYPE	Bitdepth_extract_src (SRC_TYPE src_read, float src) noexcept
{
	fstb::unused (src);

	return (src_read);
}

static inline float	Bitdepth_extract_src (float src_read, float src) noexcept
{
	fstb::unused (src_read);

	return (src);
}

template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
void	Bitdepth::quantize_pix_flt (DST_TYPE * fstb_RESTRICT dst_ptr, const SRC_TYPE * fstb_RESTRICT src_ptr, SRC_TYPE &src_raw, int x, float & fstb_RESTRICT err, uint32_t &rnd_state, float ampe_f, float ampn_f, float mul, float add) noexcept
{
	const int      vmax = (1 << DST_BITS) - 1;

	const SRC_TYPE src_read = src_ptr [x];
	const float    src      = float (src_read) * mul + add;
	src_raw = Bitdepth_extract_src (src_read, src);
	const float    preq     = src + err;

	float          sum      = preq;
	if (! S_FLAG)
	{
		const int      dith_n  = generate_dith_n_scalar <TN_FLAG> (rnd_state); // s8
		const float    err_add = (err < 0) ? -ampe_f : (err > 0) ? ampe_f : 0;
		const float    noise   = float (dith_n) * ampn_f + err_add;

		sum += noise;
	}

	const int      quant   = fstb::round_int (sum);

	err = preq - float (quant);
	const int      pix = fstb::limit (quant, 0, vmax);

	dst_ptr [x] = static_cast <DST_TYPE> (pix);
}



// Original coefficients                     : 7, 3, 5, 1
// Optimised coefficients for serpentine scan: 7, 4, 5, 0
// Source:
// Sam Hocevar and Gary Niger,
// Reinstating Floyd-Steinberg: Improved Metrics for Quality Assessment
// of Error Diffusion Algorithms,
// Lecture Notes in Computer Science LNCS 5099, pp. 38�45, 2008
// (Proceedings of the International Conference on Image and Signal Processing
// ICISP 2008) ISSN 0302-9743

#define fmtc_Bitdepth_FS_OPTIMIZED_SERPENTINE_COEF

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseFloydSteinberg <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (err_nxt1, err1_ptr, src_raw);

#if defined (fmtc_Bitdepth_FS_OPTIMIZED_SERPENTINE_COEF)
	const int      e1 = 0;
	const int      e3 = (err * 4 + 8) >> 4;
#else
	const int      e1 = (err     + 8) >> 4;
	const int      e3 = (err * 3 + 8) >> 4;
#endif
	const int      e5 = (err * 5 + 8) >> 4;
	const int      e7 = err - e1 - e3 - e5;
	spread_error <DIR> (e1, e3, e5, e7, err_nxt0, err0_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseFloydSteinberg <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (err_nxt1, err1_ptr, src_raw);

#if defined (fmtc_Bitdepth_FS_OPTIMIZED_SERPENTINE_COEF)
	const float    e1 = 0;
	const float    e3 = err * (4.0f / 16);
#else
	const float    e1 = err * (1.0f / 16);
	const float    e3 = err * (3.0f / 16);
#endif
	const float    e5 = err * (5.0f / 16);
	const float    e7 = err * (7.0f / 16);
	spread_error <DIR> (e1, e3, e5, e7, err_nxt0, err0_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <typename EB>
void	Bitdepth::DiffuseFloydSteinberg <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept
{
	// Nothing
	fstb::unused (err_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR, typename ET, typename EB>
void	Bitdepth::DiffuseFloydSteinberg <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::spread_error (ET e1, ET e3, ET e5, ET e7, ET & fstb_RESTRICT err_nxt0, EB * fstb_RESTRICT err0_ptr) noexcept
{
	err_nxt0         = err0_ptr [DIR];
	err0_ptr [-DIR] += EB (e3);
	err0_ptr [   0] += EB (e5);
	err0_ptr [ DIR]  = EB (e1);
	err_nxt0        += e7;
}



template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseFilterLite <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (err_nxt1, err1_ptr, src_raw);

	const int      e1 = (err + 2) >> 2;
	const int      e2 = err - 2 * e1;
	spread_error <DIR> (e1, e2, err_nxt0, err0_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseFilterLite <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (err_nxt1, err1_ptr, src_raw);

	const float    e1 = err * (1.0f / 4);
	const float    e2 = err * (2.0f / 4);
	spread_error <DIR> (e1, e2, err_nxt0, err0_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <typename EB>
void	Bitdepth::DiffuseFilterLite <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept
{
	err_ptr [0] = EB (0);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR, typename ET, typename EB>
void	Bitdepth::DiffuseFilterLite <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::spread_error (ET e1, ET e2, ET & fstb_RESTRICT err_nxt0, EB * fstb_RESTRICT err0_ptr) noexcept
{
	err_nxt0         = err0_ptr [DIR];
	err0_ptr [-DIR] += EB (e1);
	err0_ptr [   0]  = EB (e1);
	err_nxt0        += e2;
}



template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseStucki <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (src_raw);

	const int      m  = (err << 4) / 42;
	const int      e1 = (m + 8) >> 4;
	const int      e2 = (m + 4) >> 3;
	const int      e4 = (m + 2) >> 2;
//	const int      e8 = (m + 1) >> 1;
	const int      sum = (e1 << 1) + ((e2 + e4) << 2);
	const int      e8 = (err - sum + 1) >> 1;
	spread_error <DIR> (e1, e2, e4, e8, err_nxt0, err_nxt1, err0_ptr, err1_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseStucki <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (src_raw);

	const float    e1 = err * (1.0f / 42);
	const float    e2 = err * (2.0f / 42);
	const float    e4 = err * (4.0f / 42);
	const float    e8 = err * (8.0f / 42);
	spread_error <DIR> (e1, e2, e4, e8, err_nxt0, err_nxt1, err0_ptr, err1_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <typename EB>
void	Bitdepth::DiffuseStucki <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept
{
	// Nothing
	fstb::unused (err_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR, typename ET, typename EB>
void	Bitdepth::DiffuseStucki <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::spread_error (ET e1, ET e2, ET e4, ET e8, ET & fstb_RESTRICT err_nxt0, ET & fstb_RESTRICT err_nxt1, EB * fstb_RESTRICT err0_ptr, EB * fstb_RESTRICT err1_ptr) noexcept
{
	err_nxt0             = err_nxt1 + e8;
	err_nxt1             = err1_ptr [DIR * 2] + e4;
	err0_ptr [-DIR * 2] += EB (e2);
	err0_ptr [-DIR    ] += EB (e4);
	err0_ptr [   0    ] += EB (e8);
	err0_ptr [ DIR    ] += EB (e4);
	err0_ptr [ DIR * 2] += EB (e2);
	err1_ptr [-DIR * 2] += EB (e1);
	err1_ptr [-DIR    ] += EB (e2);
	err1_ptr [   0    ] += EB (e4);
	err1_ptr [ DIR    ] += EB (e2);
	err1_ptr [ DIR * 2]  = EB (e1);
}



template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseAtkinson <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (src_raw);

	const int      e1 = (err + 4) >> 3;
	spread_error <DIR> (e1, err_nxt0, err_nxt1, err0_ptr, err1_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseAtkinson <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (src_raw);

	const float    e1 = err * (1.0f / 8);
	spread_error <DIR> (e1, err_nxt0, err_nxt1, err0_ptr, err1_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <typename EB>
void	Bitdepth::DiffuseAtkinson <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept
{
	err_ptr [0] = EB (0);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR, typename ET, typename EB>
void	Bitdepth::DiffuseAtkinson <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::spread_error (ET e1, ET & fstb_RESTRICT err_nxt0, ET & fstb_RESTRICT err_nxt1, EB * fstb_RESTRICT err0_ptr, EB * fstb_RESTRICT err1_ptr) noexcept
{
	err_nxt0         = err_nxt1           + e1;
	err_nxt1         = err1_ptr [2 * DIR] + e1;
	err0_ptr [-DIR] += EB (e1);
	err0_ptr [   0] += EB (e1);
	err0_ptr [+DIR] += EB (e1);
	err1_ptr [   0]  = EB (e1);
}



template <int DST_BITS, int SRC_BITS>
template <class SRC_TYPE>
int	Bitdepth::DiffuseOstromoukhovBase2 <DST_BITS, SRC_BITS>::get_index (SRC_TYPE src_raw) noexcept
{
	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;

	return (fstb::sshift_l <
		int,
		DiffuseOstromoukhovBase::T_BITS - DIF_BITS
	> (src_raw) & DiffuseOstromoukhovBase::T_MASK);
}

template <int DST_BITS, int SRC_BITS>
int	Bitdepth::DiffuseOstromoukhovBase2 <DST_BITS, SRC_BITS>::get_index (float src_raw) noexcept
{
	return 
		  fstb::round_int (src_raw * DiffuseOstromoukhovBase::T_LEN)
	   & DiffuseOstromoukhovBase::T_MASK;
}

// Victor Ostromoukhov,
// A Simple and Efficient Error-Diffusion Algorithm
// Proceedings of SIGGRAPH 2001, in ACM Computer Graphics,
// Annual Conference Series, pp. 567-572, 2001.
// Not optimised at all
template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseOstromoukhov <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (err_nxt1, err1_ptr);

	constexpr int  DIF_BITS = SRC_BITS - DST_BITS;

	const int      index    = fstb::sshift_l <
		int,
		DiffuseOstromoukhov::T_BITS - DIF_BITS
	> (src_raw) & DiffuseOstromoukhov::T_MASK;
	const typename ThisType::TableEntry & fstb_RESTRICT te = ThisType::_table [index];
	const int      d        = te._sum;

	const int      e1 = err * te._c0 / d;
	const int      e2 = err * te._c1 / d;
	const int      e3 = err - e1 - e2;

	spread_error <DIR> (e1, e2, e3, err_nxt0, err0_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR>
void	Bitdepth::DiffuseOstromoukhov <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept
{
	fstb::unused (err_nxt1, err1_ptr);

	const int      index    = DiffuseOstromoukhov::get_index (src_raw);
	const typename ThisType::TableEntry & fstb_RESTRICT te = ThisType::_table [index];
	const float    invd     = te._inv_sum;

	const float    e1 = err * float (te._c0) * invd;
	const float    e2 = err * float (te._c1) * invd;
	const float    e3 = err - e1 - e2;

	spread_error <DIR> (e1, e2, e3, err_nxt0, err0_ptr);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <typename EB>
void	Bitdepth::DiffuseOstromoukhov <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept
{
	err_ptr [0] = EB (0);
}

template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
template <int DIR, typename ET, typename EB>
void	Bitdepth::DiffuseOstromoukhov <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS>::spread_error (ET e1, ET e2, ET e3, ET & fstb_RESTRICT err_nxt0, EB * fstb_RESTRICT err0_ptr) noexcept
{
	err_nxt0         = err0_ptr [DIR];
	err0_ptr [-DIR] += EB (e2);
	err0_ptr [   0]  = EB (e3);
	err_nxt0        += e1;
}



const std::array <
	Bitdepth::DiffuseOstromoukhovBase::TableEntry,
	Bitdepth::DiffuseOstromoukhovBase::T_LEN
>	Bitdepth::DiffuseOstromoukhovBase::_table =
{{
	{   13,    0,    5,   18, 1.0f /   18 },
	{   13,    0,    5,   18, 1.0f /   18 },
	{   21,    0,   10,   31, 1.0f /   31 },
	{    7,    0,    4,   11, 1.0f /   11 },
	{    8,    0,    5,   13, 1.0f /   13 },
	{   47,    3,   28,   78, 1.0f /   78 },
	{   23,    3,   13,   39, 1.0f /   39 },
	{   15,    3,    8,   26, 1.0f /   26 },
	{   22,    6,   11,   39, 1.0f /   39 },
	{   43,   15,   20,   78, 1.0f /   78 },
	{    7,    3,    3,   13, 1.0f /   13 },
	{  501,  224,  211,  936, 1.0f /  936 },
	{  249,  116,  103,  468, 1.0f /  468 },
	{  165,   80,   67,  312, 1.0f /  312 },
	{  123,   62,   49,  234, 1.0f /  234 },
	{  489,  256,  191,  936, 1.0f /  936 },
	{   81,   44,   31,  156, 1.0f /  156 },
	{  483,  272,  181,  936, 1.0f /  936 },
	{   60,   35,   22,  117, 1.0f /  117 },
	{   53,   32,   19,  104, 1.0f /  104 },
	{  237,  148,   83,  468, 1.0f /  468 },
	{  471,  304,  161,  936, 1.0f /  936 },
	{    3,    2,    1,    6, 1.0f /    6 },
	{  481,  314,  185,  980, 1.0f /  980 },
	{  354,  226,  155,  735, 1.0f /  735 },
	{ 1389,  866,  685, 2940, 1.0f / 2940 },
	{  227,  138,  125,  490, 1.0f /  490 },
	{  267,  158,  163,  588, 1.0f /  588 },
	{  327,  188,  220,  735, 1.0f /  735 },
	{   61,   34,   45,  140, 1.0f /  140 },
	{  627,  338,  505, 1470, 1.0f / 1470 },
	{ 1227,  638, 1075, 2940, 1.0f / 2940 },

	{   20,   10,   19,   49, 1.0f /   49 },
	{ 1937, 1000, 1767, 4704, 1.0f / 4704 },
	{  977,  520,  855, 2352, 1.0f / 2352 },
	{  657,  360,  551, 1568, 1.0f / 1568 },
	{   71,   40,   57,  168, 1.0f /  168 },
	{ 2005, 1160, 1539, 4704, 1.0f / 4704 },
	{  337,  200,  247,  784, 1.0f /  784 },
	{ 2039, 1240, 1425, 4704, 1.0f / 4704 },
	{  257,  160,  171,  588, 1.0f /  588 },
	{  691,  440,  437, 1568, 1.0f / 1568 },
	{ 1045,  680,  627, 2352, 1.0f / 2352 },
	{  301,  200,  171,  672, 1.0f /  672 },
	{  177,  120,   95,  392, 1.0f /  392 },
	{ 2141, 1480, 1083, 4704, 1.0f / 4704 },
	{ 1079,  760,  513, 2352, 1.0f / 2352 },
	{  725,  520,  323, 1568, 1.0f / 1568 },
	{  137,  100,   57,  294, 1.0f /  294 },
	{ 2209, 1640,  855, 4704, 1.0f / 4704 },
	{   53,   40,   19,  112, 1.0f /  112 },
	{ 2243, 1720,  741, 4704, 1.0f / 4704 },
	{  565,  440,  171, 1176, 1.0f / 1176 },
	{  759,  600,  209, 1568, 1.0f / 1568 },
	{ 1147,  920,  285, 2352, 1.0f / 2352 },
	{ 2311, 1880,  513, 4704, 1.0f / 4704 },
	{   97,   80,   19,  196, 1.0f /  196 },
	{  335,  280,   57,  672, 1.0f /  672 },
	{ 1181, 1000,  171, 2352, 1.0f / 2352 },
	{  793,  680,   95, 1568, 1.0f / 1568 },
	{  599,  520,   57, 1176, 1.0f / 1176 },
	{ 2413, 2120,  171, 4704, 1.0f / 4704 },
	{  405,  360,   19,  784, 1.0f /  784 },
	{ 2447, 2200,   57, 4704, 1.0f / 4704 },

	{   11,   10,    0,   21, 1.0f /   21 },
	{  158,  151,    3,  312, 1.0f /  312 },
	{  178,  179,    7,  364, 1.0f /  364 },
	{ 1030, 1091,   63, 2184, 1.0f / 2184 },
	{  248,  277,   21,  546, 1.0f /  546 },
	{  318,  375,   35,  728, 1.0f /  728 },
	{  458,  571,   63, 1092, 1.0f / 1092 },
	{  878, 1159,  147, 2184, 1.0f / 2184 },
	{    5,    7,    1,   13, 1.0f /   13 },
	{  172,  181,   37,  390, 1.0f /  390 },
	{   97,   76,   22,  195, 1.0f /  195 },
	{   72,   41,   17,  130, 1.0f /  130 },
	{  119,   47,   29,  195, 1.0f /  195 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{   65,   18,   17,  100, 1.0f /  100 },
	{   95,   29,   26,  150, 1.0f /  150 },
	{  185,   62,   53,  300, 1.0f /  300 },
	{   30,   11,    9,   50, 1.0f /   50 },
	{   35,   14,   11,   60, 1.0f /   60 },
	{   85,   37,   28,  150, 1.0f /  150 },
	{   55,   26,   19,  100, 1.0f /  100 },
	{   80,   41,   29,  150, 1.0f /  150 },
	{  155,   86,   59,  300, 1.0f /  300 },
	{    5,    3,    2,   10, 1.0f /   10 },

	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{  305,  176,  119,  600, 1.0f /  600 },
	{  155,   86,   59,  300, 1.0f /  300 },
	{  105,   56,   39,  200, 1.0f /  200 },
	{   80,   41,   29,  150, 1.0f /  150 },
	{   65,   32,   23,  120, 1.0f /  120 },
	{   55,   26,   19,  100, 1.0f /  100 },
	{  335,  152,  113,  600, 1.0f /  600 },
	{   85,   37,   28,  150, 1.0f /  150 },
	{  115,   48,   37,  200, 1.0f /  200 },
	{   35,   14,   11,   60, 1.0f /   60 },
	{  355,  136,  109,  600, 1.0f /  600 },
	{   30,   11,    9,   50, 1.0f /   50 },
	{  365,  128,  107,  600, 1.0f /  600 },
	{  185,   62,   53,  300, 1.0f /  300 },
	{   25,    8,    7,   40, 1.0f /   40 },
	{   95,   29,   26,  150, 1.0f /  150 },
	{  385,  112,  103,  600, 1.0f /  600 },
	{   65,   18,   17,  100, 1.0f /  100 },
	{  395,  104,  101,  600, 1.0f /  600 },
	{    4,    1,    1,    6, 1.0f /    6 },

	// Symetric
	{    4,    1,    1,    6, 1.0f /    6 },
	{  395,  104,  101,  600, 1.0f /  600 },
	{   65,   18,   17,  100, 1.0f /  100 },
	{  385,  112,  103,  600, 1.0f /  600 },
	{   95,   29,   26,  150, 1.0f /  150 },
	{   25,    8,    7,   40, 1.0f /   40 },
	{  185,   62,   53,  300, 1.0f /  300 },
	{  365,  128,  107,  600, 1.0f /  600 },
	{   30,   11,    9,   50, 1.0f /   50 },
	{  355,  136,  109,  600, 1.0f /  600 },
	{   35,   14,   11,   60, 1.0f /   60 },
	{  115,   48,   37,  200, 1.0f /  200 },
	{   85,   37,   28,  150, 1.0f /  150 },
	{  335,  152,  113,  600, 1.0f /  600 },
	{   55,   26,   19,  100, 1.0f /  100 },
	{   65,   32,   23,  120, 1.0f /  120 },
	{   80,   41,   29,  150, 1.0f /  150 },
	{  105,   56,   39,  200, 1.0f /  200 },
	{  155,   86,   59,  300, 1.0f /  300 },
	{  305,  176,  119,  600, 1.0f /  600 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },
	{    5,    3,    2,   10, 1.0f /   10 },

	{    5,    3,    2,   10, 1.0f /   10 },
	{  155,   86,   59,  300, 1.0f /  300 },
	{   80,   41,   29,  150, 1.0f /  150 },
	{   55,   26,   19,  100, 1.0f /  100 },
	{   85,   37,   28,  150, 1.0f /  150 },
	{   35,   14,   11,   60, 1.0f /   60 },
	{   30,   11,    9,   50, 1.0f /   50 },
	{  185,   62,   53,  300, 1.0f /  300 },
	{   95,   29,   26,  150, 1.0f /  150 },
	{   65,   18,   17,  100, 1.0f /  100 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{    4,    1,    1,    6, 1.0f /    6 },
	{  119,   47,   29,  195, 1.0f /  195 },
	{   72,   41,   17,  130, 1.0f /  130 },
	{   97,   76,   22,  195, 1.0f /  195 },
	{  172,  181,   37,  390, 1.0f /  390 },
	{    5,    7,    1,   13, 1.0f /   13 },
	{  878, 1159,  147, 2184, 1.0f / 2184 },
	{  458,  571,   63, 1092, 1.0f / 1092 },
	{  318,  375,   35,  728, 1.0f /  728 },
	{  248,  277,   21,  546, 1.0f /  546 },
	{ 1030, 1091,   63, 2184, 1.0f / 2184 },
	{  178,  179,    7,  364, 1.0f /  364 },
	{  158,  151,    3,  312, 1.0f /  312 },
	{   11,   10,    0,   21, 1.0f /   21 },

	{ 2447, 2200,   57, 4704, 1.0f / 4704 },
	{  405,  360,   19,  784, 1.0f /  784 },
	{ 2413, 2120,  171, 4704, 1.0f / 4704 },
	{  599,  520,   57, 1176, 1.0f / 1176 },
	{  793,  680,   95, 1568, 1.0f / 1568 },
	{ 1181, 1000,  171, 2352, 1.0f / 2352 },
	{  335,  280,   57,  672, 1.0f /  672 },
	{   97,   80,   19,  196, 1.0f /  196 },
	{ 2311, 1880,  513, 4704, 1.0f / 4704 },
	{ 1147,  920,  285, 2352, 1.0f / 2352 },
	{  759,  600,  209, 1568, 1.0f / 1568 },
	{  565,  440,  171, 1176, 1.0f / 1176 },
	{ 2243, 1720,  741, 4704, 1.0f / 4704 },
	{   53,   40,   19,  112, 1.0f /  112 },
	{ 2209, 1640,  855, 4704, 1.0f / 4704 },
	{  137,  100,   57,  294, 1.0f /  294 },
	{  725,  520,  323, 1568, 1.0f / 1568 },
	{ 1079,  760,  513, 2352, 1.0f / 2352 },
	{ 2141, 1480, 1083, 4704, 1.0f / 4704 },
	{  177,  120,   95,  392, 1.0f /  392 },
	{  301,  200,  171,  672, 1.0f /  672 },
	{ 1045,  680,  627, 2352, 1.0f / 2352 },
	{  691,  440,  437, 1568, 1.0f / 1568 },
	{  257,  160,  171,  588, 1.0f /  588 },
	{ 2039, 1240, 1425, 4704, 1.0f / 4704 },
	{  337,  200,  247,  784, 1.0f /  784 },
	{ 2005, 1160, 1539, 4704, 1.0f / 4704 },
	{   71,   40,   57,  168, 1.0f /  168 },
	{  657,  360,  551, 1568, 1.0f / 1568 },
	{  977,  520,  855, 2352, 1.0f / 2352 },
	{ 1937, 1000, 1767, 4704, 1.0f / 4704 },
	{   20,   10,   19,   49, 1.0f /   49 },

	{ 1227,  638, 1075, 2940, 1.0f / 2940 },
	{  627,  338,  505, 1470, 1.0f / 1470 },
	{   61,   34,   45,  140, 1.0f /  140 },
	{  327,  188,  220,  735, 1.0f /  735 },
	{  267,  158,  163,  588, 1.0f /  588 },
	{  227,  138,  125,  490, 1.0f /  490 },
	{ 1389,  866,  685, 2940, 1.0f / 2940 },
	{  354,  226,  155,  735, 1.0f /  735 },
	{  481,  314,  185,  980, 1.0f /  980 },
	{    3,    2,    1,    6, 1.0f /    6 },
	{  471,  304,  161,  936, 1.0f /  936 },
	{  237,  148,   83,  468, 1.0f /  468 },
	{   53,   32,   19,  104, 1.0f /  104 },
	{   60,   35,   22,  117, 1.0f /  117 },
	{  483,  272,  181,  936, 1.0f /  936 },
	{   81,   44,   31,  156, 1.0f /  156 },
	{  489,  256,  191,  936, 1.0f /  936 },
	{  123,   62,   49,  234, 1.0f /  234 },
	{  165,   80,   67,  312, 1.0f /  312 },
	{  249,  116,  103,  468, 1.0f /  468 },
	{  501,  224,  211,  936, 1.0f /  936 },
	{    7,    3,    3,   13, 1.0f /   13 },
	{   43,   15,   20,   78, 1.0f /   78 },
	{   22,    6,   11,   39, 1.0f /   39 },
	{   15,    3,    8,   26, 1.0f /   26 },
	{   23,    3,   13,   39, 1.0f /   39 },
	{   47,    3,   28,   78, 1.0f /   78 },
	{    8,    0,    5,   13, 1.0f /   13 },
	{    7,    0,    4,   11, 1.0f /   11 },
	{   21,    0,   10,   31, 1.0f /   31 },
	{   13,    0,    5,   18, 1.0f /   18 },
	{   13,    0,    5,   18, 1.0f /   18 }
}};



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
