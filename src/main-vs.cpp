/*****************************************************************************

        main.cpp
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#include "fmtc/Bitdepth.h"
#if 0
	#include "fmtc/Convert.h"
#endif
#include "fmtc/Matrix.h"
#include "fmtc/Matrix2020CL.h"
#include "fmtc/NativeToStack16.h"
#include "fmtc/Primaries.h"
#include "fmtc/Resample.h"
#include "fmtc/Stack16ToNative.h"
#include "fmtc/Transfer.h"
#include "fmtc/version.h"
#include "fstb/def.h"
#include "vsutl/Redirect.h"
#include "VapourSynth4.h"

#include <algorithm>



/*############################################################################

        Temporary stuff

############################################################################*/



#define main_HISTLUMA

#include "vsutl/fnc.h"



#if defined (main_HISTLUMA)

class TmpHistLuma
:	public vsutl::FilterBase
{

public:

	TmpHistLuma (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi)
	:	vsutl::FilterBase (vsapi, "histluma", ::fmParallel)
	,	_clip_src_sptr (vsapi.mapGetNode (&in, "clip", 0, 0), vsapi)
	,	_vi_in (*_vsapi.getVideoInfo (_clip_src_sptr.get ()))
	,	_vi_out (_vi_in)
	,	_full_flag (get_arg_int (in, out, "full", 0) != 0)
	,	_amp (std::max (get_arg_int (in, out, "amp", 16), 1))
	{
		fstb::unused (user_data_ptr, core);

		if (! vsutl::is_constant_format (_vi_in))
		{
			throw_inval_arg ("only constant formats are supported.");
		}
		const auto &   fmt_src = _vi_in.format;
		if (   fmt_src.sampleType != ::stInteger
		    || fmt_src.bitsPerSample > 16)
		{
			throw_inval_arg ("only integer input with 16 bits or less.");
		}
		if (_vsapi.mapGetError (&out) != nullptr)
		{
			throw -1;
		}
	}

	// vsutl::FilterBase
	::VSVideoInfo	get_video_info () const
	{
		return _vi_out;
	}

	std::vector <::VSFilterDependency>	get_dependencies () const
	{
		return std::vector <::VSFilterDependency> {
			{ &*_clip_src_sptr, ::rpStrictSpatial }
		};
	}

	virtual const ::VSFrame *	get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core)
	{
		fstb::unused (frame_data_ptr);

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
			dst_ptr = _vsapi.newVideoFrame (
				&_vi_out.format, _vi_out.width, _vi_out.height, &src, &core
			);
			const int      bits = _vi_out.format.bitsPerSample;

			// Luma
			{
				const uint8_t* data_src_ptr = _vsapi.getReadPtr (&src, 0);
				const auto     stride_src   = _vsapi.getStride (&src, 0);
				uint8_t *      data_dst_ptr = _vsapi.getWritePtr (dst_ptr, 0);
				const auto     stride_dst   = _vsapi.getStride (dst_ptr, 0);
				const int      w = _vsapi.getFrameWidth (dst_ptr, 0);
				const int      h = _vsapi.getFrameHeight (dst_ptr, 0);

				const int      range = (_full_flag) ? ((1 << bits) - 1) : (219 << (bits - 8));	// Half open
				const int      mi    = (_full_flag) ? 0 : (16 << (bits - 8));
				const int      mi_in = mi;
				const int      rpa   = (range + 1) / _amp - 1;

				for (int y = 0; y < h; ++y)
				{
					if (bits > 8)
					{
						for (int x = 0; x < w; ++x)
						{
							int            v = reinterpret_cast <const uint16_t *> (data_src_ptr) [x];
							v = mi + (rpa - std::abs ((v - mi_in) % (rpa * 2) - rpa)) * _amp;
							reinterpret_cast <uint16_t *> (data_dst_ptr) [x] = uint16_t (v);
						}
					}
					else
					{
						for (int x = 0; x < w; ++x)
						{
							int            v = data_src_ptr [x];
							v = mi + (rpa - std::abs ((v - mi_in) % (rpa * 2) - rpa)) * _amp;
							data_dst_ptr [x] = uint8_t (v);
						}
					}

					data_src_ptr += stride_src;
					data_dst_ptr += stride_dst;
				}
			}

			// Chroma
			for (int plane = 1; plane < _vi_out.format.numPlanes; ++plane)
			{
				uint8_t *      data_dst_ptr = _vsapi.getWritePtr (dst_ptr, plane);
				const auto     stride_dst   = _vsapi.getStride (dst_ptr, plane);
				const int      w = _vsapi.getFrameWidth (dst_ptr, plane);
				const int      h = _vsapi.getFrameHeight (dst_ptr, plane);
				if (_vi_out.format.bytesPerSample == 2)
				{
					const uint16_t    fill_cst = uint16_t (1 << (bits - 1));
					for (int y = 0; y < h; ++y)
					{
						for (int x = 0; x < w; ++x)
						{
							reinterpret_cast <uint16_t *> (data_dst_ptr) [x] = fill_cst;
						}

						data_dst_ptr += stride_dst;
					}
				}
				else
				{
					memset (data_dst_ptr, 128, stride_dst * h);
				}
			}
		}

		return dst_ptr;
	}

