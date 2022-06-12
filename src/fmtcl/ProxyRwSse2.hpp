/*****************************************************************************

        ProxyRwSse2.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ProxyRwSse2_CODEHEADER_INCLUDED)
#define	fmtcl_ProxyRwSse2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fstb/ToolsSse2.h"



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	ProxyRwSse2 <SplFmt_FLOAT>::read_flt (const PtrConst::Type &ptr, __m128 &src0, __m128 &src1, const __m128i &/*zero*/)
{
	src0 = _mm_loadu_ps (ptr    );
	src1 = _mm_loadu_ps (ptr + 4);
}

void	ProxyRwSse2 <SplFmt_FLOAT>::read_flt_partial (const PtrConst::Type &ptr, __m128 &src0, __m128 &src1, const __m128i &/*zero*/, int len)
{
	if (len >= 4)
	{
		src0 = _mm_loadu_ps (ptr    );
		src1 = fstb::ToolsSse2::load_ps_partial (ptr + 4, len - 4);
	}
	else
	{
		src0 = fstb::ToolsSse2::load_ps_partial (ptr    , len    );
		src1 = _mm_setzero_ps ();
	}
}

void	ProxyRwSse2 <SplFmt_FLOAT>::write_flt (const Ptr::Type &ptr, const __m128 &src0, const __m128 &src1, const __m128i &/*mask_lsb*/, const __m128i &/*sign_bit*/, const __m128 &/*offset*/)
{
	_mm_store_ps (ptr,     src0);
	_mm_store_ps (ptr + 4, src1);
}

void	ProxyRwSse2 <SplFmt_FLOAT>::write_flt_partial (const Ptr::Type &ptr, const __m128 &src0, const __m128 &src1, const __m128i &/*mask_lsb*/, const __m128i &/*sign_bit*/, const __m128 &/*offset*/, int len)
{
	if (len >= 4)
	{
		_mm_store_ps (ptr, src0);
		fstb::ToolsSse2::store_ps_partial (ptr + 4, src1, len - 4);
	}
	else
	{
		fstb::ToolsSse2::store_ps_partial (ptr    , src0, len    );
	}
}



// const __m128i	zero = _mm_setzero_si128 ();
void	ProxyRwSse2 <SplFmt_INT8>::read_flt (const PtrConst::Type &ptr, __m128 &src0, __m128 &src1, const __m128i &zero)
{
	const __m128i	src = fstb::ToolsSse2::load_8_16l (ptr, zero);
	ProxyRwSse2 <SplFmt_INT16>::finish_read_flt (src0, src1, src, zero);
}

// const __m128i	zero = _mm_setzero_si128 ();
void	ProxyRwSse2 <SplFmt_INT8>::read_flt_partial (const PtrConst::Type &ptr, __m128 &src0, __m128 &src1, const __m128i &zero, int len)
{
	const __m128i	src = fstb::ToolsSse2::load_8_16l_partial (ptr, zero, len);
	ProxyRwSse2 <SplFmt_INT16>::finish_read_flt (src0, src1, src, zero);
}

__m128i	ProxyRwSse2 <SplFmt_INT8>::read_i16 (const PtrConst::Type &ptr, const __m128i &zero)
{
	return (fstb::ToolsSse2::load_8_16l (ptr, zero));
}

__m128i	ProxyRwSse2 <SplFmt_INT8>::read_i16_partial (const PtrConst::Type &ptr, const __m128i &zero, int len)
{
	return (fstb::ToolsSse2::load_8_16l_partial (ptr, zero, len));
}

//	const __m128i	mask_lsb = _mm_set1_epi16 (0x00FF);
//	const __m128i	sign_bit = _mm_set1_epi16 (-0x8000);
//	const __m128	offset   = _mm_set1_ps (-32768);
void	ProxyRwSse2 <SplFmt_INT8>::write_flt (const Ptr::Type &ptr, const __m128 &src0, const __m128 &src1, const __m128i &mask_lsb, const __m128i &sign_bit, const __m128 &offset)
{
	const __m128i  val = prepare_write_flt (src0, src1, sign_bit, offset);
	fstb::ToolsSse2::store_8_16l (ptr, val, mask_lsb);
}

void	ProxyRwSse2 <SplFmt_INT8>::write_flt_partial (const Ptr::Type &ptr, const __m128 &src0, const __m128 &src1, const __m128i &mask_lsb, const __m128i &sign_bit, const __m128 &offset, int len)
{
	const __m128i  val = prepare_write_flt (src0, src1, sign_bit, offset);
	fstb::ToolsSse2::store_8_16l_partial (ptr, val, mask_lsb, len);
}

