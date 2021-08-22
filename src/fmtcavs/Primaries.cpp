/*****************************************************************************

        Primaries.cpp
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

#include "avsutl/CsPlane.h"
#include "fmtcavs/CpuOpt.h"
#include "fmtcavs/fnc.h"
#include "fmtcavs/function_names.h"
#include "fmtcavs/Primaries.h"
#include "fmtcl/fnc.h"
#include "fmtcl/PrimUtil.h"
#include "fstb/fnc.h"

#include <array>

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Primaries::Primaries (::IScriptEnvironment &env, const ::AVSValue &args)
:	Inherited (env, args [Param_CLIP_SRC].AsClip ())
,	_clip_src_sptr (args [Param_CLIP_SRC].AsClip ())
,	_vi_src (vi)
{
	const CpuOpt   cpu_opt (args [Param_CPUOPT]);
	_sse_flag  = cpu_opt.has_sse ();
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx_flag  = cpu_opt.has_avx ();
	_avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::unique_ptr <fmtcl::MatrixProc> (new fmtcl::MatrixProc (
		_sse_flag, _sse2_flag, _avx_flag, _avx2_flag
	));

	// Checks the input clip
	const FmtAvs   fmt_src (vi);
	if (! fmt_src.is_planar ())
	{
		env.ThrowError (fmtcavs_PRIMARIES ": input must be planar.");
	}
	const auto     col_fam = fmt_src.get_col_fam ();
	if (col_fam != fmtcl::ColorFamily_RGB)
	{
		env.ThrowError (
			fmtcavs_PRIMARIES ": colorspace must be RGB (assumed linear)."
		);
	}
	const auto     res      = fmt_src.get_bitdepth ();
	const bool     flt_flag = fmt_src.is_float ();
	if (   (! flt_flag && res != 16)
	    || (  flt_flag && res != 32))
	{
		env.ThrowError (fmtcavs_PRIMARIES
			": pixel bitdepth not supported, "
			"clip must be 16-bit integer or 32-bit float."
		);
	}
	assert (fmt_src.get_subspl_h () == 0 && fmt_src.get_subspl_v () == 0);
	assert (fmt_src.get_nbr_comp_non_alpha () == _nbr_planes_proc);

	// Destination format
	const auto     fmt_dst = fmt_src;

	// Alpha plane processing, if any
	_proc_alpha_uptr = std::make_unique <fmtcavs::ProcAlpha> (
		fmt_dst, fmt_src, vi.width, vi.height, cpu_opt
	);

	// Primaries
	init (_prim_s, env, args, Param_PRIMS);
	init (_prim_s, env, args, Param_RS, Param_GS, Param_BS, Param_WS);
	if (! _prim_s.is_ready ())
	{
		env.ThrowError (fmtcavs_PRIMARIES ": input primaries not set.");
	}

	_prim_d = _prim_s;
	init (_prim_d, env, args, Param_PRIMD);
	init (_prim_d, env, args, Param_RD, Param_GD, Param_BD, Param_WD);
	assert (_prim_d.is_ready ());

	const fmtcl::Mat3 mat_conv =
		fmtcl::PrimUtil::compute_conversion_matrix (_prim_s, _prim_d);
	_mat_main.insert3 (mat_conv);
	_mat_main.clean3 (1);

	prepare_matrix_coef (
		env, *_proc_uptr, _mat_main,
		fmt_dst, true,
		fmt_src, true,
		fmtcl::ColorSpaceH265_RGB, -1
	);
}



::PVideoFrame __stdcall	Primaries::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	const auto     pa { build_mat_proc (vi, dst_sptr, _vi_src, src_sptr) };
	_proc_uptr->process (pa);

	// Alpha plane now
	_proc_alpha_uptr->process_plane (dst_sptr, src_sptr);

	// Frame properties
	if (supports_props ())
	{
		::AVSMap *     props_ptr = env_ptr->getFramePropsRW (dst_sptr);

		const fmtcl::PrimariesPreset  preset_d = _prim_d._preset;
		if (preset_d >= 0 && preset_d < fmtcl::PrimariesPreset_NBR_ELT)
		{
			env_ptr->propSetInt (
				props_ptr, "_Primaries", int (preset_d), ::PROPAPPENDMODE_REPLACE
			);
		}
		else
		{
			env_ptr->propDeleteKey (props_ptr, "_Primaries");
		}
	}

	return dst_sptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	Primaries::_nbr_planes_proc;



void	Primaries::init (fmtcl::RgbSystem &prim, ::IScriptEnvironment &env, const ::AVSValue &args, Param preset)
{
	assert (preset >= 0);
	assert (preset < Param_NBR_ELT);

	std::string    preset_str = args [preset].AsString ("");
	fstb::conv_to_lower_case (preset_str);
	prim._preset = fmtcl::PrimUtil::conv_string_to_primaries (preset_str);
	if (prim._preset == fmtcl::PrimariesPreset_INVALID)
	{
		env.ThrowError (fmtcavs_PRIMARIES ": invalid preset name.");
	}
	else if (prim._preset >= 0)
	{
		prim.set (prim._preset);
	}
}



void	Primaries::init (fmtcl::RgbSystem &prim, ::IScriptEnvironment &env, const ::AVSValue &args, Param pr, Param pg, Param pb, Param pw)
{
	assert (pr >= 0);
	assert (pr < Param_NBR_ELT);
	assert (pg >= 0);
	assert (pg < Param_NBR_ELT);
	assert (pb >= 0);
	assert (pb < Param_NBR_ELT);
	assert (pw >= 0);
	assert (pw < Param_NBR_ELT);

	const bool     ready_old_flag = prim.is_ready ();
	std::array <fmtcl::RgbSystem::Vec2, _nbr_planes_proc> rgb_old = prim._rgb;
	fmtcl::RgbSystem::Vec2  w_old = prim._white;

	const std::array <Param, _nbr_planes_proc> param_arr { pr, pg, pb };
	for (int k = 0; k < _nbr_planes_proc; ++k)
	{
		prim._init_flag_arr [k] |=
			read_coord_tuple (prim._rgb [k], env, args, param_arr [k]);
	}

	prim._init_flag_arr [_nbr_planes_proc] |=
		read_coord_tuple (prim._white, env, args, pw);

	if (   ready_old_flag && prim.is_ready ()
	    && (rgb_old != prim._rgb || w_old != prim._white))
	{
		prim._preset = fmtcl::PrimariesPreset_UNDEF;
	}
}



bool	Primaries::read_coord_tuple (fmtcl::RgbSystem::Vec2 &c, ::IScriptEnvironment &env, const ::AVSValue &args, Param p)
{
	bool           set_flag = false;

	auto           c_v = extract_array_f (env, args [p], fmtcavs_PRIMARIES);
	if (! c_v.empty ())
	{
		if (c_v.size () != c.size ())
		{
			env.ThrowError (fmtcavs_PRIMARIES
				": wrong number of coordinates (expected x and y)."
			);
		}
		double            sum = 0;
		for (size_t k = 0; k < c_v.size (); ++k)
		{
			sum  += c_v [k];
			c [k] = c_v [k];
		}
		if (c [1] == 0)
		{
			env.ThrowError (
				fmtcavs_PRIMARIES ": y coordinate cannot be 0."
			);
		}

		set_flag = true;
	}

	return set_flag;
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
