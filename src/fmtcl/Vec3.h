/*****************************************************************************

        Vec3.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Vec3_HEADER_INCLUDED)
#define	fmtcl_Vec3_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <array>



namespace fmtcl
{



class Vec3
:	public std::array <double, 3>
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef std::array <double, 3> Inherited;

	static const int  VECT_SIZE = 3;

	               Vec3 ()                        = default;
	constexpr      Vec3 (const Vec3 &other)       = default;
	               Vec3 (Vec3 &&other)            = default;
	inline constexpr explicit
	               Vec3 (double c0, double c1, double c2);
	inline constexpr explicit
	               Vec3 (const double arr [VECT_SIZE]);
	               ~Vec3 ()                       = default;

	Vec3 &         operator = (const Vec3 &other) = default;
	Vec3 &         operator = (Vec3 &&other)      = default;

	inline bool    operator == (const Vec3 &other) const;
	inline bool    operator != (const Vec3 &other) const;

	inline Vec3 &  operator += (const Vec3 &other);
	inline Vec3 &  operator -= (const Vec3 &other);
	inline Vec3 &  operator *= (double scalar);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



};	// class Vec3



inline Vec3	operator + (const Vec3 &lhs, const Vec3 &rhs);
inline Vec3	operator - (const Vec3 &lhs, const Vec3 &rhs);
inline Vec3	operator * (const Vec3 &lhs, double rhs);



}	// namespace fmtcl



#include "fmtcl/Vec3.hpp"



#endif	// fmtcl_Vec3_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
