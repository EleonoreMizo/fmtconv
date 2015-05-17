/*****************************************************************************

        Matrix2020CL.cpp
        Author: Laurent de Soras, 2013

TO DO:
	- SSE2/AVX2 optimizations

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

#include "fmtcl/ColorSpaceH265.h"
#include "fmtc/Matrix2020CL.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#include <algorithm>

#include <cassert>



namespace fmtc
{



const double	Matrix2020CL::_coef_rgb_to_y_dbl [NBR_PLANES] =
{
	0.2627,
	0.6780,
	0.0593
};

const double	Matrix2020CL::_coef_ryb_to_g_dbl [NBR_PLANES] =
{
	-_coef_rgb_to_y_dbl [Col_R] / _coef_rgb_to_y_dbl [Col_G],
	+1                          / _coef_rgb_to_y_dbl [Col_G],
	-_coef_rgb_to_y_dbl [Col_B] / _coef_rgb_to_y_dbl [Col_G]
};

const double	Matrix2020CL::_coef_cb_neg = 1.9404;
const double	Matrix2020CL::_coef_cb_pos = 1.5816;
const double	Matrix2020CL::_coef_cr_neg = 1.7184;
const double	Matrix2020CL::_coef_cr_pos = 0.9936;

const double	Matrix2020CL::_alpha_b12   = 1.0993;
const double	Matrix2020CL::_alpha_low   = 1.099 ;
const double	Matrix2020CL::_beta_b12    = 0.0181;
const double	Matrix2020CL::_beta_low    = 0.018 ;

const double	Matrix2020CL::_slope_lin   = 4.5;
const double	Matrix2020CL::_gam_pow     = 0.45;



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Matrix2020CL::Matrix2020CL (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "matrix2020cl", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_sse_flag (false)
,	_sse2_flag (false)
,	_range_set_flag (false)
,	_full_range_flag (false)
,	_b12_flag (false)
,	_to_yuv_flag (false)
,	_flt_flag (false)
/*,	_coef_rgby_int [NBR_PLANES];
,	_map_gamma_int [1 << RGB_INT_BITS];*/
,	_coef_yg_a_int (0)
,	_coef_yg_b_int (0)
/*,	_coef_cb_a_int [2]
,	_coef_cr_a_int [2]*/
,	_coef_cbcr_b_int (0)
,	_apply_matrix_ptr (0)
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&vsapi != 0);

	vsutl::CpuOpt  cpu_opt (*this, in, out);
	_sse_flag  = cpu_opt.has_sse ();
	_sse2_flag = cpu_opt.has_sse2 ();

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	const ::VSFormat &   fmt_src = *_vi_in.format;

	if (fmt_src.subSamplingW != 0 || fmt_src.subSamplingH != 0)
	{
		throw_inval_arg ("input must be 4:4:4.");
	}
	if (fmt_src.numPlanes != NBR_PLANES)
	{
		throw_inval_arg ("greyscale format not supported.");
	}
	if (   fmt_src.colorFamily != ::cmRGB
	    && fmt_src.colorFamily != ::cmYUV)
	{
		throw_inval_arg ("Only RGB and YUV color families are supported.");
	}
	if (   (   fmt_src.sampleType == ::stInteger
	        && fmt_src.bitsPerSample !=  8
	        && fmt_src.bitsPerSample !=  9
	        && fmt_src.bitsPerSample != 10
	        && fmt_src.bitsPerSample != 12
	        && fmt_src.bitsPerSample != 16)
	    || (   fmt_src.sampleType == ::stFloat
	        && fmt_src.bitsPerSample != 32))
	{
		throw_inval_arg ("pixel bitdepth not supported.");
	}
	if (   fmt_src.colorFamily   == ::cmRGB
	    && fmt_src.sampleType    == ::stInteger
		 && fmt_src.bitsPerSample != RGB_INT_BITS)
	{
		throw_inval_arg ("input clip: RGB depth cannot be less than 16 bits.");
	}

	// Destination colorspace
	const ::VSFormat& fmt_dst = get_output_colorspace (in, out, core, fmt_src);

	if (   fmt_dst.colorFamily != ::cmRGB
	    && fmt_dst.colorFamily != ::cmYUV)
	{
		throw_inval_arg ("unsupported color family for output.");
	}
	if (   (   fmt_dst.sampleType == ::stInteger
	        && fmt_dst.bitsPerSample !=  8
	        && fmt_dst.bitsPerSample !=  9
	        && fmt_dst.bitsPerSample != 10
	        && fmt_dst.bitsPerSample != 12
	        && fmt_dst.bitsPerSample != 16)
	    || (   fmt_dst.sampleType == ::stFloat
	        && fmt_dst.bitsPerSample != 32))
	{
		throw_inval_arg ("output bitdepth not supported.");
	}
	if (   fmt_src.colorFamily   == ::cmRGB
	    && fmt_src.sampleType    == ::stInteger
		 && fmt_src.bitsPerSample != RGB_INT_BITS)
	{
		throw_inval_arg ("output clip: RGB depth cannot be less than 16 bits.");
	}

	// Compatibility
	if (   fmt_dst.sampleType   != fmt_src.sampleType
	    || fmt_dst.subSamplingW != fmt_src.subSamplingW
	    || fmt_dst.subSamplingH != fmt_src.subSamplingH
	    || fmt_dst.numPlanes    != fmt_src.numPlanes)
	{
		throw_inval_arg (
			"specified output colorspace is not compatible with the input."
		);
	}
	if (fmt_dst.colorFamily == fmt_src.colorFamily)
	{
		throw_inval_arg (
			"Input and output clips must be of different color families."
		);
	}

	// Output format is validated.
	_vi_out.format = &fmt_dst;
	_flt_flag      = (fmt_dst.sampleType    == ::stFloat);
	_b12_flag      = (fmt_dst.bitsPerSample >= 12);
	_to_yuv_flag   = (fmt_dst.colorFamily   == ::cmYUV);

	// Range
	const ::VSFormat &   fmt_yuv = (_to_yuv_flag) ? fmt_dst : fmt_src;
	_full_range_flag = (get_arg_int (
		in, out, "full" ,
		vsutl::is_full_range_default (fmt_yuv) ? 1 : 0,
		0, &_range_set_flag
	) != 0);

	// Tables, coefficients and functions
	if (_to_yuv_flag)
	{
		setup_rgb_2_ycbcr ();
	}
	else
	{
		setup_ycbcr_2_rgb ();
	}

	if (_apply_matrix_ptr == 0)
	{
		assert (false);
		throw_logic_err (
			"missing function initialisation. "
			"Please contact the plug-in developer and tell him how dumb he is."
		);
	}
}



