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
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransUtil.h"

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
	fmt_dst.conv_to_vi (vi);

	// Configures the plane processor
	_plane_proc_uptr =
		std::make_unique <avsutl::PlaneProcessor> (vi, *this, false);
	_plane_proc_uptr->set_dst_clip_info (avsutl::PlaneProcessor::ClipType_NORMAL);
	_plane_proc_uptr->set_clip_info (
		avsutl::PlaneProcessor::ClipIdx_SRC1,
		_clip_src_sptr,
		avsutl::PlaneProcessor::ClipType_NORMAL
	);
	set_masktools_planes_param (
		*_plane_proc_uptr, env, args [Param_PLANES], fmtcavs_TRANSFER ", planes"
	);

	// Alpha plane is copied instead of processed
	const int      nbr_proc_planes = fmt_dst.get_nbr_comp_non_alpha ();
	if (nbr_proc_planes < vi.NumComponents ())
	{
		const double   mode = _plane_proc_uptr->get_proc_mode (nbr_proc_planes);
		if (mode == double (avsutl::PlaneProcMode_PROCESS))
		{
			_plane_proc_uptr->set_proc_mode (
				nbr_proc_planes, avsutl::PlaneProcMode_COPY1
			);
		}
	}

	// Other parameters
	const std::string transs  = args [Param_TRANSS  ].AsString ("");
	const std::string transd  = args [Param_TRANSD  ].AsString ("");
	const double   contrast   = args [Param_CONT    ].AsFloat (1);
	const double   gcor       = args [Param_GCOR    ].AsFloat (1);
	const double   lvl_black  = args [Param_BLACKLVL].AsFloat (0);
	const int      logc_ei_raw_s = args [Param_LOGCEIS].AsInt (800);
	const int      logc_ei_raw_d = args [Param_LOGCEID].AsInt (800);

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
	if (lvl_black < 0)
	{
		env.ThrowError (fmtcavs_TRANSFER ": invalid blacklvl value.");
	}

	// Finally...
	const fmtcl::PicFmt  src_picfmt =
		conv_fmtavs_to_picfmt (fmt_src, _fulls_flag);
	const fmtcl::PicFmt  dst_picfmt =
		conv_fmtavs_to_picfmt (fmt_dst, _fulld_flag);
	_model_uptr = std::make_unique <fmtcl::TransModel> (
		dst_picfmt, _curve_d, logc_ei_d,
		src_picfmt, _curve_s, logc_ei_s,
		contrast, gcor, lvl_black,
		sse2_flag, avx2_flag
	);
}



::PVideoFrame __stdcall	Transfer::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	_plane_proc_uptr->process_frame (dst_sptr, n, *env_ptr, nullptr);

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
	}

	return dst_sptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Transfer::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr)
{
	fstb::unused (plane_index, ctx_ptr);
	assert (plane_index < _max_nbr_planes_proc);

	::PVideoFrame  src_sptr     = _clip_src_sptr->GetFrame (n, &env);

	uint8_t *      data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int      stride_dst   = dst_sptr->GetPitch (plane_id);
	const uint8_t* data_src_ptr = src_sptr->GetReadPtr (plane_id);
	const int      stride_src   = src_sptr->GetPitch (plane_id);
	const int      w = _plane_proc_uptr->get_width (
		dst_sptr, plane_id, avsutl::PlaneProcessor::ClipIdx_DST
	);
	const int      h = _plane_proc_uptr->get_height (dst_sptr, plane_id);

	// Standard channel
	_model_uptr->process_plane (
		{ data_dst_ptr, stride_dst },
		{ data_src_ptr, stride_src },
		w, h
	);
}



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
	else if (   flt_def_flag && res_def_flag
	         && (   (  flt_flag && res != 32)
	             || (! flt_flag && res >  16)))
	{
		env.ThrowError (
			fmtcavs_TRANSFER ": flt and bits combination not supported."
		);
	}

	return fmt_dst;
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
