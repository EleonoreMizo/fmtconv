/*****************************************************************************

        CoefArrInt.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_CoefArrInt_CODEHEADER_INCLUDED)
#define	fmtcl_CoefArrInt_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	CoefArrInt::get_size () const
{
	return (_size);
}



int	CoefArrInt::get_coef (int pos) const
{
	assert (pos >= 0);
	assert (pos < _size);

	return (_coef_arr [pos << _vect_shift]);
}



const int16_t *	CoefArrInt::use_vect_sse2 (int pos) const
{
	assert (pos >= 0);
	assert (pos < _size);

	return (&_coef_arr [pos * VECT_LEN_SSE2]);
}



const int16_t *	CoefArrInt::use_vect_avx2 (int pos) const
{
	assert (pos >= 0);
	assert (pos < _size);

	return (&_coef_arr [pos * VECT_LEN_AVX2]);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_CoefArrInt_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
