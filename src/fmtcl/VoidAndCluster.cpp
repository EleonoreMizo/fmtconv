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



void	VoidAndCluster::set_aztec_mode (bool flag)
{
	_aztec_flag = flag;
}



void	VoidAndCluster::create_matrix (MatrixWrap <Rank> &vnc)
{
	const int      w   = vnc.get_w ();
	const int      h   = vnc.get_h ();
	assert (w * h <= std::numeric_limits <Rank>::max ());
	const int      ks  = _kernel_def_rad * 2 + 1;
	create_kernel (ks, ks, 1.5);

	_base._pat      = Monochrome { w, h };
	_base._pat_filt = Filtered { w, h };
	generate_initial_mat ();
	homogenize_initial_mat ();

	vnc.clear ();

	const int      rank_base = count_elt (_base._pat, 1);
	std::vector <Coord>  coord_arr;

	int            rank = rank_base;
	_cur = _base;
	while (rank > 0)
	{
		-- rank;
		_cur.find_cluster (coord_arr);
		const auto &   c = pick_one (coord_arr, uint32_t (rank));
		set_pix <0> (_cur, c);
		vnc.at (c._x, c._y) = Rank (rank);
	}

	rank = rank_base;
	_cur = _base;
	while (rank < w * h)
	{
		_cur.find_void (coord_arr);
		const auto &   v = pick_one (coord_arr, uint32_t (rank));
		set_pix <1> (_cur, v);
		vnc.at (v._x, v._y) = Rank (rank);
		++ rank;
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr int	VoidAndCluster::_kernel_def_rad;
constexpr VoidAndCluster::SampleType	VoidAndCluster::_kscale;



bool	VoidAndCluster::Coord::operator == (const Coord &other) const
{
	return (_x == other._x && _y == other._y);
}

bool	VoidAndCluster::Coord::operator != (const Coord &other) const
{
	return ! (*this == other);
}

bool	VoidAndCluster::Coord::operator < (const Coord &other) const
{
	return (std::tie (_y, _x) < std::tie (other._y, other._x));
}



void	VoidAndCluster::create_kernel (int w, int h, double sigma)
{
	const auto     w2 = 1 << fstb::get_next_pow_2 (w);
	const auto     h2 = 1 << fstb::get_next_pow_2 (h);
	_kernel._m = KernelData (w2, h2);
	_kernel._w = w;
	_kernel._h = h;

	const int      kw2 = (_kernel._w - 1) / 2;
	const int      kh2 = (_kernel._h - 1) / 2;
	const auto     mul = -1.0 / (2 * sigma * sigma);
	for (int j = 0; j <= kh2; ++j)
	{
		for (int i = 0; i <= kw2; ++i)
		{
			const auto     r2 = i * i + j * j;
			const auto     cf = exp (r2 * mul);
			const auto     ci = int64_t (cf * _kscale + 0.5);
			_kernel._m ( i,  j) = ci;
			_kernel._m (-i,  j) = ci;
			_kernel._m ( i, -j) = ci;
			_kernel._m (-i, -j) = ci;
		}
	}
}



void	VoidAndCluster::generate_initial_mat ()
{
	constexpr double  thr = 0.1;

	const int      w = _base._pat.get_w ();
	const int      h = _base._pat.get_h ();
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
				_base._pat.at (x, y) = typename Monochrome::DataType (qnt);
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

	filter_pat (_base);
}



void	VoidAndCluster::homogenize_initial_mat ()
{
	Coord          c { 0, 0 };
	Coord          v { 0, 0 };
	uint32_t       count = 0;
	std::vector <Coord> coord_arr;
	do
	{
		_base.find_cluster (coord_arr);
		c = pick_one (coord_arr, count);
		set_pix <0> (_base, c);
		++ count;

		_base.find_void (coord_arr);
		v = pick_one (coord_arr, count);
		set_pix <1> (_base, v);
		++ count;
	}
	while (c != v);
}



void	VoidAndCluster::filter_pat (PatState &state)
{
	state._pat_filt.clear ();
	state._histo.clear ();

	const int      w = state._pat.get_w ();
	const int      h = state._pat.get_h ();

	const int      kw2 = (_kernel._w - 1) / 2;
	const int      kh2 = (_kernel._h - 1) / 2;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			auto           sum = state._pat (x, y) * _kernel._m.at (0, 0);
			for (int j = 1; j <= kh2; ++j)
			{
				if (! _aztec_flag)
				{
					const auto     vx = _kernel._m.at (j, 0);
					const auto     vy = _kernel._m.at (0, j);
					sum += state._pat (x + j, y    ) * vx;
					sum += state._pat (x - j, y    ) * vx;
					sum += state._pat (x    , y + j) * vy;
					sum += state._pat (x    , y - j) * vy;
				}
				for (int i = 1; i <= kw2; ++i)
				{
					const auto     vk = _kernel._m.at (i, j);
					sum += state._pat (x + i, y + j) * vk;
					sum += state._pat (x - i, y + j) * vk;
					sum += state._pat (x + i, y - j) * vk;
					sum += state._pat (x - i, y - j) * vk;
				}
			}
			state._pat_filt (x, y) = sum;
			const auto     index = state._pat_filt.encode_coord (x, y);
			state._histo.insert (std::make_tuple (sum, index));
		}
	}
}



