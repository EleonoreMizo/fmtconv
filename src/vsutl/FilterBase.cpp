/*****************************************************************************

        FilterBase.cpp
        Author: Laurent de Soras, 2012

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

#include "fstb/fnc.h"
#include "vsutl/FilterBase.h"
#include "vsutl/fnc.h"

#include <algorithm>
#include <stdexcept>

#include <cassert>
#include <cstdio>



namespace vsutl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FilterBase::FilterBase (const ::VSAPI &vsapi, const char name_0 [], ::VSFilterMode filter_mode)
:	_vsapi (vsapi)
,	_filter_name (name_0)
,	_filter_mode (filter_mode)
{
	assert (name_0 != nullptr);
}



const std::string &	FilterBase::use_filter_name () const
{
	return _filter_name;
}



::VSFilterMode	FilterBase::get_filter_mode () const
{
	return _filter_mode;
}



void	FilterBase::throw_inval_arg (const char msg_0 []) const
{
	throw_generic (msg_0, ExceptionType_INVALID_ARGUMENT);
}



void	FilterBase::throw_rt_err (const char msg_0 []) const
{
	throw_generic (msg_0, ExceptionType_RUNTIME_ERROR);
}



void	FilterBase::throw_logic_err (const char msg_0 []) const
{
	throw_generic (msg_0, ExceptionType_LOGIC_ERROR);
}



bool	FilterBase::is_arg_defined (const ::VSMap &in, const char name_0 []) const
{
	assert (name_0 != 0);

	const int      nbr_elt = _vsapi.mapNumElements (&in, name_0);

	return (nbr_elt >= 0);
}



int	FilterBase::get_arg_int (const ::VSMap &in, ::VSMap &out, const char name_0 [], int def_val, int pos, bool *defined_ptr) const
{
	assert (name_0 != 0);

	const bool     defined_flag = is_arg_defined (in, name_0);
	if (defined_ptr != 0)
	{
		*defined_ptr = defined_flag;
	}

	if (defined_flag)
	{
		int            err = 0;
		clip_neg_arg_pos (pos, in, name_0);
		def_val = int (_vsapi.mapGetInt (&in, name_0, pos, &err));
		test_arg_err (out, name_0, err);
	}

	return (def_val);
}



double	FilterBase::get_arg_flt (const ::VSMap &in, ::VSMap &out, const char name_0 [], double def_val, int pos, bool *defined_ptr) const
{
	assert (name_0 != 0);

	const bool     defined_flag = is_arg_defined (in, name_0);
	if (defined_ptr != 0)
	{
		*defined_ptr = defined_flag;
	}

	if (defined_flag)
	{
		int            err = 0;
		clip_neg_arg_pos (pos, in, name_0);
		def_val = _vsapi.mapGetFloat (&in, name_0, pos, &err);
		test_arg_err (out, name_0, err);
	}

	return (def_val);
}



std::string	FilterBase::get_arg_str (const ::VSMap &in, ::VSMap &out, const char name_0 [], std::string def_val, int pos, bool *defined_ptr) const
{
	assert (name_0 != 0);

	const bool     defined_flag = is_arg_defined (in, name_0);
	if (defined_ptr != 0)
	{
		*defined_ptr = defined_flag;
	}

	if (defined_flag)
	{
		int            err = 0;
		clip_neg_arg_pos (pos, in, name_0);
		const char *   tmp_0_ptr = _vsapi.mapGetData (&in, name_0, pos, &err);
		test_arg_err (out, name_0, err);
		assert (tmp_0_ptr != 0);
		def_val = tmp_0_ptr;
	}

	return (def_val);
}



std::vector <int>	FilterBase::get_arg_vint (const ::VSMap &in, ::VSMap &out, const char name_0 [], const std::vector <int> &def_val, bool *defined_ptr) const
{
	assert (name_0 != 0);

	std::vector <int> vec;
	const int      nbr_elt = _vsapi.mapNumElements (&in, name_0);
	const bool     defined_flag = (nbr_elt >= 0);
	if (defined_ptr != 0)
	{
		*defined_ptr = defined_flag;
	}

	if (! defined_flag)
	{
		vec = def_val;
	}
	else
	{
		int            err = 0;
		for (int pos = 0; pos < nbr_elt; ++pos)
		{
			const int      elt = int (_vsapi.mapGetInt (&in, name_0, pos, &err));
			test_arg_err (out, name_0, err);
			vec.push_back (elt);
		}
	}

	return (vec);
}



std::vector <double>	FilterBase::get_arg_vflt (const ::VSMap &in, ::VSMap &out, const char name_0 [], const std::vector <double> &def_val, bool *defined_ptr) const
{
	assert (name_0 != 0);

	std::vector <double> vec;
	const int      nbr_elt = _vsapi.mapNumElements (&in, name_0);
	const bool     defined_flag = (nbr_elt >= 0);
	if (defined_ptr != 0)
	{
		*defined_ptr = defined_flag;
	}

	if (! defined_flag)
	{
		vec = def_val;
	}
	else
	{
		int            err = 0;
		for (int pos = 0; pos < nbr_elt; ++pos)
		{
			const double   elt = _vsapi.mapGetFloat (&in, name_0, pos, &err);
			test_arg_err (out, name_0, err);
			vec.push_back (elt);
		}
	}

	return (vec);
}



std::vector <std::string>	FilterBase::get_arg_vstr (const ::VSMap &in, ::VSMap &out, const char name_0 [], const std::vector <std::string> &def_val, bool *defined_ptr) const
{
	assert (name_0 != 0);

	std::vector <std::string>  vec;
	const int      nbr_elt = _vsapi.mapNumElements (&in, name_0);
	const bool     defined_flag = (nbr_elt >= 0);
	if (defined_ptr != 0)
	{
		*defined_ptr = defined_flag;
	}

	if (! defined_flag)
	{
		vec = def_val;
	}
	else
	{
		int            err = 0;
		for (int pos = 0; pos < nbr_elt; ++pos)
		{
			const char *   tmp_0_ptr = _vsapi.mapGetData (&in, name_0, pos, &err);
			test_arg_err (out, name_0, err);
			vec.push_back (tmp_0_ptr);
		}
	}

	return (vec);
}



char	FilterBase::_filter_error_msg_0 [_max_error_buf_len] = "";



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// This is a simple wrapper around ::VSAPI::queryVideoFormat(), so the plug-in
// emits and error message instead of crashing when inconsistent arguments
// are passed to the function.
bool	FilterBase::register_format (::VSVideoFormat &fmt, int color_family, int sample_type, int bits_per_sample, int sub_sampling_w, int sub_sampling_h, ::VSCore &core) const
{
	// Copy of the beginning of VSCore::registerFormat()
	if (   sub_sampling_h < 0 || sub_sampling_w < 0
	    || sub_sampling_h > 4 || sub_sampling_w > 4)
	{
		throw_rt_err ("Nonsense format registration");
	}

	if (sample_type < 0 || sample_type > 1)
	{
		throw_rt_err ("Invalid sample type");
	}

	if (is_vs_rgb (color_family) && (sub_sampling_h != 0 || sub_sampling_w != 0))
	{
		throw_rt_err ("We do not like subsampled rgb around here");
	}

	if (sample_type == ::stFloat && (   bits_per_sample != 16
	                                 && bits_per_sample != 32))
	{
		throw_rt_err ("Only floating point formats with 16 or 32 bit precision are allowed");
	}

	if (bits_per_sample < 8 || bits_per_sample > 32)
	{
		throw_rt_err ("Only formats with 8-32 bits per sample are allowed");
	}

	const auto        qvf_ret = _vsapi.queryVideoFormat (
		&fmt,
		color_family,
		sample_type,
		bits_per_sample,
		sub_sampling_w,
		sub_sampling_h,
		&core
	);

	return (qvf_ret != 0);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void  FilterBase::clip_neg_arg_pos (int &pos, const ::VSMap &in, const char name_0 []) const
{
	assert (name_0 != 0);

	if (pos < 0)
	{
		pos = -pos;
		const int      nbr_elt = _vsapi.mapNumElements (&in, name_0);
		pos = std::max (std::min (pos, nbr_elt - 1), 0);
	}
}



void	FilterBase::test_arg_err (::VSMap &out, const char name_0 [], int err) const
{
	assert (name_0 != 0);

	if (err != 0)
	{
		fstb::snprintf4all (
			_filter_error_msg_internal_0,
			_max_error_buf_len,
			"%s: invalid argument %s, error code %d.",
			_filter_name.c_str (), name_0, err
		);

		_vsapi.mapSetError (&out, _filter_error_msg_internal_0);

		throw std::invalid_argument (_filter_error_msg_internal_0);
	}
}



void	FilterBase::throw_generic (const char msg_0 [], ExceptionType e) const
{
	assert (msg_0 != 0);
	assert (e >= 0);
	assert (e < ExceptionType_NBR_ELT);

	fstb::snprintf4all (
		_filter_error_msg_internal_0,
		_max_error_buf_len,
		"%s: %s",
		_filter_name.c_str (),
		msg_0
	);

	switch (e)
	{
	case	ExceptionType_INVALID_ARGUMENT:
		throw std::invalid_argument (_filter_error_msg_internal_0);
		break;
	case	ExceptionType_RUNTIME_ERROR:
		throw std::runtime_error (_filter_error_msg_internal_0);
		break;
	case	ExceptionType_LOGIC_ERROR:
	default:
		throw std::logic_error (_filter_error_msg_internal_0);
		break;
	}
}



char	FilterBase::_filter_error_msg_internal_0 [_max_error_buf_len] = "";



}	// namespace vsutl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
