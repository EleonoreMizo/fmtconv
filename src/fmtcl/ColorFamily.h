/*****************************************************************************

        ColorFamily.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ColorFamily_HEADER_INCLUDED)
#define	fmtcl_ColorFamily_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



enum ColorFamily
{
	ColorFamily_INVALID = -1,

	ColorFamily_GRAY = 0,
	ColorFamily_RGB,
	ColorFamily_YUV,

	ColorFamily_NBR_ELT

};	// enum ColorFamily



}	// namespace fmtcl



//#include "fmtcl/ColorFamily.hpp"



#endif	// fmtcl_ColorFamily_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
