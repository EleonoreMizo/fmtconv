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

#include "vswrap.h"

#include <stdexcept>

#include <cassert>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Does not increase the reference count.
template <typename T, typename FW>
ObjRefSPtr <T, FW>::ObjRefSPtr (T *ptr, const ::VSAPI &vsapi)
:	_obj_ptr (ptr)
,	_vsapi_ptr (&vsapi)
{
	assert (_obj_ptr == nullptr || _vsapi_ptr != nullptr);
}



template <typename T, typename FW>
ObjRefSPtr <T, FW>::ObjRefSPtr (const ObjRefSPtr <T, FW> &other)
:	_obj_ptr (nullptr)
,	_vsapi_ptr (other._vsapi_ptr)
{
	if (other._obj_ptr != nullptr)
	{
		_obj_ptr = FW::clone (*_vsapi_ptr, other._obj_ptr);
		if (_obj_ptr == nullptr)
		{
			throw std::runtime_error ("Cannot clone VS object reference.");
		}
	}
}



template <typename T, typename FW>
ObjRefSPtr <T, FW>::ObjRefSPtr (ObjRefSPtr <T, FW> &&other)
:	_obj_ptr (other._obj_ptr)
,	_vsapi_ptr (other._vsapi_ptr)
{
	other._obj_ptr = nullptr;
}



template <typename T, typename FW>
ObjRefSPtr <T, FW>::~ObjRefSPtr ()
{
	release_resource ();
}



template <typename T, typename FW>
ObjRefSPtr <T, FW> &	ObjRefSPtr <T, FW>::operator = (const ObjRefSPtr <T, FW> &other)
{
	if (other._obj_ptr != _obj_ptr)
	{
		T *            tmp_ptr = nullptr;

		if (other._obj_ptr != nullptr)
		{
			if (_vsapi_ptr == nullptr)
			{
				assert (other._vsapi_ptr != nullptr);
				_vsapi_ptr = other._vsapi_ptr;
			}

			tmp_ptr = FW::clone (*_vsapi_ptr, other._obj_ptr);
			if (tmp_ptr == nullptr)
			{
				throw std::runtime_error ("Cannot clone VS object reference.");
			}
		}

		release_resource ();

		_obj_ptr = tmp_ptr;
	}

	return *this;
}



template <typename T, typename FW>
ObjRefSPtr <T, FW> &	ObjRefSPtr <T, FW>::operator = (ObjRefSPtr <T, FW> &&other)
{
	if (other._obj_ptr != _obj_ptr)
	{
		_obj_ptr   = other._obj_ptr;
		_vsapi_ptr = other._vsapi_ptr;
		other._obj_ptr = nullptr;
	}

	return *this;
}



template <typename T, typename FW>
T *	ObjRefSPtr <T, FW>::operator -> () const
{
	return _obj_ptr;
}



template <typename T, typename FW>
T &	ObjRefSPtr <T, FW>::operator * () const
{
	return *_obj_ptr;
}



template <typename T, typename FW>
T *	ObjRefSPtr <T, FW>::get () const
{
	return _obj_ptr;
}



template <typename T, typename FW>
T *	ObjRefSPtr <T, FW>::dup () const
{
	assert (_obj_ptr != nullptr);
	assert (_vsapi_ptr != nullptr);

	T *            tmp_ptr = FW::clone (*_vsapi_ptr, _obj_ptr);

	return tmp_ptr;
}



template <typename T, typename FW>
void	ObjRefSPtr <T, FW>::clear ()
{
	release_resource ();
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <typename T, typename FW>
void	ObjRefSPtr <T, FW>::release_resource ()
{
	if (_obj_ptr != nullptr)
	{
		assert (_vsapi_ptr != nullptr);
		FW::free (*_vsapi_ptr, _obj_ptr);
		_obj_ptr = nullptr;
	}
}



}	// namespace vsutl



#endif	// vsutl_ObjRefSPtr_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
