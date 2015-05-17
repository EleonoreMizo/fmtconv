/*****************************************************************************

        ContFirSpline36.cpp
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

#include "fmtcl/ContFirSpline36.h"

#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	ContFirSpline36::do_get_support () const
{
	return (3.0);
}



double	ContFirSpline36::do_get_val (double x) const
{
	x = fabs (x);

	double         v = 0;
	if (x < 1.0)
	{
		v = ((13.0/11.0 * x - 453.0/209.0) * x -   3.0/209.0) * x + 1.0;
	}
	else if (x < 2.0)
	{
		x -= 1.0;
		v = ((-6.0/11.0 * x + 270.0/209.0) * x - 156.0/209.0) * x;
	}
	else if (x < 3.0)
	{
		x -= 2.0;
		v = (( 1.0/11.0 * x -  45.0/209.0) * x +  26.0/209.0) * x;
	}

	return (v);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
