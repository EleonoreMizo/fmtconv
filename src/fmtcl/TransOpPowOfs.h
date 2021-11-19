/*****************************************************************************

        TransOpPowOfs.h
        Author: Laurent de Soras, 2021

Reference:
https://github.com/imageworks/OpenColorIO-Configs/blob/master/nuke-default/make.py

Gamma to lin is in the form:

f (x) = (pow (10, (m * x - o) / d) - l) / (1 - l)
w/  l =  pow (10, (b     - o) / d)

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/

#include "fmtcl/TransOpInterface.h"



#pragma once
#if ! defined (fmtcl_TransOpPowOfs_HEADER_INCLUDED)
#define fmtcl_TransOpPowOfs_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpPowOfs
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpPowOfs (bool inv_flag, double m, double w, double d, double b);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	inline double  gamma_to_lin (double x) const noexcept;
	inline double  lin_to_gamma (double x) const noexcept;

	bool           _inv_flag = false;
	double         _kx       = 1;
	double         _kw       = 0;
	double         _kl       = 1;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpPowOfs ()                               = delete;
	               TransOpPowOfs (const TransOpPowOfs &other)     = delete;
	               TransOpPowOfs (TransOpPowOfs &&other)          = delete;
	TransOpPowOfs& operator = (const TransOpPowOfs &other)        = delete;
	TransOpPowOfs& operator = (TransOpPowOfs &&other)             = delete;
	bool           operator == (const TransOpPowOfs &other) const = delete;
	bool           operator != (const TransOpPowOfs &other) const = delete;

}; // class TransOpPowOfs



}  // namespace fmtcl



//#include "fmtcl/TransOpPowOfs.hpp"



#endif   // fmtcl_TransOpPowOfs_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
