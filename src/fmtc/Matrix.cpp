/*****************************************************************************

        Matrix.cpp
        Author: Laurent de Soras, 2012

TO DO:
	- Make the SSE2 code use aligned read/write.
	- Make a special case for kRkGkB matrix conversions, where the luma plane
		is not used to compute the chroma planes.

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

#include "fstb/def.h"
#include "conc/Array.h"
#include "fmtc/Matrix.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fmtcl/ProxyRwSse2.h"
#endif
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include <xmmintrin.h>
#endif

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix::Matrix (const ::VSMap &in, ::VSMap &out, void * /*user_data_ptr*/, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "matrix", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_sse_flag (false)
,	_sse2_flag (false)
,	_avx_flag (false)
,	_avx2_flag (false)
,	_range_set_src_flag (false)
,	_range_set_dst_flag (false)
,	_full_range_src_flag (false)
,	_full_range_dst_flag (false)
/*,	_mat_main ()*/
,	_csp_out (fmtcl::ColorSpaceH265_UNSPECIFIED)
,	_plane_out (get_arg_int (in, out, "singleout", -1))
,	_coef_flt_arr ()
,	_coef_int_arr ()
,	_apply_matrix_ptr (0)
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&vsapi != 0);

	vsutl::CpuOpt  cpu_opt (*this, in, out);
	_sse_flag  = cpu_opt.has_sse ();
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx_flag  = cpu_opt.has_avx ();
	_avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const ::VSFormat &   fmt_src = *_vi_in.format;

	if (fmt_src.subSamplingW != 0 || fmt_src.subSamplingH != 0)
	{
		throw_inval_arg ("input must be 4:4:4.");
	}
	if (fmt_src.numPlanes != NBR_PLANES)
	{
		throw_inval_arg ("greyscale format not supported as input.");
	}
	if (   (   fmt_src.sampleType == ::stInteger
	        && fmt_src.bitsPerSample !=  8
	        && fmt_src.bitsPerSample !=  9
	        && fmt_src.bitsPerSample != 10
	        && fmt_src.bitsPerSample != 12
	        && fmt_src.bitsPerSample != 16)
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_src.bitsPerSample != 32))
	{
		throw_inval_arg ("pixel bitdepth not supported.");
	}

	if (_plane_out >= NBR_PLANES)
	{
		throw_inval_arg (
			"singleout is a plane index and must be -1 or ranging from 0 to 3."
		);
	}

	// Destination colorspace
	bool           force_col_fam_flag;
	const ::VSFormat *   fmt_dst_ptr = get_output_colorspace (
		in, out, core, fmt_src, _plane_out, force_col_fam_flag
	);

	if (   fmt_dst_ptr->colorFamily != ::cmGray
	    && fmt_dst_ptr->colorFamily != ::cmRGB
	    && fmt_dst_ptr->colorFamily != ::cmYUV
	    && fmt_dst_ptr->colorFamily != ::cmYCoCg)
	{
		throw_inval_arg ("unsupported color family for output.");
	}
	if (   (   fmt_dst_ptr->sampleType == ::stInteger
	        && fmt_dst_ptr->bitsPerSample !=  8
	        && fmt_dst_ptr->bitsPerSample !=  9
	        && fmt_dst_ptr->bitsPerSample != 10
	        && fmt_dst_ptr->bitsPerSample != 12
	        && fmt_dst_ptr->bitsPerSample != 16)
	    || (   fmt_dst_ptr->sampleType == ::stFloat
	        && fmt_dst_ptr->bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}
	if (   fmt_dst_ptr->sampleType    != fmt_src.sampleType
	    || fmt_dst_ptr->bitsPerSample <  fmt_src.bitsPerSample
	    || fmt_dst_ptr->subSamplingW  != fmt_src.subSamplingW
	    || fmt_dst_ptr->subSamplingH  != fmt_src.subSamplingH)
	{
		throw_inval_arg (
			"specified output colorspace is not compatible with the input."
		);
	}

	// Preliminary matrix test: deduce the target color family if unspecified
	if (   ! force_col_fam_flag
	    && fmt_dst_ptr->colorFamily != ::cmGray)
	{
		int               def_count = 0;
		def_count += is_arg_defined (in, "mat" ) ? 1 : 0;
		def_count += is_arg_defined (in, "mats") ? 1 : 0;
		def_count += is_arg_defined (in, "matd") ? 1 : 0;
		if (def_count == 1)
		{
			std::string    tmp_mat (get_arg_str (in, out, "mat", ""));
			tmp_mat = get_arg_str (in, out, "mats", tmp_mat);
			tmp_mat = get_arg_str (in, out, "matd", tmp_mat);

			fmtcl::ColorSpaceH265   tmp_csp =
				find_cs_from_mat_str (*this, tmp_mat, false);

			fmt_dst_ptr = find_dst_col_fam (tmp_csp, fmt_dst_ptr, fmt_src, core);
		}
	}

	// Output format is validated.
	_vi_out.format = fmt_dst_ptr;
	const ::VSFormat &fmt_dst = *fmt_dst_ptr;

	const int      nbr_expected_coef = NBR_PLANES * (NBR_PLANES + 1);

	bool           mat_init_flag = false;

	// Matrix presets
	std::string    mat (get_arg_str (in, out, "mat", ""));
	std::string    mats ((   fmt_src.colorFamily == ::cmYUV ) ? mat : "");
	std::string    matd ((   fmt_dst.colorFamily == ::cmYUV
	                      || fmt_dst.colorFamily == ::cmGray) ? mat : "");
	mats = get_arg_str (in, out, "mats", mats);
	matd = get_arg_str (in, out, "matd", matd);
	if (! mats.empty () || ! matd.empty ())
	{
		fstb::conv_to_lower_case (mats);
		fstb::conv_to_lower_case (matd);
		select_def_mat (mats, fmt_src);
		select_def_mat (matd, fmt_dst);

		Mat4           m2s;
		Mat4           m2d;
		make_mat_from_str (m2s, mats, true);
		make_mat_from_str (m2d, matd, false);
		_csp_out = find_cs_from_mat_str (*this, matd, false);

		copy_mat (_mat_main, m2d);
		mul_mat (_mat_main, m2s);

		mat_init_flag = true;
	}

	// Range
	_full_range_src_flag = (get_arg_int (
		in, out, "fulls" ,
		vsutl::is_full_range_default (fmt_src) ? 1 : 0,
		0, &_range_set_src_flag
	) != 0);
	_full_range_dst_flag = (get_arg_int (
		in, out, "fulld",
		vsutl::is_full_range_default (fmt_dst) ? 1 : 0,
		0, &_range_set_dst_flag
	) != 0);

	// Custom coefficients
	const int      nbr_coef = _vsapi.propNumElements (&in, "coef");
	const bool     custom_mat_flag = (nbr_coef > 0);
	if (custom_mat_flag)
	{
		if (nbr_coef != nbr_expected_coef)
		{
			throw_inval_arg ("coef has a wrong number of elements.");
		}

		for (int y = 0; y < NBR_PLANES + 1; ++y)
		{
			for (int x = 0; x < NBR_PLANES + 1; ++x)
			{
				_mat_main [y] [x] = (x == y) ? 1 : 0;

				if (   (x < fmt_src.numPlanes || x == NBR_PLANES)
				    &&  y < fmt_dst.numPlanes)
				{
					int            err = 0;
					const int      index = y * (fmt_src.numPlanes + 1) + x;
					const double   c = _vsapi.propGetFloat (&in, "coef", index, &err);
					if (err != 0)
					{
						throw_rt_err ("error while reading the matrix coefficients.");
					}
					_mat_main [y] [x] = c;
				}
			}
		}

		mat_init_flag = true;
	}

	if (! mat_init_flag)
	{
		throw_inval_arg (
			"you must specify a matrix preset or a custom coefficient list."
		);
	}

	// Float coefficients
	prepare_coef_flt (fmt_dst, fmt_src);

	// Integer coefficients
	if (   fmt_src.sampleType == ::stInteger
	    && fmt_dst.sampleType == ::stInteger)
	{
		prepare_coef_int (fmt_dst, fmt_src, nbr_expected_coef);
	}

	if (_vsapi.getError (&out) != 0)
	{
		throw -1;
	}

	// Selects the coeffients for single-plane processing by
	// moving the desired row to the top of the matrix.
	if (_plane_out > 0)  // Strictly > 0 because 0 is neutral.
	{
		move_matrix_row ();
	}

	if (fmt_src.sampleType == ::stFloat)
	{
		if (_plane_out >= 0)
		{
			_apply_matrix_ptr = &ThisType::apply_matrix_1_cpp_flt;
		}
		else
		{
			_apply_matrix_ptr = &ThisType::apply_matrix_3_cpp_flt;
		}
#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_sse_flag)
		{
			if (_plane_out >= 0)
			{
				_apply_matrix_ptr = &ThisType::apply_matrix_1_sse_flt;
			}
			else
			{
				_apply_matrix_ptr = &ThisType::apply_matrix_3_sse_flt;
			}
		}
		if (_avx_flag)
		{
			if (_plane_out >= 0)
			{
				_apply_matrix_ptr = &ThisType::apply_matrix_1_avx_flt;
			}
			else
			{
				_apply_matrix_ptr = &ThisType::apply_matrix_3_avx_flt;
			}
		}
#endif
	}
	else
	{
		const int      np = (_plane_out >= 0) ? 1 : 0;

#define Matrix_CASE_CPP(DT, DB, ST, SB) \
		case	(DB << 8) + (SB << 1) + 0: \
		_apply_matrix_ptr = &ThisType::apply_matrix_3_cpp_int <DT, DB, ST, SB>; \
			break; \
		case	(DB << 8) + (SB << 1) + 1: \
		_apply_matrix_ptr = &ThisType::apply_matrix_1_cpp_int <DT, DB, ST, SB>; \
			break;

		switch ((fmt_dst.bitsPerSample << 8) + (fmt_src.bitsPerSample << 1) + np)
		{
		Matrix_CASE_CPP (uint8_t ,  8, uint8_t ,  8)
		Matrix_CASE_CPP (uint16_t,  9, uint8_t ,  8)
		Matrix_CASE_CPP (uint16_t,  9, uint16_t,  9)
		Matrix_CASE_CPP (uint16_t, 10, uint8_t,   8)
		Matrix_CASE_CPP (uint16_t, 10, uint16_t,  9)
		Matrix_CASE_CPP (uint16_t, 10, uint16_t, 10)
		Matrix_CASE_CPP (uint16_t, 12, uint8_t,   8)
		Matrix_CASE_CPP (uint16_t, 12, uint16_t,  9)
		Matrix_CASE_CPP (uint16_t, 12, uint16_t, 10)
		Matrix_CASE_CPP (uint16_t, 12, uint16_t, 12)
		Matrix_CASE_CPP (uint16_t, 16, uint8_t ,  8)
		Matrix_CASE_CPP (uint16_t, 16, uint16_t,  9)
		Matrix_CASE_CPP (uint16_t, 16, uint16_t, 10)
		Matrix_CASE_CPP (uint16_t, 16, uint16_t, 12)
		Matrix_CASE_CPP (uint16_t, 16, uint16_t, 16)
		default:
			assert (false);
			throw_logic_err (
				"unhandled case for CPP function assignation. "
				"Please contact the plug-in developer."
			);
			break;
		}

#undef Matrix_CASE_CPP

#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_sse2_flag)
		{
#define Matrix_CASE_SSE2(DST, DB, SRC, SB) \
			case	(DB << 8) + (SB << 1) + 0: \
				_apply_matrix_ptr = &ThisType::apply_matrix_n_sse2_int < \
					fmtcl::ProxyRwSse2 <fmtcl::SplFmt_##DST>, DB, \
					fmtcl::ProxyRwSse2 <fmtcl::SplFmt_##SRC>, SB, \
					3 \
				>; \
				break; \
			case	(DB << 8) + (SB << 1) + 1: \
				_apply_matrix_ptr = &ThisType::apply_matrix_n_sse2_int < \
					fmtcl::ProxyRwSse2 <fmtcl::SplFmt_##DST>, DB, \
					fmtcl::ProxyRwSse2 <fmtcl::SplFmt_##SRC>, SB, \
					1 \
				>; \
				break;

			switch ((fmt_dst.bitsPerSample << 8) + (fmt_src.bitsPerSample << 1) + np)
			{
			Matrix_CASE_SSE2 (INT8 ,  8, INT8 ,  8)
			Matrix_CASE_SSE2 (INT16,  9, INT8 ,  8)
			Matrix_CASE_SSE2 (INT16,  9, INT16,  9)
			Matrix_CASE_SSE2 (INT16, 10, INT8 ,  8)
			Matrix_CASE_SSE2 (INT16, 10, INT16,  9)
			Matrix_CASE_SSE2 (INT16, 10, INT16, 10)
			Matrix_CASE_SSE2 (INT16, 12, INT8 ,  8)
			Matrix_CASE_SSE2 (INT16, 12, INT16,  9)
			Matrix_CASE_SSE2 (INT16, 12, INT16, 10)
			Matrix_CASE_SSE2 (INT16, 12, INT16, 12)
			Matrix_CASE_SSE2 (INT16, 16, INT8 ,  8)
			Matrix_CASE_SSE2 (INT16, 16, INT16,  9)
			Matrix_CASE_SSE2 (INT16, 16, INT16, 10)
			Matrix_CASE_SSE2 (INT16, 16, INT16, 12)
			Matrix_CASE_SSE2 (INT16, 16, INT16, 16)
			default:
				assert (false);
				throw_logic_err (
					"unhandled case for SSE2 function assignation. "
					"Please contact the plug-in developer."
				);
				break;
			}

#undef Matrix_CASE_SSE2
		}

		if (_avx2_flag)
		{
			config_avx2_matrix_n ();
		}
#endif
	}

	if (_apply_matrix_ptr == 0)
	{
		assert (false);
		throw_logic_err (
			"missing function initialisation. "
			"Please contact the plug-in developer."
		);
	}
}



