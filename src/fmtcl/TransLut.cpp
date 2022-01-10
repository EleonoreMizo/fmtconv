/*****************************************************************************

        TransLut.cpp
        Author: Laurent de Soras, 2015

To do:
	- Remove code for destination bitdepth < 16

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

#include "fmtcl/Cst.h"
#include "fmtcl/TransLut.h"
#include "fmtcl/TransOpInterface.h"
#include "fstb/fnc.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fstb/ToolsSse2.h"
#endif

#include <algorithm>

#include <cassert>
#include <cmath>
#include <cstdlib>



namespace fmtcl
{



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <class M>
class TransLut_FindIndexSse2
{
public:
	static const int  LINLUT_RES_L2  = TransLut::LINLUT_RES_L2;
	static const int  LINLUT_MIN_F   = TransLut::LINLUT_MIN_F;
	static const int  LINLUT_MAX_F   = TransLut::LINLUT_MAX_F;
	static const int  LINLUT_SIZE_F  = TransLut::LINLUT_SIZE_F;

	static const int  LOGLUT_MIN_L2  = TransLut::LOGLUT_MIN_L2;
	static const int  LOGLUT_MAX_L2  = TransLut::LOGLUT_MAX_L2;
	static const int  LOGLUT_RES_L2  = TransLut::LOGLUT_RES_L2;
	static const int  LOGLUT_HSIZE   = TransLut::LOGLUT_HSIZE;
	static const int  LOGLUT_SIZE    = TransLut::LOGLUT_SIZE;

	static inline void
		            find_index (const TransLut::FloatIntMix val_arr [4], __m128i &index, __m128 &frac) noexcept;
};



template <>
void	TransLut_FindIndexSse2 <TransLut::MapperLin>::find_index (const TransLut::FloatIntMix val_arr [4], __m128i &index, __m128 &frac) noexcept
{
	assert (val_arr != nullptr);
	
	constexpr int  offset    = -LINLUT_MIN_F * (1 << LINLUT_RES_L2);
	const __m128   scale     = _mm_set1_ps (1 << LINLUT_RES_L2);
	const __m128i  offset_ps = _mm_set1_epi32 (offset);
	const __m128   val_min   = _mm_set1_ps (0                 - offset);
	const __m128   val_max   = _mm_set1_ps (LINLUT_SIZE_F - 2 - offset);

	const __m128   v         =
		_mm_load_ps (reinterpret_cast <const float *> (val_arr));
	__m128         val_scl   = _mm_mul_ps (v, scale);
	val_scl = _mm_min_ps (val_scl, val_max);
	val_scl = _mm_max_ps (val_scl, val_min);
	const __m128i  index_raw = _mm_cvtps_epi32 (val_scl);
	index     = _mm_add_epi32 (index_raw, offset_ps);
	frac      = _mm_sub_ps (val_scl, _mm_cvtepi32_ps (index_raw));
}



template <>
void	TransLut_FindIndexSse2 <TransLut::MapperLog>::find_index (const TransLut::FloatIntMix val_arr [4], __m128i &index, __m128 &frac) noexcept
{
	assert (val_arr != nullptr);

	// Constants
	constexpr int        mant_size = 23;
	constexpr int        exp_bias  = 127;
	constexpr uint32_t   base      = (exp_bias + LOGLUT_MIN_L2) << mant_size;
	constexpr float      val_min   = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
//	constexpr float      val_max   = float (int64_t (1) << LOGLUT_MAX_L2);
	constexpr int        frac_size = mant_size - LOGLUT_RES_L2;
	constexpr uint32_t   frac_mask = (1 << frac_size) - 1;

	const __m128   zero_f     = _mm_setzero_ps ();
	const __m128   one_f      = _mm_set1_ps (1);
	const __m128   frac_mul   = _mm_set1_ps (1.0f / (1 << frac_size));
	const __m128   mul_eps    = _mm_set1_ps (1.0f / val_min);
	const __m128   mask_abs_f = _mm_load_ps (
		reinterpret_cast <const float *> (fstb::ToolsSse2::_mask_abs)
	);

	const __m128i  zero_i          = _mm_setzero_si128 ();
	const __m128i  mask_abs_epi32  = _mm_set1_epi32 (0x7FFFFFFF);
	const __m128i  one_epi32       = _mm_set1_epi32 (1);
	const __m128i  base_epi32      = _mm_set1_epi32 (int (base));
	const __m128i  frac_mask_epi32 = _mm_set1_epi32 (frac_mask);
	const __m128i  val_min_epi32   =
		_mm_set1_epi32 ((LOGLUT_MIN_L2 + exp_bias) << mant_size);
	const __m128i  val_max_epi32   =
		_mm_set1_epi32 ((LOGLUT_MAX_L2 + exp_bias) << mant_size);
	const __m128i  index_max_epi32 =
		_mm_set1_epi32 ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2);
	const __m128i  hsize_epi32     = _mm_set1_epi32 (LOGLUT_HSIZE);
	const __m128i  mirror_epi32    = _mm_set1_epi32 (LOGLUT_HSIZE - 1);

	// It really starts here
	const __m128   val_f = _mm_load_ps (reinterpret_cast <const float *> (val_arr));
	const __m128   val_a = _mm_and_ps (val_f, mask_abs_f);
	const __m128i  val_i = _mm_load_si128 (reinterpret_cast <const __m128i *> (val_arr));
	const __m128i  val_u = _mm_and_si128 (val_i, mask_abs_epi32);

	// Standard path
	__m128i        index_std = _mm_sub_epi32 (val_u, base_epi32);
	index_std = _mm_srli_epi32 (index_std, frac_size);
	index_std = _mm_add_epi32 (index_std, one_epi32);
	__m128i        frac_stdi = _mm_and_si128 (val_u, frac_mask_epi32);
	__m128         frac_std  = _mm_cvtepi32_ps (frac_stdi);
	frac_std  = _mm_mul_ps (frac_std, frac_mul);

	// Epsilon path
	__m128         frac_eps  = _mm_max_ps (val_a, zero_f);
	frac_eps = _mm_mul_ps (frac_eps, mul_eps);

	// Range cases
	const __m128i  eps_flag_i = _mm_cmpgt_epi32 (val_min_epi32, val_u);
	const __m128i  std_flag_i = _mm_cmpgt_epi32 (val_max_epi32, val_u);
	const __m128   eps_flag_f = _mm_castsi128_ps (eps_flag_i);
	const __m128   std_flag_f = _mm_castsi128_ps (std_flag_i);
	__m128i        index_tmp  =
		fstb::ToolsSse2::select (std_flag_i, index_std, index_max_epi32);
	__m128         frac_tmp   =
		fstb::ToolsSse2::select (std_flag_f, frac_std, one_f);
	index_tmp = fstb::ToolsSse2::select (eps_flag_i, zero_i, index_tmp);
	frac_tmp  = fstb::ToolsSse2::select (eps_flag_f, frac_eps, frac_tmp);

	// Sign cases
	const __m128i  neg_flag_i = _mm_srai_epi32 (val_i, 31);
	const __m128   neg_flag_f = _mm_castsi128_ps (neg_flag_i);
	const __m128i  index_neg  = _mm_sub_epi32 (mirror_epi32, index_tmp);
	const __m128i  index_pos  = _mm_add_epi32 (hsize_epi32, index_tmp);
	const __m128   frac_neg   = _mm_sub_ps (one_f, frac_tmp);
	index = fstb::ToolsSse2::select (neg_flag_i, index_neg, index_pos);
	frac  = fstb::ToolsSse2::select (neg_flag_f, frac_neg, frac_tmp);
}



template <class T>
static fstb_FORCEINLINE void	TransLut_store_sse2 (T *dst_ptr, __m128 val) noexcept
{
	_mm_store_si128 (
		reinterpret_cast <__m128i *> (dst_ptr),
		_mm_cvtps_epi32 (val)
	);
}

static fstb_FORCEINLINE void	TransLut_store_sse2 (float *dst_ptr, __m128 val) noexcept
{
	_mm_store_ps (dst_ptr, val);
}



#endif   // fstb_ARCHI_X86



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	TransLut::LINLUT_RES_L2;
constexpr int	TransLut::LINLUT_MIN_F;
constexpr int	TransLut::LINLUT_MAX_F;
constexpr int	TransLut::LINLUT_SIZE_F;
constexpr int	TransLut::LOGLUT_MIN_L2;
constexpr int	TransLut::LOGLUT_MAX_L2;
constexpr int	TransLut::LOGLUT_RES_L2;
constexpr int	TransLut::LOGLUT_HSIZE;
constexpr int	TransLut::LOGLUT_SIZE;



TransLut::TransLut (const TransOpInterface &curve, bool log_flag, SplFmt src_fmt, int src_bits, bool src_full_flag, SplFmt dst_fmt, int dst_bits, bool dst_full_flag, bool sse2_flag, bool avx2_flag)
:	_loglut_flag (log_flag)
,	_src_fmt (src_fmt)
,	_src_bits (src_bits)
,	_src_full_flag (src_full_flag)
,	_dst_fmt (dst_fmt)
,	_dst_bits (dst_bits)
,	_dst_full_flag (dst_full_flag)
,	_sse2_flag (sse2_flag)
,	_avx2_flag (avx2_flag)
{
	assert (src_fmt >= 0);
	assert (src_fmt < SplFmt_NBR_ELT);
	assert (src_bits >= 8);
	assert (dst_fmt >= 0);
	assert (dst_fmt < SplFmt_NBR_ELT);
	assert (dst_bits >= 8);

	generate_lut (curve);
	init_proc_fnc ();

	// Mutes unused member variable warning for non-x86 architectures
	fstb::unused (_sse2_flag, _avx2_flag);
}



void	TransLut::process_plane (const Plane <> &dst, const PlaneRO <> &src, int w, int h) const noexcept
{
	assert (dst.is_valid (h));
	assert (src.is_valid (h));
	assert (w > 0);
	assert (h > 0);

	assert (_process_plane_ptr != nullptr);
	(this->*_process_plane_ptr) (dst, src, w, h);
}



TransLut::MapperLin::MapperLin (int lut_size, double range_beg, double range_lst) noexcept
:	_lut_size (lut_size)
,	_range_beg (range_beg)
,	_step ((range_lst - range_beg) / (lut_size - 1))
{
	assert (lut_size >= 2);
	assert (range_beg < range_lst);
}



void	TransLut::MapperLin::find_index (const FloatIntMix &val, int &index, float &frac) noexcept
{
	const float    val_scl   = val._f * (1 << LINLUT_RES_L2);
	const int      index_raw = fstb::floor_int (val_scl);
	constexpr int  offset    = -LINLUT_MIN_F * (1 << LINLUT_RES_L2);
	index = fstb::limit (index_raw + offset, 0, LINLUT_SIZE_F - 2);
	frac  = val_scl - float (index_raw);
}



double	TransLut::MapperLin::find_val (int index) const noexcept
{
	return _range_beg + index * _step;
}



void	TransLut::MapperLog::find_index (const FloatIntMix &val, int &index, float &frac) noexcept
{
	static_assert (LOGLUT_MIN_L2 <= 0, "LOGLUT_MIN_L2 must be negative");
	static_assert (LOGLUT_MAX_L2 >= 0, "LOGLUT_MAX_L2 must be positive");

	constexpr int        mant_size = 23;
	constexpr int        exp_bias  = 127;
	constexpr uint32_t   base      = (exp_bias + LOGLUT_MIN_L2) << mant_size;
	constexpr float      val_min   = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
	constexpr float      val_max   = float (int64_t (1) << LOGLUT_MAX_L2);
	constexpr int        frac_size = mant_size - LOGLUT_RES_L2;
	constexpr uint32_t   frac_mask = (1 << frac_size) - 1;

	const uint32_t val_u = val._i & 0x7FFFFFFF;
	const float    val_a = fabsf (val._f);

	// index is set relatively to the x=0 index...
	if (val_a < val_min)
	{
		index = 0;
		frac  = std::max (val_a, 0.0f) * (1.0f / val_min);
	}
	else if (val_a >= val_max)
	{
		index = ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2);
		frac  = 1;
	}
	else
	{
		index = ((val_u - base) >> frac_size) + 1;
		frac  = float (val_u & frac_mask) * (1.0f / (1 << frac_size));
	}

	// ...and shifted or mirrored depending on the sign
	if (val._f >= 0)
	{
		index += LOGLUT_HSIZE;
	}
	else
	{
		// Because frac cannot be negative, step one index behind.
		index = LOGLUT_HSIZE - 1 - index;
		frac  = 1 - frac;
	}

	assert (index >= 0);
	assert (index < LOGLUT_SIZE - 1);
	assert (frac >= 0);
	assert (frac <= 1);
}



double	TransLut::MapperLog::find_val (int index) const noexcept
{
	assert (index >= 0);
	assert (index < LOGLUT_SIZE);

	static constexpr float   val_min  = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
	static constexpr int     seg_size = 1 << LOGLUT_RES_L2;

	// float is OK because the values are exactly represented in float.
	float          val   = 0;
	int            ind_2 = index - LOGLUT_HSIZE;
	if (ind_2 != 0)
	{
		const int      ind_3     = std::abs (ind_2) - 1;
		const int      log2_part = ind_3 >> LOGLUT_RES_L2;
		const int      seg_part  = ind_3 & (seg_size - 1);
		const float    lerp      = float (seg_part) * (1.0f / seg_size);
		const float    v0        = float (int64_t (1) << log2_part) * val_min;
		val = v0 * (1 + lerp);
		if (ind_2 < 0)
		{
			val = -val;
		}
	}

	return val;
}



// For float input. Only checks the curvature, not the extended range
bool	TransLut::is_loglut_req (const TransOpInterface &curve)
{
	// Delta to compute the slope
	constexpr double  delta = 1.0 / 65536;

	// Slope at 1, for reference
	// Curve may be clipping early because of contrast increase, so we
	// try smaller values
	double         x1 = 1;
	double         s1 = 0;
	do
	{
		const double   v1  = curve (x1);
		const double   v1d = curve (x1 - delta);
		s1 = (v1 - v1d) / delta;
		x1 *= 0.5;
	}
	while (s1 <= 0 && x1 >= 0.01);
	// At this point s1 may still be 0, we will ignore the result.

	// Slope at 0
	const double   v0  = curve (0);
	const double   v0d = curve (0 + delta);
	const double   s0  = (v0d - v0) / delta;
	assert (s0 > 0);

	// Arbitrary factor, seems to work decently
	if (s1 > 0 && s0 >= 50 * s1)
	{
		return true;
	}

	// Slope close to 0
	constexpr double  xs = 1.0 / 4096;
	const double   vsn = curve (xs - delta * 0.5);
	const double   vsp = curve (xs + delta * 0.5);
	const double   ss  = (vsp - vsn) / delta;
	assert (ss > 0);

	if (s0 >= 3 * ss)
	{
		return true;
	}

	return false;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	TransLut::generate_lut (const TransOpInterface &curve)
{
	if (_src_fmt == SplFmt_FLOAT)
	{
		// When the source is float, the LUT output is always float
		// so we can interpolate it easily and obtain the exact values.
		// If the target data type is int, we quantize the interpolated
		// values as a second step.
		_lut.set_type <float> ();

		if (_loglut_flag)
		{
			_lut.resize (LOGLUT_SIZE);
			MapperLog   mapper;
			generate_lut_flt <float> (curve, mapper);
		}
		else
		{
			_lut.resize (LINLUT_SIZE_F);
			MapperLin   mapper (LINLUT_SIZE_F, LINLUT_MIN_F, LINLUT_MAX_F);
			generate_lut_flt <float> (curve, mapper);
		}
	}

	else
	{
		_loglut_flag = false;

		int            range = 1 << _src_bits;
		if (_src_fmt == SplFmt_INT8)
		{
			_lut.resize (1 << 8);
		}
		else
		{
			_lut.resize (1 << 16);
		}
		const int      sb16  = (_src_full_flag) ? 0      : Cst::_rtv_lum_blk << 8;
		const int      sw16  = (_src_full_flag) ? 0xFFFF : Cst::_rtv_lum_wht << 8;
		int            sbn   = sb16 >> (16 - _src_bits);
		int            swn   = sw16 >> (16 - _src_bits);
		const int      sdif  = swn - sbn;
		const double   r_beg = double (0         - sbn) / sdif;
		const double   r_lst = double (range - 1 - sbn) / sdif;
		if (_dst_fmt == SplFmt_FLOAT)
		{
			_lut.set_type <float> ();
			MapperLin      mapper (range, r_beg, r_lst);
			generate_lut_flt <float> (curve, mapper);
		}
		else
		{
			const int      db16 = (_dst_full_flag) ? 0      : Cst::_rtv_lum_blk << 8;
			const int      dw16 = (_dst_full_flag) ? 0xFFFF : Cst::_rtv_lum_wht << 8;
			int            dbn  = db16 >> (16 - _dst_bits);
			int            dwn  = dw16 >> (16 - _dst_bits);
			const double   mul  = dwn - dbn;
			const double   add  = dbn;
			if (_dst_bits > 8)
			{
				_lut.set_type <uint16_t> ();
				generate_lut_int <uint16_t> (
					curve, range, r_beg, r_lst, mul, add
				);
			}
			else
			{
				_lut.set_type <uint8_t> ();
				generate_lut_int <uint8_t> (
					curve, range, r_beg, r_lst, mul, add
				);
			}
		}
	}
}



// T = LUT data type (int or float)
template <class T>
void	TransLut::generate_lut_int (const TransOpInterface &curve, int lut_size, double range_beg, double range_lst, double mul, double add)
{
	assert (_dst_fmt != SplFmt_FLOAT);
	assert (lut_size > 1);
	assert (range_beg < range_lst);

	const double   scale   = (range_lst - range_beg) / (lut_size - 1);
	const int      max_val = (1 << _dst_bits) - 1;
	for (int pos = 0; pos < lut_size; ++pos)
	{
		const double   x = range_beg + pos * scale;
		const double   y = curve (x) * mul + add;
		_lut.use <T> (pos) = T (fstb::limit (fstb::round_int (y), 0, max_val));
	}
}



// T = float
template <class T, class M>
void	TransLut::generate_lut_flt (const TransOpInterface &curve, const M &mapper)
{
	const int      lut_size = mapper.get_lut_size ();
	for (int pos = 0; pos < lut_size; ++pos)
	{
		const double   x = mapper.find_val (pos);
		const double   y = curve (x);
		_lut.use <T> (pos) = T (y);
	}
}



void	TransLut::init_proc_fnc ()
{
	assert (! _loglut_flag || _src_fmt == SplFmt_FLOAT);

	const int      s =
		  (_loglut_flag            ) ? 0
		: (_src_fmt == SplFmt_FLOAT) ? 1
		: (_src_bits > 8           ) ? 2
		:                              3;
	const int      d =
		  (_dst_fmt == SplFmt_FLOAT) ? 0
		: (_dst_bits > 8           ) ? 1
		:                              2;

	const int      selector = d * 4 + s;

	switch (selector)
	{
	case 0*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          float   , MapperLog>; break;
	case 0*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          float   , MapperLin>; break;
	case 0*4+2:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint16_t, float              >; break;
	case 0*4+3:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint8_t , float              >; break;
	case 1*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint16_t, MapperLog>; break;
	case 1*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint16_t, MapperLin>; break;
	case 1*4+2:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint16_t, uint16_t           >; break;
	case 1*4+3:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint8_t , uint16_t           >; break;
	case 2*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint8_t , MapperLog>; break;
	case 2*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint8_t , MapperLin>; break;
	case 2*4+2:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint16_t, uint8_t            >; break;
	case 2*4+3:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint8_t , uint8_t            >; break;

	default:
		assert (false);
		break;
	}
#if (fstb_ARCHI == fstb_ARCHI_X86)
	init_proc_fnc_sse2 (selector);
	init_proc_fnc_avx2 (selector);
#endif
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

void	TransLut::init_proc_fnc_sse2 (int selector)
{
	if (_sse2_flag && _src_fmt == SplFmt_FLOAT)
	{
		switch (selector)
		{
		case 0*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <float   , MapperLog>; break;
		case 0*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <float   , MapperLin>; break;
		case 1*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <uint16_t, MapperLog>; break;
		case 1*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <uint16_t, MapperLin>; break;
		case 2*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <uint8_t , MapperLog>; break;
		case 2*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <uint8_t , MapperLin>; break;

		default:
			// Nothing
			break;
		}
	}
}

#endif   // fstb_ARCHI_X86



template <class TS, class TD>
void	TransLut::process_plane_int_any_cpp (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept
{
	assert (dst.is_valid (h));
	assert (src.is_valid (h));
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const PlaneRO <TS>   s { src };
		const Plane <TD>     d { dst };

		for (int x = 0; x < w; ++x)
		{
			const int          index = s._ptr [x];
			d._ptr [x] = _lut.use <TD> (index);
		}

		src.step_line ();
		dst.step_line ();
	}
}



template <class TD, class M>
void	TransLut::process_plane_flt_any_cpp (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept
{
	assert (dst.is_valid (h));
	assert (src.is_valid (h));
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const PlaneRO <FloatIntMix>   s { src };
		const Plane <TD>              d { dst };

		for (int x = 0; x < w; ++x)
		{
			int                index;
			float              lerp;
			M::find_index (s._ptr [x], index, lerp);
			const float        p_0  = _lut.use <float> (index    );
			const float        p_1  = _lut.use <float> (index + 1);
			const float        dif  = p_1 - p_0;
			const float        val  = p_0 + lerp * dif;
			d._ptr [x] = Convert <TD>::cast (val);
		}

		src.step_line ();
		dst.step_line ();
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <class TD, class M>
void	TransLut::process_plane_flt_any_sse2 (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept
{
	assert (dst.is_valid (h));
	assert (src.is_valid (h));
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const PlaneRO <FloatIntMix>   s { src };
		const Plane <TD>              d { dst };

		for (int x = 0; x < w; x += 4)
		{
			union
			{
				__m128i            _vect;
				uint32_t           _scal [4];
			}                  index;
			__m128             lerp;
			TransLut_FindIndexSse2 <M>::find_index (s._ptr + x, index._vect, lerp);
			__m128             val = _mm_set_ps (
				_lut.use <float> (index._scal [3]    ),
				_lut.use <float> (index._scal [2]    ),
				_lut.use <float> (index._scal [1]    ),
				_lut.use <float> (index._scal [0]    )
			);
			__m128             va2 = _mm_set_ps (
				_lut.use <float> (index._scal [3] + 1),
				_lut.use <float> (index._scal [2] + 1),
				_lut.use <float> (index._scal [1] + 1),
				_lut.use <float> (index._scal [0] + 1)
			);
			const __m128       dif = _mm_sub_ps (va2, val);
			val = _mm_add_ps (val, _mm_mul_ps (dif, lerp));
			TransLut_store_sse2 (&d._ptr [x], val);
		}

		src.step_line ();
		dst.step_line ();
	}
}



#endif



template <class T>
T	TransLut::Convert <T>::cast (float val) noexcept
{
	return T (fstb::conv_int_fast (val));
}

template <>
float	TransLut::Convert <float>::cast (float val) noexcept
{
	return val;
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
