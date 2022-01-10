/*****************************************************************************

        Resample.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtc_Resample_HEADER_INCLUDED)
#define	fmtc_Resample_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/FilterResize.h"
#include "fmtcl/InterlacingType.h"
#include "fmtcl/KernelData.h"
#include "fmtcl/ResamplePlaneData.h"
#include "fmtcl/ResampleSpecPlane.h"
#include "fmtcl/ResampleUtil.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "vsutl/PlaneProcCbInterface.h"
#include "vsutl/PlaneProcessor.h"
#include "VapourSynth4.h"

#include <array>
#include <map>
#include <memory>
#include <mutex>



namespace fmtc
{



class Resample
:	public vsutl::FilterBase
,	public vsutl::PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       Resample (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Resample () = default;

	// vsutl::FilterBase
	virtual ::VSVideoInfo
	               get_video_info () const;
	virtual std::vector <::VSFilterDependency>
	               get_dependencies () const;
	virtual const ::VSFrame *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);

	static fmtcl::ChromaPlacement
	               conv_str_to_chroma_placement (const vsutl::FilterBase &flt, std::string cplace);
	static void    conv_str_to_chroma_subspl (const vsutl::FilterBase &flt, int &ssh, int &ssv, std::string css);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// vsutl::PlaneProcCbInterface
	virtual int    do_process_plane (::VSFrame &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	using Ru = fmtcl::ResampleUtil;

	static constexpr int _max_nbr_planes = 3;

	typedef std::array <fmtcl::ResamplePlaneData, _max_nbr_planes> PlaneDataArray;

	::VSVideoFormat
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSVideoFormat &fmt_src) const;
	bool           cumulate_flag (bool flag, const ::VSMap &in, ::VSMap &out, const char name_0 [], int pos = 0) const;
	int            process_plane_proc (::VSFrame &dst, int n, int plane_index, ::VSFrameContext &frame_ctx, const vsutl::NodeRefSPtr &src_node1_sptr, const Ru::FrameInfo &frame_info);
	int            process_plane_copy (::VSFrame &dst, int n, int plane_index, ::VSFrameContext &frame_ctx, const vsutl::NodeRefSPtr &src_node1_sptr);
	fmtcl::FilterResize *
	               create_or_access_plane_filter (int plane_index, fmtcl::InterlacingType itl_d, fmtcl::InterlacingType itl_s);
	void           create_all_plane_specs ();

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.

	int            _src_width  = 0;
	int            _src_height = 0;
	fmtcl::SplFmt  _src_type   = fmtcl::SplFmt_ILLEGAL;
	int            _src_res    = 0;
	fmtcl::SplFmt  _dst_type   = fmtcl::SplFmt_ILLEGAL;
	int            _dst_res    = 0;
	Ru::InterlacingParam
	               _interlaced_src  = Ru::InterlacingParam_INVALID;
	Ru::InterlacingParam
	               _interlaced_dst  = Ru::InterlacingParam_INVALID;
	Ru::FieldOrder _field_order_src = Ru::FieldOrder_INVALID;
	Ru::FieldOrder _field_order_dst = Ru::FieldOrder_INVALID;
	bool           _int_flag   = false;
	bool           _norm_flag  = false;
	bool           _range_set_in_flag   = false;
	bool           _range_set_out_flag  = false;
	bool           _full_range_in_flag  = false;
	bool           _full_range_out_flag = false;
	bool           _cplace_d_set_flag   = false;
	fmtcl::ChromaPlacement
	               _cplace_s   = fmtcl::ChromaPlacement_MPEG2;
	fmtcl::ChromaPlacement
	               _cplace_d   = fmtcl::ChromaPlacement_MPEG2;

	bool           _sse2_flag  = false;
	bool           _avx2_flag  = false;
	vsutl::PlaneProcessor
	               _plane_processor;
	std::mutex     _filter_mutex;          // To access _filter_uptr_map.
	std::map <fmtcl::ResampleSpecPlane, std::unique_ptr <fmtcl::FilterResize> >
	               _filter_uptr_map;       // Created only on request.

	PlaneDataArray _plane_data_arr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Resample ()                               = delete;
	               Resample (const Resample &other)          = delete;
	               Resample (Resample &&other)               = delete;
	Resample &     operator = (const Resample &other)        = delete;
	Resample &     operator = (Resample &&other)             = delete;
	bool           operator == (const Resample &other) const = delete;
	bool           operator != (const Resample &other) const = delete;

};	// class Resample



}	// namespace fmtc



//#include "fmtc/Resample.hpp"



#endif	// fmtc_Resample_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
