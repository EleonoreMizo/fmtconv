/*****************************************************************************

        FmtAvs.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/fnc.h"
#include "fmtcavs/FmtAvs.h"
#include "avisynth.h"

#include <stdexcept>

#include <cassert>
#include <cctype>



namespace fmtcavs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FmtAvs::FmtAvs (std::string fmt_str)
{
	if (conv_from_str (fmt_str) != 0)
	{
		throw std::runtime_error ("Bad video format string");
	}
}



FmtAvs::FmtAvs (const VideoInfo &vi) noexcept
{
	conv_from_vi (vi);
}



void	FmtAvs::invalidate () noexcept
{
	_bitdepth    = -1;
	_col_fam     = fmtcl::ColorFamily_INVALID;
	_planar_flag = false;
	_alpha_flag  = false;
	_subspl_h    = -1;
	_subspl_v    = -1;
}



// Converts from an Avisynth colorspace
// http://avisynth.nl/index.php/Convert
// Returns 0 if everything is OK.
int	FmtAvs::conv_from_str (std::string fmt_str)
{
	invalidate ();

	fmt_str = remove_outer_spaces (fmt_str);
	fstb::conv_to_lower_case (fmt_str);

	if (is_eq_leftstr_and_eat (fmt_str, "rgb"))
	{
		_col_fam = fmtcl::ColorFamily_RGB;
		_subspl_h = 0; // Required for consistency
		_subspl_v = 0;
		if (fmt_str.empty ())
		{
			return -1; // Nothing specified after "RGB"
		}

		if (fmt_str.front () == 'a')
		{
			_planar_flag = true;
			_alpha_flag  = true;
			fmt_str.erase (0, 1);
			if (fmt_str.empty ())
			{
				return -1; // We need the bitdepth
			}
		}

		// Planar
		if (check_planar_bits_and_eat (fmt_str, _bitdepth))
		{
			if (_bitdepth < 0)
			{
				return -1; // Ill-formed string or invalid bitdepth
			}
		}

		// Interleaved
		else
		{
			if (_planar_flag)
			{
				return -1; // Interleaved format is always RGBxx, not RGBAxx
			}
			size_t         pos      = 0;
			const int      val_bits = std::stoi (fmt_str, &pos, 10);
			if (fmt_str.length () != pos)
			{
				return -1; // Garbage after RGBxx
			}
			switch (val_bits)
			{
			case 24: _bitdepth =  8; _alpha_flag = false; break;
			case 32: _bitdepth =  8; _alpha_flag = true;  break;
			case 48: _bitdepth = 16; _alpha_flag = false; break;
			case 64: _bitdepth = 16; _alpha_flag = true;  break;
			default: return -1; // Invalid bitdepth/alpha combination
			}
		}
	}

	else if (is_eq_leftstr_and_eat (fmt_str, "yuy2"))
	{
		_col_fam  = fmtcl::ColorFamily_YUV;
		_bitdepth = 8;
		_subspl_h = 1;
		_subspl_v = 0;

		if (! fmt_str.empty ())
		{
			return -1; // Garbage after YUY2
		}
	}

	else if (is_eq_leftstr_and_eat (fmt_str, "yuv"))
	{
		_col_fam     = fmtcl::ColorFamily_YUV;
		_planar_flag = true;

		if (fmt_str.front () == 'a')
		{
			_alpha_flag = true;
			fmt_str.erase (0, 1);
			if (fmt_str.empty ())
			{
				return -1; // We need chroma subsampling + planar bitdepth
			}
		}

		size_t         pos = 0;
		const int      css = std::stoi (fmt_str, &pos, 10);
		fmt_str.erase (0, pos);
		switch (css)
		{
		case 420: _subspl_h = 1; _subspl_v = 1; break;
		case 422: _subspl_h = 1; _subspl_v = 0; break;
		case 444: _subspl_h = 0; _subspl_v = 0; break;
		default: return -1; // Invalid chroma subsampling
		}

		if (! check_planar_bits_and_eat (fmt_str, _bitdepth))
		{
			return -1;
		}
		if (_bitdepth < 0)
		{
			return -1; // Ill-formed string or invalid bitdepth
		}
	}

	else if (is_eq_leftstr_and_eat (fmt_str, "yv"))
	{
		_col_fam     = fmtcl::ColorFamily_YUV;
		_planar_flag = true;
		_bitdepth    = 8;
		size_t         pos      = 0;
		const int      val_bits = std::stoi (fmt_str, &pos, 10);
		if (fmt_str.length () != pos)
		{
			return -1; // Garbage after YVxx
		}
		switch (val_bits)
		{
		case 12:  _subspl_h = 1; _subspl_v = 1; break;
		case 16:  _subspl_h = 1; _subspl_v = 0; break;
		case 24:  _subspl_h = 0; _subspl_v = 0; break;
		case 411: _subspl_h = 2; _subspl_v = 0; break;
		default: return -1; // Invalid chroma subsampling
		}
	}

	else if (is_eq_leftstr_and_eat (fmt_str, "y"))
	{
		_col_fam     = fmtcl::ColorFamily_GRAY;
		_planar_flag = true;
		_subspl_h    = 0; // Required for consistency
		_subspl_v    = 0;
		_bitdepth    = check_bits_and_eat (fmt_str, false);
		if (_bitdepth < 0)
		{
			return -1; // Ill-formed or unsupported bitdepth
		}
	}

	return 0;
}



// Assumes pixel_type is valid
void	FmtAvs::conv_from_vi (const VideoInfo &vi)
{
	invalidate ();

	_bitdepth    = vi.BitsPerComponent ();
	_planar_flag = vi.IsPlanar ();

	if (vi.IsRGB ())
	{
		_col_fam    = fmtcl::ColorFamily_RGB;
		_subspl_h   = 0; // Required for consistency
		_subspl_v   = 0;
		_alpha_flag = (vi.IsRGB32 () || vi.IsRGB64 ());
	}
	else if (vi.IsY ())
	{
		_col_fam    = fmtcl::ColorFamily_GRAY;
		_subspl_h   = 0; // Required for consistency
		_subspl_v   = 0;
	}
	else
	{
		assert (vi.IsYUV () || vi.IsYUVA ());
		_col_fam    = fmtcl::ColorFamily_YUV;
		_alpha_flag = vi.IsYUVA ();
		_subspl_h   = ((vi.pixel_type >> VideoInfo::CS_Shift_Sub_Width ) + 1) & 3;
		_subspl_v   = ((vi.pixel_type >> VideoInfo::CS_Shift_Sub_Height) + 1) & 3;
	}
}



// Returns 0 if conversion is OK.
int	FmtAvs::conv_to_vi (VideoInfo &vi)
{
	assert (is_valid ());

	int            pixel_type = 0;

	switch (_bitdepth)
	{
	case 8:  pixel_type |= VideoInfo::CS_Sample_Bits_8;  break;
	case 10: pixel_type |= VideoInfo::CS_Sample_Bits_10; break;
	case 12: pixel_type |= VideoInfo::CS_Sample_Bits_12; break;
	case 14: pixel_type |= VideoInfo::CS_Sample_Bits_14; break;
	case 16: pixel_type |= VideoInfo::CS_Sample_Bits_16; break;
	case 32: pixel_type |= VideoInfo::CS_Sample_Bits_32; break;
	default: return -1; // Bitdepth not supported
	}

	if (_col_fam == fmtcl::ColorFamily_GRAY)
	{
		// Chroma subsampling is ignored
		if (_alpha_flag)
		{
			return -1;
		}
		pixel_type |= VideoInfo::CS_GENERIC_Y;
	}

	else if (_col_fam == fmtcl::ColorFamily_RGB)
	{
		// Chroma subsampling is ignored
		pixel_type |= VideoInfo::CS_BGR;
		pixel_type |= (_alpha_flag)
			? VideoInfo::CS_RGBA_TYPE
			: VideoInfo::CS_RGB_TYPE;

		if (_planar_flag)
		{
			pixel_type |= VideoInfo::CS_PLANAR;
		}
		else
		{
			if (_bitdepth != 8 && _bitdepth != 16)
			{
				return -1;
			}
			pixel_type |= VideoInfo::CS_INTERLEAVED;
		}
	}

	else if (_col_fam == fmtcl::ColorFamily_YUV)
	{
		if (_planar_flag)
		{
			pixel_type |= VideoInfo::CS_YUY2;
			pixel_type |= VideoInfo::CS_PLANAR;
			pixel_type |= VideoInfo::CS_VPlaneFirst;
			pixel_type |= (_alpha_flag) ? VideoInfo::CS_YUVA : VideoInfo::CS_YUV;

			switch ((_subspl_v << 4) | _subspl_h)
			{
			case 0x00:
				pixel_type |= VideoInfo::CS_Sub_Height_1 | VideoInfo::CS_Sub_Width_1;
				break;
			case 0x01:
				pixel_type |= VideoInfo::CS_Sub_Height_1 | VideoInfo::CS_Sub_Width_2;
				break;
			case 0x11:
				pixel_type |= VideoInfo::CS_Sub_Height_2 | VideoInfo::CS_Sub_Width_2;
				break;
			case 0x02:
				pixel_type |= VideoInfo::CS_Sub_Height_1 | VideoInfo::CS_Sub_Width_4;
				if (_alpha_flag || _bitdepth != 8)
				{
					return -1; // Only YV411 in 4:1:1
				}
				break;
			/**** To do: should we allow 4:1:0 (YUV9)? It is in the API but it 
				seems Avisynth filter don't use it.
				http://avisynth.nl/index.php/Avisynthplus_color_formats
			***/
			default:
				return -1; // Invalid chroma subsampling
			}
		}
		else
		{
			// Only YUY2 allowed with interleaved formats
			if (_bitdepth != 8 || _alpha_flag || _subspl_h != 1 || _subspl_v != 0)
			{
				return -1;
			}
			pixel_type = VideoInfo::CS_YUY2;
		}
	}

	else
	{
		return -1;
	}

	vi.pixel_type = pixel_type;

	return 0;
}



