/*****************************************************************************

        Scaler.cpp
        Author: Laurent de Soras, 2011

Optimizations TO DO:
- Support for low bitdepth output? Would need clipping to logical limits.

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
#include "fmtcl/ContFirInterface.h"
#include "fmtcl/ProxyRwCpp.h"
#include "fmtcl/Scaler.h"
#include "fmtcl/ScalerCopy.h"
#include "fstb/fnc.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fmtcl/ProxyRwSse2.h"
	#include "fmtcl/ReadWrapperFlt.h"
	#include "fmtcl/ReadWrapperInt.h"
	#include "fstb/ToolsSse2.h"
#endif   // fstb_ARCHI_X86

#include <algorithm>

#include <cassert>
#include <climits>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#define fmtcl_Scaler_INIT_F_CPP(DT, ST, DE, SE, FN) \
,	_process_plane_flt_##FN##_ptr (&ThisType::process_plane_flt_cpp <ProxyRwCpp <SplFmt_##DE>, ProxyRwCpp <SplFmt_##SE> >)

#define fmtcl_Scaler_INIT_F_SSE(DT, ST, DE, SE, FN) \
	_process_plane_flt_##FN##_ptr = &ThisType::process_plane_flt_sse2 <ProxyRwSse2 <SplFmt_##DE>, ProxyRwSse2 <SplFmt_##SE> >;

#define fmtcl_Scaler_INIT_I_CPP(DT, ST, DE, SE, DB, SB, FN) \
,	_process_plane_int_##FN##_ptr (&ThisType::process_plane_int_cpp <ProxyRwCpp <SplFmt_##DE>, DB, ProxyRwCpp <SplFmt_##SE>, SB>)

#define fmtcl_Scaler_INIT_I_SSE2(DT, ST, DE, SE, DB, SB, FN) \
	_process_plane_int_##FN##_ptr = &ThisType::process_plane_int_sse2 <ProxyRwSse2 <SplFmt_##DE>, DB, ProxyRwSse2 <SplFmt_##SE>, SB>;

/*
gain and add_cst are MAC constants to match different bitdepths and ranges.
When scaling in integer, the bitdepth difference is handled with internal
shifting, so these constant should not contain bitdepth scaling.

All paths:
- Coefficients are multiplied with the _gain constant during precalculation.

Float path (SSE/SSE2)
- Possible straight int to float conversion done by the read proxy. Data
	are not scaled here and remain unsigned.
- Convolution summing to _add_cst_flt.
- Possible straigth saturated float to unsigned int conversion done by the
	write proxy. 2-byte bitdepths below 16 bits are saturated to 16 bits, not
	to their logical limits.

Integer path (SSE2):
- Data are read by the proxy. 16-bit data have their 15th bit flipped to make
	them signed, lower bitdepth data remain unsigned.
- Convolution products are done in 32 bits signed with signed input at its
	natural depth and coefficients scaled to 12 bits (SHIFT_INT)
- The convolution is summed to _add_cst_int, then scaled down from
	SHIFT_INT + the in/out bitdepth difference.
- 16-bit data have their 15th bit flipped back to make them unsigned by the
	write proxy. 2-byte bitdepths below 16 bits are not saturated to their
	logical limits.
*/

Scaler::Scaler (int src_height, int dst_height, double win_top, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, bool norm_flag, double norm_val, double center_pos_src, double center_pos_dst, double gain, double add_cst, bool int_flag, bool sse2_flag, bool avx2_flag)
:	_src_height (src_height)
,	_dst_height (dst_height)
,	_win_top (win_top)
,	_win_height (win_height)
,	_kernel_scale (kernel_scale)
,	_kernel_fnc (kernel_fnc)
,	_can_int_flag (int_flag)
,	_norm_flag (norm_flag)
,	_norm_val (norm_val)
,	_center_pos_src (center_pos_src)
,	_center_pos_dst (center_pos_dst)
,	_gain (gain)
,	_add_cst_flt (add_cst)
,	_add_cst_int (fstb::round_int (
#if defined (fmtcl_Scaler_SSE2_16BITS)
		add_cst / (1 << (16 - SHIFT_INT))
#else
		add_cst * (1 << SHIFT_INT)
#endif
	))
