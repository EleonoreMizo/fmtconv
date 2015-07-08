/*****************************************************************************

        MatrixWrap.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_MatrixWrap_HEADER_INCLUDED)
#define	fmtcl_MatrixWrap_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <vector>



namespace fmtcl
{



template <class T>
class MatrixWrap
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	explicit       MatrixWrap (int w, int h);
	               MatrixWrap (const MatrixWrap &other) = default;
	virtual        ~MatrixWrap () {}

	void           clear (T fill_val = 0);
	int            get_w () const { return (_w); }
	int            get_h () const { return (_h); }
	T &            operator () (int x, int y);
	const T &      operator () (int x, int y) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {         MARGIN = 1024 };

	int            _w;
	int            _h;
	std::vector <T>
	               _mat;


/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               MatrixWrap ()                               = delete;
	MatrixWrap &   operator = (const MatrixWrap &other)        = delete;
	bool           operator == (const MatrixWrap &other) const = delete;
	bool           operator != (const MatrixWrap &other) const = delete;

};	// class MatrixWrap



}	// namespace fmtcl



#include "fmtcl/MatrixWrap.hpp"



#endif	// fmtcl_MatrixWrap_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
