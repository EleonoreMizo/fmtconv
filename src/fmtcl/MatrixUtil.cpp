/*****************************************************************************

        MatrixUtil.cpp
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

#include "fstb/fnc.h"
#include "fmtcl/Mat3.h"
#include "fmtcl/Mat4.h"
#include "fmtcl/MatrixUtil.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// mat should be already converted to lower case
void	MatrixUtil::select_def_mat (std::string &mat, ColorFamily col_fam)
{
	if (mat.empty ())
	{
		switch (col_fam)
		{
		case	ColorFamily_YUV:
			mat = "601";
			break;

		case	ColorFamily_YCGCO:
			mat = "ycgco";
			break;

		case	ColorFamily_GRAY: // Should not happen actually
		case	ColorFamily_RGB:
		default:
			// Nothing
			break;
		}
	}
}



// mat should be already converted to lower case
// Returns ColorSpaceH265_UNDEF if mat is unknown
ColorSpaceH265	MatrixUtil::find_cs_from_mat_str (const std::string &mat, bool allow_2020cl_flag)
{
	ColorSpaceH265   cs = ColorSpaceH265_UNSPECIFIED;

	if (mat.empty () || mat == "rgb")
	{
		cs = ColorSpaceH265_RGB;
	}
	else if (mat == "601")
	{
		cs = ColorSpaceH265_SMPTE170M;
	}
	else if (mat == "709")
	{
		cs = ColorSpaceH265_BT709;
	}
	else if (mat == "240")
	{
		cs = ColorSpaceH265_SMPTE240M;
	}
	else if (mat == "fcc")
	{
		cs = ColorSpaceH265_FCC;
	}
	else if (mat == "ycgco" || mat == "ycocg")
	{
		cs = ColorSpaceH265_YCGCO;
	}
	else if (mat == "2020")
	{
		cs = ColorSpaceH265_BT2020NCL;
	}
	else if (mat == "2020cl" && allow_2020cl_flag)
	{
		cs = ColorSpaceH265_BT2020CL;
	}
	else if (mat == "ydzdx")
	{
		cs = ColorSpaceH265_YDZDX;
	}
	else if (mat == "lms")
	{
		cs = ColorSpaceH265_LMS;
	}
	else if (mat == "ictcp_pq")
	{
		cs = ColorSpaceH265_ICTCP_PQ;
	}
	else if (mat == "ictcp_hlg")
	{
		cs = ColorSpaceH265_ICTCP_HLG;
	}

	// Unknown matrix identifier
	else
	{
		assert (false);
		cs = ColorSpaceH265_UNDEF;
	}

	return cs;
}



// ycgco_flag indicates that YCgCo should be treated as a separate colorspace
// (not YUV)
ColorFamily	MatrixUtil::find_cf_from_cs (ColorSpaceH265 cs, bool ycgco_flag)
{
	assert (cs >= 0);
	assert (cs < ColorSpaceH265_NBR_ELT);

	ColorFamily    cf = ColorFamily_INVALID;

	switch (cs)
	{
	case ColorSpaceH265_RGB:
	case ColorSpaceH265_LMS:
		cf = ColorFamily_RGB;
		break;

	case ColorSpaceH265_YCGCO:
		cf = (ycgco_flag) ? ColorFamily_YCGCO: ColorFamily_YUV;
		break;

	case ColorSpaceH265_BT709:
	case ColorSpaceH265_FCC:
	case ColorSpaceH265_BT470BG:
	case ColorSpaceH265_SMPTE170M:
	case ColorSpaceH265_SMPTE240M:
	case ColorSpaceH265_BT2020NCL:
	case ColorSpaceH265_BT2020CL:
	case ColorSpaceH265_YDZDX:
	case ColorSpaceH265_CHRODERNCL:
	case ColorSpaceH265_CHRODERCL:
	case ColorSpaceH265_ICTCP:
	case ColorSpaceH265_ICTCP_PQ:
	case ColorSpaceH265_ICTCP_HLG:
		cf = ColorFamily_YUV;
		break;

	default:
		assert (false);
	}

	return cf;
}



// Returns -1 if mat is unknown
int	MatrixUtil::make_mat_from_str (Mat4 &m, const std::string &mat, bool to_rgb_flag)
{
	int            ret_val = 0;

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
	else if (mat == "ydzdx")
	{
		make_mat_ydzdx (m, to_rgb_flag);
	}
	else if (mat == "lms")
	{
		make_mat_lms (m, to_rgb_flag);
	}
	else if (mat == "ictcp_pq")
	{
		make_mat_ictcp (m, false, to_rgb_flag);
	}
	else if (mat == "ictcp_hlg")
	{
		make_mat_ictcp (m, true, to_rgb_flag);
	}
	else
	{
		assert (false);
		ret_val = -1;
	}

	return ret_val;
}



/*
kr/kg/kb matrix (Rec. ITU-T H.265 2019-06, p. 413):

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

void	MatrixUtil::make_mat_yuv (Mat4 &m, double kr, double kg, double kb, bool to_rgb_flag)
{
	assert (! fstb::is_null (kg));
	assert (! fstb::is_eq (kb, 1.0));
	assert (! fstb::is_eq (kr, 1.0));

	constexpr double  r = 0.5;
	constexpr double  x = 1.0 / r;
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
YCgCo matrix (Rec. ITU-T H.265 2019-06, p. 413):

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

void	MatrixUtil::make_mat_ycgco (Mat4 &m, bool to_rgb_flag)
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



/*
YDzDx transform (Rec. ITU-T H.265 2019-06, p. 414)

Y  = G
Dz = 0.5 * (0.986566 * B - Y)
Dx = 0.5 * (R - 0.991902 * Y)

Y  =                      G
Dz =         - 0.5      * G + 0.493283 * B
Dx = 0.5 * R - 0.495951 * G
*/

