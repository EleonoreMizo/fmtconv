/*****************************************************************************

        ArrayMultiType.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ArrayMultiType_HEADER_INCLUDED)
#define	fmtcl_ArrayMultiType_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include <vector>

#include <cstddef>
#include <cstdint>



namespace fmtcl
{



class ArrayMultiType
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ArrayMultiType ();
	virtual			~ArrayMultiType () {}

	template <class T>
	void           set_type ();
	void           resize (size_t length);
	template <class T>
	fstb_FORCEINLINE T &
		            use (int pos);
	template <class T>
	fstb_FORCEINLINE const T &
	               use (int pos) const;
	inline size_t  get_size () const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	std::vector <uint8_t>
	               _arr;
	size_t         _length;
	int            _data_len;     // Element size in bytes. 0 = not set



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ArrayMultiType (const ArrayMultiType &other);
	ArrayMultiType &
	               operator = (const ArrayMultiType &other);
	bool           operator == (const ArrayMultiType &other) const;
	bool           operator != (const ArrayMultiType &other) const;

};	// class ArrayMultiType



}	// namespace fmtcl



#include "fmtcl/ArrayMultiType.hpp"



#endif	// fmtcl_ArrayMultiType_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
