/*****************************************************************************

        Transfer.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Transfer_CODEHEADER_INCLUDED)
#define	fmtc_Transfer_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cassert>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void	Transfer::MultiTypeArray::set_type ()
{
	const int      old_len = _data_len;
	_data_len = int (sizeof (T));
	if (_data_len != old_len)
	{
		_arr.resize (_length * _data_len);
	}
}



template <class T>
T &	Transfer::MultiTypeArray::use (int pos)
{
	assert (_data_len > 0);
	assert (sizeof (T) == _data_len);
	assert (pos >= 0);
	assert (pos < int (_length));

	return ((reinterpret_cast <      T *> (&_arr [0])) [pos]);
}



template <class T>
const T &	Transfer::MultiTypeArray::use (int pos) const
{
	assert (_data_len > 0);
	assert (sizeof (T) == _data_len);
	assert (pos >= 0);
	assert (pos < _length);

	return ((reinterpret_cast <const T *> (&_arr [0])) [pos]);
}



size_t	Transfer::MultiTypeArray::get_size () const
{
	assert (_data_len >= 0);

	return (_length);
};



}	// namespace fmtc



#endif	// fmtc_Transfer_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