bool	FmtAvs::is_valid () const noexcept
{
	return (
		   _bitdepth >  0 && _col_fam  >= 0
		&& _subspl_h >= 0 && _subspl_v >= 0);
}



void	FmtAvs::set_bitdepth (int bitdepth) noexcept
{
	assert (is_bitdepth_valid (bitdepth));

	_bitdepth = bitdepth;
}



int	FmtAvs::get_bitdepth () const noexcept
{
	assert (is_valid ());

	return _bitdepth;
}



bool	FmtAvs::is_float () const noexcept
{
	assert (is_valid ());

	return (_bitdepth >= 32);
}



void	FmtAvs::set_col_fam (fmtcl::ColorFamily col_fam) noexcept
{
	assert (col_fam >= 0);
	assert (col_fam < fmtcl::ColorFamily_NBR_ELT);
	assert (col_fam != fmtcl::ColorFamily_YCGCO);

	_col_fam = col_fam;
}



fmtcl::ColorFamily	FmtAvs::get_col_fam () const noexcept
{
	assert (is_valid ());

	return _col_fam;
}



bool	FmtAvs::is_planar () const noexcept
{
	assert (is_valid ());

	return _planar_flag;
}



bool	FmtAvs::has_alpha () const noexcept
{
	assert (is_valid ());

	return _alpha_flag;
}



