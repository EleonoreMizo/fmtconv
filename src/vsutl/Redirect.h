/*****************************************************************************

        Redirect.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_Redirect_HEADER_INCLUDED)
#define	vsutl_Redirect_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vswrap.h"



namespace vsutl
{



template <class T>
class Redirect
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	// ::VSPublicFunction
	static void VS_CC
	               create (const ::VSMap *in, ::VSMap *out, void *userData, ::VSCore *core, const ::VSAPI *vsapi);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static void VS_CC
	               init_filter (::VSMap *in, ::VSMap *out, void **instanceData, ::VSNode *node, ::VSCore *core, const ::VSAPI *vsapi);
	static const ::VSFrameRef * VS_CC
	               get_frame (int n, int activationReason, void **instanceData, void **frameData, ::VSFrameContext *frameCtx, ::VSCore *core, const ::VSAPI *vsapi);
	static void VS_CC
	               free_filter (void *instanceData, ::VSCore *core, const ::VSAPI *vsapi);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Redirect ()                                   = delete;
	virtual        ~Redirect ()                                  = delete;
	               Redirect (const Redirect <T> &other)          = delete;
	Redirect <T> & operator = (const Redirect <T> &other)        = delete;
	bool           operator == (const Redirect <T> &other) const = delete;
	bool           operator != (const Redirect <T> &other) const = delete;

};	// class Redirect



}	// namespace vsutl



#include "vsutl/Redirect.hpp"



#endif	// vsutl_Redirect_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
