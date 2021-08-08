/*****************************************************************************

        PlaneProcMode.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (avsutl_PlaneProcMode_HEADER_INCLUDED)
#define avsutl_PlaneProcMode_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace avsutl
{



enum PlaneProcMode
{
	PlaneProcMode_FILL = 0,
	PlaneProcMode_GARBAGE,
	PlaneProcMode_COPY1,
	PlaneProcMode_PROCESS,
	PlaneProcMode_COPY2,
	PlaneProcMode_COPY3,

	PlaneProcMode_NBR_ELT

};	// enum PlaneProcMode



}  // namespace avsutl



//#include "avsutl/PlaneProcMode.hpp"



#endif   // avsutl_PlaneProcMode_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