int	FmtAvs::get_subspl_h () const noexcept
{
	assert (is_valid ());

	return _subspl_h;
}



int	FmtAvs::get_subspl_v () const noexcept
{
	assert (is_valid ());

	return _subspl_v;
}



int	FmtAvs::get_nbr_comp_non_alpha () const noexcept
{
	assert (is_valid ());

	return (_col_fam == fmtcl::ColorFamily_GRAY) ? 1 : 3;
}



bool	FmtAvs::is_bitdepth_valid (int bitdepth) noexcept
{
	return (
		   bitdepth ==  8
		|| bitdepth == 10
		|| bitdepth == 12
		|| bitdepth == 14
		|| bitdepth == 16
		|| bitdepth == 32
	);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



std::string	FmtAvs::remove_outer_spaces (std::string str)
{
	while (! str.empty () && std::isspace (str.back ()) != 0)
	{
		str.pop_back ();
	}
	while (! str.empty () && std::isspace (str.front ()) != 0)
	{
		str.erase (0, 1);
	}

	return str;
}



bool	FmtAvs::is_eq_leftstr_and_eat (std::string &str, std::string stest)
{
	const auto     len = stest.length ();
	if (str.substr (0, len) == stest)
	{
		str.erase (0, len);
		return true;
	}

	return false;
}



// str must be lower-case
// If the string is ill-formed, returns true and res < 0
// As the pxx value should be last, the string is considered ill-formed
// if there are characters after the resolution.
bool	FmtAvs::check_planar_bits_and_eat (std::string &str, int &res)
{
	if (! str.empty () && str.front () == 'p')
	{
		str.erase (0, 1);
		if (str.empty ())
		{
			res = -1;
			return true;
		}
		res = check_bits_and_eat (str, true);
		return true;
	}

	return false;
}



int	FmtAvs::check_bits_and_eat (std::string &str, bool allow_s_flag)
{
	int            res = -1;

	if (str.empty ())
	{
		return -1;
	}

	if (str [0] == 's')
	{
		if (! allow_s_flag)
		{
			return -1;
		}
		res = 32;
		str.erase (0, 1);
	}
	else
	{
		size_t         pos = 0;
		res = std::stoi (str, &pos, 10);
		switch (res)
		{
		case 8:
		case 10:
		case 12:
		case 14:
		case 16:
		case 32:
			break;
		default:
			return -1;
		}
		str.erase (0, pos);
	}

	if (! str.empty ())
	{
		return -1;
	}

	return res;
}



}  // namespace fmtcavs



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
