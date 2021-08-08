/*****************************************************************************

        CsPlane.cpp
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
#include "avisynth.h"

#include <cassert>



namespace avsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	CsPlane::_max_nbr_planes;
constexpr int	CsPlane::_plane_index_alpha;



CsPlane::CategCs	CsPlane::get_cs_categ (const ::VideoInfo &vi) noexcept
{
	return vi.IsRGB () ? CategCs_RGB : CategCs_YUV;
}



const CsPlane::CategInfo &	CsPlane::use_categ_info (const ::VideoInfo &vi) noexcept
{
	const CategCs  categ = get_cs_categ (vi);

	return _plane_info_list [categ];
}



const CsPlane::PlaneInfo &	CsPlane::use_plane_info (int plane_index, const ::VideoInfo &vi) noexcept
{
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);
	assert (plane_index < vi.NumComponents ());

	const auto &   categ_info = use_categ_info (vi);

	return categ_info [plane_index];
}



int	CsPlane::get_plane_id (int plane_index, const ::VideoInfo &vi) noexcept
{
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);
	assert (plane_index < vi.NumComponents ());

	const auto &   plane_info = use_plane_info (plane_index, vi);

	return plane_info._id;
}



const std::array <
	CsPlane::CategInfo,
	CsPlane::CategCs_NBR_ELT
>	CsPlane::_plane_info_list =
{{
	{{
		{ ::PLANAR_Y, 'y' },
		{ ::PLANAR_U, 'u' },
		{ ::PLANAR_V, 'v' },
		{ ::PLANAR_A, 'a' }
	}},
	{{
		{ ::PLANAR_R, 'r' },
		{ ::PLANAR_G, 'g' },
		{ ::PLANAR_B, 'b' },
		{ ::PLANAR_A, 'a' }
	}}
}};



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace avsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
