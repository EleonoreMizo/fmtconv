/*****************************************************************************

        VideoFilterBase.cpp
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

#include "avsutl/VideoFilterBase.h"

#include <cassert>



namespace avsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



VideoFilterBase::VideoFilterBase (::IScriptEnvironment &env, ::PClip c)
:	Inherited (c)
{
	try
	{
		env.CheckVersion (8);
		_prop_flag = true;
	}
	catch (const ::AvisynthError &)
	{
		_prop_flag = false;
	}
}



bool	VideoFilterBase::supports_props () const noexcept
{
	return _prop_flag;
}



::PVideoFrame	VideoFilterBase::build_new_frame (::IScriptEnvironment &env, const ::VideoInfo &vi_n, ::PVideoFrame *src_ptr, int align)
{
	if (supports_props ())
	{
		return env.NewVideoFrameP (vi_n, src_ptr, align);
	}
	else
	{
		return env.NewVideoFrame (vi_n, align);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace avsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
