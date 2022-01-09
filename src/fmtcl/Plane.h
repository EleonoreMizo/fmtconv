/*****************************************************************************

        Plane.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Plane_HEADER_INCLUDED)
#define fmtcl_Plane_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <type_traits>

#include <cstddef>
#include <cstdint>



namespace fmtcl
{



template <typename T = uint8_t>
class Plane
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef T DataType;

	               Plane ()                            = default;
	               Plane (const Plane <T> &other)      = default;
	               Plane (Plane <T> &&other)           = default;
	               ~Plane ()                           = default;
	Plane <T> &    operator = (const Plane <T> &other) = default;
	Plane <T> &    operator = (Plane <T> &&other)      = default;

	constexpr      Plane (T *ptr, int stride) noexcept;
	template <typename U>
	constexpr inline
	               Plane (const Plane <U> &other) noexcept;
	template <typename U>
	constexpr inline
	               Plane (Plane <U> &&other) noexcept;

	constexpr inline bool
	               is_valid (int h = 1) const noexcept;

	inline void    step_pix () noexcept;
	inline void    step_pix (int n) noexcept;
	inline void    step_line () noexcept;
	inline void    step_line (int n) noexcept;

	T *            _ptr    = nullptr;
	ptrdiff_t      _stride = 0;         // Always in bytes



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef typename std::conditional <
		std::is_const <T>::value, const uint8_t, uint8_t
	>::type TEquivByte;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const Plane <T> &other) const = delete;
	bool           operator != (const Plane <T> &other) const = delete;

}; // class Plane



}  // namespace fmtcl



#include "fmtcl/Plane.hpp"



#endif   // fmtcl_Plane_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
