/*****************************************************************************

        Matrix_avx2.cpp
        Author: Laurent de Soras, 2015

To be compiled with /arch:AVX in order to avoid SSE/AVX state switch
slowdown.

TO DO:
	- Make the AVX2 code use aligned read/write.
	- Make a special case for kRkGkB matrix conversions, where the luma plane
		is not used to compute the chroma planes.

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

#include "fmtc/Matrix.h"
#include "fmtcl/ProxyRwAvx2.h"
#include "fstb/ToolsAvx2.h"

#include <immintrin.h>

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/




/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Matrix::config_avx2_matrix_n ()
{
	const ::VSFormat &   fmt_src = *_vi_in.format;
	const ::VSFormat &   fmt_dst = *_vi_out.format;
	const int            np      = (_plane_out >= 0) ? 1 : 0;

#define Matrix_CASE_AVX2(DST, DB, SRC, SB) \
	case	(DB << 8) + (SB << 1) + 0: \
		_apply_matrix_ptr = &ThisType::apply_matrix_n_avx2_int < \
			fmtcl::ProxyRwAvx2 <fmtcl::SplFmt_##DST>, DB, \
			fmtcl::ProxyRwAvx2 <fmtcl::SplFmt_##SRC>, SB, \
			3 \
		>; \
		break; \
	case	(DB << 8) + (SB << 1) + 1: \
		_apply_matrix_ptr = &ThisType::apply_matrix_n_avx2_int < \
			fmtcl::ProxyRwAvx2 <fmtcl::SplFmt_##DST>, DB, \
			fmtcl::ProxyRwAvx2 <fmtcl::SplFmt_##SRC>, SB, \
			1 \
		>; \
		break;

	switch ((fmt_dst.bitsPerSample << 8) + (fmt_src.bitsPerSample << 1) + np)
	{
	Matrix_CASE_AVX2 (INT8 ,  8, INT8 ,  8)
	Matrix_CASE_AVX2 (INT16,  9, INT8 ,  8)
	Matrix_CASE_AVX2 (INT16,  9, INT16,  9)
	Matrix_CASE_AVX2 (INT16, 10, INT8 ,  8)
	Matrix_CASE_AVX2 (INT16, 10, INT16,  9)
	Matrix_CASE_AVX2 (INT16, 10, INT16, 10)
	Matrix_CASE_AVX2 (INT16, 12, INT8 ,  8)
	Matrix_CASE_AVX2 (INT16, 12, INT16,  9)
	Matrix_CASE_AVX2 (INT16, 12, INT16, 10)
	Matrix_CASE_AVX2 (INT16, 12, INT16, 12)
	Matrix_CASE_AVX2 (INT16, 16, INT8 ,  8)
	Matrix_CASE_AVX2 (INT16, 16, INT16,  9)
	Matrix_CASE_AVX2 (INT16, 16, INT16, 10)
	Matrix_CASE_AVX2 (INT16, 16, INT16, 12)
	Matrix_CASE_AVX2 (INT16, 16, INT16, 16)
	default:
		assert (false);
		throw_logic_err (
			"unhandled case for AVX2 function assignation. "
			"Please contact the plug-in developer."
		);
		break;
	}

#undef Matrix_CASE_AVX2
}



// DST and SRC are ProxyRwAvx2 classes
template <class DST, int DB, class SRC, int SB, int NP>
void	Matrix::apply_matrix_n_avx2_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);
	assert (_vi_in.format->bitsPerSample == SB);
	assert (_vi_out.format->bitsPerSample == DB);

	enum { BPS_SRC = (SB + 7) >> 3 };
	enum { BPS_DST = (DB + 7) >> 3 };
	assert (_vi_in.format->bytesPerSample == BPS_SRC);
	assert (_vi_out.format->bytesPerSample == BPS_DST);

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const __m256i  zero     = _mm256_setzero_si256 ();
	const __m256i  mask_lsb = _mm256_set1_epi16 (0x00FF);
	const __m256i  sign_bit = _mm256_set1_epi16 (-0x8000);
	const __m256i  ma       = _mm256_set1_epi16 (int16_t ((1 << DB) - 1));

	const __m256i* coef_ptr = reinterpret_cast <const __m256i *> (
		_coef_simd_arr.use_vect_avx2 (0)
	);

	const int      nbr_planes = std::min (_vi_out.format->numPlanes, NP);
	const int      src_0_str = _vsapi.getStride (&src, 0);
	const int      src_1_str = _vsapi.getStride (&src, 1);
	const int      src_2_str = _vsapi.getStride (&src, 2);

	for (int y = 0; y < h; ++y)
	{
		for (int plane_index = 0; plane_index < nbr_planes; ++ plane_index)
		{
			const uint8_t* src_0_ptr = _vsapi.getReadPtr (&src, 0) + y * src_0_str;
			const uint8_t* src_1_ptr = _vsapi.getReadPtr (&src, 1) + y * src_1_str;
			const uint8_t* src_2_ptr = _vsapi.getReadPtr (&src, 2) + y * src_2_str;
			const int      dst_str   = _vsapi.getStride (&dst, plane_index);
			uint8_t *      dst_ptr   = _vsapi.getWritePtr (&dst, plane_index) + y * dst_str;
			const int      cind      = plane_index * (NBR_PLANES + 1);

			for (int x = 0; x < w; x += 16)
			{
				typedef typename SRC::template S16 <false     , (SB == 16)> SrcS16R;
				typedef typename DST::template S16 <(DB != 16), (DB == 16)> DstS16W;

				const __m256i  s0 = SrcS16R::read (
					reinterpret_cast <SrcPtr> (src_0_ptr + x * BPS_SRC),
					zero,
					sign_bit
				);
				const __m256i  s1 = SrcS16R::read (
					reinterpret_cast <SrcPtr> (src_1_ptr + x * BPS_SRC),
					zero,
					sign_bit
				);
				const __m256i  s2 = SrcS16R::read (
					reinterpret_cast <SrcPtr> (src_2_ptr + x * BPS_SRC),
					zero,
					sign_bit
				);

				__m256i        d0 = _mm256_load_si256 (coef_ptr + cind + 3);
				__m256i        d1 = d0;

				// src is variable, up to 16-bit signed (full range, +1 = 32767+1)
				// coef is 13-bit signed (+1 = 4096)
				// dst1 and dst2 are 28-bit signed (+1 = 2 ^ 27) packed on 32-bit int.
				// Maximum headroom: *16 (4 bits)
				fstb::ToolsAvx2::mac_s16_s16_s32 (
					d0, d1, s0, _mm256_load_si256 (coef_ptr + cind + 0));
				fstb::ToolsAvx2::mac_s16_s16_s32 (
					d0, d1, s1, _mm256_load_si256 (coef_ptr + cind + 1));
				fstb::ToolsAvx2::mac_s16_s16_s32 (
					d0, d1, s2, _mm256_load_si256 (coef_ptr + cind + 2));

				d0 = _mm256_srai_epi32 (d0, SHIFT_INT + SB - DB);
				d1 = _mm256_srai_epi32 (d1, SHIFT_INT + SB - DB);

				__m256i			val = _mm256_packs_epi32 (d0, d1);

				DstS16W::write_clip (
					reinterpret_cast <DstPtr> (dst_ptr + x * BPS_DST),
					val,
					mask_lsb,
					zero,
					ma,
					sign_bit
				);
			}
		}
	}

	_mm256_zeroupper ();	// Back to SSE state
}



void	Matrix::apply_matrix_3_avx_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	float *        dst_1_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 1));
	float *        dst_2_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	const __m256   c00 = _mm256_set1_ps (_coef_flt_arr [ 0]);
	const __m256   c01 = _mm256_set1_ps (_coef_flt_arr [ 1]);
	const __m256   c02 = _mm256_set1_ps (_coef_flt_arr [ 2]);
	const __m256   c03 = _mm256_set1_ps (_coef_flt_arr [ 3]);
	const __m256   c04 = _mm256_set1_ps (_coef_flt_arr [ 4]);
	const __m256   c05 = _mm256_set1_ps (_coef_flt_arr [ 5]);
	const __m256   c06 = _mm256_set1_ps (_coef_flt_arr [ 6]);
	const __m256   c07 = _mm256_set1_ps (_coef_flt_arr [ 7]);
	const __m256   c08 = _mm256_set1_ps (_coef_flt_arr [ 8]);
	const __m256   c09 = _mm256_set1_ps (_coef_flt_arr [ 9]);
	const __m256   c10 = _mm256_set1_ps (_coef_flt_arr [10]);
	const __m256   c11 = _mm256_set1_ps (_coef_flt_arr [11]);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; x += 8)
		{
			const __m256   s0 = _mm256_load_ps (src_0_dat_ptr + x);
			const __m256   s1 = _mm256_load_ps (src_1_dat_ptr + x);
			const __m256   s2 = _mm256_load_ps (src_2_dat_ptr + x);

			const __m256   d0 = _mm256_add_ps (_mm256_add_ps (_mm256_add_ps (
				_mm256_mul_ps (s0, c00),
				_mm256_mul_ps (s1, c01)),
				_mm256_mul_ps (s2, c02)),
				                c03);
			const __m256   d1 = _mm256_add_ps (_mm256_add_ps (_mm256_add_ps (
				_mm256_mul_ps (s0, c04),
				_mm256_mul_ps (s1, c05)),
				_mm256_mul_ps (s2, c06)),
				                c07);
			const __m256   d2 = _mm256_add_ps (_mm256_add_ps (_mm256_add_ps (
				_mm256_mul_ps (s0, c08),
				_mm256_mul_ps (s1, c09)),
				_mm256_mul_ps (s2, c10)),
				                c11);

			_mm256_store_ps (dst_0_dat_ptr + x, d0);
			_mm256_store_ps (dst_1_dat_ptr + x, d1);
			_mm256_store_ps (dst_2_dat_ptr + x, d2);
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}

	_mm256_zeroupper ();	// Back to SSE state
}



void	Matrix::apply_matrix_1_avx_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));

	const __m256   c00 = _mm256_set1_ps (_coef_flt_arr [ 0]);
	const __m256   c01 = _mm256_set1_ps (_coef_flt_arr [ 1]);
	const __m256   c02 = _mm256_set1_ps (_coef_flt_arr [ 2]);
	const __m256   c03 = _mm256_set1_ps (_coef_flt_arr [ 3]);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; x += 4)
		{
			const __m256   s0 = _mm256_load_ps (src_0_dat_ptr + x);
			const __m256   s1 = _mm256_load_ps (src_1_dat_ptr + x);
			const __m256   s2 = _mm256_load_ps (src_2_dat_ptr + x);

			const __m256   d0 = _mm256_add_ps (_mm256_add_ps (_mm256_add_ps (
				_mm256_mul_ps (s0, c00),
				_mm256_mul_ps (s1, c01)),
				_mm256_mul_ps (s2, c02)),
				                c03);

			_mm256_store_ps (dst_0_dat_ptr + x, d0);
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
	}

	_mm256_zeroupper ();	// Back to SSE state
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
