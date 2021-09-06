/*****************************************************************************

        TransUtil.cpp
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

#include "fmtcl/TransOp2084.h"
#include "fmtcl/TransOpAcesCc.h"
#include "fmtcl/TransOpBypass.h"
#include "fmtcl/TransOpCanonLog.h"
#include "fmtcl/TransOpErimm.h"
#include "fmtcl/TransOpFilmStream.h"
#include "fmtcl/TransOpHlg.h"
#include "fmtcl/TransOpLinPow.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransOpLogTrunc.h"
#include "fmtcl/TransOpPow.h"
#include "fmtcl/TransOpSLog.h"
#include "fmtcl/TransOpSLog3.h"
#include "fmtcl/TransUtil.h"
#include "fstb/fnc.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// str should be already converted to lower case
TransCurve	TransUtil::conv_string_to_curve (const std::string &str)
{
	assert (! str.empty ());

	TransCurve c = TransCurve_UNDEF;
	if (str == "709")
	{
		c = TransCurve_709;
	}
	else if (str == "470m")
	{
		c = TransCurve_470M;
	}
	else if (str == "470bg")
	{
		c = TransCurve_470BG;
	}
	else if (str == "601")
	{
		c = TransCurve_601;
	}
	else if (str == "240")
	{
		c = TransCurve_240;
	}
	else if (str.empty () || str == "linear")
	{
		c = TransCurve_LINEAR;
	}
	else if (str == "log100")
	{
		c = TransCurve_LOG100;
	}
	else if (str == "log316")
	{
		c = TransCurve_LOG316;
	}
	else if (str == "61966-2-4")
	{
		c = TransCurve_61966_2_4;
	}
	else if (str == "1361")
	{
		c = TransCurve_1361;
	}
	else if (str == "61966-2-1" || str == "srgb" || str == "sycc")
	{
		c = TransCurve_SRGB;
	}
	else if (str == "2020_10")
	{
		c = TransCurve_2020_10;
	}
	else if (str == "2020_12" || str == "2020")
	{
		c = TransCurve_2020_12;
	}
	else if (str == "2084" || str == "pq")
	{
		c = TransCurve_2084;
	}
	else if (str == "428-1" || str == "428")
	{
		c = TransCurve_428;
	}
	else if (str == "hlg")
	{
		c = TransCurve_HLG;
	}
	else if (str == "1886")
	{
		c = TransCurve_1886;
	}
	else if (str == "1886a")
	{
		c = TransCurve_1886A;
	}
	else if (str == "filmstream")
	{
		c = TransCurve_FILMSTREAM;
	}
	else if (str == "slog")
	{
		c = TransCurve_SLOG;
	}
	else if (str == "logc2")
	{
		c = TransCurve_LOGC2;
	}
	else if (str == "logc3")
	{
		c = TransCurve_LOGC3;
	}
	else if (str == "canonlog")
	{
		c = TransCurve_CANONLOG;
	}
	else if (str == "adobergb")
	{
		c = TransCurve_ADOBE_RGB;
	}
	else if (str == "romm")
	{
		c = TransCurve_ROMM_RGB;
	}
	else if (str == "acescc")
	{
		c = TransCurve_ACESCC;
	}
	else if (str == "erimm")
	{
		c = TransCurve_ERIMM;
	}
	else if (str == "slog2")
	{
		c = TransCurve_SLOG2;
	}
	else if (str == "slog3")
	{
		c = TransCurve_SLOG3;
	}
	else if (str == "vlog")
	{
		c = TransCurve_VLOG;
	}
	else
	{
		assert (false);
	}

	return c;
}



TransUtil::OpSPtr	TransUtil::conv_curve_to_op (TransCurve c, bool inv_flag, TransOpLogC::ExpIdx logc_ei)
{
	assert (c >= 0);
	assert (logc_ei >= 0);
	assert (logc_ei < TransOpLogC::ExpIdx_NBR_ELT);

	OpSPtr         ptr;

	switch (c)
	{
	case TransCurve_709:
	case TransCurve_601:
	case TransCurve_2020_10:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5));
		break;
	case TransCurve_470BG:
		ptr = OpSPtr (new TransOpPow (inv_flag, 2.8));
		break;
	case TransCurve_240:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.1115, 0.0228, 0.45, 4.0));
		break;
	case TransCurve_LINEAR:
		ptr = OpSPtr (new TransOpBypass);
		break;
	case TransCurve_LOG100:
		ptr = OpSPtr (new TransOpLogTrunc (inv_flag, 0.5, 0.01));
		break;
	case TransCurve_LOG316:
		ptr = OpSPtr (new TransOpLogTrunc (inv_flag, 0.4, sqrt (10) / 1000));
		break;
	case TransCurve_61966_2_4:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5, -1e9, 1e9));
		break;
	case TransCurve_1361:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.099, 0.018, 0.45, 4.5, -0.25, 1.33, 4));
		break;
	case TransCurve_470M:	// Assumed display gamma 2.2, almost like sRGB.
	case TransCurve_SRGB:
#if 1
		{
			// More exact formula giving C1 continuity
			// https://en.wikipedia.org/wiki/SRGB#Theory_of_the_transformation
			const double   gamma = 2.4;
			const double   alpha = 1.055;
			const double   k0    = (alpha - 1) / (gamma - 1);
			const double   phi   =
				  (pow (alpha, gamma) * pow (gamma - 1, gamma - 1))
				/ (pow (alpha - 1, gamma - 1) * pow (gamma, gamma));
			const double   pwr   = 1.0 / gamma;
			// IEC 61966-2-1, annex G allows values out of [0 ; 1], but specifies
			// the range only in the coded space (-384/510 inclusive to 640/510
			// exclusive). The equivalent linear range falls within -1 and 2.
			ptr = OpSPtr (new TransOpLinPow (
				inv_flag, alpha, k0 / phi, pwr, phi, -1, 2
			));
		}
#else
		// Rounded constants used in IEC 61966-2-1
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.055, 0.04045 / 12.92, 1.0 / 2.4, 12.92));
#endif
		break;
	case TransCurve_2020_12:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1.09929682680944, 0.018053968510807, 0.45, 4.5));
		break;
	case TransCurve_2084:
		ptr = OpSPtr (new TransOp2084 (inv_flag));
		break;
	case TransCurve_428:
		ptr = OpSPtr (new TransOpPow (inv_flag, 2.6, 48.0 / 52.37));
		break;
	case TransCurve_HLG:
		ptr = OpSPtr (new TransOpHlg (inv_flag));
		break;
	case TransCurve_1886:
		ptr = OpSPtr (new TransOpPow (inv_flag, 2.4));
		break;
	case TransCurve_1886A:
		{
			const double   a1    = 2.6;
			const double   a2    = 3.0;
			const double   k0    = 0.35;
			const double   slope = pow (k0, a2 - a1);
			const double   beta  = pow (k0, a1);
			ptr = OpSPtr (new TransOpLinPow (
				inv_flag, 1, beta, 1.0 / a1, slope, 0, 1, 1, 1.0 / a2
			));
		}
		break;
	case TransCurve_FILMSTREAM:
		ptr = OpSPtr (new TransOpFilmStream (inv_flag));
		break;
	case TransCurve_SLOG:
		ptr = OpSPtr (new TransOpSLog (inv_flag, false));
		break;
	case TransCurve_LOGC2:
		ptr = OpSPtr (new TransOpLogC (
			inv_flag, TransOpLogC::Type_LOGC_V2, logc_ei
		));
		break;
	case TransCurve_LOGC3:
		ptr = OpSPtr (new TransOpLogC (
			inv_flag, TransOpLogC::Type_LOGC_V3, logc_ei
		));
		break;
	case TransCurve_CANONLOG:
		ptr = OpSPtr (new TransOpCanonLog (inv_flag));
		break;
	case TransCurve_ADOBE_RGB:
		ptr = OpSPtr (new TransOpPow (inv_flag, 563.0 / 256));
		break;
	case TransCurve_ROMM_RGB:
		ptr = OpSPtr (new TransOpLinPow (inv_flag, 1, 0.001953, 1.0 / 1.8, 16));
		break;
	case TransCurve_ACESCC:
		ptr = OpSPtr (new TransOpAcesCc (inv_flag));
		break;
	case TransCurve_ERIMM:
		ptr = OpSPtr (new TransOpErimm (inv_flag));
		break;
	case TransCurve_SLOG2:
		ptr = OpSPtr (new TransOpSLog (inv_flag, true));
		break;
	case TransCurve_SLOG3:
		ptr = OpSPtr (new TransOpSLog3 (inv_flag));
		break;
	case TransCurve_VLOG:
		ptr = OpSPtr (new TransOpLogC (inv_flag, TransOpLogC::Type_VLOG));
		break;
	default:
		assert (false);
		break;
	}

	if (ptr.get () == 0)
	{
		ptr = OpSPtr (new TransOpBypass);
	}

	return ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
