/*****************************************************************************

        ContFirBlackman.cpp
        Author: Laurent de Soras, 2011

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

#include "fmtcl/ContFirBlackman.h"
#include "fstb/def.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ContFirBlackman::ContFirBlackman (int taps)
:	_taps (taps)
{
	assert (taps >= 1);
	assert (taps <= 128);
}



double	ContFirBlackman::compute_win_coef (double x) const
{
	double         win = 0;
	if (fabs (x) <= _taps)
	{
		win = compute_win_coef_no_check (x);
	}

	return (win);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	ContFirBlackman::do_get_support () const
{
	return (_taps);
}



double	ContFirBlackman::do_get_val (double x) const
{
	double      val = 0;
	if (fabs (x) <= _taps)
	{
		const double   win = compute_win_coef_no_check (x);
		val = fstb::sinc (x) * win;
	}

	return (val);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	ContFirBlackman::compute_win_coef_no_check (double x) const
{
	const double   w_x = x * (fstb::PI / _taps);
	const double   win =
		  0.42
		+ 0.50 * cos (w_x    )
		+ 0.08 * cos (w_x * 2);

	return (win);
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
