/*****************************************************************************

        MatrixProc_avx2.cpp
        Author: Laurent de Soras, 2015

To be compiled with /arch:AVX in order to avoid SSE/AVX state switch
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

#include "fmtcl/MatrixProc.h"
#include "fmtcl/MatrixProc_macro.h"
#include "fmtcl/ProxyRwAvx2.h"
#include "fstb/ToolsAvx2.h"

#include <immintrin.h>

#include <algorithm>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	MatrixProc::setup_fnc_avx2 (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag)
{
	// Integer
	if (int_proc_flag)
	{
#define fmtcl_MatrixProc_CASE_INT(DF, DB, SF, SB) \
		case   (fmtcl::SplFmt_##DF << 18) + (DB << 11) \
		     + (fmtcl::SplFmt_##SF <<  8) + (SB <<  1) + 0: \
			_proc_ptr = &ThisType::process_n_int_avx2 < \
				ProxyRwAvx2 <fmtcl::SplFmt_##DF>, DB, \
				ProxyRwAvx2 <fmtcl::SplFmt_##SF>, SB, 3 \
			>; \
			break; \
		case   (fmtcl::SplFmt_##DF << 18) + (DB << 11) \
		     + (fmtcl::SplFmt_##SF <<  8) + (SB <<  1) + 1: \
			_proc_ptr = &ThisType::process_n_int_avx2 < \
				ProxyRwAvx2 <fmtcl::SplFmt_##DF>, DB, \
				ProxyRwAvx2 <fmtcl::SplFmt_##SF>, SB, 1 \
			>; \
			break;

		switch (
			  (dst_fmt  << 18)
			+ (dst_bits << 11)
			+ (src_fmt  <<  8)
			+ (src_bits <<  1)
			+ (single_plane_flag ? 1 : 0)
		)
		{
		fmtcl_MatrixProc_SPAN_I (fmtcl_MatrixProc_CASE_INT)
		// No default, format combination is already checked
		// and the C++ code fills all the possibilities.
		}
#undef fmtcl_MatrixProc_CASE_INT
	}
}



// DST and SRC are ProxyRwAvx2 classes
template <class DST, int DB, class SRC, int SB, int NP>
void	MatrixProc::process_n_int_avx2 (Frame <> dst, FrameRO <> src, int w, int h) const noexcept
{
	assert (dst.is_valid (NP         , h));
	assert (src.is_valid (_nbr_planes, h));
	assert (w > 0);
	assert (h > 0);

	static_assert (_nbr_planes == 3, "Code is hardcoded for 3 planes");

	enum { BPS_SRC = (SB + 7) >> 3 };
	enum { BPS_DST = (DB + 7) >> 3 };

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const int      packsize = 16;

	const __m256i  zero     = _mm256_setzero_si256 ();
	const __m256i  mask_lsb = _mm256_set1_epi16 (0x00FF);
	const __m256i  sign_bit = _mm256_set1_epi16 (-0x8000);
	const __m256i  ma       = _mm256_set1_epi16 (int16_t (uint16_t ((1 << DB) - 1)));

	const __m256i* coef_ptr = reinterpret_cast <const __m256i *> (
		_coef_simd_arr.use_vect_avx2 (0)
	);

	for (int y = 0; y < h; ++y)
	{
		// Looping over lines then over planes helps keeping input data
		// in the cache.
		for (int plane_index = 0; plane_index < NP; ++ plane_index)
		{
			SrcPtr         src_0_ptr = SRC::PtrConst::make_ptr (src [0]._ptr, src [0]._stride, h);
			SrcPtr         src_1_ptr = SRC::PtrConst::make_ptr (src [1]._ptr, src [1]._stride, h);
			SrcPtr         src_2_ptr = SRC::PtrConst::make_ptr (src [2]._ptr, src [2]._stride, h);

			DstPtr         dst_ptr   = DST::Ptr::make_ptr (
				dst [plane_index]._ptr, dst [plane_index]._stride, h
			);
			const int      cind = plane_index * _mat_size;

			for (int x = 0; x < w; x += packsize)
			{
				typedef typename SRC::template S16 <false     , (SB == 16)> SrcS16R;
				typedef typename DST::template S16 <(DB != 16), (DB == 16)> DstS16W;

				const __m256i  s0 = SrcS16R::read (src_0_ptr, zero, sign_bit);
				const __m256i  s1 = SrcS16R::read (src_1_ptr, zero, sign_bit);
				const __m256i  s2 = SrcS16R::read (src_2_ptr, zero, sign_bit);

				__m256i        d0 = _mm256_load_si256 (coef_ptr + cind + _nbr_planes);
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

				d0 = _mm256_srai_epi32 (d0, _shift_int + SB - DB);
				d1 = _mm256_srai_epi32 (d1, _shift_int + SB - DB);

				__m256i			val = _mm256_packs_epi32 (d0, d1);

				DstS16W::write_clip (dst_ptr, val, mask_lsb, zero, ma, sign_bit);

				SRC::PtrConst::jump (src_0_ptr, packsize);
				SRC::PtrConst::jump (src_1_ptr, packsize);
				SRC::PtrConst::jump (src_2_ptr, packsize);

				DST::Ptr::jump (dst_ptr, packsize);
			}
		}

		src.step_line ();
		dst.step_line ();
	}

	_mm256_zeroupper ();	// Back to SSE state
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
