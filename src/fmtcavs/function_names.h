/*****************************************************************************

        function_names.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_function_names_HEADER_INCLUDED)
#define fmtcavs_function_names_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcavs
{



#define fmtcavs_NAMESPACE    "fmtc"

#define fmtcavs_BUILD_NAME(x) fmtcavs_NAMESPACE "_" x

#define fmtcavs_BITDEPTH     fmtcavs_BUILD_NAME ("bitdepth")
#define fmtcavs_MATRIX       fmtcavs_BUILD_NAME ("matrix")
#define fmtcavs_MATRIX2020CL fmtcavs_BUILD_NAME ("matrix2020cl")
#define fmtcavs_PRIMARIES    fmtcavs_BUILD_NAME ("primaries")
#define fmtcavs_RESAMPLE     fmtcavs_BUILD_NAME ("resample")
#define fmtcavs_TRANSFER     fmtcavs_BUILD_NAME ("transfer")



}  // namespace fmtcavs



//#include "fmtcavs/function_names.hpp"



#endif   // fmtcavs_function_names_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
