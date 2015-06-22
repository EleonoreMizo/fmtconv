/*****************************************************************************

        ReadWrapperInt.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ReadWrapperInt_CODEHEADER_INCLUDED)
#define	fmtcl_ReadWrapperInt_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class SRC, class S16R, bool PF>
template <class VI>
VI	ReadWrapperInt <SRC, S16R, PF>::read (const typename SRC::PtrConst::Type &ptr, const VI &zero, const VI &sign_bit, int /*len*/)
{
	return (S16R::read (ptr, zero, sign_bit));
}



template <class SRC, class S16R>
template <class VI>
VI	ReadWrapperInt <SRC, S16R, true>::read (const typename SRC::PtrConst::Type &ptr, const VI &zero, const VI &sign_bit, int len)
{
	return (S16R::read_partial (ptr, zero, sign_bit, len));
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_ReadWrapperInt_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