void	Matrix2020CL::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&node != 0);
	assert (&core != 0);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Matrix2020CL::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
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

		const int      w  =  _vsapi.getFrameWidth (&src, 0);
		const int      h  =  _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		assert (_apply_matrix_ptr != 0);
		(this->*_apply_matrix_ptr) (*dst_ptr, src, w, h);

		// Output frame properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

		const fmtcl::ColorSpaceH265   cs_out =
			  (_to_yuv_flag)
			? fmtcl::ColorSpaceH265_BT2020CL
			: fmtcl::ColorSpaceH265_RGB;
		_vsapi.propSetInt (&dst_prop, "_ColorSpace", cs_out, ::paReplace);

		if (! _to_yuv_flag || _range_set_flag)
		{
			const int      cr_val = (! _to_yuv_flag || _full_range_flag) ? 0 : 1;
			_vsapi.propSetInt (&dst_prop, "_ColorRange", cr_val, ::paReplace);
		}
	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat &	Matrix2020CL::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const
{
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&fmt_src != 0);

	const ::VSFormat *   fmt_dst_ptr = &fmt_src;
	int            col_fam  = fmt_dst_ptr->colorFamily;
	int            bits     = fmt_dst_ptr->bitsPerSample;
	int            spl_type = fmt_dst_ptr->sampleType;

	// Automatic default conversion
	if (col_fam == ::cmRGB)
	{
		col_fam = ::cmYUV;
	}
	else
	{
		col_fam = ::cmRGB;
		if (spl_type == ::stInteger)
		{
			bits = RGB_INT_BITS;
		}
	}

	// Full colorspace
	int            csp_dst = get_arg_int (in, out, "csp", ::pfNone);
	if (csp_dst != ::pfNone)
	{
		fmt_dst_ptr = _vsapi.getFormatPreset (csp_dst, &core);
		if (fmt_dst_ptr == 0)
		{
			throw_inval_arg ("unknown output colorspace.");
		}
		col_fam  = fmt_dst_ptr->colorFamily;
		bits     = fmt_dst_ptr->bitsPerSample;
		spl_type = fmt_dst_ptr->sampleType;
	}

	int            ssh = fmt_dst_ptr->subSamplingW;
	int            ssv = fmt_dst_ptr->subSamplingH;

	// Destination bit depth
	bits = get_arg_int (in, out, "bits", bits);

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
	catch (std::exception &)
	{
		throw;
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



void	Matrix2020CL::setup_rgb_2_ycbcr ()
{
	if (_flt_flag)
	{
	   _apply_matrix_ptr = &Matrix2020CL::conv_rgb_2_ycbcr_cpp_flt;
	}

	else
	{
#define Matrix2020CL_CASE_RGB2_INT_CPP( DT, DB) \
		case	DB: \
			_apply_matrix_ptr = &ThisType::conv_rgb_2_ycbcr_cpp_int <DT, DB>; \
			break;

		switch (_vi_out.format->bitsPerSample)
		{
		Matrix2020CL_CASE_RGB2_INT_CPP (uint16_t, 16)
		Matrix2020CL_CASE_RGB2_INT_CPP (uint16_t, 12)
		Matrix2020CL_CASE_RGB2_INT_CPP (uint16_t, 10)
		Matrix2020CL_CASE_RGB2_INT_CPP (uint16_t,  9)
		Matrix2020CL_CASE_RGB2_INT_CPP (uint8_t ,  8)
		default:
			assert (false);
			throw_logic_err (
				"unhandled case for CPP function assignation. "
				"Please contact the plug-in developer."
			);
			break;
		}

#undef Matrix2020CL_CASE_RGB2_INT_CPP

		// RGB -> Y
		int            sum = 0;
		for (int p = 0; p < NBR_PLANES - 1; ++p)
		{
			_coef_rgby_int [p] = static_cast <int16_t> (fstb::round_int (
				_coef_rgb_to_y_dbl [p] * (1 << SHIFT_INT)
			));
			sum += _coef_rgby_int [p];
		}
		_coef_rgby_int [NBR_PLANES - 1] =
			static_cast <int16_t> ((1 << SHIFT_INT) - sum);

		// E -> E'
		const int      table_size = 1 << RGB_INT_BITS;
		for (int v = 0; v < table_size; ++v)
		{
			const double   v_lin = double (v) / double (table_size - 1);
			const double   v_gam = map_lin_to_gam (v_lin, _b12_flag);
			_map_gamma_int [v] =
				static_cast <uint16_t> (fstb::round_int (v_gam * (table_size - 1)));
		}

		// Scaling coefficients
		double         a_y;
		double         b_y;
		double         a_c;
		double         b_c;
		::VSFormat     fmt_fake (*_vi_out.format);
		fmt_fake.bitsPerSample = RGB_INT_BITS;
		vsutl::compute_fmt_mac_cst (
			a_y, b_y, fmt_fake, _full_range_flag, fmt_fake, true, 0
		);
		vsutl::compute_fmt_mac_cst (
			a_c, b_c, fmt_fake, _full_range_flag, fmt_fake, true, 1
		);
		const int      dif_bits   = RGB_INT_BITS - _vi_out.format->bitsPerSample;
		const int      coef_scale = 1 <<  SHIFT_INT;
		const int      cst_r      = 1 << (SHIFT_INT + dif_bits - 1);
		const int      ofs_grey   = 1 << (SHIFT_INT + RGB_INT_BITS - 1);
		const double   coef_a_y   = a_y * coef_scale;
		const double   coef_b_y   = b_y * coef_scale + cst_r;
		const double	coef_a_c   = a_c * coef_scale;
		const double   coef_a_cb0 = coef_a_c / _coef_cb_pos;
		const double   coef_a_cb1 = coef_a_c / _coef_cb_neg;
		const double   coef_a_cr0 = coef_a_c / _coef_cr_pos;
		const double   coef_a_cr1 = coef_a_c / _coef_cr_neg;
		const double   coef_b_c   = ofs_grey + cst_r;
		_coef_yg_a_int     = fstb::round_int (coef_a_y  );
		_coef_yg_b_int     = fstb::round_int (coef_b_y  );
		_coef_cb_a_int [0] = fstb::round_int (coef_a_cb0);
		_coef_cb_a_int [1] = fstb::round_int (coef_a_cb1);
		_coef_cr_a_int [0] = fstb::round_int (coef_a_cr0);
		_coef_cr_a_int [1] = fstb::round_int (coef_a_cr1);
		_coef_cbcr_b_int   = fstb::round_int (coef_b_c  );
	}
}



void	Matrix2020CL::setup_ycbcr_2_rgb ()
{
	if (_flt_flag)
	{
	   _apply_matrix_ptr = &Matrix2020CL::conv_ycbcr_2_rgb_cpp_flt;
	}

	else
	{
#define Matrix2020CL_CASE_2RGB_INT_CPP( ST, SB) \
		case	SB: \
			_apply_matrix_ptr = &ThisType::conv_ycbcr_2_rgb_cpp_int <ST, SB>; \
			break;

		switch (_vi_in.format->bitsPerSample)
		{
		Matrix2020CL_CASE_2RGB_INT_CPP (uint16_t, 16)
		Matrix2020CL_CASE_2RGB_INT_CPP (uint16_t, 12)
		Matrix2020CL_CASE_2RGB_INT_CPP (uint16_t, 10)
		Matrix2020CL_CASE_2RGB_INT_CPP (uint16_t,  9)
		Matrix2020CL_CASE_2RGB_INT_CPP (uint8_t ,  8)
		default:
			assert (false);
			throw_logic_err (
				"unhandled case for CPP function assignation. "
				"Please contact the plug-in developer."
			);
			break;
		}

#undef Matrix2020CL_CASE_2RGB_INT_CPP

		// YBR -> G
		for (int p = 0; p < NBR_PLANES; ++p)
		{
			_coef_rgby_int [p] = static_cast <int16_t> (fstb::round_int (
				_coef_ryb_to_g_dbl [p] * (1 << SHIFT_INT)
			));
		}

		// E' -> E
		const int      table_size = 1 << RGB_INT_BITS;
		for (int v = 0; v < table_size; ++v)
		{
			const double   v_gam = double (v) / double (table_size - 1);
			const double   v_lin = map_gam_to_lin (v_gam, _b12_flag);
			_map_gamma_int [v] =
				static_cast <uint16_t> (fstb::round_int (v_lin * (table_size - 1)));
		}

		// Scaling coefficients
		double         a_y;
		double         b_y;
		double         a_c;
		double         b_c;
		::VSFormat     fmt_fake (*_vi_in.format);
		vsutl::compute_fmt_mac_cst (
			a_y, b_y, fmt_fake, true, fmt_fake, _full_range_flag, 0
		);
		vsutl::compute_fmt_mac_cst (
			a_c, b_c, fmt_fake, true, fmt_fake, _full_range_flag, 1
		);
		const int      dif_bits   = RGB_INT_BITS - _vi_in.format->bitsPerSample;
		const int      coef_scale = 1 <<  SHIFT_INT;
		const int      cst_r      = 1 << (SHIFT_INT - dif_bits - 1);
		const double   coef_a_y   = a_y * coef_scale;
		const double   coef_b_y   = b_y * coef_scale + cst_r;
		const double	coef_a_c   = a_c * coef_scale;
		const double   coef_a_cb0 = coef_a_c * _coef_cb_pos;
		const double   coef_a_cb1 = coef_a_c * _coef_cb_neg;
		const double   coef_a_cr0 = coef_a_c * _coef_cr_pos;
		const double   coef_a_cr1 = coef_a_c * _coef_cr_neg;
		const double   coef_b_c   = cst_r;
		_coef_yg_a_int     = fstb::round_int (coef_a_y  );
		_coef_yg_b_int     = fstb::round_int (coef_b_y  );
		_coef_cb_a_int [0] = fstb::round_int (coef_a_cb0);
		_coef_cb_a_int [1] = fstb::round_int (coef_a_cb1);
		_coef_cr_a_int [0] = fstb::round_int (coef_a_cr0);
		_coef_cr_a_int [1] = fstb::round_int (coef_a_cr1);
		_coef_cbcr_b_int   = fstb::round_int (coef_b_c  );
	}
}



template <typename DT, int DB>
void	Matrix2020CL::conv_rgb_2_ycbcr_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);
	assert (_vi_in.format->bitsPerSample  == RGB_INT_BITS);
	assert (_vi_out.format->bitsPerSample == DB);

	const uint16_t *  src_0_dat_ptr = reinterpret_cast <const uint16_t *> (_vsapi.getReadPtr (&src, 0));
	const uint16_t *  src_1_dat_ptr = reinterpret_cast <const uint16_t *> (_vsapi.getReadPtr (&src, 1));
	const uint16_t *  src_2_dat_ptr = reinterpret_cast <const uint16_t *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	DT *           dst_0_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 0));
	DT *           dst_1_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 1));
	DT *           dst_2_dat_ptr = reinterpret_cast <DT *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	const int      ma    = (1 << DB) - 1;
	const int      shft2 = SHIFT_INT + RGB_INT_BITS - DB;
	const int      cst_r = 1 << (SHIFT_INT - 1);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      rl = src_0_dat_ptr [x];
			const int      gl = src_1_dat_ptr [x];
			const int      bl = src_2_dat_ptr [x];

			const int      yl = uint16_t (
				(  rl * _coef_rgby_int [Col_R]
				 + gl * _coef_rgby_int [Col_G]
				 + bl * _coef_rgby_int [Col_B]
			    +      cst_r                 ) >> SHIFT_INT);

			const int      yg = _map_gamma_int [yl];
			const int      bg = _map_gamma_int [bl];
			const int      rg = _map_gamma_int [rl];

			const int      cb = bg - yg;
			const int      cr = rg - yg;

			int            dy  = (yg * _coef_yg_a_int            + _coef_yg_b_int  ) >> shft2;
			int            dcb = (cb * _coef_cb_a_int [(cb < 0)] + _coef_cbcr_b_int) >> shft2;
			int            dcr = (cr * _coef_cr_a_int [(cr < 0)] + _coef_cbcr_b_int) >> shft2;

			if (DB < RGB_INT_BITS)
			{
				dy  = std::max (std::min (dy , ma), 0);
				dcb = std::max (std::min (dcb, ma), 0);
				dcr = std::max (std::min (dcr, ma), 0);
			}

			dst_0_dat_ptr [x] = static_cast <DT> (dy );
			dst_1_dat_ptr [x] = static_cast <DT> (dcb);
			dst_2_dat_ptr [x] = static_cast <DT> (dcr);
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



