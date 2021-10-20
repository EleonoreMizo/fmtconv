/*****************************************************************************

        fnc.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (avsutl_fnc_HEADER_INCLUDED)
#define avsutl_fnc_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "avsutl/TFlag.h"



class AVSValue;
struct VideoInfo;

namespace avsutl
{



TFlag set_tristate (const ::AVSValue &val);
bool set_default (TFlag tristate, bool def_flag);
bool has_alpha (const ::VideoInfo &vi);
int get_nbr_comp_non_alpha (const ::VideoInfo &vi);
bool is_rgb (const ::VideoInfo &vi);
bool is_full_range_default (const ::VideoInfo &vi);

template <class T>
void fill_block (void *ptr, T val, int stride, int w, int h);



}  // namespace avsutl



#include "avsutl/fnc.hpp"



#endif   // avsutl_fnc_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
