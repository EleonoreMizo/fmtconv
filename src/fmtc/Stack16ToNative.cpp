/*****************************************************************************

        Stack16ToNative.cpp
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

#include "fmtc/Stack16ToNative.h"
#include "fstb/def.h"
#include "vsutl/fnc.h"
#include "vsutl/FrameRefSPtr.h"

#include <stdexcept>

#include <cassert>
#include <cstdint>



namespace fmtc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Stack16ToNative::Stack16ToNative (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
:	vsutl::FilterBase (vsapi, "stack16tonative", ::fmParallel)
,	_clip_src_sptr (vsapi.mapGetNode (&in, "clip", 0, 0), vsapi)
,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
,	_vi_out (_vi_in)
{
	fstb::unused (out, user_data_ptr);

	// Checks the input clip
	if (! vsutl::is_constant_format (_vi_in))
	{
		throw_inval_arg ("only constant pixel formats are supported.");
	}

	// Source colorspace
	const auto &   fmt_src = _vi_in.format;
	if (   fmt_src.sampleType     != ::stInteger
	    || fmt_src.bytesPerSample != 1)
	{
		throw_inval_arg ("pixel format not supported.");
	}

	// The test also works with height == 0
	const int      two_chroma_rows = 2 << fmt_src.subSamplingH;
	if ((_vi_in.height & (two_chroma_rows - 1)) != 0)
	{
		throw_inval_arg ("height must be even for all planes.");
	}

	// Output format
	if (! register_format (
		_vi_out.format,
		fmt_src.colorFamily,
		fmt_src.sampleType,
		16,
		fmt_src.subSamplingW,
		fmt_src.subSamplingH,
		core
	))
	{
		throw_inval_arg ("cannot set the output format.");
	}
	_vi_out.height /= 2;	// Works also with height == 0
}



::VSVideoInfo	Stack16ToNative::get_video_info () const
{
	return _vi_out;
}



std::vector <::VSFilterDependency>	Stack16ToNative::get_dependencies () const
{
	return std::vector <::VSFilterDependency> {
		{ &*_clip_src_sptr, ::rpStrictSpatial }
	};
}



const ::VSFrame *	Stack16ToNative::get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
{
	fstb::unused (frame_data_ptr);
	assert (n >= 0);

	::VSFrame *    dst_ptr = nullptr;
	::VSNode &     node    = *_clip_src_sptr;

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
		const ::VSFrame & src = *src_sptr;

		const int      w = _vsapi.getFrameWidth (&src, 0);
		const int      h = _vsapi.getFrameHeight (&src, 0);

		const int      two_chroma_rows = 2 << _vi_in.format.subSamplingH;
		if ((h & (two_chroma_rows - 1)) != 0)
		{
			_vsapi.setFilterError (
				"stack16tonative: height must be even for all planes.",
				&frame_ctx
			);
		}

		else
		{
			dst_ptr = _vsapi.newVideoFrame (&_vi_out.format, w, h >> 1, &src, &core);

			const int      nbr_planes = _vi_out.format.numPlanes;
			for (int plane_index = 0; plane_index < nbr_planes; ++plane_index)
			{
				const int      pw = _vsapi.getFrameWidth (&src, plane_index);
				const int      ph = _vsapi.getFrameHeight (&src, plane_index);
				const int      hh = ph >> 1;

				const uint8_t* data_src_ptr = _vsapi.getReadPtr (&src, plane_index);
				const auto     stride_src   = _vsapi.getStride (&src, plane_index);
				uint8_t *      data_dst_ptr = _vsapi.getWritePtr (dst_ptr, plane_index);
				const auto     stride_dst   = _vsapi.getStride (dst_ptr, plane_index);

				const auto     lsb_offset = stride_src * hh;

				for (int y = 0; y < hh; ++y)
				{
					for (int x = 0; x < pw; ++x)
					{
						const int      msb = data_src_ptr [x             ];
						const int      lsb = data_src_ptr [x + lsb_offset];
						reinterpret_cast <uint16_t *> (data_dst_ptr) [x] =
							uint16_t ((msb << 8) + lsb);
					}

					data_src_ptr += stride_src;
					data_dst_ptr += stride_dst;
				}
			}
		}
	}

	return dst_ptr;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtc



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
