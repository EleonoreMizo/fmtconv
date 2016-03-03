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

#include "fmtcl/fnc.h"

#include <cassert>
#include <cstdint>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



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



double	compute_pix_scale (SplFmt spl_fmt, int nbr_bits, ColorFamily col_fam, int plane_index, bool full_flag)
{
	assert (spl_fmt >= 0);
	assert (spl_fmt < SplFmt_NBR_ELT);
	assert (nbr_bits > 0);
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);
	assert (plane_index >= 0);

	double         scale = 1.0;

	if (spl_fmt != SplFmt_FLOAT)
	{
		const int      bps_m8 = nbr_bits - 8;
		if (full_flag)
		{
			scale = double ((uint64_t (1) << nbr_bits) - 1);
		}
		else if (is_chroma_plane (col_fam, plane_index))
		{
			scale = double ((uint64_t (224)) << bps_m8);
		}
		else
		{
			scale = double ((uint64_t (219)) << bps_m8);
		}
	}

	return (scale);
}



double	get_pix_min (SplFmt spl_fmt, int nbr_bits, ColorFamily col_fam, int plane_index, bool full_flag)
{
	assert (spl_fmt >= 0);
	assert (spl_fmt < SplFmt_NBR_ELT);
	assert (nbr_bits > 0);
	assert (col_fam >= 0);
	assert (col_fam < ColorFamily_NBR_ELT);
	assert (plane_index >= 0);

	double         add_val = 0;

	if (spl_fmt == SplFmt_FLOAT)
	{
		if (is_chroma_plane (col_fam, plane_index))
		{
			add_val = -0.5;
		}
	}
	else if (full_flag)
	{
		if (is_chroma_plane (col_fam, plane_index))
		{
			// So the neutral value (0) is exactly: 1 << (nbr_bits - 1)
			add_val = 0.5;
		}
	}
	else
	{
		add_val = double ((uint64_t (16)) << (nbr_bits - 8));
	}

	return (add_val);
}



void	compute_fmt_mac_cst (double &gain, double &add_cst, SplFmt dst_spl_fmt, int dst_nbr_bits, ColorFamily dst_col_fam, bool dst_full_flag, SplFmt src_spl_fmt, int src_nbr_bits, ColorFamily src_col_fam, bool src_full_flag, int plane_index)
{
	// (X_d - m_d) / S_d  =  (X_s - m_s) / S_s
	// X_d = X_s * (S_d / S_s) + (m_d - m_s * S_d / S_s)
	//                gain              add_cst
	const double   scale_src = compute_pix_scale (
		src_spl_fmt, src_nbr_bits, src_col_fam, plane_index, src_full_flag
	);
	const double   scale_dst = compute_pix_scale (
		dst_spl_fmt, dst_nbr_bits, dst_col_fam, plane_index, dst_full_flag
	);
	gain = scale_dst / scale_src;

	const double   cst_src = get_pix_min (
		src_spl_fmt, src_nbr_bits, src_col_fam, plane_index, src_full_flag
	);
	const double   cst_dst = get_pix_min (
		dst_spl_fmt, dst_nbr_bits, dst_col_fam, plane_index, dst_full_flag
	);
	add_cst = cst_dst - cst_src * gain;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
