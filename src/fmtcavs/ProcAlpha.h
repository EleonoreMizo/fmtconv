/*****************************************************************************

        ProcAlpha.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_ProcAlpha_HEADER_INCLUDED)
#define fmtcavs_ProcAlpha_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcavs/FmtAvs.h"
#include "fmtcl/BitBltConv.h"
#include "fmtcl/SplFmt.h"



class PVideoFrame;

namespace fmtcavs
{



class CpuOpt;

class ProcAlpha
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       ProcAlpha (FmtAvs fmt_dst, FmtAvs fmt_src, int w, int h, const CpuOpt &cpu_opt);

	void           process_plane (::PVideoFrame &dst_sptr, ::PVideoFrame &src_sptr) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           _dst_a_flag = false;
	bool           _src_a_flag = false;
	int            _dst_res    = -1;
	int            _src_res    = -1;
	fmtcl::SplFmt  _splfmt_dst = fmtcl::SplFmt_ILLEGAL;
	fmtcl::SplFmt  _splfmt_src = fmtcl::SplFmt_ILLEGAL;
	int            _w          = 0;
	int            _h          = 0;
	fmtcl::BitBltConv::ScaleInfo // Set only when both source and dest have an alpha plane
	               _scale_info;
	bool           _sse2_flag  = false;
	bool           _avx2_flag  = false;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ProcAlpha ()                               = delete;
	               ProcAlpha (const ProcAlpha &other)         = delete;
	               ProcAlpha (ProcAlpha &&other)              = delete;
	ProcAlpha &    operator = (const ProcAlpha &other)        = delete;
	ProcAlpha &    operator = (ProcAlpha &&other)             = delete;
	bool           operator == (const ProcAlpha &other) const = delete;
	bool           operator != (const ProcAlpha &other) const = delete;

}; // class ProcAlpha



}  // namespace fmtcavs



//#include "fmtcavs/ProcAlpha.hpp"



#endif   // fmtcavs_ProcAlpha_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