void	Matrix::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&node != 0);
	assert (&core != 0);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Matrix::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);
	assert (&frame_data_ptr != 0);
	assert (&frame_ctx != 0);
	assert (&core != 0);

	::VSFrameRef *    dst_ptr = 0;
	::VSNodeRef &     node = *_clip_src_sptr;

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

		const int         w  =  _vsapi.getFrameWidth (&src, 0);
		const int         h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		assert (_apply_matrix_ptr != 0);
		(this->*_apply_matrix_ptr) (*dst_ptr, src, w, h);

		// Output frame properties
		if (_range_set_dst_flag || _csp_out != fmtcl::ColorSpaceH265_UNSPECIFIED)
		{
			::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

			if (_range_set_dst_flag)
			{
				const int      cr_val = (_full_range_dst_flag) ? 0 : 1;
				_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);
			}

			if (   _csp_out != fmtcl::ColorSpaceH265_UNSPECIFIED
			    && _csp_out <= fmtcl::ColorSpaceH265_ISO_RANGE_LAST)
			{
				_vsapi.propSetInt (&dst_prop, "_Matrix"    , int (_csp_out), ::paReplace);
				_vsapi.propSetInt (&dst_prop, "_ColorSpace", int (_csp_out), ::paReplace);
			}
		}
	}

	return (dst_ptr);
}



