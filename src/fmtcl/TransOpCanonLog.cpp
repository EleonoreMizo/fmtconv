/*****************************************************************************

        TransOpCanonLog.cpp
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

#include "fmtcl/TransOpCanonLog.h"

#include <algorithm>

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpCanonLog::TransOpCanonLog (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// 1.08676 is log peak white, at 8.00903 in linear scale.
double	TransOpCanonLog::do_convert (double x) const
{
	static constexpr double a = 10.1596;
	static constexpr double b =  0.529136;
	static constexpr double c =  0.0730597;

	double         y = x;
	if (_inv_flag)
	{
		y = (pow (10, (y - c) / b) - 1) / a;
	}
	else
	{
		y = b * log10 (std::max (x, -0.0452664) * a + 1) + c;
	}

	return y;
}



TransOpInterface::LinInfo	TransOpCanonLog::do_get_info () const
{
	return { Type::OETF, Range::UNDEF, 8.00903, 1.0, 0.0, 0.0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
