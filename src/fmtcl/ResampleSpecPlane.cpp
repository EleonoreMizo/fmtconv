/*****************************************************************************

        ResampleSpecPlane.cpp
        Author: Laurent de Soras, 2014

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ResampleSpecPlane.h"

#include <tuple>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



bool	ResampleSpecPlane::operator < (const ResampleSpecPlane &other) const
{
	return std::tie (
		_src_width,
		_src_height,
		_dst_width,
		_dst_height,
		_win_x,
		_win_y,
		_win_w,
		_win_h,
		_center_pos_src_h,
		_center_pos_src_v,
		_center_pos_dst_h,
		_center_pos_dst_v,
		_kernel_scale_h,
		_kernel_scale_v,
		_add_cst,
		_kernel_hash_h,
		_kernel_hash_v
	) < std::tie (
		other._src_width,
		other._src_height,
		other._dst_width,
		other._dst_height,
		other._win_x,
		other._win_y,
		other._win_w,
		other._win_h,
		other._center_pos_src_h,
		other._center_pos_src_v,
		other._center_pos_dst_h,
		other._center_pos_dst_v,
		other._kernel_scale_h,
		other._kernel_scale_v,
		other._add_cst,
		other._kernel_hash_h,
		other._kernel_hash_v
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
