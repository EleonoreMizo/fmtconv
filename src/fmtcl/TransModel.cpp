/*****************************************************************************

        TransModel.cpp
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

#include "fmtcl/TransCst.h"
#include "fmtcl/TransModel.h"
#include "fmtcl/TransOpAffine.h"
#include "fmtcl/TransOpBypass.h"
#include "fmtcl/TransOpCompose.h"
#include "fmtcl/TransOpContrast.h"
#include "fmtcl/TransOpLinPow.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransOpPow.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	TransModel::_max_nbr_planes;
constexpr double	TransModel::_min_luminance;



TransModel::TransModel (PicFmt dst_fmt, TransCurve curve_d, TransOpLogC::ExpIdx logc_ei_d, PicFmt src_fmt, TransCurve curve_s, TransOpLogC::ExpIdx logc_ei_s, double contrast, double gcor, double lb, double lws, double lwd, double lamb, bool scene_flag, LumMatch match, GyProc gy_proc, bool sse2_flag, bool avx2_flag)
{
	assert (dst_fmt.is_valid ());
	assert (TransCurve_is_valid (curve_d));
	assert (logc_ei_d >= 0);
	assert (logc_ei_d < TransOpLogC::ExpIdx_NBR_ELT);
	assert (src_fmt.is_valid ());
	assert (src_fmt._col_fam == dst_fmt._col_fam);
	assert (TransCurve_is_valid (curve_s));
	assert (logc_ei_s >= 0);
	assert (logc_ei_s < TransOpLogC::ExpIdx_NBR_ELT);
	assert (contrast > 0);
	assert (gcor > 0);
	assert (lb >= 0);
	assert (lws > _min_luminance || lws <= 0);
	assert (lwd > _min_luminance || lwd <= 0);
	assert (lamb > _min_luminance);
	assert (match >= 0);
	assert (match < LumMatch_NBR_ELT);

	_nbr_planes =
		(dst_fmt._col_fam == fmtcl::ColorFamily_GRAY) ? 1 : _max_nbr_planes;

	char           txt_0 [127+1];

	// Creates the requested transfer curves
	OpSPtr         op_s = TransUtil::conv_curve_to_op (curve_s, true , logc_ei_s);
	OpSPtr         op_d = TransUtil::conv_curve_to_op (curve_d, false, logc_ei_d);

	const auto     s_info = op_s->get_info ();
	const auto     d_info = op_d->get_info ();

	// Ranges for the function input.
	// Indicates that input values may be out of the [-1 ; 2] range.
	bool           large_range_s_flag = false;
	bool           large_range_d_flag = false;

	if (   curve_s == TransCurve_LINEAR
	    || curve_s == TransCurve_ACESCC)
	{
		large_range_s_flag = true;
	}

	// HDR curves may have a huge linear range
	// Also, other curves with extended range
	if (   d_info._range == TransOpInterface::Range::HDR
	    || (curve_d == TransCurve_LINEAR && large_range_s_flag)
	    || curve_d == TransCurve_SLOG
	    || curve_d == TransCurve_SLOG2
	    || curve_d == TransCurve_SLOG3
	    || curve_d == TransCurve_LOGC2
	    || curve_d == TransCurve_LOGC3
	    || curve_d == TransCurve_CANONLOG
	    || curve_d == TransCurve_ACESCC
	    || curve_d == TransCurve_ERIMM)
	{
		large_range_d_flag = true;
	}

	_dbg_txt += "range_s = ";
	_dbg_txt += (large_range_s_flag) ? "large" : "std";
	_dbg_txt += ", range_d = ";
	_dbg_txt += (large_range_d_flag) ? "large" : "std";

	// We try to estimate lw in cd/m^2 if not provided by the user
	if (! scene_flag)
	{
		estimate_lw (lws, s_info);
		estimate_lw (lwd, d_info);

		fstb::snprintf4all (txt_0, sizeof (txt_0),
			", lw_s = %.1f cd/m2, lw_d = %.1f cd/m2", lws, lwd
		);
		_dbg_txt += txt_0;
	}

	bool           gammay_flag = false; // HLG OOTF or OOTF^-1 (GammaY module)

	// Power-gain paths:
	// HLG OOTF       : xsl =  a_src *          xi   ^ g_src
	// User correction: xld =          (a_lin * xsl) ^ g_lin
	// HLG OOTF^-1    : xo  =          (a_dst * xld) ^ g_dst
	// Source and destination gamma and gain are applied in different orders,
	// depending if they belong to the OOTF or OOTF^-1 (reversed steps)
	// Direct : Fd =    scale              * E  ^    gamma
	// Inverse: E  = (1/scale) ^ (1/gamma) * Fd ^ (1/gamma)
	double         g_src = 1;
	double         a_src = 1;
	double         g_lin = gcor;
	double         a_lin = contrast;
	double         g_dst = 1;
	double         a_dst = 1;

	// Reference white for each function
	double         wref_s = s_info._wref;
	double         wref_d = d_info._wref;
	assert (wref_s > 0);
	assert (wref_d > 0);

	// Scene-referred
	if (scene_flag)
	{
		if (curve_s == TransCurve_2084)
		{
			const auto     op_ootf_inv = build_pq_ootf_inv ();
			op_s = compose (op_s, op_ootf_inv);
		}
		if (curve_d == TransCurve_2084)
		{
			const auto     op_ootf = build_pq_ootf ();
			op_d = compose (op_ootf, op_d);
		}
	}

	// Display-referred
	else
	{
		if (curve_s == TransCurve_HLG)
		{
			// OOTF
			gammay_flag = true;
			const double   g_sys = TransUtil::compute_hlg_gamma (lws, lamb);
			g_src  = g_sys;
			wref_s = pow (wref_s, g_sys);
			fstb::snprintf4all (txt_0, sizeof (txt_0),
				", sys_gam_s = %.3f", g_sys
			);
			_dbg_txt += txt_0;
		}

		if (curve_d == TransCurve_HLG)
		{
			// OOTF^-1
			gammay_flag = true;
			const double   g_sys = TransUtil::compute_hlg_gamma (lwd, lamb);
			g_dst  = 1 / g_sys;
			wref_d = pow (wref_d, g_sys);
			fstb::snprintf4all (txt_0, sizeof (txt_0),
				", sys_gam_d = %.3f", g_sys
			);
			_dbg_txt += txt_0;
		}
	}

	fstb::snprintf4all (txt_0, sizeof (txt_0),
		", ref_white_s = %.4f, ref_white_d = %.4f", wref_s, wref_d
	);
	_dbg_txt += txt_0;

	auto           scale_cdm2_s = s_info._scale_cdm2;
	auto           scale_cdm2_d = d_info._scale_cdm2;
	if (match == LumMatch_LUMINANCE)
	{
		// Luminance match is available only for display-referred signals
		if (scene_flag)
		{
			match = LumMatch_NONE;
		}

		else
		{
			// If we don't know the scale, use the peak white value.
			if (scale_cdm2_s <= 0)
			{
				scale_cdm2_s = lws;
			}
			if (scale_cdm2_d <= 0)
			{
				scale_cdm2_d = lwd;
			}

			// Scales must be specified for both transfer functions
			if (scale_cdm2_s <= 0 || scale_cdm2_d <= 0)
			{
				match = LumMatch_NONE;
			}
		}
	}

	// Matching
	if (match == LumMatch_REF_WHITE)
	{
		// Reference white matching
		a_src = 1 / wref_s;
		a_dst = wref_d;
	}
	else if (match == LumMatch_LUMINANCE)
	{
		assert (! scene_flag);
		assert (scale_cdm2_s > 0);
		assert (scale_cdm2_d > 0);

		// Tries to avoid overflows when using integer, either as temporary for
		// the GammaY stage (if any), either as linear input or output when one
		// of the transfer functions is bypassed.
		double         norm = 100.0; // 1.0 -> 100 cd/m^2
		norm = std::max (norm, scale_cdm2_s);
		norm = std::max (norm, scale_cdm2_d);
		norm = std::max (norm, lws);
		norm = std::max (norm, lwd);
		norm = std::max (norm, s_info._wpeak_cdm2);
		norm = std::max (norm, d_info._wpeak_cdm2);

		a_src = scale_cdm2_s / norm;
		a_dst = norm / scale_cdm2_d;
	}

	_dbg_txt += ", match = ";
	switch (match)
	{
	case LumMatch_NONE:      _dbg_txt += "none";      break;
	case LumMatch_REF_WHITE: _dbg_txt += "ref.white"; break;
	case LumMatch_LUMINANCE: _dbg_txt += "luminance"; break;
	default:                 _dbg_txt += "\?\?\?";    break;
	}

	fstb::snprintf4all (txt_0, sizeof (txt_0),
		", scale_s = %.4f, scale_d = %.4f", a_src, a_dst
	);
	_dbg_txt += txt_0;

	// Global gamma and gain:
	// xo  = gain * pow (xi, gamma)
	// With:
	// xo  = pow (a_dst * pow (a_lin * a_src * pow (xi, g_src), g_lin), g_dst)
	//     = pow (a_dst * pow (a_lin * a_src, g_lin), g_dst) * pow (xi, g_src * g_lin * g_dst)
	const double   gamma = g_src * g_lin * g_dst;
	const double   gain  = pow (a_dst * pow (a_lin * a_src, g_lin), g_dst);

	// Black level. Operates at the very beginning of the chain, on
	// the EOTF. We assume here that the linear range is display-referred.
	if (! scene_flag && lb > 0 && lb < lws)
	{
		// Scale to convert the lw and lb values in relative values (curve range)
		auto           lum_scale = s_info._scale_cdm2;
		if (curve_s == TransCurve_HLG)
		{
			lum_scale = lws;
		}
		else if (lum_scale <= 0)
		{
			lum_scale = 100;
		}
		assert (lum_scale > 0);

		/*
		Black level (brightness) and contrast settings as defined
		in ITU-R BT.1886, and called "b":
			L = a' * EOTF (V + b')

		With:
			L  = Lb for V = 0
			L  = Lw for V = Vmax

		For power functions, could be rewritten as:
			L = EOTF (a * V + b)

		Substitution:
			Lb = EOTF (           b)
			Lw = EOTF (a * Vmax + b)

		We get:
			EOTF^-1 (Lb) = b
			EOTF^-1 (Lw) = a * Vmax + b

			b =                 EOTF^-1 (Lb)
			a = (EOTF^-1 (Lw) - EOTF^-1 (Lb)) / Vmax
		*/
		auto           eotf_inv =
			TransUtil::conv_curve_to_op (curve_s, false, logc_ei_s);
		const double   lwg  = (*eotf_inv) (lws / lum_scale);
		const double   lbg  = (*eotf_inv) (lb  / lum_scale);
		const double   vmax =  lwg;
		const double   a    = (lwg - lbg) / vmax;
		const double   b    =        lbg;
		auto           op_a = std::make_shared <TransOpAffine> (a, b);
		op_s = compose (op_a, op_s);
	}

	// Operator for the linear-light part when GammaY is not used
	OpSPtr         op_l;

	// Do we really need a GammaY stage?
	if (fstb::is_eq (gamma, 1.0) || _nbr_planes < _max_nbr_planes)
	{
		gammay_flag = false;
	}

	// User setting override
	gammay_flag =
		((gammay_flag && gy_proc != GyProc::OFF) || gy_proc == GyProc::ON);

	if (! gammay_flag)
	{
		// Gamma correction
		if (! fstb::is_eq (gamma, 1.0))
		{
			auto           op_g =
				std::make_shared <TransOpPow> (true, gamma, 1, 1e6);
			op_l = compose (op_l, op_g);
		}

		// Contrast
		if (! fstb::is_eq (gain, 1.0))
		{
			auto           op_c =
				std::make_shared <TransOpContrast> (gain);
			op_l = compose (op_l, op_c);
		}
	}

	// Finds the processing scheme
	if (! gammay_flag)
	{
		_proc_mode = Proc::DIRECT;
		op_s = compose (compose (op_s, op_l), op_d);
		op_d.reset ();
	}
	else
	{
		const bool     s_flag =
			(dynamic_cast <const TransOpBypass *> (op_s.get ()) == nullptr);
		const bool     d_flag =
			(dynamic_cast <const TransOpBypass *> (op_d.get ()) == nullptr);

		if (s_flag && d_flag)
		{
			_proc_mode = Proc::SGD;
		}
		else if (s_flag)
		{
			_proc_mode = Proc::SG;
			op_d.reset ();
			assert (dst_fmt._full_flag || dst_fmt._sf == SplFmt_FLOAT);
		}
		else if (d_flag)
		{
			_proc_mode = Proc::GD;
			op_s.reset ();
			assert (src_fmt._full_flag || src_fmt._sf == SplFmt_FLOAT);
		}
		else
		{
			// Case not handled
			assert (false);
		}
	}

	_dbg_txt += ", proc = ";
	switch (_proc_mode)
	{
	case Proc::DIRECT: _dbg_txt += "direct";        break;
	case Proc::SG:     _dbg_txt += "src+gamma";     break;
	case Proc::GD:     _dbg_txt += "gamma+dst";     break;
	case Proc::SGD:    _dbg_txt += "src+gamma+dst"; break;
	}

	// LUTify
	const int      bps_s = src_fmt._res >> 3;
	const int      bps_d = dst_fmt._res >> 3;
	_max_len = _max_seg_len / std::max (bps_s, bps_d);

	if (op_s.get () != nullptr)
	{
		bool           loglut_flag = TransLut::is_loglut_req (*op_s);
		loglut_flag |= large_range_s_flag;
		_dbg_txt += ", lut_s = ";
		_dbg_txt += (loglut_flag) ? "log" : "lin";

		const bool     fulld_flag = (dst_fmt._full_flag || gammay_flag);
		_lut_s_uptr = std::make_unique <TransLut> (
			*op_s, loglut_flag,
			src_fmt._sf, src_fmt._res, src_fmt._full_flag,
			dst_fmt._sf, dst_fmt._res, fulld_flag,
			sse2_flag, avx2_flag
		);
		src_fmt = dst_fmt;
	}
	if (gammay_flag)
	{
		_gamma_y_uptr = std::make_unique <GammaY> (
			src_fmt._sf, src_fmt._res,
			dst_fmt._sf, dst_fmt._res,
			gamma, gain,
			sse2_flag, avx2_flag
		);
		src_fmt = dst_fmt;
	}
	if (op_d.get () != nullptr)
	{
		bool           loglut_flag = TransLut::is_loglut_req (*op_d);
		loglut_flag |= large_range_d_flag;
		_dbg_txt += ", lut_d = ";
		_dbg_txt += (loglut_flag) ? "log" : "lin";

		const bool     fulls_flag = (src_fmt._full_flag || gammay_flag);
		_lut_d_uptr = std::make_unique <TransLut> (
			*op_d, loglut_flag,
			src_fmt._sf, src_fmt._res, fulls_flag,
			dst_fmt._sf, dst_fmt._res, dst_fmt._full_flag,
			sse2_flag, avx2_flag
		);
		src_fmt = dst_fmt;
	}
}



