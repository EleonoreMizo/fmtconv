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

#include "fmtcl/Cst.h"
#include "fmtcl/fnc.h"
#include "fmtcl/Mat4.h"
#include "fmtcl/MatrixProc.h"
#include "fmtcl/PicFmt.h"

#include <algorithm>

#include <cassert>
#include <cstdint>



namespace fmtcl
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



static void	override_fmt_with_csp (PicFmt &fmt, ColorSpaceH265 csp_out, int plane_out)
{
	if (plane_out >= 0)
	{
		if (csp_out == ColorSpaceH265_RGB)
		{
			fmt._col_fam = ColorFamily_RGB;
		}
		else if (csp_out == ColorSpaceH265_YCGCO)
		{
			fmt._col_fam = ColorFamily_YCGCO;
		}
		else
		{
			fmt._col_fam = ColorFamily_YUV;
		}
	}
}



// Int: depends on the input format (may be float too)
// R, G, B, Y: [0 ; 1]
// U, V, Cg, Co : [-0.5 ; 0.5]
static void	make_mat_flt_int (Mat4 &m, bool to_flt_flag, const PicFmt &fmt)
{
	PicFmt         fmt2 (fmt);
	fmt2._sf = SplFmt_FLOAT;

	const PicFmt * fmt_src_ptr = &fmt2;
	const PicFmt * fmt_dst_ptr = &fmt;
	if (to_flt_flag)
	{
		std::swap (fmt_src_ptr, fmt_dst_ptr);
	}

	double         ay, by;
	double         ac, bc;
	const int      ch_plane = (fmt_dst_ptr->_col_fam != ColorFamily_GRAY) ? 1 : 0;
	compute_fmt_mac_cst (ay, by, *fmt_dst_ptr, *fmt_src_ptr, 0       );
	compute_fmt_mac_cst (ac, bc, *fmt_dst_ptr, *fmt_src_ptr, ch_plane);

	m[0][0] = ay; m[0][1] =  0; m[0][2] =  0; m[0][3] = by;
	m[1][0] =  0; m[1][1] = ac; m[1][2] =  0; m[1][3] = bc;
	m[2][0] =  0; m[2][1] =  0; m[2][2] = ac; m[2][3] = bc;
	m[3][0] =  0; m[3][1] =  0; m[3][2] =  0; m[3][3] =  1;
}



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	compute_plane_width (ColorFamily col_fam, int ss_h, int base_w, int plane_index)
{
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);
	assert (plane_index >= 0);
	assert (ss_h >= 0);
	assert (base_w >= 0);

	int            plane_w = base_w;
	if (is_chroma_plane (col_fam, plane_index))
	{
		assert ((base_w & ((1 << ss_h) - 1)) == 0);
		plane_w >>= ss_h;
	}

	return plane_w;
}



int	compute_plane_height (ColorFamily col_fam, int ss_v, int base_h, int plane_index)
{
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);
	assert (plane_index >= 0);
	assert (ss_v >= 0);
	assert (base_h >= 0);

	int            plane_h = base_h;
	if (is_chroma_plane (col_fam, plane_index))
	{
		assert ((base_h & ((1 << ss_v) - 1)) == 0);
		plane_h >>= ss_v;
	}

	return plane_h;
}



bool	has_chroma (ColorFamily col_fam)
{
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);

	return (   col_fam == ColorFamily_YUV
	        || col_fam == ColorFamily_YCGCO);
}



bool	is_chroma_plane (ColorFamily col_fam, int plane_index)
{
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);
	assert (plane_index >= 0);

	return (has_chroma (col_fam) && plane_index > 0 && plane_index < 3);
}



bool	is_full_range_default (ColorFamily col_fam)
{
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);

	return (   col_fam == ColorFamily_RGB
	        || col_fam == ColorFamily_YCGCO);
}



