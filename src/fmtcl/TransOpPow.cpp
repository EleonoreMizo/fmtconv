/*****************************************************************************

        TransOpPow.cpp
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

#include "fmtcl/TransOpPow.h"

#include <algorithm>

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// p_i: power curve for the EOTF or OETF^-1 ( > 1)
// alpha: scaling factor after EOTF^-1 or OETF
TransOpPow::TransOpPow (bool inv_flag, double p_i, double alpha, double val_max, double scale_cdm2, double wpeak_cdm2)
:	_inv_flag (inv_flag)
,	_p_i (p_i)
,	_alpha (alpha)
,	_p (1 / p_i)
,	_val_max (val_max)
,	_scale_cdm2 (scale_cdm2)
,	_wpeak_cdm2 (wpeak_cdm2)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpPow::do_convert (double x) const
{
	x = std::max (x, 0.0);
	double         y = x;

	if (_inv_flag)
	{
		y = pow (x / _alpha, _p_i);
		y = std::min (y, _val_max);
	}
	else
	{
		x = std::min (x, _val_max);
		y = _alpha * pow (x, _p);
	}

	return y;
}



TransOpInterface::LinInfo	TransOpPow::do_get_info () const
{
	return { Type::UNDEF, Range::SDR, _val_max, 1.0, _scale_cdm2, _wpeak_cdm2 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
