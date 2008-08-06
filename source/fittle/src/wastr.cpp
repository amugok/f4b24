#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <prsht.h>

#include "wastr.h"

static BOOL m_WAIsUnicode = FALSE;
static WASTR m_IniPath;

BOOL WAIsUnicode(void){
	m_WAIsUnicode = ((GetVersion() & 0x80000000) == 0);
	return m_WAIsUnicode;
}

BOOL WAFILE_EXIST(LPCWASTR pValue) {
	return 0xFFFFFFFF != (m_WAIsUnicode ? GetFileAttributesW(pValue->W) : GetFileAttributesA(pValue->A)); 
}

int WAstrlen(LPCWASTR pValue){
	if (m_WAIsUnicode)
		return lstrlenW(pValue->W);
	else
		return lstrlenA(pValue->A);
}

int WAstrcmp(LPCWASTR pValue1, LPCWASTR pValue2){
	if (m_WAIsUnicode)
		return lstrcmpW(pValue1->W, pValue2->W);
	else
		return lstrcmpA(pValue1->A, pValue2->A);
}

void WAstrcpy(LPWASTR pBuf, LPCWASTR pValue){
	if (m_WAIsUnicode)
		lstrcpynW(pBuf->W, pValue->W, WA_MAX_SIZE);
	else
		lstrcpynA(pBuf->A, pValue->A, WA_MAX_SIZE);
}
void WAstrcpyt(LPTSTR pszBuf, LPCWASTR pValue, int n){
#ifdef UNICODE
	if (m_WAIsUnicode)
		lstrcpynW(pszBuf, pValue->W, n);
	else
		MultiByteToWideChar(CP_ACP, 0, pValue->A, -1, pszBuf, n);
#else
	if (m_WAIsUnicode)
		WideCharToMultiByte(CP_ACP, 0, pValue->W, -1, pszBuf, n, 0, 0);
	else
		lstrcpynA(pszBuf, pValue->A, n);
#endif
}

void WAstrcpyT(LPWASTR pBuf, LPCTSTR pszValue){
#ifdef UNICODE
	if (m_WAIsUnicode)
		lstrcpynW(pBuf->W, pszValue, WA_MAX_SIZE);
	else
		WideCharToMultiByte(CP_ACP, 0, pszValue, -1, pBuf->A, WA_MAX_SIZE, 0, 0);
#else
	if (m_WAIsUnicode)
		MultiByteToWideChar(CP_ACP, 0, pszValue, -1, pBuf->W, WA_MAX_SIZE);
	else
		lstrcpynA(pBuf->A, pszValue, WA_MAX_SIZE);
#endif
}


static void WAstrcpyonA(LPWASTR pBuf, int o, LPCSTR pValue, int n){
	if (m_WAIsUnicode)
		MultiByteToWideChar(CP_ACP, 0, pValue, -1, pBuf->W + o, n);
	else
		lstrcpynA(pBuf->A + o, pValue, n);
}

void WAstrcpyA(LPWASTR pBuf, LPCSTR pValue){
	WAstrcpyonA(pBuf, 0, pValue, WA_MAX_SIZE);
}

static void WAstrcatWX(LPWASTR pBuf, LPCWSTR pValue){
	int l = lstrlenW(pBuf->W);
	lstrcpynW(pBuf->W + l, pValue, WA_MAX_SIZE - l);
}

void WAstrcatA(LPWASTR pBuf, LPCSTR pValue){
	int l = WAstrlen(pBuf);
	WAstrcpyonA(pBuf, l, pValue, WA_MAX_SIZE - l);
}

void WAstrcat(LPWASTR pBuf, LPCWASTR pValue){
	if (m_WAIsUnicode)
		WAstrcatWX(pBuf, pValue->W);
	else
		WAstrcatA(pBuf, pValue->A);
}

int WAStrStrI(LPCWASTR pBuf, LPCWASTR pPtn){
	if (m_WAIsUnicode){
		LPWSTR pMatch = StrStrIW(pBuf->W, pPtn->W);
		return pMatch ? pMatch - pBuf->W + 1: 0;
	}else{
		LPSTR pMatch = StrStrIA(pBuf->A, pPtn->A);
		return pMatch ? pMatch - pBuf->A + 1: 0;
	}
}

