/*****************************************************************************

        FilterResize.cpp
        Author: Laurent de Soras, 2011

TO DO:
	- Merge the initial format conversion with the transpose to avoid a copy

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

#include "fstb/def.h"
#include "fmtcl/ContFirInterface.h"
#include "fmtcl/FilterResize.h"
#include "fmtcl/ResampleSpecPlane.h"
#include "fmtcl/Scaler.h"
#include "fstb/fnc.h"

#if (fstb_ARCHI == fstb_ARCHI_X86)
	#if ! defined (_WIN64) && ! defined (__64BIT__) && ! defined (__amd64__) && ! defined (__x86_64__)
	 #include <mmintrin.h>
	#endif
	#include <xmmintrin.h>
	#include <emmintrin.h>
#endif

#include <algorithm>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FilterResize::FilterResize (const ResampleSpecPlane &spec, ContFirInterface &kernel_fnc_h, ContFirInterface &kernel_fnc_v, bool norm_flag, double norm_val_h, double norm_val_v, double gain, SplFmt src_type, int src_res, SplFmt dst_type, int dst_res, bool int_flag, bool sse2_flag, bool avx2_flag)
:	_avstp (AvstpWrapper::use_instance ())
,	_task_rsz_pool ()
/*,	_src_size ()
,	_dst_size ()
,	_win_pos ()
,	_win_size ()
,	_kernel_scale ()
,	_kernel_force_flag ()
,	_kernel_ptr_arr ()*/
,	_norm_flag (norm_flag)
/*,	_norm_val ()
,	_center_pos_src ()
,	_center_pos_dst ()*/
,	_gain (gain)
,	_add_cst (spec._add_cst)
,	_src_type (src_type)
,	_src_res (src_res)
,	_dst_type (dst_type)
,	_dst_res (dst_res)
,	_bd_chg_dir (Dir_H)
,	_int_flag (int_flag && _src_type != SplFmt_FLOAT && _dst_type != SplFmt_FLOAT)
,	_sse2_flag (sse2_flag)
,	_avx2_flag (avx2_flag)
,	_pool ()
,	_factory_uptr ()
/*,	_crop_pos ()
,	_crop_size ()*/
,	_scaler_uptr ()
,	_blitter (sse2_flag, avx2_flag)
/*,	_resize_flag ()
,	_roadmap ()
,	_tile_size_dst ()*/
,	_nbr_passes (0)
,	_buf_size (BUF_SIZE)
,	_buffer_flag (false)
{
	assert (spec._src_width > 0);
	assert (spec._src_height > 0);
	assert (spec._dst_width > 0);
	assert (spec._dst_height > 0);
	assert (spec._win_w > 0);
	assert (spec._win_h > 0);
	assert (! fstb::is_null (spec._kernel_scale_h));
	assert (! fstb::is_null (spec._kernel_scale_v));
	assert (! fstb::is_null (gain));
	assert (dst_type >= 0);
	assert (dst_type < SplFmt_NBR_ELT);
	assert (dst_res >= 8);
	assert (src_type >= 0);
	assert (src_type < SplFmt_NBR_ELT);
	assert (src_res >= 8);

	_src_size [Dir_H]          = spec._src_width;
	_src_size [Dir_V]          = spec._src_height;
	_dst_size [Dir_H]          = spec._dst_width;
	_dst_size [Dir_V]          = spec._dst_height;
	_win_pos [Dir_H]           = spec._win_x;
	_win_pos [Dir_V]           = spec._win_y;
	_win_size [Dir_H]          = spec._win_w;
	_win_size [Dir_V]          = spec._win_h;
	_kernel_scale [Dir_H]      = fabs (spec._kernel_scale_h);
	_kernel_scale [Dir_V]      = fabs (spec._kernel_scale_v);
	_kernel_force_flag [Dir_H] = (spec._kernel_scale_h < 0);
	_kernel_force_flag [Dir_V] = (spec._kernel_scale_v < 0);
	_center_pos_src [Dir_H]    = spec._center_pos_src_h;
	_center_pos_src [Dir_V]    = spec._center_pos_src_v;
	_center_pos_dst [Dir_H]    = spec._center_pos_dst_h;
	_center_pos_dst [Dir_V]    = spec._center_pos_dst_v;
	_crop_pos [Dir_H]          = 0;
	_crop_pos [Dir_V]          = 0;
	_crop_size [Dir_H]         = spec._src_width;
	_crop_size [Dir_V]         = spec._src_height;
	_kernel_ptr_arr [Dir_H]    = &kernel_fnc_h;
	_kernel_ptr_arr [Dir_V]    = &kernel_fnc_v;
	_norm_val [Dir_H]          = norm_val_h;
	_norm_val [Dir_V]          = norm_val_v;

	for (int dir = 0; dir < Dir_NBR_ELT; ++dir)
	{
		_resize_flag [dir] = (
			   _win_pos [dir] < 0
		   || _win_pos [dir] + _win_size [dir] > _src_size [dir]
		   || fabs (_win_pos [dir] - fstb::round (_win_pos [dir])) > 1e-5
		   || ! fstb::is_eq (_win_size [dir], double (_dst_size [dir]))
		   || ! fstb::is_eq (_kernel_scale [dir], 1.0)
		   || _kernel_force_flag [dir]
			|| ! fstb::is_eq (_center_pos_src [dir], _center_pos_dst [dir])
		);
		if (_resize_flag [dir])
		{
			Scaler::eval_req_src_area (
				_crop_pos [dir], _crop_size [dir],
				_src_size [dir], _dst_size [dir],
				_win_pos [dir], _win_size [dir],
				*(_kernel_ptr_arr [dir]), _kernel_scale [dir],
				_center_pos_src [dir], _center_pos_dst [dir]
			);
		}
		else
		{
			_crop_pos [dir]  = fstb::round_int (_win_pos [dir]);
			_crop_size [dir] = fstb::round_int (_win_size [dir]);
		}
	}

	// Finds the scaling order
	const double		r_h = _dst_size [Dir_H] / _win_size [Dir_H];
	const double		r_v = _dst_size [Dir_V] / _win_size [Dir_V];
	bool					vert_last_flag =
		   (r_v < 1 && r_v < r_h)	// Vertical downscale has the highest reduction factor
		|| (r_v > 1 && r_h > 1);	// Upscale in both direction: avoids using a buffer to store the final picture.

	// Special case for extreme deformations
	vert_last_flag = (vert_last_flag ||   (r_v / r_h > 8 && r_v > 4));
	vert_last_flag = (vert_last_flag && ! (r_h / r_v > 8 && r_h > 4));

	// Builds a roadmap
	_buffer_flag = false;
	int					rm_pos = 0;
	if (_resize_flag [Dir_V] && ! vert_last_flag)
	{
		_roadmap [rm_pos] = PassType_RESIZE;
		++ rm_pos;
	}
	if (_resize_flag [Dir_H])
	{
		_buffer_flag = true;
		_roadmap [rm_pos    ] = PassType_TRANSPOSE;
		_roadmap [rm_pos + 1] = PassType_RESIZE;
		_roadmap [rm_pos + 2] = PassType_TRANSPOSE;
		rm_pos += 3;
	}
	if (_resize_flag [Dir_V] && vert_last_flag)
	{
		_roadmap [rm_pos] = PassType_RESIZE;
		++ rm_pos;
	}
	assert (rm_pos <= MAX_NBR_PASSES);

	_nbr_passes = rm_pos;

	while (rm_pos < MAX_NBR_PASSES)
	{
		_roadmap [rm_pos] = PassType_NONE;
		++ rm_pos;
	}

	// Defines the resizer in charge of the bitdepth conversion
	// Default is Dir_H, checks if Dir_V is needed.
	const bool     bd_chg_last_flag = (_dst_res < _src_res);
	if (   (bd_chg_last_flag && vert_last_flag)
	    || ! _resize_flag [_bd_chg_dir])
	{
		_bd_chg_dir = Dir_V;
	}

	// Computes the tile size (if required)
	if (_buffer_flag)
	{
		const int      tile_dst_min_w = std::min (_dst_size [Dir_H], int (Scaler::SRC_ALIGN));
		const int      tile_dst_min_h = 1;

		int            tile_dst_w  = 0;
		int            tile_dst_h  = 0;
		bool           bigger_flag = false;
		do
		{
			bigger_flag = false;

			int            tile_src_min_w;
			int            tile_src_min_h;
			compute_req_src_tile_size (
				tile_src_min_w,
				tile_src_min_h,
				tile_dst_min_w,
				tile_dst_min_h
			);
			tile_src_min_w = (tile_src_min_w + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
			tile_src_min_h = (tile_src_min_h + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;

			// First computes a well-balanced source size
			int            tile_src_w = tile_src_min_w;
			int            tile_src_h = tile_src_min_h;
			const double   a = 1;
			const double   b = tile_src_min_w + tile_src_min_h;
			const double   c = tile_src_min_w * tile_src_min_h - _buf_size;
			const double   d = b * b - 4 * a * c;
			const int      l = fstb::round_int ((sqrt (d) - b) / (2 * a));

			tile_src_w = (l + tile_src_min_w);
			bool           search_flag = true;
			do
			{
				const int      old_tile_src_w = tile_src_w;
				tile_src_w = std::min (tile_src_w, _crop_size [Dir_H]);
				if (_resize_flag [Dir_V])
				{
					tile_src_w = (tile_src_w + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
				}
				tile_src_h = _buf_size / tile_src_w;
				tile_src_h = std::min (tile_src_h, _crop_size [Dir_V] + Scaler::SRC_ALIGN - 1);
				tile_src_h &= -Scaler::SRC_ALIGN;
				search_flag = (tile_src_h < std::min (tile_src_min_h, _crop_size [Dir_V]));
				if (search_flag)
				{
					tile_src_w = old_tile_src_w / 2;
					if (   tile_src_w < tile_src_min_w
					    || tile_src_w >= old_tile_src_w)
					{
						// Try again with a bigger buffer.
						search_flag = false;
						bigger_flag = true;
					}
				}
			}
			while (search_flag);

			if (! bigger_flag)
			{
				// Computes the equivalent destination size
				tile_dst_w = tile_src_w;
				if (_resize_flag [Dir_H])
				{
					tile_dst_w = Scaler::eval_lower_bound_of_dst_tile_height (
						tile_src_w,
						_dst_size [Dir_H],
						_win_size [Dir_H],
						*(_kernel_ptr_arr [Dir_H]),
						_kernel_scale [Dir_H],
						_crop_size [Dir_H]
					);
					tile_dst_w = std::max (tile_dst_w, tile_dst_min_w);
				}

				tile_dst_h = tile_src_h;
				if (_resize_flag [Dir_V])
				{
					tile_dst_h = Scaler::eval_lower_bound_of_dst_tile_height (
						tile_src_h,
						_dst_size [Dir_V],
						_win_size [Dir_V],
						*(_kernel_ptr_arr [Dir_V]),
						_kernel_scale [Dir_V],
						_crop_size [Dir_V]
					);
					tile_dst_h = std::max (tile_dst_h, tile_dst_min_h);
				}

				// Limits the destination size
				if (! vert_last_flag)	// If we use a buffer for the last step
				{
					const double   area_final = double (tile_dst_w) * double (tile_dst_h);
					assert (area_final > 0);
					const double   fix_final = sqrt (_buf_size / area_final);
					if (fix_final < 1)
					{
						tile_dst_w = fstb::floor_int (tile_dst_w * fix_final) & -Scaler::SRC_ALIGN;
						tile_dst_h = fstb::floor_int (tile_dst_h * fix_final) & -Scaler::SRC_ALIGN;
					}
				}

				// Limits the temporary size
				if (vert_last_flag)
				{
					const int      stride =
						(tile_dst_w + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
					const double   area_tmp = stride * tile_src_h;
					assert (area_tmp > 0);
					const double   fix_tmp = _buf_size / area_tmp;
					if (fix_tmp < 1)
					{
						tile_dst_w = fstb::floor_int (tile_dst_w * fix_tmp);
					}
//					tile_dst_h &= -Scaler::SRC_ALIGN;
				}
				else
				{
					const int      stride =
						(tile_src_w + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
					const double   area_tmp = stride * tile_dst_h;
					assert (area_tmp > 0);
					const double   fix_tmp = _buf_size / area_tmp;
					if (fix_tmp < 1)
					{
						tile_dst_h = fstb::floor_int (tile_dst_h * fix_tmp) & -Scaler::SRC_ALIGN;
					}
//					tile_dst_w &= -Scaler::SRC_ALIGN;
				}

				// For the last step, we need to ensure that the window width is well
				// aligned, otherwise we might overflow the end of the lines by
				// starting the rightmost tile on an unaligned column.
				if (tile_dst_w < _dst_size [Dir_H])
				{
					tile_dst_w &= -Scaler::SRC_ALIGN;
				}

				if (tile_dst_w <= 0 || tile_dst_h <= 0)
				{
					bigger_flag = true;
				}
			}	// bigger_flag

			if (bigger_flag)
			{
				_buf_size <<= 1;
				if (_buf_size > MAX_BUF_SIZE)
				{
					bigger_flag = false;
					throw std::runtime_error (
						"Dither_resize16: "
						"resizing ratio too low or kernel support too high."
					);
				}
			}
		}
		while (bigger_flag);

		tile_dst_w = std::max (tile_dst_w, int (Scaler::SRC_ALIGN));
		tile_dst_h = std::max (tile_dst_h, 1);

		_tile_size_dst [Dir_H] = tile_dst_w;
		_tile_size_dst [Dir_V] = tile_dst_h;

		_factory_uptr = std::unique_ptr <ResizeDataFactory> (
			new ResizeDataFactory (_buf_size, 1)
		);
		_pool.set_factory (*_factory_uptr);
	}

	// ! buffer_flag
	else
	{
		_tile_size_dst [Dir_H] = _dst_size [Dir_H];
		_tile_size_dst [Dir_V] = _dst_size [Dir_V];
	}

	if (_nbr_passes > 0)
	{
		for (int dir = 0; dir < Dir_NBR_ELT; ++dir)
		{
			assert (_resize_flag [dir] || _bd_chg_dir != dir);
			if (_resize_flag [dir])
			{
				double         dir_gain = (dir == _bd_chg_dir) ? gain          : 1;
				double         dir_acst = (dir == _bd_chg_dir) ? spec._add_cst : 0;

				// When using integer operations, we want to cancel the scaling
				// intended to change the bitdepth. Indeed, this scaling is
				// performed by a bitshift in the resizing function.
				if (_int_flag && dir == _bd_chg_dir)
				{
					const double   inv_scale = pow (2.0, _src_res - _dst_res);
					dir_gain *= inv_scale;
					dir_acst *= inv_scale;
				}

				_scaler_uptr [dir] = std::unique_ptr <Scaler> (new Scaler (
					_crop_size [dir], _dst_size [dir],
					_win_pos [dir] - _crop_pos [dir], _win_size [dir],
					*(_kernel_ptr_arr [dir]), _kernel_scale [dir],
					_norm_flag, _norm_val [dir],
					_center_pos_src [dir], _center_pos_dst [dir],
					dir_gain, dir_acst, _int_flag, _sse2_flag, _avx2_flag
				));
			}
		}
	}
}



void	FilterResize::process_plane (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_dst, int stride_src, bool chroma_flag)
{
	assert (dst_msb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (stride_dst > 0);
	assert (stride_src > 0);

	if (_nbr_passes <= 0)
	{
		process_plane_bypass (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			stride_dst, stride_src, chroma_flag
		);
	}
	else
	{
		process_plane_normal (
			dst_msb_ptr, dst_lsb_ptr,
			src_msb_ptr, src_lsb_ptr,
			stride_dst, stride_src
		);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	FilterResize::process_plane_bypass (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_dst, int stride_src, bool chroma_flag)
{
	fstb::unused (chroma_flag);

	assert (_nbr_passes <= 0);
	assert (dst_msb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (stride_dst > 0);
	assert (stride_src > 0);

	const int      nbr_bytes_src = fmtcl::SplFmt_get_unit_size (_src_type);
	const int      offset_src =
		  fstb::round_int (_win_pos [Dir_V]) * stride_src
		+ fstb::round_int (_win_pos [Dir_H]) * nbr_bytes_src;
	src_msb_ptr += offset_src;
	src_lsb_ptr += offset_src;

	BitBltConv::ScaleInfo * scale_info_ptr = 0;
	BitBltConv::ScaleInfo   scale_info;
	const bool     dst_flt_flag = (_dst_type == SplFmt_FLOAT);
	const bool     src_flt_flag = (_src_type == SplFmt_FLOAT);
	if (dst_flt_flag != src_flt_flag)
	{
		scale_info._gain    = _gain;
		scale_info._add_cst = _add_cst;

		scale_info_ptr = &scale_info;
	}

	_blitter.bitblt (
		_dst_type, _dst_res, dst_msb_ptr, dst_lsb_ptr, stride_dst,
		_src_type, _src_res, src_msb_ptr, src_lsb_ptr, stride_src,
		_dst_size [Dir_H],
		_dst_size [Dir_V],
		scale_info_ptr
	);
}



void	FilterResize::process_plane_normal (uint8_t *dst_msb_ptr, uint8_t *dst_lsb_ptr, const uint8_t *src_msb_ptr, const uint8_t *src_lsb_ptr, int stride_dst, int stride_src)
{
	assert (_nbr_passes > 0);
	assert (dst_msb_ptr != 0);
	assert (src_msb_ptr != 0);
	assert (stride_dst > 0);
	assert (stride_src > 0);

	avstp_TaskDispatcher *	task_dispatcher_ptr = _avstp.create_dispatcher ();

	TaskRszGlobal	trg;
	trg._this_ptr       = this;
	trg._dst_msb_ptr    = dst_msb_ptr;
	trg._dst_lsb_ptr    = dst_lsb_ptr;
	trg._src_msb_ptr    = src_msb_ptr;
	trg._src_lsb_ptr    = src_lsb_ptr;
	trg._dst_bpp        = fmtcl::SplFmt_get_unit_size (_dst_type);
	trg._src_bpp        = fmtcl::SplFmt_get_unit_size (_src_type);
	trg._stride_dst     = stride_dst;
	trg._stride_src     = stride_src;
	trg._offset_crop    =
		_crop_pos [Dir_V] * stride_src + _crop_pos [Dir_H] * trg._src_bpp;
	trg._stride_dst_pix = stride_dst / trg._dst_bpp;
	trg._stride_src_pix = stride_src / trg._src_bpp;
	assert (stride_dst % trg._dst_bpp == 0);
	assert (stride_src % trg._src_bpp == 0);

	int            dst_beg [Dir_NBR_ELT]  = { 0, 0 };
	int            work_dst [Dir_NBR_ELT] = { 0, 0 };
	int            src_beg [Dir_NBR_ELT]  = { 0, 0 };
	int            src_end [Dir_NBR_ELT]  = { 0, 0 };

	for (dst_beg [Dir_V] = 0
	;	dst_beg [Dir_V] < _dst_size [Dir_V]
	;	dst_beg [Dir_V] += _tile_size_dst [Dir_V])
	{
		work_dst [Dir_V] = std::min (
			_tile_size_dst [Dir_V],
			_dst_size [Dir_V] - dst_beg [Dir_V]
		);

		src_beg [Dir_V] = 0;
		src_end [Dir_V] = 0;
		if (_resize_flag [Dir_V])
		{
			_scaler_uptr [Dir_V]->get_src_boundaries (
				src_beg [Dir_V],
				src_end [Dir_V],
				dst_beg [Dir_V],
				dst_beg [Dir_V] + work_dst [Dir_V]
			);
		}
		else
		{
			src_beg [Dir_V] = dst_beg [Dir_V];
			src_end [Dir_V] = src_beg [Dir_V] + work_dst [Dir_V];
		}

		for (dst_beg [Dir_H] = 0
		;	dst_beg [Dir_H] < _dst_size [Dir_H]
		;	dst_beg [Dir_H] += _tile_size_dst [Dir_H])
		{
			work_dst [Dir_H] = std::min (
				_tile_size_dst [Dir_H],
				_dst_size [Dir_H] - dst_beg [Dir_H]
			);

			src_beg [Dir_H] = 0;
			src_end [Dir_H] = 0;
			if (_resize_flag [Dir_H])
			{
				_scaler_uptr [Dir_H]->get_src_boundaries (
					src_beg [Dir_H],
					src_end [Dir_H],
					dst_beg [Dir_H],
					dst_beg [Dir_H] + work_dst [Dir_H]
				);
			}
			else
			{
				src_beg [Dir_H] = dst_beg [Dir_H];
				src_end [Dir_H] = src_beg [Dir_H] + work_dst [Dir_H];
			}

			// The cell will be returned to the pool by the task.
			TaskRszCell *	tr_cell_ptr = _task_rsz_pool.take_cell (true);
			if (tr_cell_ptr == 0)
			{
				throw std::runtime_error (
					"Dither_resize16: Cannot allocate task cell."
				);
			}

			TaskRsz &		tr = tr_cell_ptr->_val;
			tr._glob_data_ptr = &trg;
			for (int d = 0; d < Dir_NBR_ELT; ++d)
			{
				tr._dst_beg [d]  = dst_beg [d];
				tr._src_beg [d]  = src_beg [d];
				tr._src_end [d]  = src_end [d];
				tr._work_dst [d] = work_dst [d];
			}

			_avstp.enqueue_task (
				task_dispatcher_ptr,
				&redirect_task_resize,
				tr_cell_ptr
			);
		}	// for Dir_H
	}	// for Dir_V

	_avstp.wait_completion (task_dispatcher_ptr);

	// Done
	_avstp.destroy_dispatcher (task_dispatcher_ptr);
	task_dispatcher_ptr = 0;
}



void	FilterResize::process_tile (TaskRszCell &tr_cell)
{
#if (fstb_ARCHI == fstb_ARCHI_X86)
 #if ! defined (_WIN64) && ! defined (__64BIT__) && ! defined (__amd64__) && ! defined (__x86_64__)
	// We can arrive here from any thread and we don't know the state of the
	// FP/MMX registers (it could have been changed by the previous filter that
	// used avstp) so we explicitely switch to the FP registers as we're going
	// to use them.
	_mm_empty ();
 #endif
#endif

	const TaskRsz &      tr  = tr_cell._val;
	const TaskRszGlobal& trg = *(tr._glob_data_ptr);
	assert (trg._this_ptr == this);

	ResizeData *   rd_ptr = 0;
	if (_buffer_flag)
	{
		assert (_factory_uptr.get () != 0);

		rd_ptr = _pool.take_obj ();
		if (rd_ptr == 0)
		{
			throw std::runtime_error (
				"Dither_resize16: Cannot allocate buffer memory."
			);
		}
	}

	int            cur_buf        = 0;
	int            stride_buf [2] = { 0, 0 }; // In pixels
	Dir            cur_dir        = Dir_V;

	// Size of the current tile in the tile's coordinates,
	// not the original picture coordinates.
	int            cur_size [Dir_NBR_ELT] =
	{
		tr._src_end [Dir_H] - tr._src_beg [Dir_H],
		tr._src_end [Dir_V] - tr._src_beg [Dir_V]
	};

	for (int pass = 0; pass < _nbr_passes; ++pass)
	{
		switch (_roadmap [pass])
		{
		case	PassType_RESIZE:
			process_tile_resize (
				tr, trg, *rd_ptr, stride_buf, pass, cur_dir, cur_buf, cur_size
			);
			break;

		case	PassType_TRANSPOSE:
			if (_int_flag)
			{
				process_tile_transpose <uint16_t, SplFmt_INT16> (
					tr, trg, *rd_ptr, stride_buf, pass, cur_dir, cur_buf, cur_size
				);
			}
			else
			{
				process_tile_transpose <float, SplFmt_FLOAT> (
					tr, trg, *rd_ptr, stride_buf, pass, cur_dir, cur_buf, cur_size
				);
			}
			break;

		case	PassType_NONE:
			// Nothing
			break;

		default:
			assert (false);
		}	// switch _roadmap [pass]
	}	// for pass

	if (rd_ptr != 0)
	{
		_pool.return_obj (*rd_ptr);
		rd_ptr = 0;
	}

	_task_rsz_pool.return_cell (tr_cell);
}

#undef fmtc_FilterResize_MAKE_PTR



void	FilterResize::process_tile_resize (const TaskRsz &tr, const TaskRszGlobal& trg, ResizeData &rd, int stride_buf [2], const int pass, Dir &cur_dir, int &cur_buf, int cur_size [Dir_NBR_ELT])
{
	Proxy::PtrStack16Const::Type	src_s16_ptr (0, 0);
	const float *                 src_flt_ptr = 0;
	const uint16_t *              src_i16_ptr = 0;
	const uint8_t *               src_i08_ptr = 0;
	int                           src_stride  = 0;  // Pixels
	SplFmt                        src_fmt_loc = SplFmt_ILLEGAL;
	int                           src_res_loc = 0;

	Proxy::PtrStack16::Type       dst_s16_ptr (0, 0);
	float *                       dst_flt_ptr = 0;
	uint16_t *                    dst_i16_ptr = 0;
	int                           dst_stride  = 0;  // Pixels
	SplFmt                        dst_fmt_loc = SplFmt_ILLEGAL;

	// Source is the input
	if (pass == 0)
	{
		assert (cur_dir == Dir_V);

		const int		offset_src =
				trg._offset_crop
			+ tr._src_beg [Dir_H] * trg._src_bpp;

		// Source pointers
		const uint8_t *  src_msb_ofs_ptr = trg._src_msb_ptr + offset_src;

		src_s16_ptr = Proxy::PtrStack16Const::Type (
			src_msb_ofs_ptr,
			trg._src_lsb_ptr + offset_src
		);
		src_flt_ptr = reinterpret_cast <const float *> (src_msb_ofs_ptr);
		src_i16_ptr = reinterpret_cast <const uint16_t *> (src_msb_ofs_ptr);
		src_i08_ptr = src_msb_ofs_ptr;
		src_stride  = trg._stride_src_pix;
		src_fmt_loc = _src_type;
		src_res_loc = _src_res;

		// Src = input, dst = buffer
		if (has_buf_dst (pass))
		{
			assert (_buffer_flag);

			stride_buf [cur_buf] =
				(cur_size [Dir_H] + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
			assert (tr._work_dst [cur_dir] * stride_buf [cur_buf] <= _buf_size);

			dst_flt_ptr = rd.use_buf <float   > (cur_buf);
			dst_i16_ptr = rd.use_buf <uint16_t> (cur_buf);
			dst_stride  = stride_buf [cur_buf];
			dst_fmt_loc = (_int_flag) ? SplFmt_INT16 : SplFmt_FLOAT;
		}

		// Direct vertical scaling (no buffer)
		else
		{
			assert (! _buffer_flag);

			const int		offset_dst =
				  tr._dst_beg [Dir_V] * trg._stride_dst
				+ tr._dst_beg [Dir_H] * trg._dst_bpp;

			uint8_t *      dst_msb_ofs_ptr = trg._dst_msb_ptr + offset_dst;

			dst_s16_ptr = Proxy::PtrStack16::Type (
				dst_msb_ofs_ptr,
				trg._dst_lsb_ptr + offset_dst
			);
			dst_flt_ptr = reinterpret_cast <float *> (dst_msb_ofs_ptr);
			dst_i16_ptr = reinterpret_cast <uint16_t *> (dst_msb_ofs_ptr);
			dst_stride  = trg._stride_dst_pix;
			dst_fmt_loc = _dst_type;
		}
	}  // pass == 0

	// pass > 0: source is a buffer.
	else
	{
		assert (_buffer_flag);

		const int		offset_src = -tr._src_beg [cur_dir] * stride_buf [cur_buf];
		src_flt_ptr = rd.use_buf <const float> (cur_buf) + offset_src;
		src_i16_ptr = rd.use_buf <uint16_t   > (cur_buf) + offset_src;
		src_stride  = stride_buf [cur_buf];
		src_fmt_loc = (_int_flag) ? SplFmt_INT16 : SplFmt_FLOAT;
		src_res_loc = (_int_flag) ? 16           : 32;

		// With integer operations, if the first op is a transpose AND there is
		// no format conversion (when input and buffers have the same number of
		// bytes but not the same bitdepth), the buffer is still assumed being
		// in the input bitdepth.
		if (   _int_flag && _roadmap [0] == PassType_TRANSPOSE
		    && _bd_chg_dir == cur_dir
		    && _src_res > 8 && _src_res < 16)
		{
			src_res_loc = _src_res;
		}

		// Src = buffer, dst = buffer
		if (has_buf_dst (pass))
		{
			stride_buf [1 - cur_buf] = stride_buf [cur_buf];
			assert (tr._work_dst [cur_dir] * stride_buf [1 - cur_buf] <= _buf_size);

			dst_flt_ptr = rd.use_buf <float   > (1 - cur_buf);
			dst_i16_ptr = rd.use_buf <uint16_t> (1 - cur_buf);
			dst_stride  = stride_buf [1 - cur_buf];
			dst_fmt_loc = (_int_flag) ? SplFmt_INT16 : SplFmt_FLOAT;

			cur_buf = 1 - cur_buf;
		}

		// Src = buffer, dst = output
		else
		{
			assert (cur_dir == Dir_V);

			const int		offset_dst =
				  tr._dst_beg [Dir_V] * trg._stride_dst
				+ tr._dst_beg [Dir_H] * trg._dst_bpp;

			uint8_t *      dst_msb_ofs_ptr = trg._dst_msb_ptr + offset_dst;

			dst_s16_ptr = Proxy::PtrStack16::Type (
				dst_msb_ofs_ptr,
				trg._dst_lsb_ptr + offset_dst
			);
			dst_flt_ptr = reinterpret_cast <float *> (dst_msb_ofs_ptr);
			dst_i16_ptr = reinterpret_cast <uint16_t *> (dst_msb_ofs_ptr);
			dst_stride  = trg._stride_dst_pix;
			dst_fmt_loc = _dst_type;
		}
	}  // pass > 0

#define fmtc_FilterResize_SHORT_BD(x) \
	( (x ==  8) ? 0 \
	: (x ==  9) ? 1 \
	: (x == 10) ? 2 \
	: (x == 12) ? 3 \
	: (x == 14) ? 4 \
	: (x == 16) ? 5 : -1)

#define fmtc_FilterResize_PROC_F(DF, DP, SF, SP) \
	case	((SplFmt_##DF << 2) + SplFmt_##SF): \
		_scaler_uptr [cur_dir]->process_plane_flt ( \
			dst_##DP##_ptr, \
			src_##SP##_ptr, \
			dst_stride, \
			src_stride, \
			cur_size [Dir_H], \
			tr._dst_beg [cur_dir], \
			tr._dst_beg [cur_dir] + tr._work_dst [cur_dir] \
		); \
		break;

#define fmtc_FilterResize_PROC_I(DF, DP, SF, SP, SB, FN) \
	case	((fmtc_FilterResize_SHORT_BD (SB) << 4) + (SplFmt_##DF << 2) + SplFmt_##SF): \
		_scaler_uptr [cur_dir]->process_plane_int_##FN ( \
			dst_##DP##_ptr, \
			src_##SP##_ptr, \
			dst_stride, \
			src_stride, \
			cur_size [Dir_H], \
			tr._dst_beg [cur_dir], \
			tr._dst_beg [cur_dir] + tr._work_dst [cur_dir] \
		); \
		break;

	if (_int_flag)
	{
		switch (  (fmtc_FilterResize_SHORT_BD (src_res_loc) << 4)
		        + (dst_fmt_loc << 2) + src_fmt_loc)
		{
		fmtc_FilterResize_PROC_I (INT16  , i16, INT16  , i16, 16, i16_i16)
		fmtc_FilterResize_PROC_I (INT16  , i16, INT16  , i16, 14, i16_i14)
		fmtc_FilterResize_PROC_I (INT16  , i16, INT16  , i16, 12, i16_i12)
		fmtc_FilterResize_PROC_I (INT16  , i16, INT16  , i16, 10, i16_i10)
		fmtc_FilterResize_PROC_I (INT16  , i16, INT16  , i16,  9, i16_i09)
		fmtc_FilterResize_PROC_I (INT16  , i16, STACK16, s16, 16, i16_s16)
		fmtc_FilterResize_PROC_I (INT16  , i16, INT8   , i08,  8, i16_i08)
		fmtc_FilterResize_PROC_I (STACK16, s16, INT16  , i16, 16, s16_i16)
		fmtc_FilterResize_PROC_I (STACK16, s16, INT16  , i16, 14, s16_i14)
		fmtc_FilterResize_PROC_I (STACK16, s16, INT16  , i16, 12, s16_i12)
		fmtc_FilterResize_PROC_I (STACK16, s16, INT16  , i16, 10, s16_i10)
		fmtc_FilterResize_PROC_I (STACK16, s16, INT16  , i16,  9, s16_i09)
		fmtc_FilterResize_PROC_I (STACK16, s16, STACK16, s16, 16, s16_s16)
		fmtc_FilterResize_PROC_I (STACK16, s16, INT8   , i08,  8, s16_i08)
		default:
			assert (false);
			throw std::logic_error ("Unexpected pixel format (int)");
		}
	}
	else
	{
		switch ((dst_fmt_loc << 2) + src_fmt_loc)
		{
		fmtc_FilterResize_PROC_F (FLOAT  , flt, FLOAT  , flt)
		fmtc_FilterResize_PROC_F (FLOAT  , flt, INT16  , i16)
		fmtc_FilterResize_PROC_F (FLOAT  , flt, STACK16, s16)
		fmtc_FilterResize_PROC_F (FLOAT  , flt, INT8   , i08)
		fmtc_FilterResize_PROC_F (INT16  , i16, FLOAT  , flt)
		fmtc_FilterResize_PROC_F (INT16  , i16, INT16  , i16)
		fmtc_FilterResize_PROC_F (INT16  , i16, STACK16, s16)
		fmtc_FilterResize_PROC_F (INT16  , i16, INT8   , i08)
		fmtc_FilterResize_PROC_F (STACK16, s16, FLOAT  , flt)
		fmtc_FilterResize_PROC_F (STACK16, s16, INT16  , i16)
		fmtc_FilterResize_PROC_F (STACK16, s16, STACK16, s16)
		fmtc_FilterResize_PROC_F (STACK16, s16, INT8   , i08)
		default:
			assert (false);
			throw std::logic_error ("Unexpected pixel format (flt)");
		}
	}

#undef fmtc_FilterResize_PROC

	cur_size [Dir_V] = tr._work_dst [cur_dir];
}



template <typename T, SplFmt BUFT>
void	FilterResize::process_tile_transpose (const TaskRsz &tr, const TaskRszGlobal& trg, ResizeData &rd, int stride_buf [2], const int pass, Dir &cur_dir, int &cur_buf, int cur_size [Dir_NBR_ELT])
{
	stride_buf [1 - cur_buf] =
		(cur_size [Dir_V] + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
	assert (cur_size [Dir_H] * stride_buf [1 - cur_buf] <= _buf_size);

	const T *       ptr_src      = rd.use_buf <T> (    cur_buf);
	int             stride_src   = stride_buf [    cur_buf]; // Pixels
	int             offset_src   = 0;
	const bool      src_buf_flag = has_buf_src (pass);

	T *             ptr_dst      = rd.use_buf <T> (1 - cur_buf);
	int             stride_dst   = stride_buf [1 - cur_buf]; // Pixels
	int             offset_dst   = 0;
	const bool      dst_buf_flag = has_buf_dst (pass);

	// If the source or destination are not one of the buffers,
	// computes the src/dst information.
	if (! src_buf_flag)
	{
		assert (cur_dir == Dir_V);

		offset_src =            // In bytes
			  trg._offset_crop
			+ tr._src_beg [Dir_V] * trg._stride_src
			+ tr._src_beg [Dir_H] * trg._src_bpp;

		if (_src_type == BUFT)
		{
			ptr_src    = reinterpret_cast <const T *> (
				trg._src_msb_ptr + offset_src
			);
			stride_src = trg._stride_src_pix;
		}
	}
	if (! dst_buf_flag)
	{
		assert (cur_dir == Dir_H);

		offset_dst =            // In bytes
			  tr._dst_beg [Dir_V] * trg._stride_dst
			+ tr._dst_beg [Dir_H] * trg._dst_bpp;

		if (_dst_type == BUFT)
		{
			ptr_dst    = reinterpret_cast <      T *> (
				trg._dst_msb_ptr + offset_dst
			);
			stride_dst = trg._stride_dst_pix;
		}
	}

	// Preliminary pass: converts input in buffer type if required
	if (! src_buf_flag && _src_type != BUFT)
	{
		stride_src =
			(cur_size [Dir_H] + Scaler::SRC_ALIGN - 1) & -Scaler::SRC_ALIGN;
		assert (cur_size [Dir_V] * stride_src <= _buf_size);

		_blitter.bitblt (
			BUFT, sizeof (T) * CHAR_BIT,
			rd.use_buf <uint8_t> (cur_buf),
			0,
			stride_src * sizeof (T),
			_src_type, _src_res,
			trg._src_msb_ptr + offset_src,
			trg._src_lsb_ptr + offset_src,
			trg._stride_src,
			cur_size [Dir_H], cur_size [Dir_V],
			0
		);
	}

	// Transposition
	transpose (
		ptr_dst,
		ptr_src,
		cur_size [Dir_H], cur_size [Dir_V],
		stride_dst,
		stride_src
	);

	cur_dir = (cur_dir == Dir_V) ? Dir_H : Dir_V;
	std::swap (cur_size [Dir_H], cur_size [Dir_V]);

	cur_buf = 1 - cur_buf;

	// Last pass: converts to output format if required
	if (! dst_buf_flag && _dst_type != BUFT)
	{
		_blitter.bitblt (
			_dst_type, _dst_res,
			trg._dst_msb_ptr + offset_dst,
			trg._dst_lsb_ptr + offset_dst,
			trg._stride_dst,
			BUFT, sizeof (T) * CHAR_BIT,
			rd.use_buf <const uint8_t> (cur_buf),
			0,
			stride_buf [cur_buf] * sizeof (float),
			tr._work_dst [Dir_H], tr._work_dst [Dir_V],
			0
		);
	}
}



// w and h are related to the source.
template <typename T>
void	FilterResize::transpose (T *dst_ptr, const T *src_ptr, int w, int h, int stride_dst, int stride_src)
{
	assert (src_ptr != 0);
	assert (w > 0);
	assert (h > 0);
	assert (stride_src > 0);
	assert (dst_ptr != 0);
	assert (stride_dst > 0);

#if (fstb_ARCHI == fstb_ARCHI_X86)
	if (_sse2_flag)
	{
		transpose_sse2 (dst_ptr, src_ptr, w, h, stride_dst, stride_src);
	}
	else
#endif
	{
		transpose_cpp (dst_ptr, src_ptr, w, h, stride_dst, stride_src);
	}
}



template <typename T>
void	FilterResize::transpose_cpp (T *dst_ptr, const T *src_ptr, int w, int h, int stride_dst, int stride_src)
{
	assert (src_ptr != 0);
	assert (w > 0);
	assert (h > 0);
	assert (stride_src > 0);
	assert (dst_ptr != 0);
	assert (stride_dst > 0);

	for (int y = 0; y < h; ++y)
	{
		T *            dst_2_ptr = dst_ptr + y;

		for (int x = 0; x < w; ++x)
		{
			*dst_2_ptr = src_ptr [x];
			dst_2_ptr += stride_dst;
		}

		src_ptr += stride_src;
	}
}



#if (fstb_ARCHI == fstb_ARCHI_X86)

void	FilterResize::transpose_sse2 (float *dst_ptr, const float *src_ptr, int w, int h, int stride_dst, int stride_src)
{
	assert (src_ptr != 0);
	assert (w > 0);
	assert (h > 0);
	assert (stride_src > 0);
	assert (dst_ptr != 0);
	assert (stride_dst > 0);

	const int      w4 = w & -4;
	const int      w3 = w - w4;
	const int      h4 = h & -4;
	const int      h3 = h - h4;

	const bool     dst_align_flag =
		((reinterpret_cast <ptrdiff_t> (dst_ptr) & 15) == 0);

	for (int y = 0; y < h4; y += 4)
	{
		float *        dst_2_ptr = dst_ptr + y;

		for (int x = 0; x < w4; x += 4)
		{
			__m128         a0 = _mm_loadu_ps (src_ptr                  + x);
			__m128         a1 = _mm_loadu_ps (src_ptr + stride_src     + x);
			__m128         a2 = _mm_loadu_ps (src_ptr + stride_src * 2 + x);
			__m128         a3 = _mm_loadu_ps (src_ptr + stride_src * 3 + x);

			const __m128   b0 = _mm_shuffle_ps (a0, a1, 0x44);
			const __m128   b2 = _mm_shuffle_ps (a0, a1, 0xEE);
			const __m128   b1 = _mm_shuffle_ps (a2, a3, 0x44);
			const __m128   b3 = _mm_shuffle_ps (a2, a3, 0xEE);

			a0 = _mm_shuffle_ps (b0, b1, 0x88);
			a1 = _mm_shuffle_ps (b0, b1, 0xDD);
			a2 = _mm_shuffle_ps (b2, b3, 0x88);
			a3 = _mm_shuffle_ps (b2, b3, 0xDD);

			if (dst_align_flag)
			{
				_mm_store_ps (dst_2_ptr                 , a0);
				_mm_store_ps (dst_2_ptr + stride_dst    , a1);
				_mm_store_ps (dst_2_ptr + stride_dst * 2, a2);
				_mm_store_ps (dst_2_ptr + stride_dst * 3, a3);
			}
			else
			{
				_mm_storeu_ps (dst_2_ptr                 , a0);
				_mm_storeu_ps (dst_2_ptr + stride_dst    , a1);
				_mm_storeu_ps (dst_2_ptr + stride_dst * 2, a2);
				_mm_storeu_ps (dst_2_ptr + stride_dst * 3, a3);
			}

			dst_2_ptr += stride_dst * 4;
		}

		if (w3 > 0)
		{
			transpose_cpp (dst_2_ptr, src_ptr + w4, w3, 4, stride_dst, stride_src);
		}

		src_ptr += stride_src * 4;
	}

	if (h3 > 0)
	{
		transpose_cpp (dst_ptr + h4, src_ptr, w, h3, stride_dst, stride_src);
	}
}



void	FilterResize::transpose_sse2 (uint16_t *dst_ptr, const uint16_t *src_ptr, int w, int h, int stride_dst, int stride_src)
{
	assert (src_ptr != 0);
	assert (w > 0);
	assert (h > 0);
	assert (stride_src > 0);
	assert (dst_ptr != 0);
	assert (stride_dst > 0);

	const int      w8 = w & -8;
	const int      w7 = w - w8;
	const int      h8 = h & -8;
	const int      h7 = h - h8;

	const bool     dst_align_flag =
		((reinterpret_cast <ptrdiff_t> (dst_ptr) & 15) == 0);

	for (int y = 0; y < h8; y += 8)
	{
		uint16_t *     dst_2_ptr = dst_ptr + y;

		for (int x = 0; x < w8; x += 8)
		{
			// Based on a piece of code published by Stephen Thomas
			// http://stackoverflow.com/questions/2517584/transpose-for-8-registers-of-16-bit-elements-on-sse2-ssse3

			const __m128i a = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr                  + x));
			const __m128i b = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src     + x));
			const __m128i c = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src * 2 + x));
			const __m128i d = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src * 3 + x));
			const __m128i e = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src * 4 + x));
			const __m128i f = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src * 5 + x));
			const __m128i g = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src * 6 + x));
			const __m128i i = _mm_loadu_si128 (reinterpret_cast <const __m128i *> (src_ptr + stride_src * 7 + x));

			const __m128i a03b03 = _mm_unpacklo_epi16 (a, b);
			const __m128i c03d03 = _mm_unpacklo_epi16 (c, d);
			const __m128i e03f03 = _mm_unpacklo_epi16 (e, f);
			const __m128i g03i03 = _mm_unpacklo_epi16 (g, i);
			const __m128i a47b47 = _mm_unpackhi_epi16 (a, b);
			const __m128i c47d47 = _mm_unpackhi_epi16 (c, d);
			const __m128i e47f47 = _mm_unpackhi_epi16 (e, f);
			const __m128i g47i47 = _mm_unpackhi_epi16 (g, i);

			const __m128i a01b01c01d01 = _mm_unpacklo_epi32 (a03b03, c03d03);
			const __m128i a23b23c23d23 = _mm_unpackhi_epi32 (a03b03, c03d03);
			const __m128i e01f01g01i01 = _mm_unpacklo_epi32 (e03f03, g03i03);
			const __m128i e23f23g23i23 = _mm_unpackhi_epi32 (e03f03, g03i03);
			const __m128i a45b45c45d45 = _mm_unpacklo_epi32 (a47b47, c47d47);
			const __m128i a67b67c67d67 = _mm_unpackhi_epi32 (a47b47, c47d47);
			const __m128i e45f45g45i45 = _mm_unpacklo_epi32 (e47f47, g47i47);
			const __m128i e67f67g67i67 = _mm_unpackhi_epi32 (e47f47, g47i47);

			const __m128i a0b0c0d0e0f0g0i0 = _mm_unpacklo_epi64 (a01b01c01d01, e01f01g01i01);
			const __m128i a1b1c1d1e1f1g1i1 = _mm_unpackhi_epi64 (a01b01c01d01, e01f01g01i01);
			const __m128i a2b2c2d2e2f2g2i2 = _mm_unpacklo_epi64 (a23b23c23d23, e23f23g23i23);
			const __m128i a3b3c3d3e3f3g3i3 = _mm_unpackhi_epi64 (a23b23c23d23, e23f23g23i23);
			const __m128i a4b4c4d4e4f4g4i4 = _mm_unpacklo_epi64 (a45b45c45d45, e45f45g45i45);
			const __m128i a5b5c5d5e5f5g5i5 = _mm_unpackhi_epi64 (a45b45c45d45, e45f45g45i45);
			const __m128i a6b6c6d6e6f6g6i6 = _mm_unpacklo_epi64 (a67b67c67d67, e67f67g67i67);
			const __m128i a7b7c7d7e7f7g7i7 = _mm_unpackhi_epi64 (a67b67c67d67, e67f67g67i67);

			if (dst_align_flag)
			{
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr                 ), a0b0c0d0e0f0g0i0);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst    ), a1b1c1d1e1f1g1i1);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 2), a2b2c2d2e2f2g2i2);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 3), a3b3c3d3e3f3g3i3);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 4), a4b4c4d4e4f4g4i4);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 5), a5b5c5d5e5f5g5i5);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 6), a6b6c6d6e6f6g6i6);
				_mm_store_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 7), a7b7c7d7e7f7g7i7);
			}
			else
			{
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr                 ), a0b0c0d0e0f0g0i0);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst    ), a1b1c1d1e1f1g1i1);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 2), a2b2c2d2e2f2g2i2);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 3), a3b3c3d3e3f3g3i3);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 4), a4b4c4d4e4f4g4i4);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 5), a5b5c5d5e5f5g5i5);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 6), a6b6c6d6e6f6g6i6);
				_mm_storeu_si128 (reinterpret_cast <__m128i *> (dst_2_ptr + stride_dst * 7), a7b7c7d7e7f7g7i7);
			}

			dst_2_ptr += stride_dst * 8;
		}

		if (w7 > 0)
		{
			transpose_cpp (dst_2_ptr, src_ptr + w8, w7, 8, stride_dst, stride_src);
		}

		src_ptr += stride_src * 8;
	}

	if (h7 > 0)
	{
		transpose_cpp (dst_ptr + h8, src_ptr, w, h7, stride_dst, stride_src);
	}
}

