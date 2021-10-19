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



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpSLog3::do_convert (double x) const
{
	x = std::max (x, 0.0);

	return (_inv_flag) ? log_to_lin (x) : lin_to_log (x);
}



TransOpInterface::LinInfo	TransOpSLog3::do_get_info () const
{
	return {
		Type::UNDEF,
		Range::UNDEF,
		log_to_lin (1.0),
		log_to_lin (598.0 / 1023.0),
		0.0, 0.0
	};
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpSLog3::log_to_lin (double x)
{
	return
		  (x < 171.2102946929 / 1023.0)
		? (x * 1023.0 - 95.0) * 0.01125000 / (171.2102946929 - 95.0)
		: (pow (10, (x * 1023.0 - 420.0) / 261.5)) * (0.18 + 0.01) - 0.01;
}



double	TransOpSLog3::lin_to_log (double x)
{
	return
		  (x < 0.01125000)
		? (x * (171.2102946929 - 95.0) / 0.01125000 + 95.0) / 1023.0
		: (420.0 + log10 ((x + 0.01) / (0.18 + 0.01)) * 261.5) / 1023.0;
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
