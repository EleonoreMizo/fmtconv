/*****************************************************************************

        Resample.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcavs/CpuOpt.h"
#include "fmtcavs/fnc.h"
#include "fmtcavs/function_names.h"
#include "fmtcavs/Resample.h"
#include "fmtcl/BitBltConv.h"
#include "fmtcl/fnc.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Resample::Resample (::IScriptEnvironment &env, const ::AVSValue &args)
:	Inherited (env, args [Param_CLIP_SRC].AsClip ())
,	_clip_src_sptr (args [Param_CLIP_SRC].AsClip ())
,	_vi_src (vi)
,	_interlaced_src (static_cast <Ru::InterlacingParam> (
		args [Param_INTERLACED].AsInt (Ru::InterlacingParam_AUTO)
	))
,	_interlaced_dst (static_cast <Ru::InterlacingParam> (
		args [Param_INTERLACEDD].AsInt (_interlaced_src)
	))
,	_field_order_src (static_cast <Ru::FieldOrder> (
		args [Param_TFF].AsInt (Ru::FieldOrder_AUTO)
	))
,	_field_order_dst (static_cast <Ru::FieldOrder> (
		args [Param_TFFD].AsInt (_field_order_src)
	))
,	_int_flag (! args [Param_FLT].AsBool (false))
,	_norm_flag (args [Param_CNORM].AsBool (true))
{
	const CpuOpt   cpu_opt (args [Param_CPUOPT]);
	_sse2_flag = cpu_opt.has_sse2 ();
	_avx2_flag = cpu_opt.has_avx2 ();

	// Checks the input clip
	if (! _vi_src.IsPlanar ())
	{
		env.ThrowError (fmtcavs_RESAMPLE ": input must be planar.");
	}

	_fmt_src.conv_from_vi (_vi_src);
	const bool     src_flt_flag = _fmt_src.is_float ();
	const int      nbr_planes_src = _vi_src.NumComponents ();
	_src_res = _fmt_src.get_bitdepth ();
	if (! (   (! src_flt_flag && (   _src_res ==  8
	                              || _src_res == 10
	                              || _src_res == 12
	                              || _src_res == 14
	                              || _src_res == 16))
	       || (  src_flt_flag &&     _src_res == 32 )))
	{
		env.ThrowError (
			fmtcavs_RESAMPLE ": input pixel bitdepth not supported."
		);
	}

	_src_width  = _vi_src.width;
	_src_height = _vi_src.height;
	_src_type   = conv_vi_to_splfmt (_vi_src);

	// Destination colorspace
	_fmt_dst = _fmt_src;
	if (! src_flt_flag && _src_res < 16)
	{
		_fmt_dst.set_bitdepth (16);
	}
	_fmt_dst = get_output_colorspace (env, args, _fmt_dst);

	// Checks the provided format
	const bool     dst_flt_flag   = _fmt_dst.is_float ();
	const int      nbr_planes_dst = _vi_src.NumComponents ();
	_dst_res = _fmt_dst.get_bitdepth ();
	if (_dst_res < 16)
	{
		env.ThrowError (fmtcavs_RESAMPLE
			": cannot output 8-, 12- or 12-bit data directly. "
			"Output to 16 bits then dither with " fmtcavs_BITDEPTH "."
		);
	}
	if (! (   (! dst_flt_flag && _dst_res == 16)
	       || (  dst_flt_flag && _dst_res == 32)))
	{
		env.ThrowError (
			fmtcavs_RESAMPLE ": specified output pixel bitdepth not supported."
		);
	}
	if (   _fmt_dst.get_col_fam () != _fmt_src.get_col_fam ()
	    || nbr_planes_dst          != nbr_planes_src)
	{
		env.ThrowError (fmtcavs_RESAMPLE
			": specified output colorspace is not compatible with input."
		);
	}

	// Done with the format
	if (_fmt_dst.conv_to_vi (vi) != 0)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": invalid output format.");
	}
	_dst_type = conv_vi_to_splfmt (vi);

	if (   _interlaced_src < 0
	    || _interlaced_src >= Ru::InterlacingParam_NBR_ELT)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": interlaced argument out of range.");
	}
	if (   _interlaced_dst < 0
	    || _interlaced_dst >= Ru::InterlacingParam_NBR_ELT)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": interlacedd argument out of range.");
	}
	if (   _field_order_src < 0
	    || _field_order_src >= Ru::FieldOrder_NBR_ELT)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": tff argument out of range.");
	}
	if (   _field_order_dst < 0
	    || _field_order_dst >= Ru::FieldOrder_NBR_ELT)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": tffd argument out of range.");
	}

	// Range
	_range_s_def_flag = args [Param_FULLS].Defined ();
	_fulls_flag       = args [Param_FULLS].AsBool (
		fmtcl::is_full_range_default (_fmt_src.get_col_fam ())
	);
	_range_d_def_flag = args [Param_FULLD].Defined ();
	_fulld_flag       = args [Param_FULLD].AsBool (
		fmtcl::is_full_range_default (_fmt_dst.get_col_fam ())
	);

	// Configures the plane processor
	_plane_proc_uptr =
		std::make_unique <avsutl::PlaneProcessor> (vi, *this, true);
	_plane_proc_uptr->set_dst_clip_info (avsutl::PlaneProcessor::ClipType_NORMAL);
	_plane_proc_uptr->set_clip_info (
		avsutl::PlaneProcessor::ClipIdx_SRC1,
		_clip_src_sptr,
		avsutl::PlaneProcessor::ClipType_NORMAL
	);
	_plane_proc_uptr->set_proc_mode (args [Param_PLANES].AsString ("all"));

	const auto     src_picfmt = conv_fmtavs_to_picfmt (_fmt_src, _fulls_flag);
	const auto     dst_picfmt = conv_fmtavs_to_picfmt (_fmt_dst, _fulld_flag);
	for (int plane_index = 0; plane_index < nbr_planes_src; ++plane_index)
	{
		auto &         plane_data = _plane_data_arr [plane_index];
		fmtcl::compute_fmt_mac_cst (
			plane_data._gain, plane_data._add_cst,
			dst_picfmt, src_picfmt, plane_index
		);
	}

	// Target size: scale
	const double   scale  = args [Param_SCALE ].AsFloat (0);
	const double   scaleh = args [Param_SCALEH].AsFloat (float (scale));
	const double   scalev = args [Param_SCALEV].AsFloat (float (scale));
	if (scaleh < 0 || scalev < 0)
	{
		env.ThrowError (
			fmtcavs_RESAMPLE ": scale parameters must be positive or 0."
		);
	}
	const int      cssh = 1 << _fmt_dst.get_subspl_h ();
	if (scaleh > 0)
	{
		const int      wtmp = fstb::round_int (vi.width  * scaleh / cssh);
		vi.width = std::max (wtmp, 1) * cssh;
	}
	const int      cssv = 1 << _fmt_dst.get_subspl_v ();
	if (scalev > 0)
	{
		const int      htmp = fstb::round_int (vi.height * scalev / cssv);
		vi.height = std::max (htmp, 1) * cssv;
	}

	// Target size: explicit dimensions
	vi.width = args [Param_W].AsInt (vi.width);
	if (vi.width < 1)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": w must be positive.");
	}
	else if ((vi.width & (cssh - 1)) != 0)
	{
		env.ThrowError (fmtcavs_RESAMPLE
			": w is not compatible with the output chroma subsampling."
		);
	}

	vi.height = args [Param_H].AsInt (vi.height);
	if (vi.height < 1)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": h must be positive.");
	}
	else if ((vi.height & (cssv - 1)) != 0)
	{
		env.ThrowError (fmtcavs_RESAMPLE
			": h is not compatible with the output chroma subsampling."
		);
	}

	// Chroma placement
	const std::string cplace_str = args [Param_CPLACE].AsString ("mpeg2");
	_cplace_s = conv_str_to_chroma_placement (
		env, args [Param_CPLACES].AsString (cplace_str.c_str ())
	);
	_cplace_d_set_flag =  args [Param_CPLACES].Defined ();
	_cplace_d = conv_str_to_chroma_placement (
		env, args [Param_CPLACED].AsString (cplace_str.c_str ())
	);

	// Per-plane parameters
	const auto impulse   =
		extract_array_f (env, args [Param_IMPULSE ], fmtcavs_RESAMPLE ", impulse");
	auto impulse_h = args [Param_IMPULSEH].Defined ()
		? extract_array_f (env, args [Param_IMPULSEH], fmtcavs_RESAMPLE ", impulseh")
		: impulse;
	const auto impulse_v = args [Param_IMPULSEV].Defined ()
		? extract_array_f (env, args [Param_IMPULSEV], fmtcavs_RESAMPLE ", impulsev")
		: impulse;
	const bool a1_flag   = args [Param_A1 ].Defined ();
	const bool a2_flag   = args [Param_A2 ].Defined ();
	const bool a3_flag   = args [Param_A3 ].Defined ();
	const bool a1_h_flag = args [Param_A1H].Defined ();
	const bool a2_h_flag = args [Param_A2H].Defined ();
	const bool a3_h_flag = args [Param_A3H].Defined ();
	const bool a1_v_flag = args [Param_A1V].Defined ();
	const bool a2_v_flag = args [Param_A2V].Defined ();
	const bool a3_v_flag = args [Param_A3V].Defined ();
	const auto sx_arr         = extract_array_f (env, args [Param_SX        ], fmtcavs_RESAMPLE ", sx");
	const auto sy_arr         = extract_array_f (env, args [Param_SY        ], fmtcavs_RESAMPLE ", sy");
	const auto sw_arr         = extract_array_f (env, args [Param_SW        ], fmtcavs_RESAMPLE ", sw");
	const auto sh_arr         = extract_array_f (env, args [Param_SH        ], fmtcavs_RESAMPLE ", sh");
	const auto kernel_arr     = extract_array_s (env, args [Param_KERNEL    ], fmtcavs_RESAMPLE ", kernel");
	const auto kernelh_arr    = extract_array_s (env, args [Param_KERNELH   ], fmtcavs_RESAMPLE ", kernelh");
	const auto kernelv_arr    = extract_array_s (env, args [Param_KERNELV   ], fmtcavs_RESAMPLE ", kernelv");
	const auto kovrspl_arr    = extract_array_i (env, args [Param_KOVRSPL   ], fmtcavs_RESAMPLE ", koverspl");
	const auto taps_arr       = extract_array_i (env, args [Param_TAPS      ], fmtcavs_RESAMPLE ", taps");
	const auto tapsh_arr      = extract_array_i (env, args [Param_TAPSH     ], fmtcavs_RESAMPLE ", tapsh");
	const auto tapsv_arr      = extract_array_i (env, args [Param_TAPSV     ], fmtcavs_RESAMPLE ", tapsv");
	const auto a1_arr         = extract_array_f (env, args [Param_A1        ], fmtcavs_RESAMPLE ", a1");
	const auto a2_arr         = extract_array_f (env, args [Param_A2        ], fmtcavs_RESAMPLE ", a2");
	const auto a3_arr         = extract_array_f (env, args [Param_A3        ], fmtcavs_RESAMPLE ", a3");
	const auto a1h_arr        = extract_array_f (env, args [Param_A1H       ], fmtcavs_RESAMPLE ", a1h");
	const auto a2h_arr        = extract_array_f (env, args [Param_A2H       ], fmtcavs_RESAMPLE ", a2h");
	const auto a3h_arr        = extract_array_f (env, args [Param_A3H       ], fmtcavs_RESAMPLE ", a3h");
	const auto a1v_arr        = extract_array_f (env, args [Param_A1V       ], fmtcavs_RESAMPLE ", a1v");
	const auto a2v_arr        = extract_array_f (env, args [Param_A2V       ], fmtcavs_RESAMPLE ", a2v");
	const auto a3v_arr        = extract_array_f (env, args [Param_A3V       ], fmtcavs_RESAMPLE ", a3v");
	const auto total_arr      = extract_array_f (env, args [Param_TOTAL     ], fmtcavs_RESAMPLE ", total");
	const auto totalh_arr     = extract_array_f (env, args [Param_TOTALH    ], fmtcavs_RESAMPLE ", totalh");
	const auto totalv_arr     = extract_array_f (env, args [Param_TOTALV    ], fmtcavs_RESAMPLE ", totalv");
	const auto invks_arr      = extract_array_b (env, args [Param_INVKS     ], fmtcavs_RESAMPLE ", invks");
	const auto invksh_arr     = extract_array_b (env, args [Param_INVKSH    ], fmtcavs_RESAMPLE ", invksh");
	const auto invksv_arr     = extract_array_b (env, args [Param_INVKSV    ], fmtcavs_RESAMPLE ", invksv");
	const auto invkstaps_arr  = extract_array_i (env, args [Param_INVKSTAPS ], fmtcavs_RESAMPLE ", invkstaps");
	const auto invkstapsh_arr = extract_array_i (env, args [Param_INVKSTAPSH], fmtcavs_RESAMPLE ", invkstapsh");
	const auto invkstapsv_arr = extract_array_i (env, args [Param_INVKSTAPSV], fmtcavs_RESAMPLE ", invkstapsv");
	const auto fh_arr         = extract_array_f (env, args [Param_FH        ], fmtcavs_RESAMPLE ", fh");
	const auto fv_arr         = extract_array_f (env, args [Param_FV        ], fmtcavs_RESAMPLE ", fv");
	const auto center_arr     = extract_array_b (env, args [Param_CENTER    ], fmtcavs_RESAMPLE ", center");

	for (int plane_index = 0; plane_index < nbr_planes_src; ++plane_index)
	{
		auto &         plane_data = _plane_data_arr [plane_index];

		// Source window
		auto &         s = plane_data._win;
		if (plane_index > 0)
		{
			s = _plane_data_arr [plane_index - 1]._win;
		}
		else
		{
			s._x = 0;
			s._y = 0;
			s._w = 0;
			s._h = 0;
		}

		if (plane_index < int (sx_arr.size ())) { s._x = sx_arr [plane_index]; }
		if (plane_index < int (sy_arr.size ())) { s._y = sy_arr [plane_index]; }
		if (plane_index < int (sw_arr.size ())) { s._w = sw_arr [plane_index]; }
		if (plane_index < int (sh_arr.size ())) { s._h = sh_arr [plane_index]; }

		const double   eps = 1e-9;

		if (fstb::is_null (s._w, eps))
		{
			s._w = _src_width;
		}
		else if (s._w < 0)
		{
			s._w = _src_width + s._w - s._x;
			if (s._w <= eps)
			{
				env.ThrowError (fmtcavs_RESAMPLE ": sw must be positive.");
			}
		}

		if (fstb::is_null (s._h, eps))
		{
			s._h = _src_height;
		}
		else if (s._h < 0)
		{
			s._h = _src_height + s._h - s._y;
			if (s._h <= eps)
			{
				env.ThrowError (fmtcavs_RESAMPLE ": sh must be positive.");
			}
		}

		// Kernel
		std::string  kernel_fnc    = fmtcl::get_arr_elt (kernel_arr , plane_index, { "spline36" });
		std::string  kernel_fnc_h  = fmtcl::get_arr_elt (kernelh_arr, plane_index, kernel_fnc);
		std::string  kernel_fnc_v  = fmtcl::get_arr_elt (kernelv_arr, plane_index, kernel_fnc);
		const int    kovrspl       = fmtcl::get_arr_elt (kovrspl_arr, plane_index, 0   );
		const int    taps          = fmtcl::get_arr_elt (taps_arr   , plane_index, 4   );
		const int    taps_h        = fmtcl::get_arr_elt (tapsh_arr  , plane_index, taps);
		const int    taps_v        = fmtcl::get_arr_elt (tapsv_arr  , plane_index, taps);
		const double a1            = fmtcl::get_arr_elt (a1_arr     , plane_index, 0.0);
		const double a2            = fmtcl::get_arr_elt (a2_arr     , plane_index, 0.0);
		const double a3            = fmtcl::get_arr_elt (a3_arr     , plane_index, 0.0);
		const double a1_h          = fmtcl::get_arr_elt (a1h_arr    , plane_index, a1 );
		const double a2_h          = fmtcl::get_arr_elt (a2h_arr    , plane_index, a2 );
		const double a3_h          = fmtcl::get_arr_elt (a3h_arr    , plane_index, a3 );
		const double a1_v          = fmtcl::get_arr_elt (a1v_arr    , plane_index, a1 );
		const double a2_v          = fmtcl::get_arr_elt (a2v_arr    , plane_index, a2 );
		const double a3_v          = fmtcl::get_arr_elt (a3v_arr    , plane_index, a3 );
		const double total         = fmtcl::get_arr_elt (total_arr  , plane_index, 0.0);
		plane_data._norm_val_h     = fmtcl::get_arr_elt (totalh_arr , plane_index, total);
		plane_data._norm_val_v     = fmtcl::get_arr_elt (totalv_arr , plane_index, total);
		const bool   invks_flag    = fmtcl::get_arr_elt (invks_arr  , plane_index, false);
		const bool   invks_h_flag  = fmtcl::get_arr_elt (invksh_arr , plane_index, invks_flag);
		const bool   invks_v_flag  = fmtcl::get_arr_elt (invksv_arr , plane_index, invks_flag);
		const int    invks_taps    = fmtcl::get_arr_elt (invkstaps_arr , plane_index, 4         );
		const int    invks_taps_h  = fmtcl::get_arr_elt (invkstapsh_arr, plane_index, invks_taps);
		const int    invks_taps_v  = fmtcl::get_arr_elt (invkstapsv_arr, plane_index, invks_taps);
		plane_data._kernel_scale_h = fmtcl::get_arr_elt (fh_arr     , plane_index, 1.0);
		plane_data._kernel_scale_v = fmtcl::get_arr_elt (fv_arr     , plane_index, 1.0);
		plane_data._preserve_center_flag = fmtcl::get_arr_elt (center_arr, plane_index, true);
		if (fstb::is_null (plane_data._kernel_scale_h))
		{
			env.ThrowError (fmtcavs_RESAMPLE ": fh cannot be null.");
		}
		if (fstb::is_null (plane_data._kernel_scale_v))
		{
			env.ThrowError (fmtcavs_RESAMPLE ": fv cannot be null.");
		}
		if (   taps_h < 1 || taps_h > Ru::_max_nbr_taps
		    || taps_v < 1 || taps_v > Ru::_max_nbr_taps)
		{
			env.ThrowError (
				fmtcavs_RESAMPLE ": taps* must be in the 1-128 range."
			);
		}
		if (plane_data._norm_val_h < 0)
		{
			env.ThrowError (
				fmtcavs_RESAMPLE ": totalh must be positive or null."
			);
		}
		if (plane_data._norm_val_v < 0)
		{
			env.ThrowError (
				fmtcavs_RESAMPLE ": totalv must be positive or null."
			);
		}
		if (   invks_taps_h < 1 || invks_taps_h > Ru::_max_nbr_taps
		    || invks_taps_v < 1 || invks_taps_v > Ru::_max_nbr_taps)
		{
			env.ThrowError (
				fmtcavs_RESAMPLE ": invkstaps* must be in the 1-128 range."
			);
		}

		// Serious stuff now
		plane_data._kernel_arr [fmtcl::FilterResize::Dir_H].create_kernel (
			kernel_fnc_h, impulse_h, taps_h,
			(a1_flag || a1_h_flag), a1_h,
			(a2_flag || a2_h_flag), a2_h,
			(a3_flag || a3_h_flag), a3_h,
			kovrspl,
			invks_h_flag,
			invks_taps_h
		);

		plane_data._kernel_arr [fmtcl::FilterResize::Dir_V].create_kernel (
			kernel_fnc_v, impulse_v, taps_v,
			(a1_flag || a1_v_flag), a1_v,
			(a2_flag || a2_v_flag), a2_v,
			(a3_flag || a3_v_flag), a3_v,
			kovrspl,
			invks_v_flag,
			invks_taps_v
		);
	}

	create_all_plane_specs (_fmt_dst, _fmt_src);
}



::PVideoFrame __stdcall	Resample::GetFrame (int n, ::IScriptEnvironment *env_ptr)
{
	::PVideoFrame  src_sptr = _clip_src_sptr->GetFrame (n, env_ptr);
	::PVideoFrame	dst_sptr = build_new_frame (*env_ptr, vi, &src_sptr);

	Ru::FieldBased prop_fieldbased = Ru::FieldBased_INVALID;
	Ru::Field      prop_field      = Ru::Field_INVALID;
	if (supports_props ())
	{
		const ::AVSMap *  props_ptr = env_ptr->getFramePropsRO (src_sptr);
		int            err      = 0;
		int64_t        prop_val = -1;
		prop_val = env_ptr->propGetInt (props_ptr, "_FieldBased", 0, &err);
		prop_fieldbased =
				(err      != 0) ? Ru::FieldBased_INVALID
			: (prop_val == 0) ? Ru::FieldBased_FRAMES
			: (prop_val == 1) ? Ru::FieldBased_BFF
			: (prop_val == 2) ? Ru::FieldBased_TFF
			:                   Ru::FieldBased_INVALID;
		prop_val = env_ptr->propGetInt (props_ptr, "_Field", 0, &err);
		prop_field =
				(err      != 0) ? Ru::Field_INVALID
			: (prop_val == 0) ? Ru::Field_BOT
			: (prop_val == 1) ? Ru::Field_TOP
			:                   Ru::Field_INVALID;
	}

	// Collects informations from the input frame properties
	Ru::FrameInfo  frame_info;
	Ru::get_interlacing_param (
		frame_info._itl_s_flag, frame_info._top_s_flag,
		n, _interlaced_src, _field_order_src, prop_fieldbased, prop_field,
		false
	);
	Ru::get_interlacing_param (
		frame_info._itl_d_flag, frame_info._top_d_flag,
		n, _interlaced_dst, _field_order_dst, prop_fieldbased, prop_field,
		false
	);

	// Processing
	_plane_proc_uptr->process_frame (dst_sptr, n, *env_ptr, &frame_info);

	// Output frame properties
	if (   supports_props ()
	    && (   _range_d_def_flag
	        || _cplace_d_set_flag
	        || _interlaced_dst != Ru::InterlacingParam_AUTO))
	{
		::AVSMap *     props_ptr = env_ptr->getFramePropsRW (dst_sptr);
		if (_range_d_def_flag)
		{
			const int      cr_val = (_fulld_flag) ? 0 : 1;
			env_ptr->propSetInt (
				props_ptr, "_ColorRange", cr_val, ::PROPAPPENDMODE_REPLACE
			);
		}
		if (_cplace_d_set_flag)
		{
			int            cl_val = -1; // Unknown or cannot be expressed
			if (   _cplace_d == fmtcl::ChromaPlacement_MPEG2
			    || (   _cplace_d == fmtcl::ChromaPlacement_DV
			        && _fmt_dst.get_subspl_h () == 2
			        && _fmt_dst.get_subspl_v () == 0))
			{
				cl_val = 0; // Left
			}
			else if (_cplace_d == fmtcl::ChromaPlacement_MPEG1)
			{
				cl_val = 1; // Center
			}

			if (cl_val >= 0)
			{
				env_ptr->propSetInt (
					props_ptr, "_ChromaLocation", cl_val, ::PROPAPPENDMODE_REPLACE
				);
			}
		}
		if (_interlaced_dst != Ru::InterlacingParam_AUTO)
		{
			if (frame_info._itl_d_flag)
			{
				if (_field_order_dst != Ru::FieldOrder_AUTO)
				{
					env_ptr->propSetInt (
						props_ptr, "_FieldBased",
						(_field_order_dst == Ru::FieldOrder_BFF) ? 1 : 2,
						::PROPAPPENDMODE_REPLACE
					);
					env_ptr->propSetInt (
						props_ptr, "_Field",
						(frame_info._top_d_flag) ? 1 : 0,
						::PROPAPPENDMODE_REPLACE
					);
				}
			}
			else
			{
				env_ptr->propSetInt (
					props_ptr, "_FieldBased", 0, ::PROPAPPENDMODE_REPLACE
				);
				env_ptr->propDeleteKey (props_ptr, "_Field");
			}
		}
	}

	return dst_sptr;
}



fmtcl::ChromaPlacement	Resample::conv_str_to_chroma_placement (::IScriptEnvironment &env, std::string cplace)
{
	const auto     cp_val = Ru::conv_str_to_chroma_placement (cplace);
	if (cp_val < 0)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": unexpected cplace string.");
	}

	return cp_val;
}



void	Resample::conv_str_to_chroma_subspl (::IScriptEnvironment &env, int &ssh, int &ssv, std::string css)
{
	const int      ret_val = Ru::conv_str_to_chroma_subspl (ssh, ssv, css);
	if (ret_val != 0)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": unsupported css value.");
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Resample::do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr)
{
	fstb::unused (plane_id);
	assert (ctx_ptr != nullptr);

	const auto     proc_mode = _plane_proc_uptr->get_mode (plane_index);

	if (proc_mode == avsutl::PlaneProcMode_PROCESS)
	{
		const Ru::FrameInfo &   frame_info =
			*reinterpret_cast <const Ru::FrameInfo *> (ctx_ptr);
		process_plane_proc (dst_sptr, env, n, plane_index, frame_info);
	}

	// Copy (and convert)
	else if (proc_mode == avsutl::PlaneProcMode_COPY1)
	{
		process_plane_copy (dst_sptr, env, n, plane_index);
	}

	// Fill
	else if (proc_mode < avsutl::PlaneProcMode_FILL)
	{
		const double   val = _plane_proc_uptr->get_fill_val (plane_index);
		_plane_proc_uptr->fill_plane (dst_sptr, n, val, plane_index);
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FmtAvs	Resample::get_output_colorspace (::IScriptEnvironment &env, const ::AVSValue &args, const FmtAvs &fmt_src)
{
	FmtAvs         fmt_dst = fmt_src;

	// Full colorspace
	if (args [Param_CSP].Defined ())
	{
		if (fmt_dst.conv_from_str (args [Param_CSP].AsString ()) != 0)
		{
			env.ThrowError (fmtcavs_RESAMPLE ": invalid output colorspace.");
		}
	}

	if (! fmt_dst.is_planar ())
	{
		env.ThrowError (fmtcavs_RESAMPLE ": output colorspace must be planar.");
	}

	// Chroma subsampling
	int            ssh = fmt_dst.get_subspl_h ();
	int            ssv = fmt_dst.get_subspl_v ();
	const std::string css (args [Param_CSS].AsString (""));
	if (! css.empty ())
	{
		conv_str_to_chroma_subspl (env, ssh, ssv, css);
		fmt_dst.set_subspl_h (ssh);
		fmt_dst.set_subspl_v (ssv);
	}

	return fmt_dst;
}



void	Resample::process_plane_proc (::PVideoFrame &dst_sptr, ::IScriptEnvironment &env, int n, int plane_index, const Ru::FrameInfo &frame_info)
{
	assert (n >= 0);
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);

	::PVideoFrame  src_sptr     = _clip_src_sptr->GetFrame (n, &env);

	const int      plane_id_s   = _plane_proc_uptr->get_plane_id (
		plane_index, avsutl::PlaneProcessor::ClipIdx_SRC1
	);
	const int      plane_id_d   = _plane_proc_uptr->get_plane_id (
		plane_index, avsutl::PlaneProcessor::ClipIdx_DST
	);

	uint8_t *      data_dst_ptr = dst_sptr->GetWritePtr (plane_id_d);
	const int      stride_dst   = dst_sptr->GetPitch (plane_id_d);
	const uint8_t* data_src_ptr = src_sptr->GetReadPtr (plane_id_s);
	const int      stride_src   = src_sptr->GetPitch (plane_id_s);

	const fmtcl::InterlacingType  itl_s = fmtcl::InterlacingType_get (
		frame_info._itl_s_flag, frame_info._top_s_flag
	);
	const fmtcl::InterlacingType  itl_d = fmtcl::InterlacingType_get (
		frame_info._itl_d_flag, frame_info._top_d_flag
	);

	try
	{
		fmtcl::FilterResize *   filter_ptr = create_or_access_plane_filter (
			plane_index,
			itl_d,
			itl_s
		);

		const bool     chroma_flag =
			fmtcl::is_chroma_plane (_fmt_src.get_col_fam (), plane_index);

		filter_ptr->process_plane (
			data_dst_ptr, 0,
			data_src_ptr, 0,
			stride_dst,
			stride_src,
			chroma_flag
		);
	}

	catch (std::exception &e)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": exception: %s.", e.what ());
	}
	catch (...)
	{
		env.ThrowError (fmtcavs_RESAMPLE ": exception.");
	}
}



void	Resample::process_plane_copy (::PVideoFrame &dst_sptr, ::IScriptEnvironment &env, int n, int plane_index)
{
	assert (n >= 0);
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);

	::PVideoFrame  src_sptr     = _clip_src_sptr->GetFrame (n, &env);

	const int      plane_id_s   = _plane_proc_uptr->get_plane_id (
		plane_index, avsutl::PlaneProcessor::ClipIdx_SRC1
	);
	const int      plane_id_d   = _plane_proc_uptr->get_plane_id (
		plane_index, avsutl::PlaneProcessor::ClipIdx_DST
	);

	const int      src_w = _plane_proc_uptr->get_width (
		src_sptr, plane_id_s, avsutl::PlaneProcessor::ClipIdx_SRC1
	);
	const int      src_h = _plane_proc_uptr->get_height (src_sptr, plane_id_s);
	const int      dst_w = _plane_proc_uptr->get_width (
		dst_sptr, plane_id_d, avsutl::PlaneProcessor::ClipIdx_DST
	);
	const int      dst_h = _plane_proc_uptr->get_height (dst_sptr, plane_id_d);

	uint8_t *      data_dst_ptr = dst_sptr->GetWritePtr (plane_id_d);
	const int      stride_dst   = dst_sptr->GetPitch (plane_id_d);
	const uint8_t* data_src_ptr = src_sptr->GetReadPtr (plane_id_s);
	const int      stride_src   = src_sptr->GetPitch (plane_id_s);

	const int      w = std::min (src_w, dst_w);
	const int      h = std::min (src_h, dst_h);

	// Copied from fmtcl::FilterResize::process_plane_bypass()
	fmtcl::BitBltConv::ScaleInfo *   scale_info_ptr = nullptr;
	fmtcl::BitBltConv::ScaleInfo     scale_info;
	const bool     dst_flt_flag = (_dst_type == fmtcl::SplFmt_FLOAT);
	const bool     src_flt_flag = (_src_type == fmtcl::SplFmt_FLOAT);
	if (dst_flt_flag != src_flt_flag)
	{
		const auto &   plane_data = _plane_data_arr [plane_index];
		scale_info._gain    = plane_data._gain;
		scale_info._add_cst = plane_data._add_cst;

		scale_info_ptr = &scale_info;
	}

	fmtcl::BitBltConv blitter (_sse2_flag, _avx2_flag);
	blitter.bitblt (
		_dst_type, _dst_res, data_dst_ptr, nullptr, stride_dst,
		_src_type, _src_res, data_src_ptr, nullptr, stride_src,
		w, h, scale_info_ptr
	);
}



fmtcl::FilterResize *	Resample::create_or_access_plane_filter (int plane_index, fmtcl::InterlacingType itl_d, fmtcl::InterlacingType itl_s)
{
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);
	assert (itl_d >= 0);
	assert (itl_d < fmtcl::InterlacingType_NBR_ELT);
	assert (itl_s >= 0);
	assert (itl_s < fmtcl::InterlacingType_NBR_ELT);

	const auto &   plane_data = _plane_data_arr [plane_index];
	const fmtcl::ResampleSpecPlane & key = plane_data._spec_arr [itl_d] [itl_s];

	std::lock_guard <std::mutex>  autolock (_filter_mutex);

	std::unique_ptr <fmtcl::FilterResize> &   filter_uptr = _filter_uptr_map [key];
	if (filter_uptr.get () == nullptr)
	{
		filter_uptr = std::make_unique <fmtcl::FilterResize> (
			key,
			*(plane_data._kernel_arr [fmtcl::FilterResize::Dir_H]._k_uptr),
			*(plane_data._kernel_arr [fmtcl::FilterResize::Dir_V]._k_uptr),
			_norm_flag, plane_data._norm_val_h, plane_data._norm_val_v,
			plane_data._gain,
			_src_type, _src_res, _dst_type, _dst_res,
			_int_flag, _sse2_flag, _avx2_flag
		);
	}

	return filter_uptr.get ();
}



void	Resample::create_all_plane_specs (const FmtAvs &fmt_dst, const FmtAvs &fmt_src)
{
	const fmtcl::ColorFamily src_cf = fmt_src.get_col_fam ();
	const fmtcl::ColorFamily dst_cf = fmt_dst.get_col_fam ();
	const int      src_ss_h   = fmt_src.get_subspl_h ();
	const int      src_ss_v   = fmt_src.get_subspl_v ();
	const int      dst_ss_h   = fmt_dst.get_subspl_h ();
	const int      dst_ss_v   = fmt_dst.get_subspl_v ();
	const int      nbr_planes = _vi_src.NumComponents ();
	for (int plane_index = 0; plane_index < nbr_planes; ++plane_index)
	{
		auto &         plane_data = _plane_data_arr [plane_index];
		Ru::create_plane_specs (
			plane_data, plane_index,
			src_cf, _src_width, src_ss_h, _src_height, src_ss_v, _cplace_s,
			dst_cf, vi.width  , dst_ss_h, vi.height  , dst_ss_v, _cplace_d
		);
	}
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
