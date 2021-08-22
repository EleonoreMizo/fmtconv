/*****************************************************************************

        TestGammaY.cpp
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

#include "fmtcl/PlaneRO.h"
#include "fmtcl/GammaY.h"
#include "fstb/fnc.h"
#include "test/TestGammaY.h"

#include <iostream>
#include <type_traits>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>



template <typename T>
void	dump_pic (fmtcl::FrameRO <T> frame, int w, int h, int nbr_planes)
{
	assert (nbr_planes > 0);
	assert (nbr_planes <= frame.size ());

	for (int plane_idx = 0; plane_idx < nbr_planes; ++plane_idx)
	{
		auto &         plane = frame [plane_idx];
		if (nbr_planes > 1)
		{
			std::cout << "Plane " << plane_idx << ":\n";
		}

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const auto     val = static_cast <typename std::conditional <
					std::is_floating_point <T>::value,
					double,
					long long
				>::type> (plane._ptr [x]);
				std::cout << val << " ";
			}
			std::cout << "\n";

			plane.step_line ();
		}
	}
}



template <typename T>
T	conv_val (double v, int res)
{
	fstb::unused (res);
	static_assert (
		std::is_floating_point <T>::value,
		"Specialisation required for non-floating point types."
	);
	return T (v);
}

template <>
uint8_t	conv_val <uint8_t> (double v, int res)
{
	assert (res > 0);
	assert (res <= 8);

	const auto     scale = (1 << res) - 1;
	const int      vi    = fstb::round_int (v * double (scale));

	return uint8_t (fstb::limit (vi, 0, 0xFF));
}

template <>
uint16_t	conv_val <uint16_t> (double v, int res)
{
	assert (res > 0);
	assert (res <= 16);

	const auto     scale = (1 << res) - 1;
	const int      vi = fstb::round_int (v * double (scale));

	return uint16_t (fstb::limit (vi, 0, 0xFFFF));
}



template <typename T>
constexpr fmtcl::SplFmt	get_splfmt ()
{
	static_assert (std::is_same <T, float>::value, "Unsupported type");
	return fmtcl::SplFmt_FLOAT;
}

template <>
constexpr fmtcl::SplFmt	get_splfmt <uint8_t> ()
{
	return fmtcl::SplFmt_INT8;
}

template <>
constexpr fmtcl::SplFmt	get_splfmt <uint16_t> ()
{
	return fmtcl::SplFmt_INT16;
}



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	TestGammaY::perform_test ()
{
	int            ret_val = 0;

	printf ("Testing fmtcl::GammaY...\n"); fflush (stdout);

	constexpr auto g_arr = std::array <double, 5> { 1.2, 1/1.2, 1.5, 1/1.5, 1.0 };
	for (auto gamma : g_arr)
	{
		if (ret_val == 0)
		{
			printf ("--- gamma = %f ---\n\n", gamma);
		}

		if (ret_val == 0)
		{
			ret_val = test_achrom_row <float   , float   > (32, 32, gamma);
		}
		if (ret_val == 0)
		{
			ret_val = test_achrom_row <uint16_t, uint16_t> (16, 16, gamma);
		}
		if (ret_val == 0)
		{
			ret_val = test_achrom_row <uint16_t, uint16_t> (12, 16, gamma);
		}
		if (ret_val == 0)
		{
			ret_val = test_achrom_row <uint8_t , uint16_t> ( 8, 16, gamma);
		}
		if (ret_val == 0)
		{
			ret_val = test_achrom_row <uint8_t , float   > ( 8, 32, gamma);
		}
		if (ret_val == 0)
		{
			ret_val = test_achrom_row <float   , uint16_t> (32, 16, gamma);
		}
		if (ret_val == 0)
		{
			ret_val = test_achrom_row <uint16_t, float   > (16, 32, gamma);
		}
	}

	printf ("Done.\n"); fflush (stdout);

	return ret_val;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



// Tests a single row of achromatic data
template <typename TS, typename TD>
int	TestGammaY::test_achrom_row (int src_res, int dst_res, double gamma)
{
	constexpr double  alpha = 1.0;
	int            ret_val = 0;

#if 1
	// Bigger set
	const int      w = 2345;
	constexpr double  mi = -0.25;
	constexpr double  ma =  1.25;
	std::vector <TS> base_row (w);
	for (int x = 0; x < w; ++x)
	{
		const double   val = mi + (ma - mi) * double (x) / double (w - 1);
		base_row [x] = conv_val <TS> (val, src_res);
	}
	const auto     src_row_arr {
		fstb::make_array <fmtcl::GammaY::_nbr_planes, std::vector <TS> > (
			base_row
		)
	};
#else
	// Small value set, for basic tests
	const auto     src_row_arr {
		fstb::make_array <fmtcl::GammaY::_nbr_planes, std::vector <TS> > (
			std::vector <TS> {
				conv_val <TS> (-0.25, src_res),
				conv_val <TS> ( 0   , src_res),
				conv_val <TS> ( 0.25, src_res),
				conv_val <TS> ( 0.5 , src_res),
				conv_val <TS> ( 0.75, src_res),
				conv_val <TS> ( 1.0 , src_res),
				conv_val <TS> ( 1.25, src_res)
			}
		)
	};
	const int      w = int (src_row_arr.front ().size ());
#endif

	const int      h = 1;
	auto           dst_row_arr {
		fstb::make_array <fmtcl::GammaY::_nbr_planes, std::vector <TD> > (
			std::vector <TD> (w, conv_val <TD> (0, dst_res))
		)
	};

	fmtcl::GammaY  gammay (
		get_splfmt <TS> (), src_res,
		get_splfmt <TD> (), dst_res,
		gamma, alpha,
		false, false
	);

	fmtcl::Frame <uint8_t> dst_arr {
		{ reinterpret_cast <      uint8_t *> (dst_row_arr [0].data ()), 0 },
		{ reinterpret_cast <      uint8_t *> (dst_row_arr [1].data ()), 0 },
		{ reinterpret_cast <      uint8_t *> (dst_row_arr [2].data ()), 0 }
	};
	fmtcl::Frame <const uint8_t> src_arr {
		{ reinterpret_cast <const uint8_t *> (src_row_arr [0].data ()), 0 },
		{ reinterpret_cast <const uint8_t *> (src_row_arr [1].data ()), 0 },
		{ reinterpret_cast <const uint8_t *> (src_row_arr [2].data ()), 0 }
	};

	gammay.process_plane (dst_arr, src_arr, w, h);

	const bool     dump_flag = (w <= 10);

	printf ("# Input (%2d bits) #", src_res);
	if (dump_flag)
	{
		printf ("\n");
		dump_pic (fmtcl::FrameRO <TS> (src_arr), w, h, 1);
	}
	printf ("# Output (%2d bits) #", dst_res);
	if (dump_flag)
	{
		printf ("\n");
		dump_pic (fmtcl::FrameRO <TD> (dst_arr), w, h, 1);
	}

	// Checks the result against the theoretical truth
	const double   scale_s = conv_val <TS> (1.0, src_res);
	const double   scale_d = conv_val <TD> (1.0, dst_res);
	if (dump_flag)
	{
		printf ("# Relative error %% #");
	}
	double         err_m_a = 0; // Max of |errors| (output range)
	double         err_m_r = 0; // Max of |relative errors|
	double         err_sum = 0; // Sum of errors (output range)
	double         err_sre = 0; // Sum of |relative errors|
	int            err_cnt = 0; // Sum count
	for (int x = 0; x < w; ++x)
	{
		bool           clip_flag = false;
		const auto     raw_s = src_row_arr [0] [x];
		const auto     raw_d = dst_row_arr [0] [x];
		const double   val_s = double (raw_s) / scale_s;
		double         val_r =
			(val_s == 0) ? 0 : val_s * alpha * pow (fabs (val_s), gamma - 1);
		if (! std::is_floating_point <TD>::value)
		{
			const double   val_r_old = val_r;
			val_r = fstb::limit (val_r, 0.0, 1.0);
			if (val_r != val_r_old)
			{
				clip_flag = true;
			}
		}
		double         raw_r   = val_r * scale_d;
		if (! std::is_floating_point <TD>::value)
		{
			raw_r = std::round (raw_r);
		}
		const double   err     = raw_d - raw_r;
		const double   err_abs = fabs (err);
		const double   err_rel =
			  fstb::is_null (raw_r)
			? err_abs
			: err_abs / raw_r;
		assert (err_rel < 1);
		if (! clip_flag)
		{
			err_sum += err;
			err_sre += err_rel;
			++ err_cnt;
		}
		err_m_a = std::max (err_m_a, err_abs);
		err_m_r = std::max (err_m_r, err_rel);
		if (dump_flag)
		{
			printf ("%s%f", (x == 0) ? "\n" : " ", err_rel * 100);
		}
	}
	printf ("\n");
	if (! dump_flag) // Statistics
	{
		assert (err_cnt > 0);
		const double   avg_abs = err_sum / err_cnt;
		const double   avg_rel = err_sre / err_cnt;
		printf (
			"Rounding errors: max abs = %f, avg = %+f, max rel = %f %%, avg rel = %f %%\n",
			err_m_a, avg_abs, err_m_r * 100, avg_rel * 100
		);
	}

	if (err_m_a > 1e-3 * scale_d)
	{
		printf ("*** Error is too large. Something must be wrong. ***\n");
		ret_val = -1;
	}

	printf ("\n");

	return ret_val;
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
