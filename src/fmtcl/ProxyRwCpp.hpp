/*****************************************************************************

        ProxyRwCpp.hpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ProxyRwCpp_CODEHEADER_INCLUDED)
#define	fmtcl_ProxyRwCpp_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



float	ProxyRwCpp <SplFmt_FLOAT>::read (const PtrConst::Type &ptr)
{
	return (ptr [0]);
}

void	ProxyRwCpp <SplFmt_FLOAT>::write (const Ptr::Type &ptr, float src)
{
	ptr [0] = src;
}

void	ProxyRwCpp <SplFmt_FLOAT>::read (const PtrConst::Type &ptr, float &src0, float &src1)
{
	src0 = ptr [0];
	src1 = ptr [1];
}

void	ProxyRwCpp <SplFmt_FLOAT>::write (const Ptr::Type &ptr, const float &src0, const float &src1)
{
	ptr [0] = src0;
	ptr [1] = src1;
}



int	ProxyRwCpp <SplFmt_INT8>::read (const PtrConst::Type &ptr)
{
	return (ptr [0]);
}

template <int C>
void	ProxyRwCpp <SplFmt_INT8>::write_clip (const Ptr::Type &ptr, int src)
{
	ptr [0] = uint8_t (fstb::limit (src, 0, (1 << C) - 1));
}

void	ProxyRwCpp <SplFmt_INT8>::write_no_clip (const Ptr::Type &ptr, int src)
{
	ptr [0] = uint8_t (src);
}

void	ProxyRwCpp <SplFmt_INT8>::read (const PtrConst::Type &ptr, float &src0, float &src1)
{
	src0 = float (ptr [0]);
	src1 = float (ptr [1]);
}

void	ProxyRwCpp <SplFmt_INT8>::write (const Ptr::Type &ptr, const float &src0, const float &src1)
{
	const int		v0 = fstb::limit (fstb::conv_int_fast (src0), 0, 255);
	const int		v1 = fstb::limit (fstb::conv_int_fast (src1), 0, 255);

	ptr [0] = uint8_t (v0);
	ptr [1] = uint8_t (v1);
}



int	ProxyRwCpp <SplFmt_INT16>::read (const PtrConst::Type &ptr)
{
	return (ptr [0]);
}

template <int C>
void	ProxyRwCpp <SplFmt_INT16>::write_clip (const Ptr::Type &ptr, int src)
{
	ptr [0] = uint16_t (fstb::limit (src, 0, (1 << C) - 1));
}

void	ProxyRwCpp <SplFmt_INT16>::write_no_clip (const Ptr::Type &ptr, int src)
{
	ptr [0] = uint16_t (src);
}

void	ProxyRwCpp <SplFmt_INT16>::read (const PtrConst::Type &ptr, float &src0, float &src1)
{
	src0 = float (ptr [0]);
	src1 = float (ptr [1]);
}

void	ProxyRwCpp <SplFmt_INT16>::write (const Ptr::Type &ptr, const float &src0, const float &src1)
{
	const int		v0 = fstb::limit (fstb::conv_int_fast (src0), 0, 65535);
	const int		v1 = fstb::limit (fstb::conv_int_fast (src1), 0, 65535);

	ptr [0] = uint16_t (v0);
	ptr [1] = uint16_t (v1);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_ProxyRwCpp_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
