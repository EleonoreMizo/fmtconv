/*****************************************************************************

        ChromaPlacement.cpp
        Author: Laurent de Soras, 2015

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

#include "fmtcl/ChromaPlacement.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*
ss_h and ss_v are log2(subsampling)
rgb_flag actually means that chroma subsampling doesn't apply.

http://www.mir.com/DMG/chroma.html

cp_* is the position of the sampling point relative to the frame
top/left border, in the plane coordinates. For reference, the border
of the frame is at 0.5 units of luma from the first luma sampling point.
I. e., the luma sampling point is at the pixel's center.
*/

void	ChromaPlacement_compute_cplace (double &cp_h, double &cp_v, ChromaPlacement cplace, int plane_index, int ss_h, int ss_v, bool rgb_flag, bool interlaced_flag, bool top_flag)
{
	assert (&cp_h != 0);
	assert (&cp_v != 0);
	assert (cplace >= 0);
	assert (cplace < ChromaPlacement_NBR_ELT);
	assert (ss_h >= 0);
	assert (ss_v >= 0);
	assert (plane_index >= 0);

	// Generic case for luma, non-subsampled chroma and MPEG-1 chroma.
	cp_h = 0.5;
	cp_v = 0.5;

	if (interlaced_flag)
	{
		cp_v *= 0.5;
		if (! top_flag)
		{
			cp_v += 0.5;
		}
	}

	// Subsampled chroma
	if (! rgb_flag && plane_index > 0)
	{
		if (ss_h > 0)
		{
			if (   cplace == ChromaPlacement_MPEG2
			    || cplace == ChromaPlacement_DV)
			{
				cp_h = 0.5 / (1 << ss_h);
			}
		}

		if (ss_v == 1)
		{
			if (cplace == ChromaPlacement_MPEG2)
			{
				if (interlaced_flag)
				{
					cp_v = 0.25;
					if (! top_flag)
					{
						cp_v = 0.75;
					}
				}
				else
				{
					cp_v = 0.5;
				}
			}
			else if (cplace == ChromaPlacement_DV)
			{
				cp_v = 0.25;

				if (interlaced_flag)
				{
					cp_v *= 0.5;
					if (! top_flag)
					{
						cp_v += 0.25;
					}
				}

				if (plane_index == 2)
				{
					cp_v += 0.5;
				}
			}
		}  // ss_v == 1
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
