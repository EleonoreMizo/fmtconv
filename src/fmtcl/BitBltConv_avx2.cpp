/*****************************************************************************

        BitBltConv_avx2.cpp
        Author: Laurent de Soras, 2015

To be compiled with /arch:AVX2 in order to avoid SSE/AVX state switch
slowdown.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/BitBltConv.h"
#include "fmtcl/Proxy.h"
#include "fmtcl/ProxyRwAvx2.h"
#include "fstb/fnc.h"

#include <stdexcept>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	BitBltConv::bitblt_int_to_flt_avx2_switch (uint8_t *dst_ptr, ptrdiff_t dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	const uint8_t *                    src_i08_ptr (src_ptr);
	const Proxy::PtrInt16Const::Type   src_i16_ptr (
		reinterpret_cast <const uint16_t *> (src_ptr)
	);

	const bool     scale_flag = ! is_si_neutral (scale_info_ptr);

#define	fmtcl_BitBltConv_CASE(SCF, SFMT, SRES, SPTR) \
	case	((SCF << 16) + (SplFmt_##SFMT << 8) + SRES): \
		bitblt_int_to_flt_avx2 <SCF, ProxyRwAvx2 <SplFmt_##SFMT>, SRES> ( \
			dst_ptr, dst_stride, src_##SPTR##_ptr, src_stride, \
			w, h, scale_info_ptr \
		); \
		break;

	switch ((scale_flag << 16) + (src_fmt << 8) + src_res)
	{
	fmtcl_BitBltConv_CASE (false, INT16  , 16, i16)
	fmtcl_BitBltConv_CASE (false, INT16  , 12, i16)
	fmtcl_BitBltConv_CASE (false, INT16  , 10, i16)
	fmtcl_BitBltConv_CASE (false, INT16  ,  9, i16)
	fmtcl_BitBltConv_CASE (false, INT8   ,  8, i08)
	fmtcl_BitBltConv_CASE (true , INT16  , 16, i16)
	fmtcl_BitBltConv_CASE (true , INT16  , 12, i16)
	fmtcl_BitBltConv_CASE (true , INT16  , 10, i16)
	fmtcl_BitBltConv_CASE (true , INT16  ,  9, i16)
	fmtcl_BitBltConv_CASE (true , INT8   ,  8, i08)
	default:
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: "
			"illegal int-to-float pixel format conversion."
		);
	}

#undef fmtcl_BitBltConv_CASE
}



void	BitBltConv::bitblt_flt_to_int_avx2_switch (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, ptrdiff_t dst_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	fstb::unused (dst_res);

	const Proxy::PtrInt16::Type   dst_i16_ptr (
		reinterpret_cast <uint16_t *> (dst_ptr)
	);

	const bool     scale_flag = ! is_si_neutral (scale_info_ptr);

#define	fmtcl_BitBltConv_CASE(SCF, DFMT, DPTR) \
	case	(SCF << 4) + SplFmt_##DFMT: \
		bitblt_flt_to_int_avx2 <SCF, ProxyRwAvx2 <SplFmt_##DFMT> > ( \
			dst_##DPTR##_ptr, dst_stride, src_ptr, src_stride, \
			w, h, scale_info_ptr \
		); \
		break;

	switch ((scale_flag << 4) + dst_fmt)
	{
	fmtcl_BitBltConv_CASE (false, INT16  , i16)
	fmtcl_BitBltConv_CASE (true , INT16  , i16)
	default:
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: "
			"illegal float-to-int pixel format conversion."
		);
	}

#undef fmtcl_BitBltConv_CASE
}



void	BitBltConv::bitblt_int_to_int_avx2_switch (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, ptrdiff_t dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	fstb::unused (scale_info_ptr);

	const uint8_t *                    src_i08_ptr (src_ptr);
	const Proxy::PtrInt16Const::Type   src_i16_ptr (
		reinterpret_cast <const uint16_t *> (src_ptr)
	);
	const Proxy::PtrInt16::Type        dst_i16_ptr (
		reinterpret_cast <uint16_t *> (dst_ptr)
	);

#define	fmtcl_BitBltConv_CASE(DFMT, SFMT, DRES, SRES, DPTR, SPTR) \
	case	((SplFmt_##DFMT << 20) + (SplFmt_##SFMT << 16) + (DRES << 8) + SRES): \
		bitblt_ixx_to_x16_avx2 < \
			ProxyRwAvx2 <SplFmt_##DFMT>, ProxyRwAvx2 <SplFmt_##SFMT>, \
			DRES, SRES \
		> (dst_##DPTR##_ptr, dst_stride, src_##SPTR##_ptr, src_stride, w, h); \
		break;

	switch ((dst_fmt << 20) + (src_fmt << 16) + (dst_res << 8) + src_res)
	{
	fmtcl_BitBltConv_CASE (INT16  , INT16  , 16, 12, i16, i16)
	fmtcl_BitBltConv_CASE (INT16  , INT16  , 16, 10, i16, i16)
	fmtcl_BitBltConv_CASE (INT16  , INT16  , 16,  9, i16, i16)
	fmtcl_BitBltConv_CASE (INT16  , INT8   , 16,  8, i16, i08)
	fmtcl_BitBltConv_CASE (INT16  , INT16  , 12, 10, i16, i16)
	fmtcl_BitBltConv_CASE (INT16  , INT16  , 12,  9, i16, i16)
	fmtcl_BitBltConv_CASE (INT16  , INT8   , 12,  8, i16, i08)
	fmtcl_BitBltConv_CASE (INT16  , INT16  , 10,  9, i16, i16)
	fmtcl_BitBltConv_CASE (INT16  , INT8   , 10,  8, i16, i08)
	fmtcl_BitBltConv_CASE (INT16  , INT8   ,  9,  8, i16, i08)
	default:
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: "
			"illegal int-to-int pixel format conversion."
		);
	}

#undef fmtcl_BitBltConv_CASE
}



// Stride offsets are still in bytes
// Destination pointer must be 32-byte aligned!
template <bool SF, class SRC, int SBD>
void	BitBltConv::bitblt_int_to_flt_avx2 (uint8_t *dst_ptr, ptrdiff_t dst_stride, typename SRC::PtrConst::Type src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (fstb::ToolsAvx2::check_ptr_align (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (w > 0);
	assert (h > 0);
	assert (! SF || scale_info_ptr != nullptr);

	__m256         gain;
	__m256         add_cst;
	if (SF)
	{
		gain    = _mm256_set1_ps ((SF) ? float (scale_info_ptr->_gain   ) : 1);
		add_cst = _mm256_set1_ps ((SF) ? float (scale_info_ptr->_add_cst) : 0);
	}

	float *        dst_flt_ptr = reinterpret_cast <float *> (dst_ptr);

	src_stride /= sizeof (typename SRC::PtrConst::DataType);
	dst_stride /= sizeof (*dst_flt_ptr);

	const __m256i	zero = _mm256_setzero_si256 ();

	const int      w16 = w & -16;
	const int      w15 = w - w16;

	for (int y = 0; y < h; ++y)
	{
		typename SRC::PtrConst::Type  cur_src_ptr (src_ptr);
		__m256         val_0007;
		__m256         val_0815;

		for (int x = 0; x < w16; x += 16)
		{
			SRC::read_flt (cur_src_ptr, val_0007, val_0815, zero);
			if (SF)
			{
				val_0007 = _mm256_add_ps (_mm256_mul_ps (val_0007, gain), add_cst);
				val_0815 = _mm256_add_ps (_mm256_mul_ps (val_0815, gain), add_cst);
			}
			_mm256_store_ps (dst_flt_ptr + x    , val_0007);
			_mm256_store_ps (dst_flt_ptr + x + 8, val_0815);

			SRC::PtrConst::jump (cur_src_ptr, 16);
		}

		if (w15 > 0)
		{
			SRC::read_flt (cur_src_ptr, val_0007, val_0815, zero);
			if (SF)
			{
				val_0007 = _mm256_add_ps (_mm256_mul_ps (val_0007, gain), add_cst);
				val_0815 = _mm256_add_ps (_mm256_mul_ps (val_0815, gain), add_cst);
			}
			_mm256_store_ps (dst_flt_ptr + w16, val_0007);
			if (w15 > 8)
			{
				_mm256_store_ps (dst_flt_ptr + w16 + 8, val_0815);
			}
		}

		SRC::PtrConst::jump (src_ptr, src_stride);
		dst_flt_ptr += dst_stride;
	}
}



// Stride offsets are still in bytes
template <bool SF, class DST>
void	BitBltConv::bitblt_flt_to_int_avx2 (typename DST::Ptr::Type dst_ptr, ptrdiff_t dst_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (h > 0);
	assert (! SF || scale_info_ptr != nullptr);

	__m256         gain;
	__m256         add_cst;
	if (SF)
	{
		gain    = _mm256_set1_ps ((SF) ? float (scale_info_ptr->_gain   ) : 1);
		add_cst = _mm256_set1_ps ((SF) ? float (scale_info_ptr->_add_cst) : 0);
	}

	const float *  src_flt_ptr = reinterpret_cast <const float *> (src_ptr);

	src_stride /= sizeof (*src_flt_ptr);
	dst_stride /= sizeof (typename DST::Ptr::DataType);

	const __m256i  mask_lsb = _mm256_set1_epi16 (0x00FF);
	const __m256i  sign_bit = _mm256_set1_epi16 (-0x8000);
	const __m256i  zero     = _mm256_setzero_si256 ();
	const __m256   offset   = _mm256_set1_ps (-32768);

	const int      w16 = w & -16;
	const int      w15 = w - w16;

	for (int y = 0; y < h; ++y)
	{
		typename DST::Ptr::Type cur_dst_ptr = dst_ptr;
		__m256         val_0007;
		__m256         val_0815;

		for (int x = 0; x < w16; x += 16)
		{
			val_0007 = _mm256_loadu_ps (src_flt_ptr + x    );
			val_0815 = _mm256_loadu_ps (src_flt_ptr + x + 8);
			if (SF)
			{
				val_0007 = _mm256_add_ps (_mm256_mul_ps (val_0007, gain), add_cst);
				val_0815 = _mm256_add_ps (_mm256_mul_ps (val_0815, gain), add_cst);
			}
			DST::write_flt (
				cur_dst_ptr, val_0007, val_0815, mask_lsb, sign_bit, offset
			);

			DST::Ptr::jump (cur_dst_ptr, 16);
		}

		if (w15 > 0)
		{
			ProxyRwAvx2 <SplFmt_FLOAT>::read_flt_partial (
				src_flt_ptr + w16, val_0007, val_0815, zero, w15
			);
			if (SF)
			{
				val_0007 = _mm256_add_ps (_mm256_mul_ps (val_0007, gain), add_cst);
				val_0815 = _mm256_add_ps (_mm256_mul_ps (val_0815, gain), add_cst);
			}
			DST::write_flt_partial (
				cur_dst_ptr, val_0007, val_0815, mask_lsb, sign_bit, offset, w15
			);
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
		src_flt_ptr += src_stride;
	}
}



// Stride offsets are still in bytes
template <class DST, class SRC, int DBD, int SBD>
void	BitBltConv::bitblt_ixx_to_x16_avx2 (typename DST::Ptr::Type dst_ptr, ptrdiff_t dst_stride, typename SRC::PtrConst::Type src_ptr, ptrdiff_t src_stride, int w, int h)
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (w > 0);
	assert (h > 0);

	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (w > 0);
	assert (h > 0);

	src_stride /= sizeof (typename SRC::PtrConst::DataType);
	dst_stride /= sizeof (typename DST::Ptr::DataType);

	const __m256i  zero     = _mm256_setzero_si256 ();
	const __m256i  val_ma   = _mm256_set1_epi16 ((DBD < 16) ? (1 << DBD) - 1 : 0);
	const __m256i  mask_lsb = _mm256_set1_epi16 (0x00FF);

	const int      w16 = w & -16;
	const int      w15 = w - w16;

	for (int y = 0; y < h; ++y)
	{
		typename DST::Ptr::Type       cur_dst_ptr = dst_ptr;
		typename SRC::PtrConst::Type  cur_src_ptr = src_ptr;

		for (int x = 0; x < w16; x += 16)
		{
			__m256i        val = SRC::read_i16 (cur_src_ptr, zero);
			if (DBD != SBD)
			{
				val = _mm256_slli_epi16 (val, DBD - SBD);
			}
			if (DBD < 16)
			{
				val = _mm256_min_epi16 (val, val_ma);
			}
			DST::write_i16 (cur_dst_ptr, val, mask_lsb);

			SRC::PtrConst::jump (cur_src_ptr, 16);
			DST::Ptr::jump (cur_dst_ptr, 16);
		}

		if (w15 > 0)
		{
			__m256i        val = SRC::read_i16_partial (cur_src_ptr, zero, w15);
			if (DBD != SBD)
			{
				val = _mm256_slli_epi16 (val, DBD - SBD);
			}
			if (DBD < 16)
			{
				val = _mm256_min_epi16 (val, val_ma);
			}
			DST::write_i16_partial (cur_dst_ptr, val, mask_lsb, w15);
		}

		SRC::PtrConst::jump (src_ptr, src_stride);
		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
