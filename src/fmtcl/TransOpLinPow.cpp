/*****************************************************************************

        TransOpLinPow.cpp
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

#include "fmtcl/TransOpLinPow.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpLinPow::TransOpLinPow (bool inv_flag, double alpha, double beta, double p1, double slope, double lb, double ub, double scneg, double p2, double scale_cdm2, double wpeak_cdm2)
:	_inv_flag (inv_flag)
,	_alpha (alpha)
,	_beta (beta)
,	_p1 (p1)
,	_slope (slope)
,	_lb (lb)
,	_ub (ub)
,	_scneg (scneg)
,	_p2 (p2)
,	_scale_cdm2 (scale_cdm2)
,	_wpeak_cdm2 (wpeak_cdm2)
{
	_alpha_m1 = _alpha - 1;
	_beta_n   =       -_beta / _scneg;
	_beta_i   =  pow ( _beta   * _slope, _p2);
	_beta_in  = -pow (-_beta_n * _slope, _p2);
	_ub_i     = _alpha * pow (_ub, _p1) - (_alpha - 1);
	if (_lb < _beta_n)
	{
		_lb_i = -(_alpha * pow (-_lb * _scneg, _p1) - _alpha_m1) / _scneg;
	}
	else
	{
		_lb_i = -pow (-_lb * _slope, _p2);
	}
	_p1_i = 1 / p1;
	_p2_i = 1 / p2;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpLinPow::do_convert (double x) const
{
	double         y = x;

	if (_inv_flag)
	{
		x = fstb::limit (x, _lb_i, _ub_i);
		if (x >= _beta_i)
		{
			y =  pow (( x          + _alpha_m1) / _alpha, _p1_i);
		}
		else if (x <= _beta_in)
		{
			y = -pow ((-x * _scneg + _alpha_m1) / _alpha, _p1_i) / _scneg;
		}
		else
		{
			if (fstb::is_eq (_p2, 1.0))
			{
				y =        x         / _slope;
			}
			else if (x < 0)
			{
				y = -pow (-x, _p2_i) / _slope;
			}
			else
			{
				y =  pow ( x, _p2_i) / _slope;
			}
		}
	}

	else
	{
		x = fstb::limit (x, _lb, _ub);
		if (x >= _beta)
		{
			y =   _alpha * pow ( x         , _p1) - _alpha_m1;
		}
		else if (x <= _beta_n)
		{
			y = -(_alpha * pow (-x * _scneg, _p1) - _alpha_m1) / _scneg;
		}
		else
		{
			if (fstb::is_eq (_p2, 1.0))
			{
				y =        x * _slope;
			}
			else if (x < 0)
			{
				y = -pow (-x * _slope, _p2);
			}
			else
			{
				y =  pow ( x * _slope, _p2);
			}
		}
	}

	return y;
}



TransOpInterface::LinInfo	TransOpLinPow::do_get_info () const
{
	return { Type::UNDEF, Range::UNDEF, 1.0, 1.0, _scale_cdm2, _wpeak_cdm2 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
