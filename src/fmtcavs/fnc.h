/*****************************************************************************

        fnc.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_fnc_HEADER_INCLUDED)
#define fmtcavs_fnc_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ColorFamily.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/PicFmt.h"
#include "fmtcl/SplFmt.h"

#include <string>



class IScriptEnvironment;
struct VideoInfo;



namespace fmtcl
{
   class Mat4;
   class MatrixProc;
}

namespace fmtcavs
{

class FmtAvs;



fmtcl::PicFmt conv_fmtavs_to_picfmt (const FmtAvs &fmt, bool full_flag);
fmtcl::SplFmt conv_vi_to_splfmt (const ::VideoInfo &vi);
void conv_vi_to_splfmt (fmtcl::SplFmt &type, int &bitdepth, const ::VideoInfo &vi);
fmtcl::SplFmt conv_bitdepth_to_splfmt (int bitdepth);
fmtcl::ColorFamily	conv_vi_to_colfam (const ::VideoInfo &vi);
fmtcl::ColorFamily	conv_str_to_colfam (std::string str);
void	prepare_matrix_coef (::IScriptEnvironment &env, fmtcl::MatrixProc &mat_proc, const fmtcl::Mat4 &mat_main, const FmtAvs &fmt_dst, bool full_range_dst_flag, const FmtAvs &fmt_src, bool full_range_src_flag, fmtcl::ColorSpaceH265 csp_out, int plane_out);

}  // namespace fmtcavs



//#include "fmtcavs/fnc.hpp"



#endif   // fmtcavs_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
