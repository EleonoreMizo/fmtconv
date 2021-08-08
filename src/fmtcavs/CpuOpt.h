/*****************************************************************************

        CpuOpt.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_CpuOpt_HEADER_INCLUDED)
#define fmtcavs_CpuOpt_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/CpuOptBase.h"



class AVSValue;

namespace fmtcavs
{



class CpuOpt
:	public fmtcl::CpuOptBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       CpuOpt (const ::AVSValue &arg);



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

}; // class CpuOpt



}  // namespace fmtcavs



//#include "fmtcavs/CpuOpt.hpp"



#endif   // fmtcavs_CpuOpt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
