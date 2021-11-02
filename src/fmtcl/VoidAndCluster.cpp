/*****************************************************************************

        VoidAndCluster.cpp
        Author: Laurent de Soras, 2015

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/VoidAndCluster.h"
#include "fstb/fnc.h"
#include "fstb/Hash.h"

#include <limits>

#include <cassert>
#include <cmath>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	VoidAndCluster::create_matrix (MatrixWrap <Rank> &vnc)
{
	const int      w   = vnc.get_w ();
	const int      h   = vnc.get_h ();
	assert (w * h <= std::numeric_limits <Rank>::max ());
	const int      ks  = KERNEL_MAX_RAD * 2 + 1;
	_kernel_gauss_uptr = create_gauss_kernel (ks, ks, 1.5);

	MatrixWrap <Rank> mat_base (w, h);
	generate_initial_mat (mat_base);
	homogenize_initial_mat (mat_base);

	vnc.clear ();

	{
		int            rank = count_elt (mat_base, 1);
		MatrixWrap <Rank>    mat (mat_base);
		std::vector <Coord>  c_arr;
		while (rank > 0)
		{
			-- rank;
			find_cluster_kernel (c_arr, mat, 1, KERNEL_DEF_SIZE, KERNEL_DEF_SIZE);
			const auto &   c = pick_one (c_arr, uint32_t (rank));
			mat (c._x, c._y) = 0;
			vnc (c._x, c._y) = Rank (rank);
		}
	}

	{
		int            rank = count_elt (mat_base, 1);
		MatrixWrap <Rank>    mat (mat_base);
		std::vector <Coord>  v_arr;
		while (rank < w * h)
		{
			find_cluster_kernel (v_arr, mat, 0, KERNEL_DEF_SIZE, KERNEL_DEF_SIZE);
			const auto &   v = pick_one (v_arr, uint32_t (rank));
			mat (v._x, v._y) = 1;
			vnc (v._x, v._y) = Rank (rank);
			++ rank;
		}
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	VoidAndCluster::homogenize_initial_mat (MatrixWrap <Rank> &m) const
{
	int            cx    = 0;
	int            cy    = 0;
	int            vx    = 0;
	int            vy    = 0;
	uint32_t       count = 0;
	std::vector <Coord> c_arr;
	std::vector <Coord> v_arr;
	do
	{
		find_cluster_kernel (c_arr, m, 1, KERNEL_DEF_SIZE, KERNEL_DEF_SIZE);
		const auto &   c = pick_one (c_arr, count);
		cx = c._x;
		cy = c._y;
		m (cx, cy) = 0;
		++ count;

		find_cluster_kernel (v_arr, m, 0, KERNEL_DEF_SIZE, KERNEL_DEF_SIZE);
		const auto &   v = pick_one (v_arr, count);
		vx = v._x;
		vy = v._y;
		m (vx, vy) = 1;
		++ count;
	}
	while (cx != vx || cy != vy);
}



void	VoidAndCluster::find_cluster_kernel (std::vector <Coord> &pos_arr, const MatrixWrap <Rank> &m, int color, int kw, int kh) const
{
	assert (kw <= _kernel_gauss_uptr->get_w ());
	assert (kh <= _kernel_gauss_uptr->get_h ());

	pos_arr.clear ();

	typedef typename Kernel::DataType SumType;

	SumType        max_v = -1;
	const int      w     = m.get_w ();
	const int      h     = m.get_h ();
	const int      kw2   = (kw - 1) / 2;
	const int      kh2   = (kh - 1) / 2;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      cur_c = m (x, y);
			if (cur_c == color)
			{
				SumType        sum = 0;
				for (int j = -kh2; j <= kh2; ++j)
				{
					for (int i = -kw2; i <= kw2; ++i)
					{
						const int      a = m (x + i, y + j);
						if (a == color)
						{
							const auto     c = (*_kernel_gauss_uptr) (i, j);
							sum += c;
						}
					}
				}
				if (sum >= max_v)
				{
					if (sum > max_v)
					{
						pos_arr.clear ();
					}
					max_v = sum;
					pos_arr.push_back ({ x, y });
				}
			}
		}
	}

	assert (! pos_arr.empty ());
}



const VoidAndCluster::Coord &	VoidAndCluster::pick_one (std::vector <Coord> &pos_arr, uint32_t seed) const
{
	assert (! pos_arr.empty ());

	const auto     nbr_elt = int (pos_arr.size ());
	if (nbr_elt == 1)
	{
		return pos_arr.front ();
	}

	const auto     pos = fstb::Hash::hash (seed) % nbr_elt;

	return pos_arr [pos];
}



std::unique_ptr <VoidAndCluster::Kernel>	VoidAndCluster::create_gauss_kernel (int w, int h, double sigma)
{
	const auto     w2 = 1 << fstb::get_next_pow_2 (w);
	const auto     h2 = 1 << fstb::get_next_pow_2 (h);
	auto           ker_uptr = std::make_unique <Kernel> (w2, h2);
	auto &         ker      = *ker_uptr;

	const int      kw2 = (w - 1) / 2;
	const int      kh2 = (h - 1) / 2;
	const auto     mul = -1.0 / (2 * sigma * sigma);
	for (int j = 0; j <= kh2; ++j)
	{
		for (int i = 0; i <= kw2; ++i)
		{
			const auto     c = exp ((i * i + j * j) * mul);
			ker ( i,  j) = c;
			ker (-i,  j) = c;
			ker ( i, -j) = c;
			ker (-i, -j) = c;
		}
	}

	return ker_uptr;
}



void	VoidAndCluster::generate_initial_mat (MatrixWrap <Rank> &m)
{
	const double   thr = 0.1;

	const int      w = m.get_w ();
	const int      h = m.get_h ();
	MatrixWrap <double>  err_mat (w, h);
	err_mat.clear ();
	int            dir = 1;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int y = 0; y < h; ++y)
		{
			const int      x_beg = (dir < 0) ? w - 1 : 0;
			const int      x_end = (dir < 0) ?    -1 : w;
			for (int x = x_beg; x != x_end; x += dir)
			{
				double         err = err_mat (x, y);
				err_mat (x, y) = 0;
				const double   val = thr + err;
				const int      qnt = fstb::round_int (val);
				assert (qnt >= 0 && qnt <= 1);
				m (x, y) = Rank (qnt);
				err = val - double (qnt);
				// Filter-Lite error diffusion
				const double   e2 = err * 0.5;
				const double   e4 = err * 0.25;
				err_mat (x + dir, y    ) += e2;
				err_mat (x - dir, y + 1) += e4;
				err_mat (x      , y + 1) += e4;
			}

			dir = -dir;
		}
	}
}



int	VoidAndCluster::count_elt (const MatrixWrap <Rank> &m, int val)
{
	int            total = 0;
	const int      w     = m.get_w ();
	const int      h     = m.get_h ();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      c = m (x, y);
			if (c == val)
			{
				++ total;
			}
		}
	}

	return total;
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
