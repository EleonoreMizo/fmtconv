/*****************************************************************************

        TransOpLog3G10.h
        Author: Laurent de Soras, 2021

Reference:
https://www.red.com/download/white-paper-on-redwidegamutrgb-and-log3g10

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpLog3G10_HEADER_INCLUDED)
#define fmtcl_TransOpLog3G10_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpLog3G10
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpLog3G10 (bool inv_flag);



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

	static constexpr double _a       =   0.224282;
	static constexpr double _b       = 155.975327;
	static constexpr double _c       =   0.01;
	static constexpr double _g       =  15.1927;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpLog3G10 ()                               = delete;
	               TransOpLog3G10 (const TransOpLog3G10 &other)    = delete;
	               TransOpLog3G10 (TransOpLog3G10 &&other)         = delete;
	TransOpLog3G10 &
	               operator = (const TransOpLog3G10 &other)        = delete;
	TransOpLog3G10 &
	               operator = (TransOpLog3G10 &&other)             = delete;
	bool           operator == (const TransOpLog3G10 &other) const = delete;
	bool           operator != (const TransOpLog3G10 &other) const = delete;

}; // class TransOpLog3G10



}  // namespace fmtcl



//#include "fmtcl/TransOpLog3G10.hpp"



#endif   // fmtcl_TransOpLog3G10_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
