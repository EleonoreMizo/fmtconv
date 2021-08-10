/*****************************************************************************

        Matrix2020CL.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_Matrix2020CL_HEADER_INCLUDED)
#define fmtcavs_Matrix2020CL_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "avsutl/VideoFilterBase.h"
#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/ProcAlpha.h"
#include "fmtcl/Matrix2020CLProc.h"

#include <memory>
#include <string>



namespace fmtcavs
{



class Matrix2020CL
:	public avsutl::VideoFilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef avsutl::VideoFilterBase Inherited;

	enum Param
	{
		Param_CLIP_SRC = 0,
		Param_FULL,
		Param_CSP,
		Param_BITS,
		Param_CPUOPT,

		Param_NBR_ELT
	};

	explicit       Matrix2020CL (::IScriptEnvironment &env, const ::AVSValue &args);
	virtual        ~Matrix2020CL () = default;

	// VideoFilterBase
	::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr) override;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_planes_proc = fmtcl::Matrix2020CLProc::_nbr_planes;
	static constexpr int _rgb_int_bits    = fmtcl::Matrix2020CLProc::_rgb_int_bits;

	static FmtAvs  get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src);

	::PClip        _clip_src_sptr;
	const ::VideoInfo
	               _vi_src;

	std::unique_ptr <fmtcl::Matrix2020CLProc>
	               _proc_uptr;

	std::unique_ptr <fmtcavs::ProcAlpha>
	               _proc_alpha_uptr;

	bool           _range_set_flag = false;
	bool           _full_flag      = false;
	bool           _to_yuv_flag    = false;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix2020CL ()                               = delete;
	               Matrix2020CL (const Matrix2020CL &other)      = delete;
	               Matrix2020CL (Matrix2020CL &&other)           = delete;
	Matrix2020CL & operator = (const Matrix2020CL &other)        = delete;
	Matrix2020CL & operator = (Matrix2020CL &&other)             = delete;
	bool           operator == (const Matrix2020CL &other) const = delete;
	bool           operator != (const Matrix2020CL &other) const = delete;

}; // class Matrix2020CL



}  // namespace fmtcavs



//#include "fmtcavs/Matrix2020CL.hpp"



#endif   // fmtcavs_Matrix2020CL_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
