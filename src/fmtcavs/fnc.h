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

#include "avsutl/PlaneProcMode.h"
#include "fmtcl/ColorFamily.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/PicFmt.h"
#include "fmtcl/ProcComp3Arg.h"
#include "fmtcl/SplFmt.h"

#include <string>
#include <vector>



class AVSValue;
class IScriptEnvironment;
class PVideoFrame;
struct VideoInfo;



namespace avsutl
{
   class PlaneProcessor;
}

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
fmtcl::ProcComp3Arg build_mat_proc (const ::VideoInfo &vi_dst, const ::PVideoFrame &dst_sptr, const ::VideoInfo &vi_src, const ::PVideoFrame &src_sptr, bool single_plane_flag = false);
std::vector <double> extract_array_f (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, double def_val = 0);
std::vector <int> extract_array_i (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, int def_val = 0);
std::vector <bool> extract_array_b (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, bool def_val = false);
std::vector <std::string> extract_array_s (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, std::string def_val = "");
void set_masktools_planes_param (avsutl::PlaneProcessor &pp, ::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, double def_val = double (avsutl::PlaneProcMode_PROCESS));



}  // namespace fmtcavs



//#include "fmtcavs/fnc.hpp"



#endif   // fmtcavs_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
