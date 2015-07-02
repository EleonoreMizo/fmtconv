/*****************************************************************************

        VoidAndCluster.h
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fmtcl_VoidAndCluster_HEADER_INCLUDED)
#define	fmtcl_VoidAndCluster_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <cstdint>

#include <memory>
#include <vector>



namespace fmtcl
{



class VoidAndCluster
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	template <class T>
	class MatrixWrap
	{
	public:
		explicit       MatrixWrap (int w, int h);
		               MatrixWrap (const MatrixWrap &other) = default;
		void           clear (T fill_val = 0);
		int            get_w () const { return (_w); }
		int            get_h () const { return (_h); }
		T &            operator () (int x, int y);
		const T &      operator () (int x, int y) const;
	private:
		int            _w;
		int            _h;
		std::vector <T>
		               _mat;
	};

	               VoidAndCluster ();
	virtual			~VoidAndCluster () {}

	void           create_matrix (MatrixWrap <uint16_t> &vnc);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum {         KERNEL_MAX_RAD = 5 };

	void           homogenize_initial_mat (MatrixWrap <uint16_t> &m) const;
	void           find_cluster_kernel (std::vector <std::pair <int, int> > &pos_arr, const MatrixWrap <uint16_t> &m, int color, int kw, int kh) const;

	static std::unique_ptr <MatrixWrap <double> >
	               create_gauss_kernel (int w, int h, double sigma);
	static void    generate_initial_mat (MatrixWrap <uint16_t> &m);
	static int     count_elt (const MatrixWrap <uint16_t> &m, int val);

	std::unique_ptr <MatrixWrap <double> >
	               _kernel_gauss_uptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               VoidAndCluster (const VoidAndCluster &other);
	VoidAndCluster &
	               operator = (const VoidAndCluster &other);
	bool           operator == (const VoidAndCluster &other) const;
	bool           operator != (const VoidAndCluster &other) const;

};	// class VoidAndCluster



}	// namespace fmtcl



//#include "fmtcl/VoidAndCluster.hpp"



#endif	// fmtcl_VoidAndCluster_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
