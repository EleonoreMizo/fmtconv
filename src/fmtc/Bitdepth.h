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

#include "conc/ObjPool.h"
#include "fmtcl/BitBltConv.h"
#include "fmtcl/ErrDifBuf.h"
#include "fmtcl/ErrDifBufFactory.h"
#include "fmtcl/SplFmt.h"
#include "fstb/def.h"
#include "fstb/ArrayAlign.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "vsutl/PlaneProcCbInterface.h"
#include "vsutl/PlaneProcessor.h"
#include "VapourSynth.h"

#include <array>
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

	static const int  MAX_NBR_PLANES =     3;
	static const int  PAT_WIDTH      =    32; // Number of pixels for halftone dithering
	static const int  PAT_PERIOD     =     4; // Must be a power of 2 (because cycled with & as modulo)
	static const int  AMP_BITS       =     5; // Bit depth of the amplitude fractionnal part. The whole thing is 7 bits, and we need a few bits for the integer part.
	static const int  ERR_RES        =    24; // Resolution (bits) of the temporary data for error diffusion when source bitdepth is not high enough (relative to the destination bitdepth) to guarantee an accurate error diffusion.
	static const int  MAX_UNK_WIDTH  = 65536; // Maximum width (pixels) for variable formats

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
		DMode_VOIDCLUST,  // 8
		DMode_QUASIRND,   // 9

		DMode_NBR_ELT
	};

	class SclInf
	{
	public:
		fmtcl::BitBltConv::ScaleInfo
		               _info;
		fmtcl::BitBltConv::ScaleInfo *   // 0 if _info is not used.
		               _ptr = 0;
	};

	typedef	int16_t	PatRow [PAT_WIDTH];  // Contains data in [-128; +127]
	typedef	PatRow	PatData [PAT_WIDTH]; // [y] [x]
	typedef	fstb::ArrayAlign <PatData, PAT_PERIOD, 16>	PatDataArray;

	class AmpInfo
	{
	public:
		int            _o_i = 0;   // [0 ;  127], 1.0 = 1 << AMP_BITS
		int            _n_i = 0;   // [0 ;  127], 1.0 = 1 << AMP_BITS
		int            _e_i = 0;   // [0 ; 2047], 1.0 = 256
		float          _e_f = 0;
		float          _n_f = 0;
	};

	class SegContext
	{
	public:
		inline const PatRow &
		               extract_pattern_row () const noexcept;
		const PatData* _pattern_ptr = nullptr; // Ordered dithering
		uint32_t       _rnd_state   = 0;       // Anything excepted fast mode
		const fmtcl::BitBltConv::ScaleInfo *   // Float processing
		               _scale_info_ptr = nullptr;
		fmtcl::ErrDifBuf *                     // Error diffusion
		               _ed_buf_ptr  = nullptr;
		int            _y           = -1;      // Ordered dithering and error diffusion
		uint32_t       _qrs_seed    = 0;       // For the quasirandom sequences
		AmpInfo        _amp;
	};

	const ::VSFormat &
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const;

	void           build_dither_pat ();
	void           build_dither_pat_round ();
	void           build_dither_pat_bayer ();
	void           build_dither_pat_void_and_cluster (int w);
	void           build_next_dither_pat ();
	void           copy_dither_pat_rotate (PatData &dst, const PatData &src, int angle) noexcept;
	void           init_fnc_fast () noexcept;
	void           init_fnc_ordered () noexcept;
	void           init_fnc_quasirandom () noexcept;
	void           init_fnc_errdiff () noexcept;

	void           dither_plane (fmtcl::SplFmt dst_fmt, int dst_res, uint8_t *dst_ptr, int dst_stride, fmtcl::SplFmt src_fmt, int src_res, const uint8_t *src_ptr, int src_stride, int w, int h, const fmtcl::BitBltConv::ScaleInfo &scale_info, int frame_index, int plane_index);

	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	static void    process_seg_fast_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &/*ctx*/) noexcept;
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	static void    process_seg_fast_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
	static void    process_seg_fast_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &/*ctx*/) noexcept;
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
	static void    process_seg_fast_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
#endif

	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	static void    process_seg_ord_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	static void    process_seg_ord_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
	static void    process_seg_ord_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
	static void    process_seg_ord_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
#endif

	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	static void    process_seg_qrs_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	static void    process_seg_qrs_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS>
	static void    process_seg_qrs_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
	template <bool S_FLAG, bool TO_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT>
	static void    process_seg_qrs_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