// Everything should be lower case at this point
void	Matrix::select_def_mat (std::string &mat, const ::VSFormat &fmt)
{
	assert (&mat != 0);
	assert (&fmt != 0);

	if (mat.empty ())
	{
		switch (fmt.colorFamily)
		{
		case	::cmYUV:
			mat = "601";
			break;

		case	::cmYCoCg:
			mat = "ycgco";
			break;

		case	::cmGray:   // Should not happen actually
		case	::cmRGB:
		case	::cmCompat:
		default:
			// Nothing
			break;
		}
	}
}



fmtcl::ColorSpaceH265	Matrix::find_cs_from_mat_str (const vsutl::FilterBase &flt, const std::string &mat, bool allow_2020cl_flag)
{
	assert (&flt != 0);
	assert (&mat != 0);

	fmtcl::ColorSpaceH265   cs = fmtcl::ColorSpaceH265_UNSPECIFIED;

	if (mat.empty () || mat == "rgb")
	{
		cs = fmtcl::ColorSpaceH265_RGB;
	}
	else if (mat == "601")
	{
		cs = fmtcl::ColorSpaceH265_SMPTE170M;
	}
	else if (mat == "709")
	{
		cs = fmtcl::ColorSpaceH265_BT709;
	}
	else if (mat == "240")
	{
		cs = fmtcl::ColorSpaceH265_SMPTE240M;
	}
	else if (mat == "fcc")
	{
		cs = fmtcl::ColorSpaceH265_FCC;
	}
	else if (mat == "ycgco" || mat == "ycocg")
	{
		cs = fmtcl::ColorSpaceH265_YCGCO;
	}
	else if (mat == "2020")
	{
		cs = fmtcl::ColorSpaceH265_BT2020NCL;
	}
	else if (mat == "2020CL" && allow_2020cl_flag)
	{
		cs = fmtcl::ColorSpaceH265_BT2020CL;
	}
	else
	{
		flt.throw_inval_arg ("unknown matrix identifier.");
	}

	return (cs);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat *	Matrix::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src, int &plane_out, bool &force_col_fam_flag) const
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&fmt_src != 0);
	assert (&plane_out != 0);

	force_col_fam_flag = false;

	const ::VSFormat *   fmt_dst_ptr = &fmt_src;

	// Full colorspace
	int            csp_dst = get_arg_int (in, out, "csp", ::pfNone);
	if (csp_dst != ::pfNone)
	{
		fmt_dst_ptr = _vsapi.getFormatPreset (csp_dst, &core);
		if (fmt_dst_ptr == 0)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
		else
		{
			force_col_fam_flag = true;
		}
	}

	int            col_fam  = fmt_dst_ptr->colorFamily;
	int            spl_type = fmt_dst_ptr->sampleType;
	int            bits     = fmt_dst_ptr->bitsPerSample;
	int            ssh      = fmt_dst_ptr->subSamplingW;
	int            ssv      = fmt_dst_ptr->subSamplingH;

	// Color family
	if (is_arg_defined (in, "col_fam"))
	{
		force_col_fam_flag = true;
		col_fam = get_arg_int (in, out, "col_fam", col_fam);
	}

	if (plane_out >= 0)
	{
		col_fam = ::cmGray;
	}
	else if (col_fam == ::cmGray)
	{
		plane_out = 0;
	}

	// Destination bit depth
	bits = get_arg_int (in, out, "bits", bits);

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
	catch (std::exception &)
	{
		throw;
	}
	catch (...)
	{
		fmt_dst_ptr = 0;
	}

	if (fmt_dst_ptr == 0)
	{
		throw_rt_err (
			"couldn\'t get a pixel format identifier for the output clip."
		);
	}

	return (fmt_dst_ptr);
}



