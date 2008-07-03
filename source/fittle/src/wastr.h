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

static void WAstrcatWX(LPWASTR pBuf, LPCWSTR pValue){
	int l = lstrlenW(pBuf->W);
	lstrcpynW(pBuf->W + l, pValue, WA_MAX_SIZE - l);
}

static void WAstrcatA(LPWASTR pBuf, LPCSTR pValue){
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

#ifdef GetOpenFileName
static BOOL WAOpenFilerPath(LPWASTR pPath, HWND hWnd, LPCSTR pszMsg, LPCSTR pszFilter, LPCSTR pszDefExt){
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
	if (WAIsUnicode) {
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
#endif

#ifdef SHBrowseForFolder
static BOOL WABrowseForFolder(LPWASTR pPath, HWND hWnd, LPCSTR pszMsg){
	union {
		BROWSEINFOA A;
		BROWSEINFOW W;
	} bi;
	WASTR msg;
	LPITEMIDLIST pidl;
	WAstrcpyA(&msg, pszMsg);

	ZeroMemory(&bi, sizeof(bi));
	if (WAIsUnicode) {
		bi.W.hwndOwner = hWnd;
		bi.W.lpszTitle = msg.W;
		pidl = SHBrowseForFolderW(&bi.W);
	} else {
		bi.A.hwndOwner = hWnd;
		bi.A.lpszTitle = msg.A;
		pidl = SHBrowseForFolderA(&bi.A);
	}
	if (!pidl) return FALSE;
	if (WAIsUnicode)
		SHGetPathFromIDListW(pidl, pPath->W);
	else
		SHGetPathFromIDListA(pidl, pPath->A);
	CoTaskMemFree(pidl);
	return TRUE;
}
#endif

#ifdef ChooseFont
static BOOL WAChooseFont(LPWASTR pFontName, int *pFontHeight, int *pFontStyle, HWND hWnd){
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
	if (WAIsUnicode) {
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
	if (WAIsUnicode) {
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
#endif

#ifdef CreatePropertySheetPage
#ifndef PROPSHEETPAGE_V1
#define PROPSHEETPAGEA_V1 PROPSHEETPAGEA
#define PROPSHEETPAGEW_V1 PROPSHEETPAGEW
#endif
static HPROPSHEETPAGE WACreatePropertySheetPage(HMODULE hmod, LPCWASTR pTemplate, DLGPROC lpfnDlgProc){
	union {
		PROPSHEETPAGEA_V1 A;
		PROPSHEETPAGEW_V1 W;
	} psp;
	if (WAIsUnicode) {
		psp.W.dwSize = sizeof (PROPSHEETPAGEW_V1);
		psp.W.dwFlags = PSP_DEFAULT;
		psp.W.hInstance = hmod;
		psp.W.pszTemplate = pTemplate->W;
		psp.W.pfnDlgProc = (DLGPROC)lpfnDlgProc;
		return CreatePropertySheetPageW(&psp.W);
	} else {
		psp.A.dwSize = sizeof (PROPSHEETPAGEA_V1);
		psp.A.dwFlags = PSP_DEFAULT;
		psp.A.hInstance = hmod;
		psp.A.pszTemplate = pTemplate->A;
		psp.A.pfnDlgProc = (DLGPROC)lpfnDlgProc;
		return CreatePropertySheetPageA(&psp.A);
	}
}
#endif

typedef BOOL (CALLBACK * LPFNWAREGISTERPLUGIN)(HMODULE hPlugin);
static BOOL WAEnumPlugins(HMODULE hParent, LPCSTR lpszSubDir, LPCSTR lpszMask, LPFNWAREGISTERPLUGIN lpfnRegProc){
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

	hFind = WAIsUnicode ? FindFirstFileW(szPath.W, &wfd.W) : FindFirstFileA(szPath.A, &wfd.A);

	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			HMODULE hPlugin;
			if (WAIsUnicode) {
				WAstrcpy(&szPath, &szDir);
				WAstrcatWX(&szPath, wfd.W.cFileName);
				hPlugin = LoadLibraryW(szPath.W);
			}else{
				WAstrcpy(&szPath, &szDir);
				WAstrcatA(&szPath, wfd.A.cFileName);
				hPlugin = LoadLibraryA(szPath.A);
			}
			if(hPlugin) {
				if (lpfnRegProc(hPlugin)){
					fRet = TRUE;
				}else{
					FreeLibrary(hPlugin);
				}
			}
		}while(WAIsUnicode ? FindNextFileW(hFind, &wfd.W) : FindNextFileA(hFind, &wfd.A));
		FindClose(hFind);
	}
	return fRet;
}

#endif
