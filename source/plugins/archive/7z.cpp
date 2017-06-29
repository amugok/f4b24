/*
�v 7-zip32.dll/7-zip64.dll/7z.dll���������΍��� 17.00.00.01
�����_�Ŗ{�Ƃ� SevenZipExtrctMem API ����������Ă��Ȃ�

7z �ɑΉ�����

64bit�łȂ� ����� zip �ɑΉ�����

64bit 7z.dll�Ή��� �Ȃ� ����� lzh arj �ɑΉ�����

*/
#define WINVER		0x0400	// 98�ȍ~
#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0500	// IE5�ȍ~
#include "../../fittle/src/aplugin.h"
#include <shlwapi.h>
#include <time.h>
#include "cal.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shlwapi.lib")
#ifdef UNICODE
#ifdef _WIN64
#pragma comment(lib,"..\\..\\..\\extra\\smartvc14\\smartvc14_x64.lib")
#pragma comment(linker, "/EXPORT:GetAPluginInfoW=GetAPluginInfo")
#else
#pragma comment(lib,"..\\..\\..\\extra\\smartvc14\\smartvc14_x86.lib")
#pragma comment(linker, "/EXPORT:GetAPluginInfoW=_GetAPluginInfo@0")
#endif
#else
#pragma comment(lib,"..\\..\\..\\extra\\smartvc14\\smartvc14_x86.lib")
#pragma comment(linker, "/EXPORT:GetAPluginInfo=_GetAPluginInfo@0")
#endif
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif

typedef HARC (WINAPI *LPUNLHAOPENARCHIVE)(const HWND, LPCSTR, const DWORD);
typedef int (WINAPI *LPUNLHAFINDFIRST)(HARC, LPCSTR, LPINDIVIDUALINFOA);
typedef int (WINAPI *LPUNLHAFINDNEXT)(HARC, LPINDIVIDUALINFOA);
typedef int (WINAPI *LPUNLHAEXTRACTMEM)(const HWND, LPCSTR, LPBYTE, const DWORD, int *, LPWORD, LPDWORD);
typedef int (WINAPI *LPUNLHACLOSEARCHIVE)(HARC);
typedef BOOL (WINAPI *LPUNLHASETUNICODEMODE)(BOOL);
typedef BOOL (WINAPI *LPUNLHAEXISTS7ZDLL)();

static LPUNLHAOPENARCHIVE lpUnLhaOpenArchive = NULL;
static LPUNLHAFINDFIRST lpUnLhaFindFirst = NULL;
static LPUNLHAFINDNEXT lpUnLhaFindNext = NULL;
static LPUNLHAEXTRACTMEM lpUnLhaExtractMem = NULL;
static LPUNLHACLOSEARCHIVE lpUnLhaCloseArchive = NULL;
#ifdef UNICODE
static LPUNLHASETUNICODEMODE lpUnLhaSetUnicodeMode = NULL;
#endif
#ifdef _WIN64
static LPUNLHAEXISTS7ZDLL lpUnLhaExists7zdll = NULL;
static BOOL bExists7zdll = FALSE;
#endif

static HMODULE hUnlha32 = 0;
static HMODULE hDLL = 0;

#define FUNC_PREFIXA "SevenZip"
#ifdef _WIN64
static /*const*/ TCHAR szDllName[] = TEXT("7-zip64.dll");
#else
static /*const*/ TCHAR szDllName[] = TEXT("7-zip32.dll");
#endif

