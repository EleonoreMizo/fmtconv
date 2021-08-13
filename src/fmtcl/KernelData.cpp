/*****************************************************************************

        KernelData.cpp
        Author: Laurent de Soras, 2011

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "ffft/FFTReal.h"
#include "fmtcl/KernelData.h"
#include "fmtcl/ContFirBlackman.h"
#include "fmtcl/ContFirBlackmanMinLobe.h"
#include "fmtcl/ContFirCubic.h"
#include "fmtcl/ContFirFromDiscrete.h"
#include "fmtcl/ContFirGauss.h"
#include "fmtcl/ContFirLanczos.h"
#include "fmtcl/ContFirLinear.h"
#include "fmtcl/ContFirRect.h"
#include "fmtcl/ContFirSinc.h"
#include "fmtcl/ContFirSnh.h"
#include "fmtcl/ContFirSpline.h"
#include "fmtcl/ContFirSpline16.h"
#include "fmtcl/ContFirSpline36.h"
#include "fmtcl/ContFirSpline64.h"
#include "fmtcl/DiscreteFirCustom.h"
#include "fmtcl/fnc.h"
#include "fstb/def.h"
#include "fstb/fnc.h"

#include <stdexcept>

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>



namespace fmtcl
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



uint32_t	KernelData::get_hash () const
{
	return (_hash);
}



void	KernelData::create_kernel (std::string kernel_fnc, const std::vector <double> &coef_arr, int taps, bool a1_flag, double a1, bool a2_flag, double a2, bool a3_flag, double a3, int kovrspl, bool inv_flag, int inv_taps)
{
	hash_reset ();
	create_kernel_base (kernel_fnc, coef_arr, taps, a1_flag, a1, a2_flag, a2, a3_flag, a3, kovrspl);
	hash_val (int (inv_flag ? 0 : 1));
	if (inv_flag)
	{
		hash_val (inv_taps);
		invert_kernel (inv_taps);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	KernelData::create_kernel_base (std::string kernel_fnc, std::vector <double> coef_arr, int taps, bool a1_flag, double a1, bool a2_flag, double a2, bool a3_flag, double a3, int kovrspl)
{
	fstb::unused (a3, a3_flag);

	fstb::conv_to_lower_case (kernel_fnc);
	const std::string::size_type	name_end = kernel_fnc.find (' ');
	const std::string name = kernel_fnc.substr (0, name_end);

	if (strcmp (name.c_str (), "point") == 0)
	{
		hash_byte (KType_SNH);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirSnh);
	}
	else if (   strcmp (name.c_str (), "rect") == 0
	         || strcmp (name.c_str (), "box" ) == 0)
	{
		hash_byte (KType_RECT);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirRect);
	}
	else if (   strcmp (name.c_str (), "linear"  ) == 0
	         || strcmp (name.c_str (), "bilinear") == 0)
	{
		hash_byte (KType_LINEAR);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirLinear);
	}
	else if (   strcmp (name.c_str (), "cubic"  ) == 0
	         || strcmp (name.c_str (), "bicubic") == 0)
	{
		hash_byte (KType_CUBIC);
		if (! a1_flag)
		{
			a1 = 1.0 / 3;
		}
		if (! a2_flag)
		{
			a2 = 1.0 / 3;
		}
		hash_val (a1);
		hash_val (a2);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirCubic (a1, a2));
	}
	else if (strcmp (name.c_str (), "lanczos") == 0)
	{
		hash_byte (KType_LANCZOS);
		hash_val (taps);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirLanczos (taps));
	}
	else if (strcmp (name.c_str (), "blackman") == 0)
	{
		hash_byte (KType_BLACKMAN);
		hash_val (taps);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirBlackman (taps));
	}
	else if (strcmp (name.c_str (), "blackmanminlobe") == 0)
	{
		hash_byte (KType_BLACKMAN_MINLOBE);
		hash_val (taps);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirBlackmanMinLobe (taps));
	}
	else if (strcmp (name.c_str (), "spline") == 0)
	{
		hash_byte (KType_SPLINE);
		hash_val (taps);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirSpline (taps));
	}
	else if (strcmp (name.c_str (), "spline16") == 0)
	{
		hash_byte (KType_SPLINE16);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirSpline16);
	}
	else if (strcmp (name.c_str (), "spline36") == 0)
	{
		hash_byte (KType_SPLINE36);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirSpline36);
	}
	else if (strcmp (name.c_str (), "spline64") == 0)
	{
		hash_byte (KType_SPLINE64);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirSpline64);
	}
	else if (   strcmp (name.c_str (), "gauss"   ) == 0
	         || strcmp (name.c_str (), "gaussian") == 0)
	{
		hash_byte (KType_GAUSS);
		if (! a1_flag)
		{
			a1 = 30.0;
		}
		else if (a1 < 1 || a1 > 100)
		{
			throw std::runtime_error (
				"For the gaussian kernel, a1 must be in the range 1-100."
			);
		}
		hash_val (a1);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirGauss (taps, a1));
	}
	else if (strcmp (name.c_str (), "sinc") == 0)
	{
		hash_byte (KType_SINC);
		hash_val (taps);
		_k_uptr = std::unique_ptr <ContFirInterface> (new ContFirSinc (taps));
	}
	else if (strcmp (name.c_str (), "impulse") == 0)
	{
		hash_byte (KType_IMPULSE);
		if (coef_arr.empty ())
		{
			if (name_end == std::string::npos)
			{
				throw std::runtime_error ("impulse coefficients missing.");
			}
			coef_arr = conv_str_to_arr <double> (kernel_fnc.substr (name_end));
		}
		const size_t   sz = coef_arr.size ();
		if (sz == 0)
		{
			throw std::runtime_error ("impulse coefficients missing.");
		}
		if ((sz & 1) != 1)
		{
			throw std::runtime_error ("number of coefficients must be odd.");
		}
		for (size_t i = 0; i < sz / 2; ++i)
		{
			std::swap (coef_arr [i], coef_arr [sz - i - 1]);
		}
		if (kovrspl < 1)
		{
			kovrspl = 1;
		}
		for (size_t i = 0; i < sz; ++i)
		{
			hash_val (coef_arr [i]);
		}
		hash_val (kovrspl);
		_discrete_uptr = std::unique_ptr <DiscreteFirInterface> (
			new DiscreteFirCustom (kovrspl, coef_arr)
		);
		_k_uptr = std::unique_ptr <ContFirInterface> (
			new ContFirFromDiscrete (*_discrete_uptr)
		);
	}
	else
	{
		throw std::runtime_error ("unknown kernel.");
	}
}



/*
o_s = 64;
o_f = 64;
sp = 1;	% Support
r = 1;	% Downscaling factor
taps = 8;

len = 2 .^ ceil (log2 (o_s*o_f*sp*2));
t = linspace ((-len/2) / o_s, (len/2-1) / o_s, len);
x = max (1 - abs (t), 0);

s = fft (x);
p = floor (r * o_f*sp*2);
s = [s(1:p) zeros(1,len-p*2+1) s(len-p+2:len)];
s = s / abs (s (1));
s2 = s ./ (s.^2 + 0.0001);


y = real (ifft (s2));
w = 0.5 + 0.5*cos (pi*max(min(t/taps,1),-1));
y = y .* w;
y = y / sum (y);

z = conv (x, y);
%z = z / sum (z);

plot (t, x, t, y, t, z(len/2+1:3*len/2)); %axis ([-sp*4 sp*4 -2 4]);
%plot (t, 20*log10 (abs (s .* s2) + 1e-4));
%plot (t, 20*log10 (abs (s2) + 1e-4));
grid on;

*/
void	KernelData::invert_kernel (int taps)
{
	assert (taps >= 1);

	const double   cutoff  = 0.95;

	// First, get the original oversampled kernel
	const int      ovr_s   = 64;  // Impulse oversampling, spatial
	const int      ovr_f   = 64;  // Frequency oversampling
	const double   support = _k_uptr->get_support ();
	assert (ovr_f * support >= taps);
	int            len     = fstb::ceil_int (ovr_s * ovr_f * support) * 2;
	len = 1 << (fstb::get_prev_pow_2 (len - 1) + 1); // Next power of 2
	const int      h_len   = len / 2;

	std::vector <double> x (len);
	const double   inv_ovr_s = 1.0 / ovr_s;
	for (int k = 0; k < len; ++k)
	{
		const double   pos = (k - h_len) * inv_ovr_s;
		x [k] = _k_uptr->get_val (pos);
	}

	// Frequency domain operations
	std::vector <double> f (len); // Frequency data
	ffft::FFTReal <double>  fft (len);
	fft.do_fft (&f [0], &x [0]);

	// Inverts the spectrum and applies an anti-aliasing lowpass filter
	const int      nbr_bins = fstb::floor_int (ovr_f * support);
	// Scaling factor to make sure we operate in a normalized range so the
	// bin level limit is relative to the DC bin level.
	const double   amp  = 1.0 / f [0];
	const double   gmul = 1.0 / (nbr_bins * nbr_bins * cutoff * cutoff);
	f [0] = 1;
	// Opt: we could stop at nbr_bins and just clear everything above.
	for (int k = 1; k < h_len; ++k)
	{
		// Bin coefficients
		const double   re  = amp * f [k        ];
		const double   im  = amp * f [k + h_len];
		const double   n2  = re * re + im * im;

		// Low-pass filter
		const double   k2  = (k*k) * gmul;
		const double   k8  = k2 * k2 * k2;
		const double   k32 = k8 * k8 * k2;
		const double   gau = exp (-k32);

		// Inverts gain
		const double   scale = gau / (n2 + 0.0001);
		f [k        ] =  re * scale;
		f [k + h_len] = -im * scale;
	}
	f [h_len] = 0;

	// Back to time domain.
	fft.do_ifft (&f [0], &x [0]);

	// Windowing
	const int         h_len_i  = ovr_s * taps;
	double            norm_sum = 0;
	if (taps <= 4)
	{
		apply_window <ContFirLanczos> (x, norm_sum, taps, h_len, h_len_i, inv_ovr_s);
	}
	else
	{
		apply_window <ContFirBlackman> (x, norm_sum, taps, h_len, h_len_i, inv_ovr_s);
	}

	// Normalizes the impulse
	const double   norm_scale = 0.5 * ovr_s / norm_sum;
	for (int k = 1 - h_len_i; k < h_len_i; ++k)
	{
		x [h_len - k] *= norm_scale;
	}

	const int      len_i = h_len_i * 2;
	_discrete_uptr = std::unique_ptr <DiscreteFirInterface> (
		new DiscreteFirCustom (ovr_s, &x [h_len - h_len_i + 1], len_i - 1)
	);
	_k_uptr = std::unique_ptr <ContFirInterface> (
		new ContFirFromDiscrete (*_discrete_uptr)
	);
}



