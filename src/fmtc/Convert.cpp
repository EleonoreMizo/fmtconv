/*****************************************************************************

        Convert.cpp
        Author: Laurent de Soras, 2014

	========================
	*** WORK IN PROGRESS ***
	========================

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

#include "fmtc/Convert.h"
#include "fmtc/fnc.h"
#include "fmtc/Matrix.h"
#include "fmtc/Resample.h"
#include "fmtc/version.h"
#include "fmtcl/MatrixUtil.h"
#include "fmtcl/ResampleUtil.h"
#include "fstb/def.h"
#include "vsutl/fnc.h"

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Convert::Convert (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "convert", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_fmtc (*(vsapi.getPluginById (fmtc_PLUGIN_NAME, &core)))
,	_step_list ()
,	_col_fam (-1)
,	_mats (fmtcl::ColorSpaceH265_UNSPECIFIED)
,	_matd (fmtcl::ColorSpaceH265_UNSPECIFIED)
,	_cplaces (fmtcl::ChromaPlacement_UNDEF)
,	_cplaced (fmtcl::ChromaPlacement_UNDEF)
,	_fulls (ConvStep::Range_UNDEF)
,	_fulld (ConvStep::Range_UNDEF)
,	_transs (fmtcl::TransCurve_UNDEF)
,	_transd (fmtcl::TransCurve_UNDEF)
,	_gcors (get_arg_flt (in, out, "gcors", 1))
,	_gcord (get_arg_flt (in, out, "gcord", 1))
{
	fstb::unused (user_data_ptr);

	const ::VSFormat &   fmt_src = *(_vi_in.format);
	retrieve_output_colorspace (in, out, core, fmt_src);
	const ::VSFormat &   fmt_dst = *(_vi_out.format);

	// Range
	_fulls = retrieve_range (fmt_src, in, out, "fulls");
	_fulld = retrieve_range (fmt_dst, in, out, "fulld");

	// Chroma placement
	const std::string cplace_str = get_arg_str (in, out, "cplace", "mpeg2");
	if (vsutl::has_chroma (fmt_src))
	{
		const std::string cplacex_str =
			get_arg_str (in, out, "cplaces", cplace_str);
		_cplaces = Resample::conv_str_to_chroma_placement (*this, cplacex_str);
	}
	if (vsutl::has_chroma (fmt_dst))
	{
		const std::string cplacex_str =
			get_arg_str (in, out, "cplaced", cplace_str);
		_cplaced = Resample::conv_str_to_chroma_placement (*this, cplacex_str);
	}

	// Matrix presets
	std::string    mat (get_arg_str (in, out, "mat", ""));
	std::string    mats ((   vsutl::is_vs_yuv ( fmt_src.colorFamily)) ? mat : "");
	std::string    matd ((   vsutl::is_vs_yuv ( fmt_dst.colorFamily)
	                      || vsutl::is_vs_gray (fmt_dst.colorFamily)) ? mat : "");
	mats = get_arg_str (in, out, "mats", mats);
	matd = get_arg_str (in, out, "matd", matd);
	if (! mats.empty () || ! matd.empty ())
	{
		fstb::conv_to_lower_case (mats);
		fstb::conv_to_lower_case (matd);
		const auto     col_fam_src = fmtc::conv_vsfmt_to_colfam (fmt_src);
		const auto     col_fam_dst = fmtc::conv_vsfmt_to_colfam (fmt_dst);
		fmtcl::MatrixUtil::select_def_mat (mats, col_fam_src);
		fmtcl::MatrixUtil::select_def_mat (matd, col_fam_dst);
		_mats = Matrix::find_cs_from_mat_str (*this, mats, true);
		_matd = Matrix::find_cs_from_mat_str (*this, matd, true);
	}

	// Transfer curve
	_transs = retrieve_tcurve (fmt_src, in, out, "transs", "");
	_transd = retrieve_tcurve (fmt_dst, in, out, "transd", "");



	/*** To do ***/

	find_conversion_steps (in, out);

	/*** To do ***/

}



