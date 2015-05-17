/*****************************************************************************

        ContFirBlackman.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ContFirBlackman_HEADER_INCLUDED)
#define	fmtcl_ContFirBlackman_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ContFirInterface.h"



namespace fmtcl
{



class ContFirBlackman
:	public ContFirInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       ContFirBlackman (int taps);
	virtual        ~ContFirBlackman () {}

	double         compute_win_coef (double x) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// ContFirInterface
	virtual double do_get_support () const;
	virtual double do_get_val (double x) const;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	inline double  compute_win_coef_no_check (double x) const;

	int            _taps;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ContFirBlackman ();
	               ContFirBlackman (const ContFirBlackman &other);
	ContFirBlackman &
	               operator = (const ContFirBlackman &other);
	bool           operator == (const ContFirBlackman &other) const;
	bool           operator != (const ContFirBlackman &other) const;

};	// class ContFirBlackman



}	// namespace fmtcl



//#include "fmtcl/ContFirBlackman.hpp"



#endif	// fmtcl_ContFirBlackman_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
