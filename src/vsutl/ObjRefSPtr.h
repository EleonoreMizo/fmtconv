/*****************************************************************************

        ObjRefSPtr.h
        Author: Laurent de Soras, 2012

Smart pointer template class for the Vapoursynth object references.

Template parameters:

- T: The type of the object possibly with const, but without pointer
	(currently ::VSNodeRef, const ::VSFrameRef or const ::VSFuncRef).

- FW: Wrapper class for clone and free functions. Requires:
	static inline T * FW::clone (const ::VSAPI &, T *) VS_NOEXCEPT;
	static inline void FW::free (const ::VSAPI &, T *) VS_NOEXCEPT;

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



namespace vsutl
{



template <typename T, typename FW>
class ObjRefSPtr
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ObjRefSPtr () = default;
	               ObjRefSPtr (T *ptr, const ::VSAPI &vsapi);
	               ObjRefSPtr (const ObjRefSPtr <T, FW> &other);
	               ObjRefSPtr (ObjRefSPtr <T, FW> &&other);
	virtual        ~ObjRefSPtr ();

	ObjRefSPtr <T, FW> &
	               operator = (const ObjRefSPtr <T, FW> &other);
	ObjRefSPtr <T, FW> &
	               operator = (ObjRefSPtr <T, FW> &&other);

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

	T *            _obj_ptr   = nullptr;
	const ::VSAPI* _vsapi_ptr = nullptr;   // Can be 0 only if _obj_ptr is 0 too.



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const ObjRefSPtr <T, FW> &other) const;
	bool           operator != (const ObjRefSPtr <T, FW> &other) const;

};	// class ObjRefSPtr



}	// namespace vsutl



#include "vsutl/ObjRefSPtr.hpp"



#endif	// vsutl_ObjRefSPtr_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