static /*const*/ struct IMPORT_FUNC_TABLE {
	LPSTR/*LPCSTR*/ lpszFuncName;
	FARPROC * ppFunc;
} functbl[] = {
	{ FUNC_PREFIXA "OpenArchive", (FARPROC *)&lpUnLhaOpenArchive },
	{ FUNC_PREFIXA "FindFirst", (FARPROC *)&lpUnLhaFindFirst },
	{ FUNC_PREFIXA "FindNext", (FARPROC *)&lpUnLhaFindNext },
	{ FUNC_PREFIXA "ExtractMem", (FARPROC *)&lpUnLhaExtractMem },
	{ FUNC_PREFIXA "CloseArchive", (FARPROC *)&lpUnLhaCloseArchive },
#ifdef UNICODE
	{ FUNC_PREFIXA "SetUnicodeMode", (FARPROC *)&lpUnLhaSetUnicodeMode },
#endif
#ifdef _WIN64
	{ FUNC_PREFIXA "Exists7zdll", (FARPROC *)&lpUnLhaExists7zdll },
#endif
	{ 0, (FARPROC *)0 }
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

static BOOL CALLBACK IsArchiveExt(LPTSTR pszExt){
	if(lstrcmpi(pszExt, TEXT("7z"))==0) return TRUE;
#ifdef _WIN64
	if(lstrcmpi(pszExt, TEXT("zip"))==0) return TRUE;
	if (!bExists7zdll) return FALSE;
	if(lstrcmpi(pszExt, TEXT("lzh"))==0) return TRUE;
	if(lstrcmpi(pszExt, TEXT("arj"))==0) return TRUE;
#endif
	return FALSE;
}

static LPTSTR CALLBACK CheckArchivePath(LPTSTR pszFilePath){
	LPTSTR p;
	p = StrStrI(pszFilePath, TEXT(".7z/"));
#ifdef _WIN64
	if (p) return p;
	p = StrStrI(pszFilePath, TEXT(".zip/"));
	if (p) return p;
	if (!bExists7zdll) return p;
	p = StrStrI(pszFilePath, TEXT(".lzh/"));
	if (p) return p;
	p = StrStrI(pszFilePath, TEXT(".arj/"));
#endif
	return p;
}

static BOOL CALLBACK EnumArchive(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData){
	INDIVIDUALINFOA iinfo;
	HARC hArc;
	// �A�[�J�C�u���I�[�v��
#ifdef UNICODE
	CHAR nameA[MAX_PATH * 3 + 1];
	WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, nameA, MAX_PATH + 1, NULL, NULL);
	hArc = lpUnLhaOpenArchive(NULL, nameA, M_CHECK_FILENAME_ONLY);
#else
	hArc = lpUnLhaOpenArchive(NULL, pszFilePath, M_CHECK_FILENAME_ONLY);
#endif
	if(!hArc){
		return FALSE;
	}
	// �����J�n
	if(lpUnLhaFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
			FILETIME ft;
			DosDateTimeToFileTime(iinfo.wDate, iinfo.wTime, &ft);
#ifdef UNICODE
			{
				WCHAR nameW[MAX_PATH + 1];
				MultiByteToWideChar(CP_UTF8, 0, iinfo.szFileName, -1, nameW, MAX_PATH + 1);
				lpfnProc(nameW, iinfo.dwOriginalSize, ft, pData);
			}
#else
			lpfnProc(iinfo.szFileName, iinfo.dwOriginalSize, ft, pData);
#endif
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}

	lpUnLhaCloseArchive(hArc);
	return TRUE;
}

