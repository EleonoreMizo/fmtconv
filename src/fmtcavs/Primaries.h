/*****************************************************************************

        Primaries.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_Primaries_HEADER_INCLUDED)
#define fmtcavs_Primaries_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "avsutl/VideoFilterBase.h"
#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/ProcAlpha.h"
#include "fmtcl/MatrixProc.h"
#include "fmtcl/RgbSystem.h"

#include <memory>
#include <string>



namespace fmtcavs
{



class Primaries
:	public avsutl::VideoFilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef avsutl::VideoFilterBase Inherited;

	enum Param
	{
		Param_CLIP_SRC = 0,
		Param_RS,
		Param_GS,
		Param_BS,
		Param_WS,
		Param_RD,
		Param_GD,
		Param_BD,
		Param_WD,
		Param_PRIMS,
		Param_PRIMD,
		Param_CPUOPT,

		Param_NBR_ELT
	};

	explicit       Primaries (::IScriptEnvironment &env, const ::AVSValue &args);
	virtual        ~Primaries () = default;

	// VideoFilterBase
	::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr) override;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_planes_proc = fmtcl::RgbSystem::_nbr_planes;

	static void    init (fmtcl::RgbSystem &prim, ::IScriptEnvironment &env, const ::AVSValue &args, Param preset);
	static void    init (fmtcl::RgbSystem &prim, ::IScriptEnvironment &env, const ::AVSValue &args, Param pr, Param pg, Param pb, Param pw);
	static bool    read_coord_tuple (fmtcl::RgbSystem::Vec2 &c, ::IScriptEnvironment &env, const ::AVSValue &args, Param p);

	::PClip        _clip_src_sptr;
	const ::VideoInfo
	               _vi_src;

	fmtcl::RgbSystem
	               _prim_s;
	fmtcl::RgbSystem
	               _prim_d;

	fmtcl::Mat4    _mat_main { 1.0, fmtcl::Mat4::Preset_DIAGONAL };

	std::unique_ptr <fmtcl::MatrixProc>
	               _proc_uptr;

	std::unique_ptr <fmtcavs::ProcAlpha>
	               _proc_alpha_uptr;

	bool           _sse_flag  = false;
	bool           _sse2_flag = false;
	bool           _avx_flag  = false;
	bool           _avx2_flag = false;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Primaries ()                               = delete;
	               Primaries (const Primaries &other)         = delete;
	               Primaries (Primaries &&other)              = delete;
	Primaries &    operator = (const Primaries &other)        = delete;
	Primaries &    operator = (Primaries &&other)             = delete;
	bool           operator == (const Primaries &other) const = delete;
	bool           operator != (const Primaries &other) const = delete;

}; // class Primaries



}  // namespace fmtcavs



//#include "fmtcavs/Primaries.hpp"



#endif   // fmtcavs_Primaries_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
