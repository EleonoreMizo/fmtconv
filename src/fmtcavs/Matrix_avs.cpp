/*****************************************************************************

        Matrix.cpp
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
#include "fmtcavs/CpuOpt.h"
#include "fmtcavs/fnc.h"
#include "fmtcavs/function_names.h"
#include "fmtcavs/Matrix.h"
#include "fmtcl/fnc.h"
#include "fmtcl/MatrixUtil.h"
#include "fstb/fnc.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix::Matrix (::IScriptEnvironment &env, const ::AVSValue &args)
:	Inherited (env, args [Param_CLIP_SRC].AsClip ())
,	_clip_src_sptr (args [Param_CLIP_SRC].AsClip ())
,	_vi_src (vi)
,	_plane_out (args [Param_SINGLEOUT].AsInt (-1))
{
	const CpuOpt   cpu_opt (args [Param_CPUOPT]);
	const bool     sse_flag  = cpu_opt.has_sse ();
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx_flag  = cpu_opt.has_avx ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::make_unique <fmtcl::MatrixProc> (
		sse_flag, sse2_flag, avx_flag, avx2_flag
	);

	// Checks the input clip
	if (! _vi_src.IsPlanar ())
	{
		env.ThrowError (fmtcavs_MATRIX ": input must be planar.");
	}

	const FmtAvs   fmt_src (_vi_src);
	if (fmt_src.get_subspl_h () != 0 || fmt_src.get_subspl_v () != 0)
	{
		env.ThrowError (fmtcavs_MATRIX ": input must be 4:4:4.");
	}
	if (fmt_src.get_nbr_comp_non_alpha () != _nbr_planes_proc)
	{
		env.ThrowError (
			fmtcavs_MATRIX ": greyscale format not supported as input."
		);
	}
	const int      bd_src = fmt_src.get_bitdepth ();
	if (   (   bd_src <   8
	        || bd_src >  12)
	    &&     bd_src != 14
	    &&     bd_src != 16
	    &&     bd_src != 32 )
	{
		env.ThrowError (fmtcavs_MATRIX ": pixel bitdepth not supported.");
	}

	// Plane index for single plane output (0-2), or a negative number if all
	// planes are processed.
	if (_plane_out >= _nbr_planes_proc)
	{
		env.ThrowError (fmtcavs_MATRIX
			": singleout is a plane index and must be -1 or ranging from 0 to 3."
		);
	}

	// Destination colorspace
	bool           force_col_fam_flag;
	FmtAvs         fmt_dst = get_output_colorspace (
		env, args, fmt_src, _plane_out, force_col_fam_flag
	);

	// Preliminary matrix test: deduces the target color family if unspecified
	if (   ! force_col_fam_flag
	    && fmt_dst.get_col_fam () != fmtcl::ColorFamily_GRAY)
	{
		int               def_count = 0;
		def_count += args [Param_MAT ].Defined () ? 1 : 0;
		def_count += args [Param_MATS].Defined () ? 1 : 0;
		def_count += args [Param_MATD].Defined () ? 1 : 0;
		if (def_count == 1)
		{
			std::string    tmp_mat;
			tmp_mat = args [Param_MAT ].AsString (tmp_mat.c_str ());
			tmp_mat = args [Param_MATS].AsString (tmp_mat.c_str ());
			tmp_mat = args [Param_MATD].AsString (tmp_mat.c_str ());
			fstb::conv_to_lower_case (tmp_mat);

			const auto tmp_csp = find_cs_from_mat_str (env, tmp_mat, false);
			fmt_dst = find_dst_col_fam (tmp_csp, fmt_dst, fmt_src);
		}
	}

	const int      nbr_expected_coef = _nbr_planes_proc * (_nbr_planes_proc + 1);

	bool           mat_init_flag = false;
	bool           preset_flag   = false;
	fmtcl::Mat4    mat_main; // Main matrix, float input, float output

	// Matrix presets
	std::string    mat (args [Param_MAT].AsString (""));
	const bool     mats_default_flag =
		(fmt_src.get_col_fam () == fmtcl::ColorFamily_YUV);
	const bool     matd_default_flag =
		(       fmt_dst.get_col_fam () == fmtcl::ColorFamily_YUV
		 || (   fmt_dst.get_col_fam () == fmtcl::ColorFamily_GRAY
		     && fmt_src.get_col_fam () != fmtcl::ColorFamily_YUV));
	std::string    mats ((mats_default_flag) ? mat : "");
	std::string    matd ((matd_default_flag) ? mat : "");
	mats = args [Param_MATS].AsString (mats.c_str ());
	matd = args [Param_MATD].AsString (matd.c_str ());
	_csp_out = fmtcl::ColorSpaceH265_UNSPECIFIED;
	if (! mats.empty () || ! matd.empty ())
	{
		fstb::conv_to_lower_case (mats);
		fstb::conv_to_lower_case (matd);
		fmtcl::MatrixUtil::select_def_mat (mats, fmt_src.get_col_fam ());
		fmtcl::MatrixUtil::select_def_mat (matd, fmt_dst.get_col_fam ());

		fmtcl::Mat4    m2s;
		fmtcl::Mat4    m2d;
		if (fmtcl::MatrixUtil::make_mat_from_str (m2s, mats, true) != 0)
		{
			env.ThrowError (
				fmtcavs_MATRIX ": unknown source matrix identifier."
			);
		}
		if (fmtcl::MatrixUtil::make_mat_from_str (m2d, matd, false) != 0)
		{
			env.ThrowError (
				fmtcavs_MATRIX ": unknown destination matrix identifier."
			);
		}
		_csp_out = find_cs_from_mat_str (env, matd, false);

		mat_main      = m2d * m2s;
		mat_init_flag = true;
		preset_flag   = true;
	}

	// Alpha plane processing, if any
	_proc_alpha_uptr = std::make_unique <fmtcavs::ProcAlpha> (
		fmt_dst, fmt_src, vi.width, vi.height, cpu_opt
	);

	// Custom coefficients
	const auto     coef_list =
		extract_array_f (env, args [Param_COEF], fmtcavs_MATRIX ", coef");
	const int      nbr_coef        = int (coef_list.size ());
	const bool     custom_mat_flag = (nbr_coef > 0);
	const int      nbr_proc_planes_src = fmt_src.get_nbr_comp_non_alpha ();
	const int      nbr_proc_planes_dst = fmt_dst.get_nbr_comp_non_alpha ();
	if (custom_mat_flag)
	{
		if (nbr_coef != nbr_expected_coef)
		{
			env.ThrowError (
				fmtcavs_MATRIX ": coef has a wrong number of elements."
			);
		}

		for (int y = 0; y < _nbr_planes_proc + 1; ++y)
		{
			for (int x = 0; x < _nbr_planes_proc + 1; ++x)
			{
				mat_main [y] [x] = (x == y) ? 1 : 0;

				if (   (x < nbr_proc_planes_src || x == _nbr_planes_proc)
				    &&  y < nbr_proc_planes_dst)
				{
					const int      index = y * (nbr_proc_planes_src + 1) + x;
					mat_main [y] [x] = coef_list [index];
				}
			}
		}

		mat_init_flag = true;
	}

	if (! mat_init_flag)
	{
		env.ThrowError (fmtcavs_MATRIX
			": you must specify a matrix preset or a custom coefficient list."
		);
	}

	// Fixes the output colorspace to a valid H265 colorspace
	switch (_csp_out)
	{
	case fmtcl::ColorSpaceH265_LMS:
		_csp_out = fmtcl::ColorSpaceH265_RGB;
		break;
	case fmtcl::ColorSpaceH265_ICTCP_PQ:
	case fmtcl::ColorSpaceH265_ICTCP_HLG:
		_csp_out = fmtcl::ColorSpaceH265_ICTCP;
		break;
	default:
		// Nothing to do
		break;
	}

	// Sets the output colorspace accordingly
	if (_plane_out < 0)
	{
		if (_csp_out != fmtcl::ColorSpaceH265_UNSPECIFIED)
		{
			const auto     final_cf =
				fmtcl::MatrixUtil::find_cf_from_cs (_csp_out);
			fmt_dst.set_col_fam (final_cf);
		}
	}

	// Checks the output colorspace
	if (   fmt_dst.is_float ()     != fmt_src.is_float ()
	    || fmt_dst.get_bitdepth () <  fmt_src.get_bitdepth ()
	    || fmt_dst.get_subspl_h () != fmt_src.get_subspl_h ()
	    || fmt_dst.get_subspl_v () != fmt_src.get_subspl_v ())
	{
		env.ThrowError (fmtcavs_MATRIX
			": specified output colorspace is not compatible with the input."
		);
	}

	// Output format is validated
	if (fmt_dst.conv_to_vi (vi) != 0)
	{
		env.ThrowError (fmtcavs_MATRIX ": illegal output colorspace.");
	}

	// Range
	_fulls_flag     = args [Param_FULLS].AsBool (
		fmtcl::is_full_range_default (fmt_src.get_col_fam ())
	);
	_fulld_flag     = args [Param_FULLD].AsBool (
		fmtcl::is_full_range_default (fmt_dst.get_col_fam ())
	);
	_range_def_flag = (args [Param_FULLD].Defined () || preset_flag);

	prepare_matrix_coef (
		env, *_proc_uptr, mat_main,
		fmt_dst, _fulld_flag,
		fmt_src, _fulls_flag,
		_csp_out, _plane_out
	);
}



// mat should be already converted to lower case
fmtcl::ColorSpaceH265	Matrix::find_cs_from_mat_str (::IScriptEnvironment &env, const std::string &mat, bool allow_2020cl_flag)
{
	const auto     cs =
		fmtcl::MatrixUtil::find_cs_from_mat_str (mat, allow_2020cl_flag);
	if (cs == fmtcl::ColorSpaceH265_UNDEF)
	{
		env.ThrowError ("Unknown matrix identifier.");
	}

	return cs;
}



::PVideoFrame __stdcall	Matrix::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	const auto     pa { build_mat_proc (
		vi, dst_sptr, _vi_src, src_sptr, (_plane_out >= 0)
	) };
	_proc_uptr->process (pa);

	// Alpha plane now
	_proc_alpha_uptr->process_plane (dst_sptr, src_sptr);

	// Frame properties
	if (supports_props ())
	{
		::AVSMap *     props_ptr = env_ptr->getFramePropsRW (dst_sptr);

		if (_range_def_flag)
		{
			const int      cr_val = (_fulld_flag) ? 0 : 1;
			env_ptr->propSetInt (
				props_ptr, "_ColorRange", cr_val, ::PROPAPPENDMODE_REPLACE
			);
		}

		if (   _csp_out != fmtcl::ColorSpaceH265_UNSPECIFIED
		    && _csp_out <= fmtcl::ColorSpaceH265_ISO_RANGE_LAST)
		{
			env_ptr->propSetInt (
				props_ptr, "_Matrix"    , int (_csp_out), ::PROPAPPENDMODE_REPLACE
			);
			env_ptr->propSetInt (
				props_ptr, "_ColorSpace", int (_csp_out), ::PROPAPPENDMODE_REPLACE
			);
		}
		else
		{
			env_ptr->propDeleteKey (props_ptr, "_Matrix");
			env_ptr->propDeleteKey (props_ptr, "_ColorSpace");
		}
	}

	return dst_sptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	Matrix::_nbr_planes_proc;



FmtAvs	Matrix::get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src, int &plane_out, bool &force_col_fam_flag)
{
	force_col_fam_flag = false;

	FmtAvs         fmt_dst = fmt_src;

	// Full colorspace
	if (args [Param_CSP].Defined ())
	{
		if (fmt_dst.conv_from_str (args [Param_CSP].AsString ()) != 0)
		{
			env.ThrowError (fmtcavs_MATRIX ": invalid output colorspace.");
		}
		else
		{
			force_col_fam_flag = true;
		}
	}

	if (! fmt_dst.is_planar ())
	{
		env.ThrowError (fmtcavs_MATRIX ": output colorspace must be planar.");
	}

	if (args [Param_COL_FAM].Defined ())
	{
		force_col_fam_flag = true;
		const auto     col_fam =
			conv_str_to_colfam (args [Param_COL_FAM].AsString ());
		if (col_fam == fmtcl::ColorFamily_INVALID)
		{
			env.ThrowError (fmtcavs_MATRIX ": invalid col_fam.");
		}
		fmt_dst.set_col_fam (col_fam);
	}

	if (plane_out >= 0)
	{
		fmt_dst.set_col_fam (fmtcl::ColorFamily_GRAY);
	}
	else if (fmt_dst.get_col_fam () == fmtcl::ColorFamily_GRAY)
	{
		plane_out = 0;
	}

	const int      bitdepth = args [Param_BITS].AsInt (fmt_dst.get_bitdepth ());
	if (! FmtAvs::is_bitdepth_valid (bitdepth))
	{
		env.ThrowError (fmtcavs_MATRIX ": invalid bitdepth.");
	}
	else
	{
		fmt_dst.set_bitdepth (bitdepth);
	}

	return fmt_dst;
}



FmtAvs	Matrix::find_dst_col_fam (fmtcl::ColorSpaceH265 tmp_csp, FmtAvs fmt_dst, const FmtAvs &fmt_src)
{
	fmtcl::ColorFamily   alt_cf = fmtcl::ColorFamily_INVALID;

	switch (tmp_csp)
	{
	case fmtcl::ColorSpaceH265_RGB:
	case fmtcl::ColorSpaceH265_BT709:
	case fmtcl::ColorSpaceH265_FCC:
	case fmtcl::ColorSpaceH265_BT470BG:
	case fmtcl::ColorSpaceH265_SMPTE170M:
	case fmtcl::ColorSpaceH265_SMPTE240M:
	case fmtcl::ColorSpaceH265_YCGCO:
	case fmtcl::ColorSpaceH265_BT2020NCL:
	case fmtcl::ColorSpaceH265_BT2020CL:
	case fmtcl::ColorSpaceH265_YDZDX:
	case fmtcl::ColorSpaceH265_CHRODERNCL:
	case fmtcl::ColorSpaceH265_CHRODERCL:
	case fmtcl::ColorSpaceH265_ICTCP:
	case fmtcl::ColorSpaceH265_ICTCP_PQ:
	case fmtcl::ColorSpaceH265_ICTCP_HLG:
		alt_cf = fmtcl::ColorFamily_YUV;
		break;

	case fmtcl::ColorSpaceH265_LMS:
		alt_cf = fmtcl::ColorFamily_RGB;
		break;

	default:
		// Nothing
		break;
	}

	if (alt_cf != fmtcl::ColorFamily_INVALID)
	{
		const auto     col_fam_src = fmt_src.get_col_fam ();
		if (col_fam_src == fmtcl::ColorFamily_RGB)
		{
			fmt_dst.set_col_fam (alt_cf);
		}
		else if (col_fam_src == alt_cf)
		{
			fmt_dst.set_col_fam (fmtcl::ColorFamily_RGB);
		}
	}

	return fmt_dst;
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
