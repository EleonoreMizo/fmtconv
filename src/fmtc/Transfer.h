/*****************************************************************************

        Transfer.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Transfer_HEADER_INCLUDED)
#define	fmtc_Transfer_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"

#include "fmtcl/TransCurve.h"
#include "fmtcl/TransModel.h"
#include "fmtcl/TransOpInterface.h"
#include "fmtcl/TransOpLogC.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "VapourSynth4.h"

#include <memory>



namespace fmtc
{



class Transfer
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       Transfer (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Transfer () = default;

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

	typedef  std::shared_ptr <fmtcl::TransOpInterface> OpSPtr;

	::VSVideoFormat
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src) const;

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;     // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;    // Output. Must be declared after _vi_in.

	bool           _sse2_flag = false;
	bool           _avx2_flag = false;
	std::string    _transs;
	std::string    _transd;
	double         _contrast  = 1;
	double         _gcor      = 1;
	bool           _full_range_src_flag = true;
	bool           _full_range_dst_flag = true;
	fmtcl::TransCurve
	               _curve_s = fmtcl::TransCurve_UNDEF;
	fmtcl::TransCurve
	               _curve_d = fmtcl::TransCurve_UNDEF;
	fmtcl::TransOpLogC::ExpIdx // Exposure Index for the Arri Log C curves
	               _logc_ei_s = fmtcl::TransOpLogC::ExpIdx_800;
	fmtcl::TransOpLogC::ExpIdx
	               _logc_ei_d = fmtcl::TransOpLogC::ExpIdx_800;

	std::unique_ptr <fmtcl::TransModel>
	               _model_uptr;

	bool           _dbg_flag = false;
	std::string    _dbg_name;     // Property name



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Transfer ()                               = delete;
	               Transfer (const Transfer &other)          = delete;
	               Transfer (Transfer &&other)               = delete;
	Transfer &     operator = (const Transfer &other)        = delete;
	Transfer &     operator = (Transfer &&other)             = delete;
	bool           operator == (const Transfer &other) const = delete;
	bool           operator != (const Transfer &other) const = delete;

};	// class Transfer



}	// namespace fmtc



//#include "fmtc/Transfer.hpp"



#endif	// fmtc_Transfer_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
