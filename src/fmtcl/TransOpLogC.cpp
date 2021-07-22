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



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpLogC::TransOpLogC (bool inv_flag, Type type, ExpIdx ei)
:	_inv_flag (inv_flag)
,	_n ((type == Type_VLOG) ? 0 : _noise_margin)
,	_curve (
		  (type == Type_VLOG   ) ? _vlog
		: (type == Type_LOGC_V2) ? _v2_table [ei]
		:                          _v3_table [ei]
	)
{
	assert (ei >= 0);
	assert (ei < ExpIdx_NBR_ELT);
}



// 1 is log peak white.
double	TransOpLogC::operator () (double x) const
{
	return (_inv_flag) ? compute_inverse (x) : compute_direct (x);
}

double	TransOpLogC::get_max () const
{
	return compute_inverse (1.0);
}



TransOpLogC::ExpIdx	TransOpLogC::conv_logc_ei (int val_raw)
{
	ExpIdx         ei = ExpIdx_INVALID;

	switch (val_raw)
	{
	case  160: ei = ExpIdx_160;  break;
	case  200: ei = ExpIdx_200;  break;
	case  250: ei = ExpIdx_250;  break;
	case  320: ei = ExpIdx_320;  break;
	case  400: ei = ExpIdx_400;  break;
	case  500: ei = ExpIdx_500;  break;
	case  640: ei = ExpIdx_640;  break;
	case  800: ei = ExpIdx_800;  break;
	case 1000: ei = ExpIdx_1000; break;
	case 1280: ei = ExpIdx_1280; break;
	case 1600: ei = ExpIdx_1600; break;
	default:
		assert (false);
		break;
	}

	return ei;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	TransOpLogC::compute_direct (double x) const
{
	x = std::max (x, _n);
	double         y =
		  (x > _curve._cut  )
		? _curve._c * log10 (_curve._a * x + _curve._b) + _curve._d
		: _curve._e * x + _curve._f;
	y = std::min (y, 1.0);

	return y;
}

double	TransOpLogC::compute_inverse (double x) const
{
	x = std::min (x, 1.0);
	double         y =
		  (x > _curve._cut_i)
		? (pow (10, (x - _curve._d) / _curve._c) - _curve._b) / _curve._a
		: (x - _curve._f) / _curve._e;
	y = std::max (y, _n);

	return y;
}



const double	TransOpLogC::_noise_margin = -8.0 / 65536;

const TransOpLogC::CurveData	TransOpLogC::_vlog =
{
	0.01, 1.0, 0.00873, 0.241514, 0.598206, 5.6, 0.125, 5.6 * 0.01 + 0.125
};

const std::array <TransOpLogC::CurveData, TransOpLogC::ExpIdx_NBR_ELT>	TransOpLogC::_v2_table =
{{
	// cut, a,       b,        c,        d,        e,        f,        e*cut+f
	{ 0.0, 5.061087, 0.089004, 0.269035, 0.391007, 6.332427, 0.108361, 0.108361 },
	{ 0.0, 5.061087, 0.089004, 0.266007, 0.391007, 6.189953, 0.111543, 0.111543 },
	{ 0.0, 5.061087, 0.089004, 0.262978, 0.391007, 6.034414, 0.114725, 0.114725 },
	{ 0.0, 5.061087, 0.089004, 0.259627, 0.391007, 5.844973, 0.118246, 0.118246 },
	{ 0.0, 5.061087, 0.089004, 0.256598, 0.391007, 5.656190, 0.121428, 0.121428 },
	{ 0.0, 5.061087, 0.089004, 0.253569, 0.391007, 5.449261, 0.124610, 0.124610 },
	{ 0.0, 5.061087, 0.089004, 0.250218, 0.391007, 5.198031, 0.128130, 0.128130 },
	{ 0.0, 5.061087, 0.089004, 0.247189, 0.391007, 4.950469, 0.131313, 0.131313 },
	{ 0.0, 5.061087, 0.089004, 0.244161, 0.391007, 4.684112, 0.134495, 0.134495 },
	{ 0.0, 5.061087, 0.089004, 0.240810, 0.391007, 4.369609, 0.138015, 0.138015 },
	{ 0.0, 5.061087, 0.089004, 0.237781, 0.391007, 4.070466, 0.141197, 0.141197 }
}};

const std::array <TransOpLogC::CurveData, TransOpLogC::ExpIdx_NBR_ELT>	TransOpLogC::_v3_table =
{{
	// cut,     a,        b,        c,        d,        e,        f,        e*cut+f
	{ 0.005561, 5.555556, 0.080216, 0.269036, 0.381991, 5.842037, 0.092778, 0.125266 },
	{ 0.006208, 5.555556, 0.076621, 0.266007, 0.382478, 5.776265, 0.092782, 0.128643 },
	{ 0.006871, 5.555556, 0.072941, 0.262978, 0.382966, 5.710494, 0.092786, 0.132021 },
	{ 0.007622, 5.555556, 0.068768, 0.259627, 0.383508, 5.637732, 0.092791, 0.135761 },
	{ 0.008318, 5.555556, 0.064901, 0.256598, 0.383999, 5.571960, 0.092795, 0.139142 },
	{ 0.009031, 5.555556, 0.060939, 0.253569, 0.384493, 5.506188, 0.092800, 0.142526 },
	{ 0.009840, 5.555556, 0.056443, 0.250219, 0.385040, 5.433426, 0.092805, 0.146271 },
	{ 0.010591, 5.555556, 0.052272, 0.247190, 0.385537, 5.367655, 0.092809, 0.149658 },
	{ 0.011361, 5.555556, 0.047996, 0.244161, 0.386036, 5.301883, 0.092814, 0.153047 },
	{ 0.012235, 5.555556, 0.043137, 0.240810, 0.386590, 5.229121, 0.092819, 0.156799 },
	{ 0.013047, 5.555556, 0.038625, 0.237781, 0.387093, 5.163350, 0.092824, 0.160192 }
}};



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
