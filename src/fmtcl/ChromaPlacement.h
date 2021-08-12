/*****************************************************************************

        ChromaPlacement.h
        Author: Laurent de Soras, 2014

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ChromaPlacement_HEADER_INCLUDED)
#define	fmtcl_ChromaPlacement_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



enum ChromaPlacement
{
	ChromaPlacement_INVALID = -2,
	ChromaPlacement_UNDEF   = -1,

	ChromaPlacement_MPEG1   = 0,
	ChromaPlacement_MPEG2,
	ChromaPlacement_DV,

	ChromaPlacement_NBR_ELT
};



void	ChromaPlacement_compute_cplace (double &cp_h, double &cp_v, ChromaPlacement cplace, int plane_index, int ss_h, int ss_v, bool rgb_flag, bool interlaced_flag, bool top_flag);



}	// namespace fmtcl



//#include "fmtcl/ChromaPlacement.hpp"



#endif	// fmtcl_ChromaPlacement_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
