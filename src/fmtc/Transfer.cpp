/*****************************************************************************

        Transfer.cpp
        Author: Laurent de Soras, 2015

To do:
	- Remove code for destination bitdepth < 16

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtc/Transfer.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include "fstb/ToolsSse2.h"
#endif

#include <algorithm>

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Transfer::Transfer (const ::VSMap &in, ::VSMap &out, void * /*user_data_ptr*/, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "transfer", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_sse2_flag (false)
,	_avx2_flag (false)
,	_transs (get_arg_str (in, out, "transs", ""))
,	_transd (get_arg_str (in, out, "transd", ""))
,	_contrast (get_arg_flt (in, out, "cont", 1))
,	_gcor (get_arg_flt (in, out, "gcor", 1))
,	_full_range_src_flag (get_arg_int (in, out, "fulls", 1) != 0)
,	_full_range_dst_flag (get_arg_int (in, out, "fulld", 1) != 0)
,	_curve_s (fmtcl::TransCurve_UNDEF)
,	_curve_d (fmtcl::TransCurve_UNDEF)
,	_plane_processor (vsapi, *this, "transfer", true)
,	_lut ()
,	_process_plane_ptr (0)
,	_loglut_flag (false)
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&vsapi != 0);

	fstb::conv_to_lower_case (_transs);
	fstb::conv_to_lower_case (_transd);

	vsutl::CpuOpt  cpu_opt (*this, in, out);
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const ::VSFormat &   fmt_src = *_vi_in.format;

	if (   fmt_src.colorFamily != ::cmGray
	    && fmt_src.colorFamily != ::cmRGB)
	{
		throw_inval_arg ("unsupported color family.");
	}
	if (   (   fmt_src.sampleType == ::stInteger
	        && (   fmt_src.bitsPerSample <  8
	            || fmt_src.bitsPerSample > 16))
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_src.bitsPerSample != 32))
	{
		throw_inval_arg ("pixel bitdepth not supported.");
	}

	// Destination colorspace
	const ::VSFormat& fmt_dst =
		get_output_colorspace (in, out, core, fmt_src);

	if (   (   fmt_dst.sampleType == ::stInteger
	        && fmt_dst.bitsPerSample != 16)
	    || (   fmt_dst.sampleType == ::stFloat
	        && fmt_dst.bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}

	// Output format is validated.
	_vi_out.format = &fmt_dst;

	init_table ();
	init_proc_fnc ();
}



void	Transfer::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&node != 0);
	assert (&core != 0);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
	_plane_processor.set_filter (in, out, _vi_out);
}



