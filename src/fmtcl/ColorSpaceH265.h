/*****************************************************************************

        ColorSpaceH265.h
        Author: Laurent de Soras, 2013

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ColorSpaceH265_HEADER_INCLUDED)
#define	fmtcl_ColorSpaceH265_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



// ITU-T H.265, High efficiency video coding, 2019-06, p. 415
enum ColorSpaceH265
{
	ColorSpaceH265_UNDEF = -1,

	ColorSpaceH265_RGB = 0,       // RGB or XYZ
	ColorSpaceH265_BT709,
	ColorSpaceH265_UNSPECIFIED,
	ColorSpaceH265_RESERVED,
	ColorSpaceH265_FCC,
	ColorSpaceH265_BT470BG,
	ColorSpaceH265_SMPTE170M,
	ColorSpaceH265_SMPTE240M,
	ColorSpaceH265_YCGCO,
	ColorSpaceH265_BT2020NCL,
	ColorSpaceH265_BT2020CL,
	ColorSpaceH265_YDZDX,
	ColorSpaceH265_CHRODERNCL,
	ColorSpaceH265_CHRODERCL,
	ColorSpaceH265_ICTCP,         // This matrix depends on the transfer characteristic

	ColorSpaceH265_NBR_ELT,

	ColorSpaceH265_ISO_RANGE_LAST = 255,

	ColorSpaceH265_CUSTOM = 1000,

	ColorSpaceH265_LMS,
	ColorSpaceH265_ICTCP_PQ,
	ColorSpaceH265_ICTCP_HLG

};	// enum ColorSpaceH265



}	// namespace fmtcl



//#include "fmtcl/ColorSpaceH265.hpp"



#endif	// fmtcl_ColorSpaceH265_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
