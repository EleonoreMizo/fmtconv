/*****************************************************************************

        Transfer.cpp
        Author: Laurent de Soras, 2015

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

#include "fmtc/CpuOpt.h"
#include "fmtc/Transfer.h"
#include "fmtc/fnc.h"
#include "fmtcl/TransUtil.h"
#include "fstb/fnc.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Transfer::Transfer (const ::VSMap &in, ::VSMap &out, void * /*user_data_ptr*/, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "transfer", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_sse2_flag (false)
,	_avx2_flag (false)
,	_transs (get_arg_str (in, out, "transs", ""))
,	_transd (get_arg_str (in, out, "transd", ""))
,	_contrast (get_arg_flt (in, out, "cont", 1))
,	_gcor (get_arg_flt (in, out, "gcor", 1))
,	_lvl_black (get_arg_flt (in, out, "blacklvl", 0))
,	_full_range_src_flag (get_arg_int (in, out, "fulls", 1) != 0)
,	_full_range_dst_flag (get_arg_int (in, out, "fulld", 1) != 0)
,	_curve_s (fmtcl::TransCurve_UNDEF)
,	_curve_d (fmtcl::TransCurve_UNDEF)
,	_logc_ei_s (fmtcl::TransOpLogC::ExpIdx_800)
,	_logc_ei_d (fmtcl::TransOpLogC::ExpIdx_800)
,	_loglut_flag (false)
#if defined (_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4355)
#endif // 'this': used in base member initializer list
,	_plane_processor (vsapi, *this, "transfer", true)
#if defined (_MSC_VER)
#pragma warning (pop)
#endif
,	_lut_uptr ()
{
	fstb::conv_to_lower_case (_transs);
	fstb::conv_to_lower_case (_transd);

	const fmtc::CpuOpt   cpu_opt (*this, in, out);
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const ::VSFormat &   fmt_src = *_vi_in.format;

	if (   ! vsutl::is_vs_gray (fmt_src.colorFamily)
	    && ! vsutl::is_vs_rgb ( fmt_src.colorFamily))
	{
		throw_inval_arg ("unsupported color family.");
	}
	if (   (   fmt_src.sampleType == ::stInteger
	        && (   fmt_src.bitsPerSample <  8
	            || fmt_src.bitsPerSample > 16))
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_src.bitsPerSample != 32))
	{
		throw_inval_arg ("pixel bitdepth not supported.");
	}

	// Destination colorspace
	const ::VSFormat& fmt_dst =
		get_output_colorspace (in, out, core, fmt_src);

	if (   (   fmt_dst.sampleType == ::stInteger
	        && fmt_dst.bitsPerSample != 16)
	    || (   fmt_dst.sampleType == ::stFloat
	        && fmt_dst.bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}

	// Output format is validated.
	_vi_out.format = &fmt_dst;

	// Other parameters
	_curve_s = fmtcl::TransUtil::conv_string_to_curve (_transs);
	if (_curve_s == fmtcl::TransCurve_UNDEF)
	{
		throw_inval_arg ("invalid transs value.");
	}
	_curve_d = fmtcl::TransUtil::conv_string_to_curve (_transd);
	if (_curve_d == fmtcl::TransCurve_UNDEF)
	{
		throw_inval_arg ("invalid transd value.");
	}

	const int      logc_ei_raw_s = get_arg_int (in, out, "logceis", 800);
	_logc_ei_s = fmtcl::TransOpLogC::conv_logc_ei (logc_ei_raw_s);
	if (_logc_ei_s == fmtcl::TransOpLogC::ExpIdx_INVALID)
	{
		throw_inval_arg ("invalid logceis value.");
	}

	const int      logc_ei_raw_d = get_arg_int (in, out, "logceid", 800);
	_logc_ei_d = fmtcl::TransOpLogC::conv_logc_ei (logc_ei_raw_d);
	if (_logc_ei_d == fmtcl::TransOpLogC::ExpIdx_INVALID)
	{
		throw_inval_arg ("invalid logceid value.");
	}

	if (_contrast <= 0)
	{
		throw_inval_arg ("invalid cont value.");
	}
	if (_gcor <= 0)
	{
		throw_inval_arg ("invalid gcor value.");
	}
	if (_lvl_black < 0)
	{
		throw_inval_arg ("invalid blacklvl value.");
	}

	// Finally...
	const fmtcl::PicFmt  src_fmt =
		conv_vsfmt_to_picfmt (*_vi_in.format , _full_range_src_flag);
	const fmtcl::PicFmt  dst_fmt =
		conv_vsfmt_to_picfmt (*_vi_out.format, _full_range_dst_flag);
	_lut_uptr = fmtcl::TransUtil::build_lut (
		dst_fmt, _curve_d, _logc_ei_d,
		src_fmt, _curve_s, _logc_ei_s,
		_contrast, _gcor, _lvl_black,
		_sse2_flag, _avx2_flag
	);
}



void	Transfer::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
	_plane_processor.set_filter (in, out, _vi_out);
}



const ::VSFrameRef *	Transfer::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
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

		const int         w  =  _vsapi.getFrameWidth (&src, 0);
		const int         h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		const int      ret_val = _plane_processor.process_frame (
			*dst_ptr, n, frame_data_ptr, frame_ctx, core, _clip_src_sptr
		);

		if (ret_val == 0)
		{
			// Output frame properties
			::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

			const int      cr_val = (_full_range_dst_flag) ? 0 : 1;
			_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);

			int            transfer = fmtcl::TransCurve_UNSPECIFIED;
			if (_curve_d >= 0 && _curve_d <= fmtcl::TransCurve_ISO_RANGE_LAST)
			{
				transfer = _curve_d;
			}
			_vsapi.propSetInt (&dst_prop, "_Transfer", transfer, ::paReplace);
		}

		if (ret_val != 0)
		{
			_vsapi.freeFrame (dst_ptr);
			dst_ptr = 0;
		}
	}

	return dst_ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	Transfer::do_process_plane (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr)
{
	fstb::unused (frame_data_ptr, core, src_node2_sptr, src_node3_sptr);
	assert (src_node1_sptr.get () != 0);

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

		_lut_uptr->process_plane (
			data_dst_ptr, data_src_ptr, stride_dst, stride_src, w, h
		);
	}

	return (ret_val);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat &	Transfer::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const
{
	const ::VSFormat *   fmt_dst_ptr = &fmt_src;

	const int      undef    = -666666666;
	const int      dst_flt  = get_arg_int (in, out, "flt" , undef);
	const int      dst_bits = get_arg_int (in, out, "bits", undef);

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
