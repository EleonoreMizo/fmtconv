/*****************************************************************************

        Matrix2020CL.cpp
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
#include "fmtcavs/Matrix2020CL.h"
#include "fmtcavs/function_names.h"
#include "fmtcavs/fnc.h"
#include "fmtcl/TransCurve.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix2020CL::Matrix2020CL (::IScriptEnvironment &env, const ::AVSValue &args)
:	Inherited (env, args [Param_CLIP_SRC].AsClip ())
,	_clip_src_sptr (args [Param_CLIP_SRC].AsClip ())
,	_vi_src (vi)
{
	const CpuOpt   cpu_opt (args [Param_CPUOPT]);
	const bool     sse2_flag = cpu_opt.has_sse2 ();
	const bool     avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::unique_ptr <fmtcl::Matrix2020CLProc> (
		new fmtcl::Matrix2020CLProc (sse2_flag, avx2_flag)
	);

	// Checks the input clip
	const FmtAvs   fmt_src (_vi_src);
	if (! fmt_src.is_planar ())
	{
		env.ThrowError (fmtcavs_MATRIX2020CL ": input must be planar.");
	}
	if (fmt_src.get_subspl_h () != 0 || fmt_src.get_subspl_v () != 0)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL ": input must be 4:4:4.");
	}
	if (fmt_src.get_nbr_comp_non_alpha () != _nbr_planes_proc)
	{
		env.ThrowError (
			fmtcavs_MATRIX2020CL ": greyscale format not supported as input."
		);
	}
	const auto     col_fam_src = fmt_src.get_col_fam ();
	if (   col_fam_src != fmtcl::ColorFamily_RGB
	    && col_fam_src != fmtcl::ColorFamily_YUV)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL
			": only RGB and YUV color families are supported."
		);
	}
	const int      res_src = fmt_src.get_bitdepth ();
	if (   (   res_src <  8
	        || res_src > 12)
	    &&     res_src != 14
	    &&     res_src != 16
	    &&     res_src != 32)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL ": pixel bitdepth not supported.");
	}
	if (   col_fam_src == fmtcl::ColorFamily_RGB
	    && ! fmt_src.is_float ()
		 && res_src != _rgb_int_bits)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL
			": input clip: RGB depth cannot be less than 16 bits."
		);
	}

	// Destination colorspace
	const FmtAvs   fmt_dst     = get_output_colorspace (env, args, fmt_src);
	const auto     col_fam_dst = fmt_dst.get_col_fam ();
	if (   col_fam_dst != fmtcl::ColorFamily_RGB
	    && col_fam_dst != fmtcl::ColorFamily_YUV)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL
			": unsupported color family for output."
		);
	}
	const int      res_dst = fmt_dst.get_bitdepth ();
	if (   (   res_dst <  8
	        || res_dst > 12)
	    &&     res_dst != 14
	    &&     res_dst != 16
	    &&     res_dst != 32)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL ": output bitdepth not supported.");
	}
	if (   col_fam_dst == fmtcl::ColorFamily_RGB
	    && ! fmt_dst.is_float ()
		 && res_dst != _rgb_int_bits)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL
			": output clip: RGB depth cannot be less than 16 bits."
		);
	}
	if (fmt_dst.get_subspl_h () != 0 || fmt_dst.get_subspl_v () != 0)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL ": output must be 4:4:4.");
	}
	if (fmt_src.get_nbr_comp_non_alpha () != _nbr_planes_proc)
	{
		env.ThrowError (
			fmtcavs_MATRIX2020CL ": greyscale format not supported as output."
		);
	}

	// Compatibility
	if (fmt_dst.is_float () != fmt_src.is_float ())
	{
		env.ThrowError (fmtcavs_MATRIX2020CL
			": specified output colorspace is not compatible with the input."
		);
	}
	if (col_fam_dst == col_fam_src)
	{
		env.ThrowError (fmtcavs_MATRIX2020CL
			": input and output clips must be of different color families."
		);
	}

	// Output format is validated.
	fmt_dst.conv_to_vi (vi);
	_to_yuv_flag = (col_fam_dst == fmtcl::ColorFamily_YUV);

	// Range
	_full_flag = args [Param_FULL].AsBool (false);

	// Alpha plane processing, if any
	_proc_alpha_uptr = std::make_unique <fmtcavs::ProcAlpha> (
		fmt_dst, fmt_src, vi.width, vi.height, cpu_opt
	);

	// Processor
	const fmtcl::SplFmt  splfmt_src = conv_bitdepth_to_splfmt (res_src);
	const fmtcl::SplFmt  splfmt_dst = conv_bitdepth_to_splfmt (res_dst);
	const fmtcl::Matrix2020CLProc::Err  ret_val = _proc_uptr->configure (
		_to_yuv_flag,
		splfmt_src, res_src,
		splfmt_dst, res_dst,
		_full_flag
	);

	if (ret_val != fmtcl::Matrix2020CLProc::Err_OK)
	{
		if (ret_val == fmtcl::Matrix2020CLProc::Err_INVALID_FORMAT_COMBINATION)
		{
			env.ThrowError (
				fmtcavs_MATRIX2020CL ": invalid frame format combination."
			);
		}
		else
		{
			assert (false);
			env.ThrowError (fmtcavs_MATRIX2020CL
				": unidentified error while building the matrix."
			);
		}
	}
}



::PVideoFrame __stdcall	Matrix2020CL::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	const auto     pa { build_mat_proc (vi, dst_sptr, _vi_src, src_sptr) };
	_proc_uptr->process (pa);

	// Alpha plane now
	_proc_alpha_uptr->process_plane (dst_sptr, src_sptr);

	// Frame properties
	if (supports_props ())
	{
		::AVSMap *     props_ptr = env_ptr->getFramePropsRW (dst_sptr);

		const fmtcl::ColorSpaceH265   cs_out =
			  (_to_yuv_flag)
			? fmtcl::ColorSpaceH265_BT2020CL
			: fmtcl::ColorSpaceH265_RGB;
		env_ptr->propSetInt (
			props_ptr, "_Matrix"    , int (cs_out), ::PROPAPPENDMODE_REPLACE
		);
		env_ptr->propSetInt (
			props_ptr, "_ColorSpace", int (cs_out), ::PROPAPPENDMODE_REPLACE
		);

		const auto     curve =
			  (! _to_yuv_flag)               ? fmtcl::TransCurve_LINEAR
			: (vi.BitsPerComponent () <= 10) ? fmtcl::TransCurve_2020_10
			:                                  fmtcl::TransCurve_2020_12;
		env_ptr->propSetInt (
			props_ptr, "_Transfer", int (curve), ::PROPAPPENDMODE_REPLACE
		);

		if (! _to_yuv_flag || _range_set_flag)
		{
			const int      cr_val = (! _to_yuv_flag || _full_flag) ? 0 : 1;
			env_ptr->propSetInt (
				props_ptr, "_ColorRange", cr_val, ::PROPAPPENDMODE_REPLACE
			);
		}
	}

	return dst_sptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	Matrix2020CL::_nbr_planes_proc;
constexpr int	Matrix2020CL::_rgb_int_bits;



FmtAvs	Matrix2020CL::get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src)
{
	FmtAvs         fmt_dst = fmt_src;

	auto           col_fam = fmt_dst.get_col_fam ();

	// Automatic default conversion
	if (col_fam == fmtcl::ColorFamily_RGB)
	{
		fmt_dst.set_col_fam (fmtcl::ColorFamily_YUV);
	}
	else
	{
		assert (col_fam == fmtcl::ColorFamily_YUV);
		fmt_dst.set_col_fam (fmtcl::ColorFamily_RGB);
		if (! fmt_dst.is_float ())
		{
			fmt_dst.set_bitdepth (_rgb_int_bits);
		}
	}

	// Full colorspace
	if (args [Param_CSP].Defined ())
	{
		if (fmt_dst.conv_from_str (args [Param_CSP].AsString ()) != 0)
		{
			env.ThrowError (fmtcavs_MATRIX2020CL ": invalid output colorspace.");
		}
	}

	// Destination bit depth
	const int      bitdepth = args [Param_BITS].AsInt (fmt_dst.get_bitdepth ());
	if (! FmtAvs::is_bitdepth_valid (bitdepth))
	{
		env.ThrowError (fmtcavs_MATRIX2020CL ": invalid bitdepth.");
	}
	else
	{
		fmt_dst.set_bitdepth (bitdepth);
	}

	return fmt_dst;
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