const ::VSFrameRef *	Transfer::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);
	assert (&frame_data_ptr != 0);
	assert (&frame_ctx != 0);
	assert (&core != 0);

	::VSFrameRef *    dst_ptr = 0;
	::VSNodeRef &     node = *_clip_src_sptr;

	if (activation_reason == ::arInitial)
	{
		_vsapi.requestFrameFilter (n, &node, &frame_ctx);
	}

	else if (activation_reason == ::arAllFramesReady)
	{
		vsutl::FrameRefSPtr	src_sptr (
			_vsapi.getFrameFilter (n, &node, &frame_ctx),
			_vsapi
		);
		const ::VSFrameRef & src = *src_sptr;

		const int         w  =  _vsapi.getFrameWidth (&src, 0);
		const int         h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		const int      ret_val = _plane_processor.process_frame (
			*dst_ptr, n, frame_data_ptr, frame_ctx, core, _clip_src_sptr
		);

		if (ret_val == 0)
		{
			// Output frame properties
			::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

			const int      cr_val = (_full_range_dst_flag) ? 0 : 1;
			_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);

			int            transfer = fmtcl::TransCurve_UNSPECIFIED;
			if (_curve_d >= 0 && _curve_d <= fmtcl::TransCurve_ISO_RANGE_LAST)
			{
				transfer = _curve_d;
			}
			_vsapi.propSetInt (&dst_prop, "_Transfer", transfer, ::paReplace);
		}

		if (ret_val != 0)
		{
			_vsapi.freeFrame (dst_ptr);
			dst_ptr = 0;
		}
	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	Transfer::do_process_plane (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr)
{
	assert (src_node1_sptr.get () != 0);

	int            ret_val = 0;

	const vsutl::PlaneProcMode proc_mode =
		_plane_processor.get_mode (plane_index);

	if (proc_mode == vsutl::PlaneProcMode_PROCESS)
	{
		vsutl::FrameRefSPtr	src_sptr (
			_vsapi.getFrameFilter (n, src_node1_sptr.get (), &frame_ctx),
			_vsapi
		);
		const ::VSFrameRef & src = *src_sptr;

		const int      w = _vsapi.getFrameWidth (&src, plane_index);
		const int      h = _vsapi.getFrameHeight (&src, plane_index);

		const uint8_t* data_src_ptr = _vsapi.getReadPtr (&src, plane_index);
		const int      stride_src   = _vsapi.getStride (&src, plane_index);
		uint8_t *      data_dst_ptr = _vsapi.getWritePtr (&dst, plane_index);
		const int      stride_dst   = _vsapi.getStride (&dst, plane_index);

		assert (_process_plane_ptr != 0);
		(this->*_process_plane_ptr) (
			data_dst_ptr, data_src_ptr, stride_dst, stride_src, w, h
		);
	}

	return (ret_val);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Transfer::MultiTypeArray::MultiTypeArray ()
:	_arr ()
,	_length (0)
,	_data_len (0)
{
	// Nothing
}



void	Transfer::MultiTypeArray::resize (size_t length)
{
	const size_t   old_len = _length;
	_length = length;
	if (_length != old_len)
	{
		_arr.resize (_length * _data_len);
	}
}



Transfer::TransOpCompose::TransOpCompose (OpSPtr op_1_sptr, OpSPtr op_2_sptr)
:	_op_1_sptr (op_1_sptr)
,	_op_2_sptr (op_2_sptr)
{
	assert (op_1_sptr.get () != 0);
	assert (op_2_sptr.get () != 0);
}



double	Transfer::TransOpCompose::operator () (double x) const
{
	x = (*_op_1_sptr) (x);
	x = (*_op_2_sptr) (x);

	return (x);
}



Transfer::TransOpContrast::TransOpContrast (double cont)
:	_cont (cont)
{
	// Nothing
}



double	Transfer::TransOpContrast::operator () (double x) const
{
	return (x * _cont);
}



Transfer::TransOpLinPow::TransOpLinPow (bool inv_flag, double alpha, double beta, double p1, double slope, double lb, double ub, double scneg, double p2)
:	_inv_flag (inv_flag)
,	_alpha (alpha)
,	_beta (beta)
,	_p1 (p1)
,	_slope (slope)
,	_lb (lb)
,	_ub (ub)
,	_scneg (scneg)
,	_p2 (p2)
{
	_alpha_m1 = _alpha - 1;
	_beta_n   =       -_beta / _scneg;
	_beta_i   =  pow ( _beta   * _slope, _p2);
	_beta_in  = -pow (-_beta_n * _slope, _p2);
	_ub_i     = _alpha * pow (_ub, _p1) - (_alpha - 1);
	if (_lb < _beta_n)
	{
		_lb_i = -(_alpha * pow (-_lb * _scneg, _p1) - _alpha_m1) / _scneg;
	}
	else
	{
		_lb_i = -pow (-_lb * _slope, _p2);
	}
	_p1_i = 1 / p1;
	_p2_i = 1 / p2;
}



double	Transfer::TransOpLinPow::operator () (double x) const
{
	double         y = x;

	if (_inv_flag)
	{
		x = fstb::limit (x, _lb_i, _ub_i);
		if (x >= _beta_i)
		{
			y =  pow (( x          + _alpha_m1) / _alpha, _p1_i);
		}
		else if (x <= _beta_in)
		{
			y = -pow ((-x * _scneg + _alpha_m1) / _alpha, _p1_i) / _scneg;
		}
		else
		{
			if (_p2 == 1)
			{
				y =        x         / _slope;
			}
			else if (x < 0)
			{
				y = -pow (-x, _p2_i) / _slope;
			}
			else
			{
				y =  pow ( x, _p2_i) / _slope;
			}
		}
	}

	else
	{
		x = fstb::limit (x, _lb, _ub);
		if (x >= _beta)
		{
			y =   _alpha * pow ( x         , _p1) - _alpha_m1;
		}
		else if (x <= _beta_n)
		{
			y = -(_alpha * pow (-x * _scneg, _p1) - _alpha_m1) / _scneg;
		}
		else
		{
			if (_p2 == 1)
			{
				y =        x * _slope;
			}
			else if (x < 0)
			{
				y = -pow (-x * _slope, _p2);
			}
			else
			{
				y =  pow ( x * _slope, _p2);
			}
		}
	}

	return (y);
}



Transfer::TransOpLogTrunc::TransOpLogTrunc (bool inv_flag, double alpha, double beta)
:	_inv_flag (inv_flag)
,	_alpha (alpha)
,	_beta (beta)
{
	// Nothing
}



double	Transfer::TransOpLogTrunc::operator () (double x) const
{
	x = fstb::limit (x, 0.0, 1.0);
	double         y = x;

	if (_inv_flag)
	{
		y = pow (10, (x - 1) / _alpha);
	}
	else
	{
		if (x >= _beta)
		{
			y = 1 + _alpha * log10 (x);
		}
		else
		{
			y = 0;
		}
	}

	return (y);
}



Transfer::TransOpPow::TransOpPow (bool inv_flag, double p_i, double alpha, double val_max)
:	_inv_flag (inv_flag)
,	_p_i (p_i)
,	_alpha (alpha)
,	_p (1 / p_i)
,	_val_max (val_max)
{
	// Nothing
}



double	Transfer::TransOpPow::operator () (double x) const
{
	x = std::max (x, 0.0);
	double         y = x;

	if (_inv_flag)
	{
		y = pow (x / _alpha, _p_i);
		y = std::min (y, _val_max);
	}
	else
	{
		x = std::min (x, _val_max);
		y = _alpha * pow (x, _p);
	}

	return (y);
}



Transfer::TransOp2084::TransOp2084 (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



// Linear values are absolute. 1 is 10000 cd/m2.
double	Transfer::TransOp2084::operator () (double x) const
{
	x = fstb::limit (x, 0.0, 1.0);
	double         y = x;

	static const double  c1 =   1.0  * 3424 / 4096;
	static const double  c2 =  32.0  * 2413 / 4096;
	static const double  c3 =  32.0  * 2392 / 4096;
	static const double  m  = 128.0  * 2523 / 4096;
	static const double  n  =   0.25 * 2610 / 4096;

	if (_inv_flag)
	{
		// Inverse formula from:
		// Scott Miller, Mahdi Nezamabadi, Scott Daly
		// Perceptual Signal Coding for More Efficient Usage of Bit Codes, p. 5
		// Presentation for 2012 SMPTE Annual Technical Conference & Exhibition
		const double   xp = pow (x, 1 / m);
		const double   r  = (xp - c1) / (c2 - c3 * xp);
		if (r < 0)
		{
			y = 0;
		}
		else
		{
			y = pow (r, 1 / n);
		}
	}
	else
	{
		const double   xp = pow (x, n);
		y = pow ((c1 + c2 * xp) / (1 + c3 * xp), m);
	}

	return (y);
}



Transfer::TransOpFilmStream::TransOpFilmStream (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



// Linear 1 is the sensor clipping level (3840 on a linear 12-bit scale).
double	Transfer::TransOpFilmStream::operator () (double x) const
{
	static const double  sc10    = 1024;
	static const double  bl12    = 64;
	static const double  wl12    = 3840;
	static const double  mi10    =    3 / sc10;
	static const double  ma10    = 1020 / sc10;
	static const double  sp      = 500;
	static const double  sl      = 0.02714;

	double         y = x;
	if (_inv_flag)
	{
		x = fstb::limit (x, mi10, ma10);
		const double   sensor = pow (10, x * (sc10 / sp)) / sl;
		y = (sensor - bl12) / (wl12 - bl12);
	}
	else
	{
		const double   sensor = bl12 + x * (wl12 - bl12);
		if (sensor <= 37)
		{
			y = 0;
		}
		else
		{
			y = (sp / sc10) * log10 (sl * sensor);
			y = fstb::limit (y, mi10, ma10);
		}
	}

	return (y);
}



Transfer::TransOpSLog::TransOpSLog (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



// 1 lin is reference white, peak white at 10 lin.
double	Transfer::TransOpSLog::operator () (double x) const
{
	static const double  a = 0.037584;
	static const double  b = 0.432699;
	static const double  c = 0.616596 + 0.03;

	double         y = x;
	if (_inv_flag)
	{
		y = pow (10, (y - c) / b) - a;
	}
	else
	{
		y = b * log10 (std::max (x, 0.0) + a) + c;
	}

	return (y);
}



Transfer::TransOpLogC::TransOpLogC (bool inv_flag, bool v2_flag)
:	_inv_flag (inv_flag)
,	_v2_flag (v2_flag)
,	_cut (v2_flag ? 0.000000 : 0.010591)
,	_a (  v2_flag ? 5.061087 : 5.555556)
,	_b (  v2_flag ? 0.089004 : 0.052272)
,	_c (  v2_flag ? 0.247189 : 0.247190)
,	_d (  v2_flag ? 0.391007 : 0.385537)
,	_e (  v2_flag ? 4.950469 : 5.367655)
,	_f (  v2_flag ? 0.131313 : 0.092809)
{
	_cut_i = _e * _cut + _f;
}



// 1 is log peak white.
double	Transfer::TransOpLogC::operator () (double x) const
{
	static const double  noise_margin = -8.0 / 65536;
	double         y = x;
	if (_inv_flag)
	{
		x = std::min (x, 1.0);
		y = (x > _cut_i) ? (pow (10, (x - _d) / _c) - _b) / _a : (x - _f) / _e;
		y = std::max (y, noise_margin);
	}
	else
	{
		x = std::max (x, noise_margin);
		y = (x > _cut  ) ? _c * log10 (_a * x + _b) + _d : _e * x + _f;
		y = std::min (y, 1.0);
	}

	return (y);
}



Transfer::TransOpCanonLog::TransOpCanonLog (bool inv_flag)
:	_inv_flag (inv_flag)
{
	// Nothing
}



// 1.08676 is log peak white, at 8.00903 in linear scale.
double	Transfer::TransOpCanonLog::operator () (double x) const
{
	static const double  a = 10.1596;
	static const double  b = 0.529136;
	static const double  c = 0.0730597;

	double         y = x;
	if (_inv_flag)
	{
		y = (pow (10, (y - c) / b) - 1) / a;
	}
	else
	{
		y = b * log10 (std::max (x, -0.0452664) * a + 1) + c;
	}

	return (y);
}



Transfer::MapperLin::MapperLin (int lut_size, double range_beg, double range_lst)
:	_lut_size (lut_size)
,	_range_beg (range_beg)
,	_range_lst (range_lst)
,	_step ((range_lst - range_beg) / (lut_size - 1))
{
	assert (lut_size >= 2);
	assert (_range_beg < _range_lst);
}



void	Transfer::MapperLin::find_index (const FloatIntMix &val, int &index, float &frac)
{
	assert (&val != 0);
	assert (&index != 0);
	assert (&frac != 0);

	const float    val_scl   = val._f * (1 << LINLUT_RES_L2);
	const int      index_raw = fstb::floor_int (val_scl);
	const int      offset    = -LINLUT_MIN_F * (1 << LINLUT_RES_L2);
	index = fstb::limit (index_raw + offset, 0, LINLUT_SIZE_F - 2);
	frac  = val_scl - float (index_raw);
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

void	Transfer::MapperLin::find_index (const FloatIntMix val_arr [4], __m128i &index, __m128 &frac)
{
	assert (val_arr != 0);
	assert (&index != 0);
	assert (&frac != 0);
	
	const int      offset    = -LINLUT_MIN_F * (1 << LINLUT_RES_L2);
	const __m128   scale     = _mm_set1_ps (1 << LINLUT_RES_L2);
	const __m128i  offset_ps = _mm_set1_epi32 (offset);
	const __m128   val_min   = _mm_set1_ps (0                 - offset);
	const __m128   val_max   = _mm_set1_ps (LINLUT_SIZE_F - 2 - offset);

	const __m128   v         =
		_mm_load_ps (reinterpret_cast <const float *> (val_arr));
	__m128         val_scl   = _mm_mul_ps (v, scale);
	val_scl = _mm_min_ps (val_scl, val_max);
	val_scl = _mm_max_ps (val_scl, val_min);
	const __m128i  index_raw = _mm_cvtps_epi32 (val_scl);
	index     = _mm_add_epi32 (index_raw, offset_ps);
	frac      = _mm_sub_ps (val_scl, _mm_cvtepi32_ps (index_raw));
}

#endif



double	Transfer::MapperLin::find_val (int index) const
{
	return (_range_beg + index * _step);
}



void	Transfer::MapperLog::find_index (const FloatIntMix &val, int &index, float &frac)
{
	assert (&val != 0);
	assert (&index != 0);
	assert (&frac != 0);
	static_assert (LOGLUT_MIN_L2 <= 0, "LOGLUT_MIN_L2 must be negative");
	static_assert (LOGLUT_MAX_L2 >= 0, "LOGLUT_MAX_L2 must be positive");

	static const int      mant_size = 23;
	static const int      exp_bias  = 127;
	static const uint32_t base      = (exp_bias + LOGLUT_MIN_L2) << mant_size;
	static const float    val_min   = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
	static const float    val_max   = float (int64_t (1) << LOGLUT_MAX_L2);
	static const int      frac_size = mant_size - LOGLUT_RES_L2;
	static const uint32_t frac_mask = (1 << frac_size) - 1;

	const uint32_t val_u = val._i & 0x7FFFFFFF;
	const float    val_a = fabs (val._f);

	// index is set relatively to the x=0 index...
	if (val_a < val_min)
	{
		index = 0;
		frac  = std::max (val_a, 0.0f) * (1.0f / val_min);
	}
	else if (val_a >= val_max)
	{
		index = ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2);
		frac  = 1;
	}
	else
	{
		index = ((val_u - base) >> frac_size) + 1;
		frac  = (val_u & frac_mask) * (1.0f / (1 << frac_size));
	}

	// ...and shifted or mirrored depending on the sign
	if (val._f >= 0)
	{
		index += LOGLUT_HSIZE;
	}
	else
	{
		// Because frac cannot be negative, step one index behind.
		index = LOGLUT_HSIZE - 1 - index;
		frac  = 1 - frac;
	}

	assert (index >= 0);
	assert (index < LOGLUT_SIZE - 1);
	assert (frac >= 0);
	assert (frac <= 1);
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

void	Transfer::MapperLog::find_index (const FloatIntMix val_arr [4], __m128i &index, __m128 &frac)
{
	assert (val_arr != 0);
	assert (&index != 0);
	assert (&frac != 0);

	// Constants
	static const int      mant_size = 23;
	static const int      exp_bias  = 127;
	static const uint32_t base      = (exp_bias + LOGLUT_MIN_L2) << mant_size;
	static const float    val_min   = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
	static const float    val_max   = float (int64_t (1) << LOGLUT_MAX_L2);
	static const int      frac_size = mant_size - LOGLUT_RES_L2;
	static const uint32_t frac_mask = (1 << frac_size) - 1;

	const __m128   zero_f     = _mm_setzero_ps ();
	const __m128   one_f      = _mm_set1_ps (1);
	const __m128   frac_mul   = _mm_set1_ps (1.0f / (1 << frac_size));
	const __m128   mul_eps    = _mm_set1_ps (1.0f / val_min);
	const __m128   mask_abs_f = _mm_load_ps (
		reinterpret_cast <const float *> (fstb::ToolsSse2::_mask_abs)
	);

	const __m128i  zero_i          = _mm_setzero_si128 ();
	const __m128i  mask_abs_epi32  = _mm_set1_epi32 (0x7FFFFFFF);
	const __m128i  one_epi32       = _mm_set1_epi32 (1);
	const __m128i  base_epi32      = _mm_set1_epi32 (int (base));
	const __m128i  frac_mask_epi32 = _mm_set1_epi32 (frac_mask);
	const __m128i  val_min_epi32   =
		_mm_set1_epi32 ((LOGLUT_MIN_L2 + exp_bias) << mant_size);
	const __m128i  val_max_epi32   =
		_mm_set1_epi32 ((LOGLUT_MAX_L2 + exp_bias) << mant_size);
	const __m128i  index_max_epi32 =
		_mm_set1_epi32 ((LOGLUT_MAX_L2 - LOGLUT_MIN_L2) << LOGLUT_RES_L2);
	const __m128i  hsize_epi32     = _mm_set1_epi32 (LOGLUT_HSIZE);
	const __m128i  mirror_epi32    = _mm_set1_epi32 (LOGLUT_HSIZE - 1);

	// It really starts here
	const __m128   val_f = _mm_load_ps (reinterpret_cast <const float *> (val_arr));
	const __m128   val_a = _mm_and_ps (val_f, mask_abs_f);
	const __m128i  val_i = _mm_load_si128 (reinterpret_cast <const __m128i *> (val_arr));
	const __m128i  val_u = _mm_and_si128 (val_i, mask_abs_epi32);

	// Standard path
	__m128i        index_std = _mm_sub_epi32 (val_u, base_epi32);
	index_std = _mm_srli_epi32 (index_std, frac_size);
	index_std = _mm_add_epi32 (index_std, one_epi32);
	__m128i        frac_stdi = _mm_and_si128 (val_u, frac_mask_epi32);
	__m128         frac_std  = _mm_cvtepi32_ps (frac_stdi);
	frac_std  = _mm_mul_ps (frac_std, frac_mul);

	// Epsilon path
	__m128         frac_eps  = _mm_max_ps (val_a, zero_f);
	frac_eps = _mm_mul_ps (frac_eps, mul_eps);

	// Range cases
	const __m128i  eps_flag_i = _mm_cmpgt_epi32 (val_min_epi32, val_u);
	const __m128i  std_flag_i = _mm_cmpgt_epi32 (val_max_epi32, val_u);
	const __m128   eps_flag_f = _mm_castsi128_ps (eps_flag_i);
	const __m128   std_flag_f = _mm_castsi128_ps (std_flag_i);
	__m128i        index_tmp  =
		fstb::ToolsSse2::select (std_flag_i, index_std, index_max_epi32);
	__m128         frac_tmp   =
		fstb::ToolsSse2::select (std_flag_f, frac_std, one_f);
	index_tmp = fstb::ToolsSse2::select (eps_flag_i, zero_i, index_tmp);
	frac_tmp  = fstb::ToolsSse2::select (eps_flag_f, frac_eps, frac_tmp);

	// Sign cases
	const __m128i  neg_flag_i = _mm_srai_epi32 (val_i, 31);
	const __m128   neg_flag_f = _mm_castsi128_ps (neg_flag_i);
	const __m128i  index_neg  = _mm_sub_epi32 (mirror_epi32, index_tmp);
	const __m128i  index_pos  = _mm_add_epi32 (hsize_epi32, index_tmp);
	const __m128   frac_neg   = _mm_sub_ps (one_f, frac_tmp);
	index = fstb::ToolsSse2::select (neg_flag_i, index_neg, index_pos);
	frac  = fstb::ToolsSse2::select (neg_flag_f, frac_neg, frac_tmp);
}

#endif



double	Transfer::MapperLog::find_val (int index) const
{
	assert (index >= 0);
	assert (index < LOGLUT_SIZE);

	static const float    val_min  = 1.0f / (int64_t (1) << -LOGLUT_MIN_L2);
	static const int      seg_size = 1 << LOGLUT_RES_L2;

	// float is OK because the values are exactly represented in float.
	float          val   = 0;
	int            ind_2 = index - LOGLUT_HSIZE;
	if (ind_2 != 0)
	{
		const int      ind_3     = std::abs (ind_2) - 1;
		const int      log2_part = ind_3 >> LOGLUT_RES_L2;
		const int      seg_part  = ind_3 & (seg_size - 1);
		const float    lerp      = seg_part * (1.0f / seg_size);
		const float    v0        = (int64_t (1) << log2_part) * val_min;
		val = v0 * (1 + lerp);
		if (ind_2 < 0)
		{
			val = -val;
		}
	}

	return (val);
}



const ::VSFormat &	Transfer::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&fmt_src != 0);

	const ::VSFormat *   fmt_dst_ptr = &fmt_src;

	const int      undef    = -666666666;
	const int      dst_flt  = get_arg_int (in, out, "flt" , undef);
	const int      dst_bits = get_arg_int (in, out, "bits", undef);

	int            col_fam  = fmt_dst_ptr->colorFamily;
	int            spl_type = fmt_dst_ptr->sampleType;
	int            bits     = fmt_dst_ptr->bitsPerSample;
	int            ssh      = fmt_dst_ptr->subSamplingW;
	int            ssv      = fmt_dst_ptr->subSamplingH;

	// Data type
	if (dst_flt == 0)
	{
		spl_type = ::stInteger;
	}
	else if (dst_flt != undef)
	{
		spl_type = ::stFloat;
		if (dst_bits == undef)
		{
			bits = 32;
		}
	}

	// Bitdepth
	if (dst_bits != undef)
	{
		bits = dst_bits;
		if (dst_flt == undef)
		{
			if (bits < 32)
			{
				spl_type = ::stInteger;
			}
			else
			{
				spl_type = ::stFloat;
			}
		}
	}

	// Combines the modified parameters and validates the format
	try
	{
		fmt_dst_ptr = register_format (
			col_fam,
			spl_type,
			bits,
			ssh,
			ssv,
			core
		);
	}
	catch (...)
	{
		fmt_dst_ptr = 0;
	}
	if (fmt_dst_ptr == 0)
	{
		throw_rt_err (
			"couldn\'t get a pixel format identifier for the output clip."
		);
	}

	return (*fmt_dst_ptr);
}



void	Transfer::init_table ()
{
	_curve_s = conv_string_to_curve (*this, _transs);
	_curve_d = conv_string_to_curve (*this, _transd);
	OpSPtr         op_s = conv_curve_to_op (_curve_s, true );
	OpSPtr         op_d = conv_curve_to_op (_curve_d, false);

	_loglut_flag = false;
	if (   _vi_in.format->sampleType == ::stFloat
	    && _curve_s == fmtcl::TransCurve_LINEAR)
	{
		// Curves with extended range or with fast-evolving slope at 0.
		// Actually we could just use the log LUT for all the curves...?
		// 10 bits per stop + interpolation should be enough for all of them.
		// What about the speed?
		if (   _curve_d == fmtcl::TransCurve_470BG
		    || _curve_d == fmtcl::TransCurve_LINEAR
		    || _curve_d == fmtcl::TransCurve_61966_2_4
		    || _curve_d == fmtcl::TransCurve_2084
		    || _curve_d == fmtcl::TransCurve_428
		    || _curve_d == fmtcl::TransCurve_1886
		    || _curve_d == fmtcl::TransCurve_1886A
		    || _curve_d == fmtcl::TransCurve_SLOG
		    || _curve_d == fmtcl::TransCurve_LOGC2
		    || _curve_d == fmtcl::TransCurve_LOGC3
		    || _curve_d == fmtcl::TransCurve_CANONLOG)
		{
			_loglut_flag = true;
		}
		if (_gcor < 0.5)
		{
			_loglut_flag = true;
		}
		if (fabs (_contrast) >= 3.0/2 || fabs (_contrast) <= 2.0/3)
		{
			_loglut_flag = true;
		}
	}

	if (_gcor != 1)
	{
		OpSPtr         op_g (new TransOpPow (true, _gcor, 1, 1e6));
		op_d = OpSPtr (new TransOpCompose (op_g, op_d));
	}
	if (_contrast != 1)
	{
		OpSPtr         op_c (new TransOpContrast (_contrast));
		op_d = OpSPtr (new TransOpCompose (op_c, op_d));
	}

	OpSPtr         op_f (new TransOpCompose (op_s, op_d));
	generate_lut (*op_f);
}



void	Transfer::init_proc_fnc ()
{
	const int      s =
		  (_loglut_flag                           ) ? 0
		: (_vi_in.format->sampleType  == ::stFloat) ? 1
		: (_vi_in.format->bitsPerSample  > 8      ) ? 2
		:                                             3;
	const int      d =
		  (_vi_out.format->sampleType == ::stFloat) ? 0
		: (_vi_out.format->bitsPerSample > 8      ) ? 1
		:                                             2;
	const int      p =
		  (_vi_in.format->sampleType  == ::stInteger) ? 0
		: (_sse2_flag                               ) ? 1
		:                                               0;

	const int      selector = d * 4 + s;

	switch (selector + p * 3 * 4)
	{
	case (0*3+0)*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          float   , MapperLog>; break;
	case (0*3+0)*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          float   , MapperLin>; break;
	case (0*3+0)*4+2:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint16_t, float              >; break;
	case (0*3+0)*4+3:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint8_t , float              >; break;
	case (0*3+1)*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint16_t, MapperLog>; break;
	case (0*3+1)*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint16_t, MapperLin>; break;
	case (0*3+1)*4+2:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint16_t, uint16_t           >; break;
	case (0*3+1)*4+3:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint8_t , uint16_t           >; break;
	case (0*3+2)*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint8_t , MapperLog>; break;
	case (0*3+2)*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_cpp  <          uint8_t , MapperLin>; break;
	case (0*3+2)*4+2:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint16_t, uint8_t            >; break;
	case (0*3+2)*4+3:	_process_plane_ptr = &ThisType::process_plane_int_any_cpp  <uint8_t , uint8_t            >; break;

#if (fstb_ARCHI == fstb_ARCHI_X86)
	case (1*3+0)*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <          float   , MapperLog>; break;
	case (1*3+0)*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <          float   , MapperLin>; break;
	case (1*3+1)*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <          uint16_t, MapperLog>; break;
	case (1*3+1)*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <          uint16_t, MapperLin>; break;
	case (1*3+2)*4+0:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <          uint8_t , MapperLog>; break;
	case (1*3+2)*4+1:	_process_plane_ptr = &ThisType::process_plane_flt_any_sse2 <          uint8_t , MapperLin>; break;
#endif

	default:
		assert (false);
		break;
	}
#if (fstb_ARCHI == fstb_ARCHI_X86)
	init_proc_fnc_avx2 (selector);
#endif
}



void	Transfer::generate_lut (const TransOpInterface &curve)
{
	assert (&curve != 0);

	if (_vi_in.format->sampleType == ::stFloat)
	{
		// When the source is float, the LUT output is always float
		// so we can interpolate it easily and obtain the exact values.
		// If the target data type is int, we quantize the interpolated
		// values as a second step.
		_lut.set_type <float> ();

		if (_loglut_flag)
		{
			_lut.resize (LOGLUT_SIZE);
			MapperLog   mapper;
			generate_lut_flt <float> (curve, mapper);
		}
		else
		{
			_lut.resize (LINLUT_SIZE_F);
			MapperLin   mapper (LINLUT_SIZE_F, LINLUT_MIN_F, LINLUT_MAX_F);
			generate_lut_flt <float> (curve, mapper);
		}
	}

	else
	{
		int            range = 1 << _vi_in.format->bitsPerSample;
		if (_vi_in.format->bitsPerSample <= 8)
		{
			_lut.set_type <uint8_t> ();
			_lut.resize (1 << 8);
		}
		else
		{
			_lut.set_type <uint16_t> ();
			_lut.resize (1 << 16);
		}
		const int      sb16  = (_full_range_src_flag) ? 0      :  16 << 8;
		const int      sw16  = (_full_range_src_flag) ? 0xFFFF : 235 << 8;
		int            sbn   = sb16 >> (16 - _vi_in.format->bitsPerSample);
		int            swn   = sw16 >> (16 - _vi_in.format->bitsPerSample);
		const int      sdif  = swn - sbn;
		const double   r_beg = double (0         - sbn) / sdif;
		const double   r_lst = double (range - 1 - sbn) / sdif;
		if (_vi_out.format->sampleType == ::stFloat)
		{
			MapperLin   mapper (range, r_beg, r_lst);
			generate_lut_flt <float> (curve, mapper);
		}
		else
		{
			const int      db16 = (_full_range_dst_flag) ? 0      :  16 << 8;
			const int      dw16 = (_full_range_dst_flag) ? 0xFFFF : 235 << 8;
			int            dbn  = db16 >> (16 - _vi_out.format->bitsPerSample);
			int            dwn  = dw16 >> (16 - _vi_out.format->bitsPerSample);
			const double   mul  = dwn - dbn;
			const double   add  = dbn;
			if (_vi_out.format->bitsPerSample > 8)
			{
				generate_lut_int <uint16_t> (
					curve, range, r_beg, r_lst, mul, add
				);
			}
			else
			{
				generate_lut_int <uint8_t> (
					curve, range, r_beg, r_lst, mul, add
				);
			}
		}
	}
}



// T = LUT data type (int or float)
template <class T>
void	Transfer::generate_lut_int (const TransOpInterface &curve, int lut_size, double range_beg, double range_lst, double mul, double add)
{
	assert (_vi_out.format->sampleType == ::stInteger);
	assert (&curve != 0);
	assert (lut_size > 1);
	assert (range_beg < range_lst);

	const double   scale   = (range_lst - range_beg) / (lut_size - 1);
	const int      max_val = (1 << _vi_out.format->bitsPerSample) - 1;
	for (int pos = 0; pos < lut_size; ++pos)
	{
		const double   x = range_beg + pos * scale;
		const double   y = curve (x) * mul + add;
		_lut.use <T> (pos) = T (fstb::limit (fstb::round_int (y), 0, max_val));
	}
}



// T = float
template <class T, class M>
void	Transfer::generate_lut_flt (const TransOpInterface &curve, const M &mapper)
{
	assert (&curve != 0);
	assert (&mapper != 0);

	const int      lut_size = mapper.get_lut_size ();
	for (int pos = 0; pos < lut_size; ++pos)
	{
		const double   x = mapper.find_val (pos);
		const double   y = curve (x);
		_lut.use <T> (pos) = T (y);
	}
}



template <class TS, class TD>
void	Transfer::process_plane_int_any_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h)
{
	assert (dst_ptr != 0);
	assert (src_ptr != 0);
	assert (stride_dst != 0);
	assert (stride_src != 0);
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const TS *     s_ptr = reinterpret_cast <const TS *> (src_ptr);
		TD *           d_ptr = reinterpret_cast <      TD *> (dst_ptr);

		for (int x = 0; x < w; ++x)
		{
			const int          index = s_ptr [x];
			d_ptr [x] = _lut.use <TD> (index);
		}

		src_ptr += stride_src;
		dst_ptr += stride_dst;
	}
}



template <class TD, class M>
void	Transfer::process_plane_flt_any_cpp (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h)
{
	assert (dst_ptr != 0);
	assert (src_ptr != 0);
	assert (stride_dst != 0);
	assert (stride_src != 0);
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const FloatIntMix *  s_ptr =
			reinterpret_cast <const FloatIntMix *> (src_ptr);
		TD *                 d_ptr =
			reinterpret_cast <               TD *> (dst_ptr);

		for (int x = 0; x < w; ++x)
		{
			int                index;
			float              lerp;
			M::find_index (s_ptr [x], index, lerp);
			const float        p_0  = _lut.use <float> (index    );
			const float        p_1  = _lut.use <float> (index + 1);
			const float        dif  = p_1 - p_0;
			const float        val  = p_0 + lerp * dif;
			d_ptr [x] = Convert <TD>::cast (val);
		}

		src_ptr += stride_src;
		dst_ptr += stride_dst;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <class TD, class M>
void	Transfer::process_plane_flt_any_sse2 (uint8_t *dst_ptr, const uint8_t *src_ptr, int stride_dst, int stride_src, int w, int h)
{
	assert (dst_ptr != 0);
	assert (src_ptr != 0);
	assert (stride_dst != 0);
	assert (stride_src != 0);
	assert (w > 0);
	assert (h > 0);

	for (int y = 0; y < h; ++y)
	{
		const FloatIntMix *  s_ptr =
			reinterpret_cast <const FloatIntMix *> (src_ptr);
		TD *                 d_ptr =
			reinterpret_cast <               TD *> (dst_ptr);

		for (int x = 0; x < w; x += 4)
		{
			union
			{
				__m128i            _vect;
				uint32_t           _scal [4];
			}                  index;
			__m128             lerp;
			M::find_index (s_ptr + x, index._vect, lerp);
			__m128             val = _mm_set_ps (
				_lut.use <float> (index._scal [3]    ),
				_lut.use <float> (index._scal [2]    ),
				_lut.use <float> (index._scal [1]    ),
				_lut.use <float> (index._scal [0]    )
			);
			__m128             va2 = _mm_set_ps (
				_lut.use <float> (index._scal [3] + 1),
				_lut.use <float> (index._scal [2] + 1),
				_lut.use <float> (index._scal [1] + 1),
				_lut.use <float> (index._scal [0] + 1)
			);
			const __m128       dif = _mm_sub_ps (va2, val);
			val = _mm_add_ps (val, _mm_mul_ps (dif, lerp));
			Convert <TD>::store_sse2 (&d_ptr [x], val);
		}

		src_ptr += stride_src;
		dst_ptr += stride_dst;
	}
}



#endif



// str should be already converted to lower case
fmtcl::TransCurve	Transfer::conv_string_to_curve (const vsutl::FilterBase &flt, const std::string &str)
{
	assert (&flt != 0);
	assert (&str != 0);

	fmtcl::TransCurve c = fmtcl::TransCurve_UNDEF;
	if (str == "709")
	{
		c = fmtcl::TransCurve_709;
	}
	else if (str == "470m")
	{
		c = fmtcl::TransCurve_470M;
	}
	else if (str == "470bg")
	{
		c = fmtcl::TransCurve_470BG;
	}
	else if (str == "601")
	{
		c = fmtcl::TransCurve_601;
	}
	else if (str == "240")
	{
		c = fmtcl::TransCurve_240;
	}
	else if (str.empty () || str == "linear")
	{
		c = fmtcl::TransCurve_LINEAR;
	}
	else if (str == "log100")
	{
		c = fmtcl::TransCurve_LOG100;
	}
	else if (str == "log316")
	{
		c = fmtcl::TransCurve_LOG316;
	}
	else if (str == "61966-2-4")
	{
		c = fmtcl::TransCurve_61966_2_4;
	}
	else if (str == "1361")
	{
		c = fmtcl::TransCurve_1361;
	}
	else if (str == "61966-2-1" || str == "srgb" || str == "sycc")
	{
		c = fmtcl::TransCurve_SRGB;
	}
	else if (str == "2020_10")
	{
		c = fmtcl::TransCurve_2020_10;
	}
	else if (str == "2020_12" || str == "2020")
	{
		c = fmtcl::TransCurve_2020_12;
	}
	else if (str == "2084")
	{
		c = fmtcl::TransCurve_2084;
	}
	else if (str == "428-1" || str == "428")
	{
		c = fmtcl::TransCurve_428;
	}
	else if (str == "1886")
	{
		c = fmtcl::TransCurve_1886;
	}
	else if (str == "1886a")
	{
		c = fmtcl::TransCurve_1886A;
	}
	else if (str == "filmstream")
	{
		c = fmtcl::TransCurve_FILMSTREAM;
	}
	else if (str == "slog")
	{
		c = fmtcl::TransCurve_SLOG;
	}
	else if (str == "logc2")
	{
		c = fmtcl::TransCurve_LOGC2;
	}
	else if (str == "logc3")
	{
		c = fmtcl::TransCurve_LOGC3;
	}
	else if (str == "canonlog")
	{
		c = fmtcl::TransCurve_CANONLOG;
	}
	else
	{
		flt.throw_inval_arg ("unknown matrix identifier.");
	}


	return (c);
}



Transfer::OpSPtr	Transfer::conv_curve_to_op (fmtcl::TransCurve c, bool inv_flag)
{
	assert (c >= 0);

	OpSPtr         ptr;

	switch (c)
	{
	case fmtcl::TransCurve_709:
	case fmtcl::TransCurve_601:
	case fmtcl::TransCurve_2020_10:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5));
		break;
	case fmtcl::TransCurve_470BG:
		ptr = OpSPtr (new TransOpPow (inv_flag, 2.8));
		break;
	case fmtcl::TransCurve_240:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.1115, 0.0228, 0.45, 4.0));
		break;
	case fmtcl::TransCurve_LINEAR:
		ptr = OpSPtr (new TransOpBypass);
		break;
	case fmtcl::TransCurve_LOG100:
		ptr = OpSPtr (new TransOpLogTrunc (inv_flag, 0.5, 0.01));
		break;
	case fmtcl::TransCurve_LOG316:
		ptr = OpSPtr (new TransOpLogTrunc (inv_flag, 0.4, sqrt (10) / 1000));
		break;
	case fmtcl::TransCurve_61966_2_4:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5, -999, 999));
		break;
	case fmtcl::TransCurve_1361:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5, -0.25, 1.33, 4));
		break;
	case fmtcl::TransCurve_470M:	// Assumed display gamma 2.2, almost like sRGB.
	case fmtcl::TransCurve_SRGB:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.055, 0.04045 / 12.92, 1.0 / 2.4, 12.92));
		break;
	case fmtcl::TransCurve_2020_12:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.0993, 0.0181, 0.45, 4.5));
		break;
	case fmtcl::TransCurve_2084:
		ptr = OpSPtr (new TransOp2084 (inv_flag));
		break;
	case fmtcl::TransCurve_428:
		ptr = OpSPtr (new TransOpPow (inv_flag, 2.6, 48.0 / 52.37));
		break;
	case fmtcl::TransCurve_1886:
		ptr = OpSPtr (new TransOpPow (inv_flag, 2.4));
		break;
	case fmtcl::TransCurve_1886A:
		{
			const double   p1    = 2.6;
			const double   p2    = 3.0;
			const double   k0    = 0.35;
			const double   slope = pow (k0, p1 - p2);
			ptr = OpSPtr (new TransOpLinPow (
				inv_flag, 1, k0 / slope, 1.0 / p1, slope, 0, 1, 1, 1.0 / p2
			));
		}
		break;
	case fmtcl::TransCurve_FILMSTREAM:
		ptr = OpSPtr (new TransOpFilmStream (inv_flag));
		break;
	case fmtcl::TransCurve_SLOG:
		ptr = OpSPtr (new TransOpSLog (inv_flag));
		break;
	case fmtcl::TransCurve_LOGC2:
		ptr = OpSPtr (new TransOpLogC (inv_flag, true));
		break;
	case fmtcl::TransCurve_LOGC3:
		ptr = OpSPtr (new TransOpLogC (inv_flag, false));
		break;
	case fmtcl::TransCurve_CANONLOG:
		ptr = OpSPtr (new TransOpCanonLog (inv_flag));
		break;
	default:
		assert (false);
		break;
	}

	if (ptr.get () == 0)
	{
		ptr = OpSPtr (new TransOpBypass);
	}

	return (ptr);
}



template <class T>
T	Transfer::Convert <T>::cast (float val)
{
	return (T (fstb::conv_int_fast (val)));
}

template <>
float	Transfer::Convert <float>::cast (float val)
{
	return (val);
}

#if (fstb_ARCHI == fstb_ARCHI_X86)

template <class T>
void	Transfer::Convert <T>::store_sse2 (T *dst_ptr, __m128 val)
{
	_mm_store_si128 (
		reinterpret_cast <__m128i *> (dst_ptr),
		_mm_cvtps_epi32 (val)
	);
}

template <>
void	Transfer::Convert <float>::store_sse2 (float *dst_ptr, __m128 val)
{
	_mm_store_ps (dst_ptr, val);
}


#endif



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
