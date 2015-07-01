/*****************************************************************************

        ErrDifBuf.hpp
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ErrDifBuf_CODEHEADER_INCLUDED)
#define	fmtcl_ErrDifBuf_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cassert>
#include <cstring>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	ErrDifBuf::clear (int ds)
{
	assert (ds > 0);
	assert (ds <= MAX_DATA_SIZE);

	memset (_buf_ptr, 0, _stride * NBR_LINES * ds);
	for (int m = 0; m < MARGIN * MAX_DATA_SIZE; ++m)
	{
		_mem [m] = 0;
	}
}



template <class T>
void	ErrDifBuf::clear ()
{
	memset (_buf_ptr, 0, _stride * NBR_LINES * sizeof (T));
	for (int k = 0; k < MARGIN; ++k)
	{
		reinterpret_cast <T *> (&_mem [0]) [k] = 0;
	}
}



template <class T>
T *	ErrDifBuf::get_buf (int ofy)
{
	assert (ofy >= 0);
	assert (ofy < NBR_LINES);

	return (reinterpret_cast <T *> (_buf_ptr) + ofy * _stride + MARGIN);
}



template <class T>
T &	ErrDifBuf::use_mem (int pos)
{
	assert (pos >= 0);
	assert (pos < MARGIN);

	return (reinterpret_cast <T *> (&_mem [0]) [pos]);
}



template <class T>
const T&	ErrDifBuf::use_mem (int pos) const
{
	assert (pos >= 0);
	assert (pos < MARGIN);

	return (reinterpret_cast <const T *> (&_mem [0]) [pos]);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_ErrDifBuf_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
