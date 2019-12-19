/*****************************************************************************

        ScalerCopy.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ScalerCopy_HEADER_INCLUDED)
#define	fmtcl_ScalerCopy_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"



namespace fmtcl
{



template <class DST, int DB, class SRC, int SB>
class ScalerCopy
{
public:
	static fstb_FORCEINLINE bool can_copy (bool copy_flag)
	{
		fstb::unused (copy_flag);
		return (false);
	}
	static fstb_FORCEINLINE void copy (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int width)
	{
		fstb::unused (dst_ptr, src_ptr, width);
		assert (false);
	}
};

template <class DST, int DB>
class ScalerCopy <DST, DB, DST, DB>
{
public:
	static fstb_FORCEINLINE bool can_copy (bool copy_flag)
	{
		return (copy_flag);
	}
	static fstb_FORCEINLINE void copy (typename DST::Ptr::Type dst_ptr, typename DST::PtrConst::Type src_ptr, int width)
	{
		DST::Ptr::copy (dst_ptr, src_ptr, width);
	}
};



}	// namespace fmtcl



//#include "fmtcl/ScalerCopy.hpp"



#endif	// fmtcl_ScalerCopy_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
