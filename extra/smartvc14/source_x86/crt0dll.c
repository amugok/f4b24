#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern void __cdecl SmartInit();
extern void __cdecl SmartTerm();

extern BOOL WINAPI SmartDllStartup(
	HINSTANCE const instance,
	DWORD     const reason,
	LPVOID    const reserved
	)
{
	if (reason == DLL_PROCESS_ATTACH){
		SmartInit();
	}else if (reason == DLL_PROCESS_DETACH){
		SmartTerm();
	}
	return TRUE;

}

#ifdef __cplusplus
extern "C"
}
#endif
