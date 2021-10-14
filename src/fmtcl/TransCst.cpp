/*****************************************************************************

        TransCst.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransCst.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr double	TransCst::_srgb_lw;
constexpr double	TransCst::_srgb_gamma;
constexpr double	TransCst::_srgb_alpha;
constexpr double	TransCst::_srgb_power;

// More accurate formula giving C1 continuity
// https://en.wikipedia.org/wiki/SRGB#Theory_of_the_transformation
const     double	TransCst::_srgb_slope =
	  (  pow (_srgb_alpha    , _srgb_gamma    )
	   * pow (_srgb_gamma - 1, _srgb_gamma - 1))
	/ (  pow (_srgb_alpha - 1, _srgb_gamma - 1)
	   * pow (_srgb_gamma    , _srgb_gamma    ));
const     double	TransCst::_srgb_beta  =
	(_srgb_alpha - 1) / ((_srgb_gamma - 1) * _srgb_slope);

constexpr double	TransCst::_iec61966_2_1_lw;
constexpr double	TransCst::_iec61966_2_1_gamma;
constexpr double	TransCst::_iec61966_2_1_alpha;
constexpr double	TransCst::_iec61966_2_1_power;
constexpr double	TransCst::_iec61966_2_1_slope;
constexpr double	TransCst::_iec61966_2_1_beta;

constexpr double	TransCst::_bt709_beta;
constexpr double	TransCst::_bt709_alpha;
constexpr double	TransCst::_bt709_power;
constexpr double	TransCst::_bt709_slope;
constexpr double	TransCst::_bt709_gamma;

constexpr double	TransCst::_bt1886_gamma;
constexpr double	TransCst::_bt1886_lw;
constexpr double	TransCst::_bt1886_alpha1;
constexpr double	TransCst::_bt1886_alpha2;
constexpr double	TransCst::_bt1886_vc;

constexpr double	TransCst::_bt2020_beta;
constexpr double	TransCst::_bt2020_alpha;
constexpr double	TransCst::_bt2020_power;
constexpr double	TransCst::_bt2020_slope;
constexpr double	TransCst::_bt2020_gamma;

constexpr double	TransCst::_bt2100_pq_lw;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
