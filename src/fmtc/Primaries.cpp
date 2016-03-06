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
#include "fmtcl/Mat3.h"
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
	assert (&in != 0);
	assert (&out != 0);
	assert (&core != 0);
	assert (&vsapi != 0);

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
	_prim_s.init (*this, in, out, "prims");
	_prim_d.init (*this, in, out, "primd");
	_prim_s.init (*this, in, out, "rs", "gs", "bs", "ws");
	_prim_d.init (*this, in, out, "rd", "gd", "bd", "wd");
	if (! _prim_s.is_ready ())
	{
		throw_inval_arg ("input primaries not set.");
	}
	if (! _prim_d.is_ready ())
	{
		throw_inval_arg ("input primaries not set.");
	}

	const fmtcl::Mat3 mat_conv = compute_conversion_matrix ();
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
	assert (&in != 0);
	assert (&out != 0);
	assert (&node != 0);
	assert (&core != 0);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	Primaries::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	assert (n >= 0);
	assert (&frame_data_ptr != 0);
	assert (&frame_ctx != 0);
	assert (&core != 0);

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

		uint8_t * const   dst_ptr_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			_vsapi.getWritePtr (dst_ptr, 0),
			_vsapi.getWritePtr (dst_ptr, 1),
			_vsapi.getWritePtr (dst_ptr, 2)
		};
		const int         dst_str_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			_vsapi.getStride (dst_ptr, 0),
			_vsapi.getStride (dst_ptr, 1),
			_vsapi.getStride (dst_ptr, 2)
		};
		const uint8_t * const
		                  src_ptr_arr [fmtcl::MatrixProc::NBR_PLANES] =
		{
			_vsapi.getReadPtr (&src, 0),
			_vsapi.getReadPtr (&src, 1),
			_vsapi.getReadPtr (&src, 2)
		};
		const int         src_str_arr [fmtcl::MatrixProc::NBR_PLANES] =
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
	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Primaries::Vec2::Vec2 (double c0, double c1)
:	Inherited ({ { c0, c1 } })
{
	// Nothing
}



Primaries::RGBSystem::RGBSystem ()
:	_rgb ()
,	_white ()
,	_init_flag_arr ({ {false, false, false, false } })
{
	// Nothing
}



void	Primaries::RGBSystem::init (const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *preset_0)
{
	assert (&filter != 0);
	assert (&in != 0);
	assert (&out != 0);
	assert (preset_0 != 0);

	std::string    preset = filter.get_arg_str (in, out, preset_0, "");
	if (! preset.empty ())
	{
		fstb::conv_to_lower_case (preset);

		if (        preset == "709"
		         || preset == "1361"
		         || preset == "61966-2-1"
		         || preset == "61966-2-4")
		{
			_rgb [0] = { 0.640 , 0.330  };
			_rgb [1] = { 0.300 , 0.600  };
			_rgb [2] = { 0.150 , 0.060  };
			_white   = { 0.3127, 0.3290 };
		}
		else if (   preset == "470m")
		{
			_rgb [0] = { 0.670 , 0.330  };
			_rgb [1] = { 0.210 , 0.710  };
			_rgb [2] = { 0.140 , 0.080  };
			_white   = { 0.3100, 0.3160 };
		}
		else if (   preset == "470m93")
		{
			_rgb [0] = { 0.670 , 0.330  };
			_rgb [1] = { 0.210 , 0.710  };
			_rgb [2] = { 0.140 , 0.080  };
			_white   = { 0.2848, 0.2932 };
		}
		else if (   preset == "470bg"
		         || preset == "601-625"
		         || preset == "1358-625"
		         || preset == "1700-625")
		{
			_rgb [0] = { 0.640 , 0.330  };
			_rgb [1] = { 0.290 , 0.600  };
			_rgb [2] = { 0.150 , 0.060  };
			_white   = { 0.3127, 0.3290 };
		}
		else if (   preset == "170m"
		         || preset == "240m"
		         || preset == "601-525"
		         || preset == "1358-525"
		         || preset == "1700-525")
		{
			_rgb [0] = { 0.630 , 0.340  };
			_rgb [1] = { 0.310 , 0.595  };
			_rgb [2] = { 0.155 , 0.070  };
			_white   = { 0.3127, 0.3290 };
		}
		else if (   preset == "gf")
		{
			_rgb [0] = { 0.681 , 0.319  };
			_rgb [1] = { 0.243 , 0.692  };
			_rgb [2] = { 0.145 , 0.049  };
			_white   = { 0.3100, 0.3160 };
		}
		else if (   preset == "2020")
		{
			_rgb [0] = { 0.708 , 0.292  };
			_rgb [1] = { 0.170 , 0.797  };
			_rgb [2] = { 0.131 , 0.046  };
			_white   = { 0.3127, 0.3290 };
		}
		else
		{
			fstb::snprintf4all (
				filter._filter_error_msg_0,
				filter._max_error_buf_len,
				"%s: unknown preset \"%s\".",
				preset_0,
				preset.c_str ()
			);
			filter.throw_inval_arg (filter._filter_error_msg_0);
		}

		for (bool &init_flag : _init_flag_arr)
		{
			init_flag = true;
		}
	}
}