static BOOL CALLBACK ExtractArchive(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize){
	INDIVIDUALINFOA iinfo;
	HARC hArc;
	DWORD dwOutputSize;
	DWORD dwBufferSize;

	// �A�[�J�C�u���I�[�v��
#ifdef UNICODE
	CHAR nameA[MAX_PATH * 3 + 1];
	WideCharToMultiByte(CP_UTF8, 0, pszArchivePath, -1, nameA, MAX_PATH + 1, NULL, NULL);
	hArc = lpUnLhaOpenArchive(NULL, nameA, M_CHECK_FILENAME_ONLY);
#else
	hArc = lpUnLhaOpenArchive(NULL, pszArchivePath, M_CHECK_FILENAME_ONLY);
#endif
	if(!hArc){
		return FALSE;
	}
	// �����J�n
	if(lpUnLhaFindFirst(hArc, "*.*", &iinfo)!=-1){
		do{
#ifdef UNICODE
			WCHAR nameW[MAX_PATH + 1];
			MultiByteToWideChar(CP_UTF8, 0, iinfo.szFileName, -1, nameW, MAX_PATH + 1);
			if(!lstrcmpi(nameW, pszFileName)) break;
#else
			if(!lstrcmpi(iinfo.szFileName, pszFileName)) break;
#endif
		}while(lpUnLhaFindNext(hArc, &iinfo)!=-1);
	}
	lpUnLhaCloseArchive(hArc);
	
	// ��
	if (iinfo.dwOriginalSize != 0){
		dwBufferSize = iinfo.dwOriginalSize;
#if 1
	} else if (iinfo.dwCompressedSize != 0){
		/* ���k��50%������ */
		dwBufferSize = iinfo.dwCompressedSize * 2;
	} else {
		HANDLE h = CreateFile(pszArchivePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h == INVALID_HANDLE_VALUE) return FALSE;
		dwBufferSize = GetFileSize(h, NULL) * 2;
		CloseHandle(h);
#else
	} else {
		dwBufferSize = 0;
#endif
	}
	if (dwBufferSize == 0) return FALSE;
	*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufferSize);
	while (*ppBuf){
		CHAR cmd[MAX_PATH*2*2];
		int ret;
#ifdef UNICODE
		wsprintfA(cmd, "e -hide \"%s\" \"%s\"", nameA, iinfo.szFileName);
#else
		wsprintfA(cmd, "e -hide \"%s\" \"%s\"", pszArchivePath, iinfo.szFileName);
#endif
		ret = lpUnLhaExtractMem(NULL, cmd, (LPBYTE)*ppBuf, dwBufferSize, NULL, NULL, &dwOutputSize);
#if 1
		/* SevenZipExtractMem �ł� dwOutputSize ���X�V����Ȃ��\��������  */
		if (iinfo.dwOriginalSize == 0 && dwOutputSize != 0 && dwBufferSize == dwOutputSize){
			/* �o�b�t�@������Ȃ����Ȃ�Ď��s */
			HeapFree(GetProcessHeap(), 0, *ppBuf);
			dwBufferSize += dwBufferSize;
			*ppBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufferSize);
			continue;
		}
#endif
		if(!ret){
			*pSize = (iinfo.dwOriginalSize != 0) ? iinfo.dwOriginalSize : (dwOutputSize != 0) ? dwOutputSize : dwBufferSize;
			return TRUE;
		}
#ifdef _DEBUG
		{
			TCHAR erx[64];
			wsprintf(erx,TEXT("%08x"),ret);
			MessageBox(NULL,erx,pszArchivePath,MB_OK);
		}
#endif
		HeapFree(GetProcessHeap(), 0, *ppBuf);
		return FALSE;
	}
	return FALSE;

}

static ARCHIVE_PLUGIN_INFO apinfo = {
	0,
	APDK_VER,
	IsArchiveExt,
	CheckArchivePath,
	EnumArchive,
	ExtractArchive
};

static BOOL InitArchive(){	
	const struct IMPORT_FUNC_TABLE *pft;
	if (!hUnlha32) hUnlha32 = LoadLibrary(szDllName);
	if (!hUnlha32) {
		TCHAR szDllPath[MAX_PATH];
		GetModuleFileName(hDLL, szDllPath, MAX_PATH);
		lstrcpy(PathFindFileName(szDllPath), szDllName);
		hUnlha32 = LoadLibrary(szDllPath);
		if (!hUnlha32) return FALSE;
	}
	for (pft = functbl; pft->lpszFuncName; pft++){
		FARPROC fp = GetProcAddress(hUnlha32, pft->lpszFuncName);
#ifdef _WIN64
		if (pft->ppFunc == (FARPROC *)&lpUnLhaExists7zdll)
		{
			if (!fp){
				bExists7zdll = FALSE;
				continue;
			}
			lpUnLhaExists7zdll = (LPUNLHAEXISTS7ZDLL)fp;
			if (lpUnLhaExists7zdll()){
				bExists7zdll = TRUE;
				continue;
			} else {
				fp = NULL;
			}
		}
#endif
		if (!fp){
			FreeLibrary(hUnlha32);
			hUnlha32 = NULL;
			return FALSE;
		}
		*pft->ppFunc = fp;
	}
#ifdef UNICODE
	lpUnLhaSetUnicodeMode(TRUE);
#endif
	return TRUE;
}

#ifdef __cplusplus
extern "C"
#endif
ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void){
	return InitArchive() ? &apinfo : 0;
}
