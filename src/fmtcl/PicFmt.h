/*****************************************************************************

        PicFmt.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_PicFmt_HEADER_INCLUDED)
#define fmtcl_PicFmt_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ColorFamily.h"
#include "fmtcl/SplFmt.h"



namespace fmtcl
{



class PicFmt
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	bool           is_valid () const noexcept
	{
		return (
			   _sf >= 0 && _sf < SplFmt_NBR_ELT
			&& _res >= 8
			&& _col_fam >= 0 && _col_fam < ColorFamily_NBR_ELT
		);
	}

	SplFmt         _sf        = SplFmt_ILLEGAL;
	int            _res       = 0;      // Number of bits per sample
	ColorFamily    _col_fam   = ColorFamily_INVALID;
	bool           _full_flag = false;  // Full range



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

}; // class PicFmt



}  // namespace fmtcl



//#include "fmtcl/PicFmt.hpp"



#endif   // fmtcl_PicFmt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
