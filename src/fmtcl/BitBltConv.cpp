/*****************************************************************************

        BitBltConv.cpp
        Author: Laurent de Soras, 2012

To do:
	- Separate code for aligned and unaligned data.


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

#include "fstb/def.h"

#include "fmtcl/BitBltConv.h"
#include "fmtcl/Proxy.h"
#include "fmtcl/ProxyRwCpp.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fmtcl/ProxyRwSse2.h"
#endif
#include "fstb/fnc.h"

#include <stdexcept>

#include <cassert>
#include <cstring>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



BitBltConv::BitBltConv (bool sse2_flag, bool avx2_flag)
:	_sse2_flag (sse2_flag)
,	_avx2_flag (avx2_flag)
{
	// Nothing
}



// Stride offsets in bytes
// w and h in plane pixels
// No bitdepth reduction (i.e. 10 to 8 forbidden), excepted float to 16-bit integers
void	BitBltConv::bitblt (SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, ptrdiff_t dst_stride, SplFmt src_fmt, int src_res, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (dst_fmt >= 0);
	assert (dst_fmt < SplFmt_NBR_ELT);
	assert (dst_res >= 8);
	assert (dst_ptr != nullptr);
	assert (src_fmt >= 0);
	assert (src_fmt < SplFmt_NBR_ELT);
	assert (src_res >= 8);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (h > 0);

	// Simple copy (same format)
	if (   src_fmt == dst_fmt
	    && src_res == dst_res
	    && is_si_neutral (scale_info_ptr))
	{
		bitblt_same_fmt (
			dst_fmt,
			dst_ptr, dst_stride,
			src_ptr, src_stride,
			w, h
		);
	}

	// Int to float
	else if (src_fmt != SplFmt_FLOAT && dst_fmt == SplFmt_FLOAT)
	{
#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_avx2_flag)
		{
			bitblt_int_to_flt_avx2_switch (
				                  dst_ptr, dst_stride,
				src_fmt, src_res, src_ptr, src_stride,
				w, h, scale_info_ptr
			);
		}
		else
#endif
		{
			bitblt_int_to_flt (
				                  dst_ptr, dst_stride,
				src_fmt, src_res, src_ptr, src_stride,
				w, h, scale_info_ptr
			);
		}
	}

	// Float to int
	else if (src_fmt == SplFmt_FLOAT && dst_fmt != SplFmt_FLOAT && dst_res == 16)
	{
#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_avx2_flag)
		{
			bitblt_flt_to_int_avx2_switch (
				dst_fmt, dst_res, dst_ptr, dst_stride,
				                  src_ptr, src_stride,
				w, h, scale_info_ptr
			);
		}
		else
#endif
		{
			bitblt_flt_to_int (
				dst_fmt, dst_res, dst_ptr, dst_stride,
				                  src_ptr, src_stride,
				w, h, scale_info_ptr
			);
		}
	}

	// Int to int conversion
	else if (src_res <= 16 && dst_res <= 16)
	{
#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_avx2_flag)
		{
			bitblt_int_to_int_avx2_switch (
				dst_fmt, dst_res, dst_ptr, dst_stride,
				src_fmt, src_res, src_ptr, src_stride,
				w, h, scale_info_ptr
			);
		}
		else
#endif
		{
			bitblt_int_to_int (
				dst_fmt, dst_res, dst_ptr, dst_stride,
				src_fmt, src_res, src_ptr, src_stride,
				w, h, scale_info_ptr
			);
		}
	}

	else
	{
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: illegal pixel format conversion."
		);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	BitBltConv::bitblt_int_to_flt (uint8_t *dst_ptr, ptrdiff_t dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	const uint8_t *                    src_i08_ptr (src_ptr);
	const Proxy::PtrInt16Const::Type   src_i16_ptr (
		reinterpret_cast <const uint16_t *> (src_ptr)
	);

	const bool     scale_flag = ! is_si_neutral (scale_info_ptr);

#define	fmtcl_BitBltConv_CASE(SCF, SSE2F, TYPEF, TYPEP, SFMT, SRES, SPTR) \
	case	((SCF << 17) + (SSE2F << 16) + (SplFmt_##SFMT << 8) + SRES): \
		bitblt_int_to_flt_##TYPEF <SCF, ProxyRw##TYPEP <SplFmt_##SFMT>, SRES> ( \
			dst_ptr, dst_stride, src_##SPTR##_ptr, src_stride, \
			w, h, scale_info_ptr \
		); \
		break;

	switch ((scale_flag << 17) + (_sse2_flag << 16) + (src_fmt << 8) + src_res)
	{
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT16  , 16, i16)
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT16  , 14, i16)
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT16  , 12, i16)
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT16  , 10, i16)
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT16  ,  9, i16)
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT8   ,  8, i08)
#if (fstb_ARCHI == fstb_ARCHI_X86)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT16  , 16, i16)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT16  , 14, i16)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT16  , 12, i16)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT16  , 10, i16)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT16  ,  9, i16)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT8   ,  8, i08)
#endif
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT16  , 16, i16)
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT16  , 14, i16)
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT16  , 12, i16)
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT16  , 10, i16)
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT16  ,  9, i16)
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT8   ,  8, i08)
#if (fstb_ARCHI == fstb_ARCHI_X86)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT16  , 16, i16)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT16  , 14, i16)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT16  , 12, i16)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT16  , 10, i16)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT16  ,  9, i16)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT8   ,  8, i08)
#endif
	default:
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: "
			"illegal int-to-float pixel format conversion."
		);
	}

#undef fmtcl_BitBltConv_CASE
}



void	BitBltConv::bitblt_flt_to_int (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, ptrdiff_t dst_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	fstb::unused (dst_res);

	const Proxy::PtrInt16::Type   dst_i16_ptr (
		reinterpret_cast <uint16_t *> (dst_ptr)
	);

	const bool     scale_flag = ! is_si_neutral (scale_info_ptr);

#define	fmtcl_BitBltConv_CASE(SCF, SSE2F, TYPEF, TYPEP, DFMT, DPTR) \
	case	(SCF << 5) + (SSE2F << 4) + SplFmt_##DFMT: \
		bitblt_flt_to_int_##TYPEF <SCF, ProxyRw##TYPEP <SplFmt_##DFMT> > ( \
			dst_##DPTR##_ptr, dst_stride, src_ptr, src_stride, \
			w, h, scale_info_ptr \
		); \
		break;

	switch ((scale_flag << 5) + (_sse2_flag << 4) + dst_fmt)
	{
	fmtcl_BitBltConv_CASE (false, false, cpp , Cpp , INT16  , i16)
#if (fstb_ARCHI == fstb_ARCHI_X86)
	fmtcl_BitBltConv_CASE (false, true , sse2, Sse2, INT16  , i16)
#endif
	fmtcl_BitBltConv_CASE (true , false, cpp , Cpp , INT16  , i16)
#if (fstb_ARCHI == fstb_ARCHI_X86)
	fmtcl_BitBltConv_CASE (true , true , sse2, Sse2, INT16  , i16)
#endif
	default:
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: "
			"illegal float-to-int pixel format conversion."
		);
	}

#undef fmtcl_BitBltConv_CASE
}



void	BitBltConv::bitblt_int_to_int (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, ptrdiff_t dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	fstb::unused (scale_info_ptr);

	const uint8_t *                    src_i08_ptr (src_ptr);
	const Proxy::PtrInt16Const::Type   src_i16_ptr (
		reinterpret_cast <const uint16_t *> (src_ptr)
	);
	const Proxy::PtrInt16::Type        dst_i16_ptr (
		reinterpret_cast <uint16_t *> (dst_ptr)
	);

#define	fmtcl_BitBltConv_CASE(SSE2F, TYPEF, TYPEP, DFMT, SFMT, DRES, SRES, DPTR, SPTR) \
	case	((SSE2F << 24) + (SplFmt_##DFMT << 20) + (SplFmt_##SFMT << 16) + (DRES << 8) + SRES): \
		bitblt_ixx_to_x16_##TYPEF < \
			ProxyRw##TYPEP <SplFmt_##DFMT>, ProxyRw##TYPEP <SplFmt_##SFMT>, \
			DRES, SRES \
		> (dst_##DPTR##_ptr, dst_stride, src_##SPTR##_ptr, src_stride, w, h); \
		break;

	switch ((_sse2_flag << 24) + (dst_fmt << 20) + (src_fmt << 16) + (dst_res << 8) + src_res)
	{
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 16, 14, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 16, 12, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 16, 10, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 16,  9, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT8   , 16,  8, i16, i08)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 14, 12, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 14, 10, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 14,  9, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT8   , 14,  8, i16, i08)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 12, 10, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 12,  9, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT8   , 12,  8, i16, i08)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT16  , 10,  9, i16, i16)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT8   , 10,  8, i16, i08)
	fmtcl_BitBltConv_CASE (false, cpp , Cpp , INT16  , INT8   ,  9,  8, i16, i08)
#if (fstb_ARCHI == fstb_ARCHI_X86)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 16, 14, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 16, 12, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 16, 10, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 16,  9, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT8   , 16,  8, i16, i08)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 14, 12, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 14, 10, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 14,  9, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT8   , 14,  8, i16, i08)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 12, 10, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 12,  9, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT8   , 12,  8, i16, i08)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT16  , 10,  9, i16, i16)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT8   , 10,  8, i16, i08)
	fmtcl_BitBltConv_CASE (true , sse2, Sse2, INT16  , INT8   ,  9,  8, i16, i08)
#endif
	default:
		assert (false);
		throw std::logic_error (
			"fmtcl::BitBltConv::bitblt: "
			"illegal int-to-int pixel format conversion."
		);
	}

#undef fmtcl_BitBltConv_CASE
}



void	BitBltConv::bitblt_same_fmt (SplFmt fmt, uint8_t *dst_ptr, ptrdiff_t dst_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h)
{
	assert (fmt >= 0);
	assert (fmt < SplFmt_NBR_ELT);
	assert (dst_ptr != nullptr);
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (h > 0);

	const int      nbr_bytes = fmtcl::SplFmt_get_unit_size (fmt);

	if (   dst_stride == src_stride
	    && dst_stride == w * nbr_bytes)
	{
		memcpy (dst_ptr, src_ptr, h * dst_stride);
	}

	else
	{
		for (int y = 0; y < h; ++y)
		{
			memcpy (
				dst_ptr + y * dst_stride,
				src_ptr + y * src_stride,
				w * nbr_bytes
			);
		}
	}
}



// Stride offsets are still in bytes
template <bool SF, class SRC, int SBD>
void	BitBltConv::bitblt_int_to_flt_cpp (uint8_t *dst_ptr, ptrdiff_t dst_stride, typename SRC::PtrConst::Type src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (dst_ptr != 0);
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (w > 0);
	assert (h > 0);
	assert (! SF || scale_info_ptr != nullptr);

	const float    gain    = (SF) ? float (scale_info_ptr->_gain   ) : 1;
	const float    add_cst = (SF) ? float (scale_info_ptr->_add_cst) : 0;

	float *        dst_flt_ptr = reinterpret_cast <float *> (dst_ptr);

	src_stride /= sizeof (typename SRC::PtrConst::DataType);
	dst_stride /= sizeof (*dst_flt_ptr);

	for (int y = 0; y < h; ++y)
	{
		typename SRC::PtrConst::Type  cur_src_ptr = src_ptr;

		for (int x = 0; x < w; ++x)
		{
			float          val = float (SRC::read (cur_src_ptr));
			if (SF)
			{
				val = val * gain + add_cst;
			}
			dst_flt_ptr [x] = float (val) * gain + add_cst;

			SRC::PtrConst::jump (cur_src_ptr, 1);
		}

		SRC::PtrConst::jump (src_ptr, src_stride);
		dst_flt_ptr += dst_stride;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

// Stride offsets are still in bytes
// Destination pointer must be 16-byte aligned!
template <bool SF, class SRC, int SBD>
void	BitBltConv::bitblt_int_to_flt_sse2 (uint8_t *dst_ptr, ptrdiff_t dst_stride, typename SRC::PtrConst::Type src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (fstb::ToolsSse2::check_ptr_align (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (w > 0);
	assert (h > 0);
	assert (! SF || scale_info_ptr != nullptr);

	__m128         gain;
	__m128         add_cst;
	if (SF)
	{
		gain    = _mm_set1_ps ((SF) ? float (scale_info_ptr->_gain   ) : 1);
		add_cst = _mm_set1_ps ((SF) ? float (scale_info_ptr->_add_cst) : 0);
	}

	float *        dst_flt_ptr = reinterpret_cast <float *> (dst_ptr);

	src_stride /= sizeof (typename SRC::PtrConst::DataType);
	dst_stride /= sizeof (*dst_flt_ptr);

	const __m128i	zero = _mm_setzero_si128 ();

	const int      w8 = w & -8;
	const int      w7 = w - w8;

	for (int y = 0; y < h; ++y)
	{
		typename SRC::PtrConst::Type  cur_src_ptr (src_ptr);
		__m128         val_03;
		__m128         val_47;

		for (int x = 0; x < w8; x += 8)
		{
			SRC::read_flt (cur_src_ptr, val_03, val_47, zero);
			if (SF)
			{
				val_03 = _mm_add_ps (_mm_mul_ps (val_03, gain), add_cst);
				val_47 = _mm_add_ps (_mm_mul_ps (val_47, gain), add_cst);
			}
			_mm_store_ps (dst_flt_ptr + x    , val_03);
			_mm_store_ps (dst_flt_ptr + x + 4, val_47);

			SRC::PtrConst::jump (cur_src_ptr, 8);
		}

		if (w7 > 0)
		{
			SRC::read_flt_partial (cur_src_ptr, val_03, val_47, zero, w7);
			if (SF)
			{
				val_03 = _mm_add_ps (_mm_mul_ps (val_03, gain), add_cst);
				val_47 = _mm_add_ps (_mm_mul_ps (val_47, gain), add_cst);
			}
			_mm_store_ps (dst_flt_ptr + w8    , val_03);
			if (w7 > 4)
			{
				_mm_store_ps (dst_flt_ptr + w8 + 4, val_47);
			}
		}

		SRC::PtrConst::jump (src_ptr, src_stride);
		dst_flt_ptr += dst_stride;
	}
}

#endif



// Stride offsets are still in bytes
template <bool SF, class DST>
void	BitBltConv::bitblt_flt_to_int_cpp (typename DST::Ptr::Type dst_ptr, ptrdiff_t dst_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (h > 0);
	assert (! SF || scale_info_ptr != nullptr);

	const float    gain    = (SF) ? float (scale_info_ptr->_gain   ) : 1;
	const float    add_cst = (SF) ? float (scale_info_ptr->_add_cst) : 0;

	const float *  src_flt_ptr = reinterpret_cast <const float *> (src_ptr);

	src_stride /= sizeof (*src_flt_ptr);
	dst_stride /= sizeof (typename DST::Ptr::DataType);

	for (int y = 0; y < h; ++y)
	{
		typename DST::Ptr::Type cur_dst_ptr = dst_ptr;

		for (int x = 0; x < w; ++x)
		{
			float          val = src_flt_ptr [x];
			if (SF)
			{
				val = val * gain + add_cst;
			}
			DST::template write_clip <16> (cur_dst_ptr, fstb::conv_int_fast (val));

			DST::Ptr::jump (cur_dst_ptr, 1);
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
		src_flt_ptr += src_stride;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

// Stride offsets are still in bytes
template <bool SF, class DST>
void	BitBltConv::bitblt_flt_to_int_sse2 (typename DST::Ptr::Type dst_ptr, ptrdiff_t dst_stride, const uint8_t *src_ptr, ptrdiff_t src_stride, int w, int h, const ScaleInfo *scale_info_ptr)
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (src_ptr != nullptr);
	assert (w > 0);
	assert (h > 0);
	assert (! SF || scale_info_ptr != nullptr);

	__m128         gain;
	__m128         add_cst;
	if (SF)
	{
		gain    = _mm_set1_ps ((SF) ? float (scale_info_ptr->_gain   ) : 1);
		add_cst = _mm_set1_ps ((SF) ? float (scale_info_ptr->_add_cst) : 0);
	}

	const float *  src_flt_ptr = reinterpret_cast <const float *> (src_ptr);

	src_stride /= sizeof (*src_flt_ptr);
	dst_stride /= sizeof (typename DST::Ptr::DataType);

	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit = _mm_set1_epi16 (-0x8000);
	const __m128i  zero     = _mm_setzero_si128 ();
	const __m128   offset   = _mm_set1_ps (-32768);

	const int      w8 = w & -8;
	const int      w7 = w - w8;

	for (int y = 0; y < h; ++y)
	{
		typename DST::Ptr::Type cur_dst_ptr = dst_ptr;
		__m128         val_03;
		__m128         val_47;

		for (int x = 0; x < w8; x += 8)
		{
			val_03 = _mm_loadu_ps (src_flt_ptr + x    );
			val_47 = _mm_loadu_ps (src_flt_ptr + x + 4);
			if (SF)
			{
				val_03 = _mm_add_ps (_mm_mul_ps (val_03, gain), add_cst);
				val_47 = _mm_add_ps (_mm_mul_ps (val_47, gain), add_cst);
			}
			DST::write_flt (
				cur_dst_ptr, val_03, val_47, mask_lsb, sign_bit, offset
			);

			DST::Ptr::jump (cur_dst_ptr, 8);
		}

		if (w7 > 0)
		{
			ProxyRwSse2 <SplFmt_FLOAT>::read_flt_partial (
				src_flt_ptr + w8, val_03, val_47, zero, w7
			);
			if (SF)
			{
				val_03 = _mm_add_ps (_mm_mul_ps (val_03, gain), add_cst);
				val_47 = _mm_add_ps (_mm_mul_ps (val_47, gain), add_cst);
			}
			DST::write_flt_partial (
				cur_dst_ptr, val_03, val_47, mask_lsb, sign_bit, offset, w7
			);
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
		src_flt_ptr += src_stride;
	}
}

#endif



// Stride offsets are still in bytes
// 8 <= SBD <= DBD <= 16
template <class DST, class SRC, int DBD, int SBD>
void	BitBltConv::bitblt_ixx_to_x16_cpp (typename DST::Ptr::Type dst_ptr, ptrdiff_t dst_stride, typename SRC::PtrConst::Type src_ptr, ptrdiff_t src_stride, int w, int h)
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (w > 0);
	assert (h > 0);

	src_stride /= sizeof (typename SRC::PtrConst::DataType);
	dst_stride /= sizeof (typename DST::Ptr::DataType);

	for (int y = 0; y < h; ++y)
	{
		typename DST::Ptr::Type       cur_dst_ptr = dst_ptr;
		typename SRC::PtrConst::Type  cur_src_ptr = src_ptr;

		for (int x = 0; x < w; ++x)
		{
			int            val = SRC::read (cur_src_ptr);
			if (SBD == 16 && DBD == 16)
			{
				DST::write_no_clip (cur_dst_ptr, val);
			}
			else
			{
				val = val << (DBD - SBD);
				DST::template write_clip <DBD> (cur_dst_ptr, val);
			}

			SRC::PtrConst::jump (cur_src_ptr, 1);
			DST::Ptr::jump (cur_dst_ptr, 1);
		}

		SRC::PtrConst::jump (src_ptr, src_stride);
		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

// Stride offsets are still in bytes
template <class DST, class SRC, int DBD, int SBD>
void	BitBltConv::bitblt_ixx_to_x16_sse2 (typename DST::Ptr::Type dst_ptr, ptrdiff_t dst_stride, typename SRC::PtrConst::Type src_ptr, ptrdiff_t src_stride, int w, int h)
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

	const __m128i  zero     = _mm_setzero_si128 ();
	const __m128i  val_ma   = _mm_set1_epi16 ((DBD < 16) ? (1 << DBD) - 1 : 0);
	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);

	const int      w8 = w & -8;
	const int      w7 = w - w8;

	for (int y = 0; y < h; ++y)
	{
		typename DST::Ptr::Type       cur_dst_ptr = dst_ptr;
		typename SRC::PtrConst::Type  cur_src_ptr = src_ptr;

		for (int x = 0; x < w8; x += 8)
		{
			__m128i        val = SRC::read_i16 (cur_src_ptr, zero);
			if (DBD != SBD)
			{
				val = _mm_slli_epi16 (val, DBD - SBD);
			}
			if (DBD < 16)
			{
				val = _mm_min_epi16 (val, val_ma);
			}
			DST::write_i16 (cur_dst_ptr, val, mask_lsb);

			SRC::PtrConst::jump (cur_src_ptr, 8);
			DST::Ptr::jump (cur_dst_ptr, 8);
		}

		if (w7 > 0)
		{
			__m128i        val = SRC::read_i16_partial (cur_src_ptr, zero, w7);
			if (DBD != SBD)
			{
				val = _mm_slli_epi16 (val, DBD - SBD);
			}
			if (DBD < 16)
			{
				val = _mm_min_epi16 (val, val_ma);
			}
			DST::write_i16_partial (cur_dst_ptr, val, mask_lsb, w7);
		}

		SRC::PtrConst::jump (src_ptr, src_stride);
		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}

#endif



bool	BitBltConv::is_si_neutral (const ScaleInfo *scale_info_ptr)
{
	return (       scale_info_ptr == nullptr
	        || (   fstb::is_eq (scale_info_ptr->_gain, 1.0)
	            && fstb::is_null (scale_info_ptr->_add_cst)));
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
