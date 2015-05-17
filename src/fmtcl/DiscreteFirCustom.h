/*****************************************************************************

        DiscreteFirCustom.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_DiscreteFirCustom_HEADER_INCLUDED)
#define	fmtcl_DiscreteFirCustom_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/DiscreteFirInterface.h"

#include <vector>



namespace fmtcl
{



class DiscreteFirCustom
:	public DiscreteFirInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       DiscreteFirCustom (double ovrspl, double coef_ptr [], int len);
	explicit       DiscreteFirCustom (double ovrspl, const std::vector <double> &coef_arr);
	virtual        ~DiscreteFirCustom () {}

	int            compute_real_support () const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// DiscreteFirInterface
	virtual int    do_get_len () const;
	virtual double do_get_ovrspl () const;
	virtual double do_get_val (int pos) const;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	std::vector <double>
	               _coef_arr;
	double         _ovrspl;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               DiscreteFirCustom (const DiscreteFirCustom &other);
	DiscreteFirCustom &
	               operator = (const DiscreteFirCustom &other);
	bool           operator == (const DiscreteFirCustom &other) const;
	bool           operator != (const DiscreteFirCustom &other) const;

};	// class DiscreteFirCustom



}	// namespace fmtcl



//#include "fmtcl/DiscreteFirCustom.hpp"



#endif	// fmtcl_DiscreteFirCustom_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
