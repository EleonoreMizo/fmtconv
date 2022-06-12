/*****************************************************************************

        ProxyRwAvx2.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ProxyRwAvx2_CODEHEADER_INCLUDED)
#define	fmtcl_ProxyRwAvx2_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fstb/ToolsAvx2.h"



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	ProxyRwAvx2 <SplFmt_FLOAT>::read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/)
{
	src0 = _mm256_loadu_ps (ptr    );
	src1 = _mm256_loadu_ps (ptr + 8);
}

void	ProxyRwAvx2 <SplFmt_FLOAT>::read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/, int len)
{
	if (len >= 8)
	{
		src0 = _mm256_loadu_ps (ptr    );
		src1 = fstb::ToolsAvx2::load_ps_partial (ptr + 8, len - 8);
	}
	else
	{
		src0 = fstb::ToolsAvx2::load_ps_partial (ptr    , len    );
		src1 = _mm256_setzero_ps ();
	}
}

void	ProxyRwAvx2 <SplFmt_FLOAT>::write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &/*mask_lsb*/, const __m256i &/*sign_bit*/, const __m256 &/*offset*/)
{
	_mm256_store_ps (ptr,     src0);
	_mm256_store_ps (ptr + 8, src1);
}

void	ProxyRwAvx2 <SplFmt_FLOAT>::write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &/*mask_lsb*/, const __m256i &/*sign_bit*/, const __m256 &/*offset*/, int len)
{
	if (len >= 8)
	{
		_mm256_store_ps (ptr, src0);
		fstb::ToolsAvx2::store_ps_partial (ptr + 8, src1, len - 8);
	}
	else
	{
		fstb::ToolsAvx2::store_ps_partial (ptr    , src0, len    );
	}
}



void	ProxyRwAvx2 <SplFmt_INT8>::read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/)
{
	const __m128i	src128 =
		_mm_loadu_si128 (reinterpret_cast <const __m128i *> (ptr));
	finish_read_flt (src0, src1, src128);
}

void	ProxyRwAvx2 <SplFmt_INT8>::read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/, int len)
{
	const __m128i	src128 = fstb::ToolsSse2::load_si128_partial (ptr, len);
	finish_read_flt (src0, src1, src128);
}

__m256i	ProxyRwAvx2 <SplFmt_INT8>::read_i16 (const PtrConst::Type &ptr, const __m256i &/*zero*/)
{
	return (fstb::ToolsAvx2::load_16_16l (ptr));
}

__m256i	ProxyRwAvx2 <SplFmt_INT8>::read_i16_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, int len)
{
	return (fstb::ToolsAvx2::load_16_16l_partial (ptr, len));
}

//	const __m256i	mask_lsb = _mm256_set1_epi16 (0x00FF);
//	const __m256i	sign_bit = _mm256_set1_epi16 (-0x8000);
//	const __m256	offset   = _mm256_set1_ps (-32768);
void	ProxyRwAvx2 <SplFmt_INT8>::write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset)
{
	const __m256i	val = prepare_write_flt (src0, src1, sign_bit, offset);
	fstb::ToolsAvx2::store_16_16l (ptr, val, mask_lsb);
}

void	ProxyRwAvx2 <SplFmt_INT8>::write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &mask_lsb, const __m256i &sign_bit, const __m256 &offset, int len)
{
	const __m256i	val = prepare_write_flt (src0, src1, sign_bit, offset);
	fstb::ToolsAvx2::store_16_16l_partial (ptr, val, mask_lsb, len);
}

void	ProxyRwAvx2 <SplFmt_INT8>::finish_read_flt (__m256 &src0, __m256 &src1, const __m128i &src128)
{
	const __m256i	src_0007 = _mm256_cvtepu8_epi32 (src128);
	const __m256i  src_0815 = _mm256_cvtepu8_epi32 (
		_mm_shuffle_epi32 (src128, (2<<0) + (3<<2))
	);
	src0 = _mm256_cvtepi32_ps (src_0007);
	src1 = _mm256_cvtepi32_ps (src_0815);
}

