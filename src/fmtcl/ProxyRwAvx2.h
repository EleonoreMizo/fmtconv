/*****************************************************************************

        ProxyRwAvx2.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ProxyRwAvx2_HEADER_INCLUDED)
#define	fmtcl_ProxyRwAvx2_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/Proxy.h"
#include "fmtcl/SplFmt.h"

#include <immintrin.h>

#include <cstdint>



namespace fmtcl
{



template <SplFmt PT> class ProxyRwAvx2 {};



template <>
class ProxyRwAvx2 <SplFmt_FLOAT>
{
public:
	typedef	Proxy::PtrFloat          Ptr;
	typedef	Proxy::PtrFloatConst     PtrConst;
	enum {         ALIGN_R =  4 };
	enum {         ALIGN_W = 16 };
	enum {         OFFSET  = 0  };
	static fstb_FORCEINLINE void
	               read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &zero);
	static fstb_FORCEINLINE void
	               read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &zero, int len);
	static fstb_FORCEINLINE void
	               write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset);
	static fstb_FORCEINLINE void
	               write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset, int len);
};

template <>
class ProxyRwAvx2 <SplFmt_INT8>
{
public:
	typedef	Proxy::PtrInt8           Ptr;
	typedef	Proxy::PtrInt8Const      PtrConst;
	enum {         ALIGN_R =  1 };
	enum {         ALIGN_W =  1 };
	enum {         OFFSET  = -32768 };
	static fstb_FORCEINLINE void
	               read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/);
	static fstb_FORCEINLINE void
	               read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/, int len);
	static fstb_FORCEINLINE __m256i
	               read_i16 (const PtrConst::Type &ptr, const __m256i &/*zero*/);
	static fstb_FORCEINLINE __m256i
	               read_i16_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, int len);
	static fstb_FORCEINLINE void
	               write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset);
	static fstb_FORCEINLINE void
	               write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset, int len);
	static fstb_FORCEINLINE void
	               write_i16 (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb);
	static fstb_FORCEINLINE void
	               write_i16_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, int len);

	template <bool CLIP_FLAG, bool SIGN_FLAG>
	class S16
	{
	public:
		static fstb_FORCEINLINE __m256i
		               read (const PtrConst::Type &ptr, const __m256i &zero, const __m256i &sign_bit);
		static fstb_FORCEINLINE __m256i
		               read_partial (const PtrConst::Type &ptr, const __m256i &zero, const __m256i &sign_bit, int len);
		static fstb_FORCEINLINE void
		               write_clip (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit);
		static fstb_FORCEINLINE void
		               write_clip_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit, int len);
	};
private:
	static fstb_FORCEINLINE void
	               finish_read_flt (__m256 &src0, __m256 &src1, const __m128i &src128);
	static fstb_FORCEINLINE __m256i
	               prepare_write_flt (const __m256 &src0, const __m256 &src1, const __m256i &sign_bit, const __m256 &offset);
};

template <>
class ProxyRwAvx2 <SplFmt_INT16>
{
public:
	typedef	Proxy::PtrInt16          Ptr;
	typedef	Proxy::PtrInt16Const     PtrConst;
	enum {         ALIGN_R =  2 };
	enum {         ALIGN_W =  2 };
	enum {         OFFSET  = -32768 };
	static fstb_FORCEINLINE void
	               read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/);
	static fstb_FORCEINLINE void
	               read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/, int len);
	static fstb_FORCEINLINE __m256i
	               read_i16 (const PtrConst::Type &ptr, const __m256i &/*zero*/);
	static fstb_FORCEINLINE __m256i
	               read_i16_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, int len);
	static fstb_FORCEINLINE void
	               write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset);
	static fstb_FORCEINLINE void
	               write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset, int len);
	static fstb_FORCEINLINE void
	               write_i16 (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/);
	static fstb_FORCEINLINE void
	               write_i16_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, int len);

	static fstb_FORCEINLINE void
	               finish_read_flt (__m256 &src0, __m256 &src1, const __m256i &src);

	template <bool CLIP_FLAG, bool SIGN_FLAG>
	class S16
	{
	public:
		static fstb_FORCEINLINE __m256i
		               read (const PtrConst::Type &ptr, const __m256i &/*zero*/, const __m256i &sign_bit);
		static fstb_FORCEINLINE __m256i
		               read_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, const __m256i &sign_bit, int len);
		static fstb_FORCEINLINE void
		               write_clip (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit);
		static fstb_FORCEINLINE void
		               write_clip_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit, int len);

		static fstb_FORCEINLINE __m256i
		               prepare_write_clip (const __m256i &src, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit);
	};
private:
	static fstb_FORCEINLINE __m256i
	               prepare_write_flt (const __m256 &src0, const __m256 &src1, const __m256i &sign_bit, const __m256 &offset);
};

template <>
class ProxyRwAvx2 <SplFmt_STACK16>
{
public:
	typedef	Proxy::PtrStack16        Ptr;
	typedef	Proxy::PtrStack16Const   PtrConst;
	enum {         ALIGN_R =  1 };
	enum {         ALIGN_W =  1 };
	enum {         OFFSET  = -32768 };
	static fstb_FORCEINLINE void
						read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &zero);
	static fstb_FORCEINLINE void
						read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &zero, int len);
	static fstb_FORCEINLINE __m256i
	               read_i16 (const PtrConst::Type &ptr, const __m256i &/*zero*/);
	static fstb_FORCEINLINE __m256i
	               read_i16_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, int len);
	static fstb_FORCEINLINE void
						write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset);
	static fstb_FORCEINLINE void
						write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset, int len);
	static fstb_FORCEINLINE void
	               write_i16 (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb);
	static fstb_FORCEINLINE void
	               write_i16_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, int len);

	template <bool CLIP_FLAG, bool SIGN_FLAG>
	class S16
	{
	public:
		static fstb_FORCEINLINE __m256i
		               read (const PtrConst::Type &ptr, const __m256i &/*zero*/, const __m256i &sign_bit);
		static fstb_FORCEINLINE __m256i
		               read_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, const __m256i &sign_bit, int len);
		static fstb_FORCEINLINE void
		               write_clip (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit);
		static fstb_FORCEINLINE void
		               write_clip_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit, int len);
	};
private:
	static fstb_FORCEINLINE __m256i
	               prepare_write_flt (const __m256 &src0, const __m256 &src1, const __m256i &sign_bit, const __m256 &offset);
};



}	// namespace fmtcl



#include "fmtcl/ProxyRwAvx2.hpp"



#endif	// fmtcl_ProxyRwAvx2_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
