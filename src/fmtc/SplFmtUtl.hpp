/*****************************************************************************

        SplFmtUtl.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_SplFmtUtl_CODEHEADER_INCLUDED)
#define	fmtc_SplFmtUtl_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "VapourSynth.h"

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



fmtcl::SplFmt	SplFmtUtl::conv_from_vsformat (const ::VSFormat &fmt)
{
	assert (&fmt != 0);

	fmtcl::SplFmt  type = fmtcl::SplFmt_ILLEGAL;

	if (fmt.sampleType == ::stFloat && fmt.bitsPerSample == 32)
	{
		type = fmtcl::SplFmt_FLOAT;
	}
	else
	{
		if (fmt.bitsPerSample > 8 && fmt.bitsPerSample <= 16)
		{
			type = fmtcl::SplFmt_INT16;
		}
		else if (fmt.bitsPerSample <= 8)
		{
			type = fmtcl::SplFmt_INT8;
		}
	}

	return (type);
}



void	SplFmtUtl::conv_from_vsformat (fmtcl::SplFmt &type, int &bitdepth, const ::VSFormat &fmt)
{
	assert (&type != 0);
	assert (&bitdepth != 0);
	assert (&fmt != 0);

	type     = conv_from_vsformat (fmt);
	bitdepth = fmt.bitsPerSample;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtc



#endif	// fmtc_SplFmtUtl_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
