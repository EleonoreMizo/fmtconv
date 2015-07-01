/*****************************************************************************

        Matrix2020CLProc.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Matrix2020CLProc_HEADER_INCLUDED)
#define	fmtcl_Matrix2020CLProc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtcl/SplFmt.h"
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fmtcl/TransLut.h"
#endif   // fstb_ARCHI_X86
#include "fstb/ArrayAlign.h"

#include <array>
#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include <memory>
#endif   // fstb_ARCHI_X86

#include <cstdint>



namespace fmtcl
{



class Matrix2020CLProc
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef Matrix2020CLProc ThisType;

	enum Err
	{
		Err_OK = 0,
		Err_INVALID_FORMAT_COMBINATION = -1000
	};

	enum {         NBR_PLANES = 3  };

	explicit        Matrix2020CLProc (bool sse2_flag, bool avx2_flag);
	virtual        ~Matrix2020CLProc () {}

	Err            configure (bool to_yuv_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool full_flag);

	// All stride values are in bytes
	// h must be the frame height too, not only the processed stripe height
	// (required for Stack16 formats to compute the lsb offset)
	void           process (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum Col
	{
		Col_R = 0,
		Col_G = 1,
		Col_B = 2
	};

	enum {         SHIFT_INT     =   12 };   // Number of bits for the fractional part
	enum {         RGB_INT_BITS  =   16 };
	enum {         BUF_LEN       = 2048 };

	typedef float FltBuf [BUF_LEN];
	typedef fstb::ArrayAlign <FltBuf, 3, 32> BufAlign;

	Err            setup_rgb_2_ycbcr ();
	Err            setup_ycbcr_2_rgb ();

	template <typename DST, int DB, class SRC, int SB>
	void           conv_rgb_2_ycbcr_cpp_int (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;
	void           conv_rgb_2_ycbcr_cpp_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;

	template <typename DST, int DB, class SRC, int SB>
	void           conv_ycbcr_2_rgb_cpp_int (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;
	void           conv_ycbcr_2_rgb_cpp_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           conv_rgb_2_ycbcr_sse2_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;
	void           conv_ycbcr_2_rgb_sse2_flt (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;
#endif   // fstb_ARCHI_X86

	template <typename T>
	static fstb_FORCEINLINE T
	               map_lin_to_gam (T v_lin, bool b12_flag);
	template <typename T>
	static fstb_FORCEINLINE T
	               map_gam_to_lin (T v_gam, bool b12_flag);

	SplFmt         _src_fmt;
	int            _src_bits;
	SplFmt         _dst_fmt;
	int            _dst_bits;

	bool           _sse2_flag;
	bool           _avx2_flag;

	bool           _to_yuv_flag;
	bool           _b12_flag;
	bool           _flt_flag;
	bool           _full_range_flag;

	std::array <int16_t, NBR_PLANES>
	               _coef_rgby_int;
	std::array <uint16_t, 1 << RGB_INT_BITS>
	               _map_gamma_int;
	uint16_t       _coef_yg_a_int;
	int32_t        _coef_yg_b_int;
	std::array <uint16_t, 2>
	               _coef_cb_a_int;  // 0: cb >= 0, 1: cb < 0
	std::array <uint16_t, 2>
	               _coef_cr_a_int;
	int32_t        _coef_cbcr_b_int;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	std::unique_ptr <TransLut>
	               _lut_uptr;
#endif   // fstb_ARCHI_X86

	void (ThisType::*             // 0 = not set
	               _proc_ptr) (uint8_t * const dst_ptr_arr [NBR_PLANES], const int dst_str_arr [NBR_PLANES], const uint8_t * const src_ptr_arr [NBR_PLANES], const int src_str_arr [NBR_PLANES], int w, int h) const;

	static const double
	               _coef_rgb_to_y_dbl [NBR_PLANES];
	static const double
	               _coef_ryb_to_g_dbl [NBR_PLANES];
	static const double
	               _coef_cb_neg;
	static const double
	               _coef_cb_pos;
	static const double
	               _coef_cr_neg;
	static const double
	               _coef_cr_pos;

	static const double
	               _alpha_b12;
	static const double
	               _alpha_low;
	static const double
	               _beta_b12;
	static const double
	               _beta_low;
	static const double
	               _slope_lin;
	static const double
	               _gam_pow;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix2020CLProc ();
	               Matrix2020CLProc (const Matrix2020CLProc &other);
	Matrix2020CLProc &
	               operator = (const Matrix2020CLProc &other);
	bool           operator == (const Matrix2020CLProc &other) const;
	bool           operator != (const Matrix2020CLProc &other) const;

};	// class Matrix2020CLProc



}	// namespace fmtcl



//#include "fmtcl/Matrix2020CLProc.hpp"



#endif	// fmtcl_Matrix2020CLProc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
