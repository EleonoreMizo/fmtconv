/*****************************************************************************

        ConvStep.cpp
        Author: Laurent de Soras, 2014

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

#include "fmtc/ConvStep.h"

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ConvStep::ConvStep ()
:	_col_fam (-1)
,	_range (Range_UNDEF)
,	_css_h (-1)
,	_css_v (-1)
,	_cplace (fmtcl::ChromaPlacement_UNDEF)
,	_tcurve (fmtcl::TransCurve_UNDEF)
,	_gammac (-1)
,	_resized_flag (false)
,	_sample_type (-1)
,	_bitdepth (-1)
{
	// Nothing
}



ConvStep::ConvStep (const ConvStep &other)
:	_col_fam (other._col_fam)
,	_range (other._range)
,	_css_h (other._css_h)
,	_css_v (other._css_v)
,	_cplace (other._cplace)
,	_tcurve (other._tcurve)
,	_gammac (other._gammac)
,	_resized_flag (other._resized_flag)
,	_sample_type (other._sample_type)
,	_bitdepth (other._bitdepth)
{
	assert (&other != 0);
}



ConvStep &	ConvStep::operator = (const ConvStep &other)
{
	assert (&other != 0);

	if (this != &other)
	{
		_col_fam      = other._col_fam;
		_range        = other._range;
		_css_h        = other._css_h;
		_css_v        = other._css_v;
		_cplace       = other._cplace;
		_tcurve       = other._tcurve;
		_gammac       = other._gammac;
		_resized_flag = other._resized_flag;
		_sample_type  = other._sample_type;
		_bitdepth     = other._bitdepth;
	}

	return (*this);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