void WAAddBackslash(LPWASTR pBuf){
	int l = WAstrlen(pBuf) - 1;
	if (l >= 0 && (m_WAIsUnicode ? pBuf->W[l] != L'\\' : pBuf->A[l] != '\\'))
		WAstrcatA(pBuf, "\\");
}

BOOL WAIsUNCPath(LPCWASTR pPath){
	return (m_WAIsUnicode) ?
		(pPath->W[0] == L'\\' && pPath->W[1] == L'\\') :
		(pPath->A[0] == '\\' && pPath->A[1] == '\\');
}

BOOL WAIsDirectory(LPCWASTR pPath){
	return (m_WAIsUnicode) ?
		PathIsDirectoryW(pPath->W) :
		PathIsDirectoryA(pPath->A);
}

void WAGetFileName(LPWASTR pBuf, LPCWASTR pPath){
	if (m_WAIsUnicode)
		lstrcpynW(pBuf->W, PathFindFileNameW(pPath->W), WA_MAX_SIZE);
	else
		lstrcpynA(pBuf->A, PathFindFileNameA(pPath->A), WA_MAX_SIZE);
}

void WAGetModuleParentDir(HMODULE h, LPWASTR pBuf){
	if (m_WAIsUnicode) {
		GetModuleFileNameW(h, pBuf->W, WA_MAX_SIZE);
		*PathFindFileNameW(pBuf->W) = L'\0';
	}else{
		GetModuleFileNameA(h, pBuf->A, WA_MAX_SIZE);
		*PathFindFileNameA(pBuf->A) = '\0';
	}
}

void WAGetIniDir(HMODULE h, LPWASTR pBuf){
	WAGetModuleParentDir(NULL, pBuf);
	WAstrcatA(pBuf, "install.ini");
	switch (WAGetIniInt("Install", "IniLoc", 0)) {
	case 1:
		if (m_WAIsUnicode ? SHGetSpecialFolderPathW(NULL, pBuf->W, CSIDL_APPDATA, FALSE) : SHGetSpecialFolderPathA(NULL, pBuf->A, CSIDL_APPDATA, FALSE)){
			WAAddBackslash(pBuf);
			WAstrcatA(pBuf, "Fittle\\");
			return;
		}
		break;
	case 2:
		WAGetIniStr("Install", "IniPath", pBuf);
		if(WAstrlen(pBuf))
			return;
		break;
	}
	WAGetModuleParentDir(h, pBuf);
}

void WASetIniFile(HMODULE h, LPCSTR pFilename){
	WAGetIniDir(h, &m_IniPath);
	WAstrcatA(&m_IniPath, pFilename);
}

void WAGetIniStr(LPCSTR pSec, LPCSTR pKey, LPWASTR pBuf){
	WASTR sec, key;
	WAstrcpyA(&sec, pSec);
	WAstrcpyA(&key, pKey);
	if (m_WAIsUnicode)
		GetPrivateProfileStringW(sec.W, key.W, L"", pBuf->W, WA_MAX_SIZE, m_IniPath.W);
	else
		GetPrivateProfileStringA(sec.A, key.A, "", pBuf->A, WA_MAX_SIZE, m_IniPath.A);
}
int WAGetIniInt(LPCSTR pSec, LPCSTR pKey, int nDefault){
	WASTR sec, key;
	WAstrcpyA(&sec, pSec);
	WAstrcpyA(&key, pKey);
	if (m_WAIsUnicode)
		return (int)GetPrivateProfileIntW(sec.W, key.W, nDefault, m_IniPath.W);
	else
		return (int)GetPrivateProfileIntA(sec.A, key.A, nDefault, m_IniPath.A);
}
void WASetIniStr(LPCSTR pSec, LPCSTR pKey, LPCWASTR pValue){
	WASTR sec, key;
	WAstrcpyA(&sec, pSec);
	WAstrcpyA(&key, pKey);
	if (m_WAIsUnicode)
		WritePrivateProfileStringW(sec.W, key.W, pValue ? pValue->W : NULL, m_IniPath.W);
	else
		WritePrivateProfileStringA(sec.A, key.A, pValue ? pValue->A : NULL, m_IniPath.A);
}
void WASetIniInt(LPCSTR pSec, LPCSTR pKey, int iValue){
	CHAR cnvA[12];
	WASTR cnv;
	wsprintfA(cnvA, "%d", iValue);
	WAstrcpyA(&cnv, cnvA);
	WASetIniStr(pSec, pKey, &cnv);
}
void WAFlushIni(){
	if (m_WAIsUnicode)
		WritePrivateProfileStringW(NULL, NULL, NULL, m_IniPath.W);
	else
		WritePrivateProfileStringA(NULL, NULL, NULL, m_IniPath.A);
}
void WASetWindowText(HWND hWnd, LPCWASTR pValue){
	if (m_WAIsUnicode)
		SendMessageW((HWND)hWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)pValue->W);
	else
		SendMessageA((HWND)hWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)pValue->A);
}
void WASetDlgItemText(HWND hDlg, int iItemID, LPCWASTR pValue){
	WASetWindowText(GetDlgItem(hDlg, iItemID), pValue);
}
void WAGetWindowText(HWND hWnd, LPWASTR pBuf){
	if (m_WAIsUnicode)
		SendMessageW((HWND)hWnd, WM_GETTEXT, (WPARAM)WA_MAX_SIZE, (LPARAM)pBuf->W);
	else
		SendMessageA((HWND)hWnd, WM_GETTEXT, (WPARAM)WA_MAX_SIZE, (LPARAM)pBuf->A);
}
void WAGetDlgItemText(HWND hDlg, int iItemID, LPWASTR pBuf){
	WAGetWindowText(GetDlgItem(hDlg, iItemID), pBuf);
}

