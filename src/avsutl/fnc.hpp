/*****************************************************************************

        fnc.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (avsutl_fnc_CODEHEADER_INCLUDED)
#define avsutl_fnc_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <algorithm>

#include <cassert>
#include <cstdint>



namespace avsutl
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Stride in bytes, w and h in pixels
template <class T>
void	fill_block (void *ptr, T val, int stride, int w, int h)
{
	assert (ptr != nullptr);
	assert (stride > 0);
	assert (w > 0);
	assert (h > 0);

	constexpr int  min_align = 16;

	if (sizeof (val) == 1 && stride >= 0 && stride - w < min_align)
	{
		auto           u8_ptr = static_cast <uint8_t *> (ptr);
		std::fill (u8_ptr, u8_ptr + stride * (h - 1) + w, uint8_t (val));
	}

	else
	{
		T *            data_ptr   = reinterpret_cast <T *> (ptr);
		const int      stride_pix = stride / int (sizeof (val));
		assert (stride_pix * int (sizeof (val)) == stride);
		for (int y = 0; y < h; ++y)
		{
			std::fill (data_ptr, data_ptr + w, val);
			data_ptr += stride_pix;
		}
	}
}



}  // namespace avsutl



#endif   // avsutl_fnc_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
