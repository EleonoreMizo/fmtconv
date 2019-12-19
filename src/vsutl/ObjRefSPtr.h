/*****************************************************************************

        ObjRefSPtr.h
        Author: Laurent de Soras, 2012

Smart pointer template class for the Vapoursynth object references.

Template parameters:

- T: The type of the object possibly with const, but without pointer
	(currently ::VSNodeRef, const ::VSFrameRef or const ::VSFuncRef).
- FC: VSAPI member pointer to the function for cloning const T *.
- FF: VSAPI member pointer to the function for freeing const T *.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_ObjRefSPtr_HEADER_INCLUDED)
#define	vsutl_ObjRefSPtr_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "VapourSynth.h"



#if (__cplusplus >= 201703L)
	#define vsutl_ObjRefSPtr_VS_NOEXCEPT VS_NOEXCEPT
#else
	#define vsutl_ObjRefSPtr_VS_NOEXCEPT
#endif



namespace vsutl
{



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
class ObjRefSPtr
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ObjRefSPtr () = default;
	               ObjRefSPtr (T *ptr, const ::VSAPI &vsapi);
	               ObjRefSPtr (const ObjRefSPtr <T, FC, FF> &other);
	virtual        ~ObjRefSPtr ();

	ObjRefSPtr <T, FC, FF> &
	               operator = (const ObjRefSPtr <T, FC, FF> &other);

	T *            operator -> () const;
	T &            operator * () const;
	T *            get () const;
	T *            dup () const;
	void           clear ();



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	void           release_resource ();

	T *            _obj_ptr   = 0;
	const ::VSAPI* _vsapi_ptr = 0;      // Can be 0 only if _obj_ptr is 0 too.



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const ObjRefSPtr <T, FC, FF> &other) const;
	bool           operator != (const ObjRefSPtr <T, FC, FF> &other) const;

};	// class ObjRefSPtr



}	// namespace vsutl



#include "vsutl/ObjRefSPtr.hpp"



#endif	// vsutl_ObjRefSPtr_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
