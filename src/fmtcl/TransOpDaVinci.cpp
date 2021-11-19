/*****************************************************************************

        TransOpDaVinci.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpDaVinci.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransOpDaVinci::TransOpDaVinci (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



double   TransOpDaVinci::do_convert (double x) const
{
   double         y = x;

   if (_inv_flag)
   {
      if (x <= _cut_log)
      {
         y = x / _m;
      }
      else
      {
         y = pow (2, x / _c - _b) - _a;
      }
   }
   else
   {
      if (x <= _cut_lin)
      {
         y = x * _m;
      }
      else
      {
         y = _c * (log2 (x + _a) + _b);
      }
   }

   return y;
}



TransOpInterface::LinInfo  TransOpDaVinci::do_get_info () const
{
	return { Type::OETF, Range::UNDEF, 100.0, 1.0, 100.0, 10000.0 };
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr double  TransOpDaVinci::_a;
constexpr double  TransOpDaVinci::_b;
constexpr double  TransOpDaVinci::_c;
constexpr double  TransOpDaVinci::_m;
constexpr double  TransOpDaVinci::_cut_lin;
constexpr double  TransOpDaVinci::_cut_log;



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
