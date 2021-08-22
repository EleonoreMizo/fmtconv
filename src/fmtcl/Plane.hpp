/*****************************************************************************

        Plane.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Plane_CODEHEADER_INCLUDED)
#define fmtcl_Plane_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <utility>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <typename T>
constexpr Plane <T>::Plane (T *ptr, int stride) noexcept
:	_ptr (ptr)
,	_stride (stride)
{
	assert (ptr != nullptr);
}



template <typename T>
template <typename U>
constexpr Plane <T>::Plane (const Plane <U> &other) noexcept
{
	_ptr    = reinterpret_cast <T *> (other._ptr);
	_stride = other._stride;
}



template <typename T>
template <typename U>
constexpr Plane <T>::Plane (Plane <U> &&other) noexcept
{
	_ptr    = std::move (reinterpret_cast <T *> (other._ptr));
	_stride = std::move (other._stride);
}



template <typename T>
constexpr bool	Plane <T>::is_valid (int h) const noexcept
{
	assert (h > 0);

	if (_ptr == nullptr)
	{
		return false;
	}
	if (h > 1 && _stride == 0)
	{
		return false;
	}

	return true;
}



template <typename T>
void	Plane <T>::step_pix () noexcept
{
	++ _ptr;
}



template <typename T>
void	Plane <T>::step_pix (int n) noexcept
{
	_ptr += n;
}



template <typename T>
void	Plane <T>::step_line () noexcept
{
	_ptr = reinterpret_cast <T *> (
		reinterpret_cast <TEquivByte *> (_ptr) + _stride
	);
}



template <typename T>
void	Plane <T>::step_line (int n) noexcept
{
	_ptr = reinterpret_cast <T *> (
		reinterpret_cast <TEquivByte *> (_ptr) + _stride * n
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



#endif   // fmtcl_Plane_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
