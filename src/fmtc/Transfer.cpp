/*****************************************************************************

        Transfer.cpp
        Author: Laurent de Soras, 2015

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
#include "fmtc/fnc.h"
#include "fmtcl/TransOp2084.h"
#include "fmtcl/TransOpAcesCc.h"
#include "fmtcl/TransOpAffine.h"
#include "fmtcl/TransOpBypass.h"
#include "fmtcl/TransOpCanonLog.h"
#include "fmtcl/TransOpCompose.h"
#include "fmtcl/TransOpContrast.h"
#include "fmtcl/TransOpErimm.h"
#include "fmtcl/TransOpFilmStream.h"
#include "fmtcl/TransOpHlg.h"
#include "fmtcl/TransOpLinPow.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransOpLogTrunc.h"
#include "fmtcl/TransOpPow.h"
#include "fmtcl/TransOpSLog.h"
#include "fmtcl/TransOpSLog3.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

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
,	_lvl_black (get_arg_flt (in, out, "blacklvl", 0))
,	_full_range_src_flag (get_arg_int (in, out, "fulls", 1) != 0)
,	_full_range_dst_flag (get_arg_int (in, out, "fulld", 1) != 0)
,	_curve_s (fmtcl::TransCurve_UNDEF)
,	_curve_d (fmtcl::TransCurve_UNDEF)
,	_loglut_flag (false)
,	_plane_processor (vsapi, *this, "transfer", true)
,	_lut_uptr ()
{
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
}



void	Transfer::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	_vsapi.setVideoInfo (&_vi_out, 1, &node);
	_plane_processor.set_filter (in, out, _vi_out);
}



const ::VSFrameRef *	Transfer::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);

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

		_lut_uptr->process_plane (
			data_dst_ptr, data_src_ptr, stride_dst, stride_src, w, h
		);
	}

	return (ret_val);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



const ::VSFormat &	Transfer::get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const
{
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

	// Linear or log LUT?
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
		    || _curve_d == fmtcl::TransCurve_HLG
		    || _curve_d == fmtcl::TransCurve_1886
		    || _curve_d == fmtcl::TransCurve_1886A
		    || _curve_d == fmtcl::TransCurve_SLOG
		    || _curve_d == fmtcl::TransCurve_SLOG2
		    || _curve_d == fmtcl::TransCurve_SLOG3
		    || _curve_d == fmtcl::TransCurve_LOGC2
		    || _curve_d == fmtcl::TransCurve_LOGC3
		    || _curve_d == fmtcl::TransCurve_CANONLOG
		    || _curve_d == fmtcl::TransCurve_ACESCC
		    || _curve_d == fmtcl::TransCurve_ERIMM)
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

	// Black level
	const double   lw = op_s->get_max ();
	if (_lvl_black > 0 && _lvl_black < lw)
	{
		/*
		Black level (brightness) and contrast settings as defined
		in ITU-R BT.1886:
			L = a' * fi (V + b')

		With:
			fi = EOTF (gamma to linear)
			L  = Lb for V = 0
			L  = Lw for V = Vmax

		For power functions, could be rewritten as:
			L = fi (a * V + b)

		Substitution:
			Lb = fi (           b)
			Lw = fi (a * Vmax + b)

		Then, given:
			f = OETF (linear to gamma)

		We get:
			f (Lb) = b
			f (Lw) = a * Vmax + b

			b =           f (Lb)
			a = (f (Lw) - f (Lb)) / Vmax
		*/
		OpSPtr         oetf = conv_curve_to_op (_curve_s, false);
		const double   lwg  = (*oetf) (lw        );
		const double   lbg  = (*oetf) (_lvl_black);
		const double   vmax =  lwg;
		const double   a    = (lwg - lbg) / vmax;
		const double   b    =        lbg;
		OpSPtr         op_a (new fmtcl::TransOpAffine (a, b));
		op_s = OpSPtr (new fmtcl::TransOpCompose (op_a, op_s));
	}

	// Gamma correction
	if (! fstb::is_eq (_gcor, 1.0))
	{
		OpSPtr         op_g (new fmtcl::TransOpPow (true, _gcor, 1, 1e6));
		op_d = OpSPtr (new fmtcl::TransOpCompose (op_g, op_d));
	}

	// Contrast
	if (! fstb::is_eq (_contrast, 1.0))
	{
		OpSPtr         op_c (new fmtcl::TransOpContrast (_contrast));
		op_d = OpSPtr (new fmtcl::TransOpCompose (op_c, op_d));
	}

	// LUTify
	OpSPtr         op_f (new fmtcl::TransOpCompose (op_s, op_d));

	const fmtcl::SplFmt  src_fmt = conv_vsfmt_to_splfmt (*_vi_in.format);
	const fmtcl::SplFmt  dst_fmt = conv_vsfmt_to_splfmt (*_vi_out.format);
	_lut_uptr = std::unique_ptr <fmtcl::TransLut> (new fmtcl::TransLut (
		*op_f, _loglut_flag,
		src_fmt, _vi_in.format->bitsPerSample, _full_range_src_flag,
		dst_fmt, _vi_out.format->bitsPerSample, _full_range_dst_flag,
		_sse2_flag, _avx2_flag
	));
}



