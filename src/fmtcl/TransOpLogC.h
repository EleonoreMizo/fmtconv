/*****************************************************************************

        TransOpLogC.h
        Author: Laurent de Soras, 2015

Source:
Harald Brendel,
ALEXA Log C Curve Usage in VFX,
ARRI, 2011-10-05

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpLogC_HEADER_INCLUDED)
#define	fmtcl_TransOpLogC_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/TransOpInterface.h"

#include <array>



namespace fmtcl
{



class TransOpLogC
:	public TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum LType
	{
		LType_LOGC_V3 = 0,
		LType_LOGC_V2,
		LType_VLOG,

		LType_NBR_ELT
	};

	// Exposure Index (EI)
	enum ExpIdx
	{
		ExpIdx_INVALID = -1,

		ExpIdx_160 = 0,
		ExpIdx_200,
		ExpIdx_250,
		ExpIdx_320,
		ExpIdx_400,
		ExpIdx_500,
		ExpIdx_640,
		ExpIdx_800,
		ExpIdx_1000,
		ExpIdx_1280,
		ExpIdx_1600,

		ExpIdx_NBR_ELT
	};

	explicit       TransOpLogC (bool inv_flag, LType type, ExpIdx ei = ExpIdx_800);
	virtual        ~TransOpLogC () {}

	static ExpIdx  conv_logc_ei (int val_raw);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// TransOpInterface
	double         do_convert (double x) const override;
	LinInfo        do_get_info () const override;



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	class CurveData
	{
	public:
		double         _cut;
		double         _a;
		double         _b;
		double         _c;
		double         _d;
		double         _e;
		double         _f;
		double         _cut_i; // _e * _cut + _f
	};

	double         compute_direct (double x) const;
	double         compute_inverse (double x) const;

	const bool     _inv_flag;
	const double   _n;
	const CurveData
	               _curve;

	static const double
		            _noise_margin;
	static const CurveData
	               _vlog;
	static const std::array <CurveData, ExpIdx_NBR_ELT>
	               _v2_table;
	static const std::array <CurveData, ExpIdx_NBR_ELT>
	               _v3_table;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TransOpLogC ()                               = delete;
	               TransOpLogC (const TransOpLogC &other)       = delete;
	TransOpLogC &  operator = (const TransOpLogC &other)        = delete;
	bool           operator == (const TransOpLogC &other) const = delete;
	bool           operator != (const TransOpLogC &other) const = delete;

};	// class TransOpLogC



}	// namespace fmtcl



//#include "fmtcl/TransOpLogC.hpp"



#endif	// fmtcl_TransOpLogC_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