void	Matrix2020CL::conv_rgb_2_ycbcr_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	float *        dst_1_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 1));
	float *        dst_2_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    rl = src_0_dat_ptr [x];
			const float    gl = src_1_dat_ptr [x];
			const float    bl = src_2_dat_ptr [x];

			const float    yl =
				  rl * float (_coef_rgb_to_y_dbl [Col_R])
				+ gl * float (_coef_rgb_to_y_dbl [Col_G])
				+ bl * float (_coef_rgb_to_y_dbl [Col_B]);

			const float    yg = map_lin_to_gam (yl, true);
			const float    bg = map_lin_to_gam (bl, true);
			const float    rg = map_lin_to_gam (rl, true);

			const float    cb = bg - yg;
			const float    cr = rg - yg;

			const float    dy  = yg;
			const float    dcb =
				cb * ((cb < 0) ? float (1 / _coef_cb_neg) : float (1 / _coef_cb_pos));
			const float    dcr =
				cr * ((cr < 0) ? float (1 / _coef_cr_neg) : float (1 / _coef_cr_pos));

			dst_0_dat_ptr [x] = dy;
			dst_1_dat_ptr [x] = dcb;
			dst_2_dat_ptr [x] = dcr;
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



template <typename ST, int SB>
void	Matrix2020CL::conv_ycbcr_2_rgb_cpp_int (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);
	assert (_vi_in.format->bitsPerSample  == SB);
	assert (_vi_out.format->bitsPerSample == RGB_INT_BITS);

	const ST *     src_0_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 0));
	const ST *     src_1_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 1));
	const ST *     src_2_dat_ptr = reinterpret_cast <const ST *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	uint16_t *     dst_0_dat_ptr = reinterpret_cast <uint16_t *> (_vsapi.getWritePtr (&dst, 0));
	uint16_t *     dst_1_dat_ptr = reinterpret_cast <uint16_t *> (_vsapi.getWritePtr (&dst, 1));
	uint16_t *     dst_2_dat_ptr = reinterpret_cast <uint16_t *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	const int      shft2    = SHIFT_INT + SB - RGB_INT_BITS;
	const int      cst_r    = 1 << (SHIFT_INT - 1);
	const int      ma_int   = (1 << RGB_INT_BITS) - 1;
	const int      ofs_grey = 1 << (SB - 1);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      dy  = src_0_dat_ptr [x];
			const int      dcb = src_1_dat_ptr [x];
			const int      dcr = src_2_dat_ptr [x];

			const int      dcb0 = dcb - ofs_grey;
			const int      dcr0 = dcr - ofs_grey;

			int            yg = (dy   * _coef_yg_a_int              + _coef_yg_b_int  ) >> shft2;
			const int      cb = (dcb0 * _coef_cb_a_int [(dcb0 < 0)] + _coef_cbcr_b_int) >> shft2;
			const int      cr = (dcr0 * _coef_cr_a_int [(dcr0 < 0)] + _coef_cbcr_b_int) >> shft2;

			const int      bg = std::max (std::min (cb + yg, ma_int), 0);
			const int      rg = std::max (std::min (cr + yg, ma_int), 0);
			yg = std::max (std::min (yg, ma_int), 0);

			const int      yl = _map_gamma_int [yg];
			const int      bl = _map_gamma_int [bg];
			const int      rl = _map_gamma_int [rg];

			const int      gl = uint16_t (
				(  rl * _coef_rgby_int [Col_R]
				 + yl * _coef_rgby_int [Col_G]
				 + bl * _coef_rgby_int [Col_B]
			    +      cst_r                 ) >> SHIFT_INT);

			dst_0_dat_ptr [x] = static_cast <uint16_t> (rl);
			dst_1_dat_ptr [x] = static_cast <uint16_t> (gl);
			dst_2_dat_ptr [x] = static_cast <uint16_t> (bl);
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



