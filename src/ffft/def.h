/*****************************************************************************

        def.h
        By Laurent de Soras

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (ffft_def_HEADER_INCLUDED)
#define	ffft_def_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace ffft
{



constexpr double  PI    = 3.1415926535897932384626433832795;
constexpr double  SQRT2 = 1.41421356237309514547462185873883;

#if defined (_MSC_VER)

	#define	ffft_FORCEINLINE	__forceinline

#else

	#define	ffft_FORCEINLINE	inline

#endif

// Compiler type
#define ffft_COMPILER_UNKNOWN (-1)
#define ffft_COMPILER_GCC     (1)
#define ffft_COMPILER_MSVC    (2)

#if defined (__GNUC__) || defined (__clang__)
	#define ffft_COMPILER ffft_COMPILER_GCC
#elif defined (_MSC_VER)
	#define ffft_COMPILER ffft_COMPILER_MSVC
#else
	#define ffft_COMPILER ffft_COMPILER_UNKNOWN
#endif




}	// namespace ffft



#endif	// ffft_def_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
