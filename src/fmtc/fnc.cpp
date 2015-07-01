/*****************************************************************************

        fnc.cpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtc/fnc.h"
#include "VapourSynth.h"

#include <cassert>



namespace fmtc
{



fmtcl::SplFmt	conv_vsfmt_to_splfmt (const ::VSFormat &fmt)
{
	fmtcl::SplFmt  splfmt = fmtcl::SplFmt_ILLEGAL;

	if (fmt.sampleType == ::stFloat)
	{
		if (fmt.bitsPerSample == 32)
		{
			splfmt = fmtcl::SplFmt_FLOAT;
		}
	}
	else
	{
		if (fmt.bitsPerSample <= 8)
		{
			splfmt = fmtcl::SplFmt_INT8;
		}
		else if (fmt.bitsPerSample <= 16)
		{
			splfmt = fmtcl::SplFmt_INT16;
		}
	}

	return (splfmt);
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