const std::string &		TransModel::get_debug_text () const noexcept
{
	return _dbg_txt;
}



void	TransModel::process_frame (const ProcComp3Arg &arg) const noexcept
{
	switch (_proc_mode)
	{
	case Proc::DIRECT: process_frame_direct (arg); break;
	case Proc::SG:     process_frame_sg (arg);     break;
	case Proc::GD:     process_frame_gd (arg);     break;
	case Proc::SGD:    process_frame_sgd (arg);    break;
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	TransModel::process_frame_direct (const ProcComp3Arg &arg) const noexcept
{
	assert (_lut_s_uptr.get () != nullptr);

	for (int p_idx = 0; p_idx < _nbr_planes; ++p_idx)
	{
		_lut_s_uptr->process_plane (
			arg._dst [p_idx], arg._src [p_idx], arg._w, arg._h
		);
	}
}



void	TransModel::process_frame_sg (const ProcComp3Arg &arg) const noexcept
{
	assert (_nbr_planes == _max_nbr_planes);
	assert (_lut_s_uptr.get ()   != nullptr);
	assert (_gamma_y_uptr.get () != nullptr);

	alignas (64) SegArray   tmp_seg;

	auto           line_src = arg._src;
	auto           line_dst = arg._dst;
	const Frame <> tmp {
		Plane <> { tmp_seg [0].data (), 0 },
		Plane <> { tmp_seg [1].data (), 0 },
		Plane <> { tmp_seg [2].data (), 0 }
	};

	for (int y = 0; y < arg._h; ++y)
	{
		auto           src = line_src;
		auto           dst = line_dst;

		for (int x = 0; x < arg._w; x += _max_len)
		{
			const int      work_w = std::min (arg._w - x, _max_len);

			for (int p_idx = 0; p_idx < _nbr_planes; ++p_idx)
			{
				_lut_s_uptr->process_plane (tmp [p_idx], src [p_idx], work_w, 1);
			}

			_gamma_y_uptr->process_plane (dst, tmp, work_w, 1);

			src.step_pix (_max_seg_len);
			dst.step_pix (_max_seg_len);
		}

		line_src.step_line ();
		line_dst.step_line ();
	}
}



void	TransModel::process_frame_gd (const ProcComp3Arg &arg) const noexcept
{
	assert (_nbr_planes == _max_nbr_planes);
	assert (_gamma_y_uptr.get () != nullptr);
	assert (_lut_d_uptr.get ()   != nullptr);

	alignas (64) SegArray   tmp_seg;

	auto           line_src = arg._src;
	auto           line_dst = arg._dst;
	const Frame <> tmp {
		Plane <> { tmp_seg [0].data (), 0 },
		Plane <> { tmp_seg [1].data (), 0 },
		Plane <> { tmp_seg [2].data (), 0 }
	};

	for (int y = 0; y < arg._h; ++y)
	{
		auto           src = line_src;
		auto           dst = line_dst;

		for (int x = 0; x < arg._w; x += _max_len)
		{
			const int      work_w = std::min (arg._w - x, _max_len);

			_gamma_y_uptr->process_plane (tmp, src, work_w, 1);

			for (int p_idx = 0; p_idx < _nbr_planes; ++p_idx)
			{
				_lut_d_uptr->process_plane (dst [p_idx], tmp [p_idx], work_w, 1);
			}

			src.step_pix (_max_seg_len);
			dst.step_pix (_max_seg_len);
		}

		line_src.step_line ();
		line_dst.step_line ();
	}
}



void	TransModel::process_frame_sgd (const ProcComp3Arg &arg) const noexcept
{
	assert (_nbr_planes == _max_nbr_planes);
	assert (_lut_s_uptr.get ()   != nullptr);
	assert (_gamma_y_uptr.get () != nullptr);
	assert (_lut_d_uptr.get ()   != nullptr);

	alignas (64) SegArray   tmp_seg_s;
	alignas (64) SegArray   tmp_seg_d;

	auto           line_src = arg._src;
	auto           line_dst = arg._dst;
	const Frame <> tmp_s {
		Plane <> { tmp_seg_s [0].data (), 0 },
		Plane <> { tmp_seg_s [1].data (), 0 },
		Plane <> { tmp_seg_s [2].data (), 0 }
	};
	const Frame <> tmp_d {
		Plane <> { tmp_seg_d [0].data (), 0 },
		Plane <> { tmp_seg_d [1].data (), 0 },
		Plane <> { tmp_seg_d [2].data (), 0 }
	};

	for (int y = 0; y < arg._h; ++y)
	{
		auto           src = line_src;
		auto           dst = line_dst;

		for (int x = 0; x < arg._w; x += _max_len)
		{
			const int      work_w = std::min (arg._w - x, _max_len);

			for (int p_idx = 0; p_idx < _nbr_planes; ++p_idx)
			{
				_lut_s_uptr->process_plane (tmp_s [p_idx], src [p_idx], work_w, 1);
			}

			_gamma_y_uptr->process_plane (tmp_d, tmp_s, work_w, 1);

			for (int p_idx = 0; p_idx < _nbr_planes; ++p_idx)
			{
				_lut_d_uptr->process_plane (dst [p_idx], tmp_d [p_idx], work_w, 1);
			}

			src.step_pix (_max_seg_len);
			dst.step_pix (_max_seg_len);
		}

		line_src.step_line ();
		line_dst.step_line ();
	}
}



void	TransModel::estimate_lw (double &lw, const TransOpInterface::LinInfo &info)
{
	if (lw <= 0)
	{
		if (info._wpeak_cdm2 > 0)
		{
			lw = info._wpeak_cdm2;
		}
		else
		{
			if (info._range == TransOpInterface::Range::HDR)
			{
				lw = 1000;
			}
			else if (info._range == TransOpInterface::Range::SDR)
			{
				lw = 100;
			}
			else
			{
				lw = 100; // Default value
			}
		}
	}
}



TransModel::OpSPtr	TransModel::compose (OpSPtr op_1_sptr, OpSPtr op_2_sptr)
{
	assert (op_1_sptr.get () != nullptr || op_2_sptr.get () != nullptr);

	return
		  (op_2_sptr.get () == nullptr) ? op_1_sptr
		: (op_1_sptr.get () == nullptr) ? op_2_sptr
		: std::make_shared <TransOpCompose> (op_1_sptr, op_2_sptr);
}



// OOTF_PQ = [0-1] -> *range -> OETF_709 -> EOTF_1886 -> [0-10000]
// range   = 59.5208
TransModel::OpSPtr	TransModel::build_pq_ootf ()
{
	// Linear range, no unit
	const double   range_709_10k = compute_pq_sceneref_range_709 ();
	auto           op_gain_pre =
		std::make_shared <TransOpContrast> (range_709_10k);

	auto           op_709 = std::make_shared <TransOpLinPow> (
		false,
		TransCst::_bt709_alpha,
		TransCst::_bt709_beta,
		TransCst::_bt709_power,
		TransCst::_bt709_slope,
		0, range_709_10k
	);

	// Subsequent EOTF_PQ takes 0-10000 cd/m2 in range [0-1].
	// EOTF_1886 assumes range [0-1] is 100 cd/m2
	const double   gain_post = TransCst::_bt2100_pq_lw / TransCst::_bt1886_lw;
	const double   alpha     = pow (gain_post, 1 / TransCst::_bt1886_gamma);
	auto           op_1886   = std::make_shared <TransOpPow> (
		true, TransCst::_bt1886_gamma,
		alpha, alpha, TransCst::_bt1886_lw
	);

	auto           op_ootf =
		compose (op_gain_pre, compose (op_709, op_1886));

	return op_ootf;
}



// OOTF_PQ^-1 = [0-10000] -> EOTF_1886^-1 -> OETF_709^-1 -> /range -> [0-1]
// range      = 59.5208
TransModel::OpSPtr	TransModel::build_pq_ootf_inv ()
{
	// Preceding EOTF_PQ outputs 0-10000 cd/m2 in range [0-1].
	// EOTF_1886^-1 assumes range [0-1] is 100 cd/m2
	const double   gain_pre = TransCst::_bt2100_pq_lw / TransCst::_bt1886_lw;
	const double   alpha    = pow (gain_pre, 1 / TransCst::_bt1886_gamma);
	auto           op_1886i = std::make_shared <TransOpPow> (
		false, TransCst::_bt1886_gamma,
		alpha, alpha, TransCst::_bt1886_lw
	);

	// Linear range, no unit
	const double   range_709_10k = compute_pq_sceneref_range_709 ();
	auto           op_709i = std::make_shared <TransOpLinPow> (
		true,
		TransCst::_bt709_alpha,
		TransCst::_bt709_beta,
		TransCst::_bt709_power,
		TransCst::_bt709_slope,
		0, range_709_10k
	);

	auto           op_gain_post =
		std::make_shared <TransOpContrast> (1 / range_709_10k);

	auto           op_ootf_inv =
		compose (compose (op_1886i, op_709i), op_gain_post);

	return op_ootf_inv;
}



// Linear range for the PQ scene signal at the input of the BT.709 OETF
// 10000 cd/m^2 -> EOTF_1886^-1 -> OETF_709^-1 -> upper bound of the range
double	TransModel::compute_pq_sceneref_range_709 ()
{
	constexpr double  lw_ratio = TransCst::_bt2100_pq_lw / TransCst::_bt1886_lw;

	// EOTF_1886^-1 output
	const double   ecode =
		pow (lw_ratio, 1 / TransCst::_bt1886_gamma);

	// OETF_709^-1 output
	const double   range = pow (
		(ecode + (TransCst::_bt709_alpha - 1)) / TransCst::_bt709_alpha,
		1 / TransCst::_bt709_power
	);

	// Validity check. Exact value is 59.520834...
	assert (range > 59.520 && range < 59.521);

	return range;
}



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
