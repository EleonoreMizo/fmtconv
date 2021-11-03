/*****************************************************************************

        MatrixWrap.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_MatrixWrap_CODEHEADER_INCLUDED)
#define	fmtcl_MatrixWrap_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/fnc.h"

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
MatrixWrap <T>::MatrixWrap (int w, int h)
:	_w (w)
,	_h (h)
,	_msk_x (w - 1)
,	_msk_y (h - 1)
,	_shft (fstb::get_prev_pow_2 (w))
,	_mat (w * h, 0)
{
	assert (w > 0);
	assert (h > 0);
	assert (fstb::is_pow_2 (w));
	assert (fstb::is_pow_2 (h));
}



template <class T>
void	MatrixWrap <T>::clear (T fill_val)
{
	_mat.assign (_mat.size (), fill_val);
}



template <class T>
T &	MatrixWrap <T>::operator () (int x, int y)
{
	return at (wrap_x (x), wrap_y (y));
}



template <class T>
const T &	MatrixWrap <T>::operator () (int x, int y) const
{
	return at (wrap_x (x), wrap_y (y));
}



template <class T>
T &	MatrixWrap <T>::at (int x, int y)
{
	const auto     pos = encode_coord (x, y);

	return at (pos);
}



template <class T>
const T &	MatrixWrap <T>::at (int x, int y) const
{
	const auto     pos = encode_coord (x, y);

	return at (pos);
}



template <class T>
T &	MatrixWrap <T>::at (PosType pos)
{
	assert (pos >= 0);
	assert (pos < _mat.size ());

	return _mat [pos];
}



template <class T>
const T &	MatrixWrap <T>::at (PosType pos) const
{
	assert (pos >= 0);
	assert (pos < _mat.size ());

	return _mat [pos];
}



template <class T>
int	MatrixWrap <T>::wrap_x (int x) const
{
	return x & _msk_x;
}



template <class T>
int	MatrixWrap <T>::wrap_y (int y) const
{
	return y & _msk_y;
}



template <class T>
typename MatrixWrap <T>::PosType	MatrixWrap <T>::encode_coord (int x, int y) const
{
	assert (x >= 0);
	assert (x < _w);
	assert (y >= 0);
	assert (y < _h);

	return size_t (y) * size_t (_w) + size_t (x);
}



template <class T>
int	MatrixWrap <T>::decode_x (PosType pos) const
{
	return int (pos) & _msk_x;
}



template <class T>
int	MatrixWrap <T>::decode_y (PosType pos) const
{
	return int (pos >> _shft);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_MatrixWrap_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