void	Matrix2020CL::conv_ycbcr_2_rgb_cpp_flt (::VSFrameRef &dst, const ::VSFrameRef &src, int w, int h)
{
	assert (&dst != 0);
	assert (&src != 0);
	assert (w > 0);
	assert (h > 0);

	const float *  src_0_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 0));
	const float *  src_1_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 1));
	const float *  src_2_dat_ptr = reinterpret_cast <const float *> (_vsapi.getReadPtr (&src, 2));
	const int      src_0_str     = _vsapi.getStride (&src, 0) / int (sizeof (*src_0_dat_ptr));
	const int      src_1_str     = _vsapi.getStride (&src, 1) / int (sizeof (*src_1_dat_ptr));
	const int      src_2_str     = _vsapi.getStride (&src, 2) / int (sizeof (*src_2_dat_ptr));

	float *        dst_0_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 0));
	float *        dst_1_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 1));
	float *        dst_2_dat_ptr = reinterpret_cast <float *> (_vsapi.getWritePtr (&dst, 2));
	const int      dst_0_str     = _vsapi.getStride (&dst, 0) / int (sizeof (*dst_0_dat_ptr));
	const int      dst_1_str     = _vsapi.getStride (&dst, 1) / int (sizeof (*dst_1_dat_ptr));
	const int      dst_2_str     = _vsapi.getStride (&dst, 2) / int (sizeof (*dst_2_dat_ptr));

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const float    dy  = src_0_dat_ptr [x];
			const float    dcb = src_1_dat_ptr [x];
			const float    dcr = src_2_dat_ptr [x];

			const float    yg = dy;
			const float    cb =
				dcb * ((dcb < 0) ? float (_coef_cb_neg) : float (_coef_cb_pos));
			const float    cr =
				dcr * ((dcr < 0) ? float (_coef_cr_neg) : float (_coef_cr_pos));

			const float    bg = cb + yg;
			const float    rg = cr + yg;

			const float    yl = map_gam_to_lin (yg, true);
			const float    bl = map_gam_to_lin (bg, true);
			const float    rl = map_gam_to_lin (rg, true);

			const float    gl =
				  rl * float (_coef_ryb_to_g_dbl [Col_R])
				+ yl * float (_coef_ryb_to_g_dbl [Col_G])
				+ bl * float (_coef_ryb_to_g_dbl [Col_B]);

			dst_0_dat_ptr [x] = rl;
			dst_1_dat_ptr [x] = gl;
			dst_2_dat_ptr [x] = bl;
		}

		src_0_dat_ptr += src_0_str;
		src_1_dat_ptr += src_1_str;
		src_2_dat_ptr += src_2_str;

		dst_0_dat_ptr += dst_0_str;
		dst_1_dat_ptr += dst_1_str;
		dst_2_dat_ptr += dst_2_str;
	}
}



// Input and output ranges are typically ranging from 0 to 1, but we
// allow some overshoot.
template <typename T>
T	Matrix2020CL::map_lin_to_gam (T v_lin, bool b12_flag)
{
	const T        beta  = T ((b12_flag) ?  _beta_b12 :  _beta_low);
	const T        alpha = T ((b12_flag) ? _alpha_b12 : _alpha_low);
	const T     	v_gam = T (
			  (v_lin <= beta)
			? v_lin * T (_slope_lin)
			: alpha * pow (v_lin, T (_gam_pow)) - (alpha - 1)
		);

	return (v_gam);
}



// Input and output ranges are typically ranging from 0 to 1, but we
// allow some overshoot.
template <typename T>
T	Matrix2020CL::map_gam_to_lin (T v_gam, bool b12_flag)
{
	const T        beta  = T ((b12_flag) ?  _beta_b12 :  _beta_low);
	const T        alpha = T ((b12_flag) ? _alpha_b12 : _alpha_low);
	const T     	v_lin = T (
			  (v_gam <= T (beta * _slope_lin))
			? v_gam * T (1 / _slope_lin)
			: pow ((v_gam + (alpha - 1)) * (1 / alpha), T (1 / _gam_pow))
		);

	return (v_lin);
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
