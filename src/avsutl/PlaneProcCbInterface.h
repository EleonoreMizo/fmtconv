/*****************************************************************************

        PlaneProcCbInterface.h
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (avsutl_PlaneProcCbInterface_HEADER_INCLUDED)
#define avsutl_PlaneProcCbInterface_HEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



class IScriptEnvironment;
class PVideoFrame;

namespace avsutl
{



class PlaneProcCbInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               PlaneProcCbInterface ()  = default;
	virtual        ~PlaneProcCbInterface () = default;

	void				process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	virtual void	do_process_plane (::PVideoFrame &dst_sptr, int n, ::IScriptEnvironment &env, int plane_index, int plane_id, void *ctx_ptr) = 0;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               PlaneProcCbInterface (const PlaneProcCbInterface &other) = delete;
	               PlaneProcCbInterface (PlaneProcCbInterface &&other)      = delete;
	PlaneProcCbInterface &
	               operator = (const PlaneProcCbInterface &other) = delete;
	PlaneProcCbInterface &
	               operator = (PlaneProcCbInterface &&other)      = delete;

}; // class PlaneProcCbInterface



}  // namespace avsutl



//#include "avsutl/PlaneProcCbInterface.hpp"



#endif   // avsutl_PlaneProcCbInterface_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
