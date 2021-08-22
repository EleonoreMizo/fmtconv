/*****************************************************************************

        Frame.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Frame_HEADER_INCLUDED)
#define fmtcl_Frame_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Cst.h"
#include "fmtcl/Plane.h"

#include <array>
#include <initializer_list>



namespace fmtcl
{



template <typename T = uint8_t>
class Frame
:	public std::array <Plane <T>, Cst::_max_nbr_planes>
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef std::array <Plane <T>, Cst::_max_nbr_planes> Inherited;

	using typename Inherited::value_type;
	using typename Inherited::size_type;
	using typename Inherited::difference_type;
	using typename Inherited::reference;
	using typename Inherited::const_reference;
	using typename Inherited::pointer;
	using typename Inherited::const_pointer;
	using typename Inherited::iterator;
	using typename Inherited::const_iterator;
	using typename Inherited::reverse_iterator;
	using typename Inherited::const_reverse_iterator;

	using std::array <Plane <T>, Cst::_max_nbr_planes>::array;
	template <typename... AT>
	constexpr inline Frame (value_type &&p0, AT &&... args) noexcept;
	constexpr inline
	               Frame (std::initializer_list <value_type> il);
	template <typename U>
	constexpr inline
	               Frame (const Frame <U> &other) noexcept;
	template <typename U>
	constexpr inline
	               Frame (Frame <U> &&other) noexcept;

	constexpr inline bool
	               is_valid (int nbr_planes = Cst::_max_nbr_planes, int h = 1) const noexcept;

	inline void    step_pix () noexcept;
	inline void    step_pix (int n) noexcept;
	inline void    step_line () noexcept;
	inline void    step_line (int n) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

}; // class Frame



}  // namespace fmtcl



#include "fmtcl/Frame.hpp"



#endif   // fmtcl_Frame_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
