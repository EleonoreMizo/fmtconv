/*****************************************************************************

        Primaries.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtc_Primaries_HEADER_INCLUDED)
#define fmtc_Primaries_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/MatrixProc.h"
#include "vsutl/FilterBase.h"
#include "vsutl/NodeRefSPtr.h"

#include <array>
#include <memory>



namespace fmtc
{



class Primaries
:	public vsutl::FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       Primaries (const ::VSMap &in, ::VSMap &out, void *user_data_ptr, ::VSCore &core, const ::VSAPI &vsapi);
	virtual        ~Primaries () = default;

	// vsutl::FilterBase
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core);
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static const int  NBR_PLANES    = 3;

	typedef std::array <double, NBR_PLANES - 1> Vec2;

	class RGBSystem
	{
	public:
		void           init (const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char r_0 [], const char g_0 [], const char b_0 [], const char w_0 []);
		static void    read_coord_tuple (Vec2 &c, const vsutl::FilterBase &filter, const ::VSMap &in, ::VSMap &out, const char *name_0);
		std::array <Vec2, NBR_PLANES>       // x,y coordinates for R, G and B
		               _rgb;
		Vec2           _white;              // XYZ coordinates for the ref. white
	};

	void           check_colorspace (const ::VSFormat &fmt, const char *inout_0) const;
	fmtcl::Mat3    compute_conversion_matrix () const;
	static fmtcl::Mat3
	               compute_rgb2xyz (const RGBSystem &prim);
	static fmtcl::Mat3
	               compute_chroma_adapt (const RGBSystem &prim_s, const RGBSystem &prim_d);
	static fmtcl::Vec3
	               conv_xy_to_xyz (const Vec2 &xy);

	vsutl::NodeRefSPtr
	               _clip_src_sptr;
	const ::VSVideoInfo             
	               _vi_in;        // Input. Must be declared after _clip_src_sptr because of initialisation order.
	::VSVideoInfo  _vi_out;       // Output. Must be declared after _vi_in.

	bool           _sse_flag;
	bool           _sse2_flag;
	bool           _avx_flag;
	bool           _avx2_flag;

	RGBSystem      _prim_s;
	RGBSystem      _prim_d;

	fmtcl::Mat4    _mat_main;

	std::unique_ptr <fmtcl::MatrixProc>
	               _proc_uptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Primaries ()                               = delete;
	               Primaries (const Primaries &other)         = delete;
	Primaries &    operator = (const Primaries &other)        = delete;
	bool           operator == (const Primaries &other) const = delete;
	bool           operator != (const Primaries &other) const = delete;

}; // class Primaries



}  // namespace fmtc



//#include "fmtc/Primaries.hpp"



#endif   // fmtc_Primaries_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
