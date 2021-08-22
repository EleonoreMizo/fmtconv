/*****************************************************************************

        Frame.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Frame_CODEHEADER_INCLUDED)
#define fmtcl_Frame_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <utility>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <typename T>
template <typename... AT>
constexpr Frame <T>::Frame (value_type &&p0, AT &&... args) noexcept
:	Inherited { p0, std::forward <AT> (args)... }
{
	// Nothing
}



template <typename T>
constexpr Frame <T>::Frame (std::initializer_list <value_type> il)
{
	const auto     sz = il.size ();
	assert (sz <= this->size ());
	auto           it = il.begin ();
	for (size_type p = 0; p < sz; ++p)
	{
		(*this) [p] = *it;
		++ it;
	}
}



template <typename T>
template <typename U>
constexpr Frame <T>::Frame (const Frame <U> &other) noexcept
{
	for (size_type p = 0; p < this->size (); ++p)
	{
		(*this) [p] = other [p];
	}
}



template <typename T>
template <typename U>
constexpr Frame <T>::Frame (Frame <U> &&other) noexcept
{
	for (size_type p = 0; p < this->size (); ++p)
	{
		(*this) [p] = std::move (other [p]);
	}
}



template <typename T>
constexpr bool	Frame <T>::is_valid (int nbr_planes, int h) const noexcept
{
	assert (nbr_planes > 0);
	assert (nbr_planes <= this->size ());
	assert (h > 0);

	for (int p = 0; p < nbr_planes; ++p)
	{
		if (! (*this) [p].is_valid (h))
		{
			return false;
		}
	}

	return true;
}



template <typename T>
void	Frame <T>::step_pix () noexcept
{
	for (auto &plane : *this)
   {
      plane.step_pix ();
   }
}



template <typename T>
void	Frame <T>::step_pix (int n) noexcept
{
	for (auto &plane : *this)
   {
      plane.step_pix (n);
   }
}



template <typename T>
void	Frame <T>::step_line () noexcept
{
	for (auto &plane : *this)
   {
      plane.step_line ();
   }
}



template <typename T>
void	Frame <T>::step_line (int n) noexcept
{
	for (auto &plane : *this)
   {
      plane.step_line (n);
   }
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcl



#endif   // fmtcl_Frame_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
