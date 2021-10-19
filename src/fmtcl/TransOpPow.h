/*****************************************************************************

        TransOpPow.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpPow_HEADER_INCLUDED)
#define	fmtcl_TransOpPow_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpPow
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpPow (bool inv_flag, double p_i, double alpha = 1, double val_max = 1, double scale_cdm2 = 0, double wpeak_cdm2 = 0);
	virtual        ~TransOpPow () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	const bool     _inv_flag;
	const double   _p_i;
	const double   _alpha;
	const double   _p;
	const double   _val_max;      // linear
	const double   _scale_cdm2;   // Value in cd/m^2 for linear 1.0. 0 = unknown
	const double   _wpeak_cdm2;   // Value in cd/m^2 for linear 1.0. 0 = unknown



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpPow ()                               = delete;
	               TransOpPow (const TransOpPow &other)        = delete;
	TransOpPow &   operator = (const TransOpPow &other)        = delete;
	bool           operator == (const TransOpPow &other) const = delete;
	bool           operator != (const TransOpPow &other) const = delete;

};	// class TransOpPow



}	// namespace fmtcl



//#include "fmtcl/TransOpPow.hpp"



#endif	// fmtcl_TransOpPow_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
