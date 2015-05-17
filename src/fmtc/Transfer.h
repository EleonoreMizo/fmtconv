/*****************************************************************************

        Transfer.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Transfer_HEADER_INCLUDED)
#define	fmtc_Transfer_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/TransCurve.h"
#include "fstb/AllocAlign.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "vsutl/PlaneProcCbInterface.h"
#include "vsutl/PlaneProcessor.h"
#include "VapourSynth.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include <emmintrin.h>
	#include <immintrin.h>
#endif

#include <vector>

#include <cstdint>



namespace fmtc
{



class Transfer
:	public vsutl::FilterBase
,	public vsutl::PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Transfer	ThisType;

	explicit       Transfer (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Transfer () {}

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

	enum Dir
	{
		Dir_IN = 0,
		Dir_OUT,

		Dir_NBR_ELT
	};

	enum {         LINLUT_RES_L2  = 16   }; // log2 of the linear table resolution (size of a unity segment)
	enum {         LINLUT_MIN_F   = -1   }; // Min value for float LUTs
	enum {         LINLUT_MAX_F   = 2    }; // Max value for float LUTs
	enum {         LINLUT_SIZE_F  = ((LINLUT_MAX_F - LINLUT_MIN_F) << LINLUT_RES_L2) + 1 };

	// Log LUT are only for floating point input
	enum {         LOGLUT_MIN_L2  = -32  }; // log2(x) for the first index in a log table (negative)
	enum {         LOGLUT_MAX_L2  = 16   }; // log2(x) for the last index in a log table (positive)
	enum {         LOGLUT_RES_L2  = 10   }; // log2 of the log table resolution (size of each [x ; 2*x[ segment).
	enum {         LOGLUT_HSIZE   = ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2) + 1 }; // Table made of half-open segments (and whitout x=0) + 1 more value for LOGLUT_MAX, closing the last segment.
	enum {         LOGLUT_SIZE    = 2 * LOGLUT_HSIZE + 1 };   // Negative + 0 + positive

	class MultiTypeArray
	{
	public:
		               MultiTypeArray ();
		template <class T>
		void           set_type ();
		void           resize (size_t length);
		template <class T>
		fstb_FORCEINLINE T &
			            use (int pos);
		template <class T>
		fstb_FORCEINLINE const T &
		               use (int pos) const;
		inline size_t  get_size () const;
	private:
		std::vector <uint8_t>
		               _arr;
		size_t         _length;
		int            _data_len;     // Element size in bytes. 0 = not set
	};

	// It is the operator responsibility to clip the input and output
	// (input domain or spec requirement).
	class TransOpInterface
	{
	public:
		virtual        ~TransOpInterface () {}
		virtual double operator () (double x) const = 0;
	};

	typedef  std::shared_ptr <Transfer::TransOpInterface> OpSPtr;

	class TransOpBypass
	:	public TransOpInterface
	{
	public:
		virtual double operator () (double x) const { return (x); }
	};

	class TransOpCompose
	:	public TransOpInterface
	{
	public:
		explicit       TransOpCompose (OpSPtr op_1_sptr, OpSPtr op_2_sptr);
		virtual double operator () (double x) const;
	private:
		OpSPtr         _op_1_sptr;
		OpSPtr         _op_2_sptr;
	};

	class TransOpContrast
	:	public TransOpInterface
	{
	public:
		explicit       TransOpContrast (double cont);
		virtual double operator () (double x) const;
	private:
		double         _cont;
	};

	class TransOpLinPow
	:	public TransOpInterface
	{
	public:
		//  beta         <= L                 : V =    alpha * pow ( L        , p1) - (alpha - 1)
		// -beta / scneg <  L <   beta        : V =            pow ( L * slope, p2)
		//                  L <= -beta / scneg: V = - (alpha * pow (-L * scneg, p1) - (alpha - 1)) / scneg
		explicit       TransOpLinPow (bool inv_flag, double alpha, double beta, double p1, double slope, double lb = 0, double ub = 1, double scneg = 1, double p2 = 1);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
		const double   _alpha;
		const double   _beta;
		const double   _p1;
		const double   _slope;
		const double   _lb;
		const double   _ub;
		const double   _scneg;
		const double   _p2;
		double         _alpha_m1;
		double         _beta_n;
		double         _beta_i;
		double         _beta_in;
		double         _lb_i;
		double         _ub_i;
		double         _p1_i;
		double         _p2_i;
	};

	class TransOpLogTrunc
	:	public TransOpInterface
	{
	public:
		explicit       TransOpLogTrunc (bool inv_flag, double alpha, double beta);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
		const double   _alpha;
		const double   _beta;
	};

	class TransOpPow
	:	public TransOpInterface
	{
	public:
		explicit       TransOpPow (bool inv_flag, double p_i, double alpha = 1, double val_max = 1);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
		const double   _p_i;
		const double   _alpha;
		const double   _p;
		const double   _val_max;	// linear
	};

	class TransOp2084
	:	public TransOpInterface
	{
	public:
		explicit       TransOp2084 (bool inv_flag);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
	};

	class TransOpFilmStream
	:	public TransOpInterface
	{
	public:
		explicit       TransOpFilmStream (bool inv_flag);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
	};

	class TransOpSLog
	:	public TransOpInterface
	{
	public:
		explicit       TransOpSLog (bool inv_flag);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
	};

	class TransOpLogC
	:	public TransOpInterface
	{
	public:
		explicit       TransOpLogC (bool inv_flag, bool v2_flag);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
		const bool     _v2_flag;
		double         _cut;
		double         _a;
		double         _b;
		double         _c;
		double         _d;
		double         _e;
		double         _f;
		double         _cut_i;
	};

	class TransOpCanonLog
	:	public TransOpInterface
	{
	public:
		explicit       TransOpCanonLog (bool inv_flag);
		virtual double operator () (double x) const;
	private:
		const bool     _inv_flag;
	};

	union FloatIntMix
	{
		float          _f;
		uint32_t       _i;
	};

	class MapperLin
	{
	public:
		explicit       MapperLin (int lut_size, double range_beg, double range_lst);
		inline int     get_lut_size () const { return (_lut_size); }
		inline double  find_val (int index) const;
		static inline void
		               find_index (const FloatIntMix &val, int &index, float &frac);
#if (fstb_ARCHI == fstb_ARCHI_X86)
		static inline void
		               find_index (const FloatIntMix val_arr [4], __m128i &index, __m128 &frac);
		static inline void
		               find_index (const FloatIntMix val_arr [8], __m256i &index, __m256 &frac);
#endif
	private:
		int            _lut_size;
		double         _range_beg;
		double         _range_lst;
		double         _step;
	};

	class MapperLog
	{
	public:
		inline int     get_lut_size () const { return (LOGLUT_SIZE); }
		inline double  find_val (int index) const;
		static inline void
		               find_index (const FloatIntMix &val, int &index, float &frac);
#if (fstb_ARCHI == fstb_ARCHI_X86)
		static inline void
		               find_index (const FloatIntMix val_arr [4], __m128i &index, __m128 &frac);
		static inline void
		               find_index (const FloatIntMix val_arr [8], __m256i &index, __m256 &frac);
#endif
	};

	template <class T>
	class Convert
	{
	public:
		static inline T
		               cast (float val);
#if (fstb_ARCHI == fstb_ARCHI_X86)
		static inline void
		               store_sse2 (T *dst_ptr, __m128 val);
		static inline void
		               store_avx2 (T *dst_ptr, __m256 val);
#endif
	};

	const ::VSFormat &
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const;

	void           init_table ();
	void           init_proc_fnc ();
#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           init_proc_fnc_avx2 (int selector);
#endif
	void           generate_lut (const TransOpInterface &curve);
	template <class T>
	void           generate_lut_int (const TransOpInterface &curve, int lut_size, double range_beg, double range_lst, double mul, double add);
	template <class T, class M>
	void           generate_lut_flt (const TransOpInterface &curve, const M &mapper);

	template <class TS, class TD>
	void           process_plane_int_any_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h);
	template <class TD, class M>
	void           process_plane_flt_any_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h);
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <class TD, class M>
	void           process_plane_flt_any_sse2 (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h);
	template <class TD, class M>
	void           process_plane_flt_any_avx2 (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h);
#endif

	static fmtcl::TransCurve
	               conv_string_to_curve (const vsutl::FilterBase &flt, const std::string &str);
	static OpSPtr  conv_curve_to_op (fmtcl::TransCurve c, bool inv_flag);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;          // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;         // Output. Must be declared after _vi_in.

	bool           _sse2_flag;
	bool           _avx2_flag;
	std::string    _transs;
	std::string    _transd;
	double         _contrast;
	double         _gcor;
	bool           _full_range_src_flag;
	bool           _full_range_dst_flag;
	fmtcl::TransCurve
	               _curve_s;
	fmtcl::TransCurve
	               _curve_d;
	bool           _loglut_flag;

	vsutl::PlaneProcessor
	               _plane_processor;

	void (Transfer:: *
	               _process_plane_ptr) (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h);

	MultiTypeArray _lut;            // Opaque array, contains uint8_t, uint16_t or float depending on the output datatype. Table size is always 256, 65536 or 65536*3+1 (float input, covering -1 to +2 range).



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Transfer ();
	               Transfer (const Transfer &other);
	Transfer &     operator = (const Transfer &other);
	bool           operator == (const Transfer &other) const;
	bool           operator != (const Transfer &other) const;

};	// class Transfer



}	// namespace fmtc



#include "fmtc/Transfer.hpp"



#endif	// fmtc_Transfer_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
