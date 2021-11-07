/*****************************************************************************

        PlaneProcessor.cpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/PlaneProcCbInterface.h"
#include "vsutl/PlaneProcessor.h"
#include "vsutl/PlaneProcMode.h"
#include "vswrap.h"

#include <algorithm>

#include <cassert>
#include <cstdint>
#include <cstring>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Use manual mode for:
// - time-varying formats,
// - format conversions,
// - non 1:1 frame index mapping between input and output
PlaneProcessor::PlaneProcessor (const ::VSAPI &vsapi, PlaneProcCbInterface &cb, const char filter_name_0 [], bool manual_flag)
:	_vsapi (vsapi)
,	_filter_name (filter_name_0)
,	_cb (cb)
,	_vi_out ()
,	_nbr_planes (0)
/*,	_proc_mode_arr ()*/
,	_manual_flag (manual_flag)
,	_input_flag (false)
,	_blank_frame_sptr ()
{
	assert (filter_name_0 != 0);
}



// Don't forget to check "out" to see if an error occured.
void	PlaneProcessor::set_filter (const ::VSMap &in, ::VSMap &out, const ::VSVideoInfo &vi_out, bool simple_flag, int max_def_planes, const char *prop_name_0, const char *clip_name_0)
{
	fstb::unused (clip_name_0);
	assert (max_def_planes > 0);
	assert (prop_name_0 != 0);
	assert (clip_name_0 != 0);

	int            err = 0;

	_vi_out     = vi_out;

	_nbr_planes = vi_out.format->numPlanes;
	assert (_nbr_planes <= MAX_NBR_PLANES);
	const bool     int_flag = (vi_out.format->sampleType == ::stInteger);
	double         max_val = 0;
	if (int_flag)
	{
		max_val = double ((uint64_t (1)) << (vi_out.format->bitsPerSample - 4)) * 16;
	}

	const int      nbr_def_planes = std::max (_nbr_planes, max_def_planes);
	const int      nbr_elt        = _vsapi.propNumElements (&in, prop_name_0);
	const bool     arg_def_flag   = (nbr_elt >= 0);

	// Default
	// - Normal mode: Process 0 to max_def_planes - 1
	// - Simple mode:
	//    - All planes are garbage if the argument is specified,
	//    - Process 0 to max_def_planes - 1 if not.
	for (int p = 0; p < MAX_NBR_PLANES; ++p)
	{
		_proc_mode_arr [p] =
			  (p < nbr_def_planes && (! simple_flag || ! arg_def_flag))
			? PlaneProcMode_PROCESS
			: PlaneProcMode_GARBAGE;
	}

	bool           ok_flag = true;
	if (! arg_def_flag)
	{
		_input_flag = (max_def_planes > 0);
	}
	else
	{
		for (int index = 0; index < nbr_elt && ok_flag; ++index)
		{
			// Property could be declared as float or int
			double         plane_content =
				_vsapi.propGetFloat (&in, prop_name_0, index, &err);
			if (err == ::peType)
			{
				plane_content = double (
					_vsapi.propGetInt (&in, prop_name_0, index, &err)
				);
			}

			if (err != 0)
			{
				const std::string err_msg =
					  _filter_name
					+ ": cannot read the \""
					+ prop_name_0
					+ "\" parameter.";
				_vsapi.setError (&out, err_msg.c_str ());
				ok_flag = false;
			}

			// Simple mode
			if (ok_flag)
			{
				if (simple_flag)
				{
					const int      plane_index = int (plane_content + 0.5);
					if (plane_index < 0 || plane_index >= _nbr_planes)
					{
						const std::string err_msg =
							_filter_name + ": plane index out of range.";
						_vsapi.setError (&out, err_msg.c_str ());
						ok_flag = false;
					}
					else if (   fstb::round_int (_proc_mode_arr [plane_index])
					         == PlaneProcMode_PROCESS)
					{
						const std::string err_msg =
							_filter_name + ": plane specified twice.";
						_vsapi.setError (&out, err_msg.c_str ());
						ok_flag = false;
					}
					else
					{
						_proc_mode_arr [plane_index] = PlaneProcMode_PROCESS;
						_input_flag = true;
					}
				}

				// Masktools' mode
				else
				{
					if (index >= MAX_NBR_PLANES)
					{
						const std::string err_msg =
							_filter_name + ": too many specified plane filters.";
						_vsapi.setError (&out, err_msg.c_str ());
						ok_flag = false;
					}
					else if (   plane_content >= double (PlaneProcMode_NBR_ELT)
								|| (int_flag && -plane_content >= max_val))
					{
						const std::string err_msg =
							_filter_name + ": invalid plane filter.";
						_vsapi.setError (&out, err_msg.c_str ());
						ok_flag = false;
					}
					else
					{
						if (plane_content >= 1 || int_flag)
						{
							const int      pc_int = int (plane_content + 0.5);
							if (pc_int != PlaneProcMode_GARBAGE)
							{
								_input_flag = true;
							}
							plane_content = pc_int;
						}
						_proc_mode_arr [index] = plane_content;
					}
				}
			}
		}
	}
}



