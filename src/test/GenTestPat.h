/*****************************************************************************

        GenTestPat.h
        Author: Laurent de Soras, 2022

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (GenTestPat_HEADER_INCLUDED)
#define GenTestPat_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cstdint>



class GenTestPat
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static int     generate_patterns ();



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static uint32_t
	               pack_s (uint16_t v0, uint16_t v1 = 0) noexcept;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               GenTestPat ()                               = delete;
	               GenTestPat (const GenTestPat &other)        = delete;
	               GenTestPat (GenTestPat &&other)             = delete;
	GenTestPat &   operator = (const GenTestPat &other)        = delete;
	GenTestPat &   operator = (GenTestPat &&other)             = delete;
	bool           operator == (const GenTestPat &other) const = delete;
	bool           operator != (const GenTestPat &other) const = delete;

}; // class GenTestPat



//#include "test/GenTestPat.hpp"



#endif   // GenTestPat_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
