/*****************************************************************************

        CpuOpt.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (vsutl_CpuOpt_HEADER_INCLUDED)
#define	vsutl_CpuOpt_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/CpuId.h"

#include "VapourSynth.h"



namespace vsutl
{



class FilterBase;

class CpuOpt
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

		Level_ANY_AVAILABLE = 0xFFFF
	};

	explicit       CpuOpt (FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *param_name_0 = "cpuopt");
	virtual        ~CpuOpt () {}

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

	FilterBase &   _filter;
	fstb::CpuId    _cpu;
	Level          _level;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               CpuOpt ()                               = delete;
	               CpuOpt (const CpuOpt &other)            = delete;
	CpuOpt &       operator = (const CpuOpt &other)        = delete;
	bool           operator == (const CpuOpt &other) const = delete;
	bool           operator != (const CpuOpt &other) const = delete;

};	// class CpuOpt



}	// namespace vsutl



//#include "vsutl/CpuOpt.hpp"



#endif	// vsutl_CpuOpt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
