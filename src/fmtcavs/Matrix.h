/*****************************************************************************

        Matrix.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_Matrix_HEADER_INCLUDED)
#define fmtcavs_Matrix_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "avsutl/VideoFilterBase.h"
#include "fmtcavs/FmtAvs.h"
#include "fmtcavs/ProcAlpha.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/MatrixProc.h"

#include <memory>
#include <string>



namespace fmtcavs
{



class Matrix
:	public avsutl::VideoFilterBase
{
/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef avsutl::VideoFilterBase Inherited;

	enum Param
	{
		Param_CLIP_SRC = 0,
		Param_MAT,
		Param_MATS,
		Param_MATD,
		Param_FULLS,
		Param_FULLD,
		Param_COEF,
		Param_CSP,
		Param_COL_FAM,
		Param_BITS,
		Param_SINGLEOUT,
		Param_CPUOPT,

		Param_NBR_ELT
	};

	explicit       Matrix (::IScriptEnvironment &env, const ::AVSValue &args);
	virtual        ~Matrix () = default;

	// VideoFilterBase
	::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr) override;

	static fmtcl::ColorSpaceH265
	               find_cs_from_mat_str (::IScriptEnvironment &env, const std::string &mat, bool allow_2020cl_flag);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_planes_proc = fmtcl::MatrixProc::_nbr_planes;

	FmtAvs         get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src, int &plane_out, bool &force_col_fam_flag);
	FmtAvs         find_dst_col_fam (fmtcl::ColorSpaceH265 tmp_csp, FmtAvs fmt_dst, const FmtAvs &fmt_src);

	::PClip        _clip_src_sptr;
	const ::VideoInfo
	               _vi_src;

	int            _plane_out  = -1;
	std::unique_ptr <fmtcl::MatrixProc>
	               _proc_uptr;

	std::unique_ptr <fmtcavs::ProcAlpha>
	               _proc_alpha_uptr;

	bool           _range_def_flag = false;
	bool           _fulls_flag     = false;
	bool           _fulld_flag     = false;
	fmtcl::ColorSpaceH265
	               _csp_out = fmtcl::ColorSpaceH265_UNDEF;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix ()                               = delete;
	               Matrix (const Matrix &other)            = delete;
	               Matrix (Matrix &&other)                 = delete;
	Matrix &       operator = (const Matrix &other)        = delete;
	Matrix &       operator = (Matrix &&other)             = delete;
	bool           operator == (const Matrix &other) const = delete;
	bool           operator != (const Matrix &other) const = delete;

}; // class Matrix



}  // namespace fmtcavs



//#include "fmtcavs/Matrix.hpp"



#endif   // fmtcavs_Matrix_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
