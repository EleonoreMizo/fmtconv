/*****************************************************************************

        ResizeDataFactory.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ResizeDataFactory_HEADER_INCLUDED)
#define	fmtcl_ResizeDataFactory_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "conc/ObjFactoryInterface.h"



namespace fmtcl
{



class ResizeData;

class ResizeDataFactory
:	public conc::ObjFactoryInterface <ResizeData>
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       ResizeDataFactory (int w, int h);
	virtual        ~ResizeDataFactory () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// conc::ObjFactoryInterface
	virtual ResizeData *
	               do_create ();



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	int            _w;
	int            _h;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ResizeDataFactory ()                               = delete;
	               ResizeDataFactory (const ResizeDataFactory &other) = delete;
	ResizeDataFactory &
	               operator = (const ResizeDataFactory &other)        = delete;
	bool           operator == (const ResizeDataFactory &other) const = delete;
	bool           operator != (const ResizeDataFactory &other) const = delete;

};	// class ResizeDataFactory



}	// namespace fmtcl



//#include "fmtcl/ResizeDataFactory.hpp"



#endif	// fmtcl_ResizeDataFactory_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
