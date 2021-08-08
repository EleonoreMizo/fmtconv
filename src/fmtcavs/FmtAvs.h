/*****************************************************************************

        FmtAvs.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcavs_FmtAvs_HEADER_INCLUDED)
#define fmtcavs_FmtAvs_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ColorFamily.h"

#include <string>



struct VideoInfo;

namespace fmtcavs
{



class FmtAvs
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       FmtAvs (std::string fmt_str);
	explicit       FmtAvs (const VideoInfo &vi) noexcept;
	               FmtAvs ()                        = default;
	               FmtAvs (const FmtAvs &other)     = default;
	               FmtAvs (FmtAvs &&other)          = default;
	               ~FmtAvs ()                       = default;
	FmtAvs &       operator = (const FmtAvs &other) = default;
	FmtAvs &       operator = (FmtAvs &&other)      = default;

	void           invalidate () noexcept;
	int            conv_from_str (std::string fmt_str);
	void           conv_from_vi (const VideoInfo &vi);
	int            conv_to_vi (VideoInfo &vi);

	bool           is_valid () const noexcept;

	void           set_bitdepth (int bitdepth) noexcept;
	int            get_bitdepth () const noexcept;
	bool           is_float () const noexcept;
	void           set_col_fam (fmtcl::ColorFamily col_fam) noexcept;
	fmtcl::ColorFamily
	               get_col_fam () const noexcept;
	bool           is_planar () const noexcept;
	bool           has_alpha () const noexcept;
	int            get_subspl_h () const noexcept;
	int            get_subspl_v () const noexcept;

	int            get_nbr_comp_non_alpha () const noexcept;

	static bool    is_bitdepth_valid (int bitdepth) noexcept;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static std::string
	               remove_outer_spaces (std::string str);
	static bool    is_eq_leftstr_and_eat (std::string &str, std::string stest);
	static bool    check_planar_bits_and_eat (std::string &str, int &res);
	static int     check_bits_and_eat (std::string &str, bool allow_s_flag);

	int            _bitdepth    = -1;
	fmtcl::ColorFamily
	               _col_fam     = fmtcl::ColorFamily_INVALID;
	bool           _planar_flag = false;   // Y formats are considered planar
	bool           _alpha_flag  = false;
	int            _subspl_h    = -1;      // Bitshift, >= 0. Negative: invalid
	int            _subspl_v    = -1;      // Same



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const FmtAvs &other) const = delete;
	bool           operator != (const FmtAvs &other) const = delete;

}; // class FmtAvs



}  // namespace fmtcavs



//#include "fmtcavs/FmtAvs.hpp"



#endif   // fmtcavs_FmtAvs_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
