/*****************************************************************************

        BitBltConv.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_BitBltConv_HEADER_INCLUDED)
#define	fmtcl_BitBltConv_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtcl/SplFmt.h"

#include <cstdint>



namespace fmtcl
{



class BitBltConv
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	class ScaleInfo
	{
	public:
		double         _gain;
		double         _add_cst;
	};

	explicit       BitBltConv (bool sse2_flag, bool avx2_flag);
	               BitBltConv (const BitBltConv &other);
	virtual        ~BitBltConv () {}

	BitBltConv &   operator = (const BitBltConv &other);

	void           bitblt (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, const uint8_t *src_lsb_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr = 0);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	void           bitblt_int_to_flt (uint8_t *dst_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, const uint8_t *src_lsb_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
	void           bitblt_flt_to_int (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, const uint8_t *src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
	void           bitblt_int_to_int (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, const uint8_t *src_lsb_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);

#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           bitblt_int_to_flt_avx2_switch (uint8_t *dst_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, const uint8_t *src_lsb_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
	void           bitblt_flt_to_int_avx2_switch (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, const uint8_t *src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
	void           bitblt_int_to_int_avx2_switch (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, const uint8_t *src_lsb_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
#endif

	static void    bitblt_same_fmt (fmtcl::SplFmt fmt, uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, const uint8_t *src_ptr, const uint8_t *src_lsb_ptr, int src_stride, int w, int h);
	static void    bitblt_i08_to_s16 (uint8_t *dst_ptr, uint8_t *dst_lsb_ptr, int dst_stride, const uint8_t *src_ptr, int src_stride, int w, int h);

	template <bool SF, class SRC, int SBD>
	static void    bitblt_int_to_flt_cpp (uint8_t *dst_ptr, int dst_stride, typename SRC::PtrConst::Type src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool SF, class SRC, int SBD>
	static void    bitblt_int_to_flt_sse2 (uint8_t *dst_ptr, int dst_stride, typename SRC::PtrConst::Type src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
	template <bool SF, class SRC, int SBD>
	static void    bitblt_int_to_flt_avx2 (uint8_t *dst_ptr, int dst_stride, typename SRC::PtrConst::Type src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
#endif

	template <bool SF, class DST>
	static void    bitblt_flt_to_int_cpp (typename DST::Ptr::Type dst_ptr, int dst_stride, const uint8_t *src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool SF, class DST>
	static void    bitblt_flt_to_int_sse2 (typename DST::Ptr::Type dst_ptr, int dst_stride, const uint8_t *src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
	template <bool SF, class DST>
	static void    bitblt_flt_to_int_avx2 (typename DST::Ptr::Type dst_ptr, int dst_stride, const uint8_t *src_ptr, int src_stride, int w, int h, const ScaleInfo *scale_info_ptr);
#endif

	template <class DST, class SRC, int DBD, int SBD>
	static void    bitblt_ixx_to_x16_cpp (typename DST::Ptr::Type dst_ptr, int dst_stride, typename SRC::PtrConst::Type src_ptr, int src_stride, int w, int h);
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <class DST, class SRC, int DBD, int SBD>
	static void    bitblt_ixx_to_x16_sse2 (typename DST::Ptr::Type dst_ptr, int dst_stride, typename SRC::PtrConst::Type src_ptr, int src_stride, int w, int h);
	template <class DST, class SRC, int DBD, int SBD>
	static void    bitblt_ixx_to_x16_avx2 (typename DST::Ptr::Type dst_ptr, int dst_stride, typename SRC::PtrConst::Type src_ptr, int src_stride, int w, int h);
#endif

	static bool    is_si_neutral (const ScaleInfo *scale_info_ptr);

	bool           _sse2_flag;
	bool           _avx2_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               BitBltConv ();
	bool           operator == (const BitBltConv &other) const;
	bool           operator != (const BitBltConv &other) const;

};	// class BitBltConv



}	// namespace fmtcl



//#include "fmtcl/BitBltConv.hpp"



#endif	// fmtcl_BitBltConv_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
