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
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/SplFmt.h"

#include <string>
#include <vector>



namespace fmtcl
{

class Mat4;
class MatrixProc;
class PicFmt;



int    compute_plane_width (ColorFamily col_fam, int ss_h, int base_w, int plane_index);
int    compute_plane_height (ColorFamily col_fam, int ss_v, int base_h, int plane_index);
bool   has_chroma (ColorFamily col_fam);
bool   is_chroma_plane (ColorFamily col_fam, int plane_index);
bool   is_full_range_default (ColorFamily col_fam);
double compute_pix_scale (const PicFmt &fmt, int plane_index);
double get_pix_min (const PicFmt &fmt, int plane_index);
void   compute_fmt_mac_cst (double &gain, double &add_cst, const PicFmt &dst_fmt, const PicFmt &src_fmt, int plane_index);
int    prepare_matrix_coef (MatrixProc &mat_proc, const Mat4 &mat_main, const PicFmt &dst_fmt, const PicFmt &src_fmt, ColorSpaceH265 csp_out, int plane_out);
template <typename T>
std::vector <T> conv_str_to_arr (std::string str);
template <typename T>
T      get_arr_elt (const std::vector <T> &v, int pos, const T &def) noexcept;



}	// namespace fmtcl



#include "fmtcl/fnc.hpp"



#endif	// fmtcl_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
