/*****************************************************************************

        TransOpHlg.h
        Author: Laurent de Soras, 2019

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpHlg_HEADER_INCLUDED)
#define	fmtcl_TransOpHlg_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpHlg
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpHlg (bool inv_flag);
	virtual        ~TransOpHlg () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	double         compute_direct (double x) const;
	double         compute_inverse (double x) const;

	const bool     _inv_flag;

	static const double  _a;
	static const double  _b;
	static const double  _c;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpHlg ()                               = delete;
	               TransOpHlg (const TransOpHlg &other)        = delete;
	TransOpHlg &   operator = (const TransOpHlg &other)        = delete;
	bool           operator == (const TransOpHlg &other) const = delete;
	bool           operator != (const TransOpHlg &other) const = delete;

};	// class TransOpHlg



}	// namespace fmtcl



//#include "fmtcl/TransOpHlg.hpp"



#endif	// fmtcl_TransOpHlg_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
