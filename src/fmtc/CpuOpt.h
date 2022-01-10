/*****************************************************************************

        CpuOpt.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtc_CpuOpt_HEADER_INCLUDED)
#define	fmtc_CpuOpt_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/CpuOptBase.h"

#include "VapourSynth4.h"



namespace vsutl
{
	class FilterBase;
}

namespace fmtc
{



class CpuOpt
:	public fmtcl::CpuOptBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       CpuOpt (vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *param_name_0 = "cpuopt");



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               CpuOpt ()                               = delete;
	               CpuOpt (const CpuOpt &other)            = delete;
	               CpuOpt (CpuOpt &&other)                 = delete;
	CpuOpt &       operator = (const CpuOpt &other)        = delete;
	CpuOpt &       operator = (CpuOpt &&other)             = delete;
	bool           operator == (const CpuOpt &other) const = delete;
	bool           operator != (const CpuOpt &other) const = delete;

};	// class CpuOpt



}	// namespace fmtc



//#include "fmtc/CpuOpt.hpp"



#endif	// fmtc_CpuOpt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
