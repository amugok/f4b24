/*
 * aplugin.h
 *
 */

#include <windows.h>

/* APDKのバージョン */
#define APDK_VER 1

typedef void (CALLBACK * LPFNARCHIVEENUMPROC)(char *pszFileName, DWORD dwSize, FILETIME ft, void *pData);

typedef void (CALLBACK * LPFNADDLISTPROC)(char *pszFileName, char *pszSize, char *pszTime, void *pData);
typedef BOOL (CALLBACK * LPFNCHECKFILETYPEPROC)(char *pszFileName);

/* 構造体宣言 */
typedef struct ARCHIVE_PLUGIN_INFO_TAG {
	struct ARCHIVE_PLUGIN_INFO_TAG *pNext;
	int nAPDKVer;

	/* v0 */
	BOOL (CALLBACK * IsArchiveExt)(char *pszExt);
	char *(CALLBACK * CheckArchivePath)(char *pszFilePath);
	BOOL (CALLBACK * EnumArchive)(char *pszFilePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData);
	BOOL (CALLBACK * ExtractArchive)(char *pszArchivePath, char *pszFileName, void **ppBuf, DWORD *pSize);

	/* v1 */
	HWND hwndMain;
	HMODULE hmodPlugin;

	/* v2 */
	BOOL (CALLBACK * EnumArchive2)(char *pszFilePath, LPFNADDLISTPROC lpfnAddListProc, LPFNCHECKFILETYPEPROC lpfnCheckProc, void *pData);
	BOOL (CALLBACK * ResolveIndirect)(char *pszArchivePath, char *pszTrackPart, char *pszStart, char *pszEnd);
	BOOL (CALLBACK * GetBasicTag)(char *pszArchivePath, char *pszTrackPart, char *pszTrack, char *pszTitle, char *pszAlbum, char *pszArtist);
	BOOL (CALLBACK * GetItemType)(char *pszArchivePath, char *pszTrackPart, char *pBuf, int nBufMax);
	char * (CALLBACK * GetItemFileName)(char *pszArchivePath, char *pszTrackPart);

} ARCHIVE_PLUGIN_INFO;

/* インターフェースの宣言 */
typedef ARCHIVE_PLUGIN_INFO *(CALLBACK *GetAPluginInfoFunc)();
