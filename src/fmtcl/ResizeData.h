/*****************************************************************************

        ResizeData.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ResizeData_HEADER_INCLUDED)
#define	fmtcl_ResizeData_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/AllocAlign.h"

#include <vector>



namespace fmtcl
{



class ResizeData
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const int  NBR_BUF = 2;

	explicit       ResizeData (int w, int h);
	virtual        ~ResizeData () {}

	template <class T>
	inline T *     use_buf (int index);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	std::vector <float, fstb::AllocAlign <float, 32> >
	               _buf_arr [NBR_BUF];



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ResizeData ()                               = delete;
	               ResizeData (const ResizeData &other)        = delete;
	ResizeData &   operator = (const ResizeData &other)        = delete;
	bool           operator == (const ResizeData &other) const = delete;
	bool           operator != (const ResizeData &other) const = delete;

};	// class ResizeData



}	// namespace fmtcl



#include "fmtcl/ResizeData.hpp"



#endif	// fmtcl_ResizeData_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
