/*****************************************************************************

        TransOpPowOfs.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/TransOpPowOfs.h"

#include <algorithm>

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// m = maximum code value
// w = code for reference white
// d = divisor (scale)
// b = code for black
TransOpPowOfs::TransOpPowOfs (bool inv_flag, double m, double w, double d, double b)
:	_inv_flag (inv_flag)
{
	assert (m > 0);
	assert (d > 0);
	assert (w > b);

	const auto     l  = pow (10, (b - w) / d);
	const auto     dd = fstb::LN10 / d;
	_kx = m * dd;
	_kw = w * dd + log (1 - l);
	_kl = l / (1 - l);
}



double	TransOpPowOfs::do_convert (double x) const
{
	return (_inv_flag) ? gamma_to_lin (x) : lin_to_gamma (x);
}



TransOpInterface::LinInfo	TransOpPowOfs::do_get_info () const
{
	const double   lin_max = gamma_to_lin (1);

	return { Type::UNDEF, Range::UNDEF, lin_max, 1.0, 0, 0 };
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpPowOfs::gamma_to_lin (double x) const noexcept
{
	auto           xe = x * _kx - _kw;
	xe = std::min (xe, 20.0);

	return exp (xe) - _kl;
}



double	TransOpPowOfs::lin_to_gamma (double x) const noexcept
{
	auto           xl = x + _kl;
	xl = std::max (x, 1e-20);

	return (log (xl) + _kw) / _kx;
}



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
