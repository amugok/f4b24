#include "fittle.h"
#include "wastr.h"


double BASSDEF(BASS_ChannelBytes2Seconds)(DWORD handle, QWORD pos);
BOOL BASSDEF(BASS_ChannelGetInfo)(DWORD handle, BASS_CHANNELINFO *info);
QWORD BASSDEF(BASS_ChannelGetLength)(DWORD handle, DWORD mode);
QWORD BASSDEF(BASS_ChannelGetPosition)(DWORD handle, DWORD mode);
const char *BASSDEF(BASS_ChannelGetTags)(DWORD handle, DWORD tags);
BOOL BASSDEF(BASS_ChannelPlay)(DWORD handle, BOOL restart);
QWORD BASSDEF(BASS_ChannelSeconds2Bytes)(DWORD handle, double pos);
BOOL BASSDEF(BASS_ChannelSetPosition)(DWORD handle, QWORD pos, DWORD mode);
DWORD BASSDEF(BASS_ChannelSetSync)(DWORD handle, DWORD type, QWORD param, SYNCPROC *proc, void *user);
BOOL BASSDEF(BASS_ChannelStop)(DWORD handle);
BOOL BASSDEF(BASS_Free)();
DWORD BASSDEF(BASS_GetVersion)();
BOOL BASSDEF(BASS_Init)(int device, DWORD freq, DWORD flags, HWND win, const GUID *dsguid);
BOOL BASSDEF(BASS_MusicFree)(DWORD handle);
DWORD BASSDEF(BASS_MusicLoad)(BOOL mem, const void *file, QWORD offset, DWORD length, DWORD flags, DWORD freq);
const BASS_PLUGININFO *BASSDEF(BASS_PluginGetInfo)(DWORD handle);
DWORD BASSDEF(BASS_PluginLoad)(const char *file, DWORD flags);
BOOL BASSDEF(BASS_SetConfig)(DWORD option, DWORD value);
DWORD BASSDEF(BASS_StreamCreateFile)(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags);
DWORD BASSDEF(BASS_StreamCreateURL)(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user);
BOOL BASSDEF(BASS_StreamFree)(DWORD handle);
QWORD BASSDEF(BASS_StreamGetFilePosition)(DWORD handle, DWORD mode);

#define FUNC_PREFIXA "BASS_"
static CHAR szDllNameA[] = "bass.dll";
static struct IMPORT_FUNC_TABLE {
	LPSTR lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
	{ "ChannelBytes2Seconds", (FARPROC *)&BASS_ChannelBytes2Seconds },
	{ "ChannelGetInfo", (FARPROC *)&BASS_ChannelGetInfo },
	{ "ChannelGetLength", (FARPROC *)&BASS_ChannelGetLength },
	{ "ChannelGetPosition", (FARPROC *)&BASS_ChannelGetPosition },
	{ "ChannelGetTags", (FARPROC *)&BASS_ChannelGetTags },
	{ "ChannelPlay", (FARPROC *)&BASS_ChannelPlay },
	{ "ChannelSeconds2Bytes", (FARPROC *)&BASS_ChannelSeconds2Bytes },
	{ "ChannelSetPosition", (FARPROC *)&BASS_ChannelSetPosition },
	{ "ChannelSetSync", (FARPROC *)&BASS_ChannelSetSync },
	{ "ChannelStop", (FARPROC *)&BASS_ChannelStop },
	{ "Free", (FARPROC *)&BASS_Free },
	{ "GetVersion", (FARPROC *)&BASS_GetVersion },
	{ "Init", (FARPROC *)&BASS_Init },
	{ "MusicFree", (FARPROC *)&BASS_MusicFree },
	{ "MusicLoad", (FARPROC *)&BASS_MusicLoad },
	{ "PluginGetInfo", (FARPROC *)&BASS_PluginGetInfo },
	{ "PluginLoad", (FARPROC *)&BASS_PluginLoad },
	{ "SetConfig", (FARPROC *)&BASS_SetConfig },
	{ "StreamCreateFile", (FARPROC *)&BASS_StreamCreateFile },
	{ "StreamCreateURL", (FARPROC *)&BASS_StreamCreateURL },
	{ "StreamFree", (FARPROC *)&BASS_StreamFree },
	{ "StreamGetFilePosition", (FARPROC *)&BASS_StreamGetFilePosition },
	{ 0, (FARPROC *)0 }
};


static HMODULE m_hBASS = 0;

BOOL LoadBASS(void){
	const struct IMPORT_FUNC_TABLE *pft;
	CHAR funcname[32];
	WASTR szPath;
	int l;
	if (m_hBASS) return TRUE;

	WAGetModuleParentDir(NULL, &szPath);
	WAstrcatA(&szPath, "Plugins\\BASS\\");
	WAstrcatA(&szPath, szDllNameA);
	m_hBASS = WALoadLibrary(&szPath);
	if(!m_hBASS) return FALSE;

	lstrcpynA(funcname, FUNC_PREFIXA, 32);
	l = lstrlenA(funcname);

	for (pft = functbl; pft->lpszFuncName; pft++){
		lstrcpynA(funcname + l, pft->lpszFuncName, 32 - l);
		FARPROC fp = GetProcAddress(m_hBASS, funcname);
		if (!fp){
			FreeLibrary(m_hBASS);
			m_hBASS = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}
	return TRUE;
}
