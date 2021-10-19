/*****************************************************************************

        GammaY.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/FrameRO.h"
#include "fmtcl/GammaY.h"
#include "fstb/def.h"
#include "fstb/fnc.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>

#include <cassert>
#include <cmath>



namespace fmtcl
{



constexpr inline int	GammaY_encode (SplFmt src_fmt, SplFmt src_dst, bool flt_flag, int shft) noexcept
{
	return (src_fmt << 11) + (src_dst << 8) + (shft << 1) + (flt_flag ? 1 : 0);
}



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	GammaY::_nbr_planes;



GammaY::GammaY (SplFmt src_fmt, int src_res, SplFmt dst_fmt, int dst_res, double gamma, double alpha, bool sse2_flag, bool avx2_flag)
{
	assert (src_fmt == SplFmt_FLOAT || src_fmt == SplFmt_INT16 || src_fmt == SplFmt_INT8);
	assert (dst_fmt == SplFmt_FLOAT || dst_fmt == SplFmt_INT16);
	assert (src_fmt == SplFmt_FLOAT || src_res <= 16);
	assert (dst_fmt == SplFmt_FLOAT || dst_res == 16);
	assert (gamma > 0);

	/*
	Y = sum (E * c_e)
	Y_raw / scale_y = sum (E_raw / scale_i * c_e)
	Y_raw = sum (E_raw * (c_e * scale_y / scale_i))
	Fd = E * pow (Y, gamma - 1) * alpha
	Fd_raw / scale_o = (E_raw / scale_i) * pow (Y, gamma - 1) * alpha
	Fd_raw = E_raw * (alpha * scale_o / scale_i) * pow (Y, gamma - 1)
	*/

	constexpr auto scale_c  = double (1 << _coef_res);
	double         scale_i  = 1;
	double         scale_y  = 1;
	SplFmt         luma_fmt = SplFmt_FLOAT;
	int            luma_res = 32;
	double         scale    = 1;
	if (src_fmt != SplFmt_FLOAT)
	{
		luma_fmt = SplFmt_INT16;
		luma_res = _luma_res_int;
		scale_i = double (1 << src_res) - 1;
		scale_y = double (1 << luma_res) - 1;
		const double   scale_iy = scale_y * scale_c / scale_i;
		_r2y_i  = fstb::round_int (_r2y_f * scale_iy);
		_g2y_i  = fstb::round_int (_g2y_f * scale_iy);
		_b2y_i  = fstb::round_int (         scale_iy) - _r2y_i - _g2y_i;
		scale /= scale_i;
	}

	SplFmt         lut_out_fmt  = SplFmt_FLOAT;
	int            lut_out_res  = 32;
	int            shft         = 0;
	bool           flt_amp_flag = false;
	if (dst_fmt != SplFmt_FLOAT)
	{
		assert (dst_res == 16);
		const double   scale_o = double (1 << dst_res) - 1;
		scale  *= scale_o;
		if (gamma < 1)
		{
			flt_amp_flag = true;
		}
		else if (src_fmt != SplFmt_FLOAT)
		{
			if (alpha < 0.5 || alpha > 2.0)
			{
				// It would be possible to handle an alpha out of this range
				// using shft to compensate, but this would be a bit complicated
				// to setup.
				flt_amp_flag = true;
			}
			else
			{
				// In this case:
				// out_raw = (in_raw * lut_raw) >> shft
				// shft = _coef_res - (res_o - res_i)
				// lut_raw = lut * scale_l
				// -->
				// scale_o = (scale_i * lut * scale_l) >> shft
				// lut = (scale_o << shft) / (scale_i * scale_l)
				lut_out_fmt  = SplFmt_INT16;
				lut_out_res  = 16;
				const double   scale_l = double ((1 << lut_out_res) - 1);
				shft         = _coef_res + src_res - dst_res;
				scale        = (scale_o * double (1 << shft)) / (scale_i * scale_l);
			}
		}
	}

	const Op       op (gamma, alpha * scale);
	_pow_uptr = std::make_unique <fmtcl::TransLut> (
		op, (luma_fmt == SplFmt_FLOAT),
		luma_fmt   , luma_res   , true,
		lut_out_fmt, lut_out_res, true,
		sse2_flag, avx2_flag
	);

