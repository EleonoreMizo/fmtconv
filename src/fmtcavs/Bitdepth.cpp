/*****************************************************************************

        Bitdepth.cpp
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

#include "avsutl/fnc.h"
#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/Bitdepth.h"
#include "fmtcavs/CpuOpt.h"
#include "fmtcavs/fnc.h"
#include "fmtcavs/function_names.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Bitdepth::Bitdepth (::IScriptEnvironment &env, const ::AVSValue &args)
:	Inherited (env, args [Param_CLIP_SRC].AsClip ())
,	_clip_src_sptr (args [Param_CLIP_SRC].AsClip ())
,	_vi_src (vi)
{
	const CpuOpt   cpu_opt (args [Param_CPUOPT]);
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	if (! _vi_src.IsPlanar ())
	{
		env.ThrowError (fmtcavs_BITDEPTH ": input must be planar.");
	}

	// Guess the output format if incomplete
	const FmtAvs   fmt_src (_vi_src);
	int            res      = args [Param_BITS].AsInt (-1);
	const auto &   arg_flt  = args [Param_FLT];
	bool           flt_flag = arg_flt.AsBool ();
	if (arg_flt.Defined ())
	{
		if (res < 0 && flt_flag)
		{
			res = 32;
		}
	}
	else
	{
		flt_flag = (res == 32);
	}
	if (res < 0)
	{
		res = _vi_src.BitsPerComponent ();
	}

	// Checks the output format
	if (! (   (! flt_flag && (   res ==  8
	                          || res == 10
	                          || res == 12
	                          || res == 16))
	       || (  flt_flag &&     res == 32 )))
	{
		env.ThrowError (
			fmtcavs_BITDEPTH ": output pixel bitdepth not supported."
		);
	}

	// Builds and validates the output format
	auto           fmt_dst = fmt_src;
	fmt_dst.set_bitdepth (res);
	if (fmt_dst.conv_to_vi (vi) != 0)
	{
		env.ThrowError (fmtcavs_BITDEPTH ": illegal output colorspace.");
	}

	// Conversion-related things
	_fulls_flag     =
		args [Param_FULLS].AsBool (avsutl::is_full_range_default (_vi_src));
	_fulld_flag     = args [Param_FULLD].AsBool (_fulls_flag);
	_range_def_flag =
		(args [Param_FULLS].Defined () || args [Param_FULLD].Defined ());

	// Configures the plane processor
	_plane_proc_uptr =
		std::make_unique <avsutl::PlaneProcessor> (vi, *this, false);
	_plane_proc_uptr->set_dst_clip_info (avsutl::PlaneProcessor::ClipType_NORMAL);
	_plane_proc_uptr->set_clip_info (
		avsutl::PlaneProcessor::ClipIdx_SRC1,
		_clip_src_sptr,
		avsutl::PlaneProcessor::ClipType_NORMAL
	);
	_plane_proc_uptr->set_proc_mode (args [Param_PLANES].AsString ("all"));

	// Dithering parameters
	auto           dmode = static_cast <fmtcl::Dither::DMode> (
		args [Param_DMODE].AsInt (fmtcl::Dither::DMode_FILTERLITE)
	);
	if (dmode == fmtcl::Dither::DMode_ROUND_ALIAS)
	{
		dmode = fmtcl::Dither::DMode_ROUND;
	}
	if (   dmode <  0
	    || dmode >= fmtcl::Dither::DMode_NBR_ELT)
	{
		env.ThrowError (fmtcavs_BITDEPTH ": invalid dmode.");
	}

	const double   ampo = args [Param_AMPO].AsFloat (1.0);
	if (ampo < 0)
	{
		env.ThrowError (fmtcavs_BITDEPTH ": ampo cannot be negative.");
	}

	const double   ampn = args [Param_AMPN].AsFloat (0.0);
	if (ampn < 0)
	{
		env.ThrowError (fmtcavs_BITDEPTH ": ampn cannot be negative.");
	}

	const int      pat_size =
		args [Param_PATSIZE].AsInt (fmtcl::Dither::_max_pat_width);
	if (pat_size < 4 || fmtcl::Dither::_max_pat_width % pat_size != 0)
	{
		env.ThrowError (fmtcavs_BITDEPTH ": wrong value for patsize.");
	}

	const bool     dyn_flag          = args [Param_DYN        ].AsBool (false);
	const bool     static_noise_flag = args [Param_STATICNOISE].AsBool (false);
	const bool     tpdfo_flag        = args [Param_TPDFO      ].AsBool (false);
	const bool     tpdfn_flag        = args [Param_TPDFN      ].AsBool (false);
	const bool     corplane_flag     = args [Param_CORPLANE   ].AsBool (false);

	// Finally...
	const int      nbr_planes = vi.NumComponents ();
	int            res_src;
	fmtcl::SplFmt  splfmt_src;
	conv_vi_to_splfmt (splfmt_src, res_src, _vi_src);
	fmtcl::SplFmt  splfmt_dst = conv_vi_to_splfmt (vi);
	fmtcl::ColorFamily   col_fam = conv_vi_to_colfam (vi);

	_engine_uptr = std::make_unique <fmtcl::Dither> (
		splfmt_src, res_src, _fulls_flag,
		splfmt_dst, res    , _fulld_flag,
		col_fam, nbr_planes, vi.width,
		dmode, pat_size, ampo, ampn,
		dyn_flag, static_noise_flag, corplane_flag,
		tpdfo_flag, tpdfn_flag,
		sse2_flag, avx2_flag
	);
}



::PVideoFrame __stdcall	Bitdepth::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	_plane_proc_uptr->process_frame (dst_sptr, n, *env_ptr, nullptr);

	// Frame properties
	if (supports_props ())
	{
		::AVSMap *     props_ptr = env_ptr->getFramePropsRW (dst_sptr);

		if (_range_def_flag)
		{
			const int      cr_val = (_fulld_flag) ? 0 : 1;
			env_ptr->propSetInt (
				props_ptr, "_ColorRange", cr_val, ::PROPAPPENDMODE_REPLACE
			);
		}
	}

	return dst_sptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Bitdepth::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr)
{
	fstb::unused (ctx_ptr);

	::PVideoFrame  src_sptr     = _clip_src_sptr->GetFrame (n, &env);

	uint8_t *      data_dst_ptr = dst_sptr->GetWritePtr (plane_id);
	const int      stride_dst   = dst_sptr->GetPitch (plane_id);
	const uint8_t* data_src_ptr = src_sptr->GetReadPtr (plane_id);
	const int      stride_src   = src_sptr->GetPitch (plane_id);
	const int      w = _plane_proc_uptr->get_width (
		dst_sptr, plane_id, avsutl::PlaneProcessor::ClipIdx_DST
	);
	const int      h = _plane_proc_uptr->get_height (dst_sptr, plane_id);

	try
	{
		_engine_uptr->process_plane (
			data_dst_ptr, stride_dst,
			data_src_ptr, stride_src,
			w, h, n, plane_index
		);
	}
	catch (...)
	{
		assert (false);
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
