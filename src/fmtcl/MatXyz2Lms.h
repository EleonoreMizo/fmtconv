/*****************************************************************************

        MatXyz2Lms.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_MatXyz2Lms_HEADER_INCLUDED)
#define fmtcl_MatXyz2Lms_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Mat3.h"



namespace fmtcl
{



class MatXyz2Lms
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	// XYZ to LMS matrices
	// http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
	// https://en.wikipedia.org/wiki/LMS_color_space
	// https://github.com/colour-science/colour/blob/develop/colour/adaptation/datasets/cat.py
	static constexpr Mat3 _vonkries_std {
		Vec3 {  0.38971, 0.68898, -0.07868 },
		Vec3 { -0.22981, 1.18340,  0.04641 },
		Vec3 {  0.0    , 0.0    ,  1.0      }
	};
	// Alternative version, used in IPTPQc2. Differs from the main one by
	// a diagonal-matrix factor (neutral for a chromatic adaptation).
	// Used for the Dolby Vision LMS conversion
	static constexpr Mat3 _vonkries_alt {
		Vec3 {  0.40024, 0.7076 , -0.08081 },
		Vec3 { -0.2263 , 1.16532,  0.0457  },
		Vec3 {  0.0    , 0.0    ,  0.91822 }
	};

	static constexpr Mat3 _stockman_sharpe { Mat3 {
		Vec3 { 1.94735469, -1.41445123, 0.36476327 },
		Vec3 { 0.68990272,  0.34832189, 0.0        },
		Vec3 { 0.0       ,  0.0       , 1.93485343 }
	}.compute_inverse () };

	// Not exact LMS colorspaces ("sharpened LMS")
	static constexpr Mat3 _bradford {
		Vec3 {  0.8951,  0.2664, -0.1614 },
		Vec3 { -0.7502,  1.7135,  0.0367 },
		Vec3 {  0.0389, -0.0685,  1.0296 }
	};
	static constexpr Mat3 _ciecam97s {
		Vec3 {  0.8562,  0.3372, -0.1934 },
		Vec3 { -0.8360,  1.8327,  0.0033 },
		Vec3 {  0.0357, -0.0469,  1.0112 }
	};
	static constexpr Mat3 _ciecam02 {
		Vec3 {  0.7328,  0.4296, -0.1624 },
		Vec3 { -0.7036,  1.6975,  0.0061 },
		Vec3 {  0.0030, -0.0136,  0.9834 }
	};
	static constexpr Mat3 _cam16 {
		Vec3 {  0.401288, 0.650173, -0.051461 },
		Vec3 { -0.250268, 1.204414,  0.045854 },
		Vec3 { -0.002079, 0.048952,  0.953127 }
	};



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               MatXyz2Lms ()                               = delete;
	               MatXyz2Lms (const MatXyz2Lms &other)        = delete;
	               MatXyz2Lms (MatXyz2Lms &&other)             = delete;
	               ~MatXyz2Lms ()                              = delete;
	MatXyz2Lms &   operator = (const MatXyz2Lms &other)        = delete;
	MatXyz2Lms &   operator = (MatXyz2Lms &&other)             = delete;
	bool           operator == (const MatXyz2Lms &other) const = delete;
	bool           operator != (const MatXyz2Lms &other) const = delete;

}; // class MatXyz2Lms



}  // namespace fmtcl



//#include "fmtcl/MatXyz2Lms.hpp"



#endif   // fmtcl_MatXyz2Lms_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
