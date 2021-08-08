/*****************************************************************************

        PlaneProcessor.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (avsutl_PlaneProcessor_HEADER_INCLUDED)
#define avsutl_PlaneProcessor_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "avsutl/CsPlane.h"
#include "avsutl/PlaneProcMode.h"
#include "fstb/fnc.h"

#include <array>
#include <string>

#include <cstdint>



class IScriptEnvironment;
class PClip;
class PlaneProcCbInterface;
struct VideoInfo;

namespace avsutl
{



class PlaneProcessor
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _max_nbr_planes = CsPlane::_max_nbr_planes;
	static constexpr int _max_nbr_clips  = 1+3; // Index 0 = destination

	enum ClipType
	{
		ClipType_UNKNOWN = -1,

		ClipType_NORMAL = 0,
		ClipType_STACKED_16,
		ClipType_INTERLEAVED_16,
		ClipType_MSB,
		ClipType_LSB,

		ClipType_NBR_ELT
	};

	enum FmtChkFlag
	{
		FmtChkFlag_CS_TYPE    = 1 <<  0, // RGB/YUV
		FmtChkFlag_CS_LAYOUT  = 1 <<  1, // Planar/interleaved
		FmtChkFlag_CS_SUBSPL  = 1 <<  2, // Chroma subsampling
		FmtChkFlag_CS_FORMAT  = 1 <<  3, // Data format and bitdepth
		FmtChkFlag_CS_NBRCOMP = 1 <<  4, // Number of components (planar or not)

		FmtChkFlag_CS_ALL     = (1 << 8) - 1,

		FmtChkFlag_W          = 1 <<  8,
		FmtChkFlag_H          = 1 <<  9,
		FmtChkFlag_NBR_FRAMES = 1 << 10,

		FmtChkFlag_ALL        = -1
	};

	explicit       PlaneProcessor (const ::VideoInfo &vi, const ::VideoInfo &vi_src, PlaneProcCbInterface &cb);
	virtual        ~PlaneProcessor () = default;

	void           set_proc_mode (const std::array <float, _max_nbr_planes> &pm_arr);
	void           set_proc_mode (std::string pmode);
	void           set_clip_info (int index, ClipType type);
	int            get_nbr_planes () const;

	void           process_frame (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, ::PClip *src_1_ptr, ::PClip *src_2_ptr, ::PClip *src_3_ptr, void *ctx_ptr = 0);
	int            get_plane_id (int plane_index, bool src_flag);
	int            get_width (const ::PVideoFrame &frame_sptr, int plane_id, bool src_flag = false) const;
	int            get_height (const ::PVideoFrame &frame_sptr, int plane_id) const;
	int            get_height16 (const ::PVideoFrame &frame_sptr, int plane_id) const;

	static int     get_nbr_planes (const ::VideoInfo &vi);
	static int     get_bytes_per_component (const ::VideoInfo &vi);
	static int     get_min_w (const ::VideoInfo &vi);
	static int     get_min_h (const ::VideoInfo &vi, bool stack16_flag);
	static int     compute_plane_w (const ::VideoInfo &vi, int plane_index, int w);
	static int     compute_plane_h (const ::VideoInfo &vi, int plane_index, int h);
	static void    check_same_format (::IScriptEnvironment *env_ptr, const ::VideoInfo &vi, const ::PClip tst_sptr, const char *fnc_name_0, const char *arg_name_0, int flags = FmtChkFlag_ALL);
	static bool    check_stack16_width (const ::VideoInfo &vi, int width = -1);
	static bool    check_stack16_height (const ::VideoInfo &vi, int height = -1);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	void           fill (::PVideoFrame &dst_sptr, int n, int plane_id, ClipType type, float val);
	void           fill_frame_part (::PVideoFrame &dst_sptr, int n, int plane_id, float val, bool stacked_flag, int part);
	void           copy (::PVideoFrame &dst_sptr, int n, int plane_id, ClipType type_dst, ::PClip &src_clip, ClipType type_src, ::IScriptEnvironment &env);
	void           copy_n_to_n (::PVideoFrame &dst_sptr, ::PClip &src_clip, int n, int plane_id, ::IScriptEnvironment &env);
	void           copy_8_to_stack16 (::PVideoFrame &dst_sptr, ::PClip &src_clip, int n, int plane_id, ::IScriptEnvironment &env, int part);
	void           copy_stack16_to_8 (::PVideoFrame &dst_sptr, ::PClip &src_clip, int n, int plane_id, ::IScriptEnvironment &env, int part);

	static bool    have_same_height (ClipType t1, ClipType t2);
	static bool    is_stacked (ClipType type);

	const ::VideoInfo &
	               _vi;
	const ::VideoInfo &
	               _vi_src;
	PlaneProcCbInterface &
	               _cb;
	int            _nbr_planes = 0;
	std::array <float, _max_nbr_planes>
	               _proc_mode_arr { fstb::make_array <_max_nbr_planes> (
							float (PlaneProcMode_PROCESS)
	               ) };
	std::array <ClipType, _max_nbr_clips>
	               _clip_type_arr { fstb::make_array <_max_nbr_clips> (
							ClipType_UNKNOWN
	               ) };



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               PlaneProcessor ()                               = delete;
	               PlaneProcessor (const PlaneProcessor &other)    = delete;
	               PlaneProcessor (PlaneProcessor &&other)         = delete;
	PlaneProcessor &
	               operator = (const PlaneProcessor &other)        = delete;
	PlaneProcessor &
	               operator = (PlaneProcessor &&other)             = delete;
	bool           operator == (const PlaneProcessor &other) const = delete;
	bool           operator != (const PlaneProcessor &other) const = delete;

}; // class PlaneProcessor



}  // namespace avsutl



//#include "avsutl/PlaneProcessor.hpp"



#endif   // avsutl_PlaneProcessor_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
