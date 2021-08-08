/*****************************************************************************

        CpuOptBase.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_CpuOptBase_HEADER_INCLUDED)
#define fmtcl_CpuOptBase_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/CpuId.h"



namespace fmtcl
{



class CpuOptBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum Level
	{
		Level_NO_OPT = 0,
		Level_SSE2,          // 1
		Level_SSE3,
		Level_SSSE3,
		Level_SSE41,
		Level_SSE42,
		Level_FMA4,
		Level_AVX,           // 7
		Level_FMA3,
		Level_F16C,
		Level_AVX2,          // 10
		Level_AVX512F,

		Level_MASK = 0xFFFF,
		Level_ANY_AVAILABLE = Level_MASK
	};

	               CpuOptBase ()                        = default;
	virtual        ~CpuOptBase ()                       = default;
	               CpuOptBase (const CpuOptBase &other) = default;
	               CpuOptBase (CpuOptBase &&other)      = default;
	CpuOptBase &   operator = (const CpuOptBase &other) = default;
	CpuOptBase &   operator = (CpuOptBase &&other)      = default;

	void           set_level (Level level);

	bool           has_mmx () const;
	bool           has_isse () const;
	bool           has_sse () const;
	bool           has_sse2 () const;
	bool           has_sse3 () const;
	bool           has_ssse3 () const;
	bool           has_sse41 () const;
	bool           has_sse42 () const;
	bool           has_sse4a () const;
	bool           has_fma3 () const;
	bool           has_fma4 () const;
	bool           has_avx () const;
	bool           has_avx2 () const;
	bool           has_avx512f () const;
	bool           has_f16c () const;
	bool           has_cx16 () const;

	const fstb::CpuId &
	               use_raw_cpuid () const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	fstb::CpuId    _cpu;
	Level          _level = Level_ANY_AVAILABLE;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const CpuOptBase &other) const = delete;
	bool           operator != (const CpuOptBase &other) const = delete;

}; // class CpuOptBase



}  // namespace fmtcl



//#include "fmtcl/CpuOptBase.hpp"



#endif   // fmtcl_CpuOptBase_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
