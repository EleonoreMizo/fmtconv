/*****************************************************************************

        Scaler.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Scaler_CODEHEADER_INCLUDED)
#define	fmtcl_Scaler_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#if (fstb_ARCHI == fstb_ARCHI_X86)



template <class SRC, bool PF>
template <class VI, class VF>
void	Scaler::ReadWrapperFlt <SRC, PF>::read (typename SRC::PtrConst::Type ptr, VF &src0, VF &src1, const VI &zero, int /*len*/)
{
	SRC::read_flt (ptr, src0, src1, zero);
}

template <class SRC>
template <class VI, class VF>
void	Scaler::ReadWrapperFlt <SRC, true>::read (typename SRC::PtrConst::Type ptr, VF &src0, VF &src1, const VI &zero, int len)
{
	SRC::read_flt_partial (ptr, src0, src1, zero, len);
}



template <class SRC, class S16R, bool PF>
template <class VI>
VI	Scaler::ReadWrapperInt <SRC, S16R, PF>::read (const typename SRC::PtrConst::Type &ptr, const VI &zero, const VI &sign_bit, int /*len*/)
{
	return (S16R::read (ptr, zero, sign_bit));
}

template <class SRC, class S16R>
template <class VI>
VI	Scaler::ReadWrapperInt <SRC, S16R, true>::read (const typename SRC::PtrConst::Type &ptr, const VI &zero, const VI &sign_bit, int len)
{
	return (S16R::read_partial (ptr, zero, sign_bit, len));
}



#endif   // fstb_ARCHI_X86



}	// namespace fmtcl



#endif	// fmtcl_Scaler_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
