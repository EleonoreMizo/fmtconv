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

#include <stdexcept>

#include <cassert>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void VS_CC	Redirect <T>::create (const ::VSMap *in, ::VSMap *out, void *userData, ::VSCore *core, const ::VSAPI *vsapi)
{
	assert (in != 0);
	assert (out != 0);
	assert (core != 0);
	assert (vsapi != 0);

	T *            plugin_ptr = 0;
	try
	{
		plugin_ptr = new T (*in, *out, userData, *core, *vsapi);
	}
	catch (const std::exception &e)
	{
		if (vsapi->getError (out) == 0)
		{
			vsapi->setError (out, e.what ());
		}
	}
	catch (...)
	{
		if (vsapi->getError (out) == 0)
		{
			vsapi->setError (out, "Exception");
		}
	}

	if (plugin_ptr != 0)
	{
		vsapi->createFilter (
			in,
			out,
			plugin_ptr->use_filter_name ().c_str (),
			&init_filter,
			&get_frame,
			&free_filter,
			plugin_ptr->get_filter_mode (),
			plugin_ptr->get_filter_flags (),
			plugin_ptr,
			core
		);
	}

	if (vsapi->getError (out) != 0)
	{
		delete plugin_ptr;
		plugin_ptr = 0;
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void VS_CC	Redirect <T>::init_filter (::VSMap *in, ::VSMap *out, void **instanceData, ::VSNode *node, ::VSCore *core, const ::VSAPI *vsapi)
{
	fstb::unused (vsapi);
	assert (in != 0);
	assert (out != 0);
	assert (instanceData != 0);
	assert (*instanceData != 0);
	assert (node != 0);
	assert (core != 0);
	assert (vsapi != 0);

	T *         plugin_ptr = reinterpret_cast <T *> (*instanceData);
	plugin_ptr->init_filter (*in, *out, *node, *core);
}



template <class T>
const ::VSFrameRef * VS_CC	Redirect <T>::get_frame (int n, int activationReason, void **instanceData, void **frameData, ::VSFrameContext *frameCtx, ::VSCore *core, const ::VSAPI *vsapi)
{
	fstb::unused (vsapi);
	assert (n >= 0);
	assert (instanceData != 0);
	assert (*instanceData != 0);
	assert (frameData != 0);
	assert (frameCtx != 0);
	assert (core != 0);
	assert (vsapi != 0);

	T *         plugin_ptr = reinterpret_cast <T *> (*instanceData);
	const ::VSFrameRef *	frame_ref_ptr = plugin_ptr->get_frame (
		n,
		activationReason,
		*frameData,
		*frameCtx,
		*core
	);

	return (frame_ref_ptr);
}



template <class T>
void VS_CC	Redirect <T>::free_filter (void *instanceData, ::VSCore *core, const ::VSAPI *vsapi)
{
	fstb::unused (core, vsapi);
	assert (instanceData != 0);
	assert (core != 0);
	assert (vsapi != 0);

	T *         plugin_ptr = reinterpret_cast <T *> (instanceData);

	delete plugin_ptr;
	plugin_ptr = 0;
}



}	// namespace vsutl



#endif	// vsutl_Redirect_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
