/*****************************************************************************

        PlaneProcessor.cpp
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

#include "avsutl/fnc.h"
#include "avsutl/PlaneProcCbInterface.h"
#include "avsutl/PlaneProcessor.h"
#include "fstb/fnc.h"
#include "avisynth.h"

#include <algorithm>

#include <cassert>
#include <cstdio>



namespace avsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



PlaneProcessor::PlaneProcessor (const ::VideoInfo &vi, PlaneProcCbInterface &cb, bool manual_flag)
:	_vi (vi)
,	_cb (cb)
,	_nbr_planes (get_nbr_planes (vi))
,	_manual_flag (manual_flag)
{
	// Nothing
}



void	PlaneProcessor::set_proc_mode (const ::AVSValue &arg, ::IScriptEnvironment &env, const char *filter_and_arg_0)
{
	if (! arg.Defined () || arg.IsString ())
	{
		set_proc_mode (arg.AsString ("all"));
	}

	else if (arg.IsArray ())
	{
		constexpr auto def_int = PlaneProcMode_PROCESS;
		constexpr auto def_flt = double (def_int);
		auto           pm_arr { fstb::make_array <_max_nbr_planes> (def_flt) };

		const int      nbr_val = arg.ArraySize ();
		if (nbr_val > _max_nbr_planes)
		{
			env.ThrowError ("%s: too many values.", filter_and_arg_0);
		}
		
		for (int pos = 0; pos < nbr_val; ++pos)
		{
			auto &      arg_elt = arg [pos];
			if (arg_elt.Defined ())
			{
				if (arg_elt.IsFloat ())
				{
					pm_arr [pos] = arg_elt.AsFloat (def_flt);
				}
				else if (arg_elt.IsInt ())
				{
					pm_arr [pos] = double (arg_elt.AsInt (def_int));
				}
				else
				{
					env.ThrowError (
						"%s: element %d (base 0) should be an int or float.",
						filter_and_arg_0, pos
					);
				}
			}
		}

		set_proc_mode (pm_arr);
	}

	else
	{
		env.ThrowError ("%s: unexpected argument type.", filter_and_arg_0);
	}
}



void	PlaneProcessor::set_proc_mode (const std::array <double, _max_nbr_planes> &pm_arr)
{
	assert (std::find_if (pm_arr.begin (), pm_arr.end (),
		[] (double mode) { return ! (mode < PlaneProcMode_NBR_ELT); }
	) == pm_arr.end ());
	_proc_mode_arr = pm_arr;
}



// Accepts:
// y, u, v, a
// r, g, b, a
// 0, 1, 2, 3
void	PlaneProcessor::set_proc_mode (std::string pmode)
{
	fstb::conv_to_lower_case (pmode);

	if (pmode == "all")
	{
		_proc_mode_arr.fill (double (PlaneProcMode_PROCESS));
	}
	else
	{
		_proc_mode_arr.fill (double (PlaneProcMode_GARBAGE));

		// Plane selection by name
		for (const auto &info_categ : CsPlane::_plane_info_list)
		{
			for (size_t plane_idx = 0; plane_idx < info_categ.size (); ++plane_idx)
			{
				const auto &   plane = info_categ [plane_idx];
				if (pmode.find (plane._name) != std::string::npos)
				{
					_proc_mode_arr [plane_idx] = PlaneProcMode_PROCESS;
				}
			}
		}

		// Plane selection by numeric index
		for (int plane_idx = 0; plane_idx < _max_nbr_planes; ++plane_idx)
		{
			const auto     name = char ('0' + plane_idx);
			if (pmode.find (name) != std::string::npos)
			{
				_proc_mode_arr [plane_idx] = PlaneProcMode_PROCESS;
			}
		}
	}
}



void	PlaneProcessor::set_proc_mode (int plane_index, double mode)
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);
	assert (mode < PlaneProcMode_NBR_ELT);
	assert (mode != double (PlaneProcMode_ILLEGAL));

	_proc_mode_arr [plane_index] = mode;
}



double	PlaneProcessor::get_proc_mode (int plane_index) const noexcept
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const double   mode = _proc_mode_arr [plane_index];
	assert (mode != double (PlaneProcMode_ILLEGAL));

	return mode;
}



void	PlaneProcessor::set_dst_clip_info (ClipType type)
{
	set_clip_info (ClipIdx_DST, {}, type);
}



// 0 = destination clip
void	PlaneProcessor::set_clip_info (ClipIdx index, ::PClip clip_sptr, ClipType type)
{
	assert (index >= 0);
	assert (index < ClipIdx_NBR_ELT);
	assert (index == ClipIdx_DST || clip_sptr);
	assert (type >= 0);
	assert (type < ClipType_NBR_ELT);

	// If the clip is one of the "hack-16" type, make sure it is 8 bits.
	assert (   type  == ClipType_NORMAL
	        || index == ClipIdx_DST
	        || clip_sptr->GetVideoInfo ().ComponentSize () == 1);

	_clip_info_arr [index] = { type, clip_sptr };
}



int	PlaneProcessor::get_nbr_planes () const
{
	return _nbr_planes;
}



void	PlaneProcessor::process_frame (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, void *ctx_ptr)
{
	assert (dst_sptr != nullptr);
	assert (dst_sptr->IsWritable ());
	assert (n >= 0);

	for (int plane_index = 0; plane_index < _nbr_planes; ++plane_index)
	{
		const double   mode = _proc_mode_arr [plane_index];
		assert (mode != double (PlaneProcMode_ILLEGAL));

		if (   mode == double (PlaneProcMode_PROCESS)
		    || _manual_flag)
		{
			const int      plane_id = get_plane_id (plane_index, ClipIdx_DST);
			_cb.process_plane (dst_sptr, n, env, plane_index, plane_id, ctx_ptr);
		}
		else
		{
			process_plane_default (dst_sptr, n, env, plane_index);
		}
	}
}



const ::VideoInfo &	PlaneProcessor::use_vi (ClipIdx index) const
{
	assert (index >= 0);
	assert (index < ClipIdx_NBR_ELT);

	if (index == ClipIdx_DST)
	{
		return _vi;
	}

	const ::PClip& clip_sptr = _clip_info_arr [index]._clip_sptr;
	assert (clip_sptr);

	return clip_sptr->GetVideoInfo ();
}



int	PlaneProcessor::get_plane_id (int plane_index, ClipIdx index) const
{
	assert (index >= 0);
	assert (index < ClipIdx_NBR_ELT);

	const auto &   vi = use_vi (index);

	return CsPlane::get_plane_id (plane_index, vi);
}



int	PlaneProcessor::get_width (const ::PVideoFrame &frame_sptr, int plane_id, ClipIdx clip_idx) const
{
	assert (frame_sptr != nullptr);
	assert (clip_idx >= 0);
	assert (clip_idx < ClipIdx_NBR_ELT);

	const auto &   vi          = use_vi (clip_idx);
	const int      width_bytes = frame_sptr->GetRowSize (plane_id);
	const int      bits        = vi.BitsPerComponent ();
	const int      width       = width_bytes / ((bits + 7) >> 3);

	return width;
}



int	PlaneProcessor::get_height (const ::PVideoFrame &frame_sptr, int plane_id) const
{
	assert (frame_sptr != nullptr);

	const int      height = frame_sptr->GetHeight (plane_id);

	return height;
}



int	PlaneProcessor::get_height16 (const ::PVideoFrame &frame_sptr, int plane_id) const
{
	return get_height (frame_sptr, plane_id) >> 1;
}



bool	PlaneProcessor::is_manual () const
{
	return _manual_flag;
}



PlaneProcMode	PlaneProcessor::get_mode (int plane_index) const
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const double   val = _proc_mode_arr [plane_index];
	if (val < double (PlaneProcMode_FILL + 1))
	{
		return PlaneProcMode_FILL;
	}

	return static_cast <PlaneProcMode> (fstb::round_int (val));
}



double	PlaneProcessor::get_fill_val (int plane_index) const
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const double   val = _proc_mode_arr [plane_index];
	assert (val < double (PlaneProcMode_FILL + 1));

	return val;
}



void	PlaneProcessor::process_plane_default (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index)
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const ClipType	type_dst = _clip_info_arr [0]._type;

	const double   mode = _proc_mode_arr [plane_index];
	assert (mode != double (PlaneProcMode_ILLEGAL));

	if (mode == double (PlaneProcMode_PROCESS))
	{
		// We shouldn't be there, but this is not critical.
		assert (false);
	}

	else if (   mode >= double (PlaneProcMode_COPY1)
	         && mode <= double (PlaneProcMode_COPY3))
	{
		static constexpr ClipIdx burp [PlaneProcMode_NBR_ELT] =
		{
			ClipIdx_INVALID, ClipIdx_INVALID, ClipIdx_SRC1,
			ClipIdx_INVALID, ClipIdx_SRC2, ClipIdx_SRC3
		};
		const int         mode_i    = fstb::round_int (mode);
		const ClipIdx     src_index = burp [mode_i];
		if (_clip_info_arr [src_index]._clip_sptr)
		{
			copy (dst_sptr, n, plane_index, type_dst, src_index, env);
		}
	}

	else if (mode < double (PlaneProcMode_FILL + 1))
	{
		fill (dst_sptr, n, plane_index, type_dst, float (-mode));
	}
}



void	PlaneProcessor::fill_plane (::PVideoFrame &dst_sptr, int n, double val, int plane_index)
{
	assert (plane_index >= 0);
	assert (plane_index < _nbr_planes);

	const ClipType type     = _clip_info_arr [0]._type;
	const int      plane_id = get_plane_id (plane_index, ClipIdx_DST);

	fill (dst_sptr, n, plane_id, type, float (val));
}



void	PlaneProcessor::copy_plane (::PVideoFrame &dst_sptr, ClipIdx clip_idx, int n, int plane_index, ::IScriptEnvironment &env)
{
	assert (clip_idx >= ClipIdx_SRC1);
	assert (clip_idx < ClipIdx_NBR_ELT);

	copy (dst_sptr, n, plane_index, _clip_info_arr [0]._type, clip_idx, env);
}



int	PlaneProcessor::get_nbr_planes (const ::VideoInfo &vi)
{
	return vi.NumComponents ();
}



int	PlaneProcessor::get_bytes_per_component (const ::VideoInfo &vi)
{
	return vi.ComponentSize ();
}



int	PlaneProcessor::get_min_w (const ::VideoInfo &vi)
{
	int				l          = vi.width;
	const int		nbr_planes = get_nbr_planes (vi);
	for (int index = 1; index < nbr_planes; ++index)
	{
		const int      lc = compute_plane_w (vi, index, vi.width);
		l = std::min (l, lc);
	}

	return l;
}



int	PlaneProcessor::get_min_h (const ::VideoInfo &vi, bool stack16_flag)
{
	assert (! stack16_flag || vi.BitsPerComponent () == 8);

	int				l          = vi.height;
	const int		nbr_planes = get_nbr_planes (vi);
	for (int index = 1; index < nbr_planes; ++index)
	{
		const int      lc = compute_plane_h (vi, index, vi.height);
		l = std::min (l, lc);
	}
	if (stack16_flag)
	{
		assert ((l & 1) == 0);
		l >>= 1;
	}

	return l;
}



int	PlaneProcessor::compute_plane_w (const ::VideoInfo &vi, int plane_index, int w)
{
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);
	assert (w > 0);

	int				l        = w;
	const int		plane_id = CsPlane::get_plane_id (plane_index, vi);
	const int		subspl   = vi.GetPlaneWidthSubsampling (plane_id);
	l >>= subspl;

	return l;
}



int	PlaneProcessor::compute_plane_h (const ::VideoInfo &vi, int plane_index, int h)
{
	assert (plane_index >= 0);
	assert (plane_index < _max_nbr_planes);
	assert (h > 0);

	int				l        = h;
	const int		plane_id = CsPlane::get_plane_id (plane_index, vi);
	const int		subspl   = vi.GetPlaneHeightSubsampling (plane_id);
	l >>= subspl;

	return l;
}



void	PlaneProcessor::check_same_format (::IScriptEnvironment *env_ptr, const ::VideoInfo &vi, const ::PClip tst_sptr, const char *fnc_name_0, const char *arg_name_0, int flags)
{
	assert (env_ptr != 0);
	assert (tst_sptr != 0);
	assert (fnc_name_0 != 0);
	assert (arg_name_0 != 0);

	const ::VideoInfo &  vi_tst = tst_sptr->GetVideoInfo ();

	static char    txt_0 [4095+1];

	if (   vi_tst.height != vi.height
	    && (flags & FmtChkFlag_H) != 0)
	{
		fstb::snprintf4all (
			txt_0, sizeof (txt_0),
			"%s: clip \"%s\" has a height of %d instead of %d.",
			fnc_name_0, arg_name_0, vi_tst.height, vi.height
		);
		env_ptr->ThrowError (txt_0);
	}

	if (   (flags & FmtChkFlag_W) != 0
	    && vi_tst.width != vi.width)
	{
		fstb::snprintf4all (
			txt_0, sizeof (txt_0),
			"%s: clip \"%s\" has a width of %d instead of %d.",
			fnc_name_0, arg_name_0, vi_tst.width, vi.width
		);
		env_ptr->ThrowError (txt_0);
	}

	if (   (flags & FmtChkFlag_NBR_FRAMES) != 0
	    && vi_tst.num_frames != vi.num_frames)
	{
		fstb::snprintf4all (
			txt_0, sizeof (txt_0),
			"%s: clip \"%s\" has %d frames instead of %d.",
			fnc_name_0, arg_name_0, vi_tst.num_frames, vi.num_frames
		);
		env_ptr->ThrowError (txt_0);
	}

	// Tests absolute colorspace equality
	if ((flags & FmtChkFlag_CS_ALL) == FmtChkFlag_CS_ALL)
	{
		if (! vi_tst.IsSameColorspace (vi))
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has not the same colorspace (code %X instead of %X).",
				fnc_name_0, arg_name_0, vi_tst.pixel_type, vi.pixel_type
			);
			env_ptr->ThrowError (txt_0);
		}
	}

	// Tests only specific colorspace features
	else
	{
		if (   (flags & FmtChkFlag_CS_TYPE) != 0
		    && (   is_rgb (vi_tst) != is_rgb (vi)
		        || (is_rgb (vi) && has_alpha (vi_tst) != has_alpha (vi))))
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong pixel type.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		if (   (flags & FmtChkFlag_CS_LAYOUT) != 0
		    && vi_tst.IsPlanar () != vi.IsPlanar ())
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong planarity.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		// Hahaha! The following test is unreadable. Basically, it makes
		// Y compatible with any other YUV colorspace (planar or not).
		if (   (flags & FmtChkFlag_CS_SUBSPL) != 0
		    && (   (vi_tst.IsYUV () || vi_tst.IsYUVA ()) != (vi.IsYUV () || vi.IsYUVA ())
		        || (   ((vi.IsYUV () || vi.IsYUVA ()) && ! vi.IsY () && ! vi_tst.IsY ())
		            && (   vi_tst.GetPlaneWidthSubsampling  (PLANAR_U) != vi.GetPlaneWidthSubsampling (PLANAR_U)
		                || vi_tst.GetPlaneHeightSubsampling (PLANAR_U) != vi.GetPlaneHeightSubsampling (PLANAR_U)
		               ) 
	              )
		       )
		   )
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong chroma subsampling.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		if (   (flags & FmtChkFlag_CS_FORMAT) != 0
		    && vi_tst.BitsPerComponent () != vi.BitsPerComponent ())
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong data format.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
		if (   (flags & FmtChkFlag_CS_NBRCOMP) != 0
		    && vi_tst.NumComponents () != vi.NumComponents ())
		{
			fstb::snprintf4all (
				txt_0, sizeof (txt_0),
				"%s: clip \"%s\" has a wrong number of components.",
				fnc_name_0, arg_name_0
			);
			env_ptr->ThrowError (txt_0);
		}
	}
}



bool	PlaneProcessor::check_stack16_width (const ::VideoInfo &vi, int width)
{
	assert (vi.BitsPerComponent () == 8);

	if (width < 0)
	{
		width = vi.width;
	}

	bool				ok_flag = true;

	const int		nbr_planes = get_nbr_planes (vi);
	const auto &   info_categ = CsPlane::use_categ_info (vi);
	for (int index = 0; index < nbr_planes && ok_flag; ++index)
	{
		const int		plane_id = info_categ [index]._id;
		const int		subspl   = vi.GetPlaneWidthSubsampling (plane_id);
		const int		mask     = (1 << subspl) - 1;
		if ((width & mask) != 0)
		{
			ok_flag = false;
		}
	}

	return ok_flag;
}



bool	PlaneProcessor::check_stack16_height (const ::VideoInfo &vi, int height)
{
	assert (vi.BitsPerComponent () == 8);

	if (height < 0)
	{
		height = vi.height;
	}

	bool				ok_flag = ((height & 1) == 0);

	const int		nbr_planes = get_nbr_planes (vi);
	const auto &   info_categ = CsPlane::use_categ_info (vi);
	for (int index = 0; index < nbr_planes && ok_flag; ++index)
	{
		const int		plane_id = info_categ [index]._id;
		const int		subspl   = vi.GetPlaneHeightSubsampling (plane_id);
		const int		mask     = (2 << subspl) - 1;
		if ((height & mask) != 0)
		{
			ok_flag = false;
		}
	}

	return ok_flag;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	PlaneProcessor::fill (::PVideoFrame &dst_sptr, int n, int plane_index, ClipType type, float val)
{
	assert (dst_sptr != nullptr);
	assert (n >= 0);
	assert (type >= 0);

	if (type == ClipType_STACKED_16)
	{
		const int      val_int = fstb::round_int (val);
		const int      val_msb = val_int >> 8;
		const int      val_lsb = val_int & 255;
		fill_frame_part (dst_sptr, n, plane_index, float (val_msb), true, 0);
		fill_frame_part (dst_sptr, n, plane_index, float (val_lsb), true, 1);
	}

	else if (type == ClipType_INTERLEAVED_16)
	{
		int            val_int = fstb::round_int (val);
		val_int >>= (1 - (n & 1)) * 8;
		fill_frame_part (dst_sptr, n, plane_index, float (val_int & 255), false, 0);
	}

	else
	{
		fill_frame_part (dst_sptr, n, plane_index, val, false, 0);
	}
}



void	PlaneProcessor::fill_frame_part (::PVideoFrame &dst_sptr, int n, int plane_index, float val, bool stacked_flag, int part)
{
	fstb::unused (n);

	const int      plane_id = get_plane_id (plane_index, ClipIdx_DST);
	const int      stride   = dst_sptr->GetPitch (plane_id);
	const int      width    = get_width (dst_sptr, plane_id, ClipIdx_DST);
	int            height   = get_height (dst_sptr, plane_id);

	if (_vi.ComponentSize () == 1)
	{
		uint8_t *      data_ptr = dst_sptr->GetWritePtr (plane_id);

		if (stacked_flag && part != 0)
		{
			height >>= 1;
			data_ptr += stride * height;
		}

		if (height > 0)
		{
			const auto     val_int = uint8_t (fstb::round_int (val));
			avsutl::fill_block (data_ptr, val_int, stride, width, height);
		}
		else
		{
			assert (false);
		}
	}

	else if (_vi.ComponentSize () == 2)
	{
		const auto     val_int  = uint16_t (fstb::round_int (val));
		auto           data_ptr = dst_sptr->GetWritePtr (plane_id);
		avsutl::fill_block (data_ptr, val_int, stride, width, height);
	}

	else
	{
		assert (_vi.ComponentSize () == 4);
		auto           data_ptr = dst_sptr->GetWritePtr (plane_id);
		avsutl::fill_block (data_ptr, val, stride, width, height);
	}
}



void	PlaneProcessor::copy (::PVideoFrame &dst_sptr, int n, int plane_index, ClipType type_dst, ClipIdx src_idx, ::IScriptEnvironment &env)
{
	assert (dst_sptr != nullptr);
	assert (n >= 0);
	assert (type_dst >= 0);
	assert (type_dst < ClipType_NBR_ELT);
	assert (src_idx >= ClipIdx_SRC1);
	assert (src_idx < ClipIdx_NBR_ELT);

	const ClipInfo &  info_src = _clip_info_arr [src_idx];

	if (info_src._clip_sptr != nullptr)
	{
		const int      nbr_bytes_d = get_bytes_per_component (_vi);
		const int      nbr_bytes_s =
			get_bytes_per_component (info_src._clip_sptr->GetVideoInfo ());

		if (nbr_bytes_d == 1 && nbr_bytes_s == 1)
		{
			if (have_same_height (type_dst, info_src._type))
			{
				if (   type_dst       == ClipType_INTERLEAVED_16
				    && info_src._type != ClipType_INTERLEAVED_16)
				{
					n >>= 1;
				}
				else if (   type_dst       != ClipType_INTERLEAVED_16
				         && info_src._type == ClipType_INTERLEAVED_16)
				{
					n <<= 1;
					if (type_dst == ClipType_LSB)
					{
						++ n;
					}
				}

				copy_n_to_n (dst_sptr, src_idx, n, plane_index, env);
			}

			else if (is_stacked (info_src._type) && ! is_stacked (type_dst))
			{
				bool				lsb_flag = (type_dst == ClipType_LSB);

				if (type_dst == ClipType_INTERLEAVED_16)
				{
					lsb_flag = ((n & 1) != 0);
					n >>= 1;
				}

				copy_stack16_to_8 (
					dst_sptr, src_idx, n, plane_index, env, (lsb_flag) ? 1 : 0
				);
			}

			else	// ! is_stacked (type_src) && is_stacked (type_dst)
			{
				if (info_src._type == ClipType_INTERLEAVED_16)
				{
					n <<= 1;
					copy_8_to_stack16 (dst_sptr, src_idx, n,     plane_index, env, 0);
					copy_8_to_stack16 (dst_sptr, src_idx, n + 1, plane_index, env, 1);
				}

				else
				{
					copy_8_to_stack16 (dst_sptr, src_idx, n,     plane_index, env, 0);
					fill_frame_part (dst_sptr, n, plane_index, 0, true, 1);
				}
			}
		}

		else if (nbr_bytes_d == nbr_bytes_s)
		{
			copy_n_to_n (dst_sptr, src_idx, n, plane_index, env);
		}
	}
}



void	PlaneProcessor::copy_n_to_n (::PVideoFrame &dst_sptr, ClipIdx src_idx, int n, int plane_index, ::IScriptEnvironment &env)
{
	const int      plane_id_d   = get_plane_id (plane_index, ClipIdx_DST);
	const int      dst_stride   = dst_sptr->GetPitch (plane_id_d);
	const int      dst_width    = get_width (dst_sptr, plane_id_d, ClipIdx_DST);
	const int      dst_height   = get_height (dst_sptr, plane_id_d);
	uint8_t *      dst_data_ptr = dst_sptr->GetWritePtr (plane_id_d);

	auto &         src_clip     = _clip_info_arr [src_idx]._clip_sptr;
	assert (src_clip);
	n = std::min (n, src_clip->GetVideoInfo ().num_frames - 1);

	::PVideoFrame  src_sptr     = src_clip->GetFrame (n, &env);
	const int      plane_id_s   = get_plane_id (plane_index, src_idx);
	const int      src_stride   = src_sptr->GetPitch (plane_id_s);
	const int      src_width    = get_width (src_sptr, plane_id_s, src_idx);
	const int      src_height   = get_height (src_sptr, plane_id_s);
	const uint8_t* src_data_ptr = src_sptr->GetReadPtr (plane_id_s);

	const int      width        = std::min (src_width,  dst_width);
	const int      height       = std::min (src_height, dst_height);
	const int      width_bytes  = width * _vi.ComponentSize ();

	env.BitBlt (
		dst_data_ptr, dst_stride,
		src_data_ptr, src_stride, width_bytes, height
	);
}



void	PlaneProcessor::copy_8_to_stack16 (::PVideoFrame &dst_sptr, ClipIdx src_idx, int n, int plane_index, ::IScriptEnvironment &env, int part)
{
	const int      plane_id_d   = get_plane_id (plane_index, ClipIdx_DST);
	const int      dst_stride   = dst_sptr->GetPitch (plane_id_d);
	const int      dst_width    = get_width (dst_sptr, plane_id_d, ClipIdx_DST);
	const int      dst_height   = get_height16 (dst_sptr, plane_id_d);
	uint8_t *      dst_data_ptr = dst_sptr->GetWritePtr (plane_id_d);

	auto &         src_clip     = _clip_info_arr [src_idx]._clip_sptr;
	assert (src_clip);
	n = std::min (n, src_clip->GetVideoInfo ().num_frames - 1);

	::PVideoFrame  src_sptr     = src_clip->GetFrame (n, &env);
	const int      plane_id_s   = get_plane_id (plane_index, src_idx);
	const int      src_stride   = src_sptr->GetPitch (plane_id_s);
	const int      src_width    = get_width (src_sptr, plane_id_s, src_idx);
	const int      src_height   = get_height (src_sptr, plane_id_s);
	const uint8_t* src_data_ptr = src_sptr->GetReadPtr (plane_id_s);

	const int      width        = std::min (src_width,  dst_width);
	const int      height       = std::min (src_height, dst_height);

	env.BitBlt (
		dst_data_ptr + part * dst_height * dst_stride, dst_stride,
		src_data_ptr, src_stride, width, height
	);
}



void	PlaneProcessor::copy_stack16_to_8 (::PVideoFrame &dst_sptr, ClipIdx src_idx, int n, int plane_index, ::IScriptEnvironment &env, int part)
{
	const int      plane_id_d   = get_plane_id (plane_index, ClipIdx_DST);
	const int      dst_stride   = dst_sptr->GetPitch (plane_id_d);
	const int      dst_width    = get_width (dst_sptr, plane_id_d, ClipIdx_DST);
	const int      dst_height   = get_height (dst_sptr, plane_id_d);
	uint8_t *      dst_data_ptr = dst_sptr->GetWritePtr (plane_id_d);

	auto &         src_clip     = _clip_info_arr [src_idx]._clip_sptr;
	assert (src_clip);
	n = std::min (n, src_clip->GetVideoInfo ().num_frames - 1);

	::PVideoFrame  src_sptr     = src_clip->GetFrame (n, &env);
	const int      plane_id_s   = get_plane_id (plane_index, src_idx);
	const int      src_stride   = src_sptr->GetPitch (plane_id_s);
	const int      src_width    = get_width (src_sptr, plane_id_s, src_idx);
	const int      src_height   = get_height16 (src_sptr, plane_id_s);
	const uint8_t* src_data_ptr = src_sptr->GetReadPtr (plane_id_s);

	const int      width        = std::min (src_width,  dst_width);
	const int      height       = std::min (src_height, dst_height);

	env.BitBlt (
		dst_data_ptr, dst_stride,
		src_data_ptr + part * src_height * src_stride, src_stride,
		width, height
	);
}



bool	PlaneProcessor::have_same_height (ClipType t1, ClipType t2)
{
	assert (t1 >= 0);
	assert (t1 < ClipType_NBR_ELT);
	assert (t2 >= 0);
	assert (t2 < ClipType_NBR_ELT);

	return (! (is_stacked (t1) ^ is_stacked (t2)));
}



bool	PlaneProcessor::is_stacked (ClipType type)
{
	assert (type >= 0);
	assert (type < ClipType_NBR_ELT);

	return (type == ClipType_STACKED_16);
}



}  // namespace avsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
