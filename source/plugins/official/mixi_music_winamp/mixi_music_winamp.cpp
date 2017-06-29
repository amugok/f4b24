#ifdef _WIN64
#pragma comment(lib,"..\\..\\..\\..\\extra\\smartvc14\\smartvc14_x64.lib")
#pragma comment(linker,"/ENTRY:SmartDllStartup")
#else
#pragma comment(lib,"..\\..\\..\\..\\extra\\smartvc14\\smartvc14_x86.lib")
#pragma comment(linker,"/ENTRY:SmartDllStartup@12")
#endif

#include "../../../fittle/src/plugin.h"
#include "../../../fittle/src/bass_tag.h"

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

#include "MainWnd.h"
#include <shlwapi.h>
//#include "resource.h"
#include <tchar.h>
#include "GEN.H"
#include "wa_ipc.h"

#include "plugin.cpp"
#include "MainWnd.cpp"
