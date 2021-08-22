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

#include "fmtcl/TransModel.h"
#include "fmtcl/TransOpAffine.h"
#include "fmtcl/TransOpCompose.h"
#include "fmtcl/TransOpContrast.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransOpPow.h"
#include "fstb/fnc.h"

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TransModel::TransModel (PicFmt dst_fmt, TransCurve curve_d, TransOpLogC::ExpIdx logc_ei_d, PicFmt src_fmt, TransCurve curve_s, TransOpLogC::ExpIdx logc_ei_s, double contrast, double gcor, double lvl_black, bool sse2_flag, bool avx2_flag)
{
	assert (dst_fmt.is_valid ());
	assert (TransCurve_is_valid (curve_d));
	assert (logc_ei_d >= 0);
	assert (logc_ei_d < TransOpLogC::ExpIdx_NBR_ELT);
	assert (src_fmt.is_valid ());
	assert (TransCurve_is_valid (curve_s));
	assert (logc_ei_s >= 0);
	assert (logc_ei_s < TransOpLogC::ExpIdx_NBR_ELT);
	assert (contrast > 0);
	assert (gcor > 0);
	assert (lvl_black >= 0);

	OpSPtr         op_s = TransUtil::conv_curve_to_op (curve_s, true , logc_ei_s);
	OpSPtr         op_d = TransUtil::conv_curve_to_op (curve_d, false, logc_ei_d);

	// Linear or log LUT?
	bool           loglut_flag = false;
	if (   SplFmt_is_float (src_fmt._sf)
	    && curve_s == TransCurve_LINEAR)
	{
		// Curves with extended range or with fast-evolving slope at 0.
		// Actually we could just use the log LUT for all the curves...?
		// 10 bits per stop + interpolation should be enough for all of them.
		// What about the speed?
		if (   curve_d == TransCurve_470BG
		    || curve_d == TransCurve_LINEAR
		    || curve_d == TransCurve_61966_2_4
		    || curve_d == TransCurve_2084
		    || curve_d == TransCurve_428
		    || curve_d == TransCurve_HLG
		    || curve_d == TransCurve_1886
		    || curve_d == TransCurve_1886A
		    || curve_d == TransCurve_SLOG
		    || curve_d == TransCurve_SLOG2
		    || curve_d == TransCurve_SLOG3
		    || curve_d == TransCurve_LOGC2
		    || curve_d == TransCurve_LOGC3
		    || curve_d == TransCurve_CANONLOG
		    || curve_d == TransCurve_ACESCC
		    || curve_d == TransCurve_ERIMM)
		{
			loglut_flag = true;
		}
		if (gcor < 0.5)
		{
			loglut_flag = true;
		}
		if (fabs (contrast) >= 3.0/2 || fabs (contrast) <= 2.0/3)
		{
			loglut_flag = true;
		}
	}

	// Black level
	const double   lw = op_s->get_max ();
	if (lvl_black > 0 && lvl_black < lw)
	{
		/*
		Black level (brightness) and contrast settings as defined
		in ITU-R BT.1886, and called "b":
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
		auto           oetf =
			TransUtil::conv_curve_to_op (curve_s, false, logc_ei_s);
		const double   lwg  = (*oetf) (lw       );
		const double   lbg  = (*oetf) (lvl_black);
		const double   vmax =  lwg;
		const double   a    = (lwg - lbg) / vmax;
		const double   b    =        lbg;
		auto           op_a = std::make_shared <fmtcl::TransOpAffine> (a, b);
		op_s = std::make_shared <fmtcl::TransOpCompose> (op_a, op_s);
	}

	// Gamma correction
	if (! fstb::is_eq (gcor, 1.0))
	{
		auto           op_g =
			std::make_shared <fmtcl::TransOpPow> (true, gcor, 1, 1e6);
		op_d = std::make_shared <fmtcl::TransOpCompose> (op_g, op_d);
	}

	// Contrast
	if (! fstb::is_eq (contrast, 1.0))
	{
		auto           op_c =
			std::make_shared <fmtcl::TransOpContrast> (contrast);
		op_d = std::make_shared <fmtcl::TransOpCompose> (op_c, op_d);
	}

	// LUTify
	auto           op_f = std::make_shared <fmtcl::TransOpCompose> (op_s, op_d);

	_lut_uptr = std::make_unique <fmtcl::TransLut> (
		*op_f, loglut_flag,
		src_fmt._sf, src_fmt._res, src_fmt._full_flag,
		dst_fmt._sf, dst_fmt._res, dst_fmt._full_flag,
		sse2_flag, avx2_flag
	);
}



void	TransModel::process_plane (const Plane <> &dst, const PlaneRO <> &src, int w, int h) const noexcept
{
	_lut_uptr->process_plane (dst, src, w, h);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