template <typename DT, int DB, typename ST, int SB>
void	Matrix::apply_matrix_3_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);
	assert (_vi_in.format->bitsPerSample == SB);
	assert (_vi_out.format->bitsPerSample == DB);

	const ST *     src_0_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 0));
	const ST *     src_1_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 1));
	const ST *     src_2_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	DT *           dst_0_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 0));
	DT *           dst_1_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 1));
	DT *           dst_2_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	const int      ma  = (1 << DB) - 1;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      s0 = src_0_dat_ptr [x];
			const int      s1 = src_1_dat_ptr [x];
			const int      s2 = src_2_dat_ptr [x];

			const int      d0 = (  s0 * _coef_int_arr [ 0]
			                     + s1 * _coef_int_arr [ 1]
			                     + s2 * _coef_int_arr [ 2]
			                     +      _coef_int_arr [ 3]) >> (SHIFT_INT + SB - DB);
			const int      d1 = (  s0 * _coef_int_arr [ 4]
			                     + s1 * _coef_int_arr [ 5]
			                     + s2 * _coef_int_arr [ 6]
			                     +      _coef_int_arr [ 7]) >> (SHIFT_INT + SB - DB);
			const int      d2 = (  s0 * _coef_int_arr [ 8]
			                     + s1 * _coef_int_arr [ 9]
			                     + s2 * _coef_int_arr [10]
			                     +      _coef_int_arr [11]) >> (SHIFT_INT + SB - DB);

			dst_0_dat_ptr [x] = static_cast <DT> (std::max (std::min (d0, ma), 0));
			dst_1_dat_ptr [x] = static_cast <DT> (std::max (std::min (d1, ma), 0));
			dst_2_dat_ptr [x] = static_cast <DT> (std::max (std::min (d2, ma), 0));
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



template <typename DT, int DB, typename ST, int SB>
void	Matrix::apply_matrix_1_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);
	assert (_vi_in.format->bitsPerSample == SB);
	assert (_vi_out.format->bitsPerSample == DB);

	const ST *     src_0_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 0));
	const ST *     src_1_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 1));
	const ST *     src_2_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	DT *           dst_0_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 0));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));

	const int      ma  = (1 << DB) - 1;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      s0 = src_0_dat_ptr [x];
			const int      s1 = src_1_dat_ptr [x];
			const int      s2 = src_2_dat_ptr [x];

			const int      d0 = (  s0 * _coef_int_arr [ 0]
			                     + s1 * _coef_int_arr [ 1]
			                     + s2 * _coef_int_arr [ 2]
			                     +      _coef_int_arr [ 3]) >> (SHIFT_INT + SB - DB);

			dst_0_dat_ptr [x] = static_cast <DT> (std::max (std::min (d0, ma), 0));
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

