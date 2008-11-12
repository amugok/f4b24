/*
 * aplugin.h
 *
 */

#include <windows.h>

/* APDKのバージョン */
#ifndef APDK_VER
#define APDK_VER 1
#elif APDK_VER > 3
#error "Incorrect APDK version."
#endif

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

#if APDK_VER >= 1
	/* v1 */
	HWND hwndMain;
	HMODULE hmodPlugin;

#if APDK_VER >= 2
	/* v2 */
	BOOL (CALLBACK * EnumArchive2)(LPTSTR pszFilePath, LPFNADDLISTPROC lpfnAddListProc, LPFNCHECKFILETYPEPROC lpfnCheckProc, void *pData);
	BOOL (CALLBACK * ResolveIndirect)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszStart, LPTSTR pszEnd);
	BOOL (CALLBACK * GetBasicTag)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pszTrack, LPTSTR pszTitle, LPTSTR pszAlbum, LPTSTR pszArtist);
	BOOL (CALLBACK * GetItemType)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, LPTSTR pBuf, int nBufMax);
	LPTSTR (CALLBACK * GetItemFileName)(LPTSTR pszArchivePath, LPTSTR pszTrackPart);
#if APDK_VER >= 3
	/* v3 */
	BOOL (CALLBACK * GetGain)(LPTSTR pszArchivePath, LPTSTR pszTrackPart, float *pGain, DWORD hBass);
#endif
#endif
#endif
} ARCHIVE_PLUGIN_INFO;

/* インターフェースの宣言 */
typedef ARCHIVE_PLUGIN_INFO *(CALLBACK *GetAPluginInfoFunc)();
