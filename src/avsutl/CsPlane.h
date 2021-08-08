/*****************************************************************************

        CsPlane.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (avsutl_CsPlane_HEADER_INCLUDED)
#define avsutl_CsPlane_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <array>



struct VideoInfo;

namespace avsutl
{



class CsPlane
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _max_nbr_planes    = 4;
	static constexpr int _plane_index_alpha = 3;

	enum CategCs
	{
		CategCs_YUV = 0,
		CategCs_RGB,

		CategCs_NBR_ELT
	};

	struct PlaneInfo
	{
		int            _id;   // PLANAR_?
		char           _name; // Lower case
	};

	typedef std::array <PlaneInfo, _max_nbr_planes> CategInfo;

	static CategCs get_cs_categ (const ::VideoInfo &vi) noexcept;
	static const CategInfo &
	               use_categ_info (const ::VideoInfo &vi) noexcept;
	static const PlaneInfo &
	               use_plane_info (int plane_index, const ::VideoInfo &vi) noexcept;
	static int     get_plane_id (int plane_index, const ::VideoInfo &vi) noexcept;

	static const std::array <CategInfo, CategCs_NBR_ELT>
	               _plane_info_list;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:




/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               CsPlane ()                               = delete;
	               CsPlane (const CsPlane &other)           = delete;
	               CsPlane (CsPlane &&other)                = delete;
	CsPlane &      operator = (const CsPlane &other)        = delete;
	CsPlane &      operator = (CsPlane &&other)             = delete;
	bool           operator == (const CsPlane &other) const = delete;
	bool           operator != (const CsPlane &other) const = delete;

}; // class CsPlane



}  // namespace avsutl



//#include "avsutl/CsPlane.hpp"



#endif   // avsutl_CsPlane_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
