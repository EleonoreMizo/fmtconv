/*****************************************************************************

        NativeToStack16.cpp
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

#include "fmtc/NativeToStack16.h"
#include "fstb/def.h"
#include "vsutl/FrameRefSPtr.h"

#include <stdexcept>

#include <cassert>
#include <cstdint>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



NativeToStack16::NativeToStack16 (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "nativetostack16", ::fmParallel, 0)
,	_clip_src_sptr (vsapi.propGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
{
	fstb::unused (out, user_data_ptr);

	// Checks the input clip
	if (_vi_in.format == 0)
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	// Source colorspace
	const ::VSFormat &   fmt_src = *_vi_in.format;
	if (   fmt_src.sampleType     != ::stInteger
	    || fmt_src.bytesPerSample != 2
	    || fmt_src.colorFamily    == ::cmCompat)
	{
		throw_inval_arg ("pixel format not supported.");
	}

	// Output format
	_vi_out.format = register_format (
		fmt_src.colorFamily,
		fmt_src.sampleType,
		8,
		fmt_src.subSamplingW,
		fmt_src.subSamplingH,
		core
	);
	_vi_out.height *= 2;	// Works also with height == 0
}



void	NativeToStack16::init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core)
{
	fstb::unused (in, out, core);

	_vsapi.setVideoInfo (&_vi_out, 1, &node);
}



const ::VSFrameRef *	NativeToStack16::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
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

		const int      w = _vsapi.getFrameWidth (&src, 0);
		const int      h = _vsapi.getFrameHeight (&src, 0);

		dst_ptr = _vsapi.newVideoFrame (_vi_out.format, w, h << 1, &src, &core);

		const int      nbr_planes = _vi_out.format->numPlanes;
		for (int plane_index = 0; plane_index < nbr_planes; ++plane_index)
		{
			const int      pw = _vsapi.getFrameWidth (&src, plane_index);
			const int      ph = _vsapi.getFrameHeight (&src, plane_index);

			const uint8_t* data_src_ptr = _vsapi.getReadPtr (&src, plane_index);
			const int      stride_src   = _vsapi.getStride (&src, plane_index);
			uint8_t *      data_dst_ptr = _vsapi.getWritePtr (dst_ptr, plane_index);
			const int      stride_dst   = _vsapi.getStride (dst_ptr, plane_index);

			const int      lsb_offset = stride_dst * ph;

			for (int y = 0; y < ph; ++y)
			{
				for (int x = 0; x < pw; ++x)
				{
					const int      val =
						reinterpret_cast <const uint16_t *> (data_src_ptr) [x];
					data_dst_ptr [x             ] = uint8_t (val >> 8);
					data_dst_ptr [x + lsb_offset] = uint8_t (val     );
				}

				data_src_ptr += stride_src;
				data_dst_ptr += stride_dst;
			}
		}
	}

	return (dst_ptr);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