// To be called in arInitial mode, but not in manual mode
// Returns 0 if input frames are needed.
const ::VSFrameRef *	PlaneProcessor::try_initial (::VSCore &core)
{
	assert (! _manual_flag);

	const ::VSFrameRef * dst_ptr = 0;

	if (! _input_flag)
	{
		if (_blank_frame_sptr.get () == 0)
		{
			_blank_frame_sptr = FrameRefSPtr (
				_vsapi.newVideoFrame (
					_vi_out.format,
					_vi_out.width,
					_vi_out.height,
					0,
					&core
				),
				_vsapi
			);

			for (int plane_index = 0; plane_index < _nbr_planes; ++plane_index)
			{
				const double		val = _proc_mode_arr [plane_index];
				if (val < double (PlaneProcMode_COPY1))
				{
					fill_plane (
						const_cast < ::VSFrameRef &> (*_blank_frame_sptr),
						-val,
						plane_index
					);
				}
			}
		}

		dst_ptr = _blank_frame_sptr.dup ();
	}

	return (dst_ptr);
}



// To be called in arAllFramesReady mode
// In manual mode, all planes are called for processing.
int	PlaneProcessor::process_frame (::VSFrameRef &dst, int n, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, NodeRefSPtr src_node1_sptr, NodeRefSPtr src_node2_sptr, NodeRefSPtr src_node3_sptr)
{
	assert (n >= 0);
	assert (_input_flag);

	int            ret_val = 0;

	for (int plane_index = 0
	;	plane_index < _nbr_planes && ret_val == 0
	;	++plane_index)
	{
		const double   mode   = _proc_mode_arr [plane_index];
		const int      mode_i = fstb::round_int (mode);

		if (_manual_flag || mode_i == PlaneProcMode_PROCESS)
		{
			ret_val = _cb.process_plane (
				dst,
				n,
				plane_index,
				frame_data_ptr,
				frame_ctx,
				core,
				src_node1_sptr,
				src_node2_sptr,
				src_node3_sptr
			);
		}
		else if (mode_i >= PlaneProcMode_COPY1 && mode_i <= PlaneProcMode_COPY3)
		{
			NodeRefSPtr    src_clip_sptr (
				  (mode_i == PlaneProcMode_COPY3) ? src_node3_sptr
				: (mode_i == PlaneProcMode_COPY2) ? src_node2_sptr
				:                                   src_node1_sptr);
			if (src_clip_sptr.get () != 0)
			{
				FrameRefSPtr   src_sptr (
					_vsapi.getFrameFilter (n, src_clip_sptr.get (), &frame_ctx),
					_vsapi
				);

				copy_plane (dst, *src_sptr, plane_index);
			}
		}
		else if (mode_i < PlaneProcMode_COPY1)
		{
			fill_plane (dst, -mode, plane_index);
		}
	}

	return (ret_val);
}



bool	PlaneProcessor::is_manual () const
{
	return (_manual_flag);
}



PlaneProcMode	PlaneProcessor::get_mode (int plane_index) const
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	return (static_cast <PlaneProcMode> (
		int (_proc_mode_arr [plane_index] + 0.5)
	));
}



