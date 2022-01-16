/*****************************************************************************

        NodeRefSPtr.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_NodeRefSPtr_HEADER_INCLUDED)
#define	vsutl_NodeRefSPtr_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vsutl/ObjRefSPtr.h"
#include "VapourSynth4.h"



namespace vsutl
{



class NodeRefSPtr_FncWrapper
{
public:
	static inline ::VSNode * clone (const ::VSAPI &vsapi, ::VSNode *node) VS_NOEXCEPT
	{
		return (*vsapi.addNodeRef) (node);
	}
	static inline void free (const ::VSAPI &vsapi, ::VSNode *node) VS_NOEXCEPT
	{
		(*vsapi.freeNode) (node);
	}
};

typedef	ObjRefSPtr <
	::VSNode,
	NodeRefSPtr_FncWrapper
>	NodeRefSPtr;



}	// namespace vsutl



//#include "vsutl/NodeRefSPtr.hpp"



#endif	// vsutl_NodeRefSPtr_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
