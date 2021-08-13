/*****************************************************************************

        ResamplePlaneData.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_ResamplePlaneData_HEADER_INCLUDED)
#define fmtcl_ResamplePlaneData_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/FilterResize.h"
#include "fmtcl/InterlacingType.h"
#include "fmtcl/KernelData.h"
#include "fmtcl/ResampleSpecPlane.h"

#include <array>



namespace fmtcl
{



class ResamplePlaneData
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ResamplePlaneData ()                          = default;
	               ~ResamplePlaneData ()                         = default;
	               ResamplePlaneData (ResamplePlaneData &&other) = default;
	ResamplePlaneData &
	               operator = (ResamplePlaneData &&other)        = default;

	// Array order: [dest] [src]
	typedef std::array <ResampleSpecPlane, InterlacingType_NBR_ELT> SpecSrcArray;
	typedef std::array <SpecSrcArray,      InterlacingType_NBR_ELT> SpecArray;

	class Win
	{
	public:
		// Data is in full coordinates whatever the plane (never subsampled)
		double         _x = 0;
		double         _y = 0;
		double         _w = 0;
		double         _h = 0;
	};

	typedef std::array <
		KernelData,
		FilterResize::Dir_NBR_ELT
	>  KernelArray;

	Win            _win;
	SpecArray      _spec_arr;        // Contains the spec (used as a key) for each plane/interlacing combination
	KernelArray    _kernel_arr;
	double         _kernel_scale_h = 1;  // Can be negative (forced scaling)
	double         _kernel_scale_v = 1;  // Can be negative (forced scaling)
	double         _norm_val_h     = 0;  // > 0, 0 = kernel default
	double         _norm_val_v     = 0;  // > 0, 0 = kernel default
	double         _gain           = 1;
	double         _add_cst        = 0;
	bool           _preserve_center_flag = true;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ResamplePlaneData (const ResamplePlaneData &other) = delete;
	ResamplePlaneData &
	               operator = (const ResamplePlaneData &other)        = delete;
	bool           operator == (const ResamplePlaneData &other) const = delete;
	bool           operator != (const ResamplePlaneData &other) const = delete;

}; // class ResamplePlaneData



}  // namespace fmtcl



//#include "fmtcl/ResamplePlaneData.hpp"



#endif   // fmtcl_ResamplePlaneData_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
