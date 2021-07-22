/*****************************************************************************

        PrimUtil.cpp
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

#include "fmtcl/PrimUtil.h"
#include "fstb/fnc.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	PrimUtil::_nbr_planes;



Mat3	PrimUtil::compute_conversion_matrix (const RgbSystem &prim_s, const RgbSystem &prim_d)
{
	assert (prim_s.is_ready ());
	assert (prim_d.is_ready ());

	const Mat3     rgb2xyz = compute_rgb2xyz (prim_s);
	const Mat3     xyz2rgb = compute_rgb2xyz (prim_d).invert ();
	const Mat3     adapt   = compute_chroma_adapt (prim_s, prim_d);

	return xyz2rgb * adapt * rgb2xyz;
}



// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
Mat3	PrimUtil::compute_rgb2xyz (const RgbSystem &prim)
{
	assert (prim.is_ready ());

	Mat3           m;

	if (prim._preset == PrimariesPreset_CIEXYZ)
	{
		m = Mat3 (1, Mat3::Preset_DIAGONAL);
	}

	else
	{
		const Vec3     white = conv_xy_to_xyz (prim._white);

		Mat3           xyzrgb;
		for (int k = 0; k < _nbr_planes; ++k)
		{
			Vec3           comp_xyz = conv_xy_to_xyz (prim._rgb [k]);
			xyzrgb.set_col (k, comp_xyz);
		}

		const Vec3     s = xyzrgb.compute_inverse () * white;

		for (int u = 0; u < _nbr_planes; ++u)
		{
			m.set_col (u, xyzrgb.get_col (u) * s [u]);
		}
	}

	return m;
}



// http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
Mat3	PrimUtil::compute_chroma_adapt (const RgbSystem &prim_s, const RgbSystem &prim_d)
{
	assert (prim_s.is_ready ());
	assert (prim_d.is_ready ());

	const Vec3     white_s = conv_xy_to_xyz (prim_s._white);
	const Vec3     white_d = conv_xy_to_xyz (prim_d._white);

	// Bradford adaptation
	const Mat3     ma ({
		Vec3 {  0.8951,  0.2664, -0.1614 },
		Vec3 { -0.7502,  1.7135,  0.0367 },
		Vec3 {  0.0389, -0.0685,  1.0296 }
	});

	Vec3    crd_s = ma * white_s;
	Vec3    crd_d = ma * white_d;
	Mat3    scale (0.0);
	for (int k = 0; k < _nbr_planes; ++k)
	{
		assert (crd_s [k] != 0);
		scale [k] [k] = crd_d [k] / crd_s [k];
	}

	return ma.compute_inverse () * scale * ma;
}



// Obtains X, Y, Z from (x, y)
// Y is assumed to be 1.0
// X =      x      / y
// Z = (1 - x - y) / y
// http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
Vec3	PrimUtil::conv_xy_to_xyz (const RgbSystem::Vec2 &xy)
{
	Vec3           xyz;

	// When y is null, X = Y = Z = 0.
	if (fstb::is_null (xy [1]))
	{
		xyz [0] = 0;
		xyz [1] = 0;
		xyz [2] = 0;
	}
	else
	{
		xyz [0] =      xy [0]           / xy [1];
		xyz [1] = 1;
		xyz [2] = (1 - xy [0] - xy [1]) / xy [1];
	}

	return xyz;
}



// str should be already converted to lower case
PrimariesPreset	PrimUtil::conv_string_to_primaries (const std::string &str)
{
	assert (! str.empty ());

	PrimariesPreset  preset = PrimariesPreset_UNDEF;

	if (        str == "709"
	         || str == "1361"
	         || str == "61966-2-1"
	         || str == "61966-2-4"
	         || str == "hdtv"
	         || str == "srgb")
	{
		preset = PrimariesPreset_BT709;
	}
	else if (   str == "470m"
	         || str == "ntsc")
	{
		preset = PrimariesPreset_FCC;
	}
	else if (   str == "470m93"
	         || str == "ntscj")
	{
		preset = PrimariesPreset_NTSCJ;
	}
	else if (   str == "470bg"
	         || str == "601-625"
	         || str == "1358-625"
	         || str == "1700-625"
	         || str == "pal"
	         || str == "secam")
	{
		preset = PrimariesPreset_BT470BG;
	}
	else if (   str == "170m"
	         || str == "601-525"
	         || str == "1358-525"
	         || str == "1700-525")
	{
		preset = PrimariesPreset_SMPTE170M;
	}
	else if (   str == "240m")
	{
		preset = PrimariesPreset_SMPTE240M;
	}
	else if (   str == "filmc")
	{
		preset = PrimariesPreset_GENERIC_FILM;
	}
	else if (   str == "2020"
	         || str == "2100"
	         || str == "uhdtv")
	{
		preset = PrimariesPreset_BT2020;
	}
	else if (   str == "61966-2-2"
	         || str == "scrgb")
	{
		preset = PrimariesPreset_SCRGB;
	}
	else if (   str == "adobe98")
	{
		preset = PrimariesPreset_ADOBE_RGB_98;
	}
	else if (   str == "adobewide")
	{
		preset = PrimariesPreset_ADOBE_RGB_WIDE;
	}
	else if (   str == "apple")
	{
		preset = PrimariesPreset_APPLE_RGB;
	}
	else if (   str == "photopro"
	         || str == "romm")
	{
		preset = PrimariesPreset_ROMM;
	}
	else if (   str == "ciergb")
	{
		preset = PrimariesPreset_CIERGB;
	}
	else if (   str == "ciexyz")
	{
		preset = PrimariesPreset_CIEXYZ;
	}
	else if (   str == "p3d65"
	         || str == "dcip3")
	{
		preset = PrimariesPreset_P3D65;
	}
	else if (   str == "aces")
	{
		preset = PrimariesPreset_ACES;
	}
	else if (   str == "ap1")
	{
		preset = PrimariesPreset_ACESAP1;
	}
	else if (   str == "sgamut"
	         || str == "sgamut3")
	{
		preset = PrimariesPreset_SGAMUT;
	}
	else if (   str == "sgamut3cine")
	{
		preset = PrimariesPreset_SGAMUT3CINE;
	}
	else if (   str == "alexa")
	{
		preset = PrimariesPreset_ALEXA;
	}
	else if (   str == "vgamut")
	{
		preset = PrimariesPreset_VGAMUT;
	}
	else if (   str == "p3dci")
	{
		preset = PrimariesPreset_P3DCI;
	}
	else if (   str == "p3d60")
	{
		preset = PrimariesPreset_P3D60;
	}
	else if (   str == "3213")
	{
		preset = PrimariesPreset_EBU3213E;
	}

	return preset;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