void	Convert::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (in, out, core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Convert::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	fstb::unused (frame_data_ptr, core);

	assert (n >= 0);

	::VSFrameRef *    dst_ptr = 0;
	::VSNodeRef &     node = *_clip_src_sptr;

	if (activation_reason == ::arInitial)
	{
		_vsapi.requestFrameFilter (n, &node, &frame_ctx);
	}
	else if (activation_reason == ::arAllFramesReady)
	{

		/*** To do ***/

	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Convert::retrieve_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src)
{
	const ::VSFormat *   fmt_dst_ptr = &fmt_src;

	// Full colorspace
	int            csp_dst = get_arg_int (in, out, "csp", ::pfNone);
	if (csp_dst != ::pfNone)
	{
		fmt_dst_ptr = _vsapi.getFormatPreset (csp_dst, &core);
		if (fmt_dst_ptr == 0)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
	}

	int            col_fam  = fmt_dst_ptr->colorFamily;
	int            spl_type = fmt_dst_ptr->sampleType;
	int            bits     = fmt_dst_ptr->bitsPerSample;
	int            ssh      = fmt_dst_ptr->subSamplingW;
	int            ssv      = fmt_dst_ptr->subSamplingH;

	// Color family
	_col_fam = get_arg_int (in, out, "col_fam", col_fam);

	// Chroma subsampling
	std::string    css (get_arg_str (in, out, "css", ""));
	if (! css.empty ())
	{
		const int      ret_val =
			fmtcl::ResampleUtil::conv_str_to_chroma_subspl (ssh, ssv, css);
		if (ret_val != 0)
		{
			throw_inval_arg ("unsupported css value.");
		}
	}

	// Destination bit depth and sample type
	bool           bits_def_flag = false;
	bool           flt_def_flag = false;
	int            flt = (spl_type != ::stInteger) ? 1 : 0;
	bits = get_arg_int (in, out, "bits", bits, 0, &bits_def_flag);
	flt  = get_arg_int (in, out, "flt" , flt,  0, &flt_def_flag );
	spl_type = (flt != 0) ? ::stFloat : ::stInteger;

	if (flt_def_flag && ! bits_def_flag)
	{
		if (spl_type == ::stFloat)
		{
			bits = 32;
		}
		else
		{
			if (bits > 16)
			{
				throw_inval_arg (
					"Cannot deduce the output bitdepth. Please specify it."
				);
			}
		}
	}
	else if (bits_def_flag && ! flt_def_flag)
	{
		if (bits >= 32)
		{
			spl_type = ::stFloat;
		}
		else
		{
			spl_type = ::stInteger;
		}
	}

	// Combines the modified parameters and validates the format
	try
	{
		fmt_dst_ptr = register_format (
			_col_fam,
			spl_type,
			bits,
			ssh,
			ssv,
			core
		);
	}
	catch (std::exception &)
	{
		throw;
	}
	catch (...)
	{
		fmt_dst_ptr = 0;
	}

	if (fmt_dst_ptr == 0)
	{
		throw_rt_err (
			"couldn\'t get a pixel format identifier for the output clip."
		);
	}

	_vi_out.format = fmt_dst_ptr;
}



ConvStep::Range	Convert::retrieve_range (const ::VSFormat &fmt, const ::VSMap &in, ::VSMap &out, const char arg_0 [])
{
	assert (arg_0 != 0);

	bool           range_set_flag  = false;
	const bool     full_range_flag = (get_arg_int (
		in, out, arg_0,
		vsutl::is_full_range_default (fmt) ? 1 : 0,
		0, &range_set_flag
	) != 0);

	return (
		  (! range_set_flag) ? ConvStep::Range_UNDEF
		: (full_range_flag ) ? ConvStep::Range_FULL
		:                      ConvStep::Range_TV
	);
}



fmtcl::TransCurve	Convert::retrieve_tcurve (const ::VSFormat &fmt, const ::VSMap &in, ::VSMap &out, const char arg_0 [], const char def_0 [])
{
	fstb::unused (fmt);

	fmtcl::TransCurve tcurve = fmtcl::TransCurve_UNDEF;

	bool           curve_flag = false;
	std::string    curve_str =
		get_arg_str (in, out, arg_0, def_0, 0, &curve_flag);
	fstb::conv_to_lower_case (curve_str);

	if (! curve_flag || curve_str.empty ())
	{
		tcurve = fmtcl::TransCurve_UNDEF;
	}
	else if (curve_str == "linear")
	{
		tcurve = fmtcl::TransCurve_LINEAR;
	}
	else if (curve_str == "srgb" || curve_str == "6196621")
	{
		tcurve = fmtcl::TransCurve_SRGB;
	}
	else if (curve_str == "709")
	{
		tcurve = fmtcl::TransCurve_709;
	}
	else if (curve_str == "601" || curve_str == "170")
	{
		tcurve = fmtcl::TransCurve_601;
	}
	else if (curve_str == "470m")
	{
		tcurve = fmtcl::TransCurve_470M;
	}
	else if (curve_str == "470bg")
	{
		tcurve = fmtcl::TransCurve_470BG;
	}
	else if (curve_str == "240")
	{
		tcurve = fmtcl::TransCurve_240;
	}
	else if (curve_str == "2020")
	{
		tcurve = fmtcl::TransCurve_2020_12;
	}
	else if (curve_str == "log100")
	{
		tcurve = fmtcl::TransCurve_LOG100;
	}
	else if (curve_str == "log316")
	{
		tcurve = fmtcl::TransCurve_LOG316;
	}
	else if (curve_str == "6196624")
	{
		tcurve = fmtcl::TransCurve_61966_2_4;
	}
	else if (curve_str == "1361")
	{
		tcurve = fmtcl::TransCurve_1361;
	}
	else if (curve_str == "1886")
	{
		tcurve = fmtcl::TransCurve_1886;
	}
	else if (curve_str == "1886a")
	{
		tcurve = fmtcl::TransCurve_1886A;
	}
	else
	{
		throw_inval_arg ("unexpected string for the transfer curve.");
	}

	return (tcurve);
}



void	Convert::find_conversion_steps (const ::VSMap &in, ::VSMap &out)
{
	fstb::unused (in, out);

	_step_list.clear ();

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// First, creates the first and last nodes

	_step_list.emplace_back ();
	_step_list.emplace_back ();
	ConvStep &     beg = _step_list.front ();
	ConvStep &     end = _step_list.back ();

	// Colorspace information
	fill_conv_step_with_cs (beg, *_vi_in.format);
	fill_conv_step_with_cs (end, *_vi_out.format);

	// Range
	beg._range = _fulls;
	end._range = _fulld;

	// Chroma placement
	beg._cplace = _cplaces;
	end._cplace = _cplaced;

	// Transfer curve
	const bool     transs_flag =
		fill_conv_step_with_curve (beg, *_vi_in.format , _transs, _mats);
	const bool     transd_flag =
		fill_conv_step_with_curve (end, *_vi_out.format, _transd, _matd);

	// If one is undefined, copy it from the other side
	if (   beg._tcurve == fmtcl::TransCurve_UNDEF
	    && end._tcurve != fmtcl::TransCurve_UNDEF)
	{
		beg._tcurve = end._tcurve;
	}
	else if (   end._tcurve == fmtcl::TransCurve_UNDEF
	         && beg._tcurve != fmtcl::TransCurve_UNDEF)
	{
		end._tcurve = beg._tcurve;
	}
	// Now all curves are defined or all undefined.

	// Addtional gamma correction
	beg._gammac = _gcors;
	end._gammac = _gcord;

	end._resized_flag = true;

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Do we need an intermediate linear RGB step?

	bool           add_lin_rgb_flag = false;

	bool           beg_nonlin_flag = 
		(   beg._tcurve != fmtcl::TransCurve_UNDEF
	    && beg._tcurve != fmtcl::TransCurve_LINEAR);
	bool           end_nonlin_flag =
		(   end._tcurve != fmtcl::TransCurve_UNDEF
		 && end._tcurve != fmtcl::TransCurve_LINEAR);

	if (beg_nonlin_flag && end_nonlin_flag)
	{
		// Needs at least one curve to be explicitely specified
		if (transs_flag || transd_flag)
		{
			add_lin_rgb_flag = true;
		}
		// Force the linear processing if BT.2020CL is used with another
		// colorspace.
		else if (  (_mats == fmtcl::ColorSpaceH265_BT2020CL)
		         ^ (_matd == fmtcl::ColorSpaceH265_BT2020CL))
		{
			add_lin_rgb_flag = true;
		}
	}

	// If the transfer curves are unknown, we cannot use a linear step.
	if (beg._tcurve == fmtcl::TransCurve_UNDEF)
	{
		add_lin_rgb_flag = false;
	}

	// Yes? Inserts this step
	if (add_lin_rgb_flag)
	{
		// Clone the end node
		StepList::iterator   lin_it =
			_step_list.insert (++ _step_list.begin (), end);
		ConvStep &     lin = *lin_it;

		lin._col_fam      = ::cmRGB;
		lin._css_h        = 0;
		lin._css_v        = 0;
		lin._tcurve       = fmtcl::TransCurve_LINEAR;
		lin._gammac       = 1;

		lin._resized_flag = false;
		lin._sample_type  = -1;
		lin._bitdepth     = -1;
	}


	/*** To do ***/



}



// Set here: _col_fam, _css_h, _css_v, _sample_type, _bitdepth
void	Convert::fill_conv_step_with_cs (ConvStep &step, const ::VSFormat &fmt)
{
	step._col_fam = fmt.colorFamily;
	if (vsutl::has_chroma (fmt))
	{
		step._css_h = fmt.subSamplingW;
		step._css_v = fmt.subSamplingH;
	}

	step._sample_type = fmt.sampleType;
	step._bitdepth    = fmt.bitsPerSample;
}



// Returns true if explicitely specified
bool	Convert::fill_conv_step_with_curve (ConvStep &step, const ::VSFormat &fmt, fmtcl::TransCurve tcurve, fmtcl::ColorSpaceH265 mat)
{
	bool           curve_flag = (tcurve != fmtcl::TransCurve_UNDEF);
	step._tcurve = tcurve;

	// Try to guess unspecified transfer curves
	if (! curve_flag)
	{
		if (mat == fmtcl::ColorSpaceH265_UNSPECIFIED)
		{
			if (vsutl::is_vs_rgb (fmt.colorFamily))
			{
				step._tcurve = fmtcl::TransCurve_SRGB;
			}
			else  // All luma-based colorspaces
			{
				step._tcurve = fmtcl::TransCurve_601;
			}
		}
		else
		{
			// We don't check if the matrix is compatible with the specified
			// colorspace.
			switch (mat)
			{
			case fmtcl::ColorSpaceH265_RGB:
				step._tcurve = fmtcl::TransCurve_SRGB;
				break;
			case fmtcl::ColorSpaceH265_BT709:
			case fmtcl::ColorSpaceH265_YCGCO:
				step._tcurve = fmtcl::TransCurve_709;
				break;
			case fmtcl::ColorSpaceH265_FCC:
				step._tcurve = fmtcl::TransCurve_470M;
				break;
			case fmtcl::ColorSpaceH265_BT470BG:
				step._tcurve = fmtcl::TransCurve_470BG;
				break;
			case fmtcl::ColorSpaceH265_SMPTE170M:
				step._tcurve = fmtcl::TransCurve_601;
				break;
			case fmtcl::ColorSpaceH265_SMPTE240M:
				step._tcurve = fmtcl::TransCurve_240;
				break;
			case fmtcl::ColorSpaceH265_BT2020NCL:
			case fmtcl::ColorSpaceH265_BT2020CL:
				step._tcurve = fmtcl::TransCurve_2020_12;
				break;
			case fmtcl::ColorSpaceH265_UNSPECIFIED:
			case fmtcl::ColorSpaceH265_RESERVED:
				// Should not happen
				assert (false);
				break;
			case fmtcl::ColorSpaceH265_CUSTOM:
			default:
				// Nothing, keep it undefined
				break;
			}
		}
	}

	return (curve_flag);
}



void	Convert::fill_conv_step_with_gcor (ConvStep &step, const ::VSMap &in, ::VSMap &out, const char arg_0 [])
{
	double         gcor = get_arg_flt (in, out, arg_0, -1);
	if (gcor <= 0)
	{
		gcor = -1;
	}
	step._gammac = gcor;
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
