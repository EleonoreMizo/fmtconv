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
#if ! defined (fmtc_fnc_HEADER_INCLUDED)
#define	fmtc_fnc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ColorFamily.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/ProcComp3Arg.h"
#include "fmtcl/PicFmt.h"
#include "fmtcl/SplFmt.h"



struct VSAPI;
struct VSVideoFormat;
struct VSFrame;

namespace fmtcl
{
	class Mat4;
	class MatrixProc;
}
namespace vsutl
{
	class FilterBase;
}

namespace fmtc
{



fmtcl::PicFmt conv_vsfmt_to_picfmt (const ::VSVideoFormat &fmt, bool full_flag);
fmtcl::SplFmt conv_vsfmt_to_splfmt (const ::VSVideoFormat &fmt);
void conv_vsfmt_to_splfmt (fmtcl::SplFmt &type, int &bitdepth, const ::VSVideoFormat &fmt);
fmtcl::ColorFamily conv_vsfmt_to_colfam (const ::VSVideoFormat &fmt);
int conv_fmtcl_colfam_to_vs (fmtcl::ColorFamily cf);
void prepare_matrix_coef (const vsutl::FilterBase &filter, fmtcl::MatrixProc &mat_proc, const fmtcl::Mat4 &mat_main, const ::VSVideoFormat &fmt_dst, bool full_range_dst_flag, const ::VSVideoFormat &fmt_src, bool full_range_src_flag, fmtcl::ColorSpaceH265 csp_out = fmtcl::ColorSpaceH265_UNSPECIFIED, int plane_out = -1);
fmtcl::ProcComp3Arg build_mat_proc (const ::VSAPI &vsapi, ::VSFrame &dst, const ::VSFrame &src, bool single_plane_flag = false);



}	// namespace fmtc



//#include "fmtc/fnc.hpp"



#endif	// fmtc_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