void	KernelData::hash_reset ()
{
	_hash = 0xFFFFFFFF;
}



void	KernelData::hash_byte (uint8_t x)
{
	// Naive CRC32 implementation. Speed is not important here.
	_hash ^= uint32_t (x) << (32 - 8);
	for (int bit = 8; bit > 0; --bit)
	{
		if ((_hash & 0x80000000U) != 0)
		{
			_hash = (_hash << 1) ^ 0x04C11DB7;
		}
		else
		{
			_hash <<= 1;
		}
	}
}



template <typename T>
void	KernelData::hash_val (const T &val)
{
	for (int i = 0; i < int (sizeof (val)); ++i)
	{
		hash_byte (reinterpret_cast <const uint8_t *> (&val) [i]);
	}
}



template <class W>
void	KernelData::apply_window (std::vector <double>	&x, double &norm_sum, int taps, int h_len, int h_len_i, double inv_ovr_s)
{
	norm_sum = 0;
	W              win (taps);
	for (int k = 1; k < h_len_i; ++k)
	{
		const double   pos = k * inv_ovr_s;
		const double   w_val = win.compute_win_coef (pos);
		const double   an = x [h_len - k] * w_val;
		const double   ap = x [h_len + k] * w_val;
		x [h_len - k] = an;
		x [h_len + k] = ap;
		norm_sum += an + ap;
	}
}



}	// namespace fmtcl



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
