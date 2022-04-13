/*****************************************************************************

        TransOpAcesCct.cpp
        Author: Laurent de Soras, 2022

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpAcesCct.h"

#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpAcesCct::TransOpAcesCct (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpAcesCct::do_convert (double x) const
{
	constexpr auto a0  =  0.0792055341958355;
	constexpr auto a1  = 10.5402377416545;
	constexpr auto thr =  0.0078125;
	constexpr auto b0  =  9.72;
	constexpr auto b1  = 17.52;


	double         y = x;

	if (_inv_flag)
	{
		if (x <= thr * a1 + a0)
		{
			y = (x - a0) / a1;
		}
		else
		{
			y = exp2 (x * b1 - b0);
		}
	}

	else
	{
		if (x <= thr)
		{
			y = x * a1 + a0;
		}
		else
		{
			y = (log2 (x) + b0) / b1;
		}
	}

	return y;
}



TransOpInterface::LinInfo	TransOpAcesCct::do_get_info () const
{
	return { Type::OETF, Range::UNDEF, _max_val, 1.0, 0.0, 0.0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr double	TransOpAcesCct::_max_val;



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