#endif

	template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS, typename DFNC>
	static fstb_FORCEINLINE void
	               process_seg_common_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept;
	template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, typename DFNC>
	static fstb_FORCEINLINE void
	               process_seg_common_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept;
	template <bool T_FLAG>
	static fstb_FORCEINLINE int
	               generate_dith_n_scalar (uint32_t &rnd_state) noexcept;
	static fstb_FORCEINLINE int
	               remap_tpdf_scalar (int d) noexcept;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <bool S_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, int SRC_BITS, typename DFNC>
	static fstb_FORCEINLINE void
	               process_seg_common_int_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept;
	template <bool S_FLAG, bool TN_FLAG, fmtcl::SplFmt DST_FMT, int DST_BITS, fmtcl::SplFmt SRC_FMT, typename DFNC>
	static fstb_FORCEINLINE void
	               process_seg_common_flt_int_sse2 (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx, DFNC dither_fnc) noexcept;
	template <bool T_FLAG>
	static fstb_FORCEINLINE __m128i
	               generate_dith_n_vec (uint32_t &rnd_state) noexcept;
	static fstb_FORCEINLINE __m128i
	               remap_tpdf_vec (__m128i d) noexcept;
#endif

	template <bool S_FLAG, bool TN_FLAG, class ERRDIF>
	static void    process_seg_errdif_int_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;
	template <bool S_FLAG, bool TN_FLAG, class ERRDIF>
	static void    process_seg_errdif_flt_int_cpp (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx) noexcept;

	static inline void
	               generate_rnd (uint32_t &state) noexcept;
	static inline void
	               generate_rnd_eol (uint32_t &state) noexcept;

	template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	static inline void
	               quantize_pix_int (DST_TYPE * fstb_RESTRICT dst_ptr, const SRC_TYPE * fstb_RESTRICT src_ptr, SRC_TYPE &src_raw, int x, int & fstb_RESTRICT err, uint32_t &rnd_state, int ampe_i, int ampn_i) noexcept;
	template <bool S_FLAG, bool TN_FLAG, class DST_TYPE, int DST_BITS, class SRC_TYPE>
	static inline void
	               quantize_pix_flt (DST_TYPE * fstb_RESTRICT dst_ptr, const SRC_TYPE * fstb_RESTRICT src_ptr, SRC_TYPE &src_raw, int x, float & fstb_RESTRICT err, uint32_t &rnd_state, float ampe_f, float ampn_f, float mul, float add) noexcept;

	template <class DT, int DB, class ST, int SB, int EL>
	class ErrDifAddParam
	{
	public:
		typedef DT DstType;
		typedef ST SrcType;
		static const int  DST_BITS      = DB;
		static const int  SRC_BITS      = SB;
		static const int  NBR_ERR_LINES = EL;
	};

	template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	class DiffuseFloydSteinberg
	:	public ErrDifAddParam <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS, 1>
	{
	public:
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <typename EB>
		static fstb_FORCEINLINE void
		               prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept;
	private:
		template <int DIR, typename ET, typename EB>
		static fstb_FORCEINLINE void
		               spread_error (ET e1, ET e3, ET e5, ET e7, ET & fstb_RESTRICT err_nxt0, EB * fstb_RESTRICT err0_ptr) noexcept;
	};

	template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	class DiffuseFilterLite
	:	public ErrDifAddParam <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS, 1>
	{
	public:
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <typename EB>
		static fstb_FORCEINLINE void
		               prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept;
	private:
		template <int DIR, typename ET, typename EB>
		static fstb_FORCEINLINE void
		               spread_error (ET e1, ET e2, ET & fstb_RESTRICT err_nxt0, EB * fstb_RESTRICT err0_ptr) noexcept;
	};

	template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	class DiffuseStucki
	:	public ErrDifAddParam <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS, 2>
	{
	public:
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <typename EB>
		static fstb_FORCEINLINE void
		               prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept;
	private:
		template <int DIR, typename ET, typename EB>
		static fstb_FORCEINLINE void
		               spread_error (ET e1, ET e2, ET e4, ET e8, ET & fstb_RESTRICT err_nxt0, ET & fstb_RESTRICT err_nxt1, EB * fstb_RESTRICT err0_ptr, EB * fstb_RESTRICT err1_ptr) noexcept;
	};

	template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	class DiffuseAtkinson
	:	public ErrDifAddParam <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS, 2>
	{
	public:
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <typename EB>
		static fstb_FORCEINLINE void
		               prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept;
	private:
		template <int DIR, typename ET, typename EB>
		static fstb_FORCEINLINE void
		               spread_error (ET e1, ET & fstb_RESTRICT err_nxt0, ET & fstb_RESTRICT err_nxt1, EB * fstb_RESTRICT err0_ptr, EB * fstb_RESTRICT err1_ptr) noexcept;
	};

	class DiffuseOstromoukhovBase
	{
	public:
		struct TableEntry
		{
			int            _c0;
			int            _c1;
			int            _c2;        // Actually not used
			int            _sum;
			float          _inv_sum;   // Possible optimization: store 1/_c0 and 1/_c1 instead of this field.
		};
		static const int  T_BITS = 8;
		static const int  T_LEN  = 1 << T_BITS;
		static const int  T_MASK = T_LEN - 1;

		static const std::array <TableEntry, T_LEN>
		               _table;
	};

	template <int DST_BITS, int SRC_BITS>
	class DiffuseOstromoukhovBase2
	:	public DiffuseOstromoukhovBase
	{
	public:
		template <class SRC_TYPE>
		static inline int
		               get_index (SRC_TYPE src_raw) noexcept;
		static inline int
		               get_index (float src_raw) noexcept;
	};

	template <class DST_TYPE, int DST_BITS, class SRC_TYPE, int SRC_BITS>
	class DiffuseOstromoukhov
	:	public ErrDifAddParam <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS, 1>
	,	public DiffuseOstromoukhovBase2 <DST_BITS, SRC_BITS>
	{
	public:
		typedef DiffuseOstromoukhov <DST_TYPE, DST_BITS, SRC_TYPE, SRC_BITS> ThisType;
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (int err, int & fstb_RESTRICT err_nxt0, int & fstb_RESTRICT err_nxt1, int16_t * fstb_RESTRICT err0_ptr, int16_t * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <int DIR>
		static fstb_FORCEINLINE void
		               diffuse (float err, float & fstb_RESTRICT err_nxt0, float & fstb_RESTRICT err_nxt1, float * fstb_RESTRICT err0_ptr, float * fstb_RESTRICT err1_ptr, SRC_TYPE src_raw) noexcept;
		template <typename EB>
		static fstb_FORCEINLINE void
		               prepare_next_line (EB * fstb_RESTRICT err_ptr) noexcept;
	private:
		template <int DIR, typename ET, typename EB>
		static fstb_FORCEINLINE void
		               spread_error (ET e1, ET e2, ET e3, ET & fstb_RESTRICT err_nxt0, EB * fstb_RESTRICT err0_ptr) noexcept;
	};

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.

	vsutl::PlaneProcessor
	               _plane_processor;
	fmtcl::SplFmt  _splfmt_src;
	fmtcl::SplFmt  _splfmt_dst;

	std::array <SclInf, MAX_NBR_PLANES>
	               _scale_info_arr;
	bool           _upconv_flag;
	bool           _sse2_flag;
	bool           _avx2_flag;
	bool           _full_range_in_flag;
	bool           _full_range_out_flag;
	bool           _range_def_flag;

	int            _dmode;
	int            _pat_size;        // Must be a divisor of PAT_WIDTH
	double         _ampo;
	double         _ampn;
	bool           _dyn_flag;
	bool           _static_noise_flag;
	bool           _correlated_planes_flag;
	bool           _tpdfo_flag;
	bool           _tpdfn_flag;

	bool           _errdif_flag;     // Indicates a dithering method using error diffusion.
	bool           _simple_flag;     // Simplified implementation for ampo == 1 and ampn == 0
	PatDataArray   _dither_pat_arr;  // Contains levels for ordered dithering

	AmpInfo        _amp;

	conc::ObjPool <fmtcl::ErrDifBuf>
						_buf_pool;
	std::unique_ptr <fmtcl::ErrDifBufFactory>
	               _buf_factory_uptr;

	void (*        _process_seg_int_int_ptr) (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx);
	void (*        _process_seg_flt_int_ptr) (uint8_t * fstb_RESTRICT dst_ptr, const uint8_t * fstb_RESTRICT src_ptr, int w, SegContext &ctx);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Bitdepth ()                               = delete;
	               Bitdepth (const Bitdepth &other)          = delete;
	Bitdepth &     operator = (const Bitdepth &other)        = delete;
	bool           operator == (const Bitdepth &other) const = delete;
	bool           operator != (const Bitdepth &other) const = delete;

};	// class Bitdepth



}	// namespace fmtc



//#include "fmtc/Bitdepth.hpp"



#endif	// fmtc_Bitdepth_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
