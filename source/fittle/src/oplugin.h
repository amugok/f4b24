/*
 * oplugin.h
 *
 */

#include <windows.h>

/* OPDK�̃o�[�W���� */
#ifndef OPDK_VER
#define OPDK_VER 0
#elif OPDK_VER > 0
#error "Incorrect OPDK version."
#endif

typedef struct OUTPUT_PLUGIN_INFO_TAG {
	struct OUTPUT_PLUGIN_INFO_TAG *pNext;
	int nOPDKVer;

	DWORD dwOutputID;

	/* v0 */
	int (CALLBACK * GetDeviceCount)(void);
	DWORD (CALLBACK * GetDefaultDeviceID)(void);
	DWORD (CALLBACK * GetDeviceID)(int nIndex);
	BOOL (CALLBACK * GetDeviceNameA)(DWORD dwID, LPSTR szBuf, int nBufSize);
	int (CALLBACK * Init)(DWORD dwID);

#if OPDK_VER >= 1
#endif
} OUTPUT_PLUGIN_INFO;

/* �C���^�[�t�F�[�X�̐錾 */
typedef OUTPUT_PLUGIN_INFO *(CALLBACK *GetOPluginInfoFunc)();