// str should be already converted to lower case
fmtcl::TransCurve	Transfer::conv_string_to_curve (const vsutl::FilterBase &flt, const std::string &str)
{
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
	else if (str == "hlg")
	{
		c = fmtcl::TransCurve_HLG;
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
	else if (str == "adobergb")
	{
		c = fmtcl::TransCurve_ADOBE_RGB;
	}
	else if (str == "romm")
	{
		c = fmtcl::TransCurve_ROMM_RGB;
	}
	else if (str == "acescc")
	{
		c = fmtcl::TransCurve_ACESCC;
	}
	else if (str == "erimm")
	{
		c = fmtcl::TransCurve_ERIMM;
	}
	else if (str == "slog2")
	{
		c = fmtcl::TransCurve_SLOG2;
	}
	else if (str == "slog3")
	{
		c = fmtcl::TransCurve_SLOG3;
	}
	else if (str == "vlog")
	{
		c = fmtcl::TransCurve_VLOG;
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
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5));
		break;
	case fmtcl::TransCurve_470BG:
		ptr = OpSPtr (new fmtcl::TransOpPow (inv_flag, 2.8));
		break;
	case fmtcl::TransCurve_240:
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1.1115, 0.0228, 0.45, 4.0));
		break;
	case fmtcl::TransCurve_LINEAR:
		ptr = OpSPtr (new fmtcl::TransOpBypass);
		break;
	case fmtcl::TransCurve_LOG100:
		ptr = OpSPtr (new fmtcl::TransOpLogTrunc (inv_flag, 0.5, 0.01));
		break;
	case fmtcl::TransCurve_LOG316:
		ptr = OpSPtr (new fmtcl::TransOpLogTrunc (inv_flag, 0.4, sqrt (10) / 1000));
		break;
	case fmtcl::TransCurve_61966_2_4:
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5, -1e9, 1e9));
		break;
	case fmtcl::TransCurve_1361:
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5, -0.25, 1.33, 4));
		break;
	case fmtcl::TransCurve_470M:	// Assumed display gamma 2.2, almost like sRGB.
	case fmtcl::TransCurve_SRGB:
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1.055, 0.04045 / 12.92, 1.0 / 2.4, 12.92));
		break;
	case fmtcl::TransCurve_2020_12:
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1.0993, 0.0181, 0.45, 4.5));
		break;
	case fmtcl::TransCurve_2084:
		ptr = OpSPtr (new fmtcl::TransOp2084 (inv_flag));
		break;
	case fmtcl::TransCurve_428:
		ptr = OpSPtr (new fmtcl::TransOpPow (inv_flag, 2.6, 48.0 / 52.37));
		break;
	case fmtcl::TransCurve_HLG:
		ptr = OpSPtr (new fmtcl::TransOpHlg (inv_flag));
		break;
	case fmtcl::TransCurve_1886:
		ptr = OpSPtr (new fmtcl::TransOpPow (inv_flag, 2.4));
		break;
	case fmtcl::TransCurve_1886A:
		{
			const double   p1    = 2.6;
			const double   p2    = 3.0;
			const double   k0    = 0.35;
			const double   slope = pow (k0, p1 - p2);
			ptr = OpSPtr (new fmtcl::TransOpLinPow (
				inv_flag, 1, k0 / slope, 1.0 / p1, slope, 0, 1, 1, 1.0 / p2
			));
		}
		break;
	case fmtcl::TransCurve_FILMSTREAM:
		ptr = OpSPtr (new fmtcl::TransOpFilmStream (inv_flag));
		break;
	case fmtcl::TransCurve_SLOG:
		ptr = OpSPtr (new fmtcl::TransOpSLog (inv_flag, false));
		break;
	case fmtcl::TransCurve_LOGC2:
		ptr = OpSPtr (new fmtcl::TransOpLogC (inv_flag, fmtcl::TransOpLogC::Type_LOGC_V2));
		break;
	case fmtcl::TransCurve_LOGC3:
		ptr = OpSPtr (new fmtcl::TransOpLogC (inv_flag, fmtcl::TransOpLogC::Type_LOGC_V3));
		break;
	case fmtcl::TransCurve_CANONLOG:
		ptr = OpSPtr (new fmtcl::TransOpCanonLog (inv_flag));
		break;
	case fmtcl::TransCurve_ADOBE_RGB:
		ptr = OpSPtr (new fmtcl::TransOpPow (inv_flag, 563.0 / 256));
		break;
	case fmtcl::TransCurve_ROMM_RGB:
		ptr = OpSPtr (new fmtcl::TransOpLinPow (inv_flag, 1, 0.001953, 1.0 / 1.8, 16));
		break;
	case fmtcl::TransCurve_ACESCC:
		ptr = OpSPtr (new fmtcl::TransOpAcesCc (inv_flag));
		break;
	case fmtcl::TransCurve_ERIMM:
		ptr = OpSPtr (new fmtcl::TransOpErimm (inv_flag));
		break;
	case fmtcl::TransCurve_SLOG2:
		ptr = OpSPtr (new fmtcl::TransOpSLog (inv_flag, true));
		break;
	case fmtcl::TransCurve_SLOG3:
		ptr = OpSPtr (new fmtcl::TransOpSLog3 (inv_flag));
		break;
	case fmtcl::TransCurve_VLOG:
		ptr = OpSPtr (new fmtcl::TransOpLogC (inv_flag, fmtcl::TransOpLogC::Type_VLOG));
		break;
	default:
		assert (false);
		break;
	}

	if (ptr.get () == 0)
	{
		ptr = OpSPtr (new fmtcl::TransOpBypass);
	}

	return (ptr);
}



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