__m128i	ProxyRwSse2 <SplFmt_INT8>::prepare_write_flt (const __m128 &src0, const __m128 &src1, const __m128i &sign_bit, const __m128 &offset)
{
	__m128			val_03_f = _mm_add_ps (src0, offset);
	__m128			val_47_f = _mm_add_ps (src1, offset);

	const __m128i	val_03 = _mm_cvtps_epi32 (val_03_f);
	const __m128i	val_47 = _mm_cvtps_epi32 (val_47_f);

	__m128i			val = _mm_packs_epi32 (val_03, val_47);
	val = _mm_xor_si128 (val, sign_bit);

	return (val);
}

void	ProxyRwSse2 <SplFmt_INT8>::write_i16 (const Ptr::Type &ptr, const __m128i &src, const __m128i &mask_lsb)
{
	fstb::ToolsSse2::store_8_16l (ptr, src, mask_lsb);
}

void	ProxyRwSse2 <SplFmt_INT8>::write_i16_partial (const Ptr::Type &ptr, const __m128i &src, const __m128i &mask_lsb, int len)
{
	fstb::ToolsSse2::store_8_16l_partial (ptr, src, mask_lsb, len);
}



// Sign is ignored here
template <bool CLIP_FLAG, bool SIGN_FLAG>
__m128i	ProxyRwSse2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::read (const PtrConst::Type &ptr, const __m128i &zero, const __m128i &sign_bit)
{
	fstb::unused (sign_bit);

	return (fstb::ToolsSse2::load_8_16l (ptr, zero));
}

