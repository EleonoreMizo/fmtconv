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



// Linear 1 is the sensor clipping level (3840 on a linear 12-bit scale).
double	TransOpFilmStream::operator () (double x) const
{
	static const double  sc10    = 1024;
	static const double  bl12    = 64;
	static const double  wl12    = 3840;
	static const double  mi10    =    3 / sc10;
	static const double  ma10    = 1020 / sc10;
	static const double  sp      = 500;
	static const double  sl      = 0.02714;

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

	return (y);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
