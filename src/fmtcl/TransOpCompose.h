/*****************************************************************************

        TransOpCompose.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpCompose_HEADER_INCLUDED)
#define	fmtcl_TransOpCompose_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"

#include <memory>



namespace fmtcl
{



class TransOpCompose
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef  std::shared_ptr <TransOpInterface> OpSPtr;

	explicit inline
	               TransOpCompose (OpSPtr op_1_sptr, OpSPtr op_2_sptr);
	virtual        ~TransOpCompose () {}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	inline double  do_convert (double x) const override;
	LinInfo        do_get_info () const override { return _unbounded; }



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

		OpSPtr         _op_1_sptr;
		OpSPtr         _op_2_sptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpCompose ()                               = delete;
	               TransOpCompose (const TransOpCompose &other)    = delete;
	TransOpCompose &
	               operator = (const TransOpCompose &other)        = delete;
	bool           operator == (const TransOpCompose &other) const = delete;
	bool           operator != (const TransOpCompose &other) const = delete;

};	// class TransOpCompose



}	// namespace fmtcl



#include "fmtcl/TransOpCompose.hpp"



#endif	// fmtcl_TransOpCompose_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
