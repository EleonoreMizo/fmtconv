/*****************************************************************************

        fnc.cpp
        Author: Laurent de Soras, 2012

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

#include "fstb/fnc.h"
#include "vsutl/fnc.h"

#include <cassert>
#include <cstdint>



namespace vsutl
{



bool	is_vs_gray (int cf)
{
	return (cf == ::cfGray || cf == ::cmGray);
}



bool	is_vs_rgb (int cf)
{
	return (cf == ::cfRGB || cf == ::cmRGB);
}



bool	is_vs_yuv (int cf)
{
	return (cf == ::cfYUV || cf == ::cmYUV);
}



bool	is_vs_same_colfam (int lhs, int rhs)
{
	return (
		   is_vs_gray (lhs) == is_vs_gray (rhs)
		&& is_vs_rgb ( lhs) == is_vs_rgb ( rhs)
		&& is_vs_yuv ( lhs) == is_vs_yuv ( rhs)
	);
}



bool	is_constant_format (const ::VSVideoInfo &vi)
{
	return (vi.height > 0 && vi.width > 0 && vi.format != 0);
}



bool	has_chroma (const ::VSFormat &fmt)
{
	return (   is_vs_yuv (fmt.colorFamily)
	        || fmt.colorFamily == ::cmYCoCg);
}



bool	is_chroma_plane (const ::VSFormat &fmt, int plane_index)
{
	assert (plane_index >= 0);
	assert (plane_index < fmt.numPlanes);

	return (has_chroma (fmt) && plane_index > 0);
}



bool	is_full_range_default (const ::VSFormat &fmt)
{
	return (   is_vs_rgb (fmt.colorFamily)
	        || fmt.colorFamily == ::cmYCoCg);
}



double	compute_pix_scale (const ::VSFormat &fmt, int plane_index, bool full_flag)
{
	assert (plane_index >= 0);
	assert (plane_index < fmt.numPlanes);

	double         scale = 1.0;

	if (fmt.sampleType == ::stInteger)
	{
		const int      bps_m8 = fmt.bitsPerSample - 8;
		if (full_flag)
		{
			scale = double ((uint64_t (1) << fmt.bitsPerSample) - 1);
		}
		else if (is_chroma_plane (fmt, plane_index))
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



double	get_pix_min (const ::VSFormat &fmt, int plane_index, bool full_flag)
{
	assert (plane_index >= 0);
	assert (plane_index < fmt.numPlanes);

	double         add_val = 0;

	if (fmt.sampleType == ::stFloat)
	{
		if (is_chroma_plane (fmt, plane_index))
		{
			add_val = -0.5;
		}
	}
	else if (full_flag)
	{
		if (is_chroma_plane (fmt, plane_index))
		{
			// So the neutral value (0) is exactly: 1 << (nbr_bits - 1)
			add_val = 0.5;
		}
	}
	else
	{
		add_val = double ((uint64_t (16)) << (fmt.bitsPerSample - 8));
	}

	return (add_val);
}



void	compute_fmt_mac_cst (double &gain, double &add_cst, const ::VSFormat &fmt_dst, bool full_dst_flag, const ::VSFormat &fmt_src, bool full_src_flag, int plane_index)
{
	assert (plane_index >= 0);
	assert (plane_index < fmt_dst.numPlanes);
	assert (plane_index < fmt_src.numPlanes);

	// (X_d - m_d) / S_d  =  (X_s - m_s) / S_s
	// X_d = X_s * (S_d / S_s) + (m_d - m_s * S_d / S_s)
	//                gain              add_cst
	const double   scale_src = compute_pix_scale (fmt_src, plane_index, full_src_flag);
	const double   scale_dst = compute_pix_scale (fmt_dst, plane_index, full_dst_flag);
	gain = scale_dst / scale_src;

	const double   cst_src = get_pix_min (fmt_src, plane_index, full_src_flag );
	const double   cst_dst = get_pix_min (fmt_dst, plane_index, full_dst_flag);
	add_cst = cst_dst - cst_src * gain;
}



int	compute_plane_width (const ::VSFormat &fmt, int plane_index, int base_w)
{
	assert (plane_index >= 0);
	assert (plane_index < fmt.numPlanes);
	assert (base_w >= 0);

	if (is_chroma_plane (fmt, plane_index))
	{
		assert ((base_w & ((1 << fmt.subSamplingW) - 1)) == 0);
		base_w >>= fmt.subSamplingW;
	}

	return base_w;
}



int	compute_plane_height (const ::VSFormat &fmt, int plane_index, int base_h)
{
	assert (plane_index >= 0);
	assert (plane_index < fmt.numPlanes);
	assert (base_h >= 0);

	if (is_chroma_plane (fmt, plane_index))
	{
		assert ((base_h & ((1 << fmt.subSamplingH) - 1)) == 0);
		base_h >>= fmt.subSamplingH;
	}

	return base_h;
}



int	conv_str_to_chroma_subspl (int &ssh, int &ssv, std::string css)
{
	assert (! css.empty ());

	int            ret_val = 0;

	fstb::conv_to_lower_case (css);

	if (css == "444" || css == "4:4:4")
	{
		ssh = 0;
		ssv = 0;
	}
	else if (css == "422" || css == "4:2:2")
	{
		ssh = 1;
		ssv = 0;
	}
	else if (css == "420" || css == "4:2:0")
	{
		ssh = 1;
		ssv = 1;
	}
	else if (css == "411" || css == "4:1:1")
	{
		ssh = 2;
		ssv = 0;
	}
	else if (css.length () == 2 && isdigit (css [0]) && isdigit (css [1]))
	{
		const int      ssh2 = css [0] - '0';
		const int      ssv2 = css [1] - '0';
		static const int  log2table [10] = { -1, 0, 1, -1, 2, -1, -1, -1, 3, -1 };
		if (ssh2 < 0 || ssh2 > 9 || ssv2 < 0 || ssv2 > 9)
		{
			ret_val = -2;
		}
		else
		{
			ssh = log2table [ssh2];
			ssv = log2table [ssv2];
			if (ssh < 0 || ssv < 0)
			{
				ret_val = -3;
			}
		}
	}
	else
	{
		ret_val = -1;
	}

	return ret_val;
}



}	// namespace vsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
