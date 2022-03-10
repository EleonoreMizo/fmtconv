/*****************************************************************************

        TransOpSigmoid.h
        Author: Laurent de Soras, 2022

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpSigmoid_HEADER_INCLUDED)
#define	fmtcl_TransOpSigmoid_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"



namespace fmtcl
{



class TransOpSigmoid
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       TransOpSigmoid (bool inv_flag, double curve, double thr);
	virtual        ~TransOpSigmoid () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override { return _unbounded; }



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	double         inv_0 (double x) const noexcept;
	double         inv_1 (double x) const noexcept;
	double         dir_0 (double x) const noexcept;
	double         dir_1 (double x) const noexcept;

	bool           _inv_flag;
	double         _c;
	double         _t;
	double         _x0;
	double         _x1;
	double         _d0;
	double         _d1;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpSigmoid ()                               = delete;
	               TransOpSigmoid (const TransOpSigmoid &other)    = delete;
	               TransOpSigmoid (TransOpSigmoid &&other)         = delete;
	TransOpSigmoid &
	               operator = (const TransOpSigmoid &other)        = delete;
	TransOpSigmoid &
	               operator = (TransOpSigmoid &&other)             = delete;
	bool           operator == (const TransOpSigmoid &other) const = delete;
	bool           operator != (const TransOpSigmoid &other) const = delete;

};	// class TransOpSigmoid



}	// namespace fmtcl



//#include "fmtcl/TransOpSigmoid.hpp"



#endif	// fmtcl_TransOpSigmoid_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
