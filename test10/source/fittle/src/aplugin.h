/*
 * aplugin.h
 *
 */

#include <windows.h>

/* APDK�̃o�[�W���� */
#define APDK_VER 1

typedef void (CALLBACK * LPFNARCHIVEENUMPROC)(char *pszFileName, DWORD dwSize, FILETIME ft, void *pData);

/* �\���̐錾 */
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

} ARCHIVE_PLUGIN_INFO;

/* �C���^�[�t�F�[�X�̐錾 */
typedef ARCHIVE_PLUGIN_INFO *(CALLBACK *GetAPluginInfoFunc)();