#define fmtcl_GammaY_CASE( sf, st, df, dt, fa_flag, sh) \
	case GammaY_encode (SplFmt_##sf, SplFmt_##df, fa_flag, sh): \
		_process_plane_ptr = &GammaY::process_plane_cpp <st, dt, fa_flag, sh>; \
		break;

	switch (GammaY_encode (src_fmt, dst_fmt, flt_amp_flag, shft))
	{
	fmtcl_GammaY_CASE (FLOAT, float   , FLOAT, float   , false, 0)
	fmtcl_GammaY_CASE (FLOAT, float   , INT16, uint16_t, false, 0)
	fmtcl_GammaY_CASE (INT16, uint16_t, FLOAT, float   , false, 0)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, false, _coef_res+16-16)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, false, _coef_res+14-16)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, false, _coef_res+12-16)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, false, _coef_res+11-16)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, false, _coef_res+10-16)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, false, _coef_res+ 9-16)
	fmtcl_GammaY_CASE (INT8 , uint8_t , FLOAT, float   , false, 0)
	fmtcl_GammaY_CASE (INT8 , uint8_t , INT16, uint16_t, false, _coef_res+ 8-16)
	fmtcl_GammaY_CASE (FLOAT, float   , INT16, uint16_t, true , 0)
	fmtcl_GammaY_CASE (INT16, uint16_t, FLOAT, float   , true , 0)
	fmtcl_GammaY_CASE (INT16, uint16_t, INT16, uint16_t, true , 0)
	fmtcl_GammaY_CASE (INT8 , uint8_t , FLOAT, float   , true , 0)
	fmtcl_GammaY_CASE (INT8 , uint8_t , INT16, uint16_t, true , 0)
	default:
		assert (false);
		throw std::runtime_error ("fmtcl::GammaY::ctor: format not supported.");
	}

#undef fmtcl_GammaY_CASE
}