BOOL WAOpenFilerPath(LPWASTR pPath, HWND hWnd, LPCSTR pszMsg, LPCSTR pszFilter, LPCSTR pszDefExt){
	union {
		OPENFILENAMEA A;
		OPENFILENAMEW W;
	} ofn;
	int i;
	WASTR msg, filter, defext, file, title;
	WAstrcpyA(&msg, pszMsg);
	WAstrcpyA(&filter, pszFilter);
	WAstrcpyA(&defext, pszDefExt);
	WAstrcpyA(&file, "");
	WAstrcpyA(&title, "");

	ZeroMemory(&ofn, sizeof(ofn));
	if (m_WAIsUnicode) {
		for (i = 0; filter.W[i]; i++) if (filter.W[i] == L';') filter.W[i] = L'\0';
		ofn.W.lStructSize = sizeof(OPENFILENAMEW);
		ofn.W.hwndOwner = hWnd;
		ofn.W.lpstrFilter = filter.W;
		ofn.W.lpstrFile = file.W;
		ofn.W.lpstrFileTitle = title.W;
		ofn.W.lpstrInitialDir = pPath->W;
		ofn.W.nFilterIndex = 1;
		ofn.W.nMaxFile = WA_MAX_SIZE;
		ofn.W.nMaxFileTitle = WA_MAX_SIZE;
		ofn.W.Flags = OFN_HIDEREADONLY;
		ofn.W.lpstrDefExt = defext.W;
		ofn.W.lpstrTitle = msg.W;
		if(GetOpenFileNameW(&ofn.W)==0) return FALSE;
	} else {
		for (i = 0; filter.A[i]; i++) if (filter.A[i] == L';') filter.A[i] = L'\0';
		ofn.A.lStructSize = sizeof(OPENFILENAMEA);
		ofn.A.hwndOwner = hWnd;
		ofn.A.lpstrFilter = filter.A;
		ofn.A.lpstrFile = file.A;
		ofn.A.lpstrFileTitle = title.A;
		ofn.A.lpstrInitialDir = pPath->A;
		ofn.A.nFilterIndex = 1;
		ofn.A.nMaxFile = WA_MAX_SIZE;
		ofn.A.nMaxFileTitle = WA_MAX_SIZE;
		ofn.A.Flags = OFN_HIDEREADONLY;
		ofn.A.lpstrDefExt = defext.A;
		ofn.A.lpstrTitle = msg.A;
		if(GetOpenFileNameA(&ofn.A)==0) return FALSE;
	}

	WAstrcpy(pPath, &file);
	return TRUE;
}

