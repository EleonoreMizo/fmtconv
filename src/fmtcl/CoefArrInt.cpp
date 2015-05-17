/*****************************************************************************

        CoefArrInt.cpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/CoefArrInt.h"

#include <cassert>
#include <cstring>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



CoefArrInt::CoefArrInt ()
:	_coef_arr ()
,	_avx2_flag (false)
,	_size (0)
,	_vect_shift (3)   // For SSE2
{
	// Nothing
}



void	CoefArrInt::clear ()
{
	_coef_arr.clear ();
	_size = 0;
}



void	CoefArrInt::set_avx2_mode (bool avx2_flag)
{
	assert (_size == 0);

	_avx2_flag  = avx2_flag;
	_vect_shift = 4;
}



void	CoefArrInt::resize (int size)
{
	assert (size >= 0);

	_size = size;
	const int      size_i16 = _size << _vect_shift;
	_coef_arr.resize (size_i16);
}



void	CoefArrInt::set_coef (int pos, int val)
{
	assert (pos >= 0);
	assert (pos < _size);

	const int      vect_len = 1   << _vect_shift;
	const int      pos_i16  = pos << _vect_shift;

	for (int i = 0; i < vect_len; ++i)
	{
		_coef_arr [pos_i16 + i] = int16_t (val);
	}
}



void	CoefArrInt::set_coef_int32 (int pos, int val)
{
	assert (pos >= 0);
	assert (pos < _size);

	const int      vect_len = 1   << _vect_shift;
	const int      pos_i16  = pos << _vect_shift;

	for (int i = 0; i < vect_len; i += 2)
	{
		*(reinterpret_cast <int32_t *> (&_coef_arr [pos_i16 + i])) = int32_t (val);
	}
}



void	CoefArrInt::copy_coef (int pos_to, int pos_from)
{
	assert (pos_to >= 0);
	assert (pos_to < _size);
	assert (pos_from >= 0);
	assert (pos_from < _size);

	if (pos_to != pos_from)
	{
		const int      pos_to_i16   = pos_to   << _vect_shift;
		const int      pos_from_i16 = pos_from << _vect_shift;
		memcpy (
			&_coef_arr [pos_to_i16],
			&_coef_arr [pos_from_i16],
			sizeof (int16_t) << _vect_shift
		);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
