/*****************************************************************************

        Matrix.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Matrix_HEADER_INCLUDED)
#define	fmtc_Matrix_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/CoefArrInt.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fstb/AllocAlign.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "VapourSynth.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include <emmintrin.h>
#endif

#include <vector>

#include <cstdint>



namespace fmtc
{



class Matrix
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Matrix	ThisType;

	explicit       Matrix (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Matrix () {}

	// vsutl::FilterBase
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core);
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);

	static void    select_def_mat (std::string &mat, const ::VSFormat &fmt);
	static fmtcl::ColorSpaceH265
	               find_cs_from_mat_str (const vsutl::FilterBase &flt, const std::string &mat, bool allow_2020cl_flag);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {         NBR_PLANES    = 3  };
	enum {         SHIFT_INT     = 12 };   // Number of bits for the fractional part

	enum Dir
	{
		Dir_IN = 0,
		Dir_OUT,

		Dir_NBR_ELT
	};

	typedef double Mat4 [NBR_PLANES+1] [NBR_PLANES+1];

	const ::VSFormat *
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src, int &plane_out, bool &force_col_fam_flag) const;

	void           config_avx2_matrix_n ();

	template <typename DT, int DB, typename ST, int SB>
	void           apply_matrix_3_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	template <typename DT, int DB, typename ST, int SB>
	void           apply_matrix_1_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <class DST, int DB, class SRC, int SB, int NP>
	void           apply_matrix_n_sse2_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	template <class DST, int DB, class SRC, int SB, int NP>
	void           apply_matrix_n_avx2_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
#endif

	void           apply_matrix_3_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	void           apply_matrix_1_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           apply_matrix_3_sse_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	void           apply_matrix_1_sse_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	void           apply_matrix_3_avx_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	void           apply_matrix_1_avx_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
#endif

	void           prepare_coef_int (const ::VSFormat &fmt_dst, const ::VSFormat &fmt_src, int nbr_expected_coef);
	void           prepare_coef_flt (const ::VSFormat &fmt_dst, const ::VSFormat &fmt_src);
	void           override_fmt_with_csp (::VSFormat &fmt) const;
	void           move_matrix_row ();

	const ::VSFormat *
	               find_dst_col_fam (fmtcl::ColorSpaceH265 tmp_csp, const ::VSFormat *fmt_dst_ptr, const ::VSFormat &fmt_src, ::VSCore &core);
	void           make_mat_from_str (Mat4 &m, const std::string &mat, bool to_rgb_flag) const;

	static void    mul_mat (Mat4 &dst, const Mat4 &src);
	static void    mul_mat (Mat4 &dst, const Mat4 &lhs, const Mat4 &rhs);
	static void    copy_mat (Mat4 &dst, const Mat4 &src);
	static void    make_mat_yuv (Mat4 &m, double kr, double kg, double kb, bool to_rgb_flag);
	static void    make_mat_ycgco (Mat4 &m, bool to_rgb_flag);
	static void		make_mat_flt_int (Mat4 &m, bool to_flt_flag, const ::VSFormat &fmt, bool full_flag);
	static void    complete_mat3 (Mat4 &m);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;          // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;         // Output. Must be declared after _vi_in.

	bool				_sse_flag;
	bool           _sse2_flag;
	bool           _avx_flag;
	bool           _avx2_flag;

	bool           _range_set_src_flag;
	bool           _range_set_dst_flag;
	bool           _full_range_src_flag;
	bool           _full_range_dst_flag;
	Mat4           _mat_main;       // Main matrix, float input, float output
	fmtcl::ColorSpaceH265
	               _csp_out;
	int            _plane_out;      // Plane index for single plane output (0-2), or a negative number if all planes are processed.

	std::vector <float>
	               _coef_flt_arr;

	// Integer coefficients are all scaled with SHIFT_INT.
	// The additive coefficient contains the rounding constant too.
	std::vector <int>
						_coef_int_arr;

	// Same as integer, excepted:
	// Additive coefficients are stored in 4 x 32-bit integers.
	// They may also contain a bias compensating the sign bit flip when the
	// output format is 16 bits.
	fmtcl::CoefArrInt
	               _coef_simd_arr;

	void (ThisType::*
	               _apply_matrix_ptr) (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix ();
	               Matrix (const Matrix &other);
	Matrix &       operator = (const Matrix &other);
	bool           operator == (const Matrix &other) const;
	bool           operator != (const Matrix &other) const;

};	// class Matrix



}	// namespace fmtc



//#include "fmtc/Matrix.hpp"



#endif	// fmtc_Matrix_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
