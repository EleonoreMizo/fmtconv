/*****************************************************************************

        TransLut.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransLut_HEADER_INCLUDED)
#define	fmtcl_TransLut_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtcl/ArrayMultiType.h"
#include "fmtcl/Plane.h"
#include "fmtcl/PlaneRO.h"
#include "fmtcl/SplFmt.h"

#include <cstdint>



namespace fmtcl
{



class TransOpInterface;

class TransLut
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	TransLut	ThisType;

	static constexpr int LINLUT_RES_L2  = 14; // log2 of the linear table resolution (size of a unity segment)
	static constexpr int LINLUT_MIN_F   = -1; // Min value for float LUTs
	static constexpr int LINLUT_MAX_F   = 2;  // Max value for float LUTs

	static constexpr int LINLUT_SIZE_F  = ((LINLUT_MAX_F - LINLUT_MIN_F) << LINLUT_RES_L2) + 1;

	// Log LUT are only for floating point input
	static constexpr int LOGLUT_MIN_L2  = -16; // log2(x) for the first index in a log table (negative)
	static constexpr int LOGLUT_MAX_L2  = 16;  // log2(x) for the last index in a log table (positive)
	static constexpr int LOGLUT_RES_L2  = 10;  // log2 of the log table resolution (size of each [x ; 2*x[ segment).

	static constexpr int LOGLUT_HSIZE   = ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2) + 1; // Table made of half-open segments (and whitout x=0) + 1 more value for LOGLUT_MAX, closing the last segment.
	static constexpr int LOGLUT_SIZE    = 2 * LOGLUT_HSIZE + 1;   // Negative + 0 + positive

	union FloatIntMix
	{
		float          _f;
		uint32_t       _i;
	};

	class MapperLin
	{
	public:
		explicit       MapperLin (int lut_size, double range_beg, double range_lst) noexcept;
		inline int     get_lut_size () const noexcept { return (_lut_size); }
		inline double  find_val (int index) const noexcept;
		static inline void
		               find_index (const FloatIntMix &val, int &index, float &frac) noexcept;
	private:
		const int      _lut_size  = 0;
		const double   _range_beg = 0;
		const double   _step      = 0;
	};

	class MapperLog
	{
	public:
		inline int     get_lut_size () const noexcept { return (LOGLUT_SIZE); }
		inline double  find_val (int index) const noexcept;
		static inline void
		               find_index (const FloatIntMix &val, int &index, float &frac) noexcept;
	};

	explicit       TransLut (const TransOpInterface &curve, bool log_flag, SplFmt src_fmt, int src_bits, bool src_full_flag, SplFmt dst_fmt, int dst_bits, bool dst_full_flag, bool sse2_flag, bool avx2_flag);
	virtual			~TransLut () {}

	void           process_plane (const Plane <> &dst, const PlaneRO <> &src, int w, int h) const noexcept;

	static bool    is_loglut_req (const TransOpInterface &curve);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	template <class T>
	class Convert
	{
	public:
		static inline T
		               cast (float val) noexcept;
	};

	void           generate_lut (const TransOpInterface &curve);
	template <class T>
	void           generate_lut_int (const TransOpInterface &curve, int lut_size, double range_beg, double range_lst, double mul, double add);
	template <class T, class M>
	void           generate_lut_flt (const TransOpInterface &curve, const M &mapper);

	void           init_proc_fnc ();
#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           init_proc_fnc_sse2 (int selector);
	void           init_proc_fnc_avx2 (int selector);
#endif

	template <class TS, class TD>
	void           process_plane_int_any_cpp (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept;
	template <class TD, class M>
	void           process_plane_flt_any_cpp (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept;
#if (fstb_ARCHI == fstb_ARCHI_X86)
	template <class TD, class M>
	void           process_plane_flt_any_sse2 (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept;
	template <class TD, class M>
	void           process_plane_flt_any_avx2 (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept;
#endif

	bool           _loglut_flag   = false;

	// SplFmt_STACK16 not supported at the moment.
	SplFmt         _src_fmt       = SplFmt_ILLEGAL;
	int            _src_bits      = 0;
	bool           _src_full_flag = false;

	// SplFmt_STACK16 not supported at the moment.
	SplFmt         _dst_fmt       = SplFmt_ILLEGAL;
	int            _dst_bits      = 0;
	bool           _dst_full_flag = false;

	bool           _sse2_flag     = false;
	bool           _avx2_flag     = false;

	void (ThisType:: *
	               _process_plane_ptr) (Plane <> dst, PlaneRO <> src, int w, int h) const noexcept = nullptr;

	// Opaque array, contains uint8_t, uint16_t or float depending on the
	// output datatype. Table size is always 256, 65536 or 65536*3+1
	// (float input, covering -1 to +2 range inclusive).
	ArrayMultiType _lut;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransLut ()                               = delete;
	               TransLut (const TransLut &other)          = delete;
	TransLut &     operator = (const TransLut &other)        = delete;
	bool           operator == (const TransLut &other) const = delete;
	bool           operator != (const TransLut &other) const = delete;

};	// class TransLut



}	// namespace fmtcl



//#include "fmtcl/TransLut.hpp"



#endif	// fmtcl_TransLut_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
