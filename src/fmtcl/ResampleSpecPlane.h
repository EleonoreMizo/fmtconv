/*****************************************************************************

        ResampleSpecPlane.h
        Author: Laurent de Soras, 2014

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ResampleSpecPlane_HEADER_INCLUDED)
#define	fmtcl_ResampleSpecPlane_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cstdint>



namespace fmtcl
{



class ResampleSpecPlane
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ResampleSpecPlane ()                               = default;
	               ResampleSpecPlane (const ResampleSpecPlane &other) = default;
	ResampleSpecPlane &
	               operator = (const ResampleSpecPlane &other)        = default;

	bool           operator < (const ResampleSpecPlane &other) const;

	int            _src_width;
	int            _src_height;
	int            _dst_width;
	int            _dst_height;
	double         _win_x;
	double         _win_y;
	double         _win_w;
	double         _win_h;
	double         _center_pos_src_h;
	double         _center_pos_src_v;
	double         _center_pos_dst_h;
	double         _center_pos_dst_v;
	double         _kernel_scale_h;
	double         _kernel_scale_v;
	double         _add_cst;
	uint32_t       _kernel_hash_h;
	uint32_t       _kernel_hash_v;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           operator == (const ResampleSpecPlane &other) const = delete;
	bool           operator != (const ResampleSpecPlane &other) const = delete;

};	// class ResampleSpecPlane



}	// namespace fmtcl



//#include "fmtcl/ResampleSpecPlane.hpp"



#endif	// fmtcl_ResampleSpecPlane_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
