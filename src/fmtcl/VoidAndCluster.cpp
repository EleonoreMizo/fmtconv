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

#include <functional>
#include <random>

#include <cassert>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



VoidAndCluster::VoidAndCluster ()
:	_kernel_gauss_uptr ()
{
	// Nothing
}



void	VoidAndCluster::create_matrix (MatrixWrap <uint16_t> &vnc)
{
	const int      w   = vnc.get_w ();
	const int      h   = vnc.get_h ();
	const int      ks  = KERNEL_MAX_RAD * 2 + 1;
	_kernel_gauss_uptr = create_gauss_kernel (ks, ks, 1.5);

	MatrixWrap <uint16_t>   mat_base (w, h);
	generate_initial_mat (mat_base);
	homogenize_initial_mat (mat_base);

	vnc.clear ();

	{
		int            rank = count_elt (mat_base, 1);
		MatrixWrap <uint16_t>   mat (mat_base);
		while (rank > 0)
		{
			-- rank;
			std::vector <std::pair <int, int> > c_arr;
			find_cluster_kernel (c_arr, mat, 1, 9, 9);
			const int      x = c_arr [0].first;
			const int      y = c_arr [0].second;
			mat (x, y) = 0;
			vnc (x, y) = rank;
		}
	}

	{
		int            rank = count_elt (mat_base, 1);
		MatrixWrap <uint16_t>   mat (mat_base);
		while (rank < w * h)
		{
			std::vector <std::pair <int, int> > v_arr;
			find_cluster_kernel (v_arr, mat, 0, 9, 9);
			const int      x = v_arr [0].first;
			const int      y = v_arr [0].second;
			mat (x, y) = 1;
			vnc (x, y) = rank;
			++ rank;
		}
	}
}



template <class T>
VoidAndCluster::MatrixWrap <T>::MatrixWrap (int w, int h)
:	_w (w)
,	_h (h)
,	_mat (w * h, 0)
{
	assert (w > 0);
	assert (h > 0);
}



template <class T>
void	VoidAndCluster::MatrixWrap <T>::clear (T fill_val)
{
	_mat.assign (_mat.size (), fill_val);
}



template <class T>
T &	VoidAndCluster::MatrixWrap <T>::operator () (int x, int y)
{
	x = (x + _w * 1024) % _w;
	y = (y + _h * 1024) % _h;

	return (_mat [y * _w + x]);
}



template <class T>
const T &	VoidAndCluster::MatrixWrap <T>::operator () (int x, int y) const
{
	x = (x + _w * 1024) % _w;
	y = (y + _h * 1024) % _h;

	return (_mat [y * _w + x]);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	VoidAndCluster::homogenize_initial_mat (MatrixWrap <uint16_t> &m) const
{
	assert (&m != 0);

	int            cx;
	int            cy;
	int            vx;
	int            vy;
	std::vector <std::pair <int, int> > c_arr;
	std::vector <std::pair <int, int> > v_arr;
	do
	{
		find_cluster_kernel (c_arr, m, 1, 9, 9);
		cx = c_arr [0].first;
		cy = c_arr [0].second;
		m (cx, cy) = 0;
		find_cluster_kernel (v_arr, m, 0, 9, 9);
		vx = v_arr [0].first;
		vy = v_arr [0].second;
		m (vx, vy) = 1;
	}
	while (cx != vx || cy != vy);
}



void	VoidAndCluster::find_cluster_kernel (std::vector <std::pair <int, int> > &pos_arr, const MatrixWrap <uint16_t> &m, int color, int kw, int kh) const
{
	assert (&pos_arr != 0);
	assert (&m != 0);

	pos_arr.clear ();

	const double   s     = 1.5;
	double         max_v = -1;
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
				double         sum = 0;
				for (int j = -kh2; j <= kh2; ++j)
				{
					for (int i = -kw2; i <= kw2; ++i)
					{
						const int      a = m (x + i, y + j);
						if (a == color)
						{
							const double   c = (*_kernel_gauss_uptr) (i, j);
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
					pos_arr.push_back (std::make_pair (x, y));
				}
			}
		}
	}

	assert (! pos_arr.empty ());
}



std::unique_ptr <VoidAndCluster::MatrixWrap <double> >	VoidAndCluster::create_gauss_kernel (int w, int h, double sigma)
{
	std::unique_ptr <MatrixWrap <double> >   ker_uptr (
		new MatrixWrap <double> (w, h)
	);
	MatrixWrap <double> &   ker = *ker_uptr;

	const int      kw2 = (w - 1) / 2;
	const int      kh2 = (h - 1) / 2;
	for (int j = 0; j <= kh2; ++j)
	{
		for (int i = 0; i <= kw2; ++i)
		{
			const double   c = exp (-(i * i + j * j) / (2 * sigma * sigma));
			ker ( i,  j) = c;
			ker (-i,  j) = c;
			ker ( i, -j) = c;
			ker (-i, -j) = c;
		}
	}

	return (ker_uptr);
}



void	VoidAndCluster::generate_initial_mat (MatrixWrap <uint16_t> &m)
{
	assert (&m != 0);

	const double   thr = 0.1;

#if 0

	std::default_random_engine rand_gen;
	std::uniform_real_distribution <double> dist (0, 1);
	auto           dice = std::bind (dist, rand_gen);

	const int      w = m.get_w ();
	const int      h = m.get_h ();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const double   r = dice ();
			uint16_t       v = 0;
			if (r < thr)
			{
				v = 1;
			}
			m (x, y) = v;
		}
	}

#else

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
				const int      qnt = int (floor (val + 0.5));
				assert (qnt >= 0 && qnt <= 1);
				m (x, y) = qnt;
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

#endif
}



int	VoidAndCluster::count_elt (const MatrixWrap <uint16_t> &m, int val)
{
	assert (&m != 0);

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

	return (total);
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
