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
#include "fmtcl/PicFmt.h"
#include "fmtcl/SplFmt.h"



struct VSFormat;

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



fmtcl::PicFmt conv_vsfmt_to_picfmt (const ::VSFormat &fmt, bool full_flag);
fmtcl::SplFmt conv_vsfmt_to_splfmt (const ::VSFormat &fmt);
void conv_vsfmt_to_splfmt (fmtcl::SplFmt &type, int &bitdepth, const ::VSFormat &fmt);
fmtcl::ColorFamily conv_vsfmt_to_colfam (const ::VSFormat &fmt);
void prepare_matrix_coef (const vsutl::FilterBase &filter, fmtcl::MatrixProc &mat_proc, const fmtcl::Mat4 &mat_main, const ::VSFormat &fmt_dst, bool full_range_dst_flag, const ::VSFormat &fmt_src, bool full_range_src_flag, fmtcl::ColorSpaceH265 csp_out = fmtcl::ColorSpaceH265_UNSPECIFIED, int plane_out = -1);



}	// namespace fmtc



//#include "fmtc/fnc.hpp"



#endif	// fmtc_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
