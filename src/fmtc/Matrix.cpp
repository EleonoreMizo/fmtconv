/*****************************************************************************

        Matrix.cpp
        Author: Laurent de Soras, 2012

TO DO:
	- Make the SSE2 code use aligned read/write.
	- Make a special case for kRkGkB matrix conversions, where the luma plane
		is not used to compute the chroma planes.

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

#include "fstb/def.h"
#include "fmtc/Matrix.h"
#include "fmtc/fnc.h"
#include "fmtcl/Mat4.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix::Matrix (const ::VSMap &in, ::VSMap &out, void * /*user_data_ptr*/, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "matrix", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_sse_flag (false)
,	_sse2_flag (false)
,	_avx_flag (false)
,	_avx2_flag (false)
,	_range_set_src_flag (false)
,	_range_set_dst_flag (false)
,	_full_range_src_flag (false)
,	_full_range_dst_flag (false)
/*,	_mat_main ()*/
,	_csp_out (fmtcl::ColorSpaceH265_UNSPECIFIED)
,	_plane_out (get_arg_int (in, out, "singleout", -1))
,	_proc_uptr ()
{
	vsutl::CpuOpt  cpu_opt (*this, in, out);
	_sse_flag  = cpu_opt.has_sse ();
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx_flag  = cpu_opt.has_avx ();
	_avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::unique_ptr <fmtcl::MatrixProc> (new fmtcl::MatrixProc (
		_sse_flag, _sse2_flag, _avx_flag, _avx2_flag
	));

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const ::VSFormat &   fmt_src = *_vi_in.format;

	if (fmt_src.subSamplingW != 0 || fmt_src.subSamplingH != 0)
	{
		throw_inval_arg ("input must be 4:4:4.");
	}
	if (fmt_src.numPlanes != NBR_PLANES)
	{
		throw_inval_arg ("greyscale format not supported as input.");
	}
	if (   (   fmt_src.sampleType == ::stInteger
	        && (   fmt_src.bitsPerSample <  8
	            || fmt_src.bitsPerSample > 12)
	        && fmt_src.bitsPerSample != 16)
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_src.bitsPerSample != 32))
	{
		throw_inval_arg ("pixel bitdepth not supported.");
	}

	if (_plane_out >= NBR_PLANES)
	{
		throw_inval_arg (
			"singleout is a plane index and must be -1 or ranging from 0 to 3."
		);
	}

	// Destination colorspace
	bool           force_col_fam_flag;
	const ::VSFormat *   fmt_dst_ptr = get_output_colorspace (
		in, out, core, fmt_src, _plane_out, force_col_fam_flag
	);

	if (   fmt_dst_ptr->colorFamily != ::cmGray
	    && fmt_dst_ptr->colorFamily != ::cmRGB
	    && fmt_dst_ptr->colorFamily != ::cmYUV
	    && fmt_dst_ptr->colorFamily != ::cmYCoCg)
	{
		throw_inval_arg ("unsupported color family for output.");
	}
	if (   (   fmt_dst_ptr->sampleType == ::stInteger
	        && (   fmt_dst_ptr->bitsPerSample <  8
	            || fmt_dst_ptr->bitsPerSample > 12)
	        && fmt_dst_ptr->bitsPerSample != 16)
	    || (   fmt_dst_ptr->sampleType == ::stFloat
	        && fmt_dst_ptr->bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}
	if (   fmt_dst_ptr->sampleType    != fmt_src.sampleType
	    || fmt_dst_ptr->bitsPerSample <  fmt_src.bitsPerSample
	    || fmt_dst_ptr->subSamplingW  != fmt_src.subSamplingW
	    || fmt_dst_ptr->subSamplingH  != fmt_src.subSamplingH)
	{
		throw_inval_arg (
			"specified output colorspace is not compatible with the input."
		);
	}

	// Preliminary matrix test: deduce the target color family if unspecified
	if (   ! force_col_fam_flag
	    && fmt_dst_ptr->colorFamily != ::cmGray)
	{
		int               def_count = 0;
		def_count += is_arg_defined (in, "mat" ) ? 1 : 0;
		def_count += is_arg_defined (in, "mats") ? 1 : 0;
		def_count += is_arg_defined (in, "matd") ? 1 : 0;
		if (def_count == 1)
		{
			std::string    tmp_mat (get_arg_str (in, out, "mat", ""));
			tmp_mat = get_arg_str (in, out, "mats", tmp_mat);
			tmp_mat = get_arg_str (in, out, "matd", tmp_mat);
			fstb::conv_to_lower_case (tmp_mat);

			fmtcl::ColorSpaceH265   tmp_csp =
				find_cs_from_mat_str (*this, tmp_mat, false);

			fmt_dst_ptr = find_dst_col_fam (tmp_csp, fmt_dst_ptr, fmt_src, core);
		}
	}

	// Output format is validated.
	_vi_out.format = fmt_dst_ptr;
	const ::VSFormat &fmt_dst = *fmt_dst_ptr;

	const int      nbr_expected_coef = NBR_PLANES * (NBR_PLANES + 1);

	bool           mat_init_flag = false;

	// Matrix presets
	std::string    mat (get_arg_str (in, out, "mat", ""));
	std::string    mats ((   fmt_src.colorFamily == ::cmYUV ) ? mat : "");
	std::string    matd ((   fmt_dst.colorFamily == ::cmYUV
	                      || fmt_dst.colorFamily == ::cmGray) ? mat : "");
	mats = get_arg_str (in, out, "mats", mats);
	matd = get_arg_str (in, out, "matd", matd);
	if (! mats.empty () || ! matd.empty ())
	{
		fstb::conv_to_lower_case (mats);
		fstb::conv_to_lower_case (matd);
		select_def_mat (mats, fmt_src);
		select_def_mat (matd, fmt_dst);

		fmtcl::Mat4    m2s;
		fmtcl::Mat4    m2d;
		make_mat_from_str (m2s, mats, true);
		make_mat_from_str (m2d, matd, false);
		_csp_out = find_cs_from_mat_str (*this, matd, false);

		_mat_main = m2d * m2s;

		mat_init_flag = true;
	}

	// Range
	_full_range_src_flag = (get_arg_int (
		in, out, "fulls" ,
		vsutl::is_full_range_default (fmt_src) ? 1 : 0,
		0, &_range_set_src_flag
	) != 0);
	_full_range_dst_flag = (get_arg_int (
		in, out, "fulld",
		vsutl::is_full_range_default (fmt_dst) ? 1 : 0,
		0, &_range_set_dst_flag
	) != 0);

	// Custom coefficients
	const int      nbr_coef = _vsapi.propNumElements (&in, "coef");
	const bool     custom_mat_flag = (nbr_coef > 0);
	if (custom_mat_flag)
	{
		if (nbr_coef != nbr_expected_coef)
		{
			throw_inval_arg ("coef has a wrong number of elements.");
		}

		for (int y = 0; y < NBR_PLANES + 1; ++y)
		{
			for (int x = 0; x < NBR_PLANES + 1; ++x)
			{
				_mat_main [y] [x] = (x == y) ? 1 : 0;

				if (   (x < fmt_src.numPlanes || x == NBR_PLANES)
				    &&  y < fmt_dst.numPlanes)
				{
					int            err = 0;
					const int      index = y * (fmt_src.numPlanes + 1) + x;
					const double   c = _vsapi.propGetFloat (&in, "coef", index, &err);
					if (err != 0)
					{
						throw_rt_err ("error while reading the matrix coefficients.");
					}
					_mat_main [y] [x] = c;
				}
			}
		}

		mat_init_flag = true;
	}

	if (! mat_init_flag)
	{
		throw_inval_arg (
			"you must specify a matrix preset or a custom coefficient list."
		);
	}

	prepare_matrix_coef (
		*this, *_proc_uptr, _mat_main,
		fmt_dst, _full_range_dst_flag,
		fmt_src, _full_range_src_flag,
		_csp_out, _plane_out
	);

	if (_vsapi.getError (&out) != 0)
	{
		throw -1;
	}
}



