/*****************************************************************************

        fnc.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_fnc_HEADER_INCLUDED)
#define	fmtcl_fnc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ColorFamily.h"
#include "fmtcl/SplFmt.h"



namespace fmtcl
{



bool   has_chroma (ColorFamily col_fam);
bool   is_chroma_plane (ColorFamily col_fam, int plane_index);
bool   is_full_range_default (ColorFamily col_fam);
double compute_pix_scale (SplFmt spl_fmt, int nbr_bits, ColorFamily col_fam, int plane_index, bool full_flag);
double get_pix_min (SplFmt spl_fmt, int nbr_bits, ColorFamily col_fam, int plane_index, bool full_flag);
void   compute_fmt_mac_cst (double &gain, double &add_cst, SplFmt dst_spl_fmt, int dst_nbr_bits, ColorFamily dst_col_fam, bool dst_full_flag, SplFmt src_spl_fmt, int src_nbr_bits, ColorFamily src_col_fam, bool src_full_flag, int plane_index);



}	// namespace fmtcl



//#include "fmtcl/fnc.hpp"



#endif	// fmtcl_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
