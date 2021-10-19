/*****************************************************************************

        TransOpErimm.cpp
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

#include "fmtcl/TransOpErimm.h"
#include "fstb/def.h"

#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpErimm::TransOpErimm (bool inv_flag)
:	_inv_flag (inv_flag)
,	_log10_eclip (2.5)
,	_eclip (pow (10, _log10_eclip))
,	_log10_emin (-3)
,	_et (fstb::EXP1 * 0.001)
,	_log10_et (log10 (_et))
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpErimm::do_convert (double x) const
{
	double         y = x;

	if (_inv_flag)
	{
		if (x < 0)
		{
			y = 0;
		}
		else if (x < (_log10_et - _log10_emin) / (_log10_eclip - _log10_emin))
		{
			y = x * ((_log10_eclip - _log10_emin) / (_log10_et - _log10_emin)) * _et;
		}
		else if (x < 1)
		{
			y = pow (10, x * (_log10_eclip - _log10_emin) + _log10_emin);
		}
		else 
		{
			y = _eclip;
		}
	}

	else
	{
		if (x < 0)
		{
			y = 0;
		}
		else if (x < _et)
		{
			y = ((_log10_et - _log10_emin) / (_log10_eclip - _log10_emin)) * (x / _et);
		}
		else if (x < _eclip)
		{
			y = (log10 (x) - _log10_emin) / (_log10_eclip - _log10_emin);
		}
		else
		{
			y = 1;
		}
	}

	return y;
}



TransOpInterface::LinInfo	TransOpErimm::do_get_info () const
{
	return { Type::OETF, Range::UNDEF, _eclip, 1.0, 0.0, 0.0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
