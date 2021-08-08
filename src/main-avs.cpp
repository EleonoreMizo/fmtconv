
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOGDI

#include "avsutl/fnc.h"
#include "fmtcavs/function_names.h"
#include "fstb/def.h"

#include <windows.h>
#include "avisynth.h"

#if defined (_MSC_VER) && ! defined (NDEBUG) && defined (_DEBUG)
	#include	<crtdbg.h>
#endif



template <class T>
::AVSValue __cdecl	main_avs_create (::AVSValue args, void *user_data_ptr, ::IScriptEnvironment *env_ptr)
{
	fstb::unused (user_data_ptr);

	return new T (*env_ptr, args);
}



const ::AVS_Linkage *	AVS_linkage = nullptr;

extern "C" __declspec (dllexport)
const char * __stdcall	AvisynthPluginInit3 (::IScriptEnvironment *env_ptr, const ::AVS_Linkage * const vectors_ptr)
{
	AVS_linkage = vectors_ptr;

	return "fmtconv - video format conversion";
}



static void	main_avs_dll_load (::HINSTANCE hinst)
{
	fstb::unused (hinst);

#if defined (_MSC_VER) && ! defined (NDEBUG) && defined (_DEBUG)
	{
		const int	mode =   (1 * _CRTDBG_MODE_DEBUG)
						       | (1 * _CRTDBG_MODE_WNDW);
		::_CrtSetReportMode (_CRT_WARN, mode);
		::_CrtSetReportMode (_CRT_ERROR, mode);
		::_CrtSetReportMode (_CRT_ASSERT, mode);

		const int	old_flags = ::_CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);
		::_CrtSetDbgFlag (  old_flags
		                  | (1 * _CRTDBG_LEAK_CHECK_DF)
		                  | (0 * _CRTDBG_CHECK_ALWAYS_DF));
		::_CrtSetBreakAlloc (-1);	// Specify here a memory bloc number
	}
#endif	// _MSC_VER, NDEBUG
}



static void	main_avs_dll_unload (::HINSTANCE hinst)
{
	fstb::unused (hinst);

#if defined (_MSC_VER) && ! defined (NDEBUG) && defined (_DEBUG)
	{
		const int	mode =   (1 * _CRTDBG_MODE_DEBUG)
						       | (0 * _CRTDBG_MODE_WNDW);
		::_CrtSetReportMode (_CRT_WARN, mode);
		::_CrtSetReportMode (_CRT_ERROR, mode);
		::_CrtSetReportMode (_CRT_ASSERT, mode);

		::_CrtMemState	mem_state;
		::_CrtMemCheckpoint (&mem_state);
		::_CrtMemDumpStatistics (&mem_state);
	}
#endif	// _MSC_VER, NDEBUG
}



BOOL WINAPI DllMain (::HINSTANCE hinst, ::DWORD reason, ::LPVOID reserved_ptr)
{
	fstb::unused (reserved_ptr);

	switch (reason)
	{
	case	DLL_PROCESS_ATTACH:
		main_avs_dll_load (hinst);
		break;

	case	DLL_PROCESS_DETACH:
		main_avs_dll_unload (hinst);
		break;
	}

	return TRUE;
}
