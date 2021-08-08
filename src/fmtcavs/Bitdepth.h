/*****************************************************************************

        Bitdepth.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_Bitdepth_HEADER_INCLUDED)
#define fmtcavs_Bitdepth_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Dither.h"
#include "avsutl/PlaneProcCbInterface.h"
#include "avsutl/PlaneProcessor.h"
#include "avsutl/VideoFilterBase.h"

#include <memory>
#include <string>



namespace fmtcavs
{



class Bitdepth
:	public avsutl::VideoFilterBase
,	public avsutl::PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef avsutl::VideoFilterBase Inherited;

	enum Param
	{
		Param_CLIP_SRC = 0, // 0
		Param_BITS,
		Param_FLT,
		Param_PLANES,
		Param_FULLS,
		Param_FULLD,
		Param_DMODE,
		Param_AMPO,
		Param_AMPN,
		Param_DYN,
		Param_STATICNOISE, // 10
		Param_CPUOPT,
		Param_PATSIZE,
		Param_TPDFO,
		Param_TPDFN,
		Param_CORPLANE,

		Param_NBR_ELT,
	};

	explicit       Bitdepth (::IScriptEnvironment &env, const ::AVSValue &args);
	virtual        ~Bitdepth () = default;

	// VideoFilterBase
	::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr) override;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	void           do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr) override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	::PClip        _clip_src_sptr;
	const ::VideoInfo
	               _vi_src;

	std::unique_ptr <avsutl::PlaneProcessor>
	               _plane_proc_uptr;

	std::unique_ptr <fmtcl::Dither>
	               _engine_uptr;

	bool           _range_def_flag = false;
	bool           _fulld_flag     = false;
	bool           _fulls_flag     = false;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Bitdepth ()                               = delete;
	               Bitdepth (const Bitdepth &other)          = delete;
	               Bitdepth (Bitdepth &&other)               = delete;
	Bitdepth &     operator = (const Bitdepth &other)        = delete;
	Bitdepth &     operator = (Bitdepth &&other)             = delete;
	bool           operator == (const Bitdepth &other) const = delete;
	bool           operator != (const Bitdepth &other) const = delete;

}; // class Bitdepth



}  // namespace fmtcavs



//#include "fmtcavs/Bitdepth.hpp"



#endif   // fmtcavs_Bitdepth_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
