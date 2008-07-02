#ifndef WASTR_H_INC_
#define WASTR_H_INC_

typedef union WASTR_TAG_ {
	WCHAR W[WA_MAX_SIZE];
	CHAR A[WA_MAX_SIZE];
} WASTR, *LPWASTR;
typedef const WASTR CWASTR, *LPCWASTR;

static int WAstrlen(LPCWASTR pValue){
	if (WAIsUnicode)
		return lstrlenW(pValue->W);
	else
		return lstrlenA(pValue->A);
}

static int WAstrcmp(LPCWASTR pValue1, LPCWASTR pValue2){
	if (WAIsUnicode)
		return lstrcmpW(pValue1->W, pValue2->W);
	else
		return lstrcmpA(pValue1->A, pValue2->A);
}

static void WAstrcpy(LPWASTR pBuf, LPCWASTR pValue){
	if (WAIsUnicode)
		lstrcpynW(pBuf->W, pValue->W, WA_MAX_SIZE);
	else
		lstrcpynA(pBuf->A, pValue->A, WA_MAX_SIZE);
}

static void WAstrcpyonA(LPWASTR pBuf, int o, LPCSTR pValue, int n){
	if (WAIsUnicode)
		MultiByteToWideChar(CP_ACP, 0, pValue, -1, pBuf->W + o, n);
	else
		lstrcpynA(pBuf->A + o, pValue, n);
}

static void WAstrcpyA(LPWASTR pBuf, LPCSTR pValue){
	WAstrcpyonA(pBuf, 0, pValue, WA_MAX_SIZE);
}

static void WAstrcatA(LPWASTR pBuf, LPCSTR pValue){
	WASTR work;
	int l = WAstrlen(pBuf);
	WAstrcpyonA(pBuf, l, pValue, WA_MAX_SIZE - l);
}

static void WAGetModuleParentDir(HMODULE h, LPWASTR pBuf){
	if (WAIsUnicode) {
		GetModuleFileNameW(h, pBuf->W, WA_MAX_SIZE);
		*PathFindFileNameW(pBuf->W) = L'\0';
	}else{
		GetModuleFileNameA(h, pBuf->A, WA_MAX_SIZE);
		*PathFindFileNameA(pBuf->A) = '\0';
	}
}
static WASTR m_IniPath;
static void WASetIniFile(HMODULE h, LPCSTR pFilename){
	WAGetModuleParentDir(h, &m_IniPath);
	WAstrcatA(&m_IniPath,pFilename);
}
static void WAGetIniStr(LPCSTR pSec, LPCSTR pKey, LPWASTR pBuf){
	WASTR sec, key;
	WAstrcpyA(&sec, pSec);
	WAstrcpyA(&key, pKey);
	if (WAIsUnicode)
		GetPrivateProfileStringW(sec.W, key.W, L"", pBuf->W, WA_MAX_SIZE, m_IniPath.W);
	else
		GetPrivateProfileStringA(sec.A, key.A, "", pBuf->A, WA_MAX_SIZE, m_IniPath.A);
}
static int WAGetIniInt(LPCSTR pSec, LPCSTR pKey, int nDefault){
	WASTR sec, key;
	WAstrcpyA(&sec, pSec);
	WAstrcpyA(&key, pKey);
	if (WAIsUnicode)
		return (int)GetPrivateProfileIntW(sec.W, key.W, nDefault, m_IniPath.W);
	else
		return (int)GetPrivateProfileIntA(sec.A, key.A, nDefault, m_IniPath.A);
}
static void WASetIniStr(LPCSTR pSec, LPCSTR pKey, LPCWASTR pValue){
	WASTR sec, key;
	WAstrcpyA(&sec, pSec);
	WAstrcpyA(&key, pKey);
	if (WAIsUnicode)
		WritePrivateProfileStringW(sec.W, key.W, pValue->W, m_IniPath.W);
	else
		WritePrivateProfileStringA(sec.A, key.A, pValue->A, m_IniPath.A);
}
static void WASetIniInt(LPCSTR pSec, LPCSTR pKey, int iValue){
	CHAR cnvA[12];
	WASTR cnv;
	wsprintfA(cnvA, "%d", iValue);
	WAstrcpyA(&cnv, cnvA);
	WASetIniStr(pSec, pKey, &cnv);
}
static void WAFlushIni(){
	if (WAIsUnicode)
		WritePrivateProfileStringW(NULL, NULL, NULL, m_IniPath.W);
	else
		WritePrivateProfileStringA(NULL, NULL, NULL, m_IniPath.A);
}
static void WASetWindowText(HWND hWnd, LPCWASTR pValue){
	if (WAIsUnicode)
		SendMessageW((HWND)hWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)pValue->W);
	else
		SendMessageA((HWND)hWnd, WM_SETTEXT, (WPARAM)0, (LPARAM)pValue->A);
}
static void WASetDlgItemText(HWND hDlg, int iItemID, LPCWASTR pValue){
	WASetWindowText(GetDlgItem(hDlg, iItemID), pValue);
}
static void WAGetWindowText(HWND hWnd, LPWASTR pBuf){
	if (WAIsUnicode)
		SendMessageW((HWND)hWnd, WM_GETTEXT, (WPARAM)WA_MAX_SIZE, (LPARAM)pBuf->W);
	else
		SendMessageA((HWND)hWnd, WM_GETTEXT, (WPARAM)WA_MAX_SIZE, (LPARAM)pBuf->A);
}
static void WAGetDlgItemText(HWND hDlg, int iItemID, LPWASTR pBuf){
	WAGetWindowText(GetDlgItem(hDlg, iItemID), pBuf);
}

#endif
