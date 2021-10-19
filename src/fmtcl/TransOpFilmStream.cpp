/*****************************************************************************

        TransOpFilmStream.cpp
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

#include "fmtcl/TransOpFilmStream.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpFilmStream::TransOpFilmStream (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Linear 1 is the sensor clipping level (3840 on a linear 12-bit scale).
double	TransOpFilmStream::do_convert (double x) const
{
	constexpr double  sc10 = 1024;
	constexpr double  bl12 =   64;
	constexpr double  wl12 = 3840;
	constexpr double  mi10 =    3 / sc10;
	constexpr double  ma10 = 1020 / sc10;
	constexpr double  sp   =  500;
	constexpr double  sl   =    0.02714;

	double         y = x;
	if (_inv_flag)
	{
		x = fstb::limit (x, mi10, ma10);
		const double   sensor = pow (10, x * (sc10 / sp)) / sl;
		y = (sensor - bl12) / (wl12 - bl12);
	}
	else
	{
		const double   sensor = bl12 + x * (wl12 - bl12);
		if (sensor <= 37)
		{
			y = 0;
		}
		else
		{
			y = (sp / sc10) * log10 (sl * sensor);
			y = fstb::limit (y, mi10, ma10);
		}
	}

	return y;
}



TransOpInterface::LinInfo	TransOpFilmStream::do_get_info () const
{
	// R, G and B sensors don't have the same sensitivity, so white level
	// has no real meaning here.
	return { Type::OETF, Range::UNDEF, 1.0, 1.0, 0.0, 0.0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
