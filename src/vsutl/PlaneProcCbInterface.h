/*****************************************************************************

        PlaneProcCbInterface.h
        Author: Laurent de Soras, 2012

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_PlaneProcCbInterface_HEADER_INCLUDED)
#define	vsutl_PlaneProcCbInterface_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vsutl/NodeRefSPtr.h"



struct VSCore;
struct VSFrameContext;
struct VSFrame;



namespace vsutl
{



class PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	virtual        ~PlaneProcCbInterface () = default;

	int            process_plane (::VSFrame &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const NodeRefSPtr &src_node1_sptr, const NodeRefSPtr &src_node2_sptr, const NodeRefSPtr &src_node3_sptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	virtual int    do_process_plane (::VSFrame &dst, int n, int plane_index, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, const NodeRefSPtr &src_node1_sptr, const NodeRefSPtr &src_node2_sptr, const NodeRefSPtr &src_node3_sptr) = 0;



};	// class PlaneProcCbInterface



}	// namespace vsutl



//#include "vsutl/PlaneProcCbInterface.hpp"



#endif	// vsutl_PlaneProcCbInterface_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
