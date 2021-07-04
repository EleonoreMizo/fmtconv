/*****************************************************************************

        FrameRefSPtr.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_FrameRefSPtr_HEADER_INCLUDED)
#define	vsutl_FrameRefSPtr_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vsutl/ObjRefSPtr.h"
#include "VapourSynth.h"



namespace vsutl
{



class FrameRefSPtr_FncWrapper
{
public:
	static inline const ::VSFrameRef * clone (const ::VSAPI &vsapi, const ::VSFrameRef *f) VS_NOEXCEPT
	{
		return (*vsapi.cloneFrameRef) (f);
	}
	static inline void free (const ::VSAPI &vsapi, const ::VSFrameRef *f) VS_NOEXCEPT
	{
		(*vsapi.freeFrame) (f);
	}
};

typedef	ObjRefSPtr <
	const ::VSFrameRef,
	FrameRefSPtr_FncWrapper
>	FrameRefSPtr;



}	// namespace vsutl



//#include "vsutl/FrameRefSPtr.hpp"



#endif	// vsutl_FrameRefSPtr_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
