/*****************************************************************************

        ErrDifBuf.h
        Author: Laurent de Soras, 2010

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_ErrDifBuf_HEADER_INCLUDED)
#define	fmtcl_ErrDifBuf_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cstdint>



namespace fmtcl
{



class ErrDifBuf
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const int  NBR_LINES     = 2;
	static const int  MARGIN        = 2;
	static const int  MAX_DATA_SIZE = 4;

	explicit       ErrDifBuf (long width);
	virtual        ~ErrDifBuf ();

	inline void    clear (int ds);
	template <class T>
	inline void    clear ();
	template <class T>
	inline T *     get_buf (int ofy);
	template <class T>
	inline T &     use_mem (int pos);
	template <class T>
	inline const T&use_mem (int pos) const;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	uint8_t *      _buf_ptr;   // Currently not aligned with anything
	uint8_t        _mem [MARGIN * MAX_DATA_SIZE];
	long           _width;
	long           _stride;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               ErrDifBuf ()                               = delete;
	               ErrDifBuf (const ErrDifBuf &other)         = delete;
	ErrDifBuf &    operator = (const ErrDifBuf &other)        = delete;
	bool           operator == (const ErrDifBuf &other) const = delete;
	bool           operator != (const ErrDifBuf &other) const = delete;

};	// class ErrDifBuf



}	// namespace fmtcl



#include "fmtcl/ErrDifBuf.hpp"



#endif	// fmtcl_ErrDifBuf_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
