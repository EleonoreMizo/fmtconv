/*****************************************************************************

        Matrix2020CL.h
        Author: Laurent de Soras, 2013

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtc_Matrix2020CL_HEADER_INCLUDED)
#define	fmtc_Matrix2020CL_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fmtcl/Matrix2020CLProc.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "VapourSynth4.h"

#include <memory>

#include <cstdint>



namespace fmtc
{



class Matrix2020CL
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef	Matrix2020CL	ThisType;

	explicit       Matrix2020CL (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Matrix2020CL () {}

	// vsutl::FilterBase
	virtual ::VSVideoInfo
	               get_video_info () const;
	virtual std::vector <::VSFilterDependency>
	               get_dependencies () const;
	virtual const ::VSFrame *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _nbr_planes_proc = fmtcl::Matrix2020CLProc::_nbr_planes;
	static constexpr int _rgb_int_bits    = fmtcl::Matrix2020CLProc::_rgb_int_bits;

	::VSVideoFormat
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src) const;

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;          // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;         // Output. Must be declared after _vi_in.

	bool           _range_set_flag;
	bool           _full_range_flag;
	bool           _to_yuv_flag;

	std::unique_ptr <fmtcl::Matrix2020CLProc>
	               _proc_uptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Matrix2020CL ()                                = delete;
	               Matrix2020CL (const Matrix2020CL &other)       = delete;
	               Matrix2020CL (Matrix2020CL &&other)            = delete;
	Matrix2020CL & operator = (const Matrix2020CL &other)         = delete;
	Matrix2020CL & operator = (Matrix2020CL &&other)              = delete;
	bool           operator == (const Matrix2020CL &other) const  = delete;
	bool           operator != (const Matrix2020CL &other) const  = delete;

};	// class Matrix2020CL



}	// namespace fmtc



//#include "fmtc/Matrix2020CL.hpp"



#endif	// fmtc_Matrix2020CL_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
