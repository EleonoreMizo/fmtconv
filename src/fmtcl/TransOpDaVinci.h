/*****************************************************************************

        TransOpDaVinci.h
        Author: Laurent de Soras, 2021

Reference:
https://documents.blackmagicdesign.com/InformationNotes/DaVinci_Resolve_17_Wide_Gamut_Intermediate.pdf

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpDaVinci_HEADER_INCLUDED)
#define fmtcl_TransOpDaVinci_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpDaVinci
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpDaVinci (bool inv_flag);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           _inv_flag = false;

	static constexpr double _a       =  0.0075;
	static constexpr double _b       =  7.0;
	static constexpr double _c       =  0.07329248;
	static constexpr double _m       = 10.44426855;
	static constexpr double _cut_lin =  0.00262409;
	static constexpr double _cut_log =  0.02740668;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpDaVinci ()                               = delete;
	               TransOpDaVinci (const TransOpDaVinci &other)    = delete;
	               TransOpDaVinci (TransOpDaVinci &&other)         = delete;
	TransOpDaVinci &
	               operator = (const TransOpDaVinci &other)        = delete;
	TransOpDaVinci &
	               operator = (TransOpDaVinci &&other)             = delete;
	bool           operator == (const TransOpDaVinci &other) const = delete;
	bool           operator != (const TransOpDaVinci &other) const = delete;

}; // class TransOpDaVinci



}  // namespace fmtcl



//#include "fmtcl/TransOpDaVinci.hpp"



#endif   // fmtcl_TransOpDaVinci_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
