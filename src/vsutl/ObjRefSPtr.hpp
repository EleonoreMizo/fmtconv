/*****************************************************************************

        ObjRefSPtr.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_ObjRefSPtr_CODEHEADER_INCLUDED)
#define	vsutl_ObjRefSPtr_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "VapourSynth.h"

#include <stdexcept>

#include <cassert>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Does not increase the reference count.
template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
ObjRefSPtr <T, FC, FF>::ObjRefSPtr (T *ptr, const ::VSAPI &vsapi)
:	_obj_ptr (ptr)
,	_vsapi_ptr (&vsapi)
{
	assert (_obj_ptr == 0 || _vsapi_ptr != 0);
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
ObjRefSPtr <T, FC, FF>::ObjRefSPtr (const ObjRefSPtr <T, FC, FF> &other)
:	_obj_ptr (0)
,	_vsapi_ptr (other._vsapi_ptr)
{
	if (other._obj_ptr != 0)
	{
		_obj_ptr = (_vsapi_ptr->*FC) (other._obj_ptr);
		if (_obj_ptr == 0)
		{
			throw std::runtime_error ("Cannot clone VS object reference.");
		}
	}
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
ObjRefSPtr <T, FC, FF>::~ObjRefSPtr ()
{
	release_resource ();
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
ObjRefSPtr <T, FC, FF> &	ObjRefSPtr <T, FC, FF>::operator = (const ObjRefSPtr <T, FC, FF> &other)
{
	if (other._obj_ptr != _obj_ptr)
	{
		T *            tmp_ptr = 0;

		if (other._obj_ptr != 0)
		{
			if (_vsapi_ptr == 0)
			{
				assert (other._vsapi_ptr != 0);
				_vsapi_ptr = other._vsapi_ptr;
			}

			tmp_ptr = (_vsapi_ptr->*FC) (other._obj_ptr);
			if (tmp_ptr == 0)
			{
				throw std::runtime_error ("Cannot clone VS object reference.");
			}
		}

		release_resource ();

		_obj_ptr = tmp_ptr;
	}

	return (*this);
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
T *	ObjRefSPtr <T, FC, FF>::operator -> () const
{
	return (_obj_ptr);
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
T &	ObjRefSPtr <T, FC, FF>::operator * () const
{
	return (*_obj_ptr);
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
T *	ObjRefSPtr <T, FC, FF>::get () const
{
	return (_obj_ptr);
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
T *	ObjRefSPtr <T, FC, FF>::dup () const
{
	assert (_obj_ptr != 0);
	assert (_vsapi_ptr != 0);

	T *            tmp_ptr = (_vsapi_ptr->*FC) (_obj_ptr);

	return (tmp_ptr);
}



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
void	ObjRefSPtr <T, FC, FF>::clear ()
{
	release_resource ();
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T, T * (VS_CC *::VSAPI::*FC) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT, void (VS_CC *::VSAPI::*FF) (T *) vsutl_ObjRefSPtr_VS_NOEXCEPT>
void	ObjRefSPtr <T, FC, FF>::release_resource ()
{
	if (_obj_ptr != 0)
	{
		assert (_vsapi_ptr != 0);
		(_vsapi_ptr->*FF) (_obj_ptr);
		_obj_ptr = 0;
	}
}



}	// namespace vsutl



#endif	// vsutl_ObjRefSPtr_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
