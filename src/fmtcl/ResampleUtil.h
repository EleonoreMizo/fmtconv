/*****************************************************************************

        ResampleUtil.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ResampleUtil_HEADER_INCLUDED)
#define fmtcl_ResampleUtil_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/ColorFamily.h"



namespace fmtcl
{



class ResamplePlaneData;

class ResampleUtil
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static void    create_plane_specs (ResamplePlaneData &plane_data, int plane_index, ColorFamily src_cf, int src_w, int src_ss_h, int src_h, int src_ss_v, ChromaPlacement cplace_s, ColorFamily dst_cf, int dst_w, int dst_ss_h, int dst_h, int dst_ss_v, ChromaPlacement cplace_d);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ResampleUtil ()                               = delete;
	               ResampleUtil (const ResampleUtil &other)      = delete;
	               ResampleUtil (ResampleUtil &&other)           = delete;
	ResampleUtil & operator = (const ResampleUtil &other)        = delete;
	ResampleUtil & operator = (ResampleUtil &&other)             = delete;
	bool           operator == (const ResampleUtil &other) const = delete;
	bool           operator != (const ResampleUtil &other) const = delete;

}; // class ResampleUtil



}  // namespace fmtcl



//#include "fmtcl/ResampleUtil.hpp"



#endif   // fmtcl_ResampleUtil_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
