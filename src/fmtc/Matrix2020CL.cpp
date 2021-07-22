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

#include "fmtcl/ColorSpaceH265.h"
#include "fmtc/fnc.h"
#include "fmtc/Matrix2020CL.h"
#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix2020CL::Matrix2020CL (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "matrix2020cl", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_range_set_flag (false)
,	_full_range_flag (false)
,	_to_yuv_flag (false)
,	_proc_uptr ()
{
	fstb::unused (user_data_ptr);

	vsutl::CpuOpt  cpu_opt (*this, in, out);
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::unique_ptr <fmtcl::Matrix2020CLProc> (
		new fmtcl::Matrix2020CLProc (sse2_flag, avx2_flag)
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
	if (fmt_src.numPlanes != NBR_PLANES)
	{
		throw_inval_arg ("greyscale format not supported.");
	}
	if (   fmt_src.colorFamily != ::cmRGB
	    && fmt_src.colorFamily != ::cmYUV)
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
	if (   fmt_src.colorFamily   == ::cmRGB
	    && fmt_src.sampleType    == ::stInteger
		 && fmt_src.bitsPerSample != RGB_INT_BITS)
	{
		throw_inval_arg ("input clip: RGB depth cannot be less than 16 bits.");
	}

	// Destination colorspace
	const ::VSFormat& fmt_dst = get_output_colorspace (in, out, core, fmt_src);

	if (   fmt_dst.colorFamily != ::cmRGB
	    && fmt_dst.colorFamily != ::cmYUV)
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
	if (   fmt_src.colorFamily   == ::cmRGB
	    && fmt_src.sampleType    == ::stInteger
		 && fmt_src.bitsPerSample != RGB_INT_BITS)
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
	if (fmt_dst.colorFamily == fmt_src.colorFamily)
	{
		throw_inval_arg (
			"Input and output clips must be of different color families."
		);
	}

	// Output format is validated.
	_vi_out.format = &fmt_dst;
	_to_yuv_flag   = (fmt_dst.colorFamily == ::cmYUV);

	// Range
	const ::VSFormat &   fmt_yuv = (_to_yuv_flag) ? fmt_dst : fmt_src;
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



void	Matrix2020CL::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (in, out, core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Matrix2020CL::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
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

		const int      w  =  _vsapi.getFrameWidth (&src, 0);
		const int      h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		uint8_t * const   dst_ptr_arr [fmtcl::Matrix2020CLProc::NBR_PLANES] =
		{
			_vsapi.getWritePtr (dst_ptr, 0),
			_vsapi.getWritePtr (dst_ptr, 1),
			_vsapi.getWritePtr (dst_ptr, 2)
		};
		const int         dst_str_arr [fmtcl::Matrix2020CLProc::NBR_PLANES] =
		{
			_vsapi.getStride (dst_ptr, 0),
			_vsapi.getStride (dst_ptr, 1),
			_vsapi.getStride (dst_ptr, 2)
		};
		const uint8_t * const
		                  src_ptr_arr [fmtcl::Matrix2020CLProc::NBR_PLANES] =
		{
			_vsapi.getReadPtr (&src, 0),
			_vsapi.getReadPtr (&src, 1),
			_vsapi.getReadPtr (&src, 2)
		};
		const int         src_str_arr [fmtcl::Matrix2020CLProc::NBR_PLANES] =
		{
			_vsapi.getStride (&src, 0),
			_vsapi.getStride (&src, 1),
			_vsapi.getStride (&src, 2)
		};

		_proc_uptr->process (
			dst_ptr_arr, dst_str_arr,
			src_ptr_arr, src_str_arr,
			w, h
		);

		// Output frame properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

		const fmtcl::ColorSpaceH265   cs_out =
			  (_to_yuv_flag)
			? fmtcl::ColorSpaceH265_BT2020CL
			: fmtcl::ColorSpaceH265_RGB;
		_vsapi.propSetInt (&dst_prop, "_ColorSpace", cs_out, ::paReplace);

		if (! _to_yuv_flag || _range_set_flag)
		{
			const int      cr_val = (! _to_yuv_flag || _full_range_flag) ? 0 : 1;
			_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);
		}
	}

	return dst_ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat &	Matrix2020CL::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const
{
	const ::VSFormat *   fmt_dst_ptr = &fmt_src;
	int            col_fam  = fmt_dst_ptr->colorFamily;
	int            bits     = fmt_dst_ptr->bitsPerSample;
	int            spl_type = fmt_dst_ptr->sampleType;

	// Automatic default conversion
	if (col_fam == ::cmRGB)
	{
		col_fam = ::cmYUV;
	}
	else
	{
		col_fam = ::cmRGB;
		if (spl_type == ::stInteger)
		{
			bits = RGB_INT_BITS;
		}
	}

	// Full colorspace
	int            csp_dst = get_arg_int (in, out, "csp", ::pfNone);
	if (csp_dst != ::pfNone)
	{
		fmt_dst_ptr = _vsapi.getFormatPreset (csp_dst, &core);
		if (fmt_dst_ptr == 0)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
		col_fam  = fmt_dst_ptr->colorFamily;
		bits     = fmt_dst_ptr->bitsPerSample;
		spl_type = fmt_dst_ptr->sampleType;
	}

	int            ssh = fmt_dst_ptr->subSamplingW;
	int            ssv = fmt_dst_ptr->subSamplingH;

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

	return *fmt_dst_ptr;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
