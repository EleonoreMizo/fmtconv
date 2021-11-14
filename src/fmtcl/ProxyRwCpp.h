/*****************************************************************************

        ProxyRwCpp.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ProxyRwCpp_HEADER_INCLUDED)
#define	fmtcl_ProxyRwCpp_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/Proxy.h"
#include "fmtcl/SplFmt.h"

#include <cstdint>



namespace fmtcl
{



template <SplFmt PT> class ProxyRwCpp {};



template <>
class ProxyRwCpp <SplFmt_FLOAT>
{
public:
	typedef	Proxy::PtrFloat          Ptr;
	typedef	Proxy::PtrFloatConst     PtrConst;
	enum {         ALIGN_R =  1 };
	enum {         ALIGN_W =  1 };
	static fstb_FORCEINLINE float
	               read (const PtrConst::Type &ptr);
	static fstb_FORCEINLINE void
	               write (const Ptr::Type &ptr, float src);
	static fstb_FORCEINLINE void
	               read (const PtrConst::Type &ptr, float &src0, float &src1);
	static fstb_FORCEINLINE void
	               write (const Ptr::Type &ptr, const float &src0, const float &src1);
};

template <>
class ProxyRwCpp <SplFmt_INT8>
{
public:
	typedef	Proxy::PtrInt8           Ptr;
	typedef	Proxy::PtrInt8Const      PtrConst;
	enum {         ALIGN_R =  1 };
	enum {         ALIGN_W =  1 };
	static fstb_FORCEINLINE int
	               read (const PtrConst::Type &ptr);
	template <int C>
	static fstb_FORCEINLINE void
	               write_clip (const Ptr::Type &ptr, int src);
	static fstb_FORCEINLINE void
	               write_no_clip (const Ptr::Type &ptr, int src);
	static fstb_FORCEINLINE void
	               read (const PtrConst::Type &ptr, float &src0, float &src1);
	static fstb_FORCEINLINE void
	               write (const Ptr::Type &ptr, const float &src0, const float &src1);
};

template <>
class ProxyRwCpp <SplFmt_INT16>
{
public:
	typedef	Proxy::PtrInt16          Ptr;
	typedef	Proxy::PtrInt16Const     PtrConst;
	enum {         ALIGN_R =  1 };
	enum {         ALIGN_W =  1 };
	static fstb_FORCEINLINE int
	               read (const PtrConst::Type &ptr);
	template <int C>
	static fstb_FORCEINLINE void
	               write_clip (const Ptr::Type &ptr, int src);
	static fstb_FORCEINLINE void
	               write_no_clip (const Ptr::Type &ptr, int src);
	static fstb_FORCEINLINE void
	               read (const PtrConst::Type &ptr, float &src0, float &src1);
	static fstb_FORCEINLINE void
	               write (const Ptr::Type &ptr, const float &src0, const float &src1);
};



}	// namespace fmtcl



#include "fmtcl/ProxyRwCpp.hpp"



#endif	// fmtcl_ProxyRwCpp_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
