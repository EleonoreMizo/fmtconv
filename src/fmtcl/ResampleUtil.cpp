/*****************************************************************************

        ResampleUtil.cpp
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

#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/FilterResize.h"
#include "fmtcl/fnc.h"
#include "fmtcl/InterlacingType.h"
#include "fmtcl/ResamplePlaneData.h"
#include "fmtcl/ResampleSpecPlane.h"
#include "fmtcl/ResampleUtil.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	ResampleUtil::create_plane_specs (ResamplePlaneData &plane_data, int plane_index, ColorFamily src_cf, int src_w, int src_ss_h, int src_h, int src_ss_v, ChromaPlacement cplace_s, ColorFamily dst_cf, int dst_w, int dst_ss_h, int dst_h, int dst_ss_v, ChromaPlacement cplace_d)
{
	assert (plane_index >= 0);

	ResampleSpecPlane spec;

	spec._src_width  =
		compute_plane_width (src_cf, src_ss_h, src_w, plane_index);
	spec._src_height =
		compute_plane_height (src_cf, src_ss_v, src_h, plane_index);
	spec._dst_width  =
		compute_plane_width (dst_cf, dst_ss_h, dst_w, plane_index);
	spec._dst_height =
		compute_plane_height (dst_cf, dst_ss_v, dst_h, plane_index);

	const double   subspl_h = double (1 << src_ss_h);
	const double   subspl_v = double (1 << src_ss_v);

	const ResamplePlaneData::Win &   s = plane_data._win;
	spec._win_x = s._x / subspl_h;
	spec._win_y = s._y / subspl_v;
	spec._win_w = s._w / subspl_h;
	spec._win_h = s._h / subspl_v;

	spec._add_cst        = plane_data._add_cst;
	spec._kernel_scale_h = plane_data._kernel_scale_h;
	spec._kernel_scale_v = plane_data._kernel_scale_v;
	spec._kernel_hash_h  = plane_data._kernel_arr [FilterResize::Dir_H].get_hash ();
	spec._kernel_hash_v  = plane_data._kernel_arr [FilterResize::Dir_V].get_hash ();

	for (int itl_d = 0; itl_d < InterlacingType_NBR_ELT; ++itl_d)
	{
		for (int itl_s = 0; itl_s < InterlacingType_NBR_ELT; ++itl_s)
		{
			double         cp_s_h = 0;
			double         cp_s_v = 0;
			double         cp_d_h = 0;
			double         cp_d_v = 0;
			if (plane_data._preserve_center_flag)
			{
				ChromaPlacement_compute_cplace (
					cp_s_h, cp_s_v, cplace_s, plane_index, src_ss_h, src_ss_v,
					(src_cf == ColorFamily_RGB),
					(itl_s  != InterlacingType_FRAME),
					(itl_s  == InterlacingType_TOP)
				);
				ChromaPlacement_compute_cplace (
					cp_d_h, cp_d_v, cplace_d, plane_index, dst_ss_h, dst_ss_v,
					(dst_cf == ColorFamily_RGB),
					(itl_d  != InterlacingType_FRAME),
					(itl_d  == InterlacingType_TOP)
				);
			}

			spec._center_pos_src_h = cp_s_h;
			spec._center_pos_src_v = cp_s_v;
			spec._center_pos_dst_h = cp_d_h;
			spec._center_pos_dst_v = cp_d_v;

			plane_data._spec_arr [itl_d] [itl_s] = spec;
		}  // for itl_s
	}  // for itl_d
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
