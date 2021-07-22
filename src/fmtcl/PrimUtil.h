/*****************************************************************************

        PrimUtil.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_PrimUtil_HEADER_INCLUDED)
#define fmtcl_PrimUtil_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Mat3.h"
#include "fmtcl/RgbSystem.h"

#include <string>



namespace fmtcl
{



class PrimUtil
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _nbr_planes = RgbSystem::_nbr_planes;

	static Mat3    compute_conversion_matrix (const RgbSystem &prim_s, const RgbSystem &prim_d);
	static Mat3    compute_rgb2xyz (const RgbSystem &prim);
	static Mat3    compute_chroma_adapt (const RgbSystem &prim_s, const RgbSystem &prim_d);
	static Vec3    conv_xy_to_xyz (const RgbSystem::Vec2 &xy);
	static PrimariesPreset
	               conv_string_to_primaries (const std::string &str);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               PrimUtil ()                               = delete;
	               PrimUtil (const PrimUtil &other)          = delete;
	               PrimUtil (PrimUtil &&other)               = delete;
	PrimUtil &     operator = (const PrimUtil &other)        = delete;
	PrimUtil &     operator = (PrimUtil &&other)             = delete;
	bool           operator == (const PrimUtil &other) const = delete;
	bool           operator != (const PrimUtil &other) const = delete;

}; // class PrimUtil



}  // namespace fmtcl



//#include "fmtcl/PrimUtil.hpp"



#endif   // fmtcl_PrimUtil_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
