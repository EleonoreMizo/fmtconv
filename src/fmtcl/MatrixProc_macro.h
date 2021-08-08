/*****************************************************************************

        MatrixProc_macro.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_MatrixProc_macro_HEADER_INCLUDED)
#define	fmtcl_MatrixProc_macro_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



// Dst fmt, dst bits, src fmt, src bits
#define fmtcl_MatrixProc_SPAN_I(CI) \
	CI (INT8   ,  8, INT8   ,  8) \
	                              \
	CI (INT16  ,  9, INT8   ,  8) \
	CI (INT16  ,  9, INT16  ,  9) \
	                              \
	CI (INT16  , 10, INT8   ,  8) \
	CI (INT16  , 10, INT16  ,  9) \
	CI (INT16  , 10, INT16  , 10) \
	                              \
	CI (INT16  , 11, INT8   ,  8) \
	CI (INT16  , 11, INT16  ,  9) \
	CI (INT16  , 11, INT16  , 10) \
	CI (INT16  , 11, INT16  , 11) \
	                              \
	CI (INT16  , 12, INT8   ,  8) \
	CI (INT16  , 12, INT16  ,  9) \
	CI (INT16  , 12, INT16  , 10) \
	CI (INT16  , 12, INT16  , 11) \
	CI (INT16  , 12, INT16  , 12) \
	                              \
	CI (INT16  , 14, INT8   ,  8) \
	CI (INT16  , 14, INT16  ,  9) \
	CI (INT16  , 14, INT16  , 10) \
	CI (INT16  , 14, INT16  , 11) \
	CI (INT16  , 14, INT16  , 12) \
	CI (INT16  , 14, INT16  , 14) \
	                              \
	CI (INT16  , 16, INT8   ,  8) \
	CI (INT16  , 16, INT16  ,  9) \
	CI (INT16  , 16, INT16  , 10) \
	CI (INT16  , 16, INT16  , 11) \
	CI (INT16  , 16, INT16  , 12) \
	CI (INT16  , 16, INT16  , 16) \
	CI (INT16  , 16, STACK16, 16) \
	                              \
	CI (STACK16, 16, INT8   ,  8) \
	CI (STACK16, 16, INT16  ,  9) \
	CI (STACK16, 16, INT16  , 10) \
	CI (STACK16, 16, INT16  , 11) \
	CI (STACK16, 16, INT16  , 12) \
	CI (STACK16, 16, INT16  , 16) \
	CI (STACK16, 16, STACK16, 16)



#endif	// fmtcl_MatrixProc_macro_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
