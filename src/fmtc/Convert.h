/*****************************************************************************

        Convert.h
        Author: Laurent de Soras, 2014

	========================
	*** WORK IN PROGRESS ***
	========================

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtc_Convert_HEADER_INCLUDED)
#define	fmtc_Convert_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtc/ConvStep.h"
#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/ColorSpaceH265.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"

#include <list>



namespace fmtc
{



class Convert
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       Convert (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Convert () {}

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

	typedef	std::list <ConvStep>	StepList;

	void           retrieve_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src);
	ConvStep::Range
	               retrieve_range (const ::VSVideoFormat &fmt, const ::VSMap &in, ::VSMap &out, const char arg_0 []);
	fmtcl::TransCurve
						retrieve_tcurve (const ::VSVideoFormat &fmt, const ::VSMap &in, ::VSMap &out, const char arg_0 [], const char def_0 []);
	void           find_conversion_steps (const ::VSMap &in, ::VSMap &out);
	void           fill_conv_step_with_cs (ConvStep &step, const ::VSVideoFormat &fmt);
	bool           fill_conv_step_with_curve (ConvStep &step, const ::VSVideoFormat &fmt, fmtcl::TransCurve tcurve, fmtcl::ColorSpaceH265 mat);
	void           fill_conv_step_with_gcor (ConvStep &step, const ::VSMap &in, ::VSMap &out, const char arg_0 []);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.
	::VSPlugin &   _fmtc;

	StepList       _step_list;

	// Cached input parameters
	int            _col_fam;
	fmtcl::ColorSpaceH265
	               _mats;         // Custom not allowed here
	fmtcl::ColorSpaceH265
	               _matd;         // Can be ColorSpaceH265_CUSTOM
	fmtcl::ChromaPlacement
	               _cplaces;
	fmtcl::ChromaPlacement
	               _cplaced;
	ConvStep::Range
	               _fulls;
	ConvStep::Range
	               _fulld;
	fmtcl::TransCurve             // Transfer curve for source clip. Can be undefined.
	               _transs;
	fmtcl::TransCurve             // Same, for destination clip.
	               _transd;
	double         _gcors;        // Additionnal gamma correction for source clip. 1: neutral or not defined.
	double         _gcord;        // Same, for destination clip.



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Convert ()                               = delete;
	               Convert (const Convert &other)           = delete;
	Convert &      operator = (const Convert &other)        = delete;
	bool           operator == (const Convert &other) const = delete;
	bool           operator != (const Convert &other) const = delete;

};	// class Convert



}	// namespace fmtc



//#include "fmtc/Convert.hpp"



#endif	// fmtc_Convert_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
