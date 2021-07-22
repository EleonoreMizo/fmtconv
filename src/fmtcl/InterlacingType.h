/*****************************************************************************

        InterlacingType.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_InterlacingType_HEADER_INCLUDED)
#define fmtcl_InterlacingType_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



enum InterlacingType
{
	InterlacingType_INVALID = -1,

	InterlacingType_FRAME   = 0,
	InterlacingType_TOP,
	InterlacingType_BOT,

	InterlacingType_NBR_ELT

}; // enum InterlacingType



inline InterlacingType	InterlacingType_get (bool itl_flag, bool top_flag)
{
	return
		(itl_flag) ? ((top_flag) ? InterlacingType_TOP
		                         : InterlacingType_BOT)
		           :               InterlacingType_FRAME;
}



}  // namespace fmtcl



//#include "fmtcl/InterlacingType.hpp"



#endif   // fmtcl_InterlacingType_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

