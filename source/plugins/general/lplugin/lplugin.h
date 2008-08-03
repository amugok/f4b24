/*
 * lplugin.h
 *
 */

#include <windows.h>

/* LPDKのバージョン */
#ifndef LPDK_VER
#define LPDK_VER 1
#elif APDK_VER > 1
#error "Incorrect LPDK version."
#endif

/* 構造体宣言 */
typedef struct LX_PLUGIN_INFO_TAG {
	struct LX_PLUGIN_INFO_TAG *pNext;
	int nLPDKVer;

	/* v0 */
	int (CALLBACK * GetTypeNum)(LPTSTR pszExt);
	int (CALLBACK * GetTypeCode)(int nIndex);
	LPCWSTR (CALLBACK * GetTypeName)(int nIndex);

#if LPDK_VER >= 1
	/* v1 */

	BOOL (CALLBACK *InitColumnOrder)(int nColumn, int nType);
	void (CALLBACK *AddColumn)(HWND hList, int nColumn, int nType);
	void (CALLBACK *GetColumnText)(LPVOID pFileInfo, int nRow, int nColumn, int nType, LPVOID pBuf, int nBufSize);
	int (CALLBACK *CompareColumnText)(LPVOID pFileInfoLeft, LPVOID pFileInfoRight, int nColumn, int nType);
	void (CALLBACK *OnSave)();

	HWND hwndMain;
	HMODULE hmodPlugin;
	F4B24LX_INTERFACE *plxif;

	LPVOID (CALLBACK *GetUserData)(LPVOID pFileInfo, LPVOID pGuid);
	void (CALLBACK *SetUserData)(LPVOID pFileInfo, LPVOID pGuid, LPVOID pUserDaga);
#endif
} LX_PLUGIN_INFO;

/* インターフェースの宣言 */
typedef LX_PLUGIN_INFO *(CALLBACK *GetLXPluginInfoFunc)();
