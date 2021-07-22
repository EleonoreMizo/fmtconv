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
#include "fmtcl/ColorFamily.h"
#include "fmtcl/FilterResize.h"
#include "fmtcl/InterlacingType.h"
#include "fmtcl/KernelData.h"
#include "fmtcl/ResamplePlaneData.h"
#include "fmtcl/ResampleSpecPlane.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"
#include "vsutl/PlaneProcCbInterface.h"
#include "vsutl/PlaneProcessor.h"
#include "VapourSynth.h"

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <vector>



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
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core);
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);

	static fmtcl::ChromaPlacement
	               conv_str_to_chroma_placement (const vsutl::FilterBase &flt, std::string cplace);
	static void    conv_str_to_chroma_subspl (const vsutl::FilterBase &flt, int &ssh, int &ssv, std::string css);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// vsutl::PlaneProcCbInterface
	virtual int    do_process_plane (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr, const vsutl::NodeRefSPtr &src_node2_sptr, const vsutl::NodeRefSPtr &src_node3_sptr);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int _max_nbr_planes = 3;

	enum InterlacingParam
	{
		InterlacingParam_FRAMES = 0,
		InterlacingParam_FIELDS,
		InterlacingParam_AUTO,

		InterlacingParam_NBR_ELT
	};

	enum FieldOrder
	{
		FieldOrder_BFF = 0,
		FieldOrder_TFF,
		FieldOrder_AUTO,

		FieldOrder_NBR_ELT
	};

	class FrameInfo
	{
	public:
		bool           _itl_s_flag = false;
		bool           _top_s_flag = false;
		bool           _itl_d_flag = false;
		bool           _top_d_flag = false;
	};

	typedef std::array <fmtcl::ResamplePlaneData, _max_nbr_planes> PlaneDataArray;

	const ::VSFormat &
	               get_output_colorspace (const ::VSMap &in, ::VSMap &out, ::VSCore &core, const ::VSFormat &fmt_src) const;
	bool           cumulate_flag (bool flag, const ::VSMap &in, ::VSMap &out, const char name_0 [], int pos = 0) const;
	void           get_interlacing_param (bool &itl_flag, bool &top_flag, int field_index, const ::VSFrameRef &src, InterlacingParam interlaced, FieldOrder field_order) const;
	int            process_plane_proc (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr);
	int            process_plane_copy (::VSFrameRef &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const vsutl::NodeRefSPtr &src_node1_sptr);
	fmtcl::FilterResize *
	               create_or_access_plane_filter (int plane_index, fmtcl::InterlacingType itl_d, fmtcl::InterlacingType itl_s);
	void           create_all_plane_specs ();

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.

	std::vector <int>             // Not used at the moment. Will be useful to specify the planes to process/copy/trash.
	               _plane_arr;

	int            _src_width;
	int            _src_height;
	fmtcl::SplFmt  _src_type;
	int            _src_res;
	fmtcl::SplFmt  _dst_type;
	int            _dst_res;
	double         _norm_val_h;
	double         _norm_val_v;
	InterlacingParam
	               _interlaced_src;
	InterlacingParam
	               _interlaced_dst;
	FieldOrder     _field_order_src;
	FieldOrder     _field_order_dst;
	bool           _int_flag;
	bool           _norm_flag;
	bool           _range_set_in_flag;
	bool           _range_set_out_flag;
	bool           _full_range_in_flag;
	bool           _full_range_out_flag;
	bool           _cplace_d_set_flag;
	fmtcl::ChromaPlacement
	               _cplace_s;
	fmtcl::ChromaPlacement
	               _cplace_d;

	bool           _sse2_flag;
	bool           _avx2_flag;
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
