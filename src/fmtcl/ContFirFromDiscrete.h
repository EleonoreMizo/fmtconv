/*****************************************************************************

        ContFirFromDiscrete.h
        Author: Laurent de Soras, 2011

Linearly interpolated.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ContFirFromDiscrete_HEADER_INCLUDED)
#define	fmtcl_ContFirFromDiscrete_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ContFirInterface.h"



namespace fmtcl
{



class DiscreteFirInterface;



class ContFirFromDiscrete
:	public ContFirInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       ContFirFromDiscrete (const DiscreteFirInterface &discrete);
	virtual        ~ContFirFromDiscrete () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// ContFirInterface
	virtual double do_get_support () const;
	virtual double do_get_val (double x) const;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	const DiscreteFirInterface &
	               _discrete;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ContFirFromDiscrete ();
	               ContFirFromDiscrete (const ContFirFromDiscrete &other);
	ContFirFromDiscrete &
	               operator = (const ContFirFromDiscrete &other);
	bool           operator == (const ContFirFromDiscrete &other) const;
	bool           operator != (const ContFirFromDiscrete &other) const;

};	// class ContFirFromDiscrete



}	// namespace fmtcl



//#include "fmtcl/ContFirFromDiscrete.hpp"



#endif	// fmtcl_ContFirFromDiscrete_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
