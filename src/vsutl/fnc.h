/*****************************************************************************

        fnc.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_fnc_HEADER_INCLUDED)
#define	vsutl_fnc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "VapourSynth.h"

#include <string>



namespace vsutl
{


bool     is_vs_gray (int cf);
bool     is_vs_rgb (int cf);
bool     is_vs_yuv (int cf);
bool     is_vs_same_colfam (int lhs, int rhs);

bool     is_constant_format (const ::VSVideoInfo &vi);
bool     has_chroma (const ::VSFormat &fmt);
bool     is_chroma_plane (const ::VSFormat &fmt, int plane_index);
bool     is_full_range_default (const ::VSFormat &fmt);
double   compute_pix_scale (const ::VSFormat &fmt, int plane_index, bool full_flag);
double   get_pix_min (const ::VSFormat &fmt, int plane_index, bool full_flag);
void     compute_fmt_mac_cst (double &gain, double &add_cst, const ::VSFormat &fmt_dst, bool full_dst_flag, const ::VSFormat &fmt_src, bool full_src_flag, int plane_index);
int      compute_plane_width (const ::VSFormat &fmt, int plane_index, int base_w);
int      compute_plane_height (const ::VSFormat &fmt, int plane_index, int base_h);
int      conv_str_to_chroma_subspl (int &ssh, int &ssv, std::string css);



}	// namespace vsutl



//#include "vsutl/fnc.hpp"



#endif	// vsutl_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