__m256i	ProxyRwAvx2 <SplFmt_INT8>::prepare_write_flt (const __m256 &src0, const __m256 &src1, const __m256i &sign_bit, const __m256 &offset)
{
	__m256			val_0007_f = _mm256_add_ps (src0, offset);
	__m256			val_0815_f = _mm256_add_ps (src1, offset);

	const __m256i	val_0007   = _mm256_cvtps_epi32 (val_0007_f);
	const __m256i	val_0815   = _mm256_cvtps_epi32 (val_0815_f);

	__m256i			val = _mm256_packs_epi32 (val_0007, val_0815);
	val = _mm256_permute4x64_epi64 (val, (0<<0) + (2<<2) + (1<<4) + (3<<6));
	val = _mm256_xor_si256 (val, sign_bit);

	return (val);
}

void	ProxyRwAvx2 <SplFmt_INT8>::write_i16 (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb)
{
	fstb::ToolsAvx2::store_16_16l (ptr, src, mask_lsb);
}

void	ProxyRwAvx2 <SplFmt_INT8>::write_i16_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, int len)
{
	fstb::ToolsAvx2::store_16_16l_partial (ptr, src, mask_lsb, len);
}



// Sign is ignored here
template <bool CLIP_FLAG, bool SIGN_FLAG>
__m256i	ProxyRwAvx2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::read (const PtrConst::Type &ptr, const __m256i &zero, const __m256i &sign_bit)
{
	fstb::unused (zero, sign_bit);

	return (fstb::ToolsAvx2::load_16_16l (ptr));
}

// Sign is ignored here
template <bool CLIP_FLAG, bool SIGN_FLAG>
__m256i	ProxyRwAvx2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::read_partial (const PtrConst::Type &ptr, const __m256i &zero, const __m256i &sign_bit, int len)
{
	fstb::unused (zero, sign_bit);

	return (fstb::ToolsAvx2::load_16_16l_partial (ptr, len));
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwAvx2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit)
{
	const __m256i  val =
		ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::prepare_write_clip (
			src, mi, ma, sign_bit
		);
	fstb::ToolsAvx2::store_16_16l (ptr, val, mask_lsb);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwAvx2 <SplFmt_INT8>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &mask_lsb, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit, int len)
{
	const __m256i  val =
		ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::prepare_write_clip (
			src, mi, ma, sign_bit
		);
	fstb::ToolsAvx2::store_16_16l_partial (ptr, val, mask_lsb, len);
}



void	ProxyRwAvx2 <SplFmt_INT16>::read_flt (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/)
{
	const __m256i  src =
		_mm256_loadu_si256 (reinterpret_cast <const __m256i *> (ptr));
	finish_read_flt (src0, src1, src);
}

void	ProxyRwAvx2 <SplFmt_INT16>::read_flt_partial (const PtrConst::Type &ptr, __m256 &src0, __m256 &src1, const __m256i &/*zero*/, int len)
{
	const __m256i  src =
		fstb::ToolsAvx2::load_si256_partial (ptr, len * int (sizeof (uint16_t)));
	finish_read_flt (src0, src1, src);
}

__m256i	ProxyRwAvx2 <SplFmt_INT16>::read_i16 (const PtrConst::Type &ptr, const __m256i &/*zero*/)
{
	return (_mm256_loadu_si256 (reinterpret_cast <const __m256i *> (ptr)));
}

__m256i	ProxyRwAvx2 <SplFmt_INT16>::read_i16_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, int len)
{
	return (fstb::ToolsAvx2::load_si256_partial (ptr, len * int (sizeof (uint16_t))));
}

//	const __m256i	mask_lsb = _mm256_set1_epi16 (0x00FF);
//	const __m256i	sign_bit = _mm256_set1_epi16 (-0x8000);
//	const __m256	offset   = _mm256_set1_ps (-32768);
void	ProxyRwAvx2 <SplFmt_INT16>::write_flt (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &/*mask_lsb*/, const __m256i &sign_bit, const __m256 &offset)
{
	const __m256i  val = prepare_write_flt (src0, src1, sign_bit, offset);
	_mm256_storeu_si256 (reinterpret_cast <__m256i *> (ptr), val);
}

void	ProxyRwAvx2 <SplFmt_INT16>::write_flt_partial (const Ptr::Type &ptr, const __m256 &src0, const __m256 &src1, const __m256i &/*mask_lsb*/, const __m256i &sign_bit, const __m256 &offset, int len)
{
	const __m256i  val = prepare_write_flt (src0, src1, sign_bit, offset);
	fstb::ToolsAvx2::store_si256_partial (ptr, val, len * int (sizeof (uint16_t)));
}

