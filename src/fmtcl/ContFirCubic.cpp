/*****************************************************************************

        ContFirCubic.cpp
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

#include "fmtcl/ContFirCubic.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ContFirCubic::ContFirCubic (double b, double c)
:	_p0 ((  6 -  2*b       ) / 6)
,	_p2 ((-18 + 12*b +  6*c) / 6)
,	_p3 (( 12 -  9*b -  6*c) / 6)
,	_q0 ((       8*b + 24*c) / 6)
,	_q1 ((     -12*b - 48*c) / 6)
,	_q2 ((       6*b + 30*c) / 6)
,	_q3 ((    -    b -  6*c) / 6)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	ContFirCubic::do_get_support () const
{
	return (2.0);
}



double	ContFirCubic::do_get_val (double x) const
{
	x = fabs (x);

	return (  (x < 1) ? (_p0 + x *        x * (_p2 + x * _p3))
	        : (x < 2) ? (_q0 + x * (_q1 + x * (_q2 + x * _q3)))
	        : 0.0);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
