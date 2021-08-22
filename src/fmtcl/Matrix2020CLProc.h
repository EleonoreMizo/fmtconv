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

#include "fmtcl/Frame.h"
#include "fmtcl/FrameRO.h"
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



class ProcComp3Arg;

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

	static constexpr int _nbr_planes   =  3;
	static constexpr int _rgb_int_bits = 16;

	explicit        Matrix2020CLProc (bool sse2_flag, bool avx2_flag);
	virtual        ~Matrix2020CLProc () {}

	Err            configure (bool to_yuv_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool full_flag);

	// All stride values are in bytes
	// h must be the frame height too, not only the processed stripe height
	// (required for Stack16 formats to compute the lsb offset)
	void           process (const ProcComp3Arg &arg) const;



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

	static constexpr int _shift_int    =   12;   // Number of bits for the fractional part
	static constexpr int _buf_len      = 2048;

	typedef std::array <float, _buf_len> FltBuf;
	typedef fstb::ArrayAlign <FltBuf, 3, 32> BufAlign;

	Err            setup_rgb_2_ycbcr ();
	Err            setup_ycbcr_2_rgb ();

	template <typename DST, int DB, class SRC, int SB>
	void           conv_rgb_2_ycbcr_cpp_int (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           conv_rgb_2_ycbcr_cpp_flt (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;

	template <typename DST, int DB, class SRC, int SB>
	void           conv_ycbcr_2_rgb_cpp_int (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           conv_ycbcr_2_rgb_cpp_flt (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           conv_rgb_2_ycbcr_sse2_flt (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           conv_ycbcr_2_rgb_sse2_flt (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
#endif   // fstb_ARCHI_X86

	template <typename T>
	static fstb_FORCEINLINE T
	               map_lin_to_gam (T v_lin, bool b12_flag);
	template <typename T>
	static fstb_FORCEINLINE T
	               map_gam_to_lin (T v_gam, bool b12_flag);

	SplFmt         _src_fmt     = SplFmt_ILLEGAL;
	int            _src_bits    = 0;
	SplFmt         _dst_fmt     = SplFmt_ILLEGAL;
	int            _dst_bits    = 0;

	bool           _sse2_flag   = false;
	bool           _avx2_flag   = false;

	bool           _to_yuv_flag = false;
	bool           _b12_flag    = false;
	bool           _flt_flag    = false;
	bool           _full_range_flag = true;

	std::array <int16_t, _nbr_planes>
	               _coef_rgby_int;
	std::array <uint16_t, 1 << _rgb_int_bits>
	               _map_gamma_int;
	uint16_t       _coef_yg_a_int = 0;
	int32_t        _coef_yg_b_int = 0;
	std::array <uint16_t, 2>
	               _coef_cb_a_int {};  // 0: cb >= 0, 1: cb < 0
	std::array <uint16_t, 2>
	               _coef_cr_a_int {};
	int32_t        _coef_cbcr_b_int = 0;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	std::unique_ptr <TransLut>
	               _lut_uptr;
#endif   // fstb_ARCHI_X86

	void (ThisType::*             // 0 = not set
	               _proc_ptr) (Frame <> dst, FrameRO <> src, int w, int h) const noexcept = nullptr;

	static constexpr std::array <double, _nbr_planes>
	               _coef_rgb_to_y_dbl { 0.2627, 0.6780, 0.0593 };

	static constexpr std::array <double, _nbr_planes>
	               _coef_ryb_to_g_dbl
	{
		-_coef_rgb_to_y_dbl [Col_R] / _coef_rgb_to_y_dbl [Col_G],
		+1                          / _coef_rgb_to_y_dbl [Col_G],
		-_coef_rgb_to_y_dbl [Col_B] / _coef_rgb_to_y_dbl [Col_G]
	};

	static constexpr double _coef_cb_neg = 1.9404;
	static constexpr double _coef_cb_pos = 1.5816;
	static constexpr double _coef_cr_neg = 1.7184;
	static constexpr double _coef_cr_pos = 0.9936;

	static constexpr double _alpha_b12   = 1.0993;
	static constexpr double _alpha_low   = 1.099 ;
	static constexpr double _beta_b12    = 0.0181;
	static constexpr double _beta_low    = 0.018 ;

	static constexpr double _slope_lin   = 4.5;
	static constexpr double _gam_pow     = 0.45;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix2020CLProc ()                               = delete;
	               Matrix2020CLProc (const Matrix2020CLProc &other)  = delete;
	Matrix2020CLProc &
	               operator = (const Matrix2020CLProc &other)        = delete;
	bool           operator == (const Matrix2020CLProc &other) const = delete;
	bool           operator != (const Matrix2020CLProc &other) const = delete;

};	// class Matrix2020CLProc



}	// namespace fmtcl



//#include "fmtcl/Matrix2020CLProc.hpp"



#endif	// fmtcl_Matrix2020CLProc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
