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

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



bool	ResampleSpecPlane::operator < (const ResampleSpecPlane &other) const
{
	if (_src_width        < other._src_width       ) { return (true ); }
	if (_src_width        > other._src_width       ) { return (false); }

	if (_src_height       < other._src_height      ) { return (true ); }
	if (_src_height       > other._src_height      ) { return (false); }

	if (_dst_width        < other._dst_width       ) { return (true ); }
	if (_dst_width        > other._dst_width       ) { return (false); }

	if (_dst_height       < other._dst_height      ) { return (true ); }
	if (_dst_height       > other._dst_height      ) { return (false); }

	if (_win_x            < other._win_x           ) { return (true ); }
	if (_win_x            > other._win_x           ) { return (false); }

	if (_win_y            < other._win_y           ) { return (true ); }
	if (_win_y            > other._win_y           ) { return (false); }

	if (_win_w            < other._win_w           ) { return (true ); }
	if (_win_w            > other._win_w           ) { return (false); }

	if (_win_h            < other._win_h           ) { return (true ); }
	if (_win_h            > other._win_h           ) { return (false); }

	if (_center_pos_src_h < other._center_pos_src_h) { return (true ); }
	if (_center_pos_src_h > other._center_pos_src_h) { return (false); }

	if (_center_pos_src_v < other._center_pos_src_v) { return (true ); }
	if (_center_pos_src_v > other._center_pos_src_v) { return (false); }

	if (_center_pos_dst_h < other._center_pos_dst_h) { return (true ); }
	if (_center_pos_dst_h > other._center_pos_dst_h) { return (false); }

	if (_center_pos_dst_v < other._center_pos_dst_v) { return (true ); }
	if (_center_pos_dst_v > other._center_pos_dst_v) { return (false); }

	if (_kernel_scale_h   < other._kernel_scale_h  ) { return (true ); }
	if (_kernel_scale_h   > other._kernel_scale_h  ) { return (false); }

	if (_kernel_scale_v   < other._kernel_scale_v  ) { return (true ); }
	if (_kernel_scale_v   > other._kernel_scale_v  ) { return (false); }

	if (_add_cst          < other._add_cst         ) { return (true ); }

	if (_kernel_hash_h    < other._kernel_hash_h   ) { return (true ); }
	if (_kernel_hash_h    > other._kernel_hash_h   ) { return (false); }

	if (_kernel_hash_v    < other._kernel_hash_v    ) { return (true ); }
	if (_kernel_hash_v    > other._kernel_hash_v    ) { return (false); }

	return (false);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
