/*****************************************************************************

        ContFirSpline36.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ContFirSpline36_HEADER_INCLUDED)
#define	fmtcl_ContFirSpline36_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ContFirInterface.h"



namespace fmtcl
{



class ContFirSpline36
:	public ContFirInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ContFirSpline36 () {}
	virtual        ~ContFirSpline36 () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// ContFirInterface
	virtual double do_get_support () const;
	virtual double do_get_val (double x) const;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ContFirSpline36 (const ContFirSpline36 &other);
	ContFirSpline36 &
	               operator = (const ContFirSpline36 &other);
	bool           operator == (const ContFirSpline36 &other) const;
	bool           operator != (const ContFirSpline36 &other) const;

};	// class ContFirSpline36



//#include "fmtcl/ContFirSpline36.hpp"



}	// namespace fmtcl



#endif	// fmtcl_ContFirSpline36_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
