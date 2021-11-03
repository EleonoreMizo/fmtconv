/*****************************************************************************

        VoidAndCluster.h
        Author: Laurent de Soras, 2015

Reference:
Robert Ulichney,
The Void-And-Cluster Method for Dither Array Generation
Proc. SPIE, Human Vision, Visual Processing, and Digital Display IV,
vol. 1913, pp. 332-343, Feb. 1-4, 1993

*** TO DO: implement:
Hakan Ancin, Anoop K. Bhattacharjya, Joseph Shou-Pyng Shu,
New void-and-cluster method for improved halftone uniformity,
Journal of Electronic Imaging 8(1), January 1999,
https://doi.org/10.1117/1.482701 ***

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

#include "fmtcl/MatrixWrap.h"

#include <cstdint>

#include <memory>
#include <vector>



namespace fmtcl
{



class VoidAndCluster
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef int32_t Rank; // May be signed or unsigned.

	               VoidAndCluster () = default;
	virtual			~VoidAndCluster () {}

	void           create_matrix (MatrixWrap <Rank> &vnc);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static constexpr int  _kernel_def_rad = 4;

	class Coord
	{
	public:
		int            _x;
		int            _y;
		inline bool    operator == (const Coord &other) const;
		inline bool    operator != (const Coord &other) const;
	};

	typedef MatrixWrap <uint8_t> Monochrome; // Contains only 0 or 1

	typedef int64_t SampleType;
	static constexpr SampleType  _kscale = SampleType (1) << 32;

	typedef MatrixWrap <SampleType> KernelData;
	typedef MatrixWrap <SampleType> Filtered;

	class Kernel
	{
	public:
		KernelData     _m;
		int            _w = 0; // Kernel width, odd. 0 = not initialized
		int            _h = 0; // Kernel height, odd. 0 = not initialized
	};

	class PatState
	{
	public:
		void           find_cluster (std::vector <Coord> &pos_arr) const;
		void           find_void (std::vector <Coord> &pos_arr) const;
		Monochrome     _pat;
		Filtered       _pat_filt;
	};

	void           generate_initial_mat ();
	void           homogenize_initial_mat ();
	void           find_cluster_kernel (std::vector <Coord> &pos_arr, const PatState &state, int color) const;
	const Coord &  pick_one (std::vector <Coord> &pos_arr, uint32_t seed) const;
	template <typename Monochrome::DataType V>
	void           set_pix (PatState &state, Coord pos);
	template <typename F>
	inline void    apply_kernel (Filtered &pat_filt, Coord pos, F op) const;

	void           create_kernel (int w, int h, double sigma);
	void           filter_pat (PatState &state);

	static int     count_elt (const Monochrome &m, int val);

	Kernel         _kernel;
	PatState       _base;
	PatState       _cur;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               VoidAndCluster (const VoidAndCluster &other)    = delete;
	VoidAndCluster &
	               operator = (const VoidAndCluster &other)        = delete;
	bool           operator == (const VoidAndCluster &other) const = delete;
	bool           operator != (const VoidAndCluster &other) const = delete;

};	// class VoidAndCluster



}	// namespace fmtcl



//#include "fmtcl/VoidAndCluster.hpp"



#endif	// fmtcl_VoidAndCluster_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