void	GammaY::process_plane (const Frame <> &dst_arr, const FrameRO <> &src_arr, int w, int h) const noexcept
{
	assert (dst_arr.is_valid (_nbr_planes, h));
	assert (src_arr.is_valid (_nbr_planes, h));
	assert (w > 0);
	assert (h > 0);

	assert (_process_plane_ptr != nullptr);
	(this->*_process_plane_ptr) (dst_arr, src_arr, w, h);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	GammaY::_buf_size;
constexpr int	GammaY::_coef_res;
constexpr int	GammaY::_luma_res_int;
constexpr float	GammaY::_r2y_f;
constexpr float	GammaY::_g2y_f;
constexpr float	GammaY::_b2y_f;



GammaY::Op::Op (double gamma, double alpha)
:	_gamma (gamma)
,	_alpha (alpha)
{
	assert (gamma > 0);
}



double	GammaY::Op::do_convert (double x) const
{
	return (x == 0) ? 0 : _alpha * pow (fabs (x), _gamma - 1);
}



TransOpInterface::LinInfo	GammaY::Op::do_get_info () const
{
	return { Type::UNDEF, Range::UNDEF, _alpha, 0 };
}



template <typename TD, typename TA, int SHFT>
TD	GammaY::Conv <TD, TA, SHFT>::conv (TA x) noexcept
{
	static_assert (
		   std::is_floating_point <TD>::value
		&& std::is_floating_point <TA>::value,
		"Specialisation failed!"
	);
	return TD (x);
}

template <int SHFT>
inline uint16_t GammaY::Conv <uint16_t, int, SHFT>::conv (int x) noexcept
{
	constexpr auto rounding_bias = 1 << (SHFT - 1);
	const auto     y = (x + rounding_bias) >> SHFT;
	return uint16_t (fstb::limit (y, 0, 0xFFFF));
}

template <int SHFT>
inline uint16_t GammaY::Conv <uint16_t, float, SHFT>::conv (float x) noexcept
{
	return uint16_t (fstb::limit (fstb::round_int (x), 0, 0xFFFF));
}



// FLT_FLAG indicates that the gain is float, whatever in/out datatype.
// We need it when gain > 1 (consequence of gamma < 1).
template <typename TS, typename TD, bool FLT_FLAG, int SHFT>
void	GammaY::process_plane_cpp (Frame <> dst_arr, FrameRO <> src_arr, int w, int h) const noexcept
{
	typedef typename std::conditional <
		std::is_floating_point <TS>::value, float, int
	>::type LumaTmpType;
	typedef typename std::conditional <
		std::is_floating_point <TS>::value, float, uint16_t
	>::type LumaType;
	typedef typename std::conditional <
		   std::is_floating_point <TS>::value
		|| std::is_floating_point <TD>::value
		|| FLT_FLAG,
		float, uint16_t
	>::type GainType;
	typedef typename std::conditional <
		   std::is_floating_point <TS>::value
		|| std::is_floating_point <TD>::value
		|| FLT_FLAG,
		float, int
	>::type AmpType;

	// Integer: 14-bit (_coef_res) scale
	alignas (64) std::array <GainType, _buf_size> gain;

	// Integer: 16-bit scale
	alignas (64) std::array <LumaType, _buf_size> luma;

	for (int y = 0; y < h; ++y)
	{
		FrameRO <TS>   s_arr { src_arr };
		Frame <TD>     d_arr { dst_arr };

		for (int x_blk = 0; x_blk < w; x_blk += _buf_size)
		{
			const auto     blk_len = std::min <int> (w - x_blk, _buf_size);

			// Computes Y (luminance) from the RGB data
			for (int x = 0; x < blk_len; ++x)
			{
				const auto     r = LumaTmpType (s_arr [0]._ptr [x]);
				const auto     g = LumaTmpType (s_arr [1]._ptr [x]);
				const auto     b = LumaTmpType (s_arr [2]._ptr [x]);
				const auto     l = compute_luma (r, g, b);
				luma [x] = l;
			}

			// Computes the component gain: alpha * pow (Y, gamma - 1)
			_pow_uptr->process_plane (
				Plane <> {   reinterpret_cast <      uint8_t *> (gain.data ()), 0 },
				PlaneRO <> { reinterpret_cast <const uint8_t *> (luma.data ()), 0 },
				blk_len, 1
			);

			// Amplifies each component
			for (int x = 0; x < blk_len; ++x)
			{
				const auto     m = AmpType (gain [x]);
				auto           r = AmpType (s_arr [0]._ptr [x]);
				auto           g = AmpType (s_arr [1]._ptr [x]);
				auto           b = AmpType (s_arr [2]._ptr [x]);
				r *= m;
				g *= m;
				b *= m;
				d_arr [0]._ptr [x] = Conv <TD, AmpType, SHFT>::conv (r);
				d_arr [1]._ptr [x] = Conv <TD, AmpType, SHFT>::conv (g);
				d_arr [2]._ptr [x] = Conv <TD, AmpType, SHFT>::conv (b);		
			}

			s_arr.step_pix (blk_len);
			d_arr.step_pix (blk_len);
		}

		src_arr.step_line ();
		dst_arr.step_line ();
	}
}



uint16_t	GammaY::compute_luma (int r, int g, int b) const noexcept
{
	const auto     l = r * _r2y_i + g * _g2y_i + b * _b2y_i;
	constexpr auto rounding_bias = 1 << (_coef_res - 1);
	const auto     y = (l + rounding_bias) >> _coef_res;

	return uint16_t (fstb::limit (y, 0, 0xFFFF));
}

float	GammaY::compute_luma (float r, float g, float b) const noexcept
{
	return r * _r2y_f + g * _g2y_f + b * _b2y_f;
}



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