void	Primaries::RGBSystem::init (const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char r_0 [], const char g_0 [], const char b_0 [], const char w_0 [])
{
	assert (&filter != 0);
	assert (&in != 0);
	assert (&out != 0);
	assert (r_0 != 0);
	assert (g_0 != 0);
	assert (b_0 != 0);
	assert (w_0 != 0);

	const char *   name_0_arr [NBR_PLANES] = { r_0, g_0, b_0 };
	for (int k = 0; k < NBR_PLANES; ++k)
	{
		_init_flag_arr [k] |=
			read_coord_tuple (_rgb [k], filter, in, out, name_0_arr [k]);
	}

	_init_flag_arr [NBR_PLANES] |=
		read_coord_tuple (_white, filter, in, out, w_0);
}



bool	Primaries::RGBSystem::is_ready () const
{
	for (bool init_flag : _init_flag_arr)
	{
		if (! init_flag)
		{
			return false;
		}
	}
	return (true);
}



bool	Primaries::RGBSystem::read_coord_tuple (Vec2 &c, const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *name_0)
{
	bool           set_flag = false;
	typedef std::vector <double> Vect;
	Vect           v_def;

	Vect           c_v =
		filter.get_arg_vflt (in, out, name_0, v_def);
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
			if (c_v [k] < 0 || c_v [k] > 1)
			{
				fstb::snprintf4all (
					filter._filter_error_msg_0,
					filter._max_error_buf_len,
					"%s: (x, y) coordinates must be in range 0-1.",
					name_0
				);
				filter.throw_inval_arg (filter._filter_error_msg_0);
			}
			sum += c_v [k];
			c [k] = c_v [k];
		}
		if (c [1] == 0)
		{
			fstb::snprintf4all (
				filter._filter_error_msg_0,
				filter._max_error_buf_len,
				"%s: y coordinates must be greater than 0.",
				name_0
			);
			filter.throw_inval_arg (filter._filter_error_msg_0);
		}
		else if (sum > 1)
		{
			fstb::snprintf4all (
				filter._filter_error_msg_0,
				filter._max_error_buf_len,
				"%s: sum of (x, y) coordinates must be in range 0-1.",
				name_0
			);
			filter.throw_inval_arg (filter._filter_error_msg_0);
		}

		set_flag = true;
	}

	return (set_flag);
}



void	Primaries::check_colorspace (const ::VSFormat &fmt, const char *inout_0) const
{
	assert (&fmt != 0);
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

	assert (fmt.numPlanes == NBR_PLANES);
}



fmtcl::Mat3	Primaries::compute_conversion_matrix () const
{
	fmtcl::Mat3    rgb2xyz = compute_rgb2xyz (_prim_s);
	fmtcl::Mat3    xyz2rgb = compute_rgb2xyz (_prim_d).invert ();
	fmtcl::Mat3    adapt   = compute_chroma_adapt (_prim_s, _prim_d);

	return rgb2xyz * adapt * xyz2rgb;
}



// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
fmtcl::Mat3	Primaries::compute_rgb2xyz (const RGBSystem &prim)
{
	const fmtcl::Vec3 white = conv_xy_to_xyz (prim._white);

	fmtcl::Mat3    xyzrgb;
	for (int k = 0; k < NBR_PLANES; ++k)
	{
		fmtcl::Vec3    comp_xyz = conv_xy_to_xyz (prim._rgb [k]);
		xyzrgb.set_col (k, comp_xyz);
	}

	fmtcl::Vec3    s = xyzrgb.compute_inverse () * white;

	fmtcl::Mat3    m;
	for (int u = 0; u < NBR_PLANES; ++u)
	{
		m.set_col (u, xyzrgb.get_col (u) * s [u]);
	}

	return m;
}



// http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
fmtcl::Mat3	Primaries::compute_chroma_adapt (const RGBSystem &prim_s, const RGBSystem &prim_d)
{
	fmtcl::Vec3    white_s = conv_xy_to_xyz (prim_s._white);
	fmtcl::Vec3    white_d = conv_xy_to_xyz (prim_d._white);

	//	Bradford adaptation
	const fmtcl::Mat3 ma (
	{
		fmtcl::Vec3 ( 0.8951,  0.2664, -0.1614),
		fmtcl::Vec3 (-0.7502,  1.7135,  0.0367),
		fmtcl::Vec3 ( 0.0389, -0.0685,  1.0296)
	});

	fmtcl::Vec3    crd_s = ma * white_s;
	fmtcl::Vec3    crd_d = ma * white_d;
	fmtcl::Mat3    scale (0.0);
	for (int k = 0; k < NBR_PLANES; ++k)
	{
		assert (crd_s [k] != 0);
		scale [k] [k] = crd_d [k] / crd_s [k];
	}

	return ma.compute_inverse () * scale * ma;
}



// Obtains X, Y, Z from (x, y)
// Y is assumed to be 1.0
// X =      x      / y
// Z = (1 - x - y) / y
fmtcl::Vec3	Primaries::conv_xy_to_xyz (const Vec2 &xy)
{
	fmtcl::Vec3    xyz;

	xyz [0] =      xy [0]           / xy [1];
	xyz [1] = 1;
	xyz [2] = (1 - xy [0] - xy [1]) / xy [1];

	return xyz;
}



}  // namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
