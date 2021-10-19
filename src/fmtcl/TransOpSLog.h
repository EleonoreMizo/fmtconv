/*****************************************************************************

        TransOpSLog.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpSLog_HEADER_INCLUDED)
#define	fmtcl_TransOpSLog_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpSLog
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpSLog (bool inv_flag, bool slog2_flag);
	virtual        ~TransOpSLog () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr double  _a  = 0.037584;
	static constexpr double  _b  = 0.432699;
	static constexpr double  _c1 = 0.616596;
	static constexpr double  _c2 = 0.03;
	static constexpr double  _c  = _c1 + _c2;
	static constexpr double  _d  = 5.0;
	static constexpr double  _s2 = 219.0 / 155.0;

	double         compute_direct (double x) const;
	double         compute_inverse (double x) const;

	const bool     _inv_flag;
	const bool     _slog2_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpSLog ()                               = delete;
	               TransOpSLog (const TransOpSLog &other)       = delete;
	TransOpSLog &  operator = (const TransOpSLog &other)        = delete;
	bool           operator == (const TransOpSLog &other) const = delete;
	bool           operator != (const TransOpSLog &other) const = delete;

};	// class TransOpSLog



}	// namespace fmtcl



//#include "fmtcl/TransOpSLog.hpp"



#endif	// fmtcl_TransOpSLog_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
