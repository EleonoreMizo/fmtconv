/*****************************************************************************

        TransOpLog3G10.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpLog3G10.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpLog3G10::TransOpLog3G10 (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpLog3G10::do_convert (double x) const
{
	return (_inv_flag) ? gamma_to_lin (x) : lin_to_gamma (x);
}



TransOpInterface::LinInfo	TransOpLog3G10::do_get_info () const
{
	const double   lin_max = gamma_to_lin (1);

	return { Type::UNDEF, Range::HDR, lin_max, 1.0, 0, 0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpLog3G10::gamma_to_lin (double x) const noexcept
{
	auto           y = (x < 0) ? x / _g : (pow (10.0, x / _a) - 1) / _b;
	y -= _c;

	return y;
}



double	TransOpLog3G10::lin_to_gamma (double x) const noexcept
{
	x += _c;

	if (x < 0)
	{
		return x * _g;
	}
	return _a * log10 (x * _b + 1);
}



constexpr double	TransOpLog3G10::_a;
constexpr double	TransOpLog3G10::_b;
constexpr double	TransOpLog3G10::_c;
constexpr double	TransOpLog3G10::_g;



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
