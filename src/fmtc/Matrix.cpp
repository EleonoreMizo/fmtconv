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

#include "fmtc/CpuOpt.h"
#include "fmtc/Matrix.h"
#include "fmtc/fnc.h"
#include "fmtcl/MatrixUtil.h"
#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

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
,	_proc_uptr ()
{
	const fmtc::CpuOpt   cpu_opt (*this, in, out);
	_sse_flag  = cpu_opt.has_sse ();
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx_flag  = cpu_opt.has_avx ();
	_avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::make_unique <fmtcl::MatrixProc> (
		_sse_flag, _sse2_flag, _avx_flag, _avx2_flag
	);

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
	if (fmt_src.numPlanes != _nbr_planes)
	{
		throw_inval_arg ("greyscale format not supported as input.");
	}
	if (   (   fmt_src.sampleType == ::stInteger
	        && (   fmt_src.bitsPerSample <  8
	            || fmt_src.bitsPerSample > 12)
	        && fmt_src.bitsPerSample != 14
	        && fmt_src.bitsPerSample != 16)
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_src.bitsPerSample != 32))
	{
		throw_inval_arg ("pixel bitdepth not supported.");
	}

	if (_plane_out >= _nbr_planes)
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

	// Preliminary matrix test: deduces the target color family if unspecified
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
			fstb::conv_to_lower_case (tmp_mat);

			fmtcl::ColorSpaceH265   tmp_csp =
				find_cs_from_mat_str (*this, tmp_mat, false);

			fmt_dst_ptr = find_dst_col_fam (tmp_csp, fmt_dst_ptr, fmt_src, core);
		}
	}

	const ::VSFormat &fmt_dst = *fmt_dst_ptr;
	const int      nbr_expected_coef = _nbr_planes * (_nbr_planes + 1);

	bool           mat_init_flag = false;

	// Matrix presets
	std::string    mat (get_arg_str (in, out, "mat", ""));
	std::string    mats ((   vsutl::is_vs_yuv ( fmt_src.colorFamily)) ? mat : "");
	std::string    matd ((   vsutl::is_vs_yuv ( fmt_dst.colorFamily)
	                      || vsutl::is_vs_gray (fmt_dst.colorFamily)) ? mat : "");
	mats = get_arg_str (in, out, "mats", mats);
	matd = get_arg_str (in, out, "matd", matd);
	if (! mats.empty () || ! matd.empty ())
	{
		fstb::conv_to_lower_case (mats);
		fstb::conv_to_lower_case (matd);
		const auto     col_fam_src = fmtc::conv_vsfmt_to_colfam (fmt_src);
		const auto     col_fam_dst = fmtc::conv_vsfmt_to_colfam (fmt_dst);
		fmtcl::MatrixUtil::select_def_mat (mats, col_fam_src);
		fmtcl::MatrixUtil::select_def_mat (matd, col_fam_dst);

		fmtcl::Mat4    m2s;
		fmtcl::Mat4    m2d;
		if (fmtcl::MatrixUtil::make_mat_from_str (m2s, mats, true) != 0)
		{
			throw_inval_arg ("unknown source matrix identifier.");
		}
		if (fmtcl::MatrixUtil::make_mat_from_str (m2d, matd, false) != 0)
		{
			throw_inval_arg ("unknown destination matrix identifier.");
		}
		_csp_out = find_cs_from_mat_str (*this, matd, false);

		_mat_main = m2d * m2s;

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

		for (int y = 0; y < _nbr_planes + 1; ++y)
		{
			for (int x = 0; x < _nbr_planes + 1; ++x)
			{
				_mat_main [y] [x] = (x == y) ? 1 : 0;

				if (   (x < fmt_src.numPlanes || x == _nbr_planes)
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

	// Fixes the output colorspace to a valid H265 colorspace
	switch (_csp_out)
	{
	case fmtcl::ColorSpaceH265_LMS:
		_csp_out = fmtcl::ColorSpaceH265_RGB;
		break;
	case fmtcl::ColorSpaceH265_ICTCP_PQ:
	case fmtcl::ColorSpaceH265_ICTCP_HLG:
		_csp_out = fmtcl::ColorSpaceH265_ICTCP;
		break;
	default:
		// Nothing to do
		break;
	}

	// Sets the output colorspace accordingly
	const auto     final_cf =
		fmtcl::MatrixUtil::find_cf_from_cs (_csp_out, true);
	const auto     final_cm =
		fmtc::conv_fmtcl_colfam_to_vs (final_cf);
	fmt_dst_ptr = register_format (
		final_cm,
		fmt_dst_ptr->sampleType,
		fmt_dst_ptr->bitsPerSample,
		fmt_dst_ptr->subSamplingW,
		fmt_dst_ptr->subSamplingH,
		core
	);
	if (fmt_dst_ptr == nullptr)
	{
		throw_rt_err (
			"couldn\'t get a pixel format identifier for the output clip."
		);
	}

	// Checks the output colorspace
	if (   ! vsutl::is_vs_gray (fmt_dst_ptr->colorFamily)
	    && ! vsutl::is_vs_rgb ( fmt_dst_ptr->colorFamily)
	    && ! vsutl::is_vs_yuv ( fmt_dst_ptr->colorFamily)
	    && ::cmYCoCg != fmt_dst_ptr->colorFamily)
	{
		throw_inval_arg ("unsupported color family for output.");
	}
	if (   (   fmt_dst_ptr->sampleType == ::stInteger
	        && (   fmt_dst_ptr->bitsPerSample <  8
	            || fmt_dst_ptr->bitsPerSample > 12)
	        && fmt_dst_ptr->bitsPerSample != 14
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

	// Destination colorspace is validated
	_vi_out.format = fmt_dst_ptr;

	prepare_matrix_coef (
		*this, *_proc_uptr, _mat_main,
		fmt_dst, _full_range_dst_flag,
		fmt_src, _full_range_src_flag,
		_csp_out, _plane_out
	);

	if (_vsapi.getError (&out) != 0)
	{
		throw -1;
	}
}



void	Matrix::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (in, out, core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Matrix::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	fstb::unused (frame_data_ptr);

	assert (n >= 0);

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

		const int      w = _vsapi.getFrameWidth (&src, 0);
		const int      h = _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		const auto     pa { build_mat_proc (
			_vsapi, *dst_ptr, src, (_plane_out >= 0)
		) };
		_proc_uptr->process (pa);

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
			else
			{
				_vsapi.propDeleteKey (&dst_prop, "_Matrix");
				_vsapi.propDeleteKey (&dst_prop, "_ColorSpace");
			}
		}
	}

	return dst_ptr;
}



// mat should be already converted to lower case
fmtcl::ColorSpaceH265	Matrix::find_cs_from_mat_str (const vsutl::FilterBase &flt, const std::string &mat, bool allow_2020cl_flag)
{
	const auto     cs =
		fmtcl::MatrixUtil::find_cs_from_mat_str (mat, allow_2020cl_flag);

	if (cs == fmtcl::ColorSpaceH265_UNDEF)
	{
		flt.throw_inval_arg ("unknown matrix identifier.");
	}

	return cs;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	Matrix::_nbr_planes;



const ::VSFormat *	Matrix::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src, int &plane_out, bool &force_col_fam_flag) const
{
	force_col_fam_flag = false;

	const ::VSFormat *   fmt_dst_ptr = &fmt_src;

	// Full colorspace
	int            csp_dst = get_arg_int (in, out, "csp", ::pfNone);
	if (csp_dst != ::pfNone)
	{
		fmt_dst_ptr = _vsapi.getFormatPreset (csp_dst, &core);
		if (fmt_dst_ptr == nullptr)
		{
			throw_inval_arg ("unknown output colorspace.");
			// The following return statement is never reached, it just prevents
			// false positive when compiling with -Wnull-dereference
			return &fmt_src;
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
	else if (vsutl::is_vs_gray (col_fam))
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
		fmt_dst_ptr = nullptr;
	}

	if (fmt_dst_ptr == nullptr)
	{
		throw_rt_err (
			"couldn\'t get a pixel format identifier for the output clip."
		);
	}

	return fmt_dst_ptr;
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
	case fmtcl::ColorSpaceH265_YDZDX:
	case fmtcl::ColorSpaceH265_CHRODERNCL:
	case fmtcl::ColorSpaceH265_CHRODERCL:
	case fmtcl::ColorSpaceH265_ICTCP:
	case fmtcl::ColorSpaceH265_ICTCP_PQ:
	case fmtcl::ColorSpaceH265_ICTCP_HLG:
		alt_cf = ::cmYUV;
		break;

	case fmtcl::ColorSpaceH265_YCGCO:
		alt_cf = ::cmYCoCg;
		break;

	case fmtcl::ColorSpaceH265_LMS:
		alt_cf = ::cmRGB;
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
		if (vsutl::is_vs_rgb (fmt_src.colorFamily))
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

	return fmt_dst_ptr;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
