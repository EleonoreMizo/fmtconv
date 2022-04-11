/*****************************************************************************

        Redirect.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_Redirect_CODEHEADER_INCLUDED)
#define	vsutl_Redirect_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include <memory>
#include <stdexcept>

#include <cassert>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void VS_CC	Redirect <T>::create (const ::VSMap *in, ::VSMap *out, void *userData, ::VSCore *core, const ::VSAPI *vsapi)
{
	assert (in != nullptr);
	assert (out != nullptr);
	assert (core != nullptr);
	assert (vsapi != nullptr);

	std::unique_ptr <T>  plugin_uptr;

	try
	{
		plugin_uptr = std::make_unique <T> (*in, *out, userData, *core, *vsapi);
		const auto     vi   = plugin_uptr->get_video_info ();
		const auto     mode = plugin_uptr->get_filter_mode ();
		const auto     deps = plugin_uptr->get_dependencies ();

		vsapi->createVideoFilter (
			out,
			plugin_uptr->use_filter_name ().c_str (),
			&vi,
			&get_frame,
			&free_filter,
			mode,
			deps.data (),
			int (deps.size ()),
			plugin_uptr.get (),
			core
		);
	}
	catch (const std::exception &e)
	{
		if (vsapi->mapGetError (out) == nullptr)
		{
			vsapi->mapSetError (out, e.what ());
		}
	}
	catch (...)
	{
		if (vsapi->mapGetError (out) == nullptr)
		{
			vsapi->mapSetError (out, "Exception");
		}
	}

	// If finally there isn't any error, we release the unique_ptr ownership
	// so the object can live out of the function scope.
	if (vsapi->mapGetError (out) == nullptr)
	{
		plugin_uptr.release ();
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
const ::VSFrame * VS_CC	Redirect <T>::get_frame (int n, int activationReason, void *instanceData, void **frameData, ::VSFrameContext *frameCtx, ::VSCore *core, const ::VSAPI *vsapi)
{
	fstb::unused (vsapi);
	assert (n >= 0);
	assert (instanceData != nullptr);
	assert (frameData    != nullptr);
	assert (frameCtx     != nullptr);
	assert (core         != nullptr);
	assert (vsapi        != nullptr);

	T *         plugin_ptr = reinterpret_cast <T *> (instanceData);
	const ::VSFrame *	frame_ptr = plugin_ptr->get_frame (
		n,
		activationReason,
		*frameData,
		*frameCtx,
		*core
	);

	return frame_ptr;
}



template <class T>
void VS_CC	Redirect <T>::free_filter (void *instanceData, ::VSCore *core, const ::VSAPI *vsapi)
{
	fstb::unused (core, vsapi);
	assert (instanceData != nullptr);
	assert (core != nullptr);
	assert (vsapi != nullptr);

	T *         plugin_ptr = reinterpret_cast <T *> (instanceData);

	delete plugin_ptr;
	plugin_ptr = nullptr;
}



}	// namespace vsutl



#endif	// vsutl_Redirect_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