// DST and SRC are ProxyRwSse2 classes
template <class DST, int DB, class SRC, int SB, int NP>
void	Matrix::apply_matrix_n_sse2_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);
	assert (_vi_in.format->bitsPerSample == SB);
	assert (_vi_out.format->bitsPerSample == DB);

	enum { BPS_SRC = (SB + 7) >> 3 };
	enum { BPS_DST = (DB + 7) >> 3 };
	assert (_vi_in.format->bytesPerSample == BPS_SRC);
	assert (_vi_out.format->bytesPerSample == BPS_DST);

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const __m128i  zero     = _mm_setzero_si128 ();
	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit = _mm_set1_epi16 (-0x8000);
	const __m128i  ma       = _mm_set1_epi16 (int16_t ((1 << DB) - 1));

	const __m128i* coef_ptr = reinterpret_cast <const __m128i *> (
		_coef_simd_arr.use_vect_sse2 (0)
	);

	// Loop over lines then over planes helps keeping input data
	// in the cache.
	const int      nbr_planes = std::min (_vi_out.format->numPlanes, NP);
	const int      src_0_str = _vsapi.getStride (&src, 0);
	const int      src_1_str = _vsapi.getStride (&src, 1);
	const int      src_2_str = _vsapi.getStride (&src, 2);

	for (int y = 0; y < h; ++y)
	{
		for (int plane_index = 0; plane_index < nbr_planes; ++ plane_index)
		{
			const uint8_t* src_0_ptr = _vsapi.getReadPtr (&src, 0) + y * src_0_str;
			const uint8_t* src_1_ptr = _vsapi.getReadPtr (&src, 1) + y * src_1_str;
			const uint8_t* src_2_ptr = _vsapi.getReadPtr (&src, 2) + y * src_2_str;
			const int      dst_str   = _vsapi.getStride (&dst, plane_index);
			uint8_t *      dst_ptr   = _vsapi.getWritePtr (&dst, plane_index) + y * dst_str;
			const int      cind      = plane_index * (NBR_PLANES + 1);

			for (int x = 0; x < w; x += 8)
			{
				typedef typename SRC::template S16 <false     , (SB == 16)> SrcS16R;
				typedef typename DST::template S16 <(DB != 16), (DB == 16)> DstS16W;

				const __m128i  s0 = SrcS16R::read (
					reinterpret_cast <SrcPtr> (src_0_ptr + x * BPS_SRC),
					zero,
					sign_bit
				);
				const __m128i  s1 = SrcS16R::read (
					reinterpret_cast <SrcPtr> (src_1_ptr + x * BPS_SRC),
					zero,
					sign_bit
				);
				const __m128i  s2 = SrcS16R::read (
					reinterpret_cast <SrcPtr> (src_2_ptr + x * BPS_SRC),
					zero,
					sign_bit
				);

				__m128i        d0 = _mm_load_si128 (coef_ptr + cind + 3);
				__m128i        d1 = d0;

				// src is variable, up to 16-bit signed (full range, +1 = 32767+1)
				// coef is 13-bit signed (+1 = 4096)
				// dst1 and dst2 are 28-bit signed (+1 = 2 ^ 27) packed on 32-bit int.
				// Maximum headroom: *16 (4 bits)
				fstb::ToolsSse2::mac_s16_s16_s32 (
					d0, d1, s0, _mm_load_si128 (coef_ptr + cind + 0));
				fstb::ToolsSse2::mac_s16_s16_s32 (
					d0, d1, s1, _mm_load_si128 (coef_ptr + cind + 1));
				fstb::ToolsSse2::mac_s16_s16_s32 (
					d0, d1, s2, _mm_load_si128 (coef_ptr + cind + 2));

				d0 = _mm_srai_epi32 (d0, SHIFT_INT + SB - DB);
				d1 = _mm_srai_epi32 (d1, SHIFT_INT + SB - DB);

				__m128i			val = _mm_packs_epi32 (d0, d1);

				DstS16W::write_clip (
					reinterpret_cast <DstPtr> (dst_ptr + x * BPS_DST),
					val,
					mask_lsb,
					zero,
					ma,
					sign_bit
				);
			}
		}
	}
}

#endif



void	Matrix::apply_matrix_3_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	float *        dst_1_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 1));
	float *        dst_2_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    s0 = src_0_dat_ptr [x];
			const float    s1 = src_1_dat_ptr [x];
			const float    s2 = src_2_dat_ptr [x];

			const float    d0 =   s0 * _coef_flt_arr [ 0]
			                    + s1 * _coef_flt_arr [ 1]
			                    + s2 * _coef_flt_arr [ 2]
			                    +      _coef_flt_arr [ 3];
			const float    d1 =   s0 * _coef_flt_arr [ 4]
			                    + s1 * _coef_flt_arr [ 5]
			                    + s2 * _coef_flt_arr [ 6]
			                    +      _coef_flt_arr [ 7];
			const float    d2 =   s0 * _coef_flt_arr [ 8]
			                    + s1 * _coef_flt_arr [ 9]
			                    + s2 * _coef_flt_arr [10]
			                    +      _coef_flt_arr [11];

			dst_0_dat_ptr [x] = d0;
			dst_1_dat_ptr [x] = d1;
			dst_2_dat_ptr [x] = d2;
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



void	Matrix::apply_matrix_1_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    s0 = src_0_dat_ptr [x];
			const float    s1 = src_1_dat_ptr [x];
			const float    s2 = src_2_dat_ptr [x];

			const float    d0 =   s0 * _coef_flt_arr [ 0]
			                    + s1 * _coef_flt_arr [ 1]
			                    + s2 * _coef_flt_arr [ 2]
			                    +      _coef_flt_arr [ 3];

			dst_0_dat_ptr [x] = d0;
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

void	Matrix::apply_matrix_3_sse_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	float *        dst_1_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 1));
	float *        dst_2_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	const __m128   c00 = _mm_set1_ps (_coef_flt_arr [ 0]);
	const __m128   c01 = _mm_set1_ps (_coef_flt_arr [ 1]);
	const __m128   c02 = _mm_set1_ps (_coef_flt_arr [ 2]);
	const __m128   c03 = _mm_set1_ps (_coef_flt_arr [ 3]);
	const __m128   c04 = _mm_set1_ps (_coef_flt_arr [ 4]);
	const __m128   c05 = _mm_set1_ps (_coef_flt_arr [ 5]);
	const __m128   c06 = _mm_set1_ps (_coef_flt_arr [ 6]);
	const __m128   c07 = _mm_set1_ps (_coef_flt_arr [ 7]);
	const __m128   c08 = _mm_set1_ps (_coef_flt_arr [ 8]);
	const __m128   c09 = _mm_set1_ps (_coef_flt_arr [ 9]);
	const __m128   c10 = _mm_set1_ps (_coef_flt_arr [10]);
	const __m128   c11 = _mm_set1_ps (_coef_flt_arr [11]);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; x += 4)
		{
			const __m128   s0 = _mm_load_ps (src_0_dat_ptr + x);
			const __m128   s1 = _mm_load_ps (src_1_dat_ptr + x);
			const __m128   s2 = _mm_load_ps (src_2_dat_ptr + x);

			const __m128   d0 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c00),
				_mm_mul_ps (s1, c01)),
				_mm_mul_ps (s2, c02)),
				                c03);
			const __m128   d1 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c04),
				_mm_mul_ps (s1, c05)),
				_mm_mul_ps (s2, c06)),
				                c07);
			const __m128   d2 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c08),
				_mm_mul_ps (s1, c09)),
				_mm_mul_ps (s2, c10)),
				                c11);

			_mm_store_ps (dst_0_dat_ptr + x, d0);
			_mm_store_ps (dst_1_dat_ptr + x, d1);
			_mm_store_ps (dst_2_dat_ptr + x, d2);
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



