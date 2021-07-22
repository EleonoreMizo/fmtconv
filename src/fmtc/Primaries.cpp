/*****************************************************************************

        Primaries.cpp
        Author: Laurent de Soras, 2016

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

#include "fmtc/fnc.h"
#include "fmtc/Primaries.h"
#include "fmtcl/fnc.h"
#include "fmtcl/Mat3.h"
#include "fmtcl/PrimUtil.h"
#include "fstb/def.h"
#include "fstb/fnc.h"
#include "vsutl/CpuOpt.h"
#include "vsutl/FrameRefSPtr.h"

#include <cassert>
#include <cstdio>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Primaries::Primaries (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "primaries", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
,	_sse_flag (false)
,	_sse2_flag (false)
,	_avx_flag (false)
,	_avx2_flag (false)
,	_prim_s ()
,	_prim_d ()
,	_mat_main ()
,	_proc_uptr ()
{
	fstb::unused (user_data_ptr, core);

	vsutl::CpuOpt  cpu_opt (*this, in, out);
	_sse_flag  = cpu_opt.has_sse ();
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx_flag  = cpu_opt.has_avx ();
	_avx2_flag = cpu_opt.has_avx2 ();

	_proc_uptr = std::unique_ptr <fmtcl::MatrixProc> (new fmtcl::MatrixProc (
		_sse_flag, _sse2_flag, _avx_flag, _avx2_flag
	));

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	// Source colorspace
	const ::VSFormat &   fmt_src = *_vi_in.format;
	check_colorspace (fmt_src, "input");

	// Destination colorspace (currently the same as the source)
	const ::VSFormat &   fmt_dst = fmt_src;
	check_colorspace (fmt_dst, "output");

	// Output format is validated.
	_vi_out.format = &fmt_dst;

	// Primaries
	init (_prim_s, *this, in, out, "prims");
	init (_prim_s, *this, in, out, "rs", "gs", "bs", "ws");
	if (! _prim_s.is_ready ())
	{
		throw_inval_arg ("input primaries not set.");
	}

	_prim_d = _prim_s;
	init (_prim_d, *this, in, out, "primd");
	init (_prim_d, *this, in, out, "rd", "gd", "bd", "wd");
	assert (_prim_d.is_ready ());

	const fmtcl::Mat3 mat_conv =
		fmtcl::PrimUtil::compute_conversion_matrix (_prim_s, _prim_d);
	_mat_main.insert3 (mat_conv);
	_mat_main.clean3 (1);

	prepare_matrix_coef (
		*this, *_proc_uptr, _mat_main,
		fmt_dst, true,
		fmt_src, true
	);

	if (_vsapi.getError (&out) != 0)
	{
		throw -1;
	}
}



void	Primaries::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (in, out, core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Primaries::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	fstb::unused (frame_data_ptr);
	assert (n >= 0);

	::VSFrameRef *    dst_ptr = 0;
	::VSNodeRef &     node = *_clip_src_sptr;

	if (activation_reason == ::arInitial)
	{
		_vsapi.requestFrameFilter (n, &node, &frame_ctx);
	}

	else if (activation_reason == ::arAllFramesReady)
	{
		vsutl::FrameRefSPtr	src_sptr (
			_vsapi.getFrameFilter (n, &node, &frame_ctx),
			_vsapi
		);
		const ::VSFrameRef & src = *src_sptr;

		const int         w = _vsapi.getFrameWidth (&src, 0);
		const int         h = _vsapi.getFrameHeight (&src, 0);
		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h, &src, &core);

		uint8_t * const   dst_ptr_arr [fmtcl::MatrixProc::_nbr_planes] =
		{
			_vsapi.getWritePtr (dst_ptr, 0),
			_vsapi.getWritePtr (dst_ptr, 1),
			_vsapi.getWritePtr (dst_ptr, 2)
		};
		const int         dst_str_arr [fmtcl::MatrixProc::_nbr_planes] =
		{
			_vsapi.getStride (dst_ptr, 0),
			_vsapi.getStride (dst_ptr, 1),
			_vsapi.getStride (dst_ptr, 2)
		};
		const uint8_t * const
		                  src_ptr_arr [fmtcl::MatrixProc::_nbr_planes] =
		{
			_vsapi.getReadPtr (&src, 0),
			_vsapi.getReadPtr (&src, 1),
			_vsapi.getReadPtr (&src, 2)
		};
		const int         src_str_arr [fmtcl::MatrixProc::_nbr_planes] =
		{
			_vsapi.getStride (&src, 0),
			_vsapi.getStride (&src, 1),
			_vsapi.getStride (&src, 2)
		};

		_proc_uptr->process (
			dst_ptr_arr, dst_str_arr,
			src_ptr_arr, src_str_arr,
			w, h
		);

		// Output properties
		::VSMap &      dst_prop = *(_vsapi.getFramePropsRW (dst_ptr));

		const fmtcl::PrimariesPreset  preset_d = _prim_d._preset;
		if (preset_d >= 0 && preset_d < fmtcl::PrimariesPreset_NBR_ELT)
		{
			_vsapi.propSetInt (&dst_prop, "_Primaries", int (preset_d), ::paReplace);
		}
		else
		{
			_vsapi.propDeleteKey (&dst_prop, "_Primaries");
		}
	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	Primaries::_nbr_planes;



void	Primaries::check_colorspace (const ::VSFormat &fmt, const char *inout_0) const
{
	assert (inout_0 != 0);

	if (fmt.subSamplingW != 0 || fmt.subSamplingH != 0)
	{
		fstb::snprintf4all (
			_filter_error_msg_0,
			_max_error_buf_len,
			"%s must be 4:4:4.",
			inout_0
		);
		throw_inval_arg (_filter_error_msg_0);
	}
	if (fmt.colorFamily != ::cmRGB)
	{
		fstb::snprintf4all (
			_filter_error_msg_0,
			_max_error_buf_len,
			"%s colorspace must be RGB (assumed linear).",
			inout_0
		);
		throw_inval_arg (_filter_error_msg_0);
	}
	if (   (fmt.sampleType == ::stInteger && fmt.bitsPerSample != 16)
	    || (fmt.sampleType == ::stFloat   && fmt.bitsPerSample != 32))
	{
		fstb::snprintf4all (
			_filter_error_msg_0,
			_max_error_buf_len,
			"pixel bitdepth not supported, "
			"%s must be 16-bit integer or 32-bit float.",
			inout_0
		);
		throw_inval_arg (_filter_error_msg_0);
	}

	assert (fmt.numPlanes == _nbr_planes);
}



void	Primaries::init (fmtcl::RgbSystem &prim, const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *preset_0)
{
	assert (preset_0 != 0);

	std::string    preset_str = filter.get_arg_str (in, out, preset_0, "");
	fstb::conv_to_lower_case (preset_str);
	prim._preset = fmtcl::PrimUtil::conv_string_to_primaries (preset_str);
	if (prim._preset >= 0)
	{
		prim.set (prim._preset);
	}
}



void	Primaries::init (fmtcl::RgbSystem &prim, const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char r_0 [], const char g_0 [], const char b_0 [], const char w_0 [])
{
	assert (r_0 != 0);
	assert (g_0 != 0);
	assert (b_0 != 0);
	assert (w_0 != 0);

	const bool     ready_old_flag = prim.is_ready ();
	std::array <fmtcl::RgbSystem::Vec2, _nbr_planes> rgb_old = prim._rgb;
	fmtcl::RgbSystem::Vec2  w_old = prim._white;

	const char *   name_0_arr [_nbr_planes] = { r_0, g_0, b_0 };
	for (int k = 0; k < _nbr_planes; ++k)
	{
		prim._init_flag_arr [k] |=
			read_coord_tuple (prim._rgb [k], filter, in, out, name_0_arr [k]);
	}

	prim._init_flag_arr [_nbr_planes] |=
		read_coord_tuple (prim._white, filter, in, out, w_0);

	if (   ready_old_flag && prim.is_ready ()
	    && (rgb_old != prim._rgb || w_old != prim._white))
	{
		prim._preset = fmtcl::PrimariesPreset_UNDEF;
	}
}



bool	Primaries::read_coord_tuple (fmtcl::RgbSystem::Vec2 &c, const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *name_0)
{
	bool           set_flag = false;
	typedef std::vector <double> Vect;
	Vect           v_def;

	Vect           c_v = filter.get_arg_vflt (in, out, name_0, v_def);
	if (c_v.size () != 0)
	{
		if (c_v.size () != c.size ())
		{
			fstb::snprintf4all (
				filter._filter_error_msg_0,
				filter._max_error_buf_len,
				"%s: wrong number of coordinates (expected %d).",
				name_0,
				int (c.size ())
			);
			filter.throw_inval_arg (filter._filter_error_msg_0);
		}
		double            sum = 0;
		for (size_t k = 0; k < c_v.size (); ++k)
		{
			sum += c_v [k];
			c [k] = c_v [k];
		}
		if (c [1] == 0)
		{
			fstb::snprintf4all (
				filter._filter_error_msg_0,
				filter._max_error_buf_len,
				"%s: y coordinate cannot be 0.",
				name_0
			);
			filter.throw_inval_arg (filter._filter_error_msg_0);
		}

		set_flag = true;
	}

	return (set_flag);
}



}  // namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