void	ProxyRwAvx2 <SplFmt_INT16>::finish_read_flt (__m256 &src0, __m256 &src1, const __m256i &src)
{
#if 1
	const __m128i  src128_0007 = _mm256_castsi256_si128 (src);
	const __m128i  src128_0815 = _mm256_extractf128_si256 (src, 1);
	const __m256i  src_0007 = _mm256_cvtepu16_epi32 (src128_0007);
	const __m256i  src_0815 = _mm256_cvtepu16_epi32 (src128_0815);
#else
	__m256i        src_0007 = _mm256_unpacklo_epi16 (src, zero);
	__m256i        src_0815 = _mm256_unpackhi_epi16 (src, zero);
	src_0007 = _mm256_permute4x64_epi64 (src_0007, (0<<0) + (2<<2) + (1<<4) + (3<<6));
	src_0815 = _mm256_permute4x64_epi64 (src_0815, (0<<0) + (2<<2) + (1<<4) + (3<<6));
#endif
	src0 = _mm256_cvtepi32_ps (src_0007);
	src1 = _mm256_cvtepi32_ps (src_0815);
}

__m256i	ProxyRwAvx2 <SplFmt_INT16>::prepare_write_flt (const __m256 &src0, const __m256 &src1, const __m256i &sign_bit, const __m256 &offset)
{
	__m256			val_0007_f = _mm256_add_ps (src0, offset);
	__m256			val_0815_f = _mm256_add_ps (src1, offset);

	const __m256i	val_0007 = _mm256_cvtps_epi32 (val_0007_f);
	const __m256i	val_0815 = _mm256_cvtps_epi32 (val_0815_f);

	__m256i			val = _mm256_packs_epi32 (val_0007, val_0815);
	val = _mm256_permute4x64_epi64 (val, (0<<0) + (2<<2) + (1<<4) + (3<<6));
	val = _mm256_xor_si256 (val, sign_bit);

	return (val);
}



void	ProxyRwAvx2 <SplFmt_INT16>::write_i16 (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/)
{
	_mm256_storeu_si256 (reinterpret_cast <__m256i *> (ptr), src);
}

void	ProxyRwAvx2 <SplFmt_INT16>::write_i16_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, int len)
{
	fstb::ToolsAvx2::store_si256_partial (ptr, src, len * int (sizeof (uint16_t)));
}



template <bool CLIP_FLAG, bool SIGN_FLAG>
__m256i	ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::read (const PtrConst::Type &ptr, const __m256i &/*zero*/, const __m256i &sign_bit)
{
	__m256i        val =
		_mm256_loadu_si256 (reinterpret_cast <const __m256i *> (ptr));
	if (SIGN_FLAG)
	{
		val = _mm256_xor_si256 (val, sign_bit);
	}

	return (val);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
__m256i	ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::read_partial (const PtrConst::Type &ptr, const __m256i &/*zero*/, const __m256i &sign_bit, int len)
{
	__m256i        val =
		fstb::ToolsAvx2::load_si256_partial (ptr, len * int (sizeof (uint16_t)));
	if (SIGN_FLAG)
	{
		val = _mm256_xor_si256 (val, sign_bit);
	}

	return (val);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit)
{
	const __m256i  val = prepare_write_clip (src, mi, ma, sign_bit);
	_mm256_storeu_si256 (reinterpret_cast <__m256i *> (ptr), val);
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
void	ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::write_clip_partial (const Ptr::Type &ptr, const __m256i &src, const __m256i &/*mask_lsb*/, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit, int len)
{
	const __m256i  val = prepare_write_clip (src, mi, ma, sign_bit);
	fstb::ToolsAvx2::store_si256_partial (ptr, val, len * int (sizeof (uint16_t)));
}

template <bool CLIP_FLAG, bool SIGN_FLAG>
__m256i	ProxyRwAvx2 <SplFmt_INT16>::S16 <CLIP_FLAG, SIGN_FLAG>::prepare_write_clip (const __m256i &src, const __m256i &mi, const __m256i &ma, const __m256i &sign_bit)
{
	__m256i        val = src;
	if (CLIP_FLAG)
	{
		val = _mm256_min_epi16 (val, ma);
		val = _mm256_max_epi16 (val, mi);
	}
	if (SIGN_FLAG)
	{
		val = _mm256_xor_si256 (val, sign_bit);
	}

	return (val);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_ProxyRwAvx2_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
