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
	x &= _msk_x;
	y &= _msk_y;

	return (_mat [y * _w + x]);
}



template <class T>
const T &	MatrixWrap <T>::operator () (int x, int y) const
{
	x &= _msk_x;
	y &= _msk_y;

	return (_mat [y * _w + x]);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_MatrixWrap_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
