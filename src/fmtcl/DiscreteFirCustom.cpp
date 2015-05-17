/*****************************************************************************

        DiscreteFirCustom.cpp
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

#include "fmtcl/DiscreteFirCustom.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



DiscreteFirCustom::DiscreteFirCustom (double ovrspl, double coef_ptr [], int len)
:	_coef_arr (len)
,	_ovrspl (ovrspl)
{
	assert (len > 0);
	assert (ovrspl > 0);
	assert (coef_ptr != 0);

	for (int pos = 0; pos < len; ++pos)
	{
		_coef_arr [pos] = coef_ptr [pos];
	}
}



DiscreteFirCustom::DiscreteFirCustom (double ovrspl, const std::vector <double> &coef_arr)
:	_coef_arr (coef_arr)
,	_ovrspl (ovrspl)
{
	assert (coef_arr.size () > 0);
	assert (ovrspl > 0);
}



// The number of non-zero coefficients from the center (inclusive)
// Returns at least 1, even if all coefficients are 0.
int	DiscreteFirCustom::compute_real_support () const
{
	const int      len      = int (_coef_arr.size ());
	const int      half_len = (len - 1) / 2;
	int            sup      = half_len + 1;
	for (int pos = 0
	;	pos < half_len && _coef_arr [pos] == 0 && _coef_arr [len - 1 - pos] == 0
	;	++pos)
	{
		sup = half_len - pos;
	}

	return (sup);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	DiscreteFirCustom::do_get_len () const
{
	return (int (_coef_arr.size ()));
}



double	DiscreteFirCustom::do_get_ovrspl () const
{
	return (_ovrspl);
}



double	DiscreteFirCustom::do_get_val (int pos) const
{
	return (_coef_arr [pos]);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
