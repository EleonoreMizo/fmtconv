/*****************************************************************************

        TransOpSLog3.cpp
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

#include "fmtcl/TransOpSLog3.h"

#include <algorithm>

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpSLog3::TransOpSLog3 (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



// 1 lin is reference white, peak white at 10 lin.
double	TransOpSLog3::operator () (double x) const
{
	x = std::max (x, 0.0);
	double         y = x;

	if (_inv_flag)
	{
		if (x < 171.2102946929 / 1023.0)
		{
			y = (x * 1023.0 - 95.0) * 0.01125000 / (171.2102946929 - 95.0);
		}
		else
		{
			y = (pow (10, (x * 1023.0 - 420.0) / 261.5)) * (0.18 + 0.01) - 0.01;
		}
	}
	else
	{
		if (x < 0.01125000)
		{
			y = (x * (171.2102946929 - 95.0) / 0.01125000 + 95.0) / 1023.0;
		}
		else
		{
			y = (420.0 + log10 ((x + 0.01) / (0.18 + 0.01)) * 261.5) / 1023.0;
		}
	}

	return (y);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
