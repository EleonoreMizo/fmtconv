/*****************************************************************************

        TransOpLogC.cpp
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

#include "fmtcl/TransOpLogC.h"

#include <algorithm>

#include <cassert>
#include <cmath>



namespace fmtcl
{



const double		TransOpLogC::_noise_margin = -8.0 / 65536;



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpLogC::TransOpLogC (bool inv_flag, Type type)
:	_inv_flag (inv_flag)
,	_cut ((type == Type_VLOG) ? 0.01     : (type == Type_LOGC_V2) ? 0.000000 : 0.010591)
,	_a (  (type == Type_VLOG) ? 1.0      : (type == Type_LOGC_V2) ? 5.061087 : 5.555556)
,	_b (  (type == Type_VLOG) ? 0.00873  : (type == Type_LOGC_V2) ? 0.089004 : 0.052272)
,	_c (  (type == Type_VLOG) ? 0.241514 : (type == Type_LOGC_V2) ? 0.247189 : 0.247190)
,	_d (  (type == Type_VLOG) ? 0.598206 : (type == Type_LOGC_V2) ? 0.391007 : 0.385537)
,	_e (  (type == Type_VLOG) ? 5.6      : (type == Type_LOGC_V2) ? 4.950469 : 5.367655)
,	_f (  (type == Type_VLOG) ? 0.125    : (type == Type_LOGC_V2) ? 0.131313 : 0.092809)
,	_n (  (type == Type_VLOG) ? 0        : _noise_margin)
,	_cut_i (_e * _cut + _f)
{
	// Nothing
}



// 1 is log peak white.
double	TransOpLogC::operator () (double x) const
{
	return ((_inv_flag) ? compute_inverse (x) : compute_direct (x));
}

double	TransOpLogC::get_max () const
{
	return (compute_inverse (1.0));
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpLogC::compute_direct (double x) const
{
	x = std::max (x, _n);
	double         y =
		  (x > _cut  )
		? _c * log10 (_a * x + _b) + _d
		: _e * x + _f;
	y = std::min (y, 1.0);

	return (y);
}

double	TransOpLogC::compute_inverse (double x) const
{
	x = std::min (x, 1.0);
	double         y =
		  (x > _cut_i)
		? (pow (10, (x - _d) / _c) - _b) / _a
		: (x - _f) / _e;
	y = std::max (y, _n);

	return (y);
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
