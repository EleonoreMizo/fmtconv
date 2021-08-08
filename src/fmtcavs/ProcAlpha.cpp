/*****************************************************************************

        ProcAlpha.cpp
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

#include "avsutl/CsPlane.h"
#include "avsutl/fnc.h"
#include "fmtcavs/CpuOpt.h"
#include "fmtcavs/fnc.h"
#include "fmtcavs/ProcAlpha.h"
#include "fmtcl/fnc.h"
#include "avisynth.h"

#include <cassert>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// w and h in pixels
ProcAlpha::ProcAlpha (FmtAvs fmt_dst, FmtAvs fmt_src, bool fulld_flag, bool fulls_flag, int w, int h, const CpuOpt &cpu_opt)
:	_dst_a_flag (fmt_dst.has_alpha ())
,	_src_a_flag (fmt_src.has_alpha ())
,	_dst_res (fmt_dst.get_bitdepth ())
,	_src_res (fmt_dst.get_bitdepth ())
,	_splfmt_dst (conv_bitdepth_to_splfmt (_dst_res))
,	_splfmt_src (conv_bitdepth_to_splfmt (_src_res))
,	_w (w)
,	_h (h)
,	_scale_info ()
,	_sse2_flag (cpu_opt.has_sse2 ())
,	_avx2_flag (cpu_opt.has_avx2 ())
{
	if (_dst_a_flag && _src_a_flag)
	{
		const auto     col_fam_dst = fmt_dst.get_col_fam ();
		const auto     col_fam_src = fmt_src.get_col_fam ();
		fmtcl::compute_fmt_mac_cst (
			_scale_info._gain,
			_scale_info._add_cst,
			fmtcl::PicFmt { _splfmt_dst, _dst_res, col_fam_dst, fulld_flag },
			fmtcl::PicFmt { _splfmt_src, _src_res, col_fam_src, fulls_flag },
			avsutl::CsPlane::_plane_index_alpha
		);
	}
}



void	ProcAlpha::process_plane (::PVideoFrame &dst_sptr, ::PVideoFrame &src_sptr) const
{
	if (_dst_a_flag)
	{
		uint8_t *      dst_ptr    = dst_sptr->GetWritePtr (::PLANAR_A);
		const int      dst_stride = dst_sptr->GetPitch (::PLANAR_A);

		// Copy
		if (_src_a_flag)
		{
			const uint8_t* src_ptr    = src_sptr->GetReadPtr (::PLANAR_A);
			const int      src_stride = src_sptr->GetPitch (::PLANAR_A);

			fmtcl::BitBltConv blitter (_sse2_flag, _avx2_flag);
			blitter.bitblt (
				_splfmt_dst, _dst_res, dst_ptr, nullptr, dst_stride,
				_splfmt_src, _src_res, src_ptr, nullptr, src_stride,
				_w, _h,
				&_scale_info
			);
		}

		// Fill with full opacity
		else
		{
			switch (_splfmt_dst)
			{
			case fmtcl::SplFmt_FLOAT:
				avsutl::fill_block (dst_ptr, 1.f, dst_stride, _w, _h);
				break;
			case fmtcl::SplFmt_INT16:
				{
					const auto     v = uint16_t ((1 << _dst_res) - 1);
					avsutl::fill_block (dst_ptr, v, dst_stride, _w, _h);
				}
				break;
			case fmtcl::SplFmt_INT8:
				avsutl::fill_block (dst_ptr, uint8_t (0xFF), dst_stride, _w, _h);
				break;
			default:
				// Other formats not supported
				assert (false);
				break;
			}
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
