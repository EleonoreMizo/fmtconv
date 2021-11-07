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
	// Caused in an internal header by std::async
	#pragma warning (disable : 4355)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "test/PrecalcVoidAndCluster.h"
#include "fmtcl/MatrixWrap.h"
#include "fmtcl/VoidAndCluster.h"
#include "fstb/fnc.h"

#include <chrono>
#include <future>

#include <cassert>
#include <cstdio>



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



constexpr const char *	PrecalcVoidAndCluster::_namespace_0;
constexpr const char *	PrecalcVoidAndCluster::_classname_0;



PrecalcVoidAndCluster::HdrCode &	PrecalcVoidAndCluster::HdrCode::operator += (const HdrCode &other)
{
	_header += other._header;
	_code   += other._code;

	return *this;
}



PrecalcVoidAndCluster::HdrCode	PrecalcVoidAndCluster::build_all ()
{
	auto           files = print_beg ();

	auto           s10 = std::async (
		std::launch::async, generate_mat, 10, false
	);
	auto           a8 = std::async (
		std::launch::async, generate_mat, 8, true
	);
	auto           a9 = std::async (
		std::launch::async, generate_mat, 9, true
	);
	files += generate_mat (2, false);
	files += generate_mat (3, false);
	files += generate_mat (4, false);
	files += generate_mat (5, false);
	files += generate_mat (6, false);
	files += generate_mat (7, false);
	files += generate_mat (8, false);
	files += generate_mat (9, false);
	s10.wait ();
	files += s10.get ();

	files += generate_mat (2, true);
	files += generate_mat (3, true);
	files += generate_mat (4, true);
	files += generate_mat (5, true);
	files += generate_mat (6, true);
	files += generate_mat (7, true);
	a8.wait ();
	files += a8.get ();
	a9.wait ();
	files += a9.get ();

	files += print_end ();

	return files;
}



PrecalcVoidAndCluster::HdrCode	PrecalcVoidAndCluster::print_beg ()
{
	std::string    code ("// File generated automatically\n");
	std::string    header (code);

	header += "#pragma once\n#include <array>\n#include <cstdint>\nnamespace ";
	header += _namespace_0;
	header += "\n{\nclass ";
	header += _classname_0;
	header += "\n{\npublic:\n";

	code += "#include \"";
	code += _namespace_0;
	code += "/";
	code += _classname_0;
	code += ".h\"\n";
	code += "namespace ";
	code += _namespace_0;
	code += "\n{\n";

	return { header, code };
}



// Values are stored as unsigned 8-bit int, grouped by 8 in uint64_t words,
// each byte stored in reading order (byte 0 is the MSB, byte 7 is the LSB).
PrecalcVoidAndCluster::HdrCode	PrecalcVoidAndCluster::generate_mat (int size_l2, bool alt_flag)
{
	assert (size_l2 >= 2);
	assert (size_l2 <= 24);

	const int      w = 1 << size_l2;
	const int      h = 1 << size_l2;

	fmtcl::VoidAndCluster   vc_gen;
	vc_gen.set_aztec_mode (alt_flag);
	fmtcl::MatrixWrap <fmtcl::VoidAndCluster::Rank> pat (w, h);

	typedef std::chrono::high_resolution_clock Clock;
	const auto     t_beg = Clock::now ();
	vc_gen.create_matrix (pat);
	const auto     t_end = Clock::now ();
	const std::chrono::duration <double> dur = t_end - t_beg;
	const double   dur_s = dur.count ();

	if (dur_s >= 0)
	{
		printf (
			"Duration %4dx%4d, %s: %.3f s\n",
			w, h, (alt_flag) ? "alt" : "std", dur_s
		);
		fflush (stdout);
	}

	auto           header = print_var_name (size_l2, alt_flag, true);

	std::string    code;
	char           txt_0 [1023+1];

	unsigned long long   block = 0;
	constexpr int  block_size  = 8;
	static_assert (fstb::is_pow_2 (block_size), "");
	constexpr int  line_size   = 32;
	static_assert (line_size % block_size == 0, "");
	const int            area  = w * h;
	code += print_var_name (size_l2, alt_flag, false);
	int                  count = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			const auto     v = uint8_t (pat (x, y) * 256 / area /* - 128 */);
			block <<= 8;
			block += v;
			++ count;
			if ((count & (block_size - 1)) == 0)
			{
			fstb::snprintf4all (txt_0, sizeof (txt_0),
					"0x%016llX%s%s",
					block,
					(count < area) ? "," : "",
					((count & (line_size - 1)) == 0 || count == area) ? "\n" : ""
				);
				code += txt_0;
				block = 0;
			}
		}
	}
	code += "};\n";

	return { header, code };
}



PrecalcVoidAndCluster::HdrCode	PrecalcVoidAndCluster::print_end ()
{
	return { std::string ("};\n}\n"), std::string ("}\n") };
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



std::string	PrecalcVoidAndCluster::print_var_name (int size_l2, bool alt_flag, bool header_flag)
{
	const int      w = 1 << size_l2;
	const int      h = 1 << size_l2;
	char           txt_0 [1023+1];
	fstb::snprintf4all (txt_0, sizeof (txt_0),
		"%sconst std::array <uint64_t, %d*%d / %d> %s%s_pat_%d_%s%s\n",
		(header_flag) ? "static " : "",
		w, h, 8,
		(header_flag) ? "" : _classname_0,
		(header_flag) ? "" : "::",
		size_l2, (alt_flag) ? "alt" : "std",
		(header_flag) ? ";" : " {"
	);

	return std::string { txt_0 };
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