,	_fir_len (0)
,	_kernel_info_arr (dst_height)
,	_coef_flt_arr ()
,	_coef_int_arr ()
fmtcl_Scaler_SPAN_F (fmtcl_Scaler_INIT_F_CPP)
fmtcl_Scaler_SPAN_I (fmtcl_Scaler_INIT_I_CPP)
{
	assert (src_height > 0);
	assert (dst_height > 0);
	assert (win_height > 0);
	assert (kernel_scale > 0);
	assert (! fstb::is_null (gain));

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (sse2_flag)
	{
		fmtcl_Scaler_SPAN_F (fmtcl_Scaler_INIT_F_SSE)
		fmtcl_Scaler_SPAN_I (fmtcl_Scaler_INIT_I_SSE2)

		if (avx2_flag)
		{
			_coef_int_arr.set_avx2_mode (true);
			setup_avx2 ();
		}
	}
#else
	fstb::unused (sse2_flag, avx2_flag);
#endif

	build_scale_data ();
}

#undef fmtcl_Scaler_INIT_F_CPP
#undef fmtcl_Scaler_INIT_F_SSE
#undef fmtcl_Scaler_INIT_I_CPP
#undef fmtcl_Scaler_INIT_I_SSE2



void	Scaler::get_src_boundaries (int &y_src_beg, int &y_src_end, int y_dst_beg, int y_dst_end) const
{
	assert (_fir_len > 0);
	assert (y_dst_beg >= 0);
	assert (y_dst_beg < y_dst_end);
	assert (y_dst_end <= _dst_height);

	y_src_beg = INT_MAX;
	y_src_end = INT_MIN;

	const int       scan_len = std::min (_fir_len, y_dst_end - y_dst_beg);
	for (int l = 0; l < scan_len; ++l)
	{
		const KernelInfo &   ki_beg = _kernel_info_arr [y_dst_beg     + l];
		const KernelInfo &   ki_end = _kernel_info_arr [y_dst_end - 1 - l];

	  y_src_beg = std::min (y_src_beg, ki_beg._start_line                      );
	  y_src_end = std::max (y_src_end, ki_end._start_line + ki_end._kernel_size);
	}

	assert (y_src_beg >= 0);
	assert (y_src_beg < y_src_end);
	assert (y_src_end <= _src_height);
}



int	Scaler::get_fir_len () const
{
	return (_fir_len);
}