// Sign is ignored here
template <bool CLIP_FLAG, bool SIGN_FLAG>
__m128i	ProxyRwSse2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::read_partial (const PtrConst::Type &ptr, const __m128i &zero, const __m128i &sign_bit, int len)
{
	fstb::unused (sign_bit);

	return (fstb::ToolsSse2::load_8_16l_partial (ptr, zero, len));
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwSse2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip (const Ptr::Type &ptr, const __m128i &src, const __m128i &mask_lsb, const __m128i &mi, const __m128i &ma, const __m128i &sign_bit)
{
	const __m128i  val =
		ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::prepare_write_clip (
			src, mi, ma, sign_bit
		);
	fstb::ToolsSse2::store_8_16l (ptr, val, mask_lsb);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwSse2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip_partial (const Ptr::Type &ptr, const __m128i &src, const __m128i &mask_lsb, const __m128i &mi, const __m128i &ma, const __m128i &sign_bit, int len)
{
	const __m128i  val =
		ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::prepare_write_clip (
			src, mi, ma, sign_bit
		);
	fstb::ToolsSse2::store_8_16l_partial (ptr, val, mask_lsb, len);
}



// const __m128i	zero = _mm_setzero_si128 ();
void	ProxyRwSse2 <SplFmt_INT16>::read_flt (const PtrConst::Type &ptr, __m128 &src0, __m128 &src1, const __m128i &zero)
{
	const __m128i	src =
		_mm_loadu_si128 (reinterpret_cast <const __m128i *> (ptr));
	finish_read_flt (src0, src1, src, zero);
}

// const __m128i	zero = _mm_setzero_si128 ();
void	ProxyRwSse2 <SplFmt_INT16>::read_flt_partial (const PtrConst::Type &ptr, __m128 &src0, __m128 &src1, const __m128i &zero, int len)
{
	const __m128i	src =
		fstb::ToolsSse2::load_si128_partial (ptr, len * int (sizeof (uint16_t)));
	finish_read_flt (src0, src1, src, zero);
}

void	ProxyRwSse2 <SplFmt_INT16>::finish_read_flt (__m128 &src0, __m128 &src1, const __m128i &src, const __m128i &zero)
{
	const __m128i	src_03 = _mm_unpacklo_epi16 (src, zero);
	const __m128i	src_47 = _mm_unpackhi_epi16 (src, zero);
	src0 = _mm_cvtepi32_ps (src_03);
	src1 = _mm_cvtepi32_ps (src_47);
}

__m128i	ProxyRwSse2 <SplFmt_INT16>::read_i16 (const PtrConst::Type &ptr, const __m128i &/*zero*/)
{
	return (_mm_loadu_si128 (reinterpret_cast <const __m128i *> (ptr)));
}

__m128i	ProxyRwSse2 <SplFmt_INT16>::read_i16_partial (const PtrConst::Type &ptr, const __m128i &/*zero*/, int len)
{
	return (fstb::ToolsSse2::load_si128_partial (ptr, len * int (sizeof (uint16_t))));
}

//	const __m128i	mask_lsb = _mm_set1_epi16 (0x00FF);
//	const __m128i	sign_bit = _mm_set1_epi16 (-0x8000);
//	const __m128	offset   = _mm_set1_ps (-32768);
void	ProxyRwSse2 <SplFmt_INT16>::write_flt (const Ptr::Type &ptr, const __m128 &src0, const __m128 &src1, const __m128i &/*mask_lsb*/, const __m128i &sign_bit, const __m128 &offset)
{
	__m128			val_03_f = _mm_add_ps (src0, offset);
	__m128			val_47_f = _mm_add_ps (src1, offset);

	const __m128i	val_03 = _mm_cvtps_epi32 (val_03_f);
	const __m128i	val_47 = _mm_cvtps_epi32 (val_47_f);

	__m128i			val = _mm_packs_epi32 (val_03, val_47);
	val = _mm_xor_si128 (val, sign_bit);

	_mm_storeu_si128 (reinterpret_cast <__m128i *> (ptr), val);
}

void	ProxyRwSse2 <SplFmt_INT16>::write_flt_partial (const Ptr::Type &ptr, const __m128 &src0, const __m128 &src1, const __m128i &/*mask_lsb*/, const __m128i &sign_bit, const __m128 &offset, int len)
{
	__m128			val_03_f = _mm_add_ps (src0, offset);
	__m128			val_47_f = _mm_add_ps (src1, offset);

	const __m128i	val_03 = _mm_cvtps_epi32 (val_03_f);
	const __m128i	val_47 = _mm_cvtps_epi32 (val_47_f);

	__m128i			val = _mm_packs_epi32 (val_03, val_47);
	val = _mm_xor_si128 (val, sign_bit);

	fstb::ToolsSse2::store_si128_partial (ptr, val, len * int (sizeof (uint16_t)));
}

void	ProxyRwSse2 <SplFmt_INT16>::write_i16 (const Ptr::Type &ptr, const __m128i &src, const __m128i &/*mask_lsb*/)
{
	_mm_storeu_si128 (reinterpret_cast <__m128i *> (ptr), src);
}

void	ProxyRwSse2 <SplFmt_INT16>::write_i16_partial (const Ptr::Type &ptr, const __m128i &src, const __m128i &/*mask_lsb*/, int len)
{
	fstb::ToolsSse2::store_si128_partial (ptr, src, len * int (sizeof (uint16_t)));
}



template <bool CLIP_FLAG, bool SIGN_FLAG>
__m128i	ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::read (const PtrConst::Type &ptr, const __m128i &/*zero*/, const __m128i &sign_bit)
{
	__m128i        val =
		_mm_loadu_si128 (reinterpret_cast <const __m128i *> (ptr));
	if (SIGN_FLAG)
	{
		val = _mm_xor_si128 (val, sign_bit);
	}

	return (val);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
__m128i	ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::read_partial (const PtrConst::Type &ptr, const __m128i &/*zero*/, const __m128i &sign_bit, int len)
{
	__m128i        val =
		fstb::ToolsSse2::load_si128_partial (ptr, len * int (sizeof (uint16_t)));
	if (SIGN_FLAG)
	{
		val = _mm_xor_si128 (val, sign_bit);
	}

	return (val);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip (const Ptr::Type &ptr, const __m128i &src, const __m128i &/*mask_lsb*/, const __m128i &mi, const __m128i &ma, const __m128i &sign_bit)
{
	const __m128i  val = prepare_write_clip (src, mi, ma, sign_bit);
	_mm_storeu_si128 (reinterpret_cast <__m128i *> (ptr), val);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip_partial (const Ptr::Type &ptr, const __m128i &src, const __m128i &/*mask_lsb*/, const __m128i &mi, const __m128i &ma, const __m128i &sign_bit, int len)
{
	const __m128i  val = prepare_write_clip (src, mi, ma, sign_bit);
	fstb::ToolsSse2::store_si128_partial (ptr, val, len * int (sizeof (uint16_t)));
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
__m128i	ProxyRwSse2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::prepare_write_clip (const __m128i &src, const __m128i &mi, const __m128i &ma, const __m128i &sign_bit)
{
	__m128i        val = src;
	if (CLIP_FLAG)
	{
		val = _mm_min_epi16 (val, ma);
		val = _mm_max_epi16 (val, mi);
	}
	if (SIGN_FLAG)
	{
		val = _mm_xor_si128 (val, sign_bit);
	}

	return (val);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_ProxyRwSse2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