protected:

private:

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.
	bool           _full_flag;
	int            _amp;

};

#endif   // main_HISTLUMA



/*############################################################################

        End of the temporary stuff

############################################################################*/



VS_EXTERNAL_API (void) VapourSynthPluginInit2 (::VSPlugin *plugin_ptr, const ::VSPLUGINAPI *api_ptr)
{
	api_ptr->configPlugin (
		fmtc_PLUGIN_NAME,
		fmtc_NAMESPACE,
		"Format converter",
		VS_MAKE_VERSION (fmtc_VERSION, 0),
		VAPOURSYNTH_API_VERSION,
		0, // VSPluginConfigFlags
		plugin_ptr
	);

	api_ptr->registerFunction ("resample",
		"clip:vnode;"
		"w:int:opt;"
		"h:int:opt;"
		"sx:float[]:opt;"
		"sy:float[]:opt;"
		"sw:float[]:opt;"
		"sh:float[]:opt;"
		"scale:float:opt;"
		"scaleh:float:opt;"
		"scalev:float:opt;"
		"kernel:data[]:opt;"
		"kernelh:data[]:opt;"
		"kernelv:data[]:opt;"
		"impulse:float[]:opt;"
		"impulseh:float[]:opt;"
		"impulsev:float[]:opt;"
		"taps:int[]:opt;"
		"tapsh:int[]:opt;"
		"tapsv:int[]:opt;"
		"a1:float[]:opt;"
		"a2:float[]:opt;"
		"a3:float[]:opt;"
		"a1h:float[]:opt;"
		"a2h:float[]:opt;"
		"a3h:float[]:opt;"
		"a1v:float[]:opt;"
		"a2v:float[]:opt;"
		"a3v:float[]:opt;"
		"kovrspl:int[]:opt;"
		"fh:float[]:opt;"
		"fv:float[]:opt;"
		"cnorm:int[]:opt;"
		"total:float[]:opt;"
		"totalh:float[]:opt;"
		"totalv:float[]:opt;"
		"invks:int[]:opt;"
		"invksh:int[]:opt;"
		"invksv:int[]:opt;"
		"invkstaps:int[]:opt;"
		"invkstapsh:int[]:opt;"
		"invkstapsv:int[]:opt;"
		"csp:int:opt;"
		"css:data:opt;"       // "444", "422", "420", "411"
		"planes:float[]:opt;" // Masktools style
		"fulls:int:opt;"
		"fulld:int:opt;"
		"center:int[]:opt;"
		"cplace:data:opt;"
		"cplaces:data:opt;"
		"cplaced:data:opt;"
		"interlaced:int:opt;"
		"interlacedd:int:opt;"
		"tff:int:opt;"
		"tffd:int:opt;"
		"flt:int:opt;"
		"cpuopt:int:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Resample>::create, nullptr, plugin_ptr
	);

	api_ptr->registerFunction ("matrix",
		"clip:vnode;"
		"mat:data:opt;"     // Matrix for YUV <-> RGB conversions: "601", "709", "240", "FCC"
		"mats:data:opt;"    // Source matrix for YUV
		"matd:data:opt;"    // Destination matrix for YUV
		"fulls:int:opt;"
		"fulld:int:opt;"
		"coef:float[]:opt;" // A linearized 4x3 matrix, the 4th column being the constant to add. Overrides other matrix settings, including full/TV range.
		"csp:int:opt;"      // Can only convert to a colorspace of the same input pixel format and number of planes.
		"col_fam:int:opt;"  // Same goal as csp, but takes precedence over it.
		"bits:int:opt;"
		"singleout:int:opt;"
		"cpuopt:int:opt;"
		"planes:float[]:opt;" // Masktools style
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Matrix>::create, nullptr, plugin_ptr
	);

	api_ptr->registerFunction ("matrix2020cl",
		"clip:vnode;"
		"full:int:opt;"
		"csp:int:opt;"
		"bits:int:opt;"
		"cpuopt:int:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Matrix2020CL>::create, nullptr, plugin_ptr
	);

	api_ptr->registerFunction ("bitdepth",
		"clip:vnode;"
		"csp:int:opt;"
		"bits:int:opt;"
		"flt:int:opt;"
		"planes:int[]:opt;" // Simple mode (list of planes to process)
		"fulls:int:opt;"
		"fulld:int:opt;"
		"dmode:int:opt;"
		"ampo:float:opt;"
		"ampn:float:opt;"
		"dyn:int:opt;"
		"staticnoise:int:opt;"
		"cpuopt:int:opt;"
		"patsize:int:opt;"
		"tpdfo:int:opt;"
		"tpdfn:int:opt;"
		"corplane:int:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Bitdepth>::create, nullptr, plugin_ptr
	);

	api_ptr->registerFunction ("transfer",
		"clip:vnode;"
		"transs:data[]:opt;"
		"transd:data[]:opt;"
		"cont:float:opt;"
		"gcor:float:opt;"
		"bits:int:opt;"
		"flt:int:opt;"
		"fulls:int:opt;"
		"fulld:int:opt;"
		"logceis:int:opt;"
		"logceid:int:opt;"
		"cpuopt:int:opt;"
		"blacklvl:float:opt;"
		"sceneref:int:opt;"
		"lb:float:opt;"
		"lw:float:opt;"
		"lws:float:opt;"
		"lwd:float:opt;"
		"ambient:float:opt;"
		"match:int:opt;"
		"gy:int:opt;"
		"debug:int:opt;"
		"sig_c:float:opt;"
		"sig_t:float:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Transfer>::create, nullptr, plugin_ptr
	);

	api_ptr->registerFunction ("primaries",
		"clip:vnode;"
		"rs:float[]:opt;"
		"gs:float[]:opt;"
		"bs:float[]:opt;"
		"ws:float[]:opt;"
		"rd:float[]:opt;"
		"gd:float[]:opt;"
		"bd:float[]:opt;"
		"wd:float[]:opt;"
		"prims:data:opt;"
		"primd:data:opt;"
		"cpuopt:int:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Primaries>::create, nullptr, plugin_ptr
	);

#if 0
	api_ptr->registerFunction ("convert",
		"clip:vnode;"
		"w:int:opt;"
		"h:int:opt;"
		"sx:float[]:opt;"
		"sy:float[]:opt;"
		"sw:float[]:opt;"
		"sh:float[]:opt;"
		"scale:float:opt;"
		"scaleh:float:opt;"
		"scalev:float:opt;"
		"kernel:data[]:opt;"
		"kernelh:data[]:opt;"
		"kernelv:data[]:opt;"
		"impulse:float[]:opt;"
		"impulseh:float[]:opt;"
		"impulsev:float[]:opt;"
		"taps:int[]:opt;"
		"tapsh:int[]:opt;"
		"tapsh:int[]:opt;"
		"a1:float[]:opt;"
		"a2:float[]:opt;"
		"a3:float[]:opt;"
		"kovrspl:int[]:opt;"
		"fh:float[]:opt;"
		"fv:float[]:opt;"
		"cnorm:int[]:opt;"
		"totalh:float[]:opt;"
		"totalv:float[]:opt;"
		"invks:int[]:opt;"
		"invksh:int[]:opt;"
		"invksv:int[]:opt;"
		"invkstaps:int[]:opt;"
		"invkstapsh:int[]:opt;"
		"invkstapsv:int[]:opt;"
		"center:int[]:opt;"
		"csp:int:opt;"
		"css:data:opt;"
		"col_fam:int:opt;"
		"bits:int:opt;"
		"flt:int:opt;"
		"dmode:int:opt;"
		"ampo:float:opt;"
		"ampn:float:opt;"
		"dyn:int:opt;"
		"staticnoise:int:opt;"
		"cplace:data:opt;"
		"mat:data:opt;"
		"coef:float[]:opt;"
		"interlaced:int:opt;"
		"tff:int:opt;"
		"useflt:int:opt;"
		"fulls:int:opt;"
		"cplaces:data:opt;"
		"mats:data:opt;"
		"fulld:int:opt;"
		"cplaced:data:opt;"
		"matd:data:opt;"
		"transs:data[]:opt;"
		"transd:data[]:opt;"
		"gcor:float:opt;"
		"cont:float:opt;"
		"cpuopt:int:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Convert>::create, nullptr, plugin_ptr
	);
#endif

	api_ptr->registerFunction ("stack16tonative",
		"clip:vnode;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::Stack16ToNative>::create, nullptr, plugin_ptr
	);

	api_ptr->registerFunction ("nativetostack16",
		"clip:vnode;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <fmtc::NativeToStack16>::create, nullptr, plugin_ptr
	);

//### TEMPORARY ##############################################################
#if defined (main_HISTLUMA)
	api_ptr->registerFunction ("histluma",
		"clip:vnode;"
		"full:int:opt;"
		"amp:int:opt;"
	,	"clip:vnode;"
	,	&vsutl::Redirect <TmpHistLuma>::create, nullptr, plugin_ptr
	);
#endif
//### END OF TEMPORARY #######################################################
}