double	compute_pix_scale (const PicFmt &fmt, int plane_index)
{
	assert (fmt.is_valid ());
	assert (plane_index >= 0);

	double         scale = 1.0;

	if (fmt._sf != SplFmt_FLOAT)
	{
		const int      bps_m8 = fmt._res - 8;
		if (fmt._full_flag || plane_index == 3)
		{
			scale = double ((uint64_t (1) << fmt._res) - 1);
		}
		else if (is_chroma_plane (fmt._col_fam, plane_index))
		{
			scale = double ((uint64_t (Cst::_rtv_chr_dep * 2)) << bps_m8);
		}
		else
		{
			constexpr auto range = Cst::_rtv_lum_wht - Cst::_rtv_lum_blk;
			scale = double ((uint64_t (range)) << bps_m8);
		}
	}

	return scale;
}



double	get_pix_min (const PicFmt &fmt, int plane_index)
{
	assert (fmt.is_valid ());
	assert (plane_index >= 0);

	double         add_val     = 0;
	const bool     chroma_flag = is_chroma_plane (fmt._col_fam, plane_index);

	if (fmt._sf == SplFmt_FLOAT)
	{
		if (chroma_flag)
		{
			add_val = -0.5;
		}
	}
	else if (fmt._full_flag)
	{
		if (chroma_flag)
		{
			// So the neutral value (0) is exactly: 1 << (nbr_bits - 1)
			add_val = 0.5;
		}
	}
	else if (plane_index < 3)
	{
		if (chroma_flag)
		{
			constexpr auto min_val = Cst::_rtv_chr_gry - Cst::_rtv_chr_dep;
			add_val = double ((uint64_t (min_val)) << (fmt._res - 8));
		}
		else
		{
			add_val = double ((uint64_t (Cst::_rtv_lum_blk)) << (fmt._res - 8));
		}
	}

	return add_val;
}



void	compute_fmt_mac_cst (double &gain, double &add_cst, const PicFmt &dst_fmt, const PicFmt &src_fmt, int plane_index)
{
	// (X_d - m_d) / S_d  =  (X_s - m_s) / S_s
	// X_d = X_s * (S_d / S_s) + (m_d - m_s * S_d / S_s)
	//                gain              add_cst
	const double   scale_src = compute_pix_scale (src_fmt, plane_index);
	const double   scale_dst = compute_pix_scale (dst_fmt, plane_index);
	gain = scale_dst / scale_src;

	const double   cst_src = get_pix_min (src_fmt, plane_index);
	const double   cst_dst = get_pix_min (dst_fmt, plane_index);
	add_cst = cst_dst - cst_src * gain;
}



int	prepare_matrix_coef (MatrixProc &mat_proc, const Mat4 &mat_main, const PicFmt &dst_fmt, const PicFmt &src_fmt, ColorSpaceH265 csp_out, int plane_out)
{
	const bool     int_proc_flag =
		(SplFmt_is_int (src_fmt._sf) && SplFmt_is_int (dst_fmt._sf));

	Mat4           m (1, Mat4::Preset_DIAGONAL);

	PicFmt         dst_fmt2 = dst_fmt;
	if (int_proc_flag)
	{
		// For the coefficient calculation, use the same output bitdepth
		// as the input. The bitdepth change will be done separately with
		// a simple bitshift.
		dst_fmt2._res = src_fmt._res;
	}

	override_fmt_with_csp (dst_fmt2, csp_out, plane_out);

	Mat4           m1s;
	Mat4           m1d;
	make_mat_flt_int (m1s, true , src_fmt );
	make_mat_flt_int (m1d, false, dst_fmt2);
	m *= m1d;
	if (! int_proc_flag)
	{
		if (plane_out >= 0 && is_chroma_plane (dst_fmt2._col_fam, plane_out))
		{
			// When we extract a single plane, it's a conversion to R or
			// to Y, so the outout range is always [0; 1]. Therefore we
			// need to offset the chroma planes.
			m [plane_out] [MatrixProc::_nbr_planes] += 0.5;
		}
	}
	m *= mat_main;
	m *= m1s;

	const MatrixProc::Err   ret_val = mat_proc.configure (
		m, int_proc_flag,
		src_fmt._sf, src_fmt._res,
		dst_fmt._sf, dst_fmt._res,
		plane_out
	);

	return ret_val;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
