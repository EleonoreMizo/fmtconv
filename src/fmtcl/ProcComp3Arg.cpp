/*****************************************************************************

        ProcComp3Arg.cpp
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

#include "fmtcl/ProcComp3Arg.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	ProcComp3Arg::_nbr_planes;



template <typename T>
bool	ProcComp3Arg::Planes <T>::is_valid (bool single_plane_flag) const noexcept
{
	const int      p_end = (single_plane_flag) ? 1 : _nbr_planes;
	for (int p_idx = 0; p_idx < p_end; ++p_idx)
	{
		if (_ptr_arr [p_idx] == nullptr)
		{
			return false;
		}
	}

	return true;
}



bool	ProcComp3Arg::is_valid (bool single_plane_out_flag) const noexcept
{
	if (_w <= 0 || _h <= 0)
	{
		return false;
	}

	return (
		   _dst.is_valid (single_plane_out_flag)
		&& _src.is_valid (false)
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
