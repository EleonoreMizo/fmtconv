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
#include "fmtc/CpuOpt.h"
#include "fmtc/fnc.h"
#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/fnc.h"

#include <algorithm>
#include <stdexcept>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Bitdepth::Bitdepth (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "bitdepth", ::fmParallel)
,	_clip_src_sptr (vsapi.mapGetNode (&in, "clip", 0, nullptr), vsapi)
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
{
	fstb::unused (user_data_ptr);

	const fmtc::CpuOpt   cpu_opt (*this, in, out);
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (! vsutl::is_constant_format (_vi_in))
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	// Source colorspace
	const auto &   fmt_src = _vi_in.format;

	{
		const int      st  = fmt_src.sampleType;
		const int      bps = fmt_src.bytesPerSample;
		const int      res = fmt_src.bitsPerSample;
		if (! (   (st == ::stInteger && bps == 1 &&     res ==  8 )
		       || (st == ::stInteger && bps == 2 && (   (   res >=  9
		                                                 && res <= 12)
		                                             || res == 14
		                                             || res == 16))
		       || (st == ::stFloat   && bps == 4 &&     res == 32 )))
		{
			throw_inval_arg ("input pixel bitdepth not supported.");
		}
	}

	const auto     splfmt_src = conv_vsfmt_to_splfmt (fmt_src);
	const auto     col_fam    = conv_vsfmt_to_colfam (fmt_src);

	// Destination colorspace
	const auto     fmt_dst = get_output_colorspace (in, out, core, fmt_src);

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
	_vi_out.format = fmt_dst;
	const auto     splfmt_dst = conv_vsfmt_to_splfmt (fmt_dst);

	_plane_processor.set_filter (in, out, _vi_out, true);

	const int      w = _vi_in.width; // May be <= 0

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

	// Dithering parameters
	fmtcl::Dither::DMode dmode = static_cast <fmtcl::Dither::DMode> (
		get_arg_int (in, out, "dmode", fmtcl::Dither::DMode_FILTERLITE)
	);
	if (dmode == fmtcl::Dither::DMode_ROUND_ALIAS)
	{
		dmode = fmtcl::Dither::DMode_ROUND;
	}
	if (   dmode <  0
	    || (dmode & 0xFFFF) >= fmtcl::Dither::DMode_NBR_ELT)
	{
		throw_inval_arg ("invalid dmode.");
	}

	const double   ampo = get_arg_flt (in, out, "ampo", 1.0);
	if (ampo < 0)
	{
		throw_inval_arg ("ampo cannot be negative.");
	}

	const double   ampn = get_arg_flt (in, out, "ampn", 0.0);
	if (ampn < 0)
	{
		throw_inval_arg ("ampn cannot be negative.");
	}

	const int      pat_size = get_arg_int (in, out, "patsize", 32);
	if (   pat_size < 4
	    || pat_size > fmtcl::Dither::_pat_max_size
	    || ! fstb::is_pow_2 (pat_size))
	{
		throw_inval_arg ("Wrong value for patsize.");
	}

	const bool     dyn_flag = (get_arg_int (in, out, "dyn", 0) != 0);
	const bool     static_noise_flag = (get_arg_int (in, out, "staticnoise", 0) != 0);
	const bool     correlated_planes_flag = (get_arg_int (in, out, "corplane", 0) != 0);
	const bool     tpdfo_flag = (get_arg_int (in, out, "tpdfo", 0) != 0);
	const bool     tpdfn_flag = (get_arg_int (in, out, "tpdfn", 0) != 0);

	_engine_uptr = std::make_unique <fmtcl::Dither> (
		splfmt_src, fmt_src.bitsPerSample, _full_range_in_flag,
		splfmt_dst, fmt_dst.bitsPerSample, _full_range_out_flag,
		col_fam, fmt_dst.numPlanes, w,
		dmode, pat_size, ampo, ampn,
		dyn_flag, static_noise_flag, correlated_planes_flag,
		tpdfo_flag, tpdfn_flag,
		sse2_flag, avx2_flag
	);
}



::VSVideoInfo	Bitdepth::get_video_info () const
{
	return _vi_out;
}



std::vector <::VSFilterDependency>	Bitdepth::get_dependencies () const
{
	return std::vector <::VSFilterDependency> {
		{ &*_clip_src_sptr, ::rpStrictSpatial }
	};
}



const ::VSFrame *	Bitdepth::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);

	::VSFrame *    dst_ptr = nullptr;
	::VSNode &     node    = *_clip_src_sptr;

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
		const ::VSFrame & src = *src_sptr;

		const int      w = _vsapi.getFrameWidth (&src, 0);
		const int      h = _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (&_vi_out.format, w, h, &src, &core);

		const int      ret_val = _plane_processor.process_frame (
			*dst_ptr, n, frame_data_ptr, frame_ctx, core, _clip_src_sptr
		);
		if (ret_val != 0)
		{
			_vsapi.freeFrame (dst_ptr);
			dst_ptr = nullptr;
		}

		// Output frame properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropertiesRW (dst_ptr));
		if (_range_def_flag)
		{
			const int      cr_val = (_full_range_out_flag) ? 0 : 1;
			_vsapi.mapSetInt (&dst_prop, "_ColorRange", cr_val, ::maReplace);
		}
	}

	return dst_ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	Bitdepth::do_process_plane (::VSFrame &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr)
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
		const ::VSFrame & src = *src_sptr;

		const int      w = _vsapi.getFrameWidth (&src, plane_index);
		const int      h = _vsapi.getFrameHeight (&src, plane_index);

		const uint8_t* data_src_ptr = _vsapi.getReadPtr (&src, plane_index);
		const auto     stride_src   = _vsapi.getStride (&src, plane_index);
		uint8_t *      data_dst_ptr = _vsapi.getWritePtr (&dst, plane_index);
		const auto     stride_dst   = _vsapi.getStride (&dst, plane_index);

		try
		{
			_engine_uptr->process_plane (
				data_dst_ptr, stride_dst,
				data_src_ptr, stride_src,
				w, h, n, plane_index
			);
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

	return ret_val;
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



::VSVideoFormat	Bitdepth::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src) const
{
	auto           fmt_dst  = fmt_src;

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
		const auto     gvfbi_ret =
			_vsapi.getVideoFormatByID (&fmt_dst, dst_csp, &core);
		if (gvfbi_ret == 0)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
	}

	else
	{
		int            col_fam  = fmt_dst.colorFamily;
		int            spl_type = fmt_dst.sampleType;
		int            bits     = fmt_dst.bitsPerSample;
		int            ssh      = fmt_dst.subSamplingW;
		int            ssv      = fmt_dst.subSamplingH;

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
		bool        ok_flag = true;
		try
		{
			ok_flag = register_format (
				fmt_dst,
				col_fam, spl_type, bits, ssh, ssv,
				core
			);
		}
		catch (...)
		{
			ok_flag = false;
		}
		if (! ok_flag)
		{
			throw_rt_err (
				"couldn\'t get a pixel format identifier for the output clip."
			);
		}
	}

	return fmt_dst;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
