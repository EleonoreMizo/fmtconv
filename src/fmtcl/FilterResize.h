/*****************************************************************************

        FilterResize.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_FilterResize_HEADER_INCLUDED)
#define	fmtcl_FilterResize_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "conc/ObjPool.h"
#include "fmtcl/BitBltConv.h"
#include "fmtcl/SplFmt.h"
#include "fmtcl/ResizeData.h"
#include "fmtcl/ResizeDataFactory.h"
#include "fmtcl/Scaler.h"
#include "avstp.h"
#include "AvstpWrapper.h"

#include <vector>
#include <memory>

#include <cstddef>
#include <cstdint>



namespace fmtcl
{



class ContFirInterface;
class ResampleSpecPlane;

class FilterResize
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	enum Dir
	{
		Dir_H = 0,
		Dir_V,

		Dir_NBR_ELT
	};

	typedef	FilterResize	ThisType;

	explicit       FilterResize (const ResampleSpecPlane &spec, ContFirInterface &kernel_fnc_h, ContFirInterface &kernel_fnc_v, bool norm_flag, double norm_val_h, double norm_val_v, double gain, SplFmt src_type, int src_res, SplFmt dst_type, int dst_res, bool int_flag, bool sse2_flag, bool avx2_flag);
	virtual        ~FilterResize () {}

	void           process_plane (uint8_t *dst_ptr, const uint8_t *src_ptr, ptrdiff_t stride_dst, ptrdiff_t stride_src, bool chroma_flag);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum PassType
	{
		PassType_NONE = 0,
		PassType_RESIZE,
		PassType_TRANSPOSE,

		PassType_NBR_ELT
	};

	static const int  MAX_NBR_PASSES = 4;                 // 2 * (transpose + resize)
	static const int  BUF_SIZE       = 65536;             // Number of pixels (float or int16_t)
	static const int  MAX_BUF_SIZE   = BUF_SIZE * 1024;   // Number of pixels (float or int16_t)

	class TaskRszGlobal
	{
	public:
		FilterResize * _this_ptr;
		uint8_t *      _dst_ptr;
		const uint8_t *
		               _src_ptr;
		int            _dst_bpp;        // Pointed bytes per pixel (1 for stack16)
		int            _src_bpp;        // Pointed bytes per pixel (1 for stack16)
		ptrdiff_t      _stride_dst;     // Bytes
		ptrdiff_t      _stride_src;     // Bytes
		ptrdiff_t      _offset_crop;    // Bytes
		ptrdiff_t      _stride_dst_pix; // Pixels
		ptrdiff_t      _stride_src_pix; // Pixels
	};

	class TaskRsz
	{
	public:
		const TaskRszGlobal *
		               _glob_data_ptr;
		int            _dst_beg [Dir_NBR_ELT];
		int            _work_dst [Dir_NBR_ELT];
		int            _src_beg [Dir_NBR_ELT];
		int            _src_end [Dir_NBR_ELT];
	};

	typedef	conc::LockFreeCell <TaskRsz>	TaskRszCell;

	void           process_plane_bypass (uint8_t *dst_ptr, const uint8_t *src_ptr, ptrdiff_t stride_dst, ptrdiff_t stride_src, bool chroma_flag);
	void           process_plane_normal (uint8_t *dst_ptr, const uint8_t *src_ptr, ptrdiff_t stride_dst, ptrdiff_t stride_src);
	void           process_tile (TaskRszCell &tr_cell);
	void           process_tile_resize (const TaskRsz &tr, const TaskRszGlobal& trg, ResizeData &rd, ptrdiff_t stride_buf [2], const int pass, Dir &cur_dir, int &cur_buf, int cur_size [Dir_NBR_ELT]);

	template <typename T, SplFmt BUFT>
	void           process_tile_transpose (const TaskRsz &tr, const TaskRszGlobal& trg, ResizeData &rd, ptrdiff_t stride_buf [2], const int pass, Dir &cur_dir, int &cur_buf, int cur_size [Dir_NBR_ELT]);

	template <typename T>
	void           transpose (T *dst_ptr, const T *src_ptr, int w, int h, ptrdiff_t stride_dst, ptrdiff_t stride_src);

	template <typename T>
	void           transpose_cpp (T *dst_ptr, const T *src_ptr, int w, int h, ptrdiff_t stride_dst, ptrdiff_t stride_src);

#if (fstb_ARCHI == fstb_ARCHI_X86)
	void           transpose_sse2 (float *dst_ptr, const float *src_ptr, int w, int h, ptrdiff_t stride_dst, ptrdiff_t stride_src);
	void           transpose_sse2 (uint16_t *dst_ptr, const uint16_t *src_ptr, int w, int h, ptrdiff_t stride_dst, ptrdiff_t stride_src);
#endif

	bool           is_kernel_neutral (Dir di) const;

	inline bool    has_buf_src (int pass) const;
	inline bool    has_buf_dst (int pass) const;
	void           compute_req_src_tile_size (int &tw, int &th, int dw, int dh) const;

	static void    redirect_task_resize (avstp_TaskDispatcher *dispatcher_ptr, void *data_ptr);

	AvstpWrapper & _avstp;
	conc::CellPool <TaskRsz>
	               _task_rsz_pool;

	int            _src_size [Dir_NBR_ELT];
	int            _dst_size [Dir_NBR_ELT];
	double         _win_pos [Dir_NBR_ELT];
	double         _win_size [Dir_NBR_ELT];
	double         _kernel_scale [Dir_NBR_ELT];
	bool           _kernel_force_flag [Dir_NBR_ELT];
	ContFirInterface *
	               _kernel_ptr_arr [Dir_NBR_ELT];
	bool				_norm_flag;
	double         _norm_val [Dir_NBR_ELT];
	double         _center_pos_src [Dir_NBR_ELT];
	double         _center_pos_dst [Dir_NBR_ELT];
	double         _gain;            // Scale adjustment for bitdepth conversions
	double         _add_cst;         // Constant added to U and V planes when converting between floating-point (range -0.5 +0.5) and integer formats (range 0 max)
	SplFmt         _src_type;
	int            _src_res;
	SplFmt         _dst_type;
	int            _dst_res;
	Dir            _bd_chg_dir;      // The resizer in charge of the bitdepth conversion.
	bool           _int_flag;        // Use 16-bit int as temporary data instead of float, if possible
	bool           _sse2_flag;
	bool           _avx2_flag;

	conc::ObjPool <ResizeData>
						_pool;
	std::unique_ptr <ResizeDataFactory>
	               _factory_uptr;

	int            _crop_pos [Dir_NBR_ELT];
	int            _crop_size [Dir_NBR_ELT];

	std::unique_ptr <Scaler>         // 0 if bypassed
	               _scaler_uptr [Dir_NBR_ELT];
	BitBltConv     _blitter;

	bool           _resize_flag [Dir_NBR_ELT];
	PassType       _roadmap [MAX_NBR_PASSES];
	int            _tile_size_dst [Dir_NBR_ELT];
	int            _nbr_passes;      // 0 = bypass
	int            _buf_size;        // In pixels
	bool           _buffer_flag;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               FilterResize ()                               = delete;
	               FilterResize (const FilterResize &other)      = delete;
	FilterResize & operator = (const FilterResize &other)        = delete;
	bool           operator == (const FilterResize &other) const = delete;
	bool           operator != (const FilterResize &other) const = delete;

};	// class FilterResize



}	// namespace fmtcl



//#include "fmtcl/FilterResize.hpp"



#endif	// fmtcl_FilterResize_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