void	Matrix::apply_matrix_1_sse_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));

	const __m128   c00 = _mm_set1_ps (_coef_flt_arr [ 0]);
	const __m128   c01 = _mm_set1_ps (_coef_flt_arr [ 1]);
	const __m128   c02 = _mm_set1_ps (_coef_flt_arr [ 2]);
	const __m128   c03 = _mm_set1_ps (_coef_flt_arr [ 3]);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; x += 4)
		{
			const __m128   s0 = _mm_load_ps (src_0_dat_ptr + x);
			const __m128   s1 = _mm_load_ps (src_1_dat_ptr + x);
			const __m128   s2 = _mm_load_ps (src_2_dat_ptr + x);

			const __m128   d0 = _mm_add_ps (_mm_add_ps (_mm_add_ps (
				_mm_mul_ps (s0, c00),
				_mm_mul_ps (s1, c01)),
				_mm_mul_ps (s2, c02)),
				                c03);

			_mm_store_ps (dst_0_dat_ptr + x, d0);
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
	}
}

#endif



void	Matrix::prepare_coef_int (const ::VSFormat &fmt_dst, const ::VSFormat &fmt_src, int nbr_expected_coef)
{
	assert (&fmt_dst != 0);
	assert (&fmt_src != 0);

	_coef_int_arr.resize (nbr_expected_coef, 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (_sse2_flag)
	{
		if (_avx2_flag)
		{
			_coef_simd_arr.set_avx2_mode (true);
		}
		_coef_simd_arr.resize (nbr_expected_coef);
	}
#endif

	Mat4           m =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 0 }
	};

	{
		// For the coefficient calculation, use the same output bitdepth
		// as the input. The bitdepth change will be done separately with
		// a simple bitshift.
		::VSFormat     fmt_dst2 = fmt_dst;
		fmt_dst2.bitsPerSample = fmt_src.bitsPerSample;

		override_fmt_with_csp (fmt_dst2);

		Mat4           m1s;
		Mat4           m1d;
		make_mat_flt_int (m1s, true , fmt_src , _full_range_src_flag);
		make_mat_flt_int (m1d, false, fmt_dst2, _full_range_dst_flag);
		mul_mat (m, m1d);
		mul_mat (m, _mat_main);
		mul_mat (m, m1s);
	}

	// Coefficient scale
	const double   cintsc = double ((uint64_t (1)) << SHIFT_INT);

	// Rounding constant
	const int      div_shift =
		SHIFT_INT + fmt_src.bitsPerSample - fmt_dst.bitsPerSample;
	const int      rnd = 1 << (div_shift - 1);

	for (int y = 0; y < NBR_PLANES; ++y)
	{
		// (SSE2 only) Compensate for the sign in 16 bits.
		// We need to take both source and destination into account.
		// Real formula:
		//		result = (sum of (plane_i * coef_i)) + cst
		// Executed formula:
		//		result - bias_d = (sum of ((plane_i - bias_s) * coef_i)) + biased_cst
		// therefore:
		//		biased_cst = cst - bias_d + bias_s * sum of (coef_i)
		double         bias_flt = (fmt_dst.bitsPerSample == 16) ? -1 : 0;

		for (int x = 0; x < NBR_PLANES + 1; ++x)
		{
			const bool     add_flag = (x == NBR_PLANES);
			const int      index = y * (NBR_PLANES + 1) + x;

			const double   c = m [y] [x];
			double         scaled_c = c * cintsc;

			const double   chk_c =
				scaled_c * double ((uint64_t (1)) << fmt_src.bitsPerSample);
			if (   ! add_flag
			    && (chk_c < INT_MIN || chk_c > INT_MAX))
			{
				throw_inval_arg (
					"one of the coefficients could cause an overflow."
				);
			}

			const int      c_int = fstb::round_int (scaled_c);

			// Coefficient for the C++ version
			int            c_cpp = c_int;
			if (add_flag)
			{
				// Combines the additive and rounding constants to save one add.
				c_cpp += rnd;
			}
			_coef_int_arr [index] = c_cpp;

			// Coefficient for the SSE2 version
			if (_sse2_flag)
			{
				// Default: normal integer coefficient
				int            c_sse2 = c_int;

				// Multiplicative coefficient
				if (! add_flag)
				{
					if (fmt_src.bitsPerSample == 16)
					{
						bias_flt += c;
					}

					if (c_sse2 < -0x8000 || c_sse2 > 0x7FFF)
					{
						throw_inval_arg ("too big matrix coefficient.");
					}
					_coef_simd_arr.set_coef (index, c_sse2);
				}

				// Additive coefficient
				else
				{
					if (fmt_dst.bitsPerSample == 16 || fmt_src.bitsPerSample == 16)
					{
						const double   scale = double (
							(uint64_t (1)) << (fmt_src.bitsPerSample + SHIFT_INT - 1)
						);
						const int      bias = fstb::round_int (bias_flt * scale);

						c_sse2 += bias;
					}

					// Finally, the rounding constant
					c_sse2 += rnd;

					// Stores the additive constant in 32 bits
					_coef_simd_arr.set_coef_int32 (index, c_sse2);
				}  // if add_flag
			}  // if _sse2_flag
		}  // for x
	}  // for y
}



