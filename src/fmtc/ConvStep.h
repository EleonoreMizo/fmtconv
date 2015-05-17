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

	               ConvStep ();
	               ConvStep (const ConvStep &other);
	virtual        ~ConvStep () {}

	ConvStep &     operator = (const ConvStep &other);

	int            _col_fam;      // ::VSColorFamily, negative if unspecified
	Range          _range;        // Data range, negative if unspecified
	int            _css_h;        // Log2 of the chroma subsampling, negative if unspecified
	int            _css_v;        // Same, vertically
	fmtcl::ChromaPlacement        // Chroma placement, negative if unspecified
	               _cplace;
	fmtcl::TransCurve
	                _tcurve;      // Transfer curve, negative if unspecified
	double         _gammac;       // Additional gamma correction, negative if unspecified

	bool           _resized_flag; // Indicates if all the planes of the frame are now resized to their final resolution

	int            _sample_type;  // ::VSSampleType (int or float), negative if unspecified
	int            _bitdepth;     // Pixel bitdepth, negative if unspecified



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const ConvStep &other) const;
	bool           operator != (const ConvStep &other) const;

};	// class ConvStep



}	// namespace fmtc



//#include "fmtc/ConvStep.hpp"



#endif	// fmtc_ConvStep_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