// src_ptr is the top-left corner of the full source frame
// dst_ptr is the top-left corner of the destination tile
#define fmtcl_Scaler_DEFINE_F(DT, ST, DE, SE, FN) \
void	Scaler::process_plane_flt (Proxy::Ptr##DT::Type dst_ptr, Proxy::Ptr##ST##Const::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const	\
{	\
	(this->*_process_plane_flt_##FN##_ptr) (	\
		dst_ptr, src_ptr, dst_stride, src_stride, width, y_dst_beg, y_dst_end	\
	);	\
}

#define fmtcl_Scaler_DEFINE_I(DT, ST, DE, SE, DB, SB, FN) \
void	Scaler::process_plane_int_##FN (Proxy::Ptr##DT::Type dst_ptr, Proxy::Ptr##ST##Const::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const	\
{	\
	(this->*_process_plane_int_##FN##_ptr) (	\
		dst_ptr, src_ptr, dst_stride, src_stride, width, y_dst_beg, y_dst_end	\
	);	\
}

fmtcl_Scaler_SPAN_F (fmtcl_Scaler_DEFINE_F)
fmtcl_Scaler_SPAN_I (fmtcl_Scaler_DEFINE_I)

#undef fmtcl_Scaler_DEFINE_F
#undef fmtcl_Scaler_DEFINE_I



void	Scaler::eval_req_src_area (int &work_top, int &work_height, int src_height, int dst_height, double win_top, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, double center_pos_src, double center_pos_dst)
{
	assert (src_height > 0);
	assert (dst_height > 0);
	assert (win_height > 0);
	assert (kernel_scale > 0);

	const BasicInfo   bi (
		src_height, dst_height, win_top, win_height,
		kernel_fnc, kernel_scale, center_pos_src, center_pos_dst
	);

	double         pos_flt = bi._src_pos + bi._support - bi._fir_len + 1;

	int            src_pos_beg = fstb::floor_int (pos_flt);
	src_pos_beg = fstb::limit (src_pos_beg, 0, src_height - 1);

	pos_flt += (dst_height - 1) * bi._src_step;

	int            src_pos_end = fstb::floor_int (pos_flt + bi._fir_len);
	src_pos_end = fstb::limit (src_pos_end, 1, src_height);

	work_top    = src_pos_beg;
	work_height = src_pos_end - src_pos_beg;
}



int	Scaler::eval_lower_bound_of_dst_tile_height (int tile_height_src, int dst_height, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, int src_height)
{
	assert (tile_height_src > 0);
	assert (dst_height > 0);
	assert (win_height > 0);
//	assert (win_height >= tile_height_src);
	assert (kernel_scale > 0);
	assert (src_height > 0);

	int            tile_height_dst = 0;
	if (tile_height_src >= src_height)
	{
		tile_height_dst = dst_height;
	}

	else
	{
		const BasicInfo   bi (
			fstb::ceil_int (win_height), dst_height, 0, win_height,
			kernel_fnc, kernel_scale, 0, 0
		);

		// Technically this should be "- bi._fir_len + 1" but we have use
		// the minimum possible value.
		const int      real_height_src = tile_height_src - bi._fir_len;
		const double   tile_height_dst_flt = real_height_src / bi._src_step;
		tile_height_dst = fstb::floor_int (tile_height_dst_flt);
	}

	return (tile_height_dst);
}



int	Scaler::eval_lower_bound_of_src_tile_height (int tile_height_dst, int dst_height, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, int src_height)
{
	assert (tile_height_dst > 0);
	assert (dst_height > 0);
//	assert (dst_height >= tile_height_dst);
	assert (win_height > 0);
	assert (kernel_scale > 0);

	const BasicInfo   bi (
		fstb::ceil_int (win_height), dst_height, 0, win_height,
		kernel_fnc, kernel_scale, 0, 0
	);

	const int      h = fstb::ceil_int (tile_height_dst * bi._src_step);
	int            tile_height_src = h + bi._fir_len - 1;

	const int      y_src_beg = fstb::limit (
		fstb::floor_int (bi._src_pos + bi._support) - bi._fir_len + 1,
		0,
		src_height - 1
	);
	const int      y_src_end = fstb::limit (
		fstb::floor_int (bi._src_pos + bi._src_step * dst_height + bi._support) + 1,
		1,
		src_height
	);
	const int      max_src_height = y_src_end - y_src_beg;

	tile_height_src = std::min (tile_height_src, max_src_height);

	return (tile_height_src);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// DST and SRC are ProxyRwCpp classes
// Stride offsets in pixels
template <class DST, class SRC>
void	Scaler::process_plane_flt_cpp (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (dst_stride != 0);
	assert (width > 0);
	assert (y_dst_beg >= 0);
	assert (y_dst_beg < y_dst_end);
	assert (y_dst_end <= _dst_height);
	assert (width <= dst_stride);
	assert (width <= src_stride);

	const float    add_cst = float (_add_cst_flt);

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
			for (int x = 0; x < width; x += 2)
			{
				float          sum0 = add_cst;
				float          sum1 = add_cst;

				typename SRC::PtrConst::Type	pix_ptr = col_src_ptr;
				for (int k = 0; k < kernel_size; ++k)
				{
					const float    coef = coef_base_ptr [k];
					float          src0;
					float          src1;
					SRC::read (pix_ptr, src0, src1);
					sum0 += src0 * coef;
					sum1 += src1 * coef;

					SRC::PtrConst::jump (pix_ptr, src_stride);
				}

				DST::write (col_dst_ptr, sum0, sum1);

				DST::Ptr::jump (col_dst_ptr, 2);
				SRC::PtrConst::jump (col_src_ptr, 2);
			}
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



template <class DST, int DB, class SRC, int SB>
void	Scaler::process_plane_int_cpp (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const
{
	assert (DST::Ptr::check_ptr (dst_ptr));
	assert (SRC::PtrConst::check_ptr (src_ptr));
	assert (dst_stride != 0);
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

	const int      add_cst  = _add_cst_int + s_cst + r_cst;

	for (int y = y_dst_beg; y < y_dst_end; ++y)
	{
		const KernelInfo& kernel_info   = _kernel_info_arr [y];
		const int         kernel_size   = kernel_info._kernel_size;
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
			for (int x = 0; x < width; ++x)
			{
				int            sum = add_cst;

				typename SRC::PtrConst::Type	pix_ptr = col_src_ptr;
				for (int k = 0; k < kernel_size; ++k)
				{
					int            src  = SRC::read (pix_ptr);
					const int      coef =
						_coef_int_arr.get_coef (kernel_info._coef_index + k);
					sum += src * coef;

					SRC::PtrConst::jump (pix_ptr, src_stride);
				}

				sum >>= SHIFT_INT + SB - DB;

				DST::template write_clip <DB> (col_dst_ptr, sum);

				DST::Ptr::jump (col_dst_ptr, 1);
				SRC::PtrConst::jump (col_src_ptr, 1);
			}
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <class SRC, bool PF>
static fstb_FORCEINLINE void	Scaler_process_vect_flt_sse2 (__m128 &sum0, __m128 &sum1, int kernel_size, const float *coef_base_ptr, typename SRC::PtrConst::Type pix_ptr, const __m128i &zero, ptrdiff_t src_stride, const __m128 &add_cst, int len)
{
	// Possible optimization: initialize the sum with DST::OFFSET + _add_cst_flt
	// and save the add in the write proxy.
	sum0 = add_cst;
	sum1 = add_cst;

	for (int k = 0; k < kernel_size; ++k)
	{
		__m128         coef = _mm_load_ss (coef_base_ptr + k);
		coef = _mm_shuffle_ps (coef, coef, 0);
		__m128         src0;
		__m128         src1;
		ReadWrapperFlt <SRC, PF>::read (pix_ptr, src0, src1, zero, len);
		const __m128   val0 = _mm_mul_ps (src0, coef);
		const __m128   val1 = _mm_mul_ps (src1, coef);
		sum0 = _mm_add_ps (sum0, val0);
		sum1 = _mm_add_ps (sum1, val1);

		SRC::PtrConst::jump (pix_ptr, src_stride);
	}
}



// DST and SRC are ProxyRwSse2 classes
// Stride offsets in pixels
// Source pointer may be unaligned.
template <class DST, class SRC>
void	Scaler::process_plane_flt_sse2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const
{
	assert (DST::Ptr::check_ptr (dst_ptr, DST::ALIGN_W));
	assert (SRC::PtrConst::check_ptr (src_ptr, SRC::ALIGN_R));
	// When the destination is a buffer:
	// mod4 is enough to guarantee alignment, but since we process pairs of
	// vectors and write_partial() is not different from write() with float
	// data (overwriting all the 32 bytes), we must take extra-care not to
	// overflow from the output buffer.
	// When the destination is not a buffer (output frame data), data may be
	// unaligned anyway. (TO DO: check the algorithm and make sure this
	// constraint is actual).
	assert ((dst_stride & 7) == 0);	
	assert ((src_stride & 3) == 0);
	assert (width > 0);
	assert (y_dst_beg >= 0);
	assert (y_dst_beg < y_dst_end);
	assert (y_dst_end <= _dst_height);
	assert (width <= dst_stride);
	assert (width <= src_stride);

	const __m128i  zero     = _mm_setzero_si128 ();
	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit = _mm_set1_epi16 (-0x8000);
	const __m128   offset   = _mm_set1_ps (float (DST::OFFSET));
	const __m128   add_cst  = _mm_set1_ps (float (_add_cst_flt));

	const int      w8 = width & -8;
	const int      w7 = width - w8;

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
			__m128         sum0;
			__m128         sum1;

			for (int x = 0; x < w8; x += 8)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				Scaler_process_vect_flt_sse2 <SRC, false> (
					sum0, sum1, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, add_cst, 0
				);
				DST::write_flt (
					col_dst_ptr, sum0, sum1, mask_lsb, sign_bit, offset
				);

				DST::Ptr::jump (col_dst_ptr, 8);
				SRC::PtrConst::jump (col_src_ptr, 8);
			}

			if (w7 > 0)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				Scaler_process_vect_flt_sse2 <SRC, true> (
					sum0, sum1, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, add_cst, w7
				);
				DST::write_flt_partial (
					col_dst_ptr, sum0, sum1, mask_lsb, sign_bit, offset, w7
				);
			}
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



template <class DST, int DB, class SRC, int SB, bool PF>
static fstb_FORCEINLINE __m128i	Scaler_process_vect_int_sse2 (const __m128i &add_cst, int kernel_size, const __m128i coef_base_ptr [], typename SRC::PtrConst::Type pix_ptr, const __m128i &zero, ptrdiff_t src_stride, const __m128i &sign_bit, int len)
{
	typedef typename SRC::template S16 <false, (SB == 16)> SrcS16R;

#if defined (fmtcl_Scaler_SSE2_16BITS)

	static_assert ((DB >= SB), "Output bitdepth must be greater or equal to input.");

	__m128i        val = add_cst;

	for (int k = 0; k < kernel_size; ++k)
	{
		__m128i        coef = _mm_load_si128 (coef_base_ptr + k);
		__m128i        src  = ReadWrapperInt <SRC, SrcS16R, PF>::read (
			pix_ptr, zero, sign_bit, len
		);
		if (DB > SB)
		{
			src = _mm_slli_epi16 (src, DB - SB);
			if (DB == 16)
			{
				src = _mm_xor_si128 (src, sign_bit);
			}
		}
		const __m128i  mul = _mm_mulhi_epi16 (src, coef);
		val = _mm_adds_epi16 (val, mul);

		SRC::PtrConst::jump (pix_ptr, src_stride);
	}

	if (SHIFT_INT == 14)
	{
		val = _mm_adds_epi16 (val, val);
		val = _mm_adds_epi16 (val, val);
	}
	else
	{
		// Beware: the shift does not saturate, the result may overflow.
		val = _mm_slli_epi16 (val, 16 - Scaler::SHIFT_INT);
	}

#else    // fmtcl_Scaler_SSE2_16BITS

	__m128i        sum0 = add_cst;
	__m128i        sum1 = add_cst;

	for (int k = 0; k < kernel_size; ++k)
	{
		const __m128i  coef = _mm_load_si128 (coef_base_ptr + k);
		const __m128i  src  = ReadWrapperInt <SRC, SrcS16R, PF>::read (
			pix_ptr, zero, sign_bit, len
		);
		fstb::ToolsSse2::mac_s16_s16_s32 (sum0, sum1, src, coef);

		SRC::PtrConst::jump (pix_ptr, src_stride);
	}

	sum0 = _mm_srai_epi32 (sum0, Scaler::SHIFT_INT + SB - DB);
	sum1 = _mm_srai_epi32 (sum1, Scaler::SHIFT_INT + SB - DB);

	const __m128i  val = _mm_packs_epi32 (sum0, sum1);

#endif   // fmtcl_Scaler_SSE2_16BITS

	return (val);
}



template <class DST, int DB, class SRC, int SB>
void	Scaler::process_plane_int_sse2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, ptrdiff_t dst_stride, ptrdiff_t src_stride, int width, int y_dst_beg, int y_dst_end) const
{
	assert (_can_int_flag);
	assert (DST::Ptr::check_ptr (dst_ptr, DST::ALIGN_W));
	assert (SRC::PtrConst::check_ptr (src_ptr, SRC::ALIGN_R));
	assert ((dst_stride & 7) == 0);	
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
#if defined (fmtcl_Scaler_SSE2_16BITS)
	// With 16-bit SSE2, we always sign the input internally because
	// we scale it to 16 bits before any operation.
	const int      s_in     = (DB == 16) ? 0 : (SB < 16) ? -0x8000 >> (16 - (SHIFT_INT + SB - DB)) : 0;
	const int      s_out    =                  (DB < 16) ? +0x8000 >> (16 - (SHIFT_INT + SB - DB)) : 0;
#else
	const int      s_in     = (SB < 16) ? -(0x8000 << (SHIFT_INT + SB - DB)) : 0;
	const int      s_out    = (DB < 16) ?   0x8000 << (SHIFT_INT + SB - DB)  : 0;
#endif
	const int      s_cst    = s_in + s_out;

	const __m128i  zero     = _mm_setzero_si128 ();
	const __m128i  mask_lsb = _mm_set1_epi16 (0x00FF);
	const __m128i  sign_bit = _mm_set1_epi16 (-0x8000);
	const __m128i  ma       = _mm_set1_epi16 (int16_t (uint16_t ((1 << DB) - 1)));
#if defined (fmtcl_Scaler_SSE2_16BITS)
	const __m128i  add_cst  = _mm_set1_epi16 (_add_cst_int + s_cst        );
#else
	const __m128i  add_cst  = _mm_set1_epi32 (_add_cst_int + s_cst + r_cst);
#endif

	const int      w8 = width & -8;
	const int      w7 = width - w8;

	for (int y = y_dst_beg; y < y_dst_end; ++y)
	{
		const KernelInfo&    kernel_info   = _kernel_info_arr [y];
		const int            kernel_size   = kernel_info._kernel_size;
		const int            ofs_y         = kernel_info._start_line;
		const __m128i *      coef_base_ptr = reinterpret_cast <const __m128i *> (
			_coef_int_arr.use_vect_sse2 (kernel_info._coef_index)
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

			for (int x = 0; x < w8; x += 8)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				const __m128i  val = Scaler_process_vect_int_sse2 <
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

				DST::Ptr::jump (col_dst_ptr, 8);
				SRC::PtrConst::jump (col_src_ptr, 8);
			}

			if (w7 > 0)
			{
				typename SRC::PtrConst::Type  pix_ptr = col_src_ptr;

				const __m128i  val = Scaler_process_vect_int_sse2 <
					DST, DB, SRC, SB, true
				> (
					add_cst, kernel_size, coef_base_ptr,
					pix_ptr, zero, src_stride, sign_bit, w7
				);

				DstS16W::write_clip_partial (
					col_dst_ptr,
					val,
					mask_lsb,
					zero,
					ma,
					sign_bit,
					w7
				);
			}
		}

		DST::Ptr::jump (dst_ptr, dst_stride);
	}
}



#endif   // fstb_ARCHI_X86



void	Scaler::build_scale_data ()
{
	_coef_flt_arr.clear ();
	_coef_int_arr.clear ();

	BasicInfo      bi (
		_src_height, _dst_height, _win_top, _win_height,
		_kernel_fnc, _kernel_scale, _center_pos_src, _center_pos_dst
	);

	_fir_len = bi._fir_len;

	class ValPos { public: int _val; int _idx; };
	std::vector <ValPos> coef_rank;

	std::vector <double>	coef_tmp;
	const int      last_line = _src_height - 1;

	for (int y = 0; y < _dst_height; ++y)
	{
		// First pass: collects the coefficients and compute their sum to
		// normalize the impulse energy.
		coef_tmp.clear ();

		int            src_pos_beg =
			fstb::floor_int (bi._src_pos + bi._support) - bi._fir_len + 1;
		double         sum = 0;
		for (int k = 0; k < bi._fir_len; ++k)
		{
			const int      p          = src_pos_beg + k;
			const double   pos_in_fir = (bi._src_pos - p) * bi._imp_step;
			const double   val        = _kernel_fnc.get_val (pos_in_fir);

			coef_tmp.push_back (val);
			sum += val;
		}

		double         amp = 1;
		if (_norm_flag)
		{
			double         n = sum;
			if (_norm_val > 0)
			{
				n = _norm_val;
			}
			if (fstb::is_null (n))
			{
				// This shouldn't happen in normal use. However with an extremely
				// sharp gaussian kernel, it is possible that the kernel is made
				// of just a single tiny coefficient. This test should be able to
				// make it work as expected. However a simple "box" filter is more
				// suited for this kind of use.
				if (coef_tmp.size () == 1)
				{
					coef_tmp [0] = std::copysign (1.0, coef_tmp [0]);
					sum          = coef_tmp [0];
				}
				else
				{
					// Nothing to save here.
					assert (false);
				}
			}
			else
			{
				amp = 1.0 / fabs (n);
			}
		}

		amp *= _gain;

		// Second pass: builds the actual FIR, handling picture edge conditions.
		KernelInfo &   info = _kernel_info_arr [y];
		double         accu = 0;
		info._kernel_size   = 0;
		info._coef_index    = int (_coef_flt_arr.size ());
		info._start_line    = fstb::limit (src_pos_beg, 0, last_line);
		info._copy_flt_flag = false;
		info._copy_int_flag = false;
		for (int k = 0; k < bi._fir_len; ++k)
		{
			const int		p   = src_pos_beg + k;
			const double	val = coef_tmp [k] * amp;
			accu += val;
			if (p >= 0 && p < last_line)
			{
				++ info._kernel_size;

				// Float part
				_coef_flt_arr.push_back (float (accu));

				// Integer part
				if (_can_int_flag)
				{
					push_back_int_coef (accu);
				}

				accu = 0;
			}
		}

		// Case 1: We've gone past the bottom of the source and haven't stored
		//         the last line data yet
		// Case 2: We even haven't reached any valid line.
		if (! fstb::is_null (accu) || info._kernel_size == 0)
		{
			++ info._kernel_size;

			// Float part
			_coef_flt_arr.push_back (float (accu));

			// Integer part
			if (_can_int_flag)
			{
				push_back_int_coef (accu);
			}
		}

		// Integer: fix the coefficents to make the sum exactly equal to 1.
		if (_can_int_flag && _norm_flag)
		{
			int            sum_i    = 0;
			const int      nbr_coef = info._kernel_size;
			for (int k = 0; k < nbr_coef; ++k)
			{
				sum_i += _coef_int_arr.get_coef (info._coef_index + k);
			}

			const int      target = fstb::round_int (sum * amp * (1 << SHIFT_INT));
			const int      dif    = target - sum_i;
			if (dif != 0)
			{
				const int		unit  = (dif > 0) ? 1 : -1;
				const int      count = std::abs (dif);

				// Sorts coefficients by magnitude, so we'll fix the biggest ones
				// first
				coef_rank.clear ();
				for (int k = 0; k < nbr_coef; ++k)
				{
					const int      idx = info._coef_index + k;
					coef_rank.emplace_back (ValPos {
						std::abs (_coef_int_arr.get_coef (idx)), idx
					});
				}
				std::sort (
					coef_rank.begin (), coef_rank.end (),
					[] (const ValPos &lhs, const ValPos &rhs)
					{
						return (lhs._val < rhs._val);
					}
				);

				// Small fixes
				if (count <= nbr_coef)
				{
					for (int i = 0; i < count; ++i)
					{
						const int      index = coef_rank [i]._idx;
						const int      fixed = _coef_int_arr.get_coef (index) + unit;
						_coef_int_arr.set_coef (index, fixed);
					}
				}

				// Here the average fixing offset per coefficient is > 1, occuring
				// in degenerated cases when coefficients are too large and are
				// saturated during conversion to integer.
				else
				{
					int            rem = count;
					for (int i = 0; i < nbr_coef && rem > 0; ++i)
					{
						const int      index = coef_rank [i]._idx;
						const int      old   = _coef_int_arr.get_coef (index);
						int            fixed = old + rem * unit;
						fixed = fstb::limit <int> (fixed, INT16_MIN, INT16_MAX);
						rem  -= std::abs (fixed - old);
						_coef_int_arr.set_coef (index, fixed);
					}
					assert (rem == 0);
				}
			}
		}

		// Finally, trivial kernel optimization (removes null coefficients
		// on the sides)
		const float    thr_0_flt = float (_gain * 1e-6f);
		while (info._kernel_size > 1)
		{
			const int      index_last = info._coef_index + info._kernel_size - 1;
			if (fabs (_coef_flt_arr [index_last]) > thr_0_flt)
			{
				break;
			}
			-- info._kernel_size;
		}
		while (info._kernel_size > 1)
		{
			if (fabs (_coef_flt_arr [info._coef_index]) > thr_0_flt)
			{
				break;
			}
			++ info._start_line;
			++ info._coef_index;
			-- info._kernel_size;
		}

		// Single, unit coefficient: we can copy the line
		const float    thr_1_flt = 1e-5f;
		if (info._kernel_size == 1)
		{
			const float    d_flt = fabsf (_coef_flt_arr [info._coef_index] - 1.0f);
			info._copy_flt_flag = (d_flt <= thr_1_flt);

			if (_can_int_flag)
			{
				const int      c_int = _coef_int_arr.get_coef (info._coef_index);
				const int      unit  = 1 << SHIFT_INT;
				info._copy_int_flag = (c_int == unit);
			}
		}

		// Next line
		bi._src_pos += bi._src_step;
	}
}



void	Scaler::push_back_int_coef (double coef)
{
	const double   cintsc   = double ((uint64_t (1)) << SHIFT_INT);
	double         coef_mul = coef * cintsc;
	coef_mul = fstb::limit (coef_mul, double (INT16_MIN), double (INT16_MAX));
	const int      coef_int = fstb::round_int (coef_mul);
	assert (coef_int >= INT16_MIN && coef_int <= INT16_MAX);

	const size_t   ci_pos   = _coef_int_arr.get_size ();
	_coef_int_arr.resize (int (ci_pos + 1));
	_coef_int_arr.set_coef (int (ci_pos), coef_int);
}



Scaler::BasicInfo::BasicInfo (int src_height, int dst_height, double win_top, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, double center_pos_src, double center_pos_dst)
{
	fstb::unused (src_height);
	assert (src_height > 0);
	assert (dst_height > 0);
	assert (win_height > 0);
	assert (kernel_scale > 0);

	// step is the distance between the two source pixels corresponding
	// to two adjacent destination pixels
	_src_step = win_height / double (dst_height);

	// Size of a filter unit step (distance between 2 zero-crossing for a sinc)
	_zc_size  = std::max (_src_step, 1.0) / kernel_scale;

	// This is the correspounding distance in the impulse.
	_imp_step = 1.0 / _zc_size;

	// Number of source pixels covered by the FIR
	_support  = kernel_fnc.get_support () * _zc_size;
	_fir_len  = std::max (fstb::ceil_int (_support * 2), 1);

	_src_pos  = win_top;

	// Introduces an offset because samples are located at the center of the
	// pixels, not on their boundaries. Excepted for pointresize.
	if (_support > 0)
	{
		_src_pos +=   _src_step * center_pos_dst
		            -         1 * center_pos_src;
	}
	else
	{
		// Not really required, but gives the same behaviour as
		// AviSynth's PointResize().
		_support = 0.0001;
	}
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
