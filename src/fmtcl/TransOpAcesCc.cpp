/*****************************************************************************

        TransOpAcesCc.cpp
        Author: Laurent de Soras, 2016

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

#include "fmtcl/TransOpAcesCc.h"

#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpAcesCc::TransOpAcesCc (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpAcesCc::do_convert (double x) const
{
	double         y = x;

	if (_inv_flag)
	{
		x *= 17.52;
		x -= 9.72;
		if (x <= 15)
		{
			y = exp2 (x + 1) - 1.0 / (1 << 15);
		}
		else if (x <= log2 (_max_val))
		{
			y = exp2 (x    );
		}
		else
		{
			y = _max_val;
		}
	}

	else
	{
		if (x < 0)
		{
			y = -16;
		}
		else if (x < 1.0 / (1 << 15))
		{
			y = log2 (1.0 / (1 << 15) + x) - 1;
		}
		else
		{
			y = log2 (                  x);
		}
		y += 9.72;
		y /= 17.52;
	}

	return y;
}



TransOpInterface::LinInfo	TransOpAcesCc::do_get_info () const
{
	return { Type::UNDEF, Range::UNDEF, _max_val, 1.0, 0.0, 0.0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr double	TransOpAcesCc::_max_val;



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
