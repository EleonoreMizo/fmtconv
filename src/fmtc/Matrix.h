/*****************************************************************************

        Matrix.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Matrix_HEADER_INCLUDED)
#define	fmtc_Matrix_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/CoefArrInt.h"
#include "fmtcl/ColorSpaceH265.h"
#include "fmtcl/Mat4.h"
#include "fmtcl/MatrixProc.h"
#include "fstb/AllocAlign.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "VapourSynth.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#include <emmintrin.h>
#endif

#include <memory>
#include <vector>

#include <cstdint>



namespace fmtc
{



class Matrix
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Matrix	ThisType;

	explicit       Matrix (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Matrix () = default;

	// vsutl::FilterBase
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core);
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);

	static fmtcl::ColorSpaceH265
	               find_cs_from_mat_str (const vsutl::FilterBase &flt, const std::string &mat, bool allow_2020cl_flag);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_planes = fmtcl::MatrixProc::_nbr_planes;

	enum Dir
	{
		Dir_IN = 0,
		Dir_OUT,

		Dir_NBR_ELT
	};

	const ::VSFormat *
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src, int &plane_out, bool &force_col_fam_flag) const;

	const ::VSFormat *
	               find_dst_col_fam (fmtcl::ColorSpaceH265 tmp_csp, const ::VSFormat *fmt_dst_ptr, const ::VSFormat &fmt_src, ::VSCore &core);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;          // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;         // Output. Must be declared after _vi_in.

	bool				_sse_flag;
	bool           _sse2_flag;
	bool           _avx_flag;
	bool           _avx2_flag;

	bool           _range_set_src_flag;
	bool           _range_set_dst_flag;
	bool           _full_range_src_flag;
	bool           _full_range_dst_flag;
	fmtcl::Mat4    _mat_main;       // Main matrix, float input, float output
	fmtcl::ColorSpaceH265
	               _csp_out;
	int            _plane_out;      // Plane index for single plane output (0-2), or a negative number if all planes are processed.

	std::unique_ptr <fmtcl::MatrixProc>
	               _proc_uptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix ()                               = delete;
	               Matrix (const Matrix &other)            = delete;
	               Matrix (Matrix &&other)                 = delete;
	Matrix &       operator = (const Matrix &other)        = delete;
	Matrix &       operator = (Matrix &&other)             = delete;
	bool           operator == (const Matrix &other) const = delete;
	bool           operator != (const Matrix &other) const = delete;

};	// class Matrix



}	// namespace fmtc



//#include "fmtc/Matrix.hpp"



#endif	// fmtc_Matrix_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
