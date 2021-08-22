/*****************************************************************************

        TransModel.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransModel_HEADER_INCLUDED)
#define fmtcl_TransModel_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Frame.h"
#include "fmtcl/FrameRO.h"
#include "fmtcl/PicFmt.h"
#include "fmtcl/TransCurve.h"
#include "fmtcl/TransLut.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransUtil.h"

#include <memory>

#include <cstdint>



namespace fmtcl
{



class TransModel
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef TransUtil::OpSPtr OpSPtr;

	explicit       TransModel (PicFmt dst_fmt, TransCurve curve_d, TransOpLogC::ExpIdx logc_ei_d, PicFmt src_fmt, TransCurve curve_s, TransOpLogC::ExpIdx logc_ei_s, double contrast, double gcor, double lvl_black, bool sse2_flag, bool avx2_flag);

	void           process_plane (const Plane <> &dst, const PlaneRO <> &src, int w, int h) const noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	std::unique_ptr <TransLut>
	               _lut_uptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransModel ()                               = delete;
	               TransModel (const TransModel &other)        = delete;
	               TransModel (TransModel &&other)             = delete;
	TransModel &   operator = (const TransModel &other)        = delete;
	TransModel &   operator = (TransModel &&other)             = delete;
	bool           operator == (const TransModel &other) const = delete;
	bool           operator != (const TransModel &other) const = delete;

}; // class TransModel



}  // namespace fmtcl



//#include "fmtcl/TransModel.hpp"



#endif   // fmtcl_TransModel_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
