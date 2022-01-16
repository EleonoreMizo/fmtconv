/*****************************************************************************

        Scaler.cpp
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

#include "fmtcl/ContFirInterface.h"
#include "fmtcl/ProxyRwAvx2.h"
#include "fmtcl/ReadWrapperFlt.h"
#include "fmtcl/ReadWrapperInt.h"
#include "fmtcl/Scaler.h"
#include "fmtcl/ScalerCopy.h"
#include "fstb/fnc.h"
#include "fstb/ToolsSse2.h"

#include <algorithm>

#include <cassert>
#include <climits>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#define fmtcl_Scaler_INIT_F_AVX2(DT, ST, DE, SE, FN) \
	_process_plane_flt_##FN##_ptr = &ThisType::process_plane_flt_avx2 <ProxyRwAvx2 <SplFmt_##DE>, ProxyRwAvx2 <SplFmt_##SE> >;

#define fmtcl_Scaler_INIT_I_AVX2(DT, ST, DE, SE, DB, SB, FN) \
	_process_plane_int_##FN##_ptr = &ThisType::process_plane_int_avx2 <ProxyRwAvx2 <SplFmt_##DE>, DB, ProxyRwAvx2 <SplFmt_##SE>, SB>;

void  Scaler::setup_avx2 ()
{
	fmtcl_Scaler_SPAN_F (fmtcl_Scaler_INIT_F_AVX2)
#if ! defined (fmtcl_Scaler_SSE2_16BITS)
	fmtcl_Scaler_SPAN_I (fmtcl_Scaler_INIT_I_AVX2)
#endif
}

#undef fmtcl_Scaler_INIT_F_AVX2
#undef fmtcl_Scaler_INIT_I_AVX2



template <class SRC, bool PF>
static fstb_FORCEINLINE void	Scaler_process_vect_flt_avx2 (__m256 &sum0, __m256 &sum1, int kernel_size, const float *coef_base_ptr, typename SRC::PtrConst::Type pix_ptr, const __m256i &zero, ptrdiff_t src_stride, const __m256 &add_cst, int len)
{
	// Possible optimization: initialize the sum with DST::OFFSET + _add_cst_flt
	// and save the add in the write proxy.
	sum0 = add_cst;
	sum1 = add_cst;

	for (int k = 0; k < kernel_size; ++k)
	{
		__m256         coef = _mm256_set1_ps (coef_base_ptr [k]);
		__m256         src0;
		__m256         src1;
		ReadWrapperFlt <SRC, PF>::read (pix_ptr, src0, src1, zero, len);
		const __m256   val0 = _mm256_mul_ps (src0, coef);
		const __m256   val1 = _mm256_mul_ps (src1, coef);
		sum0 = _mm256_add_ps (sum0, val0);
		sum1 = _mm256_add_ps (sum1, val1);

		SRC::PtrConst::jump (pix_ptr, src_stride);
	}
}



// DST and SRC are ProxyRwAvx2 classes
// Stride offsets in pixels
// Source pointer may be unaligned.
template <class DST, class SRC>
void	Scaler::process_plane_flt_avx2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const
{
	assert (DST::Ptr::check_ptr (dst_ptr, DST::ALIGN_W));
	assert (SRC::PtrConst::check_ptr (src_ptr, SRC::ALIGN_R));
	// When the destination is a buffer:
	// mod4 is enough to guarantee alignment, but since we process pairs of
	// vectors and write_partial() is not different from write() with float
	// data (overwriting all the 64 bytes), we must take extra-care not to
	// overflow from the output buffer.
	// When the destination is not a buffer (output frame data), data may be
	// unaligned anyway. (TO DO: check the algorithm and make sure this
	// constraint is actual).
	assert ((dst_stride & 15) == 0);	
	assert ((src_stride & 3) == 0);
	assert (width > 0);
	assert (y_dst_beg >= 0);
	assert (y_dst_beg < y_dst_end);
	assert (y_dst_end <= _dst_height);
	assert (width <= dst_stride);
	assert (width <= src_stride);

	const __m256i  zero     = _mm256_setzero_si256 ();
	const __m256i  mask_lsb = _mm256_set1_epi16 (0x00FF);
	const __m256i  sign_bit = _mm256_set1_epi16 (-0x8000);
	const __m256   offset   = _mm256_set1_ps (float (DST::OFFSET));
	const __m256   add_cst  = _mm256_set1_ps (float (_add_cst_flt));

	const int      w16 = width & -16;
	const int      w15 = width - w16;

	for (int y = y_dst_beg; y < y_dst_end; ++y)
	{
		const KernelInfo& kernel_info   = _kernel_info_arr [y];
		const int         kernel_size   = kernel_info._kernel_size;
		const float *     coef_base_ptr = &_coef_flt_arr [kernel_info._coef_index];
		const int         ofs_y         = kernel_info._start_line;

		typename SRC::PtrConst::Type  col_src_ptr = src_ptr;
		SRC::PtrConst::jump (col_src_ptr, src_stride * ofs_y);
		typename DST::Ptr::Type       col_dst_ptr = dst_ptr;

		typedef ScalerCopy <DST, 0, SRC, 0> ScCopy;

		if (ScCopy::can_copy (kernel_info._copy_flt_flag))
		{
			ScCopy::copy (col_dst_ptr, col_src_ptr, width);
		}

		else
		{
			__m256         sum0;
			__m256         sum1;

			for (int x = 0; x < w16; x += 16)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				Scaler_process_vect_flt_avx2 <SRC, false> (
					sum0, sum1, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, add_cst, 0
				);
				DST::write_flt (
					col_dst_ptr, sum0, sum1, mask_lsb, sign_bit, offset
				);

				DST::Ptr::jump (col_dst_ptr, 16);
				SRC::PtrConst::jump (col_src_ptr, 16);
			}

			if (w15 > 0)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				Scaler_process_vect_flt_avx2 <SRC, true> (
					sum0, sum1, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, add_cst, w15
				);
				DST::write_flt_partial (
					col_dst_ptr, sum0, sum1, mask_lsb, sign_bit, offset, w15
				);
			}
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
	}

	_mm256_zeroupper ();	// Back to SSE state
}



template <class DST, int DB, class SRC, int SB, bool PF>
static fstb_FORCEINLINE __m256i	Scaler_process_vect_int_avx2 (const __m256i &add_cst, int kernel_size, const __m256i coef_base_ptr [], typename SRC::PtrConst::Type pix_ptr, const __m256i &zero, ptrdiff_t src_stride, const __m256i &sign_bit, int len)
{
	typedef typename SRC::template S16 <false, (SB == 16)> SrcS16R;

	__m256i        sum0 = add_cst;
	__m256i        sum1 = add_cst;

	for (int k = 0; k < kernel_size; ++k)
	{
		const __m256i  coef = _mm256_load_si256 (coef_base_ptr + k);
		const __m256i  src  = ReadWrapperInt <SRC, SrcS16R, PF>::read (
			pix_ptr, zero, sign_bit, len
		);

		fstb::ToolsAvx2::mac_s16_s16_s32 (sum0, sum1, src, coef);

		SRC::PtrConst::jump (pix_ptr, src_stride);
	}

	sum0 = _mm256_srai_epi32 (sum0, Scaler::SHIFT_INT + SB - DB);
	sum1 = _mm256_srai_epi32 (sum1, Scaler::SHIFT_INT + SB - DB);

	const __m256i  val = _mm256_packs_epi32 (sum0, sum1);

	return (val);
}



template <class DST, int DB, class SRC, int SB>
void	Scaler::process_plane_int_avx2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const
{
	assert (_can_int_flag);
	assert (DST::Ptr::check_ptr (dst_ptr, DST::ALIGN_W));
	assert (SRC::PtrConst::check_ptr (src_ptr, SRC::ALIGN_R));
	assert ((dst_stride & 15) == 0);	
	assert (width > 0);
	assert (y_dst_beg >= 0);
	assert (y_dst_beg < y_dst_end);
	assert (y_dst_end <= _dst_height);
	assert (width <= dst_stride);
	assert (width <= src_stride);

	// Rounding constant for the final shift
	const int      r_cst    = 1 << (SHIFT_INT + SB - DB - 1);

	// Sign constants: when we have 16-bit data at one end only,
	// we need to make data signed at the oposite end. This sign
	// constant is reported on the summing constant.
	const int      s_in     = (SB < 16) ? -(0x8000 << (SHIFT_INT + SB - DB)) : 0;
	const int      s_out    = (DB < 16) ?   0x8000 << (SHIFT_INT + SB - DB)  : 0;
	const int      s_cst    = s_in + s_out;

	const __m256i  zero     = _mm256_setzero_si256 ();
	const __m256i  mask_lsb = _mm256_set1_epi16 (0x00FF);
	const __m256i  sign_bit = _mm256_set1_epi16 (-0x8000);
	const __m256i  ma       = _mm256_set1_epi16 (int16_t (uint16_t ((1 << DB) - 1)));
	const __m256i  add_cst  = _mm256_set1_epi32 (_add_cst_int + s_cst + r_cst);

	const int      w16 = width & -16;
	const int      w15 = width - w16;

	for (int y = y_dst_beg; y < y_dst_end; ++y)
	{
		const KernelInfo&    kernel_info   = _kernel_info_arr [y];
		const int            kernel_size   = kernel_info._kernel_size;
		const int            ofs_y         = kernel_info._start_line;
		const __m256i *      coef_base_ptr = reinterpret_cast <const __m256i *> (
			_coef_int_arr.use_vect_avx2 (kernel_info._coef_index)
		);

		typename SRC::PtrConst::Type  col_src_ptr = src_ptr;
		SRC::PtrConst::jump (col_src_ptr, src_stride * ofs_y);
		typename DST::Ptr::Type       col_dst_ptr = dst_ptr;

		typedef ScalerCopy <DST, DB, SRC, SB> ScCopy;

		if (ScCopy::can_copy (kernel_info._copy_int_flag))
		{
			ScCopy::copy (col_dst_ptr, col_src_ptr, width);
		}

		else
		{
			typedef typename DST::template S16 <false, (DB == 16)> DstS16W;

			for (int x = 0; x < w16; x += 16)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				const __m256i  val = Scaler_process_vect_int_avx2 <
					DST, DB, SRC, SB, false
				> (
					add_cst, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, sign_bit, 0
				);

				DstS16W::write_clip (
					col_dst_ptr,
					val,
					mask_lsb,
					zero,
					ma,
					sign_bit
				);

				DST::Ptr::jump (col_dst_ptr, 16);
				SRC::PtrConst::jump (col_src_ptr, 16);
			}

			if (w15 > 0)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				const __m256i  val = Scaler_process_vect_int_avx2 <
					DST, DB, SRC, SB, true
				> (
					add_cst, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, sign_bit, w15
				);

				DstS16W::write_clip_partial (
					col_dst_ptr,
					val,
					mask_lsb,
					zero,
					ma,
					sign_bit,
					w15
				);
			}
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
