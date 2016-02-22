/*****************************************************************************

        TransOpLogC.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpLogC_HEADER_INCLUDED)
#define	fmtcl_TransOpLogC_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpLogC
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpLogC (bool inv_flag, bool v2_flag);
	virtual        ~TransOpLogC () {}

	// TransOpInterface
	virtual double operator () (double x) const;
	virtual double get_max () const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	double         compute_direct (double x) const;
	double         compute_inverse (double x) const;

	const bool     _inv_flag;
	const bool     _v2_flag;
	double         _cut;
	double         _a;
	double         _b;
	double         _c;
	double         _d;
	double         _e;
	double         _f;
	double         _cut_i;

	static const double
		            _noise_margin;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpLogC ()                               = delete;
	               TransOpLogC (const TransOpLogC &other)       = delete;
	TransOpLogC &  operator = (const TransOpLogC &other)        = delete;
	bool           operator == (const TransOpLogC &other) const = delete;
	bool           operator != (const TransOpLogC &other) const = delete;

};	// class TransOpLogC



}	// namespace fmtcl



//#include "fmtcl/TransOpLogC.hpp"



#endif	// fmtcl_TransOpLogC_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
