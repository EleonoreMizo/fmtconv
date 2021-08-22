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

#include "avsutl/CsPlane.h"
#include "avsutl/fnc.h"
#include "avsutl/PlaneProcessor.h"
#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/fnc.h"
#include "fmtcl/fnc.h"
#include "fmtcl/MatrixProc.h"
#include "fstb/fnc.h"
#include "avisynth.h"

#include <algorithm>

#include <cassert>



namespace fmtcavs
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <typename T, typename FT, typename FR>
std::vector <T>	extract_array_any (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, const char *typename_0, FT fnc_test, FR fnc_read)
{
	std::vector <T> val_arr;

	if (arg.Defined ())
	{
		if (arg.IsString ())
		{
			val_arr = fmtcl::conv_str_to_arr <T> (arg.AsString (""));
		}

		else if (arg.IsArray ())
		{
			const int      sz = arg.ArraySize ();
			for (int k = 0; k < sz; ++k)
			{
				const ::AVSValue &   elt = arg [k];
				if (! fnc_test (elt))
				{
					env.ThrowError (
						"%s: element %d (base 0) should be a %s.",
						filter_and_arg_0, k, typename_0
					);
				}
				val_arr.push_back (fnc_read (elt));
			}
		}

		else if (fnc_test (arg))
		{
			val_arr.push_back (fnc_read (arg));
		}

		else
		{
			env.ThrowError (
				"%s: unexpected type. Should be a string or an array of float.",
				filter_and_arg_0
			);
		}
	}

	return val_arr;
}



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
	else if (avsutl::is_rgb (vi))
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



// plane_out < 0: all planes are selected for output
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



fmtcl::ProcComp3Arg	build_mat_proc (const ::VideoInfo &vi_dst, const ::PVideoFrame &dst_sptr, const ::VideoInfo &vi_src, const ::PVideoFrame &src_sptr, bool single_plane_flag)
{
	fmtcl::ProcComp3Arg  pa;
	pa._w = vi_dst.width;
	pa._h = vi_dst.height;

	for (int p_idx = 0; p_idx < fmtcl::ProcComp3Arg::_nbr_planes; ++p_idx)
	{
		if (! single_plane_flag || p_idx == 0)
		{
			const int      pd = avsutl::CsPlane::get_plane_id (p_idx, vi_dst);
			pa._dst [p_idx]._ptr    = dst_sptr->GetWritePtr (pd);
			pa._dst [p_idx]._stride = dst_sptr->GetPitch (pd);
		}
		const int      ps = avsutl::CsPlane::get_plane_id (p_idx, vi_src);
		pa._src [p_idx]._ptr    = src_sptr->GetReadPtr (ps);
		pa._src [p_idx]._stride = src_sptr->GetPitch (ps);
	}

	return pa;
}



std::vector <double>	extract_array_f (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, double def_val)
{
	return extract_array_any <double> (env, arg, filter_and_arg_0, "float",
		[       ] (const ::AVSValue &elt) { return elt.IsFloat (); },
		[def_val] (const ::AVSValue &elt) { return elt.AsFloat (float (def_val)); }
	);
}



std::vector <int>	extract_array_i (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, int def_val)
{
	return extract_array_any <int> (env, arg, filter_and_arg_0, "int",
		[       ] (const ::AVSValue &elt) { return elt.IsInt (); },
		[def_val] (const ::AVSValue &elt) { return elt.AsInt (def_val); }
	);
}



std::vector <bool>	extract_array_b (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, bool def_val)
{
	return extract_array_any <bool> (env, arg, filter_and_arg_0, "bool",
		[       ] (const ::AVSValue &elt) { return elt.IsBool (); },
		[def_val] (const ::AVSValue &elt) { return elt.AsBool (def_val); }
	);
}



std::vector <std::string>	extract_array_s (::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, std::string def_val)
{
	return extract_array_any <std::string> (env, arg, filter_and_arg_0, "string",
		[       ] (const ::AVSValue &elt) { return elt.IsString (); },
		[def_val] (const ::AVSValue &elt) { return elt.AsString (def_val.c_str ()); }
	);
}



void	set_masktools_planes_param (avsutl::PlaneProcessor &pp, ::IScriptEnvironment &env, const ::AVSValue &arg, const char *filter_and_arg_0, double def_val)
{
	if (arg.IsString ())
	{
		pp.set_proc_mode (arg.AsString ("all"));
	}
	else
	{
		const auto     plist =
			extract_array_f (env, arg, filter_and_arg_0, def_val);
		const auto     nbr_planes = pp.get_nbr_planes ();
		for (int p_idx = 0; p_idx < nbr_planes; ++p_idx)
		{
			const auto     mode = fmtcl::get_arr_elt (plist, p_idx, def_val);
			pp.set_proc_mode (p_idx, mode);
		}
	}
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
