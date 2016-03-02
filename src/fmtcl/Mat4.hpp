/*****************************************************************************

        Mat4.hpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_Mat4_CODEHEADER_INCLUDED)
#define	fmtcl_Mat4_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Mat4::Mat4 (double filler, Preset preset)
:	_data ()
{
	assert (preset >= 0);
	assert (preset < Preset_NBR_ELT);

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			if (preset == Preset_DIAGONAL && x != y)
			{
				_data [y] [x] = 0;
			}
			else
			{
				_data [y] [x] = filler;
			}
		}
	}
}



Mat4::Mat4 (const double content [4] [4])
:	_data ()
{
	assert (content != 0);

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			_data [y] [x] = content [y] [x];
		}
	}
}



bool	Mat4::operator == (const Mat4 &other) const
{
	assert (&other != 0);

	bool           eq_flag = true;
	for (int y = 0; y < VECT_SIZE && eq_flag; ++y)
	{
		for (int x = 0; x < VECT_SIZE && eq_flag; ++x)
		{
			eq_flag = (_data [y] [x] == other._data [y] [x]);
		}
	}

	return (eq_flag);
}



bool	Mat4::operator != (const Mat4 &other) const
{
	return (! (*this == other));
}



Mat4 &	Mat4::operator += (const Mat4 &other)
{
	assert (&other != 0);

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			_data [y] [x] += other._data [y] [x];
		}
	}

	return (*this);
}



Mat4 &	Mat4::operator -= (const Mat4 &other)
{
	assert (&other != 0);

	for (int y = 0; y < VECT_SIZE; ++y)
	{
		for (int x = 0; x < VECT_SIZE; ++x)
		{
			_data [y] [x] -= other._data [y] [x];
		}
	}

	return (*this);
}



Mat4 &	Mat4::operator *= (const Mat4 &other)
{
	assert (&other != 0);

	*this = *this * other;

	return (*this);
}



double	Mat4::det3 () const
{
	return (
		  _data [0] [0] * _data [1] [1] * _data [2] [2]
		+ _data [1] [0] * _data [2] [1] * _data [0] [2]
		+ _data [2] [0] * _data [0] [1] * _data [1] [2]
		- _data [0] [0] * _data [2] [1] * _data [1] [2]
		- _data [1] [0] * _data [0] [1] * _data [2] [2]
		- _data [2] [0] * _data [1] [1] * _data [0] [2]
	);
}



Mat4	Mat4::compute_inverse3 () const
{
	const double   d3 = det3 ();

	Mat4           r (*this);
	r [0] [0] = (_data [1] [1] * _data [2] [2] - _data [2] [1] * _data [1] [2]) / d3;
	r [0] [1] = (_data [2] [0] * _data [1] [2] - _data [1] [0] * _data [2] [2]) / d3;
	r [0] [2] = (_data [1] [0] * _data [2] [1] - _data [2] [0] * _data [1] [1]) / d3;
	r [1] [0] = (_data [2] [1] * _data [0] [2] - _data [0] [1] * _data [2] [2]) / d3;
	r [1] [1] = (_data [0] [0] * _data [2] [2] - _data [2] [0] * _data [0] [2]) / d3;
	r [1] [2] = (_data [2] [0] * _data [0] [1] - _data [0] [0] * _data [2] [1]) / d3;
	r [2] [0] = (_data [0] [1] * _data [1] [2] - _data [1] [1] * _data [0] [2]) / d3;
	r [2] [1] = (_data [1] [0] * _data [0] [2] - _data [0] [0] * _data [1] [2]) / d3;
	r [2] [2] = (_data [0] [0] * _data [1] [1] - _data [1] [0] * _data [0] [1]) / d3;

	return (r);
}



Mat4 &	Mat4::invert3 ()
{
	*this = compute_inverse3 ();

	return (*this);
}



Mat4 &	Mat4::clean3 (double diag)
{
	                                                         _data [0] [3] = 0;
	                                                         _data [1] [3] = 0;
	                                                         _data [2] [3] = 0;
	_data [3] [0] = 0; _data [3] [1] = 0; _data [3] [2] = 0; _data [3] [3] = diag;

	return (*this);
}



const Mat4::Row4 &	Mat4::operator [] (long pos) const
{
	assert (pos >= 0);
	assert (pos < VECT_SIZE);

	return (_data [pos]);
}



Mat4::Row4 &	Mat4::operator [] (long pos)
{
	assert (pos >= 0);
	assert (pos < VECT_SIZE);

	return (_data [pos]);
}



Mat4	operator + (const Mat4 &lhs, const Mat4 &rhs)
{
	assert (&lhs != 0);

	return (Mat4 (lhs) += rhs);
}



Mat4	operator - (const Mat4 &lhs, const Mat4 &rhs)
{
	assert (&lhs != 0);

	return (Mat4 (lhs) -= rhs);
}



Mat4	operator * (const Mat4 &lhs, const Mat4 &rhs)
{
	assert (&lhs != 0);
	assert (&rhs != 0);

	Mat4           tmp;
	for (int y = 0; y < Mat4::VECT_SIZE; ++y)
	{
		for (int x = 0; x < Mat4::VECT_SIZE; ++x)
		{
			double         sum = 0;
			for (int z = 0; z < Mat4::VECT_SIZE; ++z)
			{
				sum += lhs [y] [z] * rhs [z] [x];
			}
			tmp [y] [x] = sum;
		}
	}

	return (tmp);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}	// namespace fmtcl



#endif	// fmtcl_Mat4_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
