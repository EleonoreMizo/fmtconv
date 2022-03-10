/*****************************************************************************

        TransModel.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransModel_HEADER_INCLUDED)
#define fmtcl_TransModel_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Frame.h"
#include "fmtcl/FrameRO.h"
#include "fmtcl/GammaY.h"
#include "fmtcl/LumMatch.h"
#include "fmtcl/PicFmt.h"
#include "fmtcl/ProcComp3Arg.h"
#include "fmtcl/TransCurve.h"
#include "fmtcl/TransLut.h"
#include "fmtcl/TransOpLogC.h"
#include "fmtcl/TransUtil.h"

#include <memory>

#include <cstdint>



namespace fmtcl
{



class TransModel
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr int _max_nbr_planes = ProcComp3Arg::_nbr_planes;

	// Minimum accepted luminance in cd/m^2 for peak luminance or ambient
	// surround luminance. Always > 0.
	static constexpr double _min_luminance = 0.1;

	typedef TransUtil::OpSPtr OpSPtr;

	enum class GyProc
	{
		UNDEF = -1,
		OFF   = 0,
		ON
	};

	explicit       TransModel (PicFmt dst_fmt, TransCurve curve_d, TransOpLogC::ExpIdx logc_ei_d, PicFmt src_fmt, TransCurve curve_s, TransOpLogC::ExpIdx logc_ei_s, double contrast, double gcor, double lb, double lws, double lwd, double lamb, bool scene_flag, LumMatch match, GyProc gy_proc, double sig_curve, double sig_thr, bool sse2_flag, bool avx2_flag);

	const std::string &
	               get_debug_text () const noexcept;
	void           process_frame (const ProcComp3Arg &arg) const noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	// Maximum segment length in bytes. Multiple of 64.
	static constexpr int _max_seg_len = 4096;

	enum class Proc
	{
		DIRECT = 0, // LUT S only
		SG,         // LUT S -> Gamma Y
		GD,         //          Gamma Y -> LUT D
		SGD         // LUT S -> Gamma Y -> LUT D
	};

	typedef std::array <uint8_t, _max_seg_len> Segment;
	typedef std::array <Segment, _max_nbr_planes> SegArray;

	void           process_frame_direct (const ProcComp3Arg &arg) const noexcept;
	void           process_frame_sg (const ProcComp3Arg &arg) const noexcept;
	void           process_frame_gd (const ProcComp3Arg &arg) const noexcept;
	void           process_frame_sgd (const ProcComp3Arg &arg) const noexcept;

	static void    estimate_lw (double &lw, const TransOpInterface::LinInfo &info);
	static OpSPtr  compose (OpSPtr op_1_sptr, OpSPtr op_2_sptr);
	static OpSPtr  build_pq_ootf ();
	static OpSPtr  build_pq_ootf_inv ();
	static double  compute_pq_sceneref_range_709 ();

	Proc           _proc_mode  = Proc::DIRECT;
	int            _max_len    = 0; // Pixels
	int            _nbr_planes = _max_nbr_planes;

	// At least one of these functions must be populated
	std::unique_ptr <TransLut>
	               _lut_s_uptr;
	std::unique_ptr <GammaY>
	               _gamma_y_uptr;
	std::unique_ptr <TransLut>
	               _lut_d_uptr;

	// Contains debugging information about what is done
	std::string    _dbg_txt;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransModel ()                               = delete;
	               TransModel (const TransModel &other)        = delete;
	               TransModel (TransModel &&other)             = delete;
	TransModel &   operator = (const TransModel &other)        = delete;
	TransModel &   operator = (TransModel &&other)             = delete;
	bool           operator == (const TransModel &other) const = delete;
	bool           operator != (const TransModel &other) const = delete;

}; // class TransModel



}  // namespace fmtcl



//#include "fmtcl/TransModel.hpp"



#endif   // fmtcl_TransModel_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
