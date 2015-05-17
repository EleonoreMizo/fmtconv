/*****************************************************************************

        CpuOpt.cpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vsutl/CpuOpt.h"
#include "vsutl/FilterBase.h"

#include <cassert>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



CpuOpt::CpuOpt (FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *param_name_0)
:	_filter (filter)
,	_cpu ()
,	_level (static_cast <Level> (filter.get_arg_int (
		in, out, param_name_0, Level_ANY_AVAILABLE
	) & 0xFFFF))
{
	assert (&filter != 0);
	assert (&in != 0);
	assert (&out != 0);
	assert (param_name_0 != 0);
}



void	CpuOpt::set_level (Level level)
{
	assert (level >= 0);
	assert (level <= Level_ANY_AVAILABLE);

	_level = level;
}



bool	CpuOpt::has_mmx () const
{
	return (_cpu._mmx_flag && _level >= Level_SSE2);
}



bool	CpuOpt::has_isse () const
{
	return (_cpu._isse_flag && _level >= Level_SSE2);
}



bool	CpuOpt::has_sse () const
{
	return (_cpu._sse_flag && _level >= Level_SSE2);
}



bool	CpuOpt::has_sse2 () const
{
	return (_cpu._sse2_flag && _level >= Level_SSE2);
}



bool	CpuOpt::has_sse3 () const
{
	return (_cpu._sse3_flag && _level >= Level_SSE3);
}



bool	CpuOpt::has_ssse3 () const
{
	return (_cpu._ssse3_flag && _level >= Level_SSSE3);
}



bool	CpuOpt::has_sse41 () const
{
	return (_cpu._sse41_flag && _level >= Level_SSE41);
}



bool	CpuOpt::has_sse42 () const
{
	return (_cpu._sse42_flag && _level >= Level_SSE42);
}



bool	CpuOpt::has_sse4a () const
{
	return (_cpu._sse4a_flag && _level >= Level_FMA4);
}



bool	CpuOpt::has_fma3 () const
{
	return (_cpu._fma3_flag && _level >= Level_FMA3);
}



bool	CpuOpt::has_fma4 () const
{
	return (_cpu._fma4_flag && _level >= Level_FMA4);
}



bool	CpuOpt::has_avx () const
{
	return (_cpu._avx_flag && _level >= Level_AVX);
}



bool	CpuOpt::has_avx2 () const
{
	return (_cpu._avx2_flag && _level >= Level_AVX2);
}



bool	CpuOpt::has_avx512f () const
{
	return (_cpu._avx512f_flag && _level >= Level_AVX512F);
}



bool	CpuOpt::has_f16c () const
{
	return (_cpu._f16c_flag && _level >= Level_F16C);
}



bool	CpuOpt::has_cx16 () const
{
	return (_cpu._cx16_flag);
}



const fstb::CpuId &	CpuOpt::use_raw_cpuid () const
{
	return (_cpu);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace vsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
