/*****************************************************************************

        KernelData.h
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fmtcl_KernelData_HEADER_INCLUDED)
#define	fmtcl_KernelData_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fmtcl/ContFirInterface.h"
#include "fmtcl/DiscreteFirInterface.h"

#include <memory>
#include <string>
#include <vector>

#include <cstdint>



namespace fmtcl
{



class KernelData
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               KernelData ()                   = default;
	               ~KernelData ()                  = default;
	               KernelData (KernelData &&other) = default;
	KernelData &   operator = (KernelData &&other) = default;

   uint32_t       get_hash () const;

	void           create_kernel (std::string kernel_fnc, std::vector <double> &coef_arr, int taps, bool a1_flag, double a1, bool a2_flag, double a2, bool a3_flag, double a3, int kovrspl, bool inv_flag, int inv_taps);
	std::unique_ptr <ContFirInterface>
	               _k_uptr;
	std::unique_ptr <DiscreteFirInterface>
	               _discrete_uptr;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	enum KType
	{
		KType_UNDEFINED = -1,
		KType_SNH = 0,
		KType_RECT,
		KType_LINEAR,
		KType_CUBIC,
		KType_LANCZOS,
		KType_BLACKMAN,
		KType_BLACKMAN_MINLOBE,
		KType_SPLINE,
		KType_SPLINE16,
		KType_SPLINE36,
		KType_SPLINE64,
		KType_GAUSS,
		KType_SINC,
		KType_IMPULSE,
		KType_NBR_ELT
	};

	void           create_kernel_base (std::string kernel_fnc, std::vector <double> &coef_arr, int taps, bool a1_flag, double a1, bool a2_flag, double a2, bool a3_flag, double a3, int kovrspl);
	void           invert_kernel (int taps);

	void           hash_reset ();
	void           hash_byte (uint8_t x);
	template <typename T>
	void           hash_val (const T &val);

	uint32_t       _hash = 0;

	static void    conv_to_float_arr (std::vector <double> &coef_arr, const std::string &str);

	template <class W>
	static void    apply_window (std::vector <double> &x, double &norm_sum, int taps, int h_len, int h_len_i, double inv_ovr_s);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               KernelData (const KernelData &other)        = delete;
	KernelData &   operator = (const KernelData &other)        = delete;
	bool           operator == (const KernelData &other) const = delete;
	bool           operator != (const KernelData &other) const = delete;

};	// class KernelData



}	// namespace fmtcl



//#include "fmtcl/KernelData.hpp"



#endif	// fmtcl_KernelData_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