void	Matrix::prepare_coef_flt (const ::VSFormat &fmt_dst, const ::VSFormat &fmt_src)
{
	assert (&fmt_dst != 0);
	assert (&fmt_src != 0);

	Mat4           m =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};

	::VSFormat     fmt_dst2 = fmt_dst;
	override_fmt_with_csp (fmt_dst2);

	Mat4           m1s;
	Mat4           m1d;
	make_mat_flt_int (m1s, true , fmt_src , _full_range_src_flag);
	make_mat_flt_int (m1d, false, fmt_dst2, _full_range_dst_flag);
	mul_mat (m, m1d);
	if (_plane_out > 0 && vsutl::is_chroma_plane (fmt_dst2, _plane_out))
	{
		m [_plane_out] [NBR_PLANES] += 0.5;
	}
	mul_mat (m, _mat_main);
	mul_mat (m, m1s);

	_coef_flt_arr.clear ();
	for (int y = 0; y < NBR_PLANES; ++y)
	{
		for (int x = 0; x < NBR_PLANES + 1; ++x)
		{
			const float    c = float (m [y] [x]);
			_coef_flt_arr.push_back (c);
		}
	}
}



void	Matrix::override_fmt_with_csp (::VSFormat &fmt) const
{
	assert (&fmt != 0);

	if (_plane_out >= 0)
	{
		fmt.numPlanes = 3;
		if (_csp_out == fmtcl::ColorSpaceH265_RGB)
		{
			fmt.colorFamily = ::cmRGB;
		}
		else if (_csp_out == fmtcl::ColorSpaceH265_YCGCO)
		{
			fmt.colorFamily = ::cmYCoCg;
		}
		else
		{
			fmt.colorFamily = ::cmYUV;
		}
	}
}



void	Matrix::move_matrix_row ()
{
	assert (_plane_out >= 0);
	
	const int      ofs      = _plane_out * (NBR_PLANES + 1);
	const bool     int_flag = (_vi_in.format->sampleType == ::stInteger);

	for (int c = 0; c < NBR_PLANES + 1; ++c)
	{
		_coef_flt_arr [c] = _coef_flt_arr [c + ofs];

		if (int_flag)
		{
			_coef_int_arr [c] = _coef_int_arr [c + ofs];
			if (_sse2_flag)
			{
				_coef_simd_arr.copy_coef (c, c + ofs);
			}
		}
	}
}



const ::VSFormat *	Matrix::find_dst_col_fam (fmtcl::ColorSpaceH265 tmp_csp, const ::VSFormat *fmt_dst_ptr, const ::VSFormat &fmt_src, ::VSCore &core)
{
	int               alt_cf = -1;

	switch (tmp_csp)
	{
	case fmtcl::ColorSpaceH265_RGB:
	case fmtcl::ColorSpaceH265_BT709:
	case fmtcl::ColorSpaceH265_FCC:
	case fmtcl::ColorSpaceH265_BT470BG:
	case fmtcl::ColorSpaceH265_SMPTE170M:
	case fmtcl::ColorSpaceH265_SMPTE240M:
	case fmtcl::ColorSpaceH265_BT2020NCL:
	case fmtcl::ColorSpaceH265_BT2020CL:
		alt_cf = ::cmYUV;
		break;

	case fmtcl::ColorSpaceH265_YCGCO:
		alt_cf = ::cmYCoCg;
		break;

	default:
		// Nothing
		break;
	}

	if (alt_cf >= 0)
	{
		int            col_fam  = fmt_dst_ptr->colorFamily;
		int            spl_type = fmt_dst_ptr->sampleType;
		int            bits     = fmt_dst_ptr->bitsPerSample;
		int            ssh      = fmt_dst_ptr->subSamplingW;
		int            ssv      = fmt_dst_ptr->subSamplingH;
		if (fmt_src.colorFamily == ::cmRGB)
		{
			col_fam = alt_cf;
		}
		else if (fmt_src.colorFamily == alt_cf)
		{
			col_fam = ::cmRGB;
		}

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
		catch (std::exception &)
		{
			throw;
		}
		catch (...)
		{
			fmt_dst_ptr = 0;
		}
	}

	return (fmt_dst_ptr);
}



void	Matrix::make_mat_from_str (Mat4 &m, const std::string &mat, bool to_rgb_flag) const
{
	assert (&m != 0);
	assert (&mat != 0);

	if (mat.empty () || mat == "rgb")
	{
		m[0][0] = 1; m[0][1] = 0; m[0][2] = 0;
		m[1][0] = 0; m[1][1] = 1; m[1][2] = 0;
		m[2][0] = 0; m[2][1] = 0; m[2][2] = 1;
		complete_mat3 (m);
	}
	else if (mat == "601")
	{
		make_mat_yuv (m, 0.299, 0.587, 0.114, to_rgb_flag);
	}
	else if (mat == "709")
	{
		make_mat_yuv (m, 0.2126, 0.7152, 0.0722, to_rgb_flag);
	}
	else if (mat == "240")
	{
		make_mat_yuv (m, 0.212, 0.701, 0.087, to_rgb_flag);
	}
	else if (mat == "fcc")
	{
		make_mat_yuv (m, 0.30, 0.59, 0.11, to_rgb_flag);
	}
	else if (mat == "ycgco" || mat == "ycocg")
	{
		make_mat_ycgco (m, to_rgb_flag);
	}
	else if (mat == "2020")
	{
		make_mat_yuv (m, 0.2627, 0.678, 0.0593, to_rgb_flag);
	}
	else
	{
		throw_inval_arg ("unknown matrix identifier.");
	}
}



void	Matrix::mul_mat (Mat4 &dst, const Mat4 &src)
{
	assert (&dst != 0);
	assert (&src != 0);

	Mat4           tmp;
	mul_mat (tmp, dst, src);
	copy_mat (dst, tmp);
}



