/*****************************************************************************

        ErrDifBufFactory.h
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ErrDifBufFactory_HEADER_INCLUDED)
#define	fmtcl_ErrDifBufFactory_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "conc/ObjFactoryInterface.h"
#include "fmtcl/ErrDifBuf.h"



namespace fmtcl
{



class ErrDifBufFactory
:	public conc::ObjFactoryInterface <ErrDifBuf>
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       ErrDifBufFactory (long width);
	virtual        ~ErrDifBufFactory () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// conc::ObjFactoryInterface
	virtual ErrDifBuf *
	               do_create ();



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	long           _width;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ErrDifBufFactory ()                              = delete;
	               ErrDifBufFactory (const ErrDifBufFactory &other) = delete;
	ErrDifBufFactory &
	               operator = (const ErrDifBufFactory &other)        = delete;
	bool           operator == (const ErrDifBufFactory &other) const = delete;
	bool           operator != (const ErrDifBufFactory &other) const = delete;

};	// class ErrDifBufFactory



}	// namespace fmtcl



//#include "fmtcl/ErrDifBufFactory.hpp"



#endif	// fmtcl_ErrDifBufFactory_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
