/*****************************************************************************

        Transfer.cpp
        Author: Laurent de Soras, 2015

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
#include "fmtc/Transfer.h"
#include "fstb/ToolsAvx2.h"

#include <immintrin.h>

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Transfer::init_proc_fnc_avx2 (int selector)
{
	if (_avx2_flag)
	{
		switch (selector)
		{
		case 0*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_avx2 <          float   , MapperLog>; break;
		case 0*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_avx2 <          float   , MapperLin>; break;
		case 1*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_avx2 <          uint16_t, MapperLog>; break;
		case 1*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_avx2 <          uint16_t, MapperLin>; break;
		case 2*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_avx2 <          uint8_t , MapperLog>; break;
		case 2*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_avx2 <          uint8_t , MapperLin>; break;

		default:
			// Nothing
			break;
		}
	}
}



void	Transfer::MapperLin::find_index (const FloatIntMix val_arr [8], __m256i &index, __m256 &frac)
{
	assert (val_arr != 0);
	assert (&index != 0);
	assert (&frac != 0);

	const __m256   scale     = _mm256_set1_ps (1 << LINLUT_RES_L2);
	const __m256i  offset    =
		_mm256_set1_epi32 (-LINLUT_MIN_F * (1 << LINLUT_RES_L2));
	const __m256i  val_min   = _mm256_setzero_si256 ();
	const __m256i  val_max   = _mm256_set1_epi32 (LINLUT_SIZE_F - 2);

	const __m256   v         =
		_mm256_load_ps (reinterpret_cast <const float *> (val_arr));
	const __m256   val_scl   = _mm256_mul_ps (v, scale);
	const __m256i  index_raw = _mm256_cvtps_epi32 (val_scl);
	__m256i        index_tmp = _mm256_add_epi32 (index_raw, offset);
	index_tmp = _mm256_min_epi32 (index_tmp, val_max);
	index     = _mm256_max_epi32 (index_tmp, val_min);
	frac      = _mm256_sub_ps (val_scl, _mm256_cvtepi32_ps (index_raw));
}



void	Transfer::MapperLog::find_index (const FloatIntMix val_arr [8], __m256i &index, __m256 &frac)
{
	assert (val_arr != 0);
	assert (&index != 0);
	assert (&frac != 0);

	// Constants
	static const int      mant_size = 23;
	static const int      exp_bias  = 127;
	static const uint32_t base      = (exp_bias + LOGLUT_MIN_L2) << mant_size;
	static const float    val_min   = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
	static const float    val_max   = float (int64_t (1) << LOGLUT_MAX_L2);
	static const int      frac_size = mant_size - LOGLUT_RES_L2;
	static const uint32_t frac_mask = (1 << frac_size) - 1;

	const __m256   zero_f     = _mm256_setzero_ps ();
	const __m256   one_f      = _mm256_set1_ps (1);
	const __m256   frac_mul   = _mm256_set1_ps (1.0f / (1 << frac_size));
	const __m256   mul_eps    = _mm256_set1_ps (1.0f / val_min);
	const __m256   mask_abs_f = _mm256_load_ps (
		reinterpret_cast <const float *> (fstb::ToolsAvx2::_mask_abs)
	);

	const __m256i  zero_i          = _mm256_setzero_si256 ();
	const __m256i  mask_abs_epi32  = _mm256_set1_epi32 (0x7FFFFFFF);
	const __m256i  one_epi32       = _mm256_set1_epi32 (1);
	const __m256i  base_epi32      = _mm256_set1_epi32 (int (base));
	const __m256i  frac_mask_epi32 = _mm256_set1_epi32 (frac_mask);
	const __m256i  val_min_epi32   =
		_mm256_set1_epi32 ((LOGLUT_MIN_L2 + exp_bias) << mant_size);
	const __m256i  val_max_epi32   =
		_mm256_set1_epi32 ((LOGLUT_MAX_L2 + exp_bias) << mant_size);
	const __m256i  index_max_epi32 =
		_mm256_set1_epi32 ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2);
	const __m256i  hsize_epi32     = _mm256_set1_epi32 (LOGLUT_HSIZE);
	const __m256i  mirror_epi32    = _mm256_set1_epi32 (LOGLUT_HSIZE - 1);

	// It really starts here
	const __m256   val_f = _mm256_load_ps (reinterpret_cast <const float *> (val_arr));
	const __m256   val_a = _mm256_and_ps (val_f, mask_abs_f);
	const __m256i  val_i = _mm256_load_si256 (reinterpret_cast <const __m256i *> (val_arr));
	const __m256i  val_u = _mm256_and_si256 (val_i, mask_abs_epi32);

	// Standard path
	__m256i        index_std = _mm256_sub_epi32 (val_u, base_epi32);
	index_std = _mm256_srli_epi32 (index_std, frac_size);
	index_std = _mm256_add_epi32 (index_std, one_epi32);
	__m256i        frac_stdi = _mm256_and_si256 (val_u, frac_mask_epi32);
	__m256         frac_std  = _mm256_cvtepi32_ps (frac_stdi);
	frac_std  = _mm256_mul_ps (frac_std, frac_mul);

	// Epsilon path
	__m256         frac_eps  = _mm256_max_ps (val_a, zero_f);
	frac_eps = _mm256_mul_ps (frac_eps, mul_eps);

	// Range cases
	const __m256i  eps_flag_i = _mm256_cmpgt_epi32 (val_min_epi32, val_u);
	const __m256i  std_flag_i = _mm256_cmpgt_epi32 (val_max_epi32, val_u);
	const __m256   eps_flag_f = _mm256_castsi256_ps (eps_flag_i);
	const __m256   std_flag_f = _mm256_castsi256_ps (std_flag_i);
	__m256i        index_tmp  =
		fstb::ToolsAvx2::select (std_flag_i, index_std, index_max_epi32);
	__m256         frac_tmp   =
		fstb::ToolsAvx2::select (std_flag_f, frac_std, one_f);
	index_tmp = fstb::ToolsAvx2::select (eps_flag_i, zero_i, index_tmp);
	frac_tmp  = fstb::ToolsAvx2::select (eps_flag_f, frac_eps, frac_tmp);

	// Sign cases
	const __m256i  neg_flag_i = _mm256_srai_epi32 (val_i, 31);
	const __m256   neg_flag_f = _mm256_castsi256_ps (neg_flag_i);
	const __m256i  index_neg  = _mm256_sub_epi32 (mirror_epi32, index_tmp);
	const __m256i  index_pos  = _mm256_add_epi32 (hsize_epi32, index_tmp);
	const __m256   frac_neg   = _mm256_sub_ps (one_f, frac_tmp);
	index = fstb::ToolsAvx2::select (neg_flag_i, index_neg, index_pos);
	frac  = fstb::ToolsAvx2::select (neg_flag_f, frac_neg, frac_tmp);
}



template <class TD, class M>
void	Transfer::process_plane_flt_any_avx2 (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h)
{
	assert (dst_ptr != 0);
	assert (src_ptr != 0);
	assert (stride_dst != 0);
	assert (stride_src != 0);
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const FloatIntMix *  s_ptr =
			reinterpret_cast <const FloatIntMix *> (src_ptr);
		TD *                 d_ptr =
			reinterpret_cast <               TD *> (dst_ptr);

		for (int x = 0; x < w; x += 8)
		{
			union
			{
				__m256i            _vect;
				uint32_t           _scal [8];
			}                  index;
			__m256             lerp;
			M::find_index (s_ptr + x, index._vect, lerp);
#if 1	// Looks as fast as _mm256_set_ps
			__m256             val = _mm256_i32gather_ps (
				&_lut.use <float> (0), index._vect, sizeof (float)
			);
			const __m256       va2 = _mm256_i32gather_ps (
				&_lut.use <float> (1), index._vect, sizeof (float)
			);
#else
			__m256             val = _mm256_set_ps (
				_lut.use <float> (index._scal [7]    ),
				_lut.use <float> (index._scal [6]    ),
				_lut.use <float> (index._scal [5]    ),
				_lut.use <float> (index._scal [4]    ),
				_lut.use <float> (index._scal [3]    ),
				_lut.use <float> (index._scal [2]    ),
				_lut.use <float> (index._scal [1]    ),
				_lut.use <float> (index._scal [0]    )
			);
			const __m256       va2 = _mm256_set_ps (
				_lut.use <float> (index._scal [7] + 1),
				_lut.use <float> (index._scal [6] + 1),
				_lut.use <float> (index._scal [5] + 1),
				_lut.use <float> (index._scal [4] + 1),
				_lut.use <float> (index._scal [3] + 1),
				_lut.use <float> (index._scal [2] + 1),
				_lut.use <float> (index._scal [1] + 1),
				_lut.use <float> (index._scal [0] + 1)
			);
#endif
			const __m256       dif = _mm256_sub_ps (va2, val);
			val = _mm256_add_ps (val, _mm256_mul_ps (dif, lerp));
			Convert <TD>::store_avx2 (&d_ptr [x], val);
		}

		src_ptr += stride_src;
		dst_ptr += stride_dst;
	}
}



template <class T>
void	Transfer::Convert <T>::store_avx2 (T *dst_ptr, __m256 val)
{
	_mm256_store_si256 (
		reinterpret_cast <__m256i *> (dst_ptr),
		_mm256_cvtps_epi32 (val)
	);
}

template <>
void	Transfer::Convert <float>::store_avx2 (float *dst_ptr, __m256 val)
{
	_mm256_store_ps (dst_ptr, val);
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
