/*****************************************************************************

        CoefArrInt.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_CoefArrInt_HEADER_INCLUDED)
#define	fmtcl_CoefArrInt_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/AllocAlign.h"

#include <array>
#include <vector>

#include <cstdint>



namespace fmtcl
{



class CoefArrInt
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const int  VECT_LEN_SSE2 = 16 / int (sizeof (int16_t));
	static const int  VECT_LEN_AVX2 = 32 / int (sizeof (int16_t));

	typedef std::array <int16_t, VECT_LEN_SSE2>  VectSse2i16;
	typedef std::array <int16_t, VECT_LEN_AVX2>  VectAvx2i16;

	               CoefArrInt () = default;
	virtual			~CoefArrInt () {}

	void           clear ();
	void           set_avx2_mode (bool avx2_flag);

	void           resize (int size);
	inline int     get_size () const;

	void           set_coef (int pos, int val);
	void           set_coef_int32 (int pos, int val);
	inline int     get_coef (int pos) const;
	void           copy_coef (int pos_to, int pos_from);

	fstb_FORCEINLINE const int16_t *
	               use_vect_sse2 (int pos) const;
	fstb_FORCEINLINE const int16_t *
	               use_vect_avx2 (int pos) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	std::vector <int16_t, fstb::AllocAlign <int16_t, 32> >
	               _coef_arr;
	bool           _avx2_flag  = false;
	int            _size       = 0;
	int            _vect_shift = 3;     // For SSE2



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               CoefArrInt (const CoefArrInt &other)        = delete;
	CoefArrInt &   operator = (const CoefArrInt &other)        = delete;
	bool           operator == (const CoefArrInt &other) const = delete;
	bool           operator != (const CoefArrInt &other) const = delete;

};	// class CoefArrInt



}	// namespace fmtcl



#include "fmtcl/CoefArrInt.hpp"



#endif	// fmtcl_CoefArrInt_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
