/*****************************************************************************

        GammaY.h
        Author: Laurent de Soras, 2021

This class applies a gamma correction on the luminance of a RGB signal instead
of each component separately. This avoids distorting the color saturation and
hue, as indicated in ITU-R BT.2390-9, p. 26.

Fd = alpha * Ys ^ (gamma - 1) * Es
for the 3 components Rs, Gs and Bs with:
Ys = 0.2627 * Rs + 0.6780 * Gs + 0.0593 * Bs

The formula is described as part of the HLG reference OOTF in ITU-R BT.2100-2,
p. 6.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_GammaY_HEADER_INCLUDED)
#define fmtcl_GammaY_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Frame.h"
#include "fmtcl/FrameRO.h"
#include "fmtcl/SplFmt.h"
#include "fmtcl/TransLut.h"
#include "fmtcl/TransOpInterface.h"

#include <memory>

#include <cstdint>



namespace fmtcl
{



class GammaY
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _nbr_planes = 3;

	typedef GammaY ThisType;

	explicit       GammaY (SplFmt src_fmt, int src_res, SplFmt dst_fmt, int dst_res, double gamma, double alpha, bool sse2_flag, bool avx2_flag);
	               ~GammaY () = default;

	void           process_plane (const Frame <> &dst_arr, const FrameRO <> &src_arr, int w, int h) const noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _buf_size = 1024;     // Pixels, multiple of 32
	static constexpr int _coef_res = 14;       // Resolution (bits) of fractional part
	static constexpr int _luma_res_int = 16;   // Luma resolution (bits) for integers

	// ITU-R BT.2100-2, p. 6
	static constexpr float  _r2y_f = 0.2627f;
	static constexpr float  _g2y_f = 0.6780f;
	static constexpr float  _b2y_f = float (1.0 - _r2y_f - _g2y_f); // 0.0593

	class Op
	:	public TransOpInterface
	{
	public:
		explicit       Op (double gamma, double alpha);
		double         operator () (double x) const override;
		double         get_max () const override;
	private:
		double         _gamma = 1;
		double         _alpha = 1;
	};

	template <typename TD, typename TA, int SHFT>
	class Conv
	{
	public:
		static inline TD conv (TA x) noexcept;
	};
	template <int SHFT> class Conv <uint16_t, int  , SHFT> { public:
		static inline uint16_t conv (int x) noexcept;
	};
	template <int SHFT> class Conv <uint16_t, float, SHFT> { public:
		static inline uint16_t conv (float x) noexcept;
	};

	template <typename TS, typename TD, bool FLT_FLAG, int SHFT>
	void           process_plane_cpp (Frame <> dst_arr, FrameRO <> src_arr, int w, int h) const noexcept;

	inline uint16_t
	               compute_luma (int r, int g, int b) const noexcept;
	inline float   compute_luma (float r, float g, float b) const noexcept;

	// The LUT implements f (x) = alpha * abs (x) ^ (gamma - 1)
	// x and f(x) may be scaled depending on the I/O format requirements.
	// The abs() makes sure that undershoots are treated correctly (no NaNs nor
	// INFs) and logically (consistent sign after the complete gamma operation).
	std::unique_ptr <TransLut>
	               _pow_uptr;

	// For integer calculations.
	int            _r2y_i = 0;
	int            _g2y_i = 0;
	int            _b2y_i = 0;

	void (ThisType:: *
	               _process_plane_ptr) (Frame <> dst_arr, FrameRO <> src_arr, int w, int h) const = nullptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               GammaY ()                               = delete;
	               GammaY (const GammaY &other)            = delete;
	               GammaY (GammaY &&other)                 = delete;
	GammaY &       operator = (const GammaY &other)        = delete;
	GammaY &       operator = (GammaY &&other)             = delete;
	bool           operator == (const GammaY &other) const = delete;
	bool           operator != (const GammaY &other) const = delete;

}; // class GammaY



}  // namespace fmtcl



//#include "fmtcl/GammaY.hpp"



#endif   // fmtcl_GammaY_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
