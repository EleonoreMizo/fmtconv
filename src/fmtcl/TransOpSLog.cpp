/*****************************************************************************

        TransOpSLog.cpp
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

#include "fmtcl/TransOpSLog.h"

#include <algorithm>

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpSLog::TransOpSLog (bool inv_flag, bool slog2_flag)
:	_inv_flag (inv_flag)
,	_slog2_flag (slog2_flag)
{
	// Nothing
}



// 1 lin is reference white, peak white at 10 lin.
double	TransOpSLog::operator () (double x) const
{
	return (_inv_flag) ? compute_inverse (x) : compute_direct (x);
}



double	TransOpSLog::get_max () const
{
	return (_slog2_flag) ? 10.0 * _s2 : 10.0;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr double	TransOpSLog::_a;
constexpr double	TransOpSLog::_b;
constexpr double	TransOpSLog::_c1;
constexpr double	TransOpSLog::_c2;
constexpr double	TransOpSLog::_c;
constexpr double	TransOpSLog::_d;
constexpr double	TransOpSLog::_s2;



double	TransOpSLog::compute_direct (double x) const
{
	if (_slog2_flag)
	{
		x /= _s2;
	}
	double         y = x;
	if (x < 0)
	{
		y = x * _d + _c2;
	}
	else
	{
		y = _b * log10 (x + _a) + _c;
	}

	return y;
}



double	TransOpSLog::compute_inverse (double x) const
{
	double         y = x;
	if (x < _c2)
	{
		y = (x - _c2) / _d;
	}
	else
	{
		y = pow (10, (x - _c) / _b) - _a;
	}
	if (_slog2_flag)
	{
		y *= _s2;
	}

	return y;
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
