/*****************************************************************************

        TransOpInterface.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_TransOpInterface_HEADER_INCLUDED)
#define	fmtcl_TransOpInterface_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



class TransOpInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum class Type
	{
		UNDEF = 0, // Unknown, unspecified or not applicable (but still valid)
		OETF,
		EOTF
	};

	enum class Range
	{
		UNDEF = 0,
		SDR,
		HDR
	};

	// Information about the linear scale
	class LinInfo
	{
	public:
		Type           _type       = Type::UNDEF;
		Range          _range      = Range::UNDEF;

		// Maximum supported linear value, for 16-bit coding. Should be >= 1.0.
		double         _vmax       = 1.0;

		// Reference white level, linear scale. > 0. Set to 1.0 when unknown.
		double         _wref       = 1.0;

		// Luminance corresponding to linear 1.0, in cd/m^2.
		// Not necessarily the peak white nor the reference white.
		// Dedicated to EOTFs, but not mandatory. 0 = unknown/unspecified
		double         _scale_cdm2 = 0;

		// Peak white, in cd/m^2.
		// Dedicated to EOTFs, but not mandatory. 0 = unknown/unspecified
		double         _wpeak_cdm2 = 0;
	};

	// Return this if nothing is known (modifiers)
	static constexpr LinInfo   _unbounded { Type::UNDEF, Range::UNDEF, 1e9, 1, 0, 0 };

	virtual        ~TransOpInterface () {}

	// It is the operator responsibility to clip the input and output
	// (input domain or spec requirement).
	double         operator () (double x) const;

	LinInfo        get_info () const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	virtual double do_convert (double x) const = 0;
	virtual LinInfo
	               do_get_info () const { return { }; }



};	// class TransOpInterface



}	// namespace fmtcl



//#include "fmtcl/TransOpInterface.hpp"



#endif	// fmtcl_TransOpInterface_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