void	MatrixUtil::make_mat_ydzdx (Mat4 &m, bool to_rgb_flag)
{
	Mat3           m3;
	m3[0][0] = 0  ; m3[0][1] =  1       ; m3[0][2] = 0;
	m3[1][0] = 0  ; m3[1][1] = -0.5     ; m3[1][2] = 0.493283;
	m3[2][0] = 0.5; m3[2][1] = -0.495951; m3[2][2] = 0;

	if (to_rgb_flag)
	{
		m3.invert ();
	}

	m.insert3 (m3);
	m.clean3 (1);
}



/*
LMS transform (Rec. ITU-T H.265 2019-06, p. 411)

LMS is an intermediate colorspace for ICtCp transforms.
LMS data are conveyed on RGB planes.
Here, to_rgb_flag indicates real RGB target.
*/

void	MatrixUtil::make_mat_lms (Mat4 &m, bool to_rgb_flag)
{
	Mat3           m3;
	m3[0][0] = 1688; m3[0][1] = 2146; m3[0][2] =  262;
	m3[1][0] =  683; m3[1][1] = 2951; m3[1][2] =  462;
	m3[2][0] =   99; m3[2][1] =  309; m3[2][2] = 3688;
	m3 *= 1.0 / 4096;

	if (to_rgb_flag)
	{
		m3.invert ();
	}

	m.insert3 (m3);
	m.clean3 (1);
}



/*
ICtCp transfrom from and to LMS (Rec. ITU-T H.265 2019-06, p. 414)

LMS data are conveyed on RGB planes.
*/

void	MatrixUtil::make_mat_ictcp (Mat4 &m, bool hlg_flag, bool to_lms_flag)
{
	Mat3           m3;
	m3[0][0] =  2048; m3[0][1] =   2048; m3[0][2] =    0;
	if (hlg_flag)
	{
		m3[1][0] =  3625; m3[1][1] =  -7465; m3[1][2] = 3840;
		m3[2][0] =  9500; m3[2][1] =  -9212; m3[2][2] = -288;
	}
	else
	{
		m3[1][0] =  6610; m3[1][1] = -13613; m3[1][2] = 7003;
		m3[2][0] = 17933; m3[2][1] = -17390; m3[2][2] = -543;
	}
	m3 *= 1.0 / 4096;

	if (to_lms_flag)
	{
		m3.invert ();
	}

	m.insert3 (m3);
	m.clean3 (1);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
