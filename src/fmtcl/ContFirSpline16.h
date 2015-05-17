/*****************************************************************************

        ContFirSpline16.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ContFirSpline16_HEADER_INCLUDED)
#define	fmtcl_ContFirSpline16_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ContFirInterface.h"



namespace fmtcl
{



class ContFirSpline16
:	public ContFirInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               ContFirSpline16 () {}
	virtual        ~ContFirSpline16 () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// ContFirInterface
	virtual double do_get_support () const;
	virtual double do_get_val (double x) const;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ContFirSpline16 (const ContFirSpline16 &other);
	ContFirSpline16 &
	               operator = (const ContFirSpline16 &other);
	bool           operator == (const ContFirSpline16 &other) const;
	bool           operator != (const ContFirSpline16 &other) const;

};	// class ContFirSpline16



}	// namespace fmtcl



//#include "fmtcl/ContFirSpline16.hpp"



#endif	// fmtcl_ContFirSpline16_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
