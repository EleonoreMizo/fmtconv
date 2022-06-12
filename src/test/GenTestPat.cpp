/*****************************************************************************

        GenTestPat.cpp
        Author: Laurent de Soras, 2022

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
#include "test/GenTestPat.h"

#include <array>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstdio>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	GenTestPat::generate_patterns ()
{
	int            ret_val = 0;

	printf ("Generating patterns...\n"); fflush (stdout);

	constexpr int  nbr_comp       = 3;
	constexpr int  res            = 16; // Bits
	typedef std::array <uint16_t, nbr_comp> Pix;

	constexpr int  nbr_shades     = 8; // >= 2
	constexpr int  nbr_tiles_tot  = fstb::ipowpc <3> (nbr_shades);
	constexpr int  tile_size      = 32; // Pixels
	const int      nbr_tiles_side = fstb::ceil_int (sqrt (double (nbr_tiles_tot)));
	const int      pic_w          = tile_size * nbr_tiles_side;
	const int      pic_h          = pic_w;
	const int      stride         = pic_w;
	const int      pic_area       = pic_w * pic_h;

	std::vector <Pix> pic_data (pic_area);
	for (int y = 0; y < pic_h; ++y)
	{
		const auto     y_base    = y * stride;
		const auto     tile_y    = y / tile_size;
		const auto     tile_base = tile_y * nbr_tiles_side;
		for (int x = 0; x < pic_w; ++x)
		{
			const auto     tile_x = x / tile_size;
			const auto     tile_idx = tile_base + tile_x;
			Pix &          p = pic_data [y_base + x];
			for (int c = 0; c < nbr_comp; ++c)
			{
				const auto     shade =
					(tile_idx / fstb::ipowp (nbr_shades, c)) % nbr_shades;
				auto           val_f = double (shade) / double (nbr_shades - 1);
				val_f = (0.75 * val_f + 0.25) * val_f;
				p [c] = uint16_t (fstb::round_int (val_f * UINT16_MAX));
			}
		}
	}

	constexpr char endian_c =
#if fstb_ENDIAN == fstb_ENDIAN_BIG
		'M';
#else // fstb_ENDIAN
		'I';
#endif // fstb_ENDIAN
	enum class Tag : uint16_t
	{
		image_width          = 0x100,
		image_length         = 0x101,
		bits_per_sample      = 0x102,
		photometric_interpr  = 0x106,
		strip_offsets        = 0x111,
		samples_per_pixel    = 0x115,
		strip_byte_counts    = 0x117,
		planar_configuration = 0x11C
	};
	enum class Type : uint16_t
	{
		b = 1, // Byte
		a = 2, // ASCII
		s = 3, // Short
		l = 4, // Long
		r = 5  // Rational
	};
	struct Field
	{
		Tag            _tag;
		Type           _type;
		uint32_t       _count;
		uint32_t       _val;
	};
	static_assert (sizeof (Field) == 12, "");
	enum Entry
	{
		Entry_IMAGE_WIDTH = 0,
		Entry_IMAGE_LENGTH,
		Entry_BITS_PER_SAMPLE,
		Entry_PHOTOMETRIC_INTERPR,
		Entry_STRIP_OFFSETS,
		Entry_SAMPLES_PER_PIXEL,
		Entry_STRIP_BYTE_COUNTS,
		Entry_PLANAR_CONFIGURATION,

		Entry_NBR_ELT
	};
	struct TiffHeader
	{
		char           _endian_0 [2] = { endian_c, endian_c };
		uint16_t       _tiff_id      = 42;
		uint32_t       _ifd_ofs      = offsetof (TiffHeader, _nbr_fields);

		uint16_t       _pad_ifd      = 0;

		// Image File Directory
		uint16_t       _nbr_fields   = Entry_NBR_ELT;
		Field          _filed_arr [Entry_NBR_ELT] =
		{
			{ Tag::image_width         , Type::s, 1, 0 },
			{ Tag::image_length        , Type::s, 1, 0 },
			{ Tag::bits_per_sample     , Type::s, 0, 0 },
			{ Tag::photometric_interpr , Type::s, 1, pack_s ((nbr_comp == 3) ? 2 : 1) },
			{ Tag::strip_offsets       , Type::l, 1, 0 },
			{ Tag::samples_per_pixel   , Type::s, 1, 0 },
			{ Tag::strip_byte_counts   , Type::l, 1, 0 },
			{ Tag::planar_configuration, Type::s, 1, pack_s (1) }
		};
		uint32_t       _next_ifd_ofs = 0;

		uint16_t       _spp_arr [4] = { res, res, res, res };

		// Data
	} tiff_header;

	tiff_header._filed_arr [Entry_IMAGE_WIDTH ]._val = pack_s (uint16_t (pic_w));
	tiff_header._filed_arr [Entry_IMAGE_LENGTH]._val = pack_s (uint16_t (pic_h));
	tiff_header._filed_arr [Entry_BITS_PER_SAMPLE]._count = nbr_comp;
	tiff_header._filed_arr [Entry_BITS_PER_SAMPLE]._val   =
		(nbr_comp > 2) ? offsetof (TiffHeader, _spp_arr) : pack_s (res, res);
	tiff_header._filed_arr [Entry_STRIP_OFFSETS]._val =
		uint32_t (sizeof (TiffHeader));
	tiff_header._filed_arr [Entry_SAMPLES_PER_PIXEL]._val =
		pack_s (uint16_t (nbr_comp));
	tiff_header._filed_arr [Entry_STRIP_BYTE_COUNTS]._val =
		uint32_t (sizeof (Pix)) * pic_w * pic_h;

	char           txt_0 [255+1];
	fstb::snprintf4all (txt_0, sizeof (txt_0),
		"rgb48_lin_%dx%d.tiff", pic_w, pic_h
	);

	auto           f_ptr = fopen (txt_0, "wb");
	fwrite (&tiff_header, sizeof (tiff_header), 1, f_ptr);
	fwrite (pic_data.data (), sizeof (pic_data [0]), pic_data.size (), f_ptr);
	fclose (f_ptr);

	// "C:\Program Files\FFmpeg\bin\ffmpeg.exe" -i "rgb48_lin_736x736.tiff" -c:v libx264 -qp 0 -color_range 2 -color_primaries 9 -color_trc 8 "rgb48.mp4"

	printf ("Done.\n"); fflush (stdout);

	return ret_val;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



uint32_t	GenTestPat::pack_s (uint16_t v0, uint16_t v1) noexcept
{
#if fstb_ENDIAN == fstb_ENDIAN_BIG
	return (uint32_t (v0) << 16) + uint32_t (v1);
#else
	return (uint32_t (v1) << 16) + uint32_t (v0);
#endif
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
