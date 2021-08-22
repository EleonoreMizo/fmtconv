/*****************************************************************************

        ProcComp3Arg.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ProcComp3Arg_HEADER_INCLUDED)
#define fmtcl_ProcComp3Arg_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Frame.h"
#include "fmtcl/FrameRO.h"

#include <cstdint>


namespace fmtcl
{



class ProcComp3Arg
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _nbr_planes = 3;

	               ProcComp3Arg ()                          = default;
	               ProcComp3Arg (const ProcComp3Arg &other) = default;
	               ProcComp3Arg (ProcComp3Arg &&other)      = default;
	               ~ProcComp3Arg ()                         = default;
	ProcComp3Arg & operator = (const ProcComp3Arg &other)   = default;
	ProcComp3Arg & operator = (ProcComp3Arg &&other)        = default;

	bool           is_valid (bool single_plane_out_flag = false) const noexcept;

	Frame <>       _dst {};
	FrameRO <>     _src {};

	int            _w = 0; // Pixels, no subsampling
	int            _h = 0; // Full or half-frame height (required for Stack16 formats)



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const ProcComp3Arg &other) const = delete;
	bool           operator != (const ProcComp3Arg &other) const = delete;

}; // class ProcComp3Arg



}  // namespace fmtcl



//#include "fmtcl/ProcComp3Arg.hpp"



#endif   // fmtcl_ProcComp3Arg_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
