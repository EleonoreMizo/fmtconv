/*****************************************************************************

        PrecalcVoidAndCluster.cpp
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

#include "test/PrecalcVoidAndCluster.h"
#include "fmtcl/MatrixWrap.h"
#include "fmtcl/VoidAndCluster.h"

#include <chrono>

#include <cassert>
#include <cstdio>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	PrecalcVoidAndCluster::generate_mat (int size_l2)
{
	assert (size_l2 >= 2);
	assert (size_l2 <= 24);

	const int      w = 1 << size_l2;
	const int      h = 1 << size_l2;

	fmtcl::VoidAndCluster   vc_gen;
	fmtcl::MatrixWrap <fmtcl::VoidAndCluster::Rank> pat (w, h);

	typedef std::chrono::high_resolution_clock Clock;
	const auto     t_beg = Clock::now ();
	vc_gen.create_matrix (pat);
	const auto     t_end = Clock::now ();
	const std::chrono::duration <double> dur = t_end - t_beg;

	const int      area  = w * h;
	int            count = 0;
	printf (
		"static const std::array <int16_t, %d * %d> pat_%d {\n",
		w, h, size_l2
	);
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const auto     v = int16_t (pat (x, y) * 256 / area - 128);
			++ count;
			printf (
				"%d%s%s",
				v,
				(count < area) ? "," : "",
				((count & 15) == 0) ? "\n" : ""
			);
		}
	}
	printf ("};\n");

	printf ("Duration: %.3f s\n", dur.count ());

	return 0;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
