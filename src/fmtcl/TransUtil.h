/*****************************************************************************

        TransUtil.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransUtil_HEADER_INCLUDED)
#define fmtcl_TransUtil_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/PicFmt.h"
#include "fmtcl/TransCurve.h"
#include "fmtcl/TransLut.h"
#include "fmtcl/TransOpInterface.h"
#include "fmtcl/TransOpLogC.h"

#include <memory>
#include <string>



namespace fmtcl
{



class TransUtil
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef  std::shared_ptr <TransOpInterface> OpSPtr;

	static std::unique_ptr <TransLut>
	               build_lut (PicFmt dst_fmt, TransCurve curve_d, TransOpLogC::ExpIdx logc_ei_d, PicFmt src_fmt, TransCurve curve_s, TransOpLogC::ExpIdx logc_ei_s, double contrast, double gcor, double lvl_black, bool sse2_flag, bool avx2_flag);

	static TransCurve
	               conv_string_to_curve (const std::string &str);
	static OpSPtr  conv_curve_to_op (TransCurve c, bool inv_flag, TransOpLogC::ExpIdx logc_ei);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransUtil ()                               = delete;
	               TransUtil (const TransUtil &other)         = delete;
	               TransUtil (TransUtil &&other)              = delete;
	TransUtil &    operator = (const TransUtil &other)        = delete;
	TransUtil &    operator = (TransUtil &&other)             = delete;
	bool           operator == (const TransUtil &other) const = delete;
	bool           operator != (const TransUtil &other) const = delete;

}; // class TransUtil



}  // namespace fmtcl



//#include "fmtcl/TransUtil.hpp"



#endif   // fmtcl_TransUtil_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
