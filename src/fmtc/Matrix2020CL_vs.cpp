/*****************************************************************************

        Matrix2020CL.cpp
        Author: Laurent de Soras, 2013

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
#include "fmtc/fnc.h"
#include "fmtc/Matrix2020CL.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/TransCurve.h"
#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix2020CL::Matrix2020CL (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "matrix2020cl", ::fmParallel)
,	_clip_src_sptr (vsapi.mapGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_range_set_flag (false)
,	_full_range_flag (false)
,	_to_yuv_flag (false)
,	_proc_uptr ()
{
	fstb::unused (user_data_ptr);

	const fmtc::CpuOpt   cpu_opt (*this, in, out);
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::unique_ptr <fmtcl::Matrix2020CLProc> (
		new fmtcl::Matrix2020CLProc (sse2_flag, avx2_flag)
	);

	// Checks the input clip
	if (! vsutl::is_constant_format (_vi_in))
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const auto &   fmt_src = _vi_in.format;

	if (fmt_src.subSamplingW != 0 || fmt_src.subSamplingH != 0)
	{
		throw_inval_arg ("input must be 4:4:4.");
	}
	if (fmt_src.numPlanes != _nbr_planes_proc)
	{
		throw_inval_arg ("greyscale format not supported.");
	}
	if (   ! vsutl::is_vs_rgb (fmt_src.colorFamily)
	    && ! vsutl::is_vs_yuv (fmt_src.colorFamily))
	{
		throw_inval_arg ("Only RGB and YUV color families are supported.");
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
	if (   vsutl::is_vs_rgb (fmt_src.colorFamily)
	    && fmt_src.sampleType    == ::stInteger
		 && fmt_src.bitsPerSample != _rgb_int_bits)
	{
		throw_inval_arg ("input clip: RGB depth cannot be less than 16 bits.");
	}

	// Destination colorspace
	const auto     fmt_dst = get_output_colorspace (in, out, core, fmt_src);

	if (   ! vsutl::is_vs_rgb (fmt_dst.colorFamily)
	    && ! vsutl::is_vs_yuv (fmt_dst.colorFamily))
	{
		throw_inval_arg ("unsupported color family for output.");
	}
	if (   (   fmt_dst.sampleType == ::stInteger
	        && (   fmt_dst.bitsPerSample <  8
	            || fmt_dst.bitsPerSample > 12)
	        && fmt_dst.bitsPerSample != 14
	        && fmt_dst.bitsPerSample != 16)
	    || (   fmt_dst.sampleType == ::stFloat
	        && fmt_dst.bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}
	if (   vsutl::is_vs_rgb (fmt_dst.colorFamily)
	    && fmt_dst.sampleType    == ::stInteger
	    && fmt_dst.bitsPerSample != _rgb_int_bits)
	{
		throw_inval_arg ("output clip: RGB depth cannot be less than 16 bits.");
	}

	// Compatibility
	if (   fmt_dst.sampleType   != fmt_src.sampleType
	    || fmt_dst.subSamplingW != fmt_src.subSamplingW
	    || fmt_dst.subSamplingH != fmt_src.subSamplingH
	    || fmt_dst.numPlanes    != fmt_src.numPlanes)
	{
		throw_inval_arg (
			"specified output colorspace is not compatible with the input."
		);
	}
	if (vsutl::is_vs_same_colfam (fmt_dst.colorFamily, fmt_src.colorFamily))
	{
		throw_inval_arg (
			"Input and output clips must be of different color families."
		);
	}

	// Output format is validated.
	_vi_out.format = fmt_dst;
	_to_yuv_flag   = vsutl::is_vs_yuv (fmt_dst.colorFamily);

	// Range
	const auto &   fmt_yuv = (_to_yuv_flag) ? fmt_dst : fmt_src;
	_full_range_flag = (get_arg_int (
		in, out, "full" ,
		vsutl::is_full_range_default (fmt_yuv) ? 1 : 0,
		0, &_range_set_flag
	) != 0);

	const fmtcl::SplFmt  splfmt_src = conv_vsfmt_to_splfmt (fmt_src);
	const fmtcl::SplFmt  splfmt_dst = conv_vsfmt_to_splfmt (fmt_dst);
	const fmtcl::Matrix2020CLProc::Err  ret_val =  _proc_uptr->configure (
		_to_yuv_flag,
		splfmt_src, fmt_src.bitsPerSample,
		splfmt_dst, fmt_dst.bitsPerSample,
		_full_range_flag
	);

	if (ret_val != fmtcl::Matrix2020CLProc::Err_OK)
	{
		if (ret_val == fmtcl::Matrix2020CLProc::Err_INVALID_FORMAT_COMBINATION)
		{
			throw_inval_arg ("invalid frame format combination.");
		}
		else
		{
			assert (false);
			throw_inval_arg ("unidentified error while building the matrix.");
		}
	}
}



::VSVideoInfo	Matrix2020CL::get_video_info () const
{
	return _vi_out;
}



std::vector <::VSFilterDependency>	Matrix2020CL::get_dependencies () const
{
	return std::vector <::VSFilterDependency> {
		{ &*_clip_src_sptr, ::rpStrictSpatial }
	};
}



const ::VSFrame *	Matrix2020CL::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	fstb::unused (frame_data_ptr);

	assert (n >= 0);

	::VSFrame *    dst_ptr = 0;
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

		const auto     pa { build_mat_proc (_vsapi, *dst_ptr, src) };
		_proc_uptr->process (pa);

		// Output frame properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropertiesRW (dst_ptr));

		const fmtcl::ColorSpaceH265   cs_out =
			  (_to_yuv_flag)
			? fmtcl::ColorSpaceH265_BT2020CL
			: fmtcl::ColorSpaceH265_RGB;
		_vsapi.mapSetInt (&dst_prop, "_ColorSpace", cs_out, ::maReplace);
		_vsapi.mapSetInt (&dst_prop, "_Matrix"    , cs_out, ::maReplace);

		const auto     curve =
			  (! _to_yuv_flag) ?                     fmtcl::TransCurve_LINEAR
			: (_vi_out.format.bitsPerSample <= 10) ? fmtcl::TransCurve_2020_10
			:                                        fmtcl::TransCurve_2020_12;
		_vsapi.mapSetInt (&dst_prop, "_Transfer", int (curve), ::maReplace);

		if (! _to_yuv_flag || _range_set_flag)
		{
			const int      cr_val = (! _to_yuv_flag || _full_range_flag) ? 0 : 1;
			_vsapi.mapSetInt (&dst_prop, "_ColorRange", cr_val, ::maReplace);
		}
	}

	return dst_ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	Matrix2020CL::_nbr_planes_proc;
constexpr int	Matrix2020CL::_rgb_int_bits;



::VSVideoFormat	Matrix2020CL::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src) const
{
	auto           fmt_dst  = fmt_src;
	int            col_fam  = fmt_dst.colorFamily;
	int            bits     = fmt_dst.bitsPerSample;
	int            spl_type = fmt_dst.sampleType;

	// Automatic default conversion
	if (vsutl::is_vs_rgb (col_fam))
	{
		col_fam = ::cfYUV;
	}
	else
	{
		col_fam = ::cfRGB;
		if (spl_type == ::stInteger)
		{
			bits = _rgb_int_bits;
		}
	}

	// Full colorspace
	int            csp_dst = get_arg_int (in, out, "csp", ::pfNone);
	if (csp_dst != ::pfNone)
	{
		const auto     gvfbi_ret =
			_vsapi.getVideoFormatByID (&fmt_dst, csp_dst, &core);
		if (gvfbi_ret == 0)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
		col_fam  = fmt_dst.colorFamily;
		bits     = fmt_dst.bitsPerSample;
		spl_type = fmt_dst.sampleType;
	}

	int            ssh = fmt_dst.subSamplingW;
	int            ssv = fmt_dst.subSamplingH;

	// Destination bit depth
	bits = get_arg_int (in, out, "bits", bits);

	// Combines the modified parameters and validates the format
	bool           ok_flag = true;
	try
	{
		ok_flag = register_format (
			fmt_dst,
			col_fam, spl_type, bits, ssh, ssv,
			core
		);
	}
	catch (std::exception &)
	{
		throw;
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

	return fmt_dst;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
