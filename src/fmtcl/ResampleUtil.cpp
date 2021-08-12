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
#include "fstb/fnc.h"

#include <array>

#include <cassert>
#include <cctype>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	ResampleUtil::_max_nbr_taps;



ChromaPlacement	ResampleUtil::conv_str_to_chroma_placement (std::string cplace)
{
	ChromaPlacement   cp_val = ChromaPlacement_INVALID;

	fstb::conv_to_lower_case (cplace);
	if (cplace == "mpeg1")
	{
		cp_val = ChromaPlacement_MPEG1;
	}
	else if (cplace == "mpeg2")
	{
		cp_val = ChromaPlacement_MPEG2;
	}
	else if (cplace == "dv")
	{
		cp_val = ChromaPlacement_DV;
	}

	return cp_val;
}



int	ResampleUtil::conv_str_to_chroma_subspl (int &ssh, int &ssv, std::string css)
{
	assert (! css.empty ());

	int            ret_val = 0;

	fstb::conv_to_lower_case (css);

	if (     css == "444" || css == "4:4:4")
	{
		ssh = 0;
		ssv = 0;
	}
	else if (css == "422" || css == "4:2:2")
	{
		ssh = 1;
		ssv = 0;
	}
	else if (css == "420" || css == "4:2:0")
	{
		ssh = 1;
		ssv = 1;
	}
	else if (css == "411" || css == "4:1:1")
	{
		ssh = 2;
		ssv = 0;
	}
	else if (css.length () == 2 && isdigit (css [0]) && isdigit (css [1]))
	{
		const int      ssh2  = css [0] - '0';
		const int      ssv2  = css [1] - '0';
		constexpr int  nbr_d = 10;
		constexpr std::array <int, nbr_d> log2table {
			-1, 0, 1, -1, 2, -1, -1, -1, 3, -1
		};
		if (   ssh2 < 0 || ssh2 >= nbr_d
		    || ssv2 < 0 || ssv2 >= nbr_d)
		{
			ret_val = -2;
		}
		else
		{
			ssh = log2table [ssh2];
			ssv = log2table [ssv2];
			if (ssh < 0 || ssv < 0)
			{
				ret_val = -3;
			}
		}
	}
	else
	{
		ret_val = -1;
	}

	return ret_val;
}



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

	// We don't use src_ss_* because they are only valid for the chroma planes.
	// The formulas belows are valid for any plane.
	const double   subspl_h = double (src_w / spec._src_width );
	const double   subspl_v = double (src_h / spec._src_height);

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



void	ResampleUtil::get_interlacing_param (bool &itl_flag, bool &top_flag, int field_index, InterlacingParam interlaced, FieldOrder field_order, FieldBased prop_fieldbased, Field prop_field, bool old_behaviour_flag)
{
	assert (interlaced >= 0);
	assert (interlaced < InterlacingParam_NBR_ELT);
	assert (field_order >= 0);
	assert (field_order < FieldOrder_NBR_ELT);

	// Default: assumes interlaced only if explicitely specified.
	itl_flag = (interlaced == InterlacingParam_FIELDS);
	top_flag = true;

	// Fields specified or automatic
	if (interlaced != InterlacingParam_FRAMES)
	{
		if (prop_fieldbased >= 0)
		{
			itl_flag = (itl_flag || prop_fieldbased != FieldBased_FRAMES);
		}

		// Now finds the field order. First check explicit parameters.
		if (field_order == FieldOrder_BFF)
		{
			top_flag = ((field_index & 1) != 0);
		}
		else if (field_order == FieldOrder_TFF)
		{
			top_flag = ((field_index & 1) == 0);
		}

		// Else, assumes auto-detection. If it cannot be detected,
		// assumes simple frames.
		else if (prop_fieldbased < 0 && prop_field < 0)
		{
			itl_flag = false;
		}
		else if (itl_flag)
		{
			if (prop_field >= 0)
			{
				top_flag = (prop_field != Field_BOT);
			}
			else if (   ! old_behaviour_flag
			         && (   prop_fieldbased == FieldBased_BFF
			             || prop_fieldbased == FieldBased_TFF))
			{
				top_flag =
				   (((field_index & 1) == 0) == (prop_fieldbased == FieldBased_TFF));
			}
			else
			{
				// Or should we emit an error?
				itl_flag = false;
			}
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
