/*****************************************************************************

        ReadWrapperFlt.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ReadWrapperFlt_HEADER_INCLUDED)
#define	fmtcl_ReadWrapperFlt_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



template <class SRC, bool PF>
class ReadWrapperFlt
{
public:

	template <class VI, class VF>
	static fstb_FORCEINLINE void
	               read (typename SRC::PtrConst::Type ptr, VF &src0, VF &src1, const VI &zero, int /*len*/);

};	// class ReadWrapperFlt

template <class SRC>
class ReadWrapperFlt <SRC, true>
{
public:

	template <class VI, class VF>
	static fstb_FORCEINLINE void
	               read (typename SRC::PtrConst::Type ptr, VF &src0, VF &src1, const VI &zero, int len);
};



}	// namespace fmtcl



#include "fmtcl/ReadWrapperFlt.hpp"



#endif	// fmtcl_ReadWrapperFlt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
