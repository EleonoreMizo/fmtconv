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
#include "fmtcl/Mat4.h"
#include "fmtcl/MatrixProc.h"
#include "vsutl/FilterBase.h"
#include "vsutl/fnc.h"
#include "VapourSynth.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

	
	
static void	override_fmt_with_csp (::VSFormat &fmt, fmtcl::ColorSpaceH265 csp_out, int plane_out)
{
	assert (&fmt != 0);

	if (plane_out >= 0)
	{
		fmt.numPlanes = 3;
		if (csp_out == fmtcl::ColorSpaceH265_RGB)
		{
			fmt.colorFamily = ::cmRGB;
		}
		else if (csp_out == fmtcl::ColorSpaceH265_YCGCO)
		{
			fmt.colorFamily = ::cmYCoCg;
		}
		else
		{
			fmt.colorFamily = ::cmYUV;
		}
	}
}



// Int: depends on the input format (may be float too)
// R, G, B, Y: [0 ; 1]
// U, V, Cg, Co : [-0.5 ; 0.5]
static void	make_mat_flt_int (fmtcl::Mat4 &m, bool to_flt_flag, const ::VSFormat &fmt, bool full_flag)
{
	assert (&m != 0);
	assert (&fmt != 0);

	::VSFormat     fmt2 (fmt);
	fmt2.sampleType = ::stFloat;

	const ::VSFormat* fmt_src_ptr = &fmt2;
	const ::VSFormat* fmt_dst_ptr = &fmt;
	if (to_flt_flag)
	{
		std::swap (fmt_src_ptr, fmt_dst_ptr);
	}

	double         ay, by;
	double         ac, bc;
	const int      ch_plane = (fmt_dst_ptr->numPlanes > 1) ? 1 : 0;
	vsutl::compute_fmt_mac_cst (
		ay, by, *fmt_dst_ptr, full_flag, *fmt_src_ptr, full_flag, 0
	);
	vsutl::compute_fmt_mac_cst (
		ac, bc, *fmt_dst_ptr, full_flag, *fmt_src_ptr, full_flag, ch_plane
	);

	m[0][0] = ay; m[0][1] =  0; m[0][2] =  0; m[0][3] = by;
	m[1][0] =  0; m[1][1] = ac; m[1][2] =  0; m[1][3] = bc;
	m[2][0] =  0; m[2][1] =  0; m[2][2] = ac; m[2][3] = bc;
	m[3][0] =  0; m[3][1] =  0; m[3][2] =  0; m[3][3] =  1;
}



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



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

	return (splfmt);
}



void	prepare_matrix_coef (const vsutl::FilterBase &filter, fmtcl::MatrixProc &mat_proc, const fmtcl::Mat4 &mat_main, const ::VSFormat &fmt_dst, bool full_range_dst_flag, const ::VSFormat &fmt_src, bool full_range_src_flag, fmtcl::ColorSpaceH265 csp_out, int plane_out)
{
	const bool     int_proc_flag =
		(   fmt_src.sampleType == ::stInteger
		 && fmt_dst.sampleType == ::stInteger);

	fmtcl::Mat4    m (1, fmtcl::Mat4::Preset_DIAGONAL);

	::VSFormat     fmt_dst2 = fmt_dst;
	if (int_proc_flag)
	{
		// For the coefficient calculation, use the same output bitdepth
		// as the input. The bitdepth change will be done separately with
		// a simple bitshift.
		fmt_dst2.bitsPerSample = fmt_src.bitsPerSample;
	}

	override_fmt_with_csp (fmt_dst2, csp_out, plane_out);

	fmtcl::Mat4    m1s;
	fmtcl::Mat4    m1d;
	make_mat_flt_int (m1s, true , fmt_src , full_range_src_flag);
	make_mat_flt_int (m1d, false, fmt_dst2, full_range_dst_flag);
	m *= m1d;
	if (! int_proc_flag)
	{
		if (plane_out > 0 && vsutl::is_chroma_plane (fmt_dst2, plane_out))
		{
			// When we extract a single plane, it's a conversion to R or
			// to Y, so the outout range is always [0; 1]. Therefore we
			// need to offset the chroma planes.
			m [plane_out] [fmtcl::MatrixProc::NBR_PLANES] += 0.5;
		}
	}
	m *= mat_main;
	m *= m1s;

	const fmtcl::SplFmt  splfmt_src = conv_vsfmt_to_splfmt (fmt_src);
	const fmtcl::SplFmt  splfmt_dst = conv_vsfmt_to_splfmt (fmt_dst);
	const fmtcl::MatrixProc::Err  ret_val = mat_proc.configure (
		m, int_proc_flag,
		splfmt_src, fmt_src.bitsPerSample,
		splfmt_dst, fmt_dst.bitsPerSample,
		plane_out
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
