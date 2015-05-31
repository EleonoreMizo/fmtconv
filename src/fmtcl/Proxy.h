/*****************************************************************************

        Proxy.h
        Author: Laurent de Soras, 2012

		  Abstraction for pointers.
		  Handles Stack16 format as a single pointer.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include <cstddef>
#include <cstdint>



namespace fmtcl
{



class Proxy
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	template <class T>
	class Ptr1
	{
	public:
		typedef  T           DataType;
		typedef  T *         Type;
		static fstb_FORCEINLINE Type
		               make_ptr (const uint8_t *ptr, int /*stride_bytes*/, int /*h*/);
		static fstb_FORCEINLINE void
		               jump (Type &ptr, int stride);
		static fstb_FORCEINLINE bool
		               check_ptr (const Type &ptr, int align = 1);
		static fstb_FORCEINLINE void
		               copy (const Type &dst_ptr, const typename Ptr1 <const T>::Type &src_ptr, size_t nbr_elt);
	};

	template <class T>
	class Ptr2
	{
	public:
		typedef	T     DataType;
		class Type
		{
		public:
			explicit       Type (T *msb_ptr, T *lsb_ptr);
			T *            _msb_ptr;
			T *            _lsb_ptr;
		};
		static fstb_FORCEINLINE Type
		               make_ptr (const uint8_t *ptr, int stride_bytes, int h);
		static fstb_FORCEINLINE void
		               jump (Type &ptr, int stride);
		static fstb_FORCEINLINE bool
		               check_ptr (const Type &ptr, int align = 1);
		static fstb_FORCEINLINE void
		               copy (const Type &dst_ptr, const typename Ptr2 <const T>::Type &src_ptr, size_t nbr_elt);
	};

	typedef	Ptr1 <float>            PtrFloat;
	typedef	Ptr1 <const float>      PtrFloatConst;
	typedef	Ptr1 <uint16_t>         PtrInt16;
	typedef	Ptr1 <const uint16_t>   PtrInt16Const;
	typedef	Ptr2 <uint8_t>          PtrStack16;
	typedef	Ptr2 <const uint8_t>    PtrStack16Const;
	typedef	Ptr1 <uint8_t>          PtrInt8;
	typedef	Ptr1 <const uint8_t>    PtrInt8Const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Proxy ();
	               Proxy (const Proxy &other);
	virtual        ~Proxy () {}
	Proxy &        operator = (const Proxy &other);
	bool           operator == (const Proxy &other) const;
	bool           operator != (const Proxy &other) const;

};	// class Proxy



}	// namespace fmtcl



#include "fmtcl/Proxy.hpp"



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