void	Matrix::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Matrix::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);

	::VSFrameRef *    dst_ptr = 0;
	::VSNodeRef &     node = *_clip_src_sptr;

	if (activation_reason == ::arInitial)
	{
		_vsapi.requestFrameFilter (n, &node, &frame_ctx);
	}

	else if (activation_reason == ::arAllFramesReady)
	{
		vsutl::FrameRefSPtr	src_sptr (
			_vsapi.getFrameFilter (n, &node, &frame_ctx),
			_vsapi
		);
		const ::VSFrameRef & src = *src_sptr;

		const int         w  =  _vsapi.getFrameWidth (&src, 0);
		const int         h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		uint8_t * const   dst_ptr_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			                        _vsapi.getWritePtr (dst_ptr, 0),
			(_plane_out >= 0) ? 0 : _vsapi.getWritePtr (dst_ptr, 1),
			(_plane_out >= 0) ? 0 : _vsapi.getWritePtr (dst_ptr, 2)
		};
		const int         dst_str_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			                        _vsapi.getStride (dst_ptr, 0),
			(_plane_out >= 0) ? 0 : _vsapi.getStride (dst_ptr, 1),
			(_plane_out >= 0) ? 0 : _vsapi.getStride (dst_ptr, 2)
		};
		const uint8_t * const
		                  src_ptr_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			_vsapi.getReadPtr (&src, 0),
			_vsapi.getReadPtr (&src, 1),
			_vsapi.getReadPtr (&src, 2)
		};
		const int         src_str_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			_vsapi.getStride (&src, 0),
			_vsapi.getStride (&src, 1),
			_vsapi.getStride (&src, 2)
		};

		_proc_uptr->process (
			dst_ptr_arr, dst_str_arr,
			src_ptr_arr, src_str_arr,
			w, h
		);

		// Output frame properties
		if (_range_set_dst_flag || _csp_out != fmtcl::ColorSpaceH265_UNSPECIFIED)
		{
			::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

			if (_range_set_dst_flag)
			{
				const int      cr_val = (_full_range_dst_flag) ? 0 : 1;
				_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);
			}

			if (   _csp_out != fmtcl::ColorSpaceH265_UNSPECIFIED
			    && _csp_out <= fmtcl::ColorSpaceH265_ISO_RANGE_LAST)
			{
				_vsapi.propSetInt (&dst_prop, "_Matrix"    , int (_csp_out), ::paReplace);
				_vsapi.propSetInt (&dst_prop, "_ColorSpace", int (_csp_out), ::paReplace);
			}
		}
	}

	return (dst_ptr);
}



