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

	static constexpr int _max_nbr_taps = 128;

	class FrameInfo
	{
	public:
		bool           _itl_s_flag = false;
		bool           _top_s_flag = false;
		bool           _itl_d_flag = false;
		bool           _top_d_flag = false;
	};

	enum InterlacingParam
	{
		InterlacingParam_INVALID = -1,

		InterlacingParam_FRAMES = 0,
		InterlacingParam_FIELDS,
		InterlacingParam_AUTO,

		InterlacingParam_NBR_ELT
	};

	enum FieldOrder
	{
		FieldOrder_INVALID = -1,

		FieldOrder_BFF = 0,
		FieldOrder_TFF,
		FieldOrder_AUTO,

		FieldOrder_NBR_ELT
	};

	// Frame properties, common to Vapoursynth and Avisynth+

	enum FieldBased
	{
		FieldBased_INVALID = -1,

		FieldBased_FRAMES = 0,
		FieldBased_BFF,
		FieldBased_TFF,

		FieldBased_NBR_ELT
	};

	enum Field
	{
		Field_INVALID = -1,

		Field_BOT = 0,
		Field_TOP,

		Field_NBR_ELT
	};

	static ChromaPlacement
	               conv_str_to_chroma_placement (std::string cplace);
	static int     conv_str_to_chroma_subspl (int &ssh, int &ssv, std::string css);
	static void    create_plane_specs (ResamplePlaneData &plane_data, int plane_index, ColorFamily src_cf, int src_w, int src_ss_h, int src_h, int src_ss_v, ChromaPlacement cplace_s, ColorFamily dst_cf, int dst_w, int dst_ss_h, int dst_h, int dst_ss_v, ChromaPlacement cplace_d);
	static void    get_interlacing_param (bool &itl_flag, bool &top_flag, int field_index, InterlacingParam interlaced, FieldOrder field_order, FieldBased prop_fieldbased, Field prop_field, bool old_behaviour_flag);



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
