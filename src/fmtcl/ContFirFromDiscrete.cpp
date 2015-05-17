/*****************************************************************************

        ContFirFromDiscrete.cpp
        Author: Laurent de Soras, 2011

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

#include "fmtcl/ContFirFromDiscrete.h"
#include "fmtcl/DiscreteFirInterface.h"
#include "fstb/fnc.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ContFirFromDiscrete::ContFirFromDiscrete (const DiscreteFirInterface &discrete)
:	_discrete (discrete)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	ContFirFromDiscrete::do_get_support () const
{
	const int      len     = _discrete.get_len ();
	const double   ovrspl  = _discrete.get_ovrspl ();
	const double   support = (len + 1) / (2 * ovrspl);

	return (support);
}



double	ContFirFromDiscrete::do_get_val (double x) const
{
	const int      len     = _discrete.get_len ();
	const double   ovrspl  = _discrete.get_ovrspl ();
	const double   pos_flt = x * ovrspl + (len - 1) / 2;
	const int      pos_0   = fstb::floor_int (pos_flt);
	const int      pos_1   = pos_0 + 1;
	const double   frac    = pos_flt - pos_0;

	double         v_0 = 0;
	double         v_1 = 0;
	if (pos_0 >= 0 && pos_0 < len)
	{
		v_0 = _discrete.get_val (pos_0);
	}
	if (pos_1 >= 0 && pos_1 < len)
	{
		v_1 = _discrete.get_val (pos_1);
	}

	const double   val = v_0 + (v_1 - v_0) * frac;

	return (val);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
