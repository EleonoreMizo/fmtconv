/*****************************************************************************

        FuncRefSPtr.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_FuncRefSPtr_HEADER_INCLUDED)
#define	vsutl_FuncRefSPtr_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vsutl/ObjRefSPtr.h"
#include "VapourSynth4.h"



namespace vsutl
{



class FuncRefSPtr_FncWrapper
{
public:
	static inline ::VSFunction * clone (const ::VSAPI &vsapi, ::VSFunction *func) VS_NOEXCEPT
	{
		return (*vsapi.addFunctionRef) (func);
	}
	static inline void free (const ::VSAPI &vsapi, ::VSFunction *func) VS_NOEXCEPT
	{
		(*vsapi.freeFunc) (func);
	}
};

typedef	ObjRefSPtr <
	::VSFunction,
	FuncRefSPtr_FncWrapper
>	FuncRefSPtr;



}	// namespace vsutl



//#include	"vsutl/FuncRefSPtr.hpp"



#endif	// vsutl_FuncRefSPtr_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
