/*****************************************************************************

        Transfer.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcavs/CpuOpt.h"
#include "fmtcavs/fnc.h"
#include "fmtcavs/function_names.h"
#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/Transfer.h"
#include "fmtcl/TransModel.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransUtil.h"
#include "fstb/fnc.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Transfer::Transfer (::IScriptEnvironment &env, const ::AVSValue &args)
:	Inherited (env, args [Param_CLIP_SRC].AsClip ())
,	_clip_src_sptr (args [Param_CLIP_SRC].AsClip ())
,	_vi_src (vi)
,	_fulls_flag (args [Param_FULLS].AsBool (true))
,	_fulld_flag (args [Param_FULLD].AsBool (true))
{
	const CpuOpt   cpu_opt (args [Param_CPUOPT]);
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (! _vi_src.IsPlanar ())
	{
		env.ThrowError (fmtcavs_TRANSFER ": input must be planar.");
	}
	const FmtAvs   fmt_src (_vi_src);
	const auto     col_fam_src = fmt_src.get_col_fam ();
	if (   col_fam_src != fmtcl::ColorFamily_GRAY
	    && col_fam_src != fmtcl::ColorFamily_RGB)
	{
		env.ThrowError (fmtcavs_TRANSFER ": unsupported color family.");
	}
	const int      res_src      = fmt_src.get_bitdepth ();
	const bool     flt_src_flag = fmt_src.is_float ();
	if (   (! flt_src_flag && (res_src < 8 || res_src > 16))
	    || (  flt_src_flag &&  res_src != 32               ))
	{
		env.ThrowError (fmtcavs_TRANSFER ": pixel bitdepth not supported.");
	}

	// Destination colorspace
	const auto     fmt_dst = get_output_colorspace (env, args, fmt_src);
	const int      res_dst      = fmt_dst.get_bitdepth ();
	const bool     flt_dst_flag = fmt_dst.is_float ();
	if (   (! flt_dst_flag && res_dst != 16)
	    || (  flt_dst_flag && res_dst != 32))
	{
		env.ThrowError (fmtcavs_TRANSFER ": output bitdepth not supported.");
	}

	// Output format is validated.
	if (fmt_dst.conv_to_vi (vi) != 0)
	{
		env.ThrowError (fmtcavs_TRANSFER ": illegal output colorspace.");
	}

	// Alpha plane processing, if any
	_proc_alpha_uptr = std::make_unique <fmtcavs::ProcAlpha> (
		fmt_dst, fmt_src, vi.width, vi.height, cpu_opt
	);

	// Other parameters
	std::string    transs     = args [Param_TRANSS  ].AsString ("");
	std::string    transd     = args [Param_TRANSD  ].AsString ("");
	const double   contrast   = args [Param_CONT    ].AsFloat (1);
	const double   gcor       = args [Param_GCOR    ].AsFloat (1);
	double         lvl_black  = args [Param_BLACKLVL].AsFloat (0);
	double         lb         = args [Param_LB      ].AsFloat (0);
	const double   lw         = args [Param_LW      ].AsFloat (0);
	const double   lws        = args [Param_LWS     ].AsDblDef (lw);
	const double   lwd        = args [Param_LWD     ].AsDblDef (lw);
	const double   lamb       = args [Param_AMBIENT ].AsFloat (5);
	const bool     scene_flag = args [Param_SCENEREF].AsBool (false);
	const int      logc_ei_raw_s = args [Param_LOGCEIS].AsInt (800);
	const int      logc_ei_raw_d = args [Param_LOGCEID].AsInt (800);
	const auto     match      = fmtcl::LumMatch (
		args [Param_MATCH].AsInt (fmtcl::LumMatch_REF_WHITE)
	);
	const int      dbg        = args [Param_DEBUG   ].AsInt (0);
	const bool     gydef_flag = args [Param_GY      ].Defined ();
	const bool     gy_flag    = args [Param_GY      ].AsBool (false);
	const auto     gy_proc    =
		  (! gydef_flag) ? fmtcl::TransModel::GyProc::UNDEF
		: gy_flag        ? fmtcl::TransModel::GyProc::ON
		:                  fmtcl::TransModel::GyProc::OFF;
	const double   sig_c      = args [Param_SIG_C   ].AsFloat (6.5f);
	const double   sig_t      = args [Param_SIG_T   ].AsFloat (0.5f);

	fstb::conv_to_lower_case (transs);
	fstb::conv_to_lower_case (transd);

	_curve_s = fmtcl::TransUtil::conv_string_to_curve (transs);
	if (_curve_s == fmtcl::TransCurve_UNDEF)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid transs value.");
	}
	_curve_d = fmtcl::TransUtil::conv_string_to_curve (transd);
	if (_curve_d == fmtcl::TransCurve_UNDEF)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid transd value.");
	}

	const auto     logc_ei_s = fmtcl::TransOpLogC::conv_logc_ei (logc_ei_raw_s);
	if (logc_ei_s == fmtcl::TransOpLogC::ExpIdx_INVALID)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid logceis value.");
	}
	const auto     logc_ei_d = fmtcl::TransOpLogC::conv_logc_ei (logc_ei_raw_d);
	if (logc_ei_d == fmtcl::TransOpLogC::ExpIdx_INVALID)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid logceid value.");
	}

	if (contrast <= 0)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid cont value.");
	}
	if (gcor <= 0)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid gcor value.");
	}

	if (lws > 0 && lws < fmtcl::TransModel::_min_luminance)
	{
		env.ThrowError (fmtcavs_TRANSFER ": lws must be 0 or >= 0.1.");
	}
	if (lwd > 0 && lwd < fmtcl::TransModel::_min_luminance)
	{
		env.ThrowError (fmtcavs_TRANSFER ": lwd must be 0 or >= 0.1.");
	}

	// Lb Lw mess
	if (lvl_black < 0)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid blacklvl value.");
	}
	if (lws > 0 && lb >= lws)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid lb/lws combination.");
	}
	if (   args [Param_BLACKLVL].Defined ()
	    && args [Param_LB      ].Defined ()
	    && (args [Param_LWS].Defined () || args [Param_LW].Defined ()))
	{
		env.ThrowError (fmtcavs_TRANSFER
			": you can define at most two of these parameters:\n"
			"blacklvl, lb and (lw or lws)."
		);
	}
	else if (! args [Param_LB].Defined ())
	{
		if (args [Param_LW].Defined () || args [Param_LWS].Defined ())
		{
			lb = lvl_black * lws;
		}
		else
		{
			lb = lvl_black * 100;
		}
	}

	if (lamb < fmtcl::TransModel::_min_luminance)
	{
		env.ThrowError (fmtcavs_TRANSFER ": ambient luminance must be > 0.1.");
	}

	if (match < 0 || match >= fmtcl::LumMatch_NBR_ELT)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid match value.");
	}

	if (dbg < 0)
	{
		env.ThrowError (fmtcavs_TRANSFER ": debug must be >= 0.");
	}
	_dbg_flag = (dbg > 0);
	if (_dbg_flag)
	{
		_dbg_name = fmtcl::TransUtil::gen_degub_prop_name (dbg);
	}

	if (   _curve_s == fmtcl::TransCurve_SIGMOID
	    || _curve_d == fmtcl::TransCurve_SIGMOID)
	{
		if (sig_c <= 0 || sig_c > 10)
		{
			env.ThrowError (fmtcavs_TRANSFER ": sig_c must be in range [0.1 ; 10].");
		}
		if (sig_t < 0 || sig_t > 1)
		{
			env.ThrowError (fmtcavs_TRANSFER ": sig_t must be in range [0 ; 1].");
		}
	}

	// Finally...
	const fmtcl::PicFmt  src_picfmt =
		conv_fmtavs_to_picfmt (fmt_src, _fulls_flag);
	const fmtcl::PicFmt  dst_picfmt =
		conv_fmtavs_to_picfmt (fmt_dst, _fulld_flag);
	_model_uptr = std::make_unique <fmtcl::TransModel> (
		dst_picfmt, _curve_d, logc_ei_d,
		src_picfmt, _curve_s, logc_ei_s,
		contrast, gcor, lb, lws, lwd, lamb, scene_flag, match, gy_proc,
		sig_c, sig_t,
		sse2_flag, avx2_flag
	);
}



::PVideoFrame __stdcall	Transfer::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	const auto     pa { build_mat_proc (vi, dst_sptr, _vi_src, src_sptr) };
	_model_uptr->process_frame (pa);

	// Alpha plane now
	_proc_alpha_uptr->process_plane (dst_sptr, src_sptr);

	// Frame properties
	if (supports_props ())
	{
		::AVSMap *     props_ptr = env_ptr->getFramePropsRW (dst_sptr);

		const int      cr_val = (_fulld_flag) ? 0 : 1;
		env_ptr->propSetInt (
			props_ptr, "_ColorRange", cr_val, ::PROPAPPENDMODE_REPLACE
		);

		int            transfer = fmtcl::TransCurve_UNSPECIFIED;
		if (_curve_d >= 0 && _curve_d <= fmtcl::TransCurve_ISO_RANGE_LAST)
		{
			transfer = _curve_d;
		}
		env_ptr->propSetInt (
			props_ptr, "_Transfer", transfer, ::PROPAPPENDMODE_REPLACE
		);

		if (_dbg_flag)
		{
			const std::string &  txt = _model_uptr->get_debug_text ();
			env_ptr->propSetData (
				props_ptr, _dbg_name.c_str (),
				txt.c_str (), int (txt.length () + 1), ::PROPAPPENDMODE_REPLACE
			);
		}
	}

	return dst_sptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FmtAvs	Transfer::get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src)
{
	auto           fmt_dst      = fmt_src;

	bool           flt_flag     = fmt_dst.is_float ();
	int            res          = fmt_dst.get_bitdepth ();
	flt_flag = args [Param_FLT ].AsBool (flt_flag);
	res      = args [Param_BITS].AsInt (res);
	const bool     flt_def_flag = args [Param_FLT ].Defined ();
	const bool     res_def_flag = args [Param_BITS].Defined ();

	if (! flt_def_flag && ! res_def_flag)
	{
		if (! flt_flag && res < 16)
		{
			fmt_dst.set_bitdepth (16);
		}
	}
	else if (flt_def_flag && ! res_def_flag)
	{
		fmt_dst.set_bitdepth (32);
	}
	else if (! flt_def_flag && res_def_flag)
	{
		fmt_dst.set_bitdepth (res);
	}
	else
	{
		assert (flt_def_flag && res_def_flag);
		if (   (  flt_flag && res != 32)
		    || (! flt_flag && res >  16))
		{
			env.ThrowError (
				fmtcavs_TRANSFER ": flt and bits combination not supported."
			);
		}
		fmt_dst.set_bitdepth (res);
	}

	return fmt_dst;
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
