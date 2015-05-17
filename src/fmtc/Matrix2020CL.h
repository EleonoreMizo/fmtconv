/*****************************************************************************

        Matrix2020CL.h
        Author: Laurent de Soras, 2013

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtc_Matrix2020CL_HEADER_INCLUDED)
#define	fmtc_Matrix2020CL_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "VapourSynth.h"

#include <cstdint>



namespace fmtc
{



class Matrix2020CL
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Matrix2020CL	ThisType;

	explicit       Matrix2020CL (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Matrix2020CL () {}

	// vsutl::FilterBase
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core);
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);



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

	enum {         NBR_PLANES    = 3  };
	enum {         SHIFT_INT     = 12 };   // Number of bits for the fractional part
	enum {         VECT_LEN      = 16 / sizeof (int16_t) };
	enum {         RGB_INT_BITS  = 16 };

	const ::VSFormat &
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const;
	void           setup_rgb_2_ycbcr ();
	void           setup_ycbcr_2_rgb ();

	template <typename DT, int DB>
	void           conv_rgb_2_ycbcr_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	void           conv_rgb_2_ycbcr_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);

	template <typename ST, int SB>
	void           conv_ycbcr_2_rgb_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);
	void           conv_ycbcr_2_rgb_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);

	template <typename T>
	static fstb_FORCEINLINE T
	               map_lin_to_gam (T v_lin, bool b12_flag);
	template <typename T>
	static fstb_FORCEINLINE T
	               map_gam_to_lin (T v_gam, bool b12_flag);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;          // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;         // Output. Must be declared after _vi_in.

	bool				_sse_flag;
	bool           _sse2_flag;

	bool           _range_set_flag;
	bool           _full_range_flag;

	bool           _b12_flag;
	bool           _to_yuv_flag;
	bool           _flt_flag;

	int16_t        _coef_rgby_int [NBR_PLANES];
	uint16_t       _map_gamma_int [1 << RGB_INT_BITS];
	uint16_t       _coef_yg_a_int;
	int32_t        _coef_yg_b_int;
	uint16_t       _coef_cb_a_int [2];  // 0: cb >= 0, 1: cb < 0
	uint16_t       _coef_cr_a_int [2];
	int32_t        _coef_cbcr_b_int;

	void (ThisType::*
	               _apply_matrix_ptr) (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h);

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

	               Matrix2020CL ();
	               Matrix2020CL (const Matrix2020CL &other);
	Matrix2020CL & operator = (const Matrix2020CL &other);
	bool           operator == (const Matrix2020CL &other) const;
	bool           operator != (const Matrix2020CL &other) const;

};	// class Matrix2020CL



}	// namespace fmtc



//#include "fmtc/Matrix2020CL.hpp"



#endif	// fmtc_Matrix2020CL_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
