/*****************************************************************************

        PlaneProcessor.h
        Author: Laurent de Soras, 2012

Reorganisation necessaire:

- Separer plus clairement le mode manuel du mode pris en charge.
- On a besoin d'un suivi entre la demande des frames sources et la mise
a disposition du resultat. PlaneProcessor doit donc fournir une pool de
PlaneProcContext qui contient entre autres la frame destination.
- La frame destination devrait etre cree directement par le PlaneProcessor,
en rajoutant les parametres necessaires a la fonction principale. On pourrait
ainsi utiliser newVideoFrame2 pour tout ce qui est copie exacte de plans.
- Reflechir au cas des frames a taille variable.



--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (vsutl_PlaneProcessor_HEADER_INCLUDED)
#define	vsutl_PlaneProcessor_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "vsutl/NodeRefSPtr.h"
#include "vsutl/FrameRefSPtr.h"
#include "vsutl/PlaneProcMode.h"

#include <string>



struct VSAPI;
struct VSCore;
struct VSFrameContext;
struct VSFrame;
struct VSMap;
struct VSVideoInfo;



namespace vsutl
{



class PlaneProcCbInterface;

class PlaneProcessor
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const int  MAX_NBR_PLANES	= 3;
	static const int  MAX_NBR_CLIPS	= 1+3;   // Index 0 = destination

	explicit       PlaneProcessor (const ::VSAPI &vsapi, PlaneProcCbInterface &cb, const char filter_name_0 [], bool manual_flag);
	virtual        ~PlaneProcessor () {}

	void           set_filter (const ::VSMap &in, ::VSMap &out, const ::VSVideoInfo &vi_out, bool simple_flag = false, int max_def_planes = MAX_NBR_PLANES, const char *prop_name_0 = "planes", const char *clip_name_0 = "clip");

	const ::VSFrame *
	               try_initial (::VSCore &core);
	int            process_frame (::VSFrame &dst, int n, void *frame_data_ptr, ::VSFrameContext &frame_ctx, ::VSCore &core, NodeRefSPtr src_node1_sptr = vsutl::NodeRefSPtr (), NodeRefSPtr src_node2_sptr = vsutl::NodeRefSPtr (), NodeRefSPtr src_node3_sptr = vsutl::NodeRefSPtr ());

	// For manual operations
	bool           is_manual () const;
	PlaneProcMode  get_mode (int plane_index) const;
	double         get_mode_val (int plane_index) const;

	void           fill_plane (::VSFrame &dst, double val, int plane_index);
	void           copy_plane (::VSFrame &dst, const ::VSFrame &src, int plane_index);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	template <class T>
	void           fill_plane (void *ptr, T val, ptrdiff_t stride, int w, int h);


	const ::VSAPI& _vsapi;
	const std::string
	               _filter_name;
	PlaneProcCbInterface &
	               _cb;
	::VSVideoInfo  _vi_out;
	int            _nbr_planes;
	double         _proc_mode_arr [MAX_NBR_PLANES]; // PlaneProcMode or value < 1
	bool           _manual_flag;
	bool				_input_flag;  // Indicates that we need an input (at least one copy or process)
	FrameRefSPtr   _blank_frame_sptr;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               PlaneProcessor ()                               = delete;
	               PlaneProcessor (const PlaneProcessor &other)    = delete;
	PlaneProcessor &
	               operator = (const PlaneProcessor &other)        = delete;
	bool           operator == (const PlaneProcessor &other) const = delete;
	bool           operator != (const PlaneProcessor &other) const = delete;

};	// class PlaneProcessor



}	// namespace vsutl



//#include "vsutl/PlaneProcessor.hpp"



#endif	// vsutl_PlaneProcessor_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
