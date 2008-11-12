#ifndef WASTR_H_INC_
#define WASTR_H_INC_

#ifndef WA_MAX_SIZE
#define WA_MAX_SIZE MAX_PATH
#endif

typedef union WASTR_TAG_ {
	WCHAR W[WA_MAX_SIZE];
	CHAR A[WA_MAX_SIZE];
} WASTR, *LPWASTR;
typedef const WASTR CWASTR, *LPCWASTR;

extern BOOL WAIsUnicode(void);
extern BOOL WAFILE_EXIST(LPCWASTR pValue);
extern int WAstrlen(LPCWASTR pValue);
extern int WAstrcmp(LPCWASTR pValue1, LPCWASTR pValue2);
extern void WAstrcpy(LPWASTR pBuf, LPCWASTR pValue);
extern void WAstrcpyt(LPTSTR pszBuf, LPCWASTR pValue, int n);
extern void WAstrcpyT(LPWASTR pBuf, LPCTSTR pszValue);
extern void WAstrcpyA(LPWASTR pBuf, LPCSTR pValue);
extern void WAstrcat(LPWASTR pBuf, LPCWASTR pValue);
extern void WAstrcatA(LPWASTR pBuf, LPCSTR pValue);
extern int WAStrStrI(LPCWASTR pBuf, LPCWASTR pPtn);
extern void WAAddBackslash(LPWASTR pBuf);
extern BOOL WAIsUNCPath(LPCWASTR pPath);
extern BOOL WAIsDirectory(LPCWASTR pPath);
extern void WAGetFileName(LPWASTR pBuf, LPCWASTR pPath);
extern void WAGetModuleParentDir(HMODULE h, LPWASTR pBuf);
extern void WAGetIniDir(HMODULE h, LPWASTR pBuf);
extern void WASetIniFile(HMODULE h, LPCSTR pFilename);
extern void WAGetIniStr(LPCSTR pSec, LPCSTR pKey, LPWASTR pBuf);
extern int WAGetIniInt(LPCSTR pSec, LPCSTR pKey, int nDefault);
extern void WASetIniStr(LPCSTR pSec, LPCSTR pKey, LPCWASTR pValue);
extern void WASetIniInt(LPCSTR pSec, LPCSTR pKey, int iValue);
extern void WAFlushIni();
extern void WASetWindowText(HWND hWnd, LPCWASTR pValue);
extern void WASetDlgItemText(HWND hDlg, int iItemID, LPCWASTR pValue);
extern void WAGetWindowText(HWND hWnd, LPWASTR pBuf);
extern void WAGetDlgItemText(HWND hDlg, int iItemID, LPWASTR pBuf);
extern BOOL WAOpenFilerPath(LPWASTR pPath, HWND hWnd, LPCSTR pszMsg, LPCSTR pszFilter, LPCSTR pszDefExt);
extern BOOL WABrowseForFolder(LPWASTR pPath, HWND hWnd, LPCSTR pszMsg);
extern BOOL WAChooseFont(LPWASTR pFontName, int *pFontHeight, int *pFontStyle, HWND hWnd);
#ifdef CreatePropertySheetPage
extern HPROPSHEETPAGE WACreatePropertySheetPage(HMODULE hmod, LPCSTR lpszTemplate, DLGPROC lpfnDlgProc);
#endif
extern BOOL WAEnumFiles(HMODULE hParent, LPCSTR lpszSubDir, LPCSTR lpszMask, BOOL (CALLBACK * lpfnFileProc)(LPWASTR lpPath, LPVOID user), LPVOID user);
extern HMODULE WALoadLibrary(LPCWASTR lpPath) ;
extern BOOL WAEnumPlugins(HMODULE hParent, LPCSTR lpszSubDir, LPCSTR lpszMask, BOOL (CALLBACK * lpfnPluginProc)(HMODULE hPlugin, LPVOID user), LPVOID user);

extern HANDLE WAOpenFile(LPCWASTR pPath);
extern HANDLE WACreateFile(LPCWASTR pPath);
extern void WADeleteFile(LPCWASTR pPath);
extern void WAShellExecute(HWND hWnd, LPCWASTR pFile, LPCWASTR pPara);
extern HWND WACreateStaticWindow(HMODULE hmod);

#endif
