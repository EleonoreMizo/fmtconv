/*****************************************************************************

        fnc_avsutl.cpp
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

#include "avsutl/fnc.h"
#include "avsutl/TFlag.h"
#include "avisynth.h"

#include <cassert>



namespace avsutl
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



TFlag	set_tristate (const ::AVSValue &val)
{
	assert (val.IsBool ());

	return (
		  (! val.Defined ()) ? avsutl::TFlag::U
		: val.AsBool ()      ? avsutl::TFlag::T
		:                      avsutl::TFlag::F
	);
}



bool set_default (TFlag tristate, bool def_flag)
{
	return 
	     (tristate == avsutl::TFlag::T) ? 1
		: (tristate == avsutl::TFlag::F) ? 0
	   :                                  def_flag;
}



bool has_alpha (const ::VideoInfo &vi)
{
	return (
		   vi.IsRGB32 () || vi.IsRGB64 ()
		|| vi.IsPlanarRGBA () || vi.IsYUVA ()
	);
}



int get_nbr_comp_non_alpha (const ::VideoInfo &vi)
{
	const int      nbr_alpha = has_alpha (vi) ? 1 : 0;
	const int      nbr_comp  = vi.NumComponents ();

	return nbr_comp - nbr_alpha;
}


bool is_rgb (const ::VideoInfo &vi)
{
	return (
		   vi.IsRGB () || vi.IsRGB48 () || vi.IsRGB64 ()
		|| vi.IsPlanarRGB () || vi.IsPlanarRGBA ()
	);
}



bool is_full_range_default (const ::VideoInfo &vi)
{
	return is_rgb (vi);
}



}  // namespace avsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
