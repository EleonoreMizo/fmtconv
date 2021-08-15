/*****************************************************************************

        Resample.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_Resample_HEADER_INCLUDED)
#define fmtcavs_Resample_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcavs/FmtAvs.h"
#include "fmtcl/ChromaPlacement.h"
#include "fmtcl/FilterResize.h"
#include "fmtcl/InterlacingType.h"
#include "fmtcl/KernelData.h"
#include "fmtcl/ResamplePlaneData.h"
#include "fmtcl/ResampleSpecPlane.h"
#include "fmtcl/ResampleUtil.h"
#include "avsutl/PlaneProcCbInterface.h"
#include "avsutl/PlaneProcessor.h"
#include "avsutl/VideoFilterBase.h"

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <string>



namespace fmtcavs
{



class Resample
:	public avsutl::VideoFilterBase
,	public avsutl::PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef avsutl::VideoFilterBase Inherited;

	enum Param
	{
		Param_CLIP_SRC = 0, // 0
		Param_W,
		Param_H,
		Param_SX,
		Param_SY,
		Param_SW,
		Param_SH,
		Param_SCALE,
		Param_SCALEH,
		Param_SCALEV,
		Param_KERNEL, // 10
		Param_KERNELH,
		Param_KERNELV,
		Param_IMPULSE,
		Param_IMPULSEH,
		Param_IMPULSEV,
		Param_TAPS,
		Param_TAPSH,
		Param_TAPSV,
		Param_A1,
		Param_A2, // 20
		Param_A3,
		Param_A1H,
		Param_A2H,
		Param_A3H,
		Param_A1V,
		Param_A2V,
		Param_A3V,
		Param_KOVRSPL,
		Param_FH,
		Param_FV, // 30
		Param_CNORM,
		Param_TOTAL,
		Param_TOTALH,
		Param_TOTALV,
		Param_INVKS,
		Param_INVKSH,
		Param_INVKSV,
		Param_INVKSTAPS,
		Param_INVKSTAPSH,
		Param_INVKSTAPSV, // 40
		Param_CSP,
		Param_CSS,
		Param_PLANES,
		Param_FULLS,
		Param_FULLD,
		Param_CENTER,
		Param_CPLACE,
		Param_CPLACES,
		Param_CPLACED,
		Param_INTERLACED, // 50
		Param_INTERLACEDD,
		Param_TFF,
		Param_TFFD,
		Param_FLT,
		Param_CPUOPT,

		Param_NBR_ELT,
	};

	explicit       Resample (::IScriptEnvironment &env, const ::AVSValue &args);
	virtual        ~Resample () = default;

	// VideoFilterBase
	::PVideoFrame __stdcall
						GetFrame (int n, ::IScriptEnvironment *env_ptr) override;

	static fmtcl::ChromaPlacement
	               conv_str_to_chroma_placement (::IScriptEnvironment &env, std::string cplace);
	static void    conv_str_to_chroma_subspl (::IScriptEnvironment &env, int &ssh, int &ssv, std::string css);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// PlaneProcCbInterface
	void           do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr) override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	using Ru = fmtcl::ResampleUtil;

	static constexpr int _max_nbr_planes = 4;

	typedef std::array <fmtcl::ResamplePlaneData, _max_nbr_planes> PlaneDataArray;

	FmtAvs         get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src);
	void           process_plane_proc (::PVideoFrame &dst_sptr, ::IScriptEnvironment &env, int n, int plane_index, const Ru::FrameInfo &frame_info);
	void           process_plane_copy (::PVideoFrame &dst_sptr, ::IScriptEnvironment &env, int n, int plane_index);
	fmtcl::FilterResize *
	               create_or_access_plane_filter (int plane_index, fmtcl::InterlacingType itl_d, fmtcl::InterlacingType itl_s);
	void           create_all_plane_specs (const FmtAvs &fmt_dst, const FmtAvs &fmt_src);

	::PClip        _clip_src_sptr;
	const ::VideoInfo
	               _vi_src;

	int            _src_width  = 0;
	int            _src_height = 0;
	FmtAvs         _fmt_src;
	fmtcl::SplFmt  _src_type   = fmtcl::SplFmt_ILLEGAL;
	int            _src_res    = 0;
	FmtAvs         _fmt_dst;
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
	bool           _range_s_def_flag  = false;
	bool           _range_d_def_flag  = false;
	bool           _fulls_flag = false;
	bool           _fulld_flag = false;
	bool           _cplace_d_set_flag = false;
	fmtcl::ChromaPlacement
	               _cplace_s   = fmtcl::ChromaPlacement_MPEG2;
	fmtcl::ChromaPlacement
	               _cplace_d   = fmtcl::ChromaPlacement_MPEG2;

	bool           _sse2_flag  = false;
	bool           _avx2_flag  = false;

	std::mutex     _filter_mutex;          // To access _filter_uptr_map.
	std::map <fmtcl::ResampleSpecPlane, std::unique_ptr <fmtcl::FilterResize> >
	               _filter_uptr_map;       // Created only on request.

	PlaneDataArray _plane_data_arr;

	std::unique_ptr <avsutl::PlaneProcessor>
	               _plane_proc_uptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Resample ()                               = delete;
	               Resample (const Resample &other)          = delete;
	               Resample (Resample &&other)               = delete;
	Resample &     operator = (const Resample &other)        = delete;
	Resample &     operator = (Resample &&other)             = delete;
	bool           operator == (const Resample &other) const = delete;
	bool           operator != (const Resample &other) const = delete;

}; // class Resample



}  // namespace fmtcavs



//#include "fmtcavs/Resample.hpp"



#endif   // fmtcavs_Resample_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