#endif



// This function looks not used anymore...
bool	FilterResize::is_kernel_neutral (Dir dir) const
{
	const ContFirInterface &   kernel = *(_kernel_ptr_arr [dir]);

	const double   center = kernel.get_val (0);
	bool           neutral_flag = fabs (center) > 1e-3;
	if (neutral_flag)
	{
		const int      m = fstb::ceil_int (kernel.get_support ());
		double         sum = 0;
		for (int k = 1; k <= m && neutral_flag; ++k)
		{
			const double	vn = kernel.get_val (-k);
			const double	vp = kernel.get_val (+k);
			sum += fabs (vn) + fabs (vp);
		}
		neutral_flag = (sum / center < 1e-3);
	}

	return (neutral_flag);
}



bool	FilterResize::has_buf_src (int pass) const
{
	assert (pass >= 0);
	assert (pass < _nbr_passes);

	return (pass > 0);
}



bool	FilterResize::has_buf_dst (int pass) const
{
	assert (pass >= 0);
	assert (pass < _nbr_passes);

	return (pass < _nbr_passes - 1);
}



void	FilterResize::compute_req_src_tile_size (int &tw, int &th, int dw, int dh) const
{
	assert (_buffer_flag);
	assert (_nbr_passes > 0);
	assert (dw > 0);
	assert (dh > 0);

	Dir            cur_dir = Dir_V;
	tw = dw;
	th = dh;

	for (int pass = MAX_NBR_PASSES - 1; pass >= 0; --pass)
	{
		switch (_roadmap [pass])
		{
		case	PassType_RESIZE:
			th = Scaler::eval_lower_bound_of_src_tile_height (
				th,
				_dst_size [cur_dir],
				_win_size [cur_dir],
				*(_kernel_ptr_arr [cur_dir]),
				_kernel_scale [cur_dir],
				_src_size [cur_dir]
			);
			break;

		case	PassType_TRANSPOSE:
			std::swap (tw, th);
			cur_dir = (cur_dir == Dir_V) ? Dir_H : Dir_V;
			break;

		case	PassType_NONE:
			// Nothing
			break;

		default:
			assert (false);
			break;
		}
	}

	assert (cur_dir == Dir_V);
}



void	FilterResize::redirect_task_resize (avstp_TaskDispatcher *dispatcher_ptr, void *data_ptr)
{
	fstb::unused (dispatcher_ptr);

	TaskRszCell *  trc_ptr = reinterpret_cast <TaskRszCell *> (data_ptr);
	FilterResize * this_ptr = trc_ptr->_val._glob_data_ptr->_this_ptr;

	this_ptr->process_tile (*trc_ptr);
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
