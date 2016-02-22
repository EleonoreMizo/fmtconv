/*****************************************************************************

        ConvStep.h
        Author: Laurent de Soras, 2014

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtc_ConvStep_HEADER_INCLUDED)
#define	fmtc_ConvStep_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/TransCurve.h"



namespace fmtc
{



class ConvStep
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum Range
	{
		Range_UNDEF = -1,

		Range_TV = 0,
		Range_FULL,

		Range_NBR_ELT
	};

	               ConvStep ()                        = default;
	               ConvStep (const ConvStep &other)   = default;
	virtual        ~ConvStep ()                       = default;
	ConvStep &     operator = (const ConvStep &other) = default;

	int            _col_fam      = -1;  // ::VSColorFamily, negative if unspecified
	Range          _range        = Range_UNDEF;        // Data range, negative if unspecified
	int            _css_h        = -1;  // Log2 of the chroma subsampling, negative if unspecified
	int            _css_v        = -1;  // Same, vertically
	fmtcl::ChromaPlacement              // Chroma placement, negative if unspecified
	               _cplace       = fmtcl::ChromaPlacement_UNDEF;
	fmtcl::TransCurve                   // Transfer curve, negative if unspecified
	                _tcurve      = fmtcl::TransCurve_UNDEF;
	double         _gammac       = -1;  // Additional gamma correction, negative if unspecified

	bool           _resized_flag = false;  // Indicates if all the planes of the frame are now resized to their final resolution

	int            _sample_type  = -1;  // ::VSSampleType (int or float), negative if unspecified
	int            _bitdepth     = -1;  // Pixel bitdepth, negative if unspecified



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const ConvStep &other) const = delete;
	bool           operator != (const ConvStep &other) const = delete;

};	// class ConvStep



}	// namespace fmtc



//#include "fmtc/ConvStep.hpp"



#endif	// fmtc_ConvStep_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
