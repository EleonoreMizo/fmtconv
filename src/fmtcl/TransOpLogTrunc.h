/*****************************************************************************

        TransOpLogTrunc.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpLogTrunc_HEADER_INCLUDED)
#define	fmtcl_TransOpLogTrunc_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpLogTrunc
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpLogTrunc (bool inv_flag, double alpha, double beta);
	virtual        ~TransOpLogTrunc () {}

	// TransOpInterface
	virtual double operator () (double x) const;
	virtual double get_max () const { return (1.0); }



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	const bool     _inv_flag;
	const double   _alpha;
	const double   _beta;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpLogTrunc ();
	               TransOpLogTrunc (const TransOpLogTrunc &other);
	TransOpLogTrunc &
	               operator = (const TransOpLogTrunc &other);
	bool           operator == (const TransOpLogTrunc &other) const;
	bool           operator != (const TransOpLogTrunc &other) const;

};	// class TransOpLogTrunc



}	// namespace fmtcl



//#include "fmtcl/TransOpLogTrunc.hpp"



#endif	// fmtcl_TransOpLogTrunc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
