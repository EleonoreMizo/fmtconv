/*****************************************************************************

        fnc.hpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_fnc_CODEHEADER_INCLUDED)
#define fmtcl_fnc_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "fstb/fnc.h"

#include <type_traits>



namespace fmtcl
{



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <typename T>
inline T	conv_str_to_n (const std::string &str, std::size_t &pos_end)
{
	// Issues a compilation error if the function is instanciated.
	// static_assert condition should be dependent on T.
	static_assert (
		(std::is_void <T>::value && ! std::is_void <T>::value),
		"Only specializations of this function are allowed."
	);
	fstb::unused (str, pos_end);
	return T {};
}

template <>
inline double	conv_str_to_n <double> (const std::string &str, std::size_t &pos_end)
{
	try
	{
		return stod (str, &pos_end);
	}
	catch (...)
	{
		pos_end = 0;
	}
	return 0;
}

template <>
inline int	conv_str_to_n <int> (const std::string &str, std::size_t &pos_end)
{
	try
	{
		return stoi (str, &pos_end, 10);
	}
	catch (...)
	{
		pos_end = 0;
	}
	return 0;
}

template <>
inline bool	conv_str_to_n <bool> (const std::string &str, std::size_t &pos_end)
{
	auto           s2 = str;
	fstb::conv_to_lower_case (s2);
	pos_end = 0;
	while (! s2.empty () && std::isspace (s2.front ()) != 0)
	{
		s2.erase (0, 1);
		++ pos_end;
	}
	if (s2.substr (0, 4) == "true")
	{
		pos_end += 4;
		return true;
	}
	else if (s2.substr (0, 5) == "false")
	{
		pos_end += 5;
		return false;
	}

	return (stoi (str, &pos_end, 10) != 0);
}

template <>
inline std::string	conv_str_to_n <std::string> (const std::string &str, std::size_t &pos_end)
{
	auto           s2 = str;
	while (! s2.empty () && std::isspace (s2.front ()) != 0)
	{
		s2.erase (0, 1);
		++ pos_end;
	}

	std::size_t    pos = 0;
	while (pos < s2.size ())
	{
		if (std::isspace (s2 [pos]))
		{
			break;
		}
		++ pos;
	}
	pos_end += pos;

	return s2;
}



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <typename T>
std::vector <T>   conv_str_to_arr (std::string str)
{
	std::vector<T> coef_arr;
	bool           cont_flag = true;
	do
	{
		std::size_t    pos_end = 0;
		const T        val     = conv_str_to_n <T> (str, pos_end);
		if (pos_end == 0)
		{
			cont_flag = false;
		}
		else
		{
			coef_arr.push_back (val);
			str.erase (0, pos_end);
		}
	}
	while (cont_flag);

	return coef_arr;
}



// Gets the Nth element of an array, clips the position with the array size,
// and use the provided default value if the array is empty.
template <typename T>
T	get_arr_elt (const std::vector <T> &v, int pos, const T &def) noexcept
{
	assert (pos >= 0);

	const int      sz = int (v.size ());
	if (sz == 0)
	{
		return def;
	}

	pos = std::min (pos, sz - 1);

	return v [pos];
}



}  // namespace fmtcl



#endif   // fmtcl_fnc_CODEHEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