double	PlaneProcessor::get_mode_val (int plane_index) const
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	return (_proc_mode_arr [plane_index]);
}



void	PlaneProcessor::fill_plane (::VSFrameRef &dst, double val, int plane_index)
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const int      dst_w = _vsapi.getFrameWidth (&dst, plane_index);
	const int      dst_h = _vsapi.getFrameHeight (&dst, plane_index);
	const int      dst_s = _vsapi.getStride (&dst, plane_index);
	const ::VSFormat *   dst_fmt_ptr = _vsapi.getFrameFormat (&dst);
	uint8_t *            dst_dta_ptr = _vsapi.getWritePtr (&dst, plane_index);
	const int      bps = dst_fmt_ptr->bytesPerSample;
	const int      st  = dst_fmt_ptr->sampleType;

	switch (bps)
	{
	case	1:
		assert (st == ::stInteger);
		fill_plane (dst_dta_ptr, uint8_t (val + 0.5), dst_s, dst_w, dst_h);
		break;
	case	2:
		assert (st == ::stInteger);
		fill_plane (dst_dta_ptr, uint16_t (val + 0.5), dst_s, dst_w, dst_h);
		break;
	case	4:
		if (st == ::stFloat)
		{
			fill_plane (dst_dta_ptr, float (val), dst_s, dst_w, dst_h);
		}
		else
		{
			fill_plane (dst_dta_ptr, uint32_t (val + 0.5), dst_s, dst_w, dst_h);
		}
		break;
	case	8:
		if (st == ::stFloat)
		{
			fill_plane (dst_dta_ptr, double (val), dst_s, dst_w, dst_h);
		}
		else
		{
			fill_plane (dst_dta_ptr, uint64_t (val + 0.5), dst_s, dst_w, dst_h);
		}
		break;
	default:
		assert (false);
		break;
	}
}



void	PlaneProcessor::copy_plane (::VSFrameRef &dst, const ::VSFrameRef &src, int plane_index)
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const int      dst_w = _vsapi.getFrameWidth (&dst, plane_index);
	const int      dst_h = _vsapi.getFrameHeight (&dst, plane_index);
	const int      dst_s = _vsapi.getStride (&dst, plane_index);
	const ::VSFormat *   dst_fmt_ptr = _vsapi.getFrameFormat (&dst);
	uint8_t *            dst_dta_ptr = _vsapi.getWritePtr (&dst, plane_index);

	const int      src_w = _vsapi.getFrameWidth (&src, plane_index);
	const int      src_h = _vsapi.getFrameHeight (&src, plane_index);
	const int      src_s = _vsapi.getStride (&src, plane_index);
	const uint8_t *      src_dta_ptr = _vsapi.getReadPtr (&src, plane_index);

	const int      w   = std::min (dst_w, src_w);
	const int      h   = std::min (dst_h, src_h);

	const int      bps = dst_fmt_ptr->bytesPerSample;
	const int      row_size = w * bps; // In bytes

	if (src_s == dst_s && dst_s - row_size < 16)
	{
		memcpy (dst_dta_ptr, src_dta_ptr, dst_s * h);
	}
	else
	{
		for (int y = 0; y < h; ++y)
		{
			memcpy (dst_dta_ptr, src_dta_ptr, row_size);
			dst_dta_ptr += dst_s;
			src_dta_ptr += src_s;
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class T>
void	PlaneProcessor::fill_plane (void *ptr, T val, int stride, int w, int h)
{
	assert (ptr != nullptr);
	assert (stride > 0);
	assert (w > 0);
	assert (h > 0);

	if (sizeof (val) == 1 && stride - w < 16)
	{
		memset (ptr, char (val), stride * h);
	}

	else
	{
		T *            data_ptr = reinterpret_cast <T *> (ptr);
		const int      stride_pix = stride / sizeof (val);
		for (int y = 0; y < h; ++y)
		{
			if (sizeof (val) == 1)
			{
				memset (data_ptr, char (val), w);
			}
			else
			{
				std::fill (data_ptr, data_ptr + w, val);
			}

			data_ptr += stride_pix;
		}
	}
}



}	// namespace vsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