BOOL WABrowseForFolder(LPWASTR pPath, HWND hWnd, LPCSTR pszMsg){
	union {
		BROWSEINFOA A;
		BROWSEINFOW W;
	} bi;
	WASTR msg;
	LPITEMIDLIST pidl;
	WAstrcpyA(&msg, pszMsg);

	ZeroMemory(&bi, sizeof(bi));
	if (m_WAIsUnicode) {
		bi.W.hwndOwner = hWnd;
		bi.W.lpszTitle = msg.W;
		pidl = SHBrowseForFolderW(&bi.W);
	} else {
		bi.A.hwndOwner = hWnd;
		bi.A.lpszTitle = msg.A;
		pidl = SHBrowseForFolderA(&bi.A);
	}
	if (!pidl) return FALSE;
	if (m_WAIsUnicode)
		SHGetPathFromIDListW(pidl, pPath->W);
	else
		SHGetPathFromIDListA(pidl, pPath->A);
	CoTaskMemFree(pidl);
	return TRUE;
}

BOOL WAChooseFont(LPWASTR pFontName, int *pFontHeight, int *pFontStyle, HWND hWnd){
	union {
		CHOOSEFONTW W;
		CHOOSEFONTA A;
	} cf;
	union {
		LOGFONTW W;
		LOGFONTA A;
	} lf;
	HDC hDC;

	hDC = GetDC(hWnd);
	if (!hDC) return FALSE;
	ZeroMemory(&lf, sizeof(lf));
	if (m_WAIsUnicode) {
		lstrcpynW(lf.W.lfFaceName, pFontName->W, LF_FACESIZE);
		lf.W.lfHeight = -MulDiv(*pFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		lf.W.lfItalic = (*pFontStyle&ITALIC_FONTTYPE?TRUE:FALSE);
		lf.W.lfWeight = (*pFontStyle&BOLD_FONTTYPE?FW_BOLD:0);
	} else {
		lstrcpynA(lf.A.lfFaceName, pFontName->A, LF_FACESIZE);
		lf.A.lfHeight = -MulDiv(*pFontHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		lf.A.lfItalic = (*pFontStyle&ITALIC_FONTTYPE?TRUE:FALSE);
		lf.A.lfWeight = (*pFontStyle&BOLD_FONTTYPE?FW_BOLD:0);
	}
	ReleaseDC(hWnd, hDC);

	ZeroMemory(&cf, sizeof(cf));
	if (m_WAIsUnicode) {
		lf.W.lfCharSet = SHIFTJIS_CHARSET;
		cf.W.lStructSize = sizeof(CHOOSEFONTW);
		cf.W.hwndOwner = hWnd;
		cf.W.lpLogFont = &lf.W;
		cf.W.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
		cf.W.nFontType = SCREEN_FONTTYPE;
		if(!ChooseFontW(&cf.W)) return FALSE;
		lstrcpynW(pFontName->W, lf.W.lfFaceName, LF_FACESIZE);
		*pFontStyle = cf.W.nFontType;
		*pFontHeight = cf.W.iPointSize / 10;
	} else {
		lf.A.lfCharSet = SHIFTJIS_CHARSET;
		cf.A.lStructSize = sizeof(CHOOSEFONTA);
		cf.A.hwndOwner = hWnd;
		cf.A.lpLogFont = &lf.A;
		cf.A.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOSCRIPTSEL;
		cf.A.nFontType = SCREEN_FONTTYPE;
		if(!ChooseFontA(&cf.A)) return FALSE;
		lstrcpynA(pFontName->A, lf.A.lfFaceName, LF_FACESIZE);
		*pFontStyle = cf.A.nFontType;
		*pFontHeight = cf.A.iPointSize / 10;
	}
	return TRUE;
}

#ifndef PROPSHEETPAGE_V1
#define PROPSHEETPAGEA_V1 PROPSHEETPAGEA
#define PROPSHEETPAGEW_V1 PROPSHEETPAGEW
#endif
HPROPSHEETPAGE WACreatePropertySheetPage(HMODULE hmod, LPCSTR lpszTemplate, DLGPROC lpfnDlgProc){
	union {
		PROPSHEETPAGEA A;
		PROPSHEETPAGEW W;
	} psp;
	WASTR dlgtemp;
	WAstrcpyA(&dlgtemp, lpszTemplate);
	if (m_WAIsUnicode) {
		psp.W.dwSize = sizeof (PROPSHEETPAGEW_V1);
		psp.W.dwFlags = PSP_DEFAULT;
		psp.W.hInstance = hmod;
		psp.W.pszTemplate = dlgtemp.W;
		psp.W.pfnDlgProc = (DLGPROC)lpfnDlgProc;
		return CreatePropertySheetPageW(&psp.W);
	} else {
		psp.A.dwSize = sizeof (PROPSHEETPAGEA_V1);
		psp.A.dwFlags = PSP_DEFAULT;
		psp.A.hInstance = hmod;
		psp.A.pszTemplate = dlgtemp.A;
		psp.A.pfnDlgProc = (DLGPROC)lpfnDlgProc;
		return CreatePropertySheetPageA(&psp.A);
	}
}

BOOL WAEnumFiles(HMODULE hParent, LPCSTR lpszSubDir, LPCSTR lpszMask, BOOL (CALLBACK * lpfnFileProc)(LPWASTR lpPath, LPVOID user), LPVOID user){
	WASTR szDir, szPath;
	union {
		WIN32_FIND_DATAA A;
		WIN32_FIND_DATAW W;
	} wfd;
	HANDLE hFind;
	BOOL fRet = FALSE;
	WAGetModuleParentDir(hParent, &szDir);
	WAstrcatA(&szDir, lpszSubDir);
	WAstrcpy(&szPath, &szDir);
	WAstrcatA(&szPath, lpszMask);

	hFind = m_WAIsUnicode ? FindFirstFileW(szPath.W, &wfd.W) : FindFirstFileA(szPath.A, &wfd.A);

	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			if (m_WAIsUnicode) {
				WAstrcpy(&szPath, &szDir);
				WAstrcatWX(&szPath, wfd.W.cFileName);
			}else{
				WAstrcpy(&szPath, &szDir);
				WAstrcatA(&szPath, wfd.A.cFileName);
			}
			if (lpfnFileProc(&szPath, user))
				fRet = TRUE;
		}while(m_WAIsUnicode ? FindNextFileW(hFind, &wfd.W) : FindNextFileA(hFind, &wfd.A));
		FindClose(hFind);
	}
	return fRet;
}

