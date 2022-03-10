/*****************************************************************************

        TransOpSigmoid.cpp
        Author: Laurent de Soras, 2022

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpSigmoid.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpSigmoid::TransOpSigmoid (bool inv_flag, double curve, double thr)
:	_inv_flag (inv_flag)
,	_c (curve)
,	_t (thr)
,	_x0 (1 / (1 + exp (curve *  thr     )))
,	_x1 (1 / (1 + exp (curve * (thr - 1))))
,	_d0 (inv_flag ? inv_1 (0) : dir_1 (0))
,	_d1 (inv_flag ? inv_1 (1) : dir_1 (1))
{
	assert (curve > 0);
	assert (thr >= 0);
	assert (thr <= 1);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpSigmoid::do_convert (double x) const
{
	// Because the curves may saturate pretty quickly outside the [0 ; 1]
	// range, we extend the curves linearly out of this range using the first
	// derivative value at the range boundaries (C1 continuity).
	return
		  (x < 0)   ?      x      * _d0
		: (x > 1)   ? 1 + (x - 1) * _d1
		: _inv_flag ? inv_0 (x)
		:             dir_0 (x);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpSigmoid::inv_0 (double x) const noexcept
{
	return (1 / (1 + exp (_c * (_t - x))) - _x0) / (_x1 - _x0);
}



double	TransOpSigmoid::inv_1 (double x) const noexcept
{
	const auto     ectmx = exp (_c * (_t - x));
	return _c * ectmx / ((_x1 - _x0) * fstb::sq (ectmx + 1));
}



double	TransOpSigmoid::dir_0 (double x) const noexcept
{
	const auto     lerp_x0x1 = _x0 + x * (_x1 - _x0);
	return _t - log (1 / lerp_x0x1 - 1) / _c;
}



double	TransOpSigmoid::dir_1 (double x) const noexcept
{
	const auto     lerp_x0x1 = _x0 + x * (_x1 - _x0);
	return (_x0 - _x1) / (_c * lerp_x0x1 * (lerp_x0x1 - 1));
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
