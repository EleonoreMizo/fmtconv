/*****************************************************************************

        Bitdepth.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Bitdepth_HEADER_INCLUDED)
#define	fmtc_Bitdepth_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "conc/Array.h"
#include "conc/ObjPool.h"
#include "fmtcl/BitBltConv.h"
#include "fmtcl/ErrDifBuf.h"
#include "fmtcl/ErrDifBufFactory.h"
#include "fmtcl/SplFmt.h"
#include "fstb/ArrayAlign.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "vsutl/PlaneProcCbInterface.h"
#include "vsutl/PlaneProcessor.h"
#include "VapourSynth.h"

#include <memory>
#include <vector>



namespace fmtc
{



class Bitdepth
:	public vsutl::FilterBase
,	public vsutl::PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Bitdepth	ThisType;

	explicit       Bitdepth (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Bitdepth () {}

	// vsutl::FilterBase
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core);
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// vsutl::PlaneProcCbInterface
	virtual int    do_process_plane (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {         MAX_NBR_PLANES =     3 };
	enum {         PAT_WIDTH      =     8 };  // Number of pixels for Bayer dithering
	enum {         PAT_PERIOD     =     4 };  // Must be a power of 2 (because cycled with & as modulo)
	enum {         AMP_BITS       =     5 };  // Bit depth of the amplitude fractionnal part. The whole thing is 7 bits, and we need a few bits for the integer part.
	enum {         ERR_RES        =    24 };  // Resolution (bits) of the temporary data for error diffusion when source bitdepth is not high enough (relative to the destination bitdepth) to guarantee an accurate error diffusion.
	enum {         MAX_UNK_WIDTH  = 65536 };  // Maximum width (pixels) for variable formats

	enum DMode
	{
		DMode_ROUND_ALIAS = -1,
		DMode_BAYER = 0,
		DMode_ROUND,      // 1
		DMode_FAST,       // 2
		DMode_FILTERLITE, // 3
		DMode_STUCKI,     // 4
		DMode_ATKINSON,   // 5
		DMode_FLOYD,      // 6
		DMode_OSTRO,      // 7

		DMode_NBR_ELT
	};

	class SclInf
	{
	public:
		fmtcl::BitBltConv::ScaleInfo
		               _info;
		fmtcl::BitBltConv::ScaleInfo *   // 0 if _info is not used.
		               _ptr;
	};

	typedef	int16_t	PatRow [PAT_WIDTH];
	typedef	PatRow	PatData [PAT_WIDTH]; // [y] [x]
	typedef	fstb::ArrayAlign <PatData, PAT_PERIOD, 16>	PatDataArray;

	const ::VSFormat &
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const;

	void           build_dither_pat ();
	void           build_dither_pat_round ();
	void           build_dither_pat_bayer ();
	void           build_next_dither_pat ();
	void           copy_dither_pat_rotate (PatData &dst, const PatData &src, int angle);
	void           init_fnc_fast ();
	void           init_fnc_ordered ();
	void           init_fnc_errdiff ();

	void           dither_plane (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, int src_stride, int w, int h, const fmtcl::BitBltConv::ScaleInfo &scale_info, const PatData &pattern, uint32_t rnd_state);

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_fast_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w) const;
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool S_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
	void           process_seg_fast_int_int_sse (uint8_t *dst_ptr, const uint8_t *src_ptr, int w) const;
#endif
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	void           process_seg_fast_flt_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool S_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
	void           process_seg_fast_flt_int_sse (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;
#endif

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_ord_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const PatRow &pattern, uint32_t &rnd_state) const;
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	void           process_seg_ord_flt_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const PatRow &pattern, uint32_t &rnd_state, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_floydsteinberg_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y) const;
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	void           process_seg_floydsteinberg_flt_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_filterlite_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y) const;
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	void           process_seg_filterlite_flt_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_stucki_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y) const;
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	void           process_seg_stucki_flt_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_atkinson_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y) const;
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	void           process_seg_atkinson_flt_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	void           process_seg_ostromoukhov_int_int_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y) const;

	static inline void
	               generate_rnd (uint32_t &state);
	static inline void
	               generate_rnd_eol (uint32_t &state);

	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	static inline void
	               quantize_pix_int (DST_TYPE *dst_ptr, const SRC_TYPE *src_ptr, int x, int &err, uint32_t &rnd_state, int ampe_i, int ampn_i);
	template <bool S_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	static inline void
	               quantize_pix_flt (DST_TYPE *dst_ptr, const SRC_TYPE *src_ptr, int x, float &err, uint32_t &rnd_state, float ampe_f, float ampn_f, float mul, float add);

	static inline void
	               diffuse_floydsteinberg_int (int err, int &e1, int &e3, int &e5, int &e7);
	static inline void
	               diffuse_floydsteinberg_flt (float err, float &e1, float &e3, float &e5, float &e7);
	static inline void
	               diffuse_filterlite_int (int err, int &e1, int &e2);
	static inline void
	               diffuse_filterlite_flt (float err, float &e1, float &e2);
	static inline void
	               diffuse_stucki_int (int err, int &e1, int &e2, int &e4, int &e8);
	static inline void
	               diffuse_stucki_flt (float err, float &e1, float &e2, float &e4, float &e8);
	static inline void
	               diffuse_atkinson_int (int err, int &e1);
	static inline void
	               diffuse_atkinson_flt (float err, float &e1);
	template <int DIF_BITS>
	static inline void
	               diffuse_ostromoukhov_int (int err, int &e1, int &e2, int &e3, int val);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.

	vsutl::PlaneProcessor
	               _plane_processor;
	fmtcl::SplFmt  _splfmt_src;
	fmtcl::SplFmt  _splfmt_dst;

	conc::Array <SclInf, MAX_NBR_PLANES>
	               _scale_info_arr;
	bool           _upconv_flag;
	bool           _sse2_flag;
	bool           _avx2_flag;
	bool           _full_range_in_flag;
	bool           _full_range_out_flag;

	int            _dmode;
	double         _ampo;
	double         _ampn;
	bool           _dyn_flag;
	bool           _static_noise_flag;

	int            _ampo_i;          // [0 ;  127], 1.0 = 1 << AMP_BITS
	int            _ampn_i;          // [0 ;  127], 1.0 = 1 << AMP_BITS
	int            _ampe_i;          // [0 ; 2047], 1.0 = 256
	float          _ampe_f;
	float          _ampn_f;
	bool           _errdif_flag;     // Indicates a dithering method using error diffusion.
	PatDataArray   _dither_pat_arr;  // Contains levels for ordered dithering

	conc::ObjPool <fmtcl::ErrDifBuf>
						_buf_pool;
	std::unique_ptr <fmtcl::ErrDifBufFactory>
	               _buf_factory_uptr;

	void (ThisType::*
	               _process_seg_fast_int_int_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w) const;
	void (ThisType::*
	               _process_seg_fast_flt_int_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;
	void (ThisType::*
	               _process_seg_ord_int_int_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const PatRow &pattern, uint32_t &rnd_state) const;
	void (ThisType::*
	               _process_seg_ord_flt_int_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, const PatRow &pattern, uint32_t &rnd_state, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;
	void (ThisType::*
	               _process_seg_errdif_int_int_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y) const;
	void (ThisType::*
	               _process_seg_errdif_flt_int_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int w, uint32_t &rnd_state, fmtcl::ErrDifBuf &ed_buf, int y, const fmtcl::BitBltConv::ScaleInfo &scale_info) const;

	typedef	int	OTableEntry [4];  // 3 Coefs + sum

	static const OTableEntry
	               _ostromoukhov_table [256];



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Bitdepth ();
	               Bitdepth (const Bitdepth &other);
	Bitdepth &     operator = (const Bitdepth &other);
	bool           operator == (const Bitdepth &other) const;
	bool           operator != (const Bitdepth &other) const;

};	// class Bitdepth



}	// namespace fmtc



//#include "fmtc/Bitdepth.hpp"



#endif	// fmtc_Bitdepth_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
