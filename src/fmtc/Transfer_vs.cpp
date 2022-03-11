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
#include "fmtcl/TransModel.h"
#include "fmtcl/TransOpLogC.h"
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
:	vsutl::FilterBase (vsapi, "transfer", ::fmParallel)
,	_clip_src_sptr (vsapi.mapGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_transs (get_arg_str (in, out, "transs", ""))
,	_transd (get_arg_str (in, out, "transd", ""))
,	_contrast (get_arg_flt (in, out, "cont", 1))
,	_gcor (get_arg_flt (in, out, "gcor", 1))
,	_full_range_src_flag (get_arg_int (in, out, "fulls", 1) != 0)
,	_full_range_dst_flag (get_arg_int (in, out, "fulld", 1) != 0)
{
	fstb::conv_to_lower_case (_transs);
	fstb::conv_to_lower_case (_transd);

	const fmtc::CpuOpt   cpu_opt (*this, in, out);
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (! vsutl::is_constant_format (_vi_in))
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const auto &   fmt_src = _vi_in.format;

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
	auto           fmt_dst = get_output_colorspace (in, out, core, fmt_src);

	if (   (   fmt_dst.sampleType == ::stInteger
	        && fmt_dst.bitsPerSample != 16)
	    || (   fmt_dst.sampleType == ::stFloat
	        && fmt_dst.bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}

	// Output format is validated.
	_vi_out.format = fmt_dst;

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

	const bool     scene_flag = (get_arg_int (in, out, "sceneref", 0) != 0);

	bool           lw_def_flag  = false;
	bool           lws_def_flag = false;
	double         lw  = get_arg_flt (in, out, "lw"  , 0 , 0, &lw_def_flag);
	double         lws = get_arg_flt (in, out, "lws", lw, 0, &lws_def_flag);
	double         lwd = get_arg_flt (in, out, "lwd", lw);
	if (lws > 0 && lws < fmtcl::TransModel::_min_luminance)
	{
		throw_inval_arg ("lws must be >= 0.1.");
	}
	if (lwd > 0 && lwd < fmtcl::TransModel::_min_luminance)
	{
		throw_inval_arg ("lwd must be >= 0.1.");
	}

	// Lb Lw mess
	bool           bl_def_flag = false; // lvl_black
	bool           lb_def_flag = false;
	double         lvl_black   =
		get_arg_flt (in, out, "blacklvl", 0, 0, &bl_def_flag);
	double         lb = get_arg_flt (in, out, "lb", 0, 0, &lb_def_flag);
	if (lvl_black < 0)
	{
		throw_inval_arg ("invalid blacklvl value.");
	}
	if (lw > 0 && lb >= lws)
	{
		throw_inval_arg ("invalid lb/lw combination.");
	}
	if (   bl_def_flag
	    && lb_def_flag
	    && (lws_def_flag || lw_def_flag))
	{
		throw_inval_arg (
			"you can define at most two of these parameters: "
			"blacklvl, lb and (lw or lws)."
		);
	}
	else if (! lb_def_flag)
	{
		if (lw_def_flag || lws_def_flag)
		{
			lb = lvl_black * lws;
		}
		else
		{
			lb = lvl_black * 100;
		}
	}

	const double   lamb = get_arg_flt (in, out, "ambient", 5);
	if (lamb < fmtcl::TransModel::_min_luminance)
	{
		throw_inval_arg ("ambient luminance must be >= 0.1.");
	}

	const auto     match = fmtcl::LumMatch (
		get_arg_int (in, out, "match", fmtcl::LumMatch_REF_WHITE)
	);
	if (match < 0 || match >= fmtcl::LumMatch_NBR_ELT)
	{
		throw_inval_arg ("invalid match value.");
	}

	const int      gy_val  = get_arg_int (in, out, "gy", -1);
	const auto     gy_proc =
		  (gy_val == 0) ? fmtcl::TransModel::GyProc::OFF
		: (gy_val == 1) ? fmtcl::TransModel::GyProc::ON
		:                 fmtcl::TransModel::GyProc::UNDEF;

	const int      dbg = get_arg_int (in, out, "debug", 0);
	if (dbg < 0)
	{
		throw_inval_arg ("debug must be >= 0.");
	}
	_dbg_flag = (dbg > 0);
	if (_dbg_flag)
	{
		_dbg_name = fmtcl::TransUtil::gen_degub_prop_name (dbg);
	}

	const auto     sig_c = get_arg_flt (in, out, "sig_c", 6.5);
	const auto     sig_t = get_arg_flt (in, out, "sig_t", 0.5);
	if (   _curve_s == fmtcl::TransCurve_SIGMOID
	    || _curve_d == fmtcl::TransCurve_SIGMOID)
	{
		if (sig_c <= 0 || sig_c > 10)
		{
			throw_inval_arg ("sig_c must be in range [0.1 ; 10].");
		}
		if (sig_t < 0 || sig_t > 1)
		{
			throw_inval_arg ("sig_t must be in range [0 ; 1].");
		}
	}

	// Finally...
	const fmtcl::PicFmt  src_fmt =
		conv_vsfmt_to_picfmt (_vi_in.format , _full_range_src_flag);
	const fmtcl::PicFmt  dst_fmt =
		conv_vsfmt_to_picfmt (_vi_out.format, _full_range_dst_flag);
	_model_uptr = std::make_unique <fmtcl::TransModel> (
		dst_fmt, _curve_d, _logc_ei_d,
		src_fmt, _curve_s, _logc_ei_s,
		_contrast, _gcor, lb, lws, lwd, lamb, scene_flag, match, gy_proc,
		sig_c, sig_t,
		_sse2_flag, _avx2_flag
	);
}



::VSVideoInfo	Transfer::get_video_info () const
{
	return _vi_out;
}



std::vector <::VSFilterDependency>	Transfer::get_dependencies () const
{
	return std::vector <::VSFilterDependency> {
		{ &*_clip_src_sptr, ::rpStrictSpatial }
	};
}



const ::VSFrame *	Transfer::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
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

		const int         w  =  _vsapi.getFrameWidth (&src, 0);
		const int         h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (&_vi_out.format, w, h, &src, &core);

		const auto     pa { build_mat_proc (_vsapi, *dst_ptr, src) };
		_model_uptr->process_frame (pa);

		// Output frame properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropertiesRW (dst_ptr));

		const int      cr_val = (_full_range_dst_flag) ? 0 : 1;
		_vsapi.mapSetInt (&dst_prop, "_ColorRange", cr_val, ::maReplace);

		int            transfer = fmtcl::TransCurve_UNSPECIFIED;
		if (_curve_d >= 0 && _curve_d <= fmtcl::TransCurve_ISO_RANGE_LAST)
		{
			transfer = _curve_d;
		}
		_vsapi.mapSetInt (&dst_prop, "_Transfer", transfer, ::maReplace);

		if (_dbg_flag)
		{
			const std::string &  txt = _model_uptr->get_debug_text ();
			_vsapi.mapSetData (
				&dst_prop, _dbg_name.c_str (),
				txt.c_str (), int (txt.length () + 1), ::dtUtf8, ::maReplace
			);
		}
	}

	return dst_ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



::VSVideoFormat	Transfer::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src) const
{
	auto           fmt_dst  = fmt_src;

	const int      undef    = -666666666;
	const int      dst_flt  = get_arg_int (in, out, "flt" , undef);
	const int      dst_bits = get_arg_int (in, out, "bits", undef);

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
	if (dst_bits == undef)
	{
		// Forces output to 16 bits if input clip is low-bitdepth integer and
		// nothing else is specified
		if (dst_flt == undef && spl_type == ::stInteger && bits < 16)
		{
			bits = 16;
		}
	}
	else
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
	bool           ok_flag = true;
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

	return fmt_dst;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
