/*****************************************************************************

        RgbSystem.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_RgbSystem_HEADER_INCLUDED)
#define fmtcl_RgbSystem_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/PrimariesPreset.h"

#include <array>



namespace fmtcl
{



class RgbSystem
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _nbr_planes = 3;

	class Vec2
	:	public std::array <double, _nbr_planes - 1>
	{
		typedef std::array <double, _nbr_planes - 1> Inherited;
	public:
		               Vec2 () = default;
		               Vec2 (double c0, double c1);
	};

	               RgbSystem ();
	               RgbSystem (const RgbSystem &other)  = default;
	               RgbSystem (RgbSystem &&other)       = default;
	virtual        ~RgbSystem ()                       = default;
	RgbSystem &    operator = (const RgbSystem &other) = default;
	RgbSystem &    operator = (RgbSystem &&other)      = default;

	void           set (PrimariesPreset preset);
	bool           is_ready () const;

	std::array <Vec2, _nbr_planes>      // x,y coordinates for R, G and B
	               _rgb;
	Vec2           _white;              // XYZ coordinates for the ref. white
	std::array <bool, _nbr_planes + 1>  // R, G, B, W
	               _init_flag_arr;
	fmtcl::PrimariesPreset              // If known
	               _preset;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const RgbSystem &other) const = delete;
	bool           operator != (const RgbSystem &other) const = delete;

}; // class RgbSystem



}  // namespace fmtcl



//#include "fmtcl/RgbSystem.hpp"



#endif   // fmtcl_RgbSystem_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