HMODULE WALoadLibrary(LPCWASTR lpPath) {
	return (m_WAIsUnicode) ? LoadLibraryW(lpPath->W) : LoadLibraryA(lpPath->A);
}

typedef struct {
	BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, LPVOID user);
	LPVOID user;
} ENUMPLUGINWORK, *LPENUMPLUGINWORK;
static BOOL CALLBACK WAEnumPluginsProc(LPWASTR lpPath, LPVOID user){
	LPENUMPLUGINWORK pwork = (LPENUMPLUGINWORK)user;
	HMODULE hPlugin = WALoadLibrary(lpPath);
	if(hPlugin) {
		if (pwork->lpfnPluginProc(hPlugin, pwork->user)){
			return TRUE;
		}
		FreeLibrary(hPlugin);
	}
	return FALSE;
}

BOOL WAEnumPlugins(HMODULE hParent, LPCSTR lpszSubDir, LPCSTR lpszMask, BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, LPVOID user), LPVOID user){
	ENUMPLUGINWORK work = { lpfnPluginProc, user };
	return WAEnumFiles(hParent, lpszSubDir, lpszMask, WAEnumPluginsProc, &work);
}

HANDLE WAOpenFile(LPCWASTR pPath){
	return (m_WAIsUnicode) ? \
		CreateFileW(pPath->W, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) : \
		CreateFileA(pPath->A, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

HANDLE WACreateFile(LPCWASTR pPath){
	return (m_WAIsUnicode) ? \
		CreateFileW(pPath->W, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) : \
		CreateFileA(pPath->A, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

void WADeleteFile(LPCWASTR pPath){
	if (m_WAIsUnicode)
		DeleteFileW(pPath->W);
	else
		DeleteFileA(pPath->A);
}

void WAShellExecute(HWND hWnd, LPCWASTR pFile, LPCWASTR pPara){
	if (m_WAIsUnicode)
		ShellExecuteW(hWnd, NULL, pFile->W, pPara ? pPara->W : NULL, NULL, SW_SHOWNORMAL);
	else
		ShellExecuteA(hWnd, NULL, pFile->A, pPara ? pPara->A : NULL, NULL, SW_SHOWNORMAL);
}

HWND WACreateStaticWindow(HMODULE hmod){
	if (m_WAIsUnicode)
		return CreateWindowW(L"STATIC",L"",0,0,0,0,0,NULL,NULL,hmod,NULL);
	else
		return CreateWindowA("STATIC","",0,0,0,0,0,NULL,NULL,hmod,NULL);
}


