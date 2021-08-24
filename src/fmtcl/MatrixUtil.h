/*****************************************************************************

        MatrixUtil.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_MatrixUtil_HEADER_INCLUDED)
#define fmtcl_MatrixUtil_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ColorFamily.h"
#include "fmtcl/ColorSpaceH265.h"

#include <string>



namespace fmtcl
{



class Mat4;

class MatrixUtil
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static void    select_def_mat (std::string &mat, ColorFamily col_fam);
	static ColorSpaceH265
	               find_cs_from_mat_str (const std::string &mat, bool allow_2020cl_flag);
	static ColorFamily
	               find_cf_from_cs (ColorSpaceH265 cs, bool ycgco_flag);

	static int     make_mat_from_str (Mat4 &m, const std::string &mat, bool to_rgb_flag);
	static void    make_mat_yuv (Mat4 &m, double kr, double kg, double kb, bool to_rgb_flag);
	static void    make_mat_ycgco (Mat4 &m, bool to_rgb_flag);
	static void    make_mat_ydzdx (Mat4 &m, bool to_rgb_flag);
	static void    make_mat_lms (Mat4 &m, bool to_rgb_flag);
	static void    make_mat_ictcp (Mat4 &m, bool hlg_flag, bool to_lms_flag);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               MatrixUtil ()                               = delete;
	               MatrixUtil (const MatrixUtil &other)        = delete;
	               MatrixUtil (MatrixUtil &&other)             = delete;
	MatrixUtil &   operator = (const MatrixUtil &other)        = delete;
	MatrixUtil &   operator = (MatrixUtil &&other)             = delete;
	bool           operator == (const MatrixUtil &other) const = delete;
	bool           operator != (const MatrixUtil &other) const = delete;

}; // class MatrixUtil



}  // namespace fmtcl



//#include "fmtcl/MatrixUtil.hpp"



#endif   // fmtcl_MatrixUtil_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
