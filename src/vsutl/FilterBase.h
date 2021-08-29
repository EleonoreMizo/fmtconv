/*****************************************************************************

        FilterBase.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_FilterBase_HEADER_INCLUDED)
#define	vsutl_FilterBase_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vswrap.h"

#include <string>
#include <vector>



namespace vsutl
{



class FilterBase
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const size_t
	               _max_error_buf_len = 4096;

	explicit       FilterBase (const ::VSAPI &vsapi, const char name_0 [], ::VSFilterMode filter_mode, int /* ::NodeFlags */ flags);
	virtual        ~FilterBase () = default;

	const std::string &
	               use_filter_name () const;
	::VSFilterMode get_filter_mode () const;
	int            get_filter_flags () const;

	bool           is_arg_defined (const ::VSMap &in, const char name_0 []) const;

	int            get_arg_int (const ::VSMap &in, ::VSMap &out, const char name_0 [], int def_val, int pos = 0, bool *defined_ptr = 0) const;
	double         get_arg_flt (const ::VSMap &in, ::VSMap &out, const char name_0 [], double def_val, int pos = 0, bool *defined_ptr = 0) const;
	std::string    get_arg_str (const ::VSMap &in, ::VSMap &out, const char name_0 [], std::string def_val, int pos = 0, bool *defined_ptr = 0) const;

	std::vector <int>
	               get_arg_vint (const ::VSMap &in, ::VSMap &out, const char name_0 [], const std::vector <int> &def_val, bool *defined_ptr = 0) const;
	std::vector <double>
	               get_arg_vflt (const ::VSMap &in, ::VSMap &out, const char name_0 [], const std::vector <double> &def_val, bool *defined_ptr = 0) const;
	std::vector <std::string>
	               get_arg_vstr (const ::VSMap &in, ::VSMap &out, const char name_0 [], const std::vector <std::string> &def_val, bool *defined_ptr = 0) const;

	void           throw_inval_arg (const char msg_0 []) const;
	void           throw_rt_err (const char msg_0 []) const;
	void           throw_logic_err (const char msg_0 []) const;

	// Override this
	virtual void   init_filter (::VSMap &in, ::VSMap &out, ::VSNode &node, ::VSCore &core) = 0;
	virtual const ::VSFrameRef *
	               get_frame (int n, int activation_reason, void * &frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core) = 0;

	static char    _filter_error_msg_0 [_max_error_buf_len];



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	const VSFormat *
	               register_format (int color_family, int sample_type, int bits_per_sample, int sub_sampling_w, int sub_sampling_h, ::VSCore &core) const;

	const ::VSAPI& _vsapi;
	const std::string
	               _filter_name;
	const ::VSFilterMode
	               _filter_mode;
	const int      _filter_flags;		// Combination of ::NodeFlags



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum ExceptionType
	{
		ExceptionType_INVALID_ARGUMENT = 0,
		ExceptionType_RUNTIME_ERROR,
		ExceptionType_LOGIC_ERROR,

		ExceptionType_NBR_ELT
	};

	void           clip_neg_arg_pos (int &pos, const ::VSMap &in, const char name_0 []) const;
	void           test_arg_err (::VSMap &out, const char name_0 [], int err) const;
	void           throw_generic (const char msg_0 [], ExceptionType e) const;

	static char    _filter_error_msg_internal_0 [_max_error_buf_len];



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               FilterBase ()                               = delete;
	               FilterBase (const FilterBase &other)        = delete;
	FilterBase &   operator = (const FilterBase &other)        = delete;
	bool           operator == (const FilterBase &other) const = delete;
	bool           operator != (const FilterBase &other) const = delete;

};	// class FilterBase



}	// namespace vsutl



//#include "vsutl/FilterBase.hpp"



#endif	// vsutl_FilterBase_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
