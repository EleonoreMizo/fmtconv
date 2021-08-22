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
#include "fstb/def.h"

#include <immintrin.h>

#include <algorithm>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	MatrixProc::setup_fnc_avx (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag)
{
	fstb::unused (src_fmt, src_bits, dst_fmt, dst_bits);

	if (! int_proc_flag)
	{
		if (single_plane_flag)
		{
			_proc_ptr = &ThisType::process_1_flt_avx;
		}
		else
		{
			_proc_ptr = &ThisType::process_3_flt_avx;
		}
	}
}



void	MatrixProc::process_3_flt_avx (Frame <> dst, FrameRO <> src, int w, int h) const noexcept
{
	assert (dst.is_valid (_nbr_planes, h));
	assert (src.is_valid (_nbr_planes, h));
	assert (w > 0);
	assert (h > 0);

	static_assert (_nbr_planes == 3, "Code is hardcoded for 3 planes");

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
		const FrameRO <float>   s { src };
		const Frame <float>     d { dst };

		for (int x = 0; x < w; x += 8)
		{
			const __m256   s0 = _mm256_load_ps (s [0]._ptr + x);
			const __m256   s1 = _mm256_load_ps (s [1]._ptr + x);
			const __m256   s2 = _mm256_load_ps (s [2]._ptr + x);

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

			_mm256_store_ps (d [0]._ptr + x, d0);
			_mm256_store_ps (d [1]._ptr + x, d1);
			_mm256_store_ps (d [2]._ptr + x, d2);
		}

		src.step_line ();
		dst.step_line ();
	}

	_mm256_zeroupper ();	// Back to SSE state
}



void	MatrixProc::process_1_flt_avx (Frame <> dst, FrameRO <> src, int w, int h) const noexcept
{
	assert (dst.is_valid (          1, h));
	assert (src.is_valid (_nbr_planes, h));
	assert (w > 0);
	assert (h > 0);

	static_assert (_nbr_planes == 3, "Code is hardcoded for 3 planes");

	const __m256   c00 = _mm256_set1_ps (_coef_flt_arr [ 0]);
	const __m256   c01 = _mm256_set1_ps (_coef_flt_arr [ 1]);
	const __m256   c02 = _mm256_set1_ps (_coef_flt_arr [ 2]);
	const __m256   c03 = _mm256_set1_ps (_coef_flt_arr [ 3]);

	for (int y = 0; y < h; ++y)
	{
		const FrameRO <float>   s { src };
		const Plane <float>     d { dst [0] };

		for (int x = 0; x < w; x += 4)
		{
			const __m256   s0 = _mm256_load_ps (s [0]._ptr + x);
			const __m256   s1 = _mm256_load_ps (s [1]._ptr + x);
			const __m256   s2 = _mm256_load_ps (s [2]._ptr + x);

			const __m256   d0 = _mm256_add_ps (_mm256_add_ps (_mm256_add_ps (
				_mm256_mul_ps (s0, c00),
				_mm256_mul_ps (s1, c01)),
				_mm256_mul_ps (s2, c02)),
				                c03);

			_mm256_store_ps (d._ptr + x, d0);
		}

		src.step_line ();
		dst [0].step_line ();
	}

	_mm256_zeroupper ();	// Back to SSE state
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