void	Matrix::mul_mat (Mat4 &dst, const Mat4 &lhs, const Mat4 &rhs)
{
	assert (&dst != 0);
	assert (&lhs != 0);
	assert (&lhs != &dst);
	assert (&rhs != 0);
	assert (&rhs != &dst);

	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			dst [y] [x] =   lhs [y] [0] * rhs [0] [x]
			              + lhs [y] [1] * rhs [1] [x]
			              + lhs [y] [2] * rhs [2] [x]
			              + lhs [y] [3] * rhs [3] [x];
		}
	}
}



void	Matrix::copy_mat (Mat4 &dst, const Mat4 &src)
{
	assert (&dst != 0);
	assert (&src != 0);

#if 0
	memcpy (&dst, &src, sizeof (dst)); // Dirty
#else
	for (int y = 0; y < 4; ++y)
	{
		for (int x = 0; x < 4; ++x)
		{
			dst [y] [x] = src [y] [x];
		}
	}
#endif
}



/*
kr/kg/kb matrix (Rec. ITU-T H.264 03/2010, p. 379):

R = Y                  + V*(1-Kr)
G = Y - U*(1-Kb)*Kb/Kg - V*(1-Kr)*Kr/Kg
B = Y + U*(1-Kb)

Y =                  R * Kr        + G * Kg        + B * Kb
U = (B-Y)/(1-Kb) = - R * Kr/(1-Kb) - G * Kg/(1-Kb) + B
V = (R-Y)/(1-Kr) =   R             - G * Kg/(1-Kr) - B * Kb/(1-Kr)

The given equations work for R, G, B in range [0 ; 1] and U and V in range
[-1 ; 1]. Scaling must be applied to match the required range for U and V.

R, G, B, Y range : [0 ; 1]
U, V range : [-0.5 ; 0.5]
*/

void	Matrix::make_mat_yuv (Mat4 &m, double kr, double kg, double kb, bool to_rgb_flag)
{
	assert (&m != 0);
	assert (kg != 0);
	assert (kb != 1);
	assert (kr != 1);

	const double   r = 0.5;
	const double   x = 1.0 / r;
	if (to_rgb_flag)
	{
		m[0][0] = 1; m[0][1] =              0; m[0][2] = x*(1-kr)      ;
		m[1][0] = 1; m[1][1] = x*(kb-1)*kb/kg; m[1][2] = x*(kr-1)*kr/kg;
		m[2][0] = 1; m[2][1] = x*(1-kb)      ; m[2][2] =              0;
	}

	else
	{
		m[0][0] =     kr     ; m[0][1] =   kg       ; m[0][2] =   kb       ;
		m[1][0] = r*kr/(kb-1); m[1][1] = r*kg/(kb-1); m[1][2] = r          ;
		m[2][0] = r          ; m[2][1] = r*kg/(kr-1); m[2][2] = r*kb/(kr-1);
	}

	complete_mat3 (m);
}



/*
YCgCo matrix (Rec. ITU-T H.264 03/2010, p. 379):

R  = Y - Cg + Co
G  = Y + Cg
B  = Y - Cg - Co

Y  =  0.25 * R + 0.5  * G + 0.25 * B
Cg = -0.25 * R + 0.5  * G - 0.25 * B
Co =  0.5  * R            - 0.5  * B

R, G, B, Y range : [0 ; 1]
Cg, Co range : [-0.5 ; 0.5]

Note: this implementation is not exactly the same as specified because the
standard specifies specific steps to apply the RGB-to-YCgCo matrix, leading
to different roundings.
*/

void	Matrix::make_mat_ycgco (Mat4 &m, bool to_rgb_flag)
{
	assert (&m != 0);

	if (to_rgb_flag)
	{
		m[0][0] = 1; m[0][1] = -1; m[0][2] =  1;
		m[1][0] = 1; m[1][1] =  1; m[1][2] =  0;
		m[2][0] = 1; m[2][1] = -1; m[2][2] = -1;
	}
	else
	{
		m[0][0] =  0.25; m[0][1] = 0.5; m[0][2] =  0.25;
		m[1][0] = -0.25; m[1][1] = 0.5; m[1][2] = -0.25;
		m[2][0] =  0.5 ; m[2][1] = 0  ; m[2][2] = -0.5 ;
	}

	complete_mat3 (m);
}



// Int: depends on the input format (may be float too)
// R, G, B, Y: [0 ; 1]
// U, V, Cg, Co : [-0.5 ; 0.5]
void	Matrix::make_mat_flt_int (Mat4 &m, bool to_flt_flag, const ::VSFormat &fmt, bool full_flag)
{
	assert (&m != 0);
	assert (&fmt != 0);

	::VSFormat     fmt2 (fmt);
	fmt2.sampleType = ::stFloat;

	const ::VSFormat* fmt_src_ptr = &fmt2;
	const ::VSFormat* fmt_dst_ptr = &fmt;
	if (to_flt_flag)
	{
		std::swap (fmt_src_ptr, fmt_dst_ptr);
	}

	double         ay, by;
	double         ac, bc;
	const int      ch_plane = (fmt_dst_ptr->numPlanes > 1) ? 1 : 0;
	vsutl::compute_fmt_mac_cst (
		ay, by, *fmt_dst_ptr, full_flag, *fmt_src_ptr, full_flag, 0
	);
	vsutl::compute_fmt_mac_cst (
		ac, bc, *fmt_dst_ptr, full_flag, *fmt_src_ptr, full_flag, ch_plane
	);

	m[0][0] = ay; m[0][1] =  0; m[0][2] =  0; m[0][3] = by;
	m[1][0] =  0; m[1][1] = ac; m[1][2] =  0; m[1][3] = bc;
	m[2][0] =  0; m[2][1] =  0; m[2][2] = ac; m[2][3] = bc;
	m[3][0] =  0; m[3][1] =  0; m[3][2] =  0; m[3][3] =  1;
}



void	Matrix::complete_mat3 (Mat4 &m)
{
	assert (&m != 0);

	                                       m[0][3] = 0;
	                                       m[1][3] = 0;
	                                       m[2][3] = 0;
	m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
