/*****************************************************************************

        Mat3.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Mat3_HEADER_INCLUDED)
#define	fmtcl_Mat3_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/Vec3.h"

#include <array>



namespace fmtcl
{



class Mat3
:	public std::array <Vec3, Vec3::VECT_SIZE>
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef std::array <Vec3, Vec3::VECT_SIZE> Inherited;

	static const int  VECT_SIZE = Vec3::VECT_SIZE;

	enum Preset
	{
		Preset_SOLID = 0,
		Preset_DIAGONAL,

		Preset_NBR_ELT
	};

	               Mat3 ()                        = default;
	inline explicit
	               Mat3 (double filler, Preset preset = Preset_SOLID);
	inline constexpr
	               Mat3 (const double content [VECT_SIZE] [VECT_SIZE]);
	inline constexpr
	               Mat3 (const Vec3 &v0, const Vec3 &v1, const Vec3 &v2);
	inline constexpr
	               Mat3 (const Mat3 &other)       = default;
	inline         Mat3 (Mat3 &&other)            = default;
	               ~Mat3 ()                       = default;

	inline Mat3 &  operator = (const Mat3 &other) = default;
	inline Mat3 &  operator = (Mat3 &&other)      = default;

	inline bool    operator == (const Mat3 &other) const;
	inline bool    operator != (const Mat3 &other) const;

	inline Mat3 &  operator += (const Mat3 &other);
	inline Mat3 &  operator -= (const Mat3 &other);
	inline Mat3 &  operator *= (const Mat3 &other);
	inline Mat3 &  operator *= (double scale);

	inline constexpr double
	               det () const;
	inline constexpr Mat3
	               compute_inverse () const;
	inline Mat3 &  invert ();

	inline Mat3 &  set_col (int pos, const Vec3 &other);
	inline Vec3    get_col (int pos) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:


};	// class Mat3



inline Mat3   operator + (const Mat3 &lhs, const Mat3 &rhs);
inline Mat3   operator - (const Mat3 &lhs, const Mat3 &rhs);
inline Mat3   operator * (const Mat3 &lhs, const Mat3 &rhs);
inline Vec3   operator * (const Mat3 &lhs, const Vec3 &rhs);
inline Mat3   operator * (const Mat3 &lhs, double rhs);



}	// namespace fmtcl



#include "fmtcl/Mat3.hpp"



#endif	// fmtcl_Mat3_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
