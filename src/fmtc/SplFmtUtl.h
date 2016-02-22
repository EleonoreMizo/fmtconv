/*****************************************************************************

        SplFmtUtl.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_SplFmtUtl_HEADER_INCLUDED)
#define	fmtc_SplFmtUtl_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/SplFmt.h"



struct VSFormat;

namespace fmtc
{



class SplFmtUtl
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static inline fmtcl::SplFmt
	               conv_from_vsformat (const ::VSFormat &fmt);
	static inline void
	               conv_from_vsformat (fmtcl::SplFmt &type, int &bitdepth, const ::VSFormat &fmt);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               SplFmtUtl ()                               = delete;
	               SplFmtUtl (const SplFmtUtl &other)         = delete;
	virtual        ~SplFmtUtl ()                              = delete;
	SplFmtUtl &    operator = (const SplFmtUtl &other)        = delete;
	bool           operator == (const SplFmtUtl &other) const = delete;
	bool           operator != (const SplFmtUtl &other) const = delete;

};	// class SplFmtUtl



}	// namespace fmtc



#include "fmtc/SplFmtUtl.hpp"



#endif	// fmtc_SplFmtUtl_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
