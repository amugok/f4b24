/*
 * aplugin.h
 *
 */

#include <windows.h>

/* APDKのバージョン */
#define APDK_VER 1

typedef void (CALLBACK * LPFNARCHIVEENUMPROC)(LPTSTR pszFileName, DWORD dwSize, FILETIME ft, void *pData);

typedef void (CALLBACK * LPFNADDLISTPROC)(LPTSTR pszFileName, LPTSTR pszSize, LPTSTR pszTime, void *pData);
typedef BOOL (CALLBACK * LPFNCHECKFILETYPEPROC)(LPTSTR pszFileName);

/* 構造体宣言 */
typedef struct ARCHIVE_PLUGIN_INFO_TAG {
	struct ARCHIVE_PLUGIN_INFO_TAG *pNext;
	int nAPDKVer;

	/* v0 */
	BOOL (CALLBACK * IsArchiveExt)(LPTSTR pszExt);
	LPTSTR (CALLBACK * CheckArchivePath)(LPTSTR pszFilePath);
	BOOL (CALLBACK * EnumArchive)(LPTSTR pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData);
	BOOL (CALLBACK * ExtractArchive)(LPTSTR pszArchivePath, LPTSTR pszFileName, void **ppBuf, DWORD *pSize);

	/* v1 */
	HWND hwndMain;
	HMODULE hmodPlugin;

	/* v2 */
	BOOL (CALLBACK * EnumArchive2)(LPTSTR pszFilePath, LPFNADDLISTPROC lpfnAddListProc, LPFNCHECKFILETYPEPROC lpfnCheckProc, void *pData);
	BOOL (CALLBACK * ResolveIndirect)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszStart, LPTSTR pszEnd);
	BOOL (CALLBACK * GetBasicTag)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszTrack, LPTSTR pszTitle, LPTSTR pszAlbum, LPTSTR pszArtist);
	BOOL (CALLBACK * GetItemType)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pBuf, int nBufMax);
	LPTSTR (CALLBACK * GetItemFileName)(LPTSTR pszArchivePath, LPTSTR pszTrackPart);

} ARCHIVE_PLUGIN_INFO;

/* インターフェースの宣言 */
typedef ARCHIVE_PLUGIN_INFO *(CALLBACK *GetAPluginInfoFunc)();
