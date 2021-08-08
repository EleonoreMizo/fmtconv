/*****************************************************************************

        SplFmt.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_SplFmt_CODEHEADER_INCLUDED)
#define	fmtcl_SplFmt_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



bool	SplFmt_is_float (SplFmt fmt)
{
	assert (fmt >= 0);
	assert (fmt < SplFmt_NBR_ELT);

	return (fmt == SplFmt_FLOAT);
}



bool	SplFmt_is_int (SplFmt fmt)
{
	assert (fmt >= 0);
	assert (fmt < SplFmt_NBR_ELT);

	return (fmt != SplFmt_FLOAT);
}



int	SplFmt_get_unit_size (SplFmt fmt)
{
	assert (fmt >= 0);
	assert (fmt < SplFmt_NBR_ELT);

	static const int  size_arr [SplFmt_NBR_ELT] = { 4, 2, 1, 1 };
	assert (size_arr [SplFmt_NBR_ELT - 1] > 0);

	return (size_arr [fmt]);
}

int	SplFmt_get_data_size (SplFmt fmt)
{
	assert (fmt >= 0);
	assert (fmt < SplFmt_NBR_ELT);

	static const int  size_arr [SplFmt_NBR_ELT] = { 4, 2, 2, 1 };
	assert (size_arr [SplFmt_NBR_ELT - 1] > 0);

	return (size_arr [fmt]);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_SplFmt_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
