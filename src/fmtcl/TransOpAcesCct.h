/*****************************************************************************

        TransOpAcesCct.h
        Author: Laurent de Soras, 2022

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpAcesCct_HEADER_INCLUDED)
#define	fmtcl_TransOpAcesCct_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpAcesCct
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpAcesCct (bool inv_flag);
	virtual        ~TransOpAcesCct () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr double _max_val = 65504.0;

	const bool     _inv_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpAcesCct ()                               = delete;
	               TransOpAcesCct (const TransOpAcesCct &other)    = delete;
	               TransOpAcesCct (TransOpAcesCct &&other)         = delete;
	TransOpAcesCct &
	               operator = (const TransOpAcesCct &other)        = delete;
	TransOpAcesCct &
	               operator = (TransOpAcesCct &&other)             = delete;
	bool           operator == (const TransOpAcesCct &other) const = delete;
	bool           operator != (const TransOpAcesCct &other) const = delete;

};	// class TransOpAcesCct



}	// namespace fmtcl



//#include "fmtcl/TransOpAcesCct.hpp"



#endif	// fmtcl_TransOpAcesCct_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
