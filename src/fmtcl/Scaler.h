/*****************************************************************************

        Scaler.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Scaler_HEADER_INCLUDED)
#define	fmtcl_Scaler_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtcl/Proxy.h"
#include "fmtcl/CoefArrInt.h"
#include "fstb/AllocAlign.h"

#include <vector>

#include <cstdint>



// Define this symbol for fast integer SSE2 calculations (~13 bit accuracy, low headroom)
// Undef this symbol for accurate but slow SSE2 calculations
// Note: acutally this isn't even faster. Don't use it.
#undef fmtcl_Scaler_SSE2_16BITS



// All possible bitdepth conversions for the floating point path.
// No "to 8 bit" path (poor quality because of the rounding)
// 9 and 10 bits are treated as 16 bit but you shoudln't do any conversion
// to 9 or 10 bits because of the afore-mentioned rounding issue.
// Proxy, SplFmt (, actual bitdepth), name
// Order: Dest, Src
#define fmtcl_Scaler_SPAN_F(MC) \
	MC (Float  , Float  , FLOAT  , FLOAT  , f32_f32) \
\
	MC (Float  , Int8   , FLOAT  , INT8   , f32_i08) \
	MC (Float  , Int16  , FLOAT  , INT16  , f32_i16) \
	MC (Float  , Stack16, FLOAT  , STACK16, f32_s16) \
\
	MC (Int16  , Float  , INT16  , FLOAT  , i16_f32) \
	MC (Stack16, Float  , STACK16, FLOAT  , s16_f32) \
\
	MC (Int16  , Int16  , INT16  , INT16  , i16_i16) \
	MC (Stack16, Int16  , STACK16, INT16  , s16_i16) \
	MC (Int16  , Stack16, INT16  , STACK16, i16_s16) \
	MC (Stack16, Stack16, STACK16, STACK16, s16_s16) \
\
	MC (Int16  , Int8   , INT16  , INT8   , i16_i08) \
	MC (Stack16, Int8   , STACK16, INT8   , s16_i08)

// Same, integer path. There is no conversion involving float.
// Decreasing the bitdepth is technically possible just by
// adding lines but avoided here too.
#define fmtcl_Scaler_SPAN_I(MC) \
	MC (Int16  , Int16  , INT16  , INT16  , 16, 16, i16_i16) \
	MC (Stack16, Int16  , STACK16, INT16  , 16, 16, s16_i16) \
	MC (Int16  , Stack16, INT16  , STACK16, 16, 16, i16_s16) \
	MC (Stack16, Stack16, STACK16, STACK16, 16, 16, s16_s16) \
\
	MC (Int16  , Int16  , INT16  , INT16  , 16, 14, i16_i14) \
	MC (Stack16, Int16  , STACK16, INT16  , 16, 14, s16_i14) \
\
	MC (Int16  , Int16  , INT16  , INT16  , 16, 12, i16_i12) \
	MC (Stack16, Int16  , STACK16, INT16  , 16, 12, s16_i12) \
\
	MC (Int16  , Int16  , INT16  , INT16  , 16, 10, i16_i10) \
	MC (Stack16, Int16  , STACK16, INT16  , 16, 10, s16_i10) \
\
	MC (Int16  , Int16  , INT16  , INT16  , 16,  9, i16_i09) \
	MC (Stack16, Int16  , STACK16, INT16  , 16,  9, s16_i09) \
\
	MC (Int16  , Int8   , INT16  , INT8   , 16,  8, i16_i08) \
	MC (Stack16, Int8   , STACK16, INT8   , 16,  8, s16_i08)



namespace fmtcl
{



class ContFirInterface;

class Scaler
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Scaler	ThisType;

	static const int  SRC_ALIGN   = 16; // Pixels

#if defined (fmtcl_Scaler_SSE2_16BITS)
	static const int  SHIFT_INT   = 14; // Number of bits for the fractional part
#else
	static const int  SHIFT_INT   = 12; // Number of bits for the fractional part
#endif   // fmtcl_Scaler_SSE2_16BITS

	explicit       Scaler (int src_height, int dst_height, double win_top, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, bool norm_flag, double norm_val, double center_pos_src, double center_pos_dst, double gain, double add_cst, bool int_flag, bool sse2_flag, bool avx2_flag);
	virtual        ~Scaler () {}

	void           get_src_boundaries (int &y_src_beg, int &y_src_end, int y_dst_beg, int y_dst_end) const;
	int            get_fir_len () const;

#define fmtcl_Scaler_DECLARE_F(DT, ST, DE, SE, FN) \
	void           process_plane_flt (Proxy::Ptr##DT::Type dst_ptr, Proxy::Ptr##ST##Const::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

#define fmtcl_Scaler_DECLARE_I(DT, ST, DE, SE, DB, SB, FN) \
	void           process_plane_int_##FN (Proxy::Ptr##DT::Type dst_ptr, Proxy::Ptr##ST##Const::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

	fmtcl_Scaler_SPAN_F (fmtcl_Scaler_DECLARE_F)
	fmtcl_Scaler_SPAN_I (fmtcl_Scaler_DECLARE_I)

#undef fmtcl_Scaler_DECLARE_F
#undef fmtcl_Scaler_DECLARE_I

	static void    eval_req_src_area (int &work_top, int &work_height, int src_height, int dst_height, double win_top, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, double center_pos_src, double center_pos_dst);
	static int     eval_lower_bound_of_dst_tile_height (int tile_height_src, int dst_height, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, int src_height);
	static int     eval_lower_bound_of_src_tile_height (int tile_height_dst, int dst_height, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, int src_height);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	class BasicInfo
	{
	public:
		               BasicInfo (int src_height, int dst_height, double win_top, double win_height, ContFirInterface &kernel_fnc, double kernel_scale, double center_pos_src, double center_pos_dst);

		double         _src_step;
		double         _zc_size;
		double         _imp_step;
		double         _support;
		double         _src_pos;
		int            _fir_len;
	};

	class KernelInfo
	{
	public:
		int            _start_line;	   // Y position of the first line of the kernel
		int            _coef_index;
		int            _kernel_size;
		bool           _copy_flt_flag;
		bool           _copy_int_flag;
	};

#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           setup_avx2 ();
#endif

	template <class DST, class SRC>
	void           process_plane_flt_cpp (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

	template <class DST, int DB, class SRC, int SB>
	void           process_plane_int_cpp (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

#if (fstb_ARCHI == fstb_ARCHI_X86)

	template <class DST, class SRC>
	void           process_plane_flt_sse2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

	template <class DST, int DB, class SRC, int SB>
	void           process_plane_int_sse2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

	template <class DST, class SRC>
	void           process_plane_flt_avx2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

	template <class DST, int DB, class SRC, int SB>
	void           process_plane_int_avx2 (typename DST::Ptr::Type dst_ptr, typename SRC::PtrConst::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

#endif   // fstb_ARCHI_X86

	void           build_scale_data ();
	void           push_back_int_coef (double coef);

	int            _src_height;
	int            _dst_height;
	double         _win_top;
	double         _win_height;
	double         _kernel_scale;
	ContFirInterface &
	               _kernel_fnc;
	bool           _can_int_flag;       // Indicates that integer processing is allowed.
	bool           _norm_flag;
	double         _norm_val;
	double         _center_pos_src;
	double         _center_pos_dst;
	double         _gain;
	double         _add_cst_flt;
	int32_t        _add_cst_int;
	int            _fir_len;

	std::vector <KernelInfo>            // For each destination line
	               _kernel_info_arr;
	std::vector <float, fstb::AllocAlign <float, 16> > // All kernel coefs, for all lines.
	               _coef_flt_arr;       // Beware, kernels may not be contiguous.
	CoefArrInt     _coef_int_arr;       // Same here

#define fmtcl_Scaler_FNCPTR_F(DT, ST, DE, SE, FN) \
	void (ThisType::* \
	               _process_plane_flt_##FN##_ptr) (Proxy::Ptr##DT::Type dst_ptr, Proxy::Ptr##ST##Const::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

#define fmtcl_Scaler_FNCPTR_I(DT, ST, DE, SE, DB, SB, FN) \
	void (ThisType::* \
	               _process_plane_int_##FN##_ptr) (Proxy::Ptr##DT::Type dst_ptr, Proxy::Ptr##ST##Const::Type src_ptr, int dst_stride, int src_stride, int width, int y_dst_beg, int y_dst_end) const;

	fmtcl_Scaler_SPAN_F (fmtcl_Scaler_FNCPTR_F)
	fmtcl_Scaler_SPAN_I (fmtcl_Scaler_FNCPTR_I)

#undef fmtcl_Scaler_FNCPTR_F
#undef fmtcl_Scaler_FNCPTR_I



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Scaler ()                               = delete;
	               Scaler (const Scaler &other)            = delete;
	Scaler &       operator = (const Scaler &other)        = delete;
	bool           operator == (const Scaler &other) const = delete;
	bool           operator != (const Scaler &other) const = delete;

};	// class Scaler



}	// namespace fmtcl



//#include "fmtcl/Scaler.hpp"



#endif	// fmtcl_Scaler_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
