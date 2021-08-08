/*****************************************************************************

        VideoFilterBase.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (avsutl_VideoFilterBase_HEADER_INCLUDED)
#define avsutl_VideoFilterBase_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "avisynth.h"



namespace avsutl
{



class VideoFilterBase
:	public ::GenericVideoFilter
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       VideoFilterBase (::IScriptEnvironment &env, ::PClip c);

	bool           supports_props () const noexcept;
	::PVideoFrame  build_new_frame (::IScriptEnvironment &env, const ::VideoInfo &vi, ::PVideoFrame *src_ptr, int align = FRAME_ALIGN);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef ::GenericVideoFilter Inherited;

	bool           _prop_flag = false;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               VideoFilterBase ()                               = delete;
	               VideoFilterBase (const VideoFilterBase &other)   = delete;
	               VideoFilterBase (VideoFilterBase &&other)        = delete;
	VideoFilterBase &
	               operator = (const VideoFilterBase &other)        = delete;
	VideoFilterBase &
	               operator = (VideoFilterBase &&other)             = delete;
	bool           operator == (const VideoFilterBase &other) const = delete;
	bool           operator != (const VideoFilterBase &other) const = delete;

}; // class VideoFilterBase



}  // namespace avsutl



//#include "avsutl/VideoFilterBase.hpp"



#endif   // avsutl_VideoFilterBase_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
