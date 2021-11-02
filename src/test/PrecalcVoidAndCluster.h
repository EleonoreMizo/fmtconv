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



class PrecalcVoidAndCluster
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static int     generate_mat (int size_l2);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



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
