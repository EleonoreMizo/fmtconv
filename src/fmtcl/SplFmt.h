/*****************************************************************************

        SplFmt.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_SplFmt_HEADER_INCLUDED)
#define	fmtcl_SplFmt_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



enum SplFmt
{
	SplFmt_ILLEGAL = -1,

	SplFmt_FLOAT = 0,
	SplFmt_INT16,
	SplFmt_STACK16,
	SplFmt_INT8,

	SplFmt_NBR_ELT

};	// enum SplFmt



inline bool SplFmt_is_float (SplFmt fmt);
inline bool SplFmt_is_int (SplFmt fmt);
inline int	SplFmt_get_unit_size (SplFmt fmt);
inline int	SplFmt_get_data_size (SplFmt fmt);



}	// namespace fmtcl



#include "fmtcl/SplFmt.hpp"



#endif	// fmtcl_SplFmt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
