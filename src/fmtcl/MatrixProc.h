/*****************************************************************************

        MatrixProc.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_MatrixProc_HEADER_INCLUDED)
#define	fmtcl_MatrixProc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/CoefArrInt.h"
#include "fmtcl/Frame.h"
#include "fmtcl/FrameRO.h"
#include "fmtcl/Mat4.h"
#include "fmtcl/SplFmt.h"

#include <vector>

#include <cstdint>



namespace fmtcl
{



class ProcComp3Arg;

class MatrixProc
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef MatrixProc ThisType;

	enum Err
	{
		Err_OK = 0,
		Err_POSSIBLE_OVERFLOW = -1000,
		Err_TOO_BIG_COEF,
		Err_INVALID_FORMAT_COMBINATION
	};

	static constexpr int _nbr_planes = 3;
	static constexpr int _mat_size   = _nbr_planes + 1;

	explicit       MatrixProc (bool sse_flag, bool sse2_flag, bool avx_flag, bool avx2_flag);
	virtual        ~MatrixProc () {}

	Err            configure (const Mat4 &m, bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, int plane_out);

	// All stride values are in bytes
	// h must be the frame height too, not only the processed stripe height
	// (required for Stack16 formats to compute the lsb offset)
	void           process (const ProcComp3Arg &arg) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _shift_int = 12;  // Number of bits for the fractional part

	void           set_matrix_flt (const Mat4 &m, int plane_out);
	Err            set_matrix_int (const Mat4 &m, int plane_out, int src_bits, int dst_bits);

#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           setup_fnc_sse (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag);
	void           setup_fnc_sse2 (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag);
	void           setup_fnc_avx (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag);
	void           setup_fnc_avx2 (bool int_proc_flag, SplFmt src_fmt, int src_bits, SplFmt dst_fmt, int dst_bits, bool single_plane_flag);
#endif   // fstb_ARCHI_X86

	template <typename DST, int DB, class SRC, int SB>
	void           process_3_int_cpp (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	template <typename DT, int DB, typename ST, int SB>
	void           process_1_int_cpp (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;

	void           process_3_flt_cpp (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           process_1_flt_cpp (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <class DST, int DB, class SRC, int SB, int NP>
	void           process_n_int_sse2 (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           process_3_flt_sse (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           process_1_flt_sse (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;

	template <class DST, int DB, class SRC, int SB, int NP>
	void           process_n_int_avx2 (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           process_3_flt_avx (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
	void           process_1_flt_avx (Frame <> dst, FrameRO <> src, int w, int h) const noexcept;
#endif   // fstb_ARCHI_X86

	bool           _sse_flag  = false;
	bool           _sse2_flag = false;
	bool           _avx_flag  = false;
	bool           _avx2_flag = false;

	bool           _single_plane_flag = false;

	void (ThisType::*                   // 0 = not set
	               _proc_ptr) (Frame <> dst, FrameRO <> src, int w, int h) const noexcept = nullptr;

	std::vector <float>
	               _coef_flt_arr;

	// Integer coefficients are all scaled with _shift_int.
	// The additive coefficient contains the rounding constant too.
	std::vector <int>
	               _coef_int_arr;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	// Same as integer, excepted:
	// Additive coefficients are stored in 4 (or 8) x 32-bit integers.
	// They may also contain a bias compensating the sign bit flip when the
	// output format is 16 bits.
	fmtcl::CoefArrInt
	               _coef_simd_arr;
#endif   // fstb_ARCHI_X86



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               MatrixProc ()                               = delete;
	               MatrixProc (const MatrixProc &other)        = delete;
	MatrixProc &   operator = (const MatrixProc &other)        = delete;
	bool           operator == (const MatrixProc &other) const = delete;
	bool           operator != (const MatrixProc &other) const = delete;

};	// class MatrixProc



}	// namespace fmtcl



//#include "fmtcl/MatrixProc.hpp"



#endif	// fmtcl_MatrixProc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
