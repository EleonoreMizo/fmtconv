/*****************************************************************************

        Mat3.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Mat3_CODEHEADER_INCLUDED)
#define	fmtcl_Mat3_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Mat3::Mat3 (double filler, Preset preset)
{
	assert (preset >= 0);
	assert (preset < Preset_NBR_ELT);

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			if (preset == Preset_DIAGONAL && x != y)
			{
				(*this) [y] [x] = 0;
			}
			else
			{
				(*this) [y] [x] = filler;
			}
		}
	}
}



constexpr Mat3::Mat3 (const double content [VECT_SIZE] [VECT_SIZE])
:	Inherited ({{ Vec3 { content [0] }, Vec3 { content [1] }, Vec3 { content [2] } }})
{
	assert (content != nullptr);
#if 0

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			(*this) [y] [x] = content [y] [x];
		}
	}
#endif
}



constexpr Mat3::Mat3 (const Vec3 &v0, const Vec3 &v1, const Vec3 &v2)
:	Inherited ({ { v0, v1, v2 } })
{
	// Nothing
}



bool	Mat3::operator == (const Mat3 &other) const
{
	for (int y = 0; y < VECT_SIZE; ++y)
	{
		if ((*this) [y] != other [y])
		{
			return false;
		}
	}

	return true;
}



bool	Mat3::operator != (const Mat3 &other) const
{
	return ! (*this == other);
}



Mat3 &	Mat3::operator += (const Mat3 &other)
{
	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			(*this) [y] [x] += other [y] [x];
		}
	}

	return *this;
}



Mat3 &	Mat3::operator -= (const Mat3 &other)
{
	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			(*this) [y] [x] -= other [y] [x];
		}
	}

	return *this;
}



Mat3 &	Mat3::operator *= (const Mat3 &other)
{
	*this = *this * other;

	return *this;
}



Mat3 &	Mat3::operator *= (double scale)
{
	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			(*this) [y] [x] *= scale;
		}
	}

	return *this;
}



constexpr double	Mat3::det () const
{
	const Mat3 &   m = *this;
	return
		  m [0] [0] * m [1] [1] * m [2] [2]
		+ m [1] [0] * m [2] [1] * m [0] [2]
		+ m [2] [0] * m [0] [1] * m [1] [2]
		- m [0] [0] * m [2] [1] * m [1] [2]
		- m [1] [0] * m [0] [1] * m [2] [2]
		- m [2] [0] * m [1] [1] * m [0] [2];
}



constexpr Mat3	Mat3::compute_inverse () const
{
	const double   d3 = det ();
	assert (d3 != 0);
	const Mat3 &   m = *this;

	return {
		Vec3 {
			(m [1] [1] * m [2] [2] - m [1] [2] * m [2] [1]) / d3,
			(m [0] [2] * m [2] [1] - m [0] [1] * m [2] [2]) / d3,
			(m [0] [1] * m [1] [2] - m [0] [2] * m [1] [1]) / d3
		},
		Vec3 {
			(m [1] [2] * m [2] [0] - m [1] [0] * m [2] [2]) / d3,
			(m [0] [0] * m [2] [2] - m [0] [2] * m [2] [0]) / d3,
			(m [0] [2] * m [1] [0] - m [0] [0] * m [1] [2]) / d3
		},
		Vec3 {
			(m [1] [0] * m [2] [1] - m [1] [1] * m [2] [0]) / d3,
			(m [0] [1] * m [2] [0] - m [0] [0] * m [2] [1]) / d3,
			(m [0] [0] * m [1] [1] - m [0] [1] * m [1] [0]) / d3
		}
	};
}



Mat3 &	Mat3::invert ()
{
	*this = compute_inverse ();

	return *this;
}



Mat3 &	Mat3::set_col (int pos, const Vec3 &other)
{
	assert (pos >= 0);
	assert (pos < VECT_SIZE);

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		(*this) [y] [pos] = other [y];
	}

	return *this;
}



Vec3	Mat3::get_col (int pos) const
{
	assert (pos >= 0);
	assert (pos < VECT_SIZE);

	Vec3           tmp;
	for (int y = 0; y < VECT_SIZE; ++y)
	{
		tmp [y] = (*this) [y] [pos];
	}

	return tmp;
}



Mat3	operator + (const Mat3 &lhs, const Mat3 &rhs)
{
	return (Mat3 (lhs) += rhs);
}



Mat3	operator - (const Mat3 &lhs, const Mat3 &rhs)
{
	return (Mat3 (lhs) -= rhs);
}



Mat3	operator * (const Mat3 &lhs, const Mat3 &rhs)
{
	Mat3           tmp;
	for (int y = 0; y < Mat3::VECT_SIZE; ++y)
	{
		for (int x = 0; x < Mat3::VECT_SIZE; ++x)
		{
			double         sum = 0;
			for (int z = 0; z < Mat3::VECT_SIZE; ++z)
			{
				sum += lhs [y] [z] * rhs [z] [x];
			}
			tmp [y] [x] = sum;
		}
	}

	return tmp;
}



Vec3	operator * (const Mat3 &lhs, const Vec3 &rhs)
{
	Vec3           tmp;
	for (int y = 0; y < Vec3::VECT_SIZE; ++y)
	{
		double         sum = 0;
		for (int z = 0; z < Mat3::VECT_SIZE; ++z)
		{
			sum += lhs [y] [z] * rhs [z];
		}
		tmp [y] = sum;
	}

	return tmp;
}



Mat3	operator * (const Mat3 &lhs, double rhs)
{
	return Mat3 (lhs) *= rhs;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_Mat3_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