void	VoidAndCluster::PatState::find_cluster (std::vector <Coord> &pos_arr) const
{
	find_void_or_cluster <1> (pos_arr, _histo.rbegin (), _histo.rend ());
}



void	VoidAndCluster::PatState::find_void (std::vector <Coord> &pos_arr) const
{
	find_void_or_cluster <0> (pos_arr, _histo.begin (), _histo.end ());
}



template <typename VoidAndCluster::Monochrome::DataType V, class IT>
void VoidAndCluster::PatState::find_void_or_cluster (std::vector <Coord> &pos_arr, IT it_beg, IT it_end) const
{
	pos_arr.clear ();

	auto           it = it_beg;
	while (_pat.at (std::get <1> (*it)) != V)
	{
		++ it;
		assert (it != it_end);
	}
	const auto     val_ref = std::get <0> (*it);

	do
	{
		const auto     val_tst = std::get <0> (*it);
		if (val_tst != val_ref)
		{
			break;
		}
		const auto     idx = std::get <1> (*it);
		if (_pat.at (idx) == V)
		{
			const auto     x = _pat_filt.decode_x (idx);
			const auto     y = _pat_filt.decode_y (idx);
			pos_arr.push_back ({ x, y });
		}

		++ it;
	}
	while (it != it_end);

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



template <typename VoidAndCluster::Monochrome::DataType V>
void	VoidAndCluster::set_pix (PatState &state, Coord pos)
{
	assert (V != state._pat (pos._x, pos._y));

	state._pat (pos._x, pos._y) = V;
	if (V > 0)
	{
		apply_kernel (
			state, pos, [] (SampleType a, SampleType b) { return a + b; }
		);
	}
	else
	{
		apply_kernel (
			state, pos, [] (SampleType a, SampleType b) { return a - b; }
		);
	}
}



template <typename F>
void	VoidAndCluster::apply_kernel (PatState &state, Coord pos, F op) const
{
	const int      kw2 = (_kernel._w - 1) / 2;
	const int      kh2 = (_kernel._h - 1) / 2;

	update_filtered (state, pos, op, _kernel._m.at (0, 0));
	for (int j = 1; j <= kh2; ++j)
	{
		if (! _aztec_flag)
		{
			const auto     vx = _kernel._m.at (j, 0);
			const auto     vy = _kernel._m.at (0, j);
			update_filtered (state, { pos._x + j, pos._y     }, op, vx);
			update_filtered (state, { pos._x - j, pos._y     }, op, vx);
			update_filtered (state, { pos._x    , pos._y + j }, op, vy);
			update_filtered (state, { pos._x    , pos._y - j }, op, vy);
		}
		for (int i = 1; i <= kw2; ++i)
		{
			const auto     vk = _kernel._m.at (i, j);
			update_filtered (state, { pos._x + i, pos._y + j }, op, vk);
			update_filtered (state, { pos._x - i, pos._y + j }, op, vk);
			update_filtered (state, { pos._x + i, pos._y - j }, op, vk);
			update_filtered (state, { pos._x - i, pos._y - j }, op, vk);
		}
	}
}



template <typename F>
void	VoidAndCluster::update_filtered (PatState &state, Coord pos, F op, SampleType delta) const
{
	const auto     x       = state._pat_filt.wrap_x (pos._x);
	const auto     y       = state._pat_filt.wrap_y (pos._y);
	const auto     index   = state._pat_filt.encode_coord (x, y);

	const auto     val_old = state._pat_filt.at (index);
	const auto     key_old = std::make_tuple (val_old, index);
	const auto     it_old  = state._histo.find (key_old);
	assert (it_old != state._histo.end ());
	state._histo.erase (it_old);

	const auto     val_new = op (val_old, delta);
	state._pat_filt.at (index) = val_new;
	const auto     key_new = std::make_tuple (val_new, index);
	state._histo.insert (key_new);
}



int	VoidAndCluster::count_elt (const Monochrome &m, int val)
{
	int            total = 0;
	const int      w     = m.get_w ();
	const int      h     = m.get_h ();
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const int      c = m.at (x, y);
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
