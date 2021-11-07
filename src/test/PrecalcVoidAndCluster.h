/*****************************************************************************

        PrecalcVoidAndCluster.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (PrecalcVoidAndCluster_HEADER_INCLUDED)
#define PrecalcVoidAndCluster_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include <string>



class PrecalcVoidAndCluster
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static constexpr const char *
	               _namespace_0 = "fmtcl";
	static constexpr const char *
	               _classname_0 = "VoidAndClusterPreCalc";

	class HdrCode
	{
	public:
		HdrCode &       operator += (const HdrCode &other);
		std::string     _header;
		std::string     _code;
	};

	static HdrCode  build_all ();

	static HdrCode  print_beg ();
	static HdrCode  generate_mat (int size_l2, bool alt_flag);
	static HdrCode  print_end (); 



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static std::string
	               print_var_name (int size_l2, bool alt_flag, bool header_flag);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               PrecalcVoidAndCluster ()                               = delete;
	               PrecalcVoidAndCluster (const PrecalcVoidAndCluster &other) = delete;
	               PrecalcVoidAndCluster (PrecalcVoidAndCluster &&other)      = delete;
	PrecalcVoidAndCluster &
	               operator = (const PrecalcVoidAndCluster &other)        = delete;
	PrecalcVoidAndCluster &
	               operator = (PrecalcVoidAndCluster &&other)             = delete;
	bool           operator == (const PrecalcVoidAndCluster &other) const = delete;
	bool           operator != (const PrecalcVoidAndCluster &other) const = delete;

}; // class PrecalcVoidAndCluster



//#include "test/PrecalcVoidAndCluster.hpp"



#endif   // PrecalcVoidAndCluster_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
