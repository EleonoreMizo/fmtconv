/*****************************************************************************

        main.cpp
        Author: Laurent de Soras, 2021

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://www.wtfpl.net/ for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (4 : 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "test/TestGammaY.h"

#if defined (_MSC_VER)
#include <crtdbg.h>
#include <new.h>
#endif   // _MSC_VER

#include <iostream>
#include <new>

#include <cassert>



/*\\\ CLASS DEFINITIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



#if defined (_MSC_VER)
static int __cdecl	MAIN_new_handler_cb (size_t dummy)
{
	fstb::unused (dummy);
	throw std::bad_alloc ();
}
#endif	// _MSC_VER



#if defined (_MSC_VER) && ! defined (NDEBUG)
static int	__cdecl	MAIN_debug_alloc_hook_cb (int alloc_type, void *user_data_ptr, size_t size, int block_type, long request_nbr, const unsigned char *filename_0, int line_nbr)
{
	fstb::unused (user_data_ptr, size, request_nbr, filename_0, line_nbr);

	if (block_type != _CRT_BLOCK)	// Ignore CRT blocks to prevent infinite recursion
	{
		switch (alloc_type)
		{
		case _HOOK_ALLOC:
		case _HOOK_REALLOC:
		case _HOOK_FREE:

			// Put some debug code here

			break;

		default:
			assert (false);	// Undefined allocation type
			break;
		}
	}

	return 1;
}
#endif



#if defined (_MSC_VER) && ! defined (NDEBUG)
static int	__cdecl	MAIN_debug_report_hook_cb (int report_type, char *user_msg_0, int *ret_val_ptr)
{
	fstb::unused (user_msg_0);

	*ret_val_ptr = 0;	// 1 to override the CRT default reporting mode

	switch (report_type)
	{
	case _CRT_WARN:
	case _CRT_ERROR:
	case _CRT_ASSERT:

// Put some debug code here

		break;
	}

	return *ret_val_ptr;
}
#endif



static void	MAIN_main_prog_init ()
{
#if defined (_MSC_VER)
	::_set_new_handler (::MAIN_new_handler_cb);
#endif   // _MSC_VER

#if defined (_MSC_VER) && ! defined (NDEBUG)
	{
		const int      mode =   (1 * _CRTDBG_MODE_DEBUG)
		                      | (1 * _CRTDBG_MODE_WNDW);
		::_CrtSetReportMode (_CRT_WARN, mode);
		::_CrtSetReportMode (_CRT_ERROR, mode);
		::_CrtSetReportMode (_CRT_ASSERT, mode);

		const int      old_flags = ::_CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);
		::_CrtSetDbgFlag (  old_flags
		                  | (1 * _CRTDBG_LEAK_CHECK_DF)
		                  | (1 * _CRTDBG_CHECK_ALWAYS_DF));
		::_CrtSetBreakAlloc (-1);	// Specify here a memory bloc number
		::_CrtSetAllocHook (MAIN_debug_alloc_hook_cb);
		::_CrtSetReportHook (MAIN_debug_report_hook_cb);

		// Speed up I/O but breaks C stdio compatibility
//		std::cout.sync_with_stdio (false);
//		std::cin.sync_with_stdio (false);
//		std::cerr.sync_with_stdio (false);
//		std::clog.sync_with_stdio (false);
	}
#endif	// _MSC_VER, NDEBUG
}



static void	MAIN_main_prog_end ()
{
#if defined (_MSC_VER) && ! defined (NDEBUG)
	{
		const int      mode =   (1 * _CRTDBG_MODE_DEBUG)
		                      | (0 * _CRTDBG_MODE_WNDW);
		::_CrtSetReportMode (_CRT_WARN, mode);
		::_CrtSetReportMode (_CRT_ERROR, mode);
		::_CrtSetReportMode (_CRT_ASSERT, mode);

		::_CrtMemState mem_state;
		::_CrtMemCheckpoint (&mem_state);
		::_CrtMemDumpStatistics (&mem_state);
	}
#endif   // _MSC_VER, NDEBUG
}



int main (int argc, char *argv [])
{
	fstb::unused (argc, argv);

	int            ret_val = 0;

	MAIN_main_prog_init ();

	try
	{
		if (ret_val == 0) { ret_val = TestGammaY::perform_test (); }









	}

	catch (std::exception &e)
	{
		std::cout << "*** main() : Exception (std::exception) : " << e.what () << std::endl;
		throw;
	}

	catch (...)
	{
		std::cout << "*** main() : Undefined exception" << std::endl;
		throw;
	}

	MAIN_main_prog_end ();

	return ret_val;
}



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
