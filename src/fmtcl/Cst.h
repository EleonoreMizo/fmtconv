/*****************************************************************************

        Cst.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Cst_HEADER_INCLUDED)
#define fmtcl_Cst_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



class Cst
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _max_nbr_planes = 4; // RGB or YUV + alpha

	// TV ranges in 8-bit scale
	static constexpr int _rtv_imin       = 1;   // Minimum legal val, inclusive
	static constexpr int _rtv_emax       = 255; // Maximum legal val, exclusive
	static constexpr int _rtv_lum_blk    = 16;  // Black level
	static constexpr int _rtv_lum_wht    = 235; // White level
	static constexpr int _rtv_chr_gry    = 128; // Chroma neutral (grey) value
	static constexpr int _rtv_chr_dep    = _rtv_chr_gry - 16; // Chroma depth



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Cst ()                               = delete;
	               Cst (const Cst &other)               = delete;
	               Cst (Cst &&other)                    = delete;
	Cst &          operator = (const Cst &other)        = delete;
	Cst &          operator = (Cst &&other)             = delete;
	bool           operator == (const Cst &other) const = delete;
	bool           operator != (const Cst &other) const = delete;

}; // class Cst



}  // namespace fmtcl



//#include "fmtcl/Cst.hpp"



#endif   // fmtcl_Cst_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
