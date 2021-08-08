/*****************************************************************************

        fnc_fmtcavs.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/fnc.h"
#include "fmtcl/fnc.h"
#include "fmtcl/MatrixProc.h"
#include "fstb/fnc.h"
#include "avisynth.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



fmtcl::PicFmt conv_fmtavs_to_picfmt (const FmtAvs &fmt, bool full_flag)
{
	assert (fmt.is_valid ());

	fmtcl::PicFmt  pic_fmt;
	pic_fmt._sf        = conv_bitdepth_to_splfmt (fmt.get_bitdepth ());
	pic_fmt._res       = fmt.get_bitdepth ();
	pic_fmt._col_fam   = fmt.get_col_fam ();
	pic_fmt._full_flag = full_flag;

	return pic_fmt;
}



fmtcl::SplFmt	conv_vi_to_splfmt (const ::VideoInfo &vi)
{
	fmtcl::SplFmt  type     = fmtcl::SplFmt_ILLEGAL;
	int            bitdepth = 0;

	conv_vi_to_splfmt (type, bitdepth, vi);
	assert (type != fmtcl::SplFmt_ILLEGAL);

	return type;
}



void	conv_vi_to_splfmt (fmtcl::SplFmt &type, int &bitdepth, const ::VideoInfo &vi)
{
	bitdepth = vi.BitsPerComponent ();
	type     = conv_bitdepth_to_splfmt (bitdepth);
}



fmtcl::SplFmt conv_bitdepth_to_splfmt (int bitdepth)
{
	fmtcl::SplFmt  type = fmtcl::SplFmt_ILLEGAL;

	if (bitdepth == 32)
	{
		type = fmtcl::SplFmt_FLOAT;
	}
	else
	{
		if (bitdepth > 8 && bitdepth <= 16)
		{
			type = fmtcl::SplFmt_INT16;
		}
		else if (bitdepth <= 8)
		{
			type = fmtcl::SplFmt_INT8;
		}
	}

	return type;
}



fmtcl::ColorFamily	conv_vi_to_colfam (const ::VideoInfo &vi)
{
	auto          col_fam = fmtcl::ColorFamily_INVALID;

	if (vi.IsY ())
	{
		col_fam = fmtcl::ColorFamily_GRAY;
	}
	else if (vi.IsRGB ())
	{
		col_fam = fmtcl::ColorFamily_RGB;
	}
	else if (vi.IsYUV () || vi.IsYUVA ())
	{
		col_fam = fmtcl::ColorFamily_YUV;
	}

	return col_fam;
}



fmtcl::ColorFamily	conv_str_to_colfam (std::string str)
{
	fstb::conv_to_lower_case (str);

	auto          col_fam = fmtcl::ColorFamily_INVALID;

	if (str == "y" || str == "grey" || str == "gray")
	{
		col_fam = fmtcl::ColorFamily_GRAY;
	}
	else if (str == "rgb")
	{
		col_fam = fmtcl::ColorFamily_RGB;
	}
	else if (str == "yuv")
	{
		col_fam = fmtcl::ColorFamily_YUV;
	}

	return col_fam;
}



void	prepare_matrix_coef (::IScriptEnvironment &env, fmtcl::MatrixProc &mat_proc, const fmtcl::Mat4 &mat_main, const FmtAvs &fmt_dst, bool full_range_dst_flag, const FmtAvs &fmt_src, bool full_range_src_flag, fmtcl::ColorSpaceH265 csp_out, int plane_out)
{
	const fmtcl::PicFmt  fmt_src_fmtcl =
		conv_fmtavs_to_picfmt (fmt_src, full_range_src_flag);
	const fmtcl::PicFmt  fmt_dst_fmtcl =
		conv_fmtavs_to_picfmt (fmt_dst, full_range_dst_flag);

	const int      ret_val = fmtcl::prepare_matrix_coef (
		mat_proc, mat_main, fmt_dst_fmtcl, fmt_src_fmtcl, csp_out, plane_out
	);

	if (ret_val != fmtcl::MatrixProc::Err_OK)
	{
		if (ret_val == fmtcl::MatrixProc::Err_POSSIBLE_OVERFLOW)
		{
			env.ThrowError (
				"One of the matrix coefficients could cause an overflow."
			);
		}
		else if (ret_val == fmtcl::MatrixProc::Err_TOO_BIG_COEF)
		{
			env.ThrowError (
				"Too big matrix coefficient."
			);
		}
		else if (ret_val == fmtcl::MatrixProc::Err_INVALID_FORMAT_COMBINATION)
		{
			env.ThrowError (
				"Invalid frame format combination."
			);
		}
		else
		{
			assert (false);
			env.ThrowError (
				"Unidentified error while building the matrix."
			);
		}
	}
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
