/*****************************************************************************

        Matrix2020CLProc.cpp
        Author: Laurent de Soras, 2015

TO DO:
	- AVX2 optimizations

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

#include "fmtcl/fnc.h"
#include "fmtcl/Matrix2020CLProc.h"
#include "fmtcl/Matrix2020CLProc_macro.h"
#include "fmtcl/PicFmt.h"
#include "fmtcl/ProxyRwCpp.h"
#include "fmtcl/TransOpLinPow.h"
#include "fstb/fnc.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fstb/ToolsSse2.h"
#endif   // fstb_ARCHI_X86

#include <cassert>
#include <cmath>



namespace fmtcl
{



const double	Matrix2020CLProc::_coef_rgb_to_y_dbl [NBR_PLANES] =
{
	0.2627,
	0.6780,
	0.0593
};

const double	Matrix2020CLProc::_coef_ryb_to_g_dbl [NBR_PLANES] =
{
	-_coef_rgb_to_y_dbl [Col_R] / _coef_rgb_to_y_dbl [Col_G],
	+1                          / _coef_rgb_to_y_dbl [Col_G],
	-_coef_rgb_to_y_dbl [Col_B] / _coef_rgb_to_y_dbl [Col_G]
};

const double	Matrix2020CLProc::_coef_cb_neg = 1.9404;
const double	Matrix2020CLProc::_coef_cb_pos = 1.5816;
const double	Matrix2020CLProc::_coef_cr_neg = 1.7184;
const double	Matrix2020CLProc::_coef_cr_pos = 0.9936;

const double	Matrix2020CLProc::_alpha_b12   = 1.0993;
const double	Matrix2020CLProc::_alpha_low   = 1.099 ;
const double	Matrix2020CLProc::_beta_b12    = 0.0181;
const double	Matrix2020CLProc::_beta_low    = 0.018 ;

const double	Matrix2020CLProc::_slope_lin   = 4.5;
const double	Matrix2020CLProc::_gam_pow     = 0.45;



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix2020CLProc::Matrix2020CLProc (bool sse2_flag, bool avx2_flag)
:	_src_fmt (SplFmt_ILLEGAL)
,	_src_bits (0)
,	_dst_fmt (SplFmt_ILLEGAL)
,	_dst_bits (0)
,	_sse2_flag (sse2_flag)
,	_avx2_flag (avx2_flag)
,	_to_yuv_flag (false)
,	_b12_flag (false)
,	_flt_flag (false)
,	_full_range_flag (false)
,	_coef_rgby_int ()
,	_map_gamma_int ()
,	_coef_yg_a_int (0)
,	_coef_yg_b_int (0)
,	_coef_cb_a_int ()
,	_coef_cr_a_int ()
,	_coef_cbcr_b_int (0)
#if (fstb_ARCHI == fstb_ARCHI_X86)
,	_lut_uptr ()
#endif   // fstb_ARCHI_X86
,	_proc_ptr (0)
{
	// Nothing
}



Matrix2020CLProc::Err	Matrix2020CLProc::configure (bool to_yuv_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool full_flag)
{
	_src_fmt         = src_fmt;
	_src_bits        = src_bits;
	_dst_fmt         = dst_fmt;
	_dst_bits        = dst_bits;
	_to_yuv_flag     = to_yuv_flag;
	_b12_flag        = (dst_bits >= 12);
	_flt_flag        = (dst_fmt == SplFmt_FLOAT);
	_full_range_flag = full_flag;

	Err            ret_val = Err_OK;

	if (_to_yuv_flag)
	{
		ret_val = setup_rgb_2_ycbcr ();
	}
	else
	{
		ret_val = setup_ycbcr_2_rgb ();
	}

	return (ret_val);
}



void	Matrix2020CLProc::process (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (_src_fmt != SplFmt_ILLEGAL);
	assert (_dst_fmt != SplFmt_ILLEGAL);
	assert (_proc_ptr != 0);

	(this->*_proc_ptr) (
		dst_ptr_arr, dst_str_arr,
		src_ptr_arr, src_str_arr,
		w, h
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix2020CLProc::Err	Matrix2020CLProc::setup_rgb_2_ycbcr ()
{
	Err            ret_val = Err_OK;

	if (_flt_flag)
	{
	   _proc_ptr = &ThisType::conv_rgb_2_ycbcr_cpp_flt;

#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_sse2_flag || _avx2_flag)
		{
			if (_sse2_flag)
			{
				_proc_ptr = &ThisType::conv_rgb_2_ycbcr_sse2_flt;
			}

			std::unique_ptr <TransOpInterface>  curve_uptr (new TransOpLinPow (
				false, _alpha_b12, _beta_b12, _gam_pow, _slope_lin
			));
			_lut_uptr = std::unique_ptr <TransLut> (new TransLut (
				*curve_uptr, false,
				SplFmt_FLOAT, 32, true,
				SplFmt_FLOAT, 32, _full_range_flag,
				_sse2_flag, _avx2_flag
			));
		}
#endif   // fstb_ARCHI_X86
	}

	else
	{
#define fmtcl_Matrix2020CLProc_CASE_INT(DF, DB, SF, SB) \
		case   (fmtcl::SplFmt_##DF << 17) + (DB << 10) \
		     + (fmtcl::SplFmt_##SF <<  7) + (SB      ): \
			_proc_ptr = &ThisType::conv_rgb_2_ycbcr_cpp_int < \
				ProxyRwCpp <fmtcl::SplFmt_##DF>, DB, \
				ProxyRwCpp <fmtcl::SplFmt_##SF>, SB \
			>; \
			break;

		switch (
			  (_dst_fmt  << 17)
			+ (_dst_bits << 10)
			+ (_src_fmt  <<  7)
			+ (_src_bits      )
		)
		{
		fmtcl_Matrix2020CLProc_TO_YUV_SPAN_I (fmtcl_Matrix2020CLProc_CASE_INT)
		default:
			ret_val = Err_INVALID_FORMAT_COMBINATION;
			break;
		}

#undef fmtcl_Matrix2020CLProc_CASE_INT

		// RGB -> Y
		int            sum = 0;
		for (int p = 0; p < NBR_PLANES - 1; ++p)
		{
			_coef_rgby_int [p] = static_cast <int16_t> (fstb::round_int (
				_coef_rgb_to_y_dbl [p] * (1 << SHIFT_INT)
			));
			sum += _coef_rgby_int [p];
		}
		_coef_rgby_int [NBR_PLANES - 1] =
			static_cast <int16_t> ((1 << SHIFT_INT) - sum);

		// E -> E'
		const int      table_size = 1 << RGB_INT_BITS;
		for (int v = 0; v < table_size; ++v)
		{
			const double   v_lin = double (v) / double (table_size - 1);
			const double   v_gam = map_lin_to_gam (v_lin, _b12_flag);
			_map_gamma_int [v] =
				static_cast <uint16_t> (fstb::round_int (v_gam * (table_size - 1)));
		}

		// Scaling coefficients
		double         a_y;
		double         b_y;
		double         a_c;
		double         b_c;
		compute_fmt_mac_cst (
			a_y, b_y,
			PicFmt { _dst_fmt, RGB_INT_BITS, ColorFamily_YUV, _full_range_flag },
			PicFmt { _dst_fmt, RGB_INT_BITS, ColorFamily_YUV, true             },
			0
		);
		compute_fmt_mac_cst (
			a_c, b_c,
			PicFmt { _dst_fmt, RGB_INT_BITS, ColorFamily_YUV, _full_range_flag },
			PicFmt { _dst_fmt, RGB_INT_BITS, ColorFamily_YUV, true             },
			1
		);
		const int      dif_bits   = RGB_INT_BITS - _dst_bits;
		const int      coef_scale = 1 <<  SHIFT_INT;
		const int      cst_r      = 1 << (SHIFT_INT + dif_bits - 1);
		const int      ofs_grey   = 1 << (SHIFT_INT + RGB_INT_BITS - 1);
		const double   coef_a_y   = a_y * coef_scale;
		const double   coef_b_y   = b_y * coef_scale + cst_r;
		const double   coef_a_c   = a_c * coef_scale;
		const double   coef_a_cb0 = coef_a_c / _coef_cb_pos;
		const double   coef_a_cb1 = coef_a_c / _coef_cb_neg;
		const double   coef_a_cr0 = coef_a_c / _coef_cr_pos;
		const double   coef_a_cr1 = coef_a_c / _coef_cr_neg;
		const double   coef_b_c   = ofs_grey + cst_r;
		_coef_yg_a_int     = uint16_t (fstb::round_int (coef_a_y  ));
		_coef_yg_b_int     = int32_t ( fstb::round_int (coef_b_y  ));
		_coef_cb_a_int [0] = uint16_t (fstb::round_int (coef_a_cb0));
		_coef_cb_a_int [1] = uint16_t (fstb::round_int (coef_a_cb1));
		_coef_cr_a_int [0] = uint16_t (fstb::round_int (coef_a_cr0));
		_coef_cr_a_int [1] = uint16_t (fstb::round_int (coef_a_cr1));
		_coef_cbcr_b_int   = int32_t ( fstb::round_int (coef_b_c  ));
	}

	return (ret_val);
}



Matrix2020CLProc::Err	Matrix2020CLProc::setup_ycbcr_2_rgb ()
{
	Err            ret_val = Err_OK;

	if (_flt_flag)
	{
	   _proc_ptr = &ThisType::conv_ycbcr_2_rgb_cpp_flt;

#if (fstb_ARCHI == fstb_ARCHI_X86)
		if (_sse2_flag || _avx2_flag)
		{
			if (_sse2_flag)
			{
				_proc_ptr = &ThisType::conv_ycbcr_2_rgb_sse2_flt;
			}

			std::unique_ptr <TransOpInterface>  curve_uptr (new TransOpLinPow (
				true, _alpha_b12, _beta_b12, _gam_pow, _slope_lin
			));
			_lut_uptr = std::unique_ptr <TransLut> (new TransLut (
				*curve_uptr, false,
				SplFmt_FLOAT, 32, _full_range_flag,
				SplFmt_FLOAT, 32, true,
				_sse2_flag, _avx2_flag
			));
		}
#endif   // fstb_ARCHI_X86
	}

	else
	{
#define fmtcl_Matrix2020CLProc_CASE_INT(DF, DB, SF, SB) \
		case   (fmtcl::SplFmt_##DF << 17) + (DB << 10) \
		     + (fmtcl::SplFmt_##SF <<  7) + (SB      ): \
			_proc_ptr = &ThisType::conv_ycbcr_2_rgb_cpp_int < \
				ProxyRwCpp <fmtcl::SplFmt_##DF>, DB, \
				ProxyRwCpp <fmtcl::SplFmt_##SF>, SB \
			>; \
			break;

		switch (
			  (_dst_fmt  << 17)
			+ (_dst_bits << 10)
			+ (_src_fmt  <<  7)
			+ (_src_bits      )
		)
		{
		fmtcl_Matrix2020CLProc_TO_RGB_SPAN_I (fmtcl_Matrix2020CLProc_CASE_INT)
		default:
			ret_val = Err_INVALID_FORMAT_COMBINATION;
			break;
		}

#undef fmtcl_Matrix2020CLProc_CASE_INT

		// YBR -> G
		for (int p = 0; p < NBR_PLANES; ++p)
		{
			_coef_rgby_int [p] = static_cast <int16_t> (fstb::round_int (
				_coef_ryb_to_g_dbl [p] * (1 << SHIFT_INT)
			));
		}

		// E' -> E
		const int      table_size = 1 << RGB_INT_BITS;
		for (int v = 0; v < table_size; ++v)
		{
			const double   v_gam = double (v) / double (table_size - 1);
			const double   v_lin = map_gam_to_lin (v_gam, _b12_flag);
			_map_gamma_int [v] =
				static_cast <uint16_t> (fstb::round_int (v_lin * (table_size - 1)));
		}

		// Scaling coefficients
		double         a_y;
		double         b_y;
		double         a_c;
		double         b_c;
		compute_fmt_mac_cst (
			a_y, b_y,
			PicFmt { _src_fmt, _src_bits, ColorFamily_YUV, true             },
			PicFmt { _src_fmt, _src_bits, ColorFamily_YUV, _full_range_flag },
			0
		);
		compute_fmt_mac_cst (
			a_c, b_c,
			PicFmt { _src_fmt, _src_bits, ColorFamily_YUV, true             },
			PicFmt { _src_fmt, _src_bits, ColorFamily_YUV, _full_range_flag },
			1
		);
		const int      dif_bits   = RGB_INT_BITS - _src_bits;
		const int      coef_scale = 1 <<  SHIFT_INT;
		const int      cst_r      = 1 << (SHIFT_INT - dif_bits - 1);
		const double   coef_a_y   = a_y * coef_scale;
		const double   coef_b_y   = b_y * coef_scale + cst_r;
		const double   coef_a_c   = a_c * coef_scale;
		const double   coef_a_cb0 = coef_a_c * _coef_cb_pos;
		const double   coef_a_cb1 = coef_a_c * _coef_cb_neg;
		const double   coef_a_cr0 = coef_a_c * _coef_cr_pos;
		const double   coef_a_cr1 = coef_a_c * _coef_cr_neg;
		const double   coef_b_c   = cst_r;
		_coef_yg_a_int     = uint16_t (fstb::round_int (coef_a_y  ));
		_coef_yg_b_int     = int32_t ( fstb::round_int (coef_b_y  ));
		_coef_cb_a_int [0] = uint16_t (fstb::round_int (coef_a_cb0));
		_coef_cb_a_int [1] = uint16_t (fstb::round_int (coef_a_cb1));
		_coef_cr_a_int [0] = uint16_t (fstb::round_int (coef_a_cr0));
		_coef_cr_a_int [1] = uint16_t (fstb::round_int (coef_a_cr1));
		_coef_cbcr_b_int   = int32_t ( fstb::round_int (coef_b_c  ));
	}

	return (ret_val);
}



template <typename DST, int DB, class SRC, int SB>
void	Matrix2020CLProc::conv_rgb_2_ycbcr_cpp_int (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const int      sizeof_st = int (sizeof (typename SRC::PtrConst::DataType));
	assert (src_str_arr [0] % sizeof_st == 0);
	assert (src_str_arr [1] % sizeof_st == 0);
	assert (src_str_arr [2] % sizeof_st == 0);
	const int      sizeof_dt = int (sizeof (typename DST::Ptr::DataType));
	assert (dst_str_arr [0] % sizeof_dt == 0);
	assert (dst_str_arr [1] % sizeof_dt == 0);
	assert (dst_str_arr [2] % sizeof_dt == 0);

	SrcPtr         src_0_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [0], src_str_arr [0], h);
	SrcPtr         src_1_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [1], src_str_arr [1], h);
	SrcPtr         src_2_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [2], src_str_arr [2], h);
	const int      src_0_str = src_str_arr [0] / sizeof_st - w;
	const int      src_1_str = src_str_arr [1] / sizeof_st - w;
	const int      src_2_str = src_str_arr [2] / sizeof_st - w;

	DstPtr         dst_0_ptr = DST::Ptr::make_ptr (dst_ptr_arr [0], dst_str_arr [0], h);
	DstPtr         dst_1_ptr = DST::Ptr::make_ptr (dst_ptr_arr [1], dst_str_arr [1], h);
	DstPtr         dst_2_ptr = DST::Ptr::make_ptr (dst_ptr_arr [2], dst_str_arr [2], h);
	const int      dst_0_str = dst_str_arr [0] / sizeof_dt - w;
	const int      dst_1_str = dst_str_arr [1] / sizeof_dt - w;
	const int      dst_2_str = dst_str_arr [2] / sizeof_dt - w;

	const int      shft2 = SHIFT_INT + RGB_INT_BITS - DB;
	const int      cst_r = 1 << (SHIFT_INT - 1);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      rl = SRC::read (src_0_ptr);
			const int      gl = SRC::read (src_1_ptr);
			const int      bl = SRC::read (src_2_ptr);

			const int      yl = uint16_t (
				(  rl * _coef_rgby_int [Col_R]
				 + gl * _coef_rgby_int [Col_G]
				 + bl * _coef_rgby_int [Col_B]
			    +      cst_r                 ) >> SHIFT_INT);

			const int      yg = _map_gamma_int [yl];
			const int      bg = _map_gamma_int [bl];
			const int      rg = _map_gamma_int [rl];

			const int      cb = bg - yg;
			const int      cr = rg - yg;

			int            dy  = (yg * _coef_yg_a_int            + _coef_yg_b_int  ) >> shft2;
			int            dcb = (cb * _coef_cb_a_int [(cb < 0)] + _coef_cbcr_b_int) >> shft2;
			int            dcr = (cr * _coef_cr_a_int [(cr < 0)] + _coef_cbcr_b_int) >> shft2;

			DST::template write_clip <DB> (dst_0_ptr, dy );
			DST::template write_clip <DB> (dst_1_ptr, dcb);
			DST::template write_clip <DB> (dst_2_ptr, dcr);

			SRC::PtrConst::jump (src_0_ptr, 1);
			SRC::PtrConst::jump (src_1_ptr, 1);
			SRC::PtrConst::jump (src_2_ptr, 1);

			DST::Ptr::jump (dst_0_ptr, 1);
			DST::Ptr::jump (dst_1_ptr, 1);
			DST::Ptr::jump (dst_2_ptr, 1);
		}

		SRC::PtrConst::jump (src_0_ptr, src_0_str);
		SRC::PtrConst::jump (src_1_ptr, src_1_str);
		SRC::PtrConst::jump (src_2_ptr, src_2_str);

		DST::Ptr::jump (dst_0_ptr, dst_0_str);
		DST::Ptr::jump (dst_1_ptr, dst_1_str);
		DST::Ptr::jump (dst_2_ptr, dst_2_str);
	}
}



void	Matrix2020CLProc::conv_rgb_2_ycbcr_cpp_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);
	assert (dst_str_arr [1] % sizeof_xt == 0);
	assert (dst_str_arr [2] % sizeof_xt == 0);

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt;
	const int      src_1_str = src_str_arr [1] / sizeof_xt;
	const int      src_2_str = src_str_arr [2] / sizeof_xt;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	float *        dst_1_ptr = reinterpret_cast <      float *> (dst_ptr_arr [1]);
	float *        dst_2_ptr = reinterpret_cast <      float *> (dst_ptr_arr [2]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt;
	const int      dst_1_str = dst_str_arr [1] / sizeof_xt;
	const int      dst_2_str = dst_str_arr [2] / sizeof_xt;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    rl = src_0_ptr [x];
			const float    gl = src_1_ptr [x];
			const float    bl = src_2_ptr [x];

			const float    yl =
				  rl * float (_coef_rgb_to_y_dbl [Col_R])
				+ gl * float (_coef_rgb_to_y_dbl [Col_G])
				+ bl * float (_coef_rgb_to_y_dbl [Col_B]);

			const float    yg = map_lin_to_gam (yl, true);
			const float    bg = map_lin_to_gam (bl, true);
			const float    rg = map_lin_to_gam (rl, true);

			const float    cb = bg - yg;
			const float    cr = rg - yg;

			const float    dy  = yg;
			const float    dcb =
				cb * ((cb < 0) ? float (1 / _coef_cb_neg) : float (1 / _coef_cb_pos));
			const float    dcr =
				cr * ((cr < 0) ? float (1 / _coef_cr_neg) : float (1 / _coef_cr_pos));

			dst_0_ptr [x] = dy ;
			dst_1_ptr [x] = dcb;
			dst_2_ptr [x] = dcr;
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
		dst_1_ptr += dst_1_str;
		dst_2_ptr += dst_2_str;
	}
}



template <typename DST, int DB, class SRC, int SB>
void	Matrix2020CLProc::conv_ycbcr_2_rgb_cpp_int (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");

	typedef typename SRC::PtrConst::Type SrcPtr;
	typedef typename DST::Ptr::Type      DstPtr;

	const int      sizeof_st = int (sizeof (typename SRC::PtrConst::DataType));
	assert (src_str_arr [0] % sizeof_st == 0);
	assert (src_str_arr [1] % sizeof_st == 0);
	assert (src_str_arr [2] % sizeof_st == 0);
	const int      sizeof_dt = int (sizeof (typename DST::Ptr::DataType));
	assert (dst_str_arr [0] % sizeof_dt == 0);
	assert (dst_str_arr [1] % sizeof_dt == 0);
	assert (dst_str_arr [2] % sizeof_dt == 0);

	SrcPtr         src_0_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [0], src_str_arr [0], h);
	SrcPtr         src_1_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [1], src_str_arr [1], h);
	SrcPtr         src_2_ptr = SRC::PtrConst::make_ptr (src_ptr_arr [2], src_str_arr [2], h);
	const int      src_0_str = src_str_arr [0] / sizeof_st - w;
	const int      src_1_str = src_str_arr [1] / sizeof_st - w;
	const int      src_2_str = src_str_arr [2] / sizeof_st - w;

	DstPtr         dst_0_ptr = DST::Ptr::make_ptr (dst_ptr_arr [0], dst_str_arr [0], h);
	DstPtr         dst_1_ptr = DST::Ptr::make_ptr (dst_ptr_arr [1], dst_str_arr [1], h);
	DstPtr         dst_2_ptr = DST::Ptr::make_ptr (dst_ptr_arr [2], dst_str_arr [2], h);
	const int      dst_0_str = dst_str_arr [0] / sizeof_dt - w;
	const int      dst_1_str = dst_str_arr [1] / sizeof_dt - w;
	const int      dst_2_str = dst_str_arr [2] / sizeof_dt - w;

	const int      shft2     = SHIFT_INT + SB - RGB_INT_BITS;
	const int      cst_r     = 1 << (SHIFT_INT - 1);
	const int      ma_int    = (1 << RGB_INT_BITS) - 1;
	const int      ofs_grey  = 1 << (SB - 1);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      dy  = SRC::read (src_0_ptr);
			const int      dcb = SRC::read (src_1_ptr);
			const int      dcr = SRC::read (src_2_ptr);

			const int      dcb0 = dcb - ofs_grey;
			const int      dcr0 = dcr - ofs_grey;

			int            yg = (dy   * _coef_yg_a_int              + _coef_yg_b_int  ) >> shft2;
			const int      cb = (dcb0 * _coef_cb_a_int [(dcb0 < 0)] + _coef_cbcr_b_int) >> shft2;
			const int      cr = (dcr0 * _coef_cr_a_int [(dcr0 < 0)] + _coef_cbcr_b_int) >> shft2;

			const int      bg = std::max (std::min (cb + yg, ma_int), 0);
			const int      rg = std::max (std::min (cr + yg, ma_int), 0);
			yg = std::max (std::min (yg, ma_int), 0);

			const int      yl = _map_gamma_int [yg];
			const int      bl = _map_gamma_int [bg];
			const int      rl = _map_gamma_int [rg];

			const int      gl = uint16_t (std::max (
				(  rl * _coef_rgby_int [Col_R]
				 + yl * _coef_rgby_int [Col_G]
				 + bl * _coef_rgby_int [Col_B]
			    +      cst_r                 ) >> SHIFT_INT,
				0
			));

			DST::template write_clip <DB> (dst_0_ptr, rl);
			DST::template write_clip <DB> (dst_1_ptr, gl);
			DST::template write_clip <DB> (dst_2_ptr, bl);

			SRC::PtrConst::jump (src_0_ptr, 1);
			SRC::PtrConst::jump (src_1_ptr, 1);
			SRC::PtrConst::jump (src_2_ptr, 1);

			DST::Ptr::jump (dst_0_ptr, 1);
			DST::Ptr::jump (dst_1_ptr, 1);
			DST::Ptr::jump (dst_2_ptr, 1);
		}

		SRC::PtrConst::jump (src_0_ptr, src_0_str);
		SRC::PtrConst::jump (src_1_ptr, src_1_str);
		SRC::PtrConst::jump (src_2_ptr, src_2_str);

		DST::Ptr::jump (dst_0_ptr, dst_0_str);
		DST::Ptr::jump (dst_1_ptr, dst_1_str);
		DST::Ptr::jump (dst_2_ptr, dst_2_str);
	}
}



void	Matrix2020CLProc::conv_ycbcr_2_rgb_cpp_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);
	assert (dst_str_arr [1] % sizeof_xt == 0);
	assert (dst_str_arr [2] % sizeof_xt == 0);

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt;
	const int      src_1_str = src_str_arr [1] / sizeof_xt;
	const int      src_2_str = src_str_arr [2] / sizeof_xt;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	float *        dst_1_ptr = reinterpret_cast <      float *> (dst_ptr_arr [1]);
	float *        dst_2_ptr = reinterpret_cast <      float *> (dst_ptr_arr [2]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt;
	const int      dst_1_str = dst_str_arr [1] / sizeof_xt;
	const int      dst_2_str = dst_str_arr [2] / sizeof_xt;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    dy  = src_0_ptr [x];
			const float    dcb = src_1_ptr [x];
			const float    dcr = src_2_ptr [x];

			const float    yg = dy;
			const float    cb =
				dcb * ((dcb < 0) ? float (_coef_cb_neg) : float (_coef_cb_pos));
			const float    cr =
				dcr * ((dcr < 0) ? float (_coef_cr_neg) : float (_coef_cr_pos));

			const float    bg = cb + yg;
			const float    rg = cr + yg;

			const float    yl = map_gam_to_lin (yg, true);
			const float    bl = map_gam_to_lin (bg, true);
			const float    rl = map_gam_to_lin (rg, true);

			const float    gl =
				  rl * float (_coef_ryb_to_g_dbl [Col_R])
				+ yl * float (_coef_ryb_to_g_dbl [Col_G])
				+ bl * float (_coef_ryb_to_g_dbl [Col_B]);

			dst_0_ptr [x] = rl;
			dst_1_ptr [x] = gl;
			dst_2_ptr [x] = bl;
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
		dst_1_ptr += dst_1_str;
		dst_2_ptr += dst_2_str;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



void	Matrix2020CLProc::conv_rgb_2_ycbcr_sse2_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (_lut_uptr.get () != 0);
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);
	assert (dst_str_arr [1] % sizeof_xt == 0);
	assert (dst_str_arr [2] % sizeof_xt == 0);

	const int      stride_fix = ((w + BUF_LEN - 1) / BUF_LEN) * BUF_LEN;

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt - stride_fix;
	const int      src_1_str = src_str_arr [1] / sizeof_xt - stride_fix;
	const int      src_2_str = src_str_arr [2] / sizeof_xt - stride_fix;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	float *        dst_1_ptr = reinterpret_cast <      float *> (dst_ptr_arr [1]);
	float *        dst_2_ptr = reinterpret_cast <      float *> (dst_ptr_arr [2]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt - stride_fix;
	const int      dst_1_str = dst_str_arr [1] / sizeof_xt - stride_fix;
	const int      dst_2_str = dst_str_arr [2] / sizeof_xt - stride_fix;

	BufAlign       tmp_buf_arr;

	const __m128   c_yr   = _mm_set1_ps (float (_coef_rgb_to_y_dbl [Col_R]));
	const __m128   c_yg   = _mm_set1_ps (float (_coef_rgb_to_y_dbl [Col_G]));
	const __m128   c_yb   = _mm_set1_ps (float (_coef_rgb_to_y_dbl [Col_B]));

	const __m128   c_cb_n = _mm_set1_ps (float (1 / _coef_cb_neg));
	const __m128   c_cb_p = _mm_set1_ps (float (1 / _coef_cb_pos));
	const __m128   c_cr_n = _mm_set1_ps (float (1 / _coef_cr_neg));
	const __m128   c_cr_p = _mm_set1_ps (float (1 / _coef_cr_pos));

	const __m128   zero   = _mm_setzero_ps ();

	for (int y = 0; y < h; ++y)
	{
		for (int x_buf = 0; x_buf < w; x_buf += BUF_LEN)
		{
			const int      w_work = std::min (w - x_buf, int (BUF_LEN));

			for (int x = 0; x < w_work; x += 4)
			{
				const __m128   rl = _mm_load_ps (src_0_ptr + x);
				const __m128   gl = _mm_load_ps (src_1_ptr + x);
				const __m128   bl = _mm_load_ps (src_2_ptr + x);
				const __m128   yl = _mm_add_ps (_mm_add_ps (
					_mm_mul_ps (rl, c_yr),
					_mm_mul_ps (gl, c_yg)),
					_mm_mul_ps (bl, c_yb)
				);
				_mm_store_ps (tmp_buf_arr [0] + x, yl);
			}

			_lut_uptr->process_plane (
				reinterpret_cast <      uint8_t *> (dst_0_ptr),
				reinterpret_cast <const uint8_t *> (tmp_buf_arr [0]),
				0, 0, w_work, 1
			);
			_lut_uptr->process_plane (
				reinterpret_cast <      uint8_t *> (tmp_buf_arr [1]),
				reinterpret_cast <const uint8_t *> (src_2_ptr),
				0, 0, w_work, 1
			);
			_lut_uptr->process_plane (
				reinterpret_cast <      uint8_t *> (tmp_buf_arr [2]),
				reinterpret_cast <const uint8_t *> (src_0_ptr),
				0, 0, w_work, 1
			);

			for (int x = 0; x < w_work; x += 4)
			{
				const __m128   yg   = _mm_load_ps (dst_0_ptr       + x);
				const __m128   bg   = _mm_load_ps (tmp_buf_arr [1] + x);
				const __m128   rg   = _mm_load_ps (tmp_buf_arr [2] + x);

				const __m128   cb   = _mm_sub_ps (bg, yg);
				const __m128   cr   = _mm_sub_ps (rg, yg);

				const __m128   cb_n = _mm_cmplt_ps (cb, zero);
				const __m128   cr_n = _mm_cmplt_ps (cr, zero);

				const __m128   c_cb = fstb::ToolsSse2::select (cb_n, c_cb_n, c_cb_p);
				const __m128   c_cr = fstb::ToolsSse2::select (cr_n, c_cr_n, c_cr_p);

				const __m128   dcb  = _mm_mul_ps (cb, c_cb);
				const __m128   dcr  = _mm_mul_ps (cr, c_cr);

				_mm_store_ps (dst_1_ptr + x, dcb);
				_mm_store_ps (dst_2_ptr + x, dcr);
			}

			src_0_ptr += BUF_LEN;
			src_1_ptr += BUF_LEN;
			src_2_ptr += BUF_LEN;

			dst_0_ptr += BUF_LEN;
			dst_1_ptr += BUF_LEN;
			dst_2_ptr += BUF_LEN;
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
		dst_1_ptr += dst_1_str;
		dst_2_ptr += dst_2_str;
	}
}



void	Matrix2020CLProc::conv_ycbcr_2_rgb_sse2_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const
{
	assert (_lut_uptr.get () != 0);
	assert (dst_ptr_arr != 0);
	assert (dst_str_arr != 0);
	assert (src_ptr_arr != 0);
	assert (src_str_arr != 0);
	assert (w > 0);
	assert (h > 0);

	static_assert (NBR_PLANES == 3, "Code is hardcoded for 3 planes");
	const int      sizeof_xt = int (sizeof (float));
	assert (src_str_arr [0] % sizeof_xt == 0);
	assert (src_str_arr [1] % sizeof_xt == 0);
	assert (src_str_arr [2] % sizeof_xt == 0);
	assert (dst_str_arr [0] % sizeof_xt == 0);
	assert (dst_str_arr [1] % sizeof_xt == 0);
	assert (dst_str_arr [2] % sizeof_xt == 0);

	const int      stride_fix = ((w + BUF_LEN - 1) / BUF_LEN) * BUF_LEN;

	const float *  src_0_ptr = reinterpret_cast <const float *> (src_ptr_arr [0]);
	const float *  src_1_ptr = reinterpret_cast <const float *> (src_ptr_arr [1]);
	const float *  src_2_ptr = reinterpret_cast <const float *> (src_ptr_arr [2]);
	const int      src_0_str = src_str_arr [0] / sizeof_xt - stride_fix;
	const int      src_1_str = src_str_arr [1] / sizeof_xt - stride_fix;
	const int      src_2_str = src_str_arr [2] / sizeof_xt - stride_fix;

	float *        dst_0_ptr = reinterpret_cast <      float *> (dst_ptr_arr [0]);
	float *        dst_1_ptr = reinterpret_cast <      float *> (dst_ptr_arr [1]);
	float *        dst_2_ptr = reinterpret_cast <      float *> (dst_ptr_arr [2]);
	const int      dst_0_str = dst_str_arr [0] / sizeof_xt - stride_fix;
	const int      dst_1_str = dst_str_arr [1] / sizeof_xt - stride_fix;
	const int      dst_2_str = dst_str_arr [2] / sizeof_xt - stride_fix;

	const __m128   c_rl   = _mm_set1_ps (float (_coef_ryb_to_g_dbl [Col_R]));
	const __m128   c_gl   = _mm_set1_ps (float (_coef_ryb_to_g_dbl [Col_G]));
	const __m128   c_bl   = _mm_set1_ps (float (_coef_ryb_to_g_dbl [Col_B]));

	const __m128   c_cb_n = _mm_set1_ps (float (_coef_cb_neg));
	const __m128   c_cb_p = _mm_set1_ps (float (_coef_cb_pos));
	const __m128   c_cr_n = _mm_set1_ps (float (_coef_cr_neg));
	const __m128   c_cr_p = _mm_set1_ps (float (_coef_cr_pos));

	const __m128   zero   = _mm_setzero_ps ();

	BufAlign       tmp_buf_arr;

	for (int y = 0; y < h; ++y)
	{
		for (int x_buf = 0; x_buf < w; x_buf += BUF_LEN)
		{
			const int      w_work = std::min (w - x_buf, int (BUF_LEN));

			for (int x = 0; x < w_work; x += 4)
			{
				const __m128   yg   = _mm_load_ps (src_0_ptr + x);
				const __m128   dcb  = _mm_load_ps (src_1_ptr + x);
				const __m128   dcr  = _mm_load_ps (src_2_ptr + x);

				const __m128   cb_n = _mm_cmplt_ps (dcb, zero);
				const __m128   cr_n = _mm_cmplt_ps (dcr, zero);

				const __m128   c_cb = fstb::ToolsSse2::select (cb_n, c_cb_n, c_cb_p);
				const __m128   c_cr = fstb::ToolsSse2::select (cr_n, c_cr_n, c_cr_p);

				const __m128   cb   = _mm_mul_ps (dcb, c_cb);
				const __m128   cr   = _mm_mul_ps (dcr, c_cr);

				const __m128   bg   = _mm_add_ps (cb, yg);
				const __m128   rg   = _mm_add_ps (cr, yg);

				_mm_store_ps (tmp_buf_arr [1] + x, bg);
				_mm_store_ps (tmp_buf_arr [2] + x, rg);
			}

			_lut_uptr->process_plane (
				reinterpret_cast <      uint8_t *> (tmp_buf_arr [0]),
				reinterpret_cast <const uint8_t *> (src_0_ptr),
				0, 0, w_work, 1
			);
			_lut_uptr->process_plane (
				reinterpret_cast <      uint8_t *> (dst_2_ptr),
				reinterpret_cast <const uint8_t *> (tmp_buf_arr [1]),
				0, 0, w_work, 1
			);
			_lut_uptr->process_plane (
				reinterpret_cast <      uint8_t *> (dst_0_ptr),
				reinterpret_cast <const uint8_t *> (tmp_buf_arr [2]),
				0, 0, w_work, 1
			);

			for (int x = 0; x < w_work; x += 4)
			{
				const __m128   yl = _mm_load_ps (tmp_buf_arr [0] + x);
				const __m128   bl = _mm_load_ps (dst_2_ptr + x);
				const __m128   rl = _mm_load_ps (dst_0_ptr + x);
				const __m128   gl = _mm_add_ps (_mm_add_ps (
					_mm_mul_ps (yl, c_gl),
					_mm_mul_ps (bl, c_bl)),
					_mm_mul_ps (rl, c_rl)
				);
				_mm_store_ps (dst_1_ptr + x, gl);
			}

			src_0_ptr += BUF_LEN;
			src_1_ptr += BUF_LEN;
			src_2_ptr += BUF_LEN;

			dst_0_ptr += BUF_LEN;
			dst_1_ptr += BUF_LEN;
			dst_2_ptr += BUF_LEN;
		}

		src_0_ptr += src_0_str;
		src_1_ptr += src_1_str;
		src_2_ptr += src_2_str;

		dst_0_ptr += dst_0_str;
		dst_1_ptr += dst_1_str;
		dst_2_ptr += dst_2_str;
	}
}



#endif   // fstb_ARCHI_X86



// Input and output ranges are typically ranging from 0 to 1, but we
// allow some overshoot.
template <typename T>
T	Matrix2020CLProc::map_lin_to_gam (T v_lin, bool b12_flag)
{
	const T        beta  = T ((b12_flag) ?  _beta_b12 :  _beta_low);
	const T        alpha = T ((b12_flag) ? _alpha_b12 : _alpha_low);
	const T        v_gam = T (
			  (v_lin <= beta)
			? v_lin * T (_slope_lin)
			: alpha * pow (v_lin, T (_gam_pow)) - (alpha - 1)
		);

	return (v_gam);
}



// Input and output ranges are typically ranging from 0 to 1, but we
// allow some overshoot.
template <typename T>
T	Matrix2020CLProc::map_gam_to_lin (T v_gam, bool b12_flag)
{
	const T        beta  = T ((b12_flag) ?  _beta_b12 :  _beta_low);
	const T        alpha = T ((b12_flag) ? _alpha_b12 : _alpha_low);
	const T        v_lin = T (
			  (v_gam <= T (beta * _slope_lin))
			? v_gam * T (1 / _slope_lin)
			: pow ((v_gam + (alpha - 1)) * (1 / alpha), T (1 / _gam_pow))
		);

	return (v_lin);
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
