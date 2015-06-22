/*****************************************************************************

        ReadWrapperFlt.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ReadWrapperFlt_CODEHEADER_INCLUDED)
#define	fmtcl_ReadWrapperFlt_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class SRC, bool PF>
template <class VI, class VF>
void	ReadWrapperFlt <SRC, PF>::read (typename SRC::PtrConst::Type ptr, VF &src0, VF &src1, const VI &zero, int /*len*/)
{
	SRC::read_flt (ptr, src0, src1, zero);
}



template <class SRC>
template <class VI, class VF>
void	ReadWrapperFlt <SRC, true>::read (typename SRC::PtrConst::Type ptr, VF &src0, VF &src1, const VI &zero, int len)
{
	SRC::read_flt_partial (ptr, src0, src1, zero, len);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_ReadWrapperFlt_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
