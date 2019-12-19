/*****************************************************************************

        ContFirSpline.cpp
        Author: Laurent de Soras, 2012

Natural cubic splines
Coefficient calculation taken from SplineResize 0.2 by Wilbert Dijkhof

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

#include "fmtcl/ContFirSpline.h"

#include <vector>

#include <cmath>
#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ContFirSpline::ContFirSpline (int taps)
:	_taps (taps)
/*,	_coef ()*/
{
	assert (taps >= 1);
	assert (taps <= 128);

	// a(j) = a(2*taps-j) (this holds for b(j), c(j) and d(j) as well)
	// c2 = [taps, a(taps+1), b(taps+1), c(taps+1), d(taps+1),
	//       a(taps+2), b(taps+2), c(taps+2), d(taps+2),
	//       ......................
	//       a(2*taps), b(2*taps), c(2*taps), d(2*taps)]
	//
  	// Lz = f, z = Ux, n = 2*taps+1
	// z = [z[0]; ...; z[n-1]]
	// f = [f[0]; ...; f[n-1]]
	// x = [x[1]; ...; x[n]] (that's not consistent, but i need that x[0]=x[n+1]=0)
	// LU = [4 1 0 0 ... ]
	//	     [1 4 1 0 ... ]
	//	     [0 1 4 1 ... ]
	//	        ...
	//	     [0 ...  1 4 1]
	//	     [0 ...  0 1 4]

	std::vector <double> y (2 * taps + 1, 0.0);
	std::vector <double> f (2 * taps,     0.0);
	std::vector <double> w (2 * taps);
	std::vector <double> z (2 * taps);
	std::vector <double> x (2 * taps + 1, 0.0);

	y [taps] = 1.0;

	if (taps > 1)
	{
		f [taps - 2] = 6.0;
		f [taps    ] = 6.0;
	}
	f [taps - 1] = -12.0;

	w [0] = 4.0;
	z [0] = f [0] / w [0];
	for (int j = 1; j < 2 * taps; ++j)
	{
		w [j] = 4.0         - 1.0 / w [j-1];
		z [j] = (f [j] - z [j-1]) / w [j  ];
	}

	x [0       ] = 0;
	x [2 * taps] = 0;

	for (int j = 2 * taps - 1; j > 0; --j)
	{
		x [j] = z [j-1] - x [j+1] / w [j-1];
	}

	// j=1:2*taps
	// a(j) = (x(j) - x(j-1))/6
	// b(j) = x(j-1)/2
	// c(j) = [y(j) - y(j-1)] - [x(j) + 2*x(j-1)]/6
	// d(j) = y(j-1)
	// the values (a,b,c,d) for j is the same as for 2*taps-j

	_coef [0] = taps;

	for (int j = taps; j < 2 * taps; ++j)
	{
		const int      p = 4 * (j - taps);
		_coef [p + 1] =   (x [j+1]     - x [j]) / 6.0; // a(j)
		_coef [p + 2] =                  x [j]  / 2.0; // b(j)
		_coef [p + 3] =   (y [j+1]     - y [j])        // c(j)
		                - (x [j+1] + 2 * x [j]) / 6.0;
		_coef [p + 4] =                  y [j];        // d(j)
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double	ContFirSpline::do_get_support () const
{
	return (_taps);
}



double	ContFirSpline::do_get_val (double x) const
{
	double         v = 0;

	x = fabs (x);
	const int      p = int (x);
	if (p < _taps)
	{
		const double   r = x - p;
		v = ((
			  _coef [4*p+1]  * r
			+ _coef [4*p+2]) * r
			+ _coef [4*p+3]) * r
			+ _coef [4*p+4];
	}

	return (v);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
