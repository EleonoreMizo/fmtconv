/*****************************************************************************

        Mat4.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_Mat4_HEADER_INCLUDED)
#define	fmtcl_Mat4_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <array>



namespace fmtcl
{



class Mat3;

class Mat4
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const int  VECT_SIZE = 4;

	enum Preset
	{
		Preset_SOLID = 0,
		Preset_DIAGONAL,

		Preset_NBR_ELT
	};

	typedef std::array <double, VECT_SIZE> Row4;

	inline         Mat4 ()                        = default;
	inline explicit
	               Mat4 (double filler, Preset preset = Preset_SOLID);
	inline constexpr
	               Mat4 (const double content [VECT_SIZE] [VECT_SIZE]);
	inline         Mat4 (const Mat4 &other)       = default;
	virtual        ~Mat4 () {}
	inline Mat4 &  operator = (const Mat4 &other) = default;

	inline bool    operator == (const Mat4 &other) const;
	inline bool    operator != (const Mat4 &other) const;

	inline Mat4 &  operator += (const Mat4 &other);
	inline Mat4 &  operator -= (const Mat4 &other);
	inline Mat4 &  operator *= (const Mat4 &other);
	inline Mat4 &  operator *= (double scale);

	inline void    insert3 (const Mat3 &other);
	inline Mat4 &  clean3 (double diag = 0);

	inline const Row4 &
                  operator [] (long pos) const;
	inline Row4 &  operator [] (long pos);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	typedef std::array <Row4, VECT_SIZE> MatArr;

	MatArr         _data;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:


};	// class Mat4



inline Mat4	operator + (const Mat4 &lhs, const Mat4 &rhs);
inline Mat4	operator - (const Mat4 &lhs, const Mat4 &rhs);
inline Mat4	operator * (const Mat4 &lhs, const Mat4 &rhs);
inline Mat4	operator * (const Mat4 &lhs, double rhs);



}	// namespace fmtcl



#include "fmtcl/Mat4.hpp"



#endif	// fmtcl_Mat4_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
