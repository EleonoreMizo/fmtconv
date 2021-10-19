/*****************************************************************************

        LumMatch.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_LumMatch_HEADER_INCLUDED)
#define fmtcl_LumMatch_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



enum LumMatch
{

	LumMatch_UNDEF = -1,

	LumMatch_NONE  = 0,
	LumMatch_REF_WHITE,
	LumMatch_LUMINANCE,

	LumMatch_NBR_ELT

}; // enum LumMatch



}  // namespace fmtcl



//#include "fmtcl/LumMatch.hpp"



#endif   // fmtcl_LumMatch_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
