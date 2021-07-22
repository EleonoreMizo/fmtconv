/*****************************************************************************

        fnc.cpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtc/fnc.h"
#include "fmtcl/fnc.h"
#include "fmtcl/MatrixProc.h"
#include "vsutl/FilterBase.h"
#include "vsutl/fnc.h"
#include "VapourSynth.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

	
	
/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



fmtcl::PicFmt	conv_vsfmt_to_picfmt (const ::VSFormat &fmt, bool full_flag)
{
	fmtcl::PicFmt  pic_fmt;
	pic_fmt._sf        = conv_vsfmt_to_splfmt (fmt);
	pic_fmt._res       = fmt.bitsPerSample;
	pic_fmt._col_fam   = conv_colfam_to_fmtcl (fmt);
	pic_fmt._full_flag = full_flag;

	return pic_fmt;
}



fmtcl::SplFmt	conv_vsfmt_to_splfmt (const ::VSFormat &fmt)
{
	fmtcl::SplFmt  splfmt = fmtcl::SplFmt_ILLEGAL;

	if (fmt.sampleType == ::stFloat)
	{
		if (fmt.bitsPerSample == 32)
		{
			splfmt = fmtcl::SplFmt_FLOAT;
		}
	}
	else
	{
		if (fmt.bitsPerSample <= 8)
		{
			splfmt = fmtcl::SplFmt_INT8;
		}
		else if (fmt.bitsPerSample <= 16)
		{
			splfmt = fmtcl::SplFmt_INT16;
		}
	}

	return splfmt;
}



fmtcl::ColorFamily	conv_colfam_to_fmtcl (const ::VSFormat &fmt)
{
	auto          col_fam = fmtcl::ColorFamily_INVALID;

	switch (fmt.colorFamily)
	{
	case cmGray:  col_fam = fmtcl::ColorFamily_GRAY;  break;
	case cmRGB:   col_fam = fmtcl::ColorFamily_RGB;   break;
	case cmYUV:   col_fam = fmtcl::ColorFamily_YUV;   break;
	case cmYCoCg: col_fam = fmtcl::ColorFamily_YCGCO; break;
	default:      assert (false);                     break;
	}

	return col_fam;
}



void	prepare_matrix_coef (const vsutl::FilterBase &filter, fmtcl::MatrixProc &mat_proc, const fmtcl::Mat4 &mat_main, const ::VSFormat &fmt_dst, bool full_range_dst_flag, const ::VSFormat &fmt_src, bool full_range_src_flag, fmtcl::ColorSpaceH265 csp_out, int plane_out)
{
	const fmtcl::PicFmt  fmt_src_fmtcl =
		conv_vsfmt_to_picfmt (fmt_src, full_range_src_flag);
	const fmtcl::PicFmt  fmt_dst_fmtcl =
		conv_vsfmt_to_picfmt (fmt_dst, full_range_dst_flag);

	const int      ret_val = fmtcl::prepare_matrix_coef (
		mat_proc, mat_main, fmt_dst_fmtcl, fmt_src_fmtcl, csp_out, plane_out
	);

	if (ret_val != fmtcl::MatrixProc::Err_OK)
	{
		if (ret_val == fmtcl::MatrixProc::Err_POSSIBLE_OVERFLOW)
		{
			filter.throw_inval_arg (
				"one of the coefficients could cause an overflow."
			);
		}
		else if (ret_val == fmtcl::MatrixProc::Err_TOO_BIG_COEF)
		{
			filter.throw_inval_arg (
				"too big matrix coefficient."
			);
		}
		else if (ret_val == fmtcl::MatrixProc::Err_INVALID_FORMAT_COMBINATION)
		{
			filter.throw_inval_arg (
				"invalid frame format combination."
			);
		}
		else
		{
			assert (false);
			filter.throw_inval_arg (
				"unidentified error while building the matrix."
			);
		}
	}
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
