/*****************************************************************************

        Vec3.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Vec3_CODEHEADER_INCLUDED)
#define	fmtcl_Vec3_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr Vec3::Vec3 (double c0, double c1, double c2)
:	Inherited ({ { c0, c1, c2 } })
{
	// Nothing
}



constexpr Vec3::Vec3 (const double arr [VECT_SIZE])
:	Inherited ({ { arr [0], arr [1], arr [2] } })
{
#if 0
	for (int x = 0; x < VECT_SIZE; ++x)
	{
		(*this) [x] = arr [x];
	}
#endif
}



bool	Vec3::operator == (const Vec3 &other) const
{
	for (int x = 0; x < VECT_SIZE; ++x)
	{
		if ((*this) [x] != other [x])
		{
			return false;
		}
	}

	return true;
}



bool	Vec3::operator != (const Vec3 &other) const
{
	return ! (*this == other);
}



Vec3 &	Vec3::operator += (const Vec3 &other)
{
	for (int x = 0; x < VECT_SIZE; ++x)
	{
		(*this) [x] += other [x];
	}

	return *this;
}



Vec3 &	Vec3::operator -= (const Vec3 &other)
{
	for (int x = 0; x < VECT_SIZE; ++x)
	{
		(*this) [x] -= other [x];
	}

	return *this;
}



Vec3 &	Vec3::operator *= (double scalar)
{
	for (int x = 0; x < VECT_SIZE; ++x)
	{
		(*this) [x] *= scalar;
	}

	return *this;
}



Vec3	operator + (const Vec3 &lhs, const Vec3 &rhs)
{
	return Vec3 (lhs) += rhs;
}



Vec3	operator - (const Vec3 &lhs, const Vec3 &rhs)
{
	return Vec3 (lhs) -= rhs;
}



Vec3	operator * (const Vec3 &lhs, double rhs)
{
	return Vec3 (lhs) *= rhs;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_Vec3_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
