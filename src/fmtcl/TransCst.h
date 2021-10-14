/*****************************************************************************

        TransCst.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransCst_HEADER_INCLUDED)
#define fmtcl_TransCst_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



class TransCst
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr double _srgb_lw       = 80;    // cd/m^2
	static constexpr double _srgb_gamma    = 2.4;
	static constexpr double _srgb_alpha    = 1.055;
	static constexpr double _srgb_power    = 1 / _srgb_gamma;
	static const     double _srgb_slope;
	static const     double _srgb_beta;

	static constexpr double _iec61966_2_1_lw    = _srgb_lw;
	static constexpr double _iec61966_2_1_gamma = _srgb_gamma;
	static constexpr double _iec61966_2_1_alpha = _srgb_alpha;
	static constexpr double _iec61966_2_1_power = 1 / _iec61966_2_1_gamma;
	static constexpr double _iec61966_2_1_slope = 12.92;
	static constexpr double _iec61966_2_1_beta  = 0.04045 / _iec61966_2_1_slope;

	static constexpr double _bt709_beta    = 0.018;
	static constexpr double _bt709_alpha   = 1 + 5.5 * _bt709_beta; // 1.099
	static constexpr double _bt709_power   = 0.45;
	static constexpr double _bt709_slope   = 4.5;
	static constexpr double _bt709_gamma   = 1 / _bt709_power;

	static constexpr double _bt1886_gamma  = 2.4;
	static constexpr double _bt1886_lw     = 100;   // cd/m^2

	static constexpr double _bt1886_alpha1 = 2.6;
	static constexpr double _bt1886_alpha2 = 3.0;
	static constexpr double _bt1886_vc     = 0.35;

	// Refinement over the BT-709 curve for 12-bit data
	static constexpr double _bt2020_beta   = 0.018053968510807;
	static constexpr double _bt2020_alpha  = 1 + 5.5 * _bt2020_beta; // 1.09929682680944
	static constexpr double _bt2020_power  = _bt709_power;
	static constexpr double _bt2020_slope  = _bt709_slope;
	static constexpr double _bt2020_gamma  = 1 / _bt2020_power;

	static constexpr double _bt2100_pq_lw  = 10000; // cd/m^2



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransCst ()                               = delete;
	               TransCst (const TransCst &other)          = delete;
	               TransCst (TransCst &&other)               = delete;
	TransCst &     operator = (const TransCst &other)        = delete;
	TransCst &     operator = (TransCst &&other)             = delete;
	bool           operator == (const TransCst &other) const = delete;
	bool           operator != (const TransCst &other) const = delete;

}; // class TransCst



}  // namespace fmtcl



//#include "fmtcl/TransCst.hpp"



#endif   // fmtcl_TransCst_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