// Everything should be lower case at this point
void	Matrix::select_def_mat (std::string &mat, const ::VSFormat &fmt)
{
	if (mat.empty ())
	{
		switch (fmt.colorFamily)
		{
		case	::cmYUV:
			mat = "601";
			break;

		case	::cmYCoCg:
			mat = "ycgco";
			break;

		case	::cmGray:   // Should not happen actually
		case	::cmRGB:
		case	::cmCompat:
		default:
			// Nothing
			break;
		}
	}
}



// mat should be already converted to lower case
fmtcl::ColorSpaceH265	Matrix::find_cs_from_mat_str (const vsutl::FilterBase &flt, const std::string &mat, bool allow_2020cl_flag)
{
	fmtcl::ColorSpaceH265   cs = fmtcl::ColorSpaceH265_UNSPECIFIED;

	if (mat.empty () || mat == "rgb")
	{
		cs = fmtcl::ColorSpaceH265_RGB;
	}
	else if (mat == "601")
	{
		cs = fmtcl::ColorSpaceH265_SMPTE170M;
	}
	else if (mat == "709")
	{
		cs = fmtcl::ColorSpaceH265_BT709;
	}
	else if (mat == "240")
	{
		cs = fmtcl::ColorSpaceH265_SMPTE240M;
	}
	else if (mat == "fcc")
	{
		cs = fmtcl::ColorSpaceH265_FCC;
	}
	else if (mat == "ycgco" || mat == "ycocg")
	{
		cs = fmtcl::ColorSpaceH265_YCGCO;
	}
	else if (mat == "2020")
	{
		cs = fmtcl::ColorSpaceH265_BT2020NCL;
	}
	else if (mat == "2020cl" && allow_2020cl_flag)
	{
		cs = fmtcl::ColorSpaceH265_BT2020CL;
	}
	else
	{
		flt.throw_inval_arg ("unknown matrix identifier.");
	}

	return (cs);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat *	Matrix::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src, int &plane_out, bool &force_col_fam_flag) const
{
	force_col_fam_flag = false;

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
		else
		{
			force_col_fam_flag = true;
		}
	}

	int            col_fam  = fmt_dst_ptr->colorFamily;
	int            spl_type = fmt_dst_ptr->sampleType;
	int            bits     = fmt_dst_ptr->bitsPerSample;
	int            ssh      = fmt_dst_ptr->subSamplingW;
	int            ssv      = fmt_dst_ptr->subSamplingH;

	// Color family
	if (is_arg_defined (in, "col_fam"))
	{
		force_col_fam_flag = true;
		col_fam = get_arg_int (in, out, "col_fam", col_fam);
	}

	if (plane_out >= 0)
	{
		col_fam = ::cmGray;
	}
	else if (col_fam == ::cmGray)
	{
		plane_out = 0;
	}

	// Destination bit depth
	bits = get_arg_int (in, out, "bits", bits);

	// Combines the modified parameters and validates the format
	try
	{
		fmt_dst_ptr = register_format (
			col_fam,
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

	return (fmt_dst_ptr);
}



const ::VSFormat *	Matrix::find_dst_col_fam (fmtcl::ColorSpaceH265 tmp_csp, const ::VSFormat *fmt_dst_ptr, const ::VSFormat &fmt_src, ::VSCore &core)
{
	int               alt_cf = -1;

	switch (tmp_csp)
	{
	case fmtcl::ColorSpaceH265_RGB:
	case fmtcl::ColorSpaceH265_BT709:
	case fmtcl::ColorSpaceH265_FCC:
	case fmtcl::ColorSpaceH265_BT470BG:
	case fmtcl::ColorSpaceH265_SMPTE170M:
	case fmtcl::ColorSpaceH265_SMPTE240M:
	case fmtcl::ColorSpaceH265_BT2020NCL:
	case fmtcl::ColorSpaceH265_BT2020CL:
		alt_cf = ::cmYUV;
		break;

	case fmtcl::ColorSpaceH265_YCGCO:
		alt_cf = ::cmYCoCg;
		break;

	default:
		// Nothing
		break;
	}

	if (alt_cf >= 0)
	{
		int            col_fam  = fmt_dst_ptr->colorFamily;
		int            spl_type = fmt_dst_ptr->sampleType;
		int            bits     = fmt_dst_ptr->bitsPerSample;
		int            ssh      = fmt_dst_ptr->subSamplingW;
		int            ssv      = fmt_dst_ptr->subSamplingH;
		if (fmt_src.colorFamily == ::cmRGB)
		{
			col_fam = alt_cf;
		}
		else if (fmt_src.colorFamily == alt_cf)
		{
			col_fam = ::cmRGB;
		}

		try
		{
			fmt_dst_ptr = register_format (
				col_fam,
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
	}

	return (fmt_dst_ptr);
}



void	Matrix::make_mat_from_str (fmtcl::Mat4 &m, const std::string &mat, bool to_rgb_flag) const
{
	if (mat.empty () || mat == "rgb")
	{
		m[0][0] = 1; m[0][1] = 0; m[0][2] = 0;
		m[1][0] = 0; m[1][1] = 1; m[1][2] = 0;
		m[2][0] = 0; m[2][1] = 0; m[2][2] = 1;
		m.clean3 (1);
	}
	else if (mat == "601")
	{
		make_mat_yuv (m, 0.299, 0.587, 0.114, to_rgb_flag);
	}
	else if (mat == "709")
	{
		make_mat_yuv (m, 0.2126, 0.7152, 0.0722, to_rgb_flag);
	}
	else if (mat == "240")
	{
		make_mat_yuv (m, 0.212, 0.701, 0.087, to_rgb_flag);
	}
	else if (mat == "fcc")
	{
		make_mat_yuv (m, 0.30, 0.59, 0.11, to_rgb_flag);
	}
	else if (mat == "ycgco" || mat == "ycocg")
	{
		make_mat_ycgco (m, to_rgb_flag);
	}
	else if (mat == "2020")
	{
		make_mat_yuv (m, 0.2627, 0.678, 0.0593, to_rgb_flag);
	}
	else
	{
		throw_inval_arg ("unknown matrix identifier.");
	}
}



/*
kr/kg/kb matrix (Rec. ITU-T H.264 03/2010, p. 379):

R = Y                  + V*(1-Kr)
G = Y - U*(1-Kb)*Kb/Kg - V*(1-Kr)*Kr/Kg
B = Y + U*(1-Kb)

Y =                  R * Kr        + G * Kg        + B * Kb
U = (B-Y)/(1-Kb) = - R * Kr/(1-Kb) - G * Kg/(1-Kb) + B
V = (R-Y)/(1-Kr) =   R             - G * Kg/(1-Kr) - B * Kb/(1-Kr)

The given equations work for R, G, B in range [0 ; 1] and U and V in range
[-1 ; 1]. Scaling must be applied to match the required range for U and V.

R, G, B, Y range : [0 ; 1]
U, V range : [-0.5 ; 0.5]
*/

void	Matrix::make_mat_yuv (fmtcl::Mat4 &m, double kr, double kg, double kb, bool to_rgb_flag)
{
	assert (! fstb::is_null (kg));
	assert (! fstb::is_eq (kb, 1.0));
	assert (! fstb::is_eq (kr, 1.0));

	const double   r = 0.5;
	const double   x = 1.0 / r;
	if (to_rgb_flag)
	{
		m[0][0] = 1; m[0][1] =              0; m[0][2] = x*(1-kr)      ;
		m[1][0] = 1; m[1][1] = x*(kb-1)*kb/kg; m[1][2] = x*(kr-1)*kr/kg;
		m[2][0] = 1; m[2][1] = x*(1-kb)      ; m[2][2] =              0;
	}

	else
	{
		m[0][0] =     kr     ; m[0][1] =   kg       ; m[0][2] =   kb       ;
		m[1][0] = r*kr/(kb-1); m[1][1] = r*kg/(kb-1); m[1][2] = r          ;
		m[2][0] = r          ; m[2][1] = r*kg/(kr-1); m[2][2] = r*kb/(kr-1);
	}

	m.clean3 (1);
}



/*
YCgCo matrix (Rec. ITU-T H.264 03/2010, p. 379):

R  = Y - Cg + Co
G  = Y + Cg
B  = Y - Cg - Co

Y  =  0.25 * R + 0.5  * G + 0.25 * B
Cg = -0.25 * R + 0.5  * G - 0.25 * B
Co =  0.5  * R            - 0.5  * B

R, G, B, Y range : [0 ; 1]
Cg, Co range : [-0.5 ; 0.5]

Note: this implementation is not exactly the same as specified because the
standard specifies specific steps to apply the RGB-to-YCgCo matrix, leading
to different roundings.
*/

void	Matrix::make_mat_ycgco (fmtcl::Mat4 &m, bool to_rgb_flag)
{
	if (to_rgb_flag)
	{
		m[0][0] = 1; m[0][1] = -1; m[0][2] =  1;
		m[1][0] = 1; m[1][1] =  1; m[1][2] =  0;
		m[2][0] = 1; m[2][1] = -1; m[2][2] = -1;
	}
	else
	{
		m[0][0] =  0.25; m[0][1] = 0.5; m[0][2] =  0.25;
		m[1][0] = -0.25; m[1][1] = 0.5; m[1][2] = -0.25;
		m[2][0] =  0.5 ; m[2][1] = 0  ; m[2][2] = -0.5 ;
	}

	m.clean3 (1);
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
