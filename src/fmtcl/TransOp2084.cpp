/*****************************************************************************

        TransOp2084.cpp
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

#include "fmtcl/TransOp2084.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOp2084::TransOp2084 (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Linear values are absolute. 1 is 10000 cd/m2.
double	TransOp2084::do_convert (double x) const
{
	x = fstb::limit (x, 0.0, 1.0);
	double         y = x;

	constexpr double  c1 =   1.0  * 3424 / 4096;
	constexpr double  c2 =  32.0  * 2413 / 4096;
	constexpr double  c3 =  32.0  * 2392 / 4096;
	constexpr double  m  = 128.0  * 2523 / 4096;
	constexpr double  n  =   0.25 * 2610 / 4096;

	// Makes sure that f(0) = 0
	if (x > 0)
	{
		if (_inv_flag)
		{
			// Inverse formula from:
			// Scott Miller, Mahdi Nezamabadi, Scott Daly
			// Perceptual Signal Coding for More Efficient Usage of Bit Codes, p. 5
			// Presentation for 2012 SMPTE Annual Technical Conference & Exhibition
			const double   xp = pow (x, 1 / m);
			const double   r  = (xp - c1) / (c2 - c3 * xp);
			if (r < 0)
			{
				y = 0;
			}
			else
			{
				y = pow (r, 1 / n);
			}
		}
		else
		{
			const double   xp = pow (x, n);
			y = pow ((c1 + c2 * xp) / (1 + c3 * xp), m);
		}
	}

	return y;
}



TransOpInterface::LinInfo	TransOp2084::do_get_info () const
{
	constexpr double  w_peak = 10'000.0; // cd/m^2
	constexpr double  w_ref  =    203.152145938; // cd/m^2
	// w_ref = 1000 * EOTF_HLG (0.75)
	//       = 1000 * OOTF_HLG (OETF_HLG^-1 (0.75))
	//       = 1000 * (0.264962560421 ^ 1.2)

	return { Type::EOTF, Range::HDR, 1.0, w_ref / w_peak, w_peak, w_peak };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
