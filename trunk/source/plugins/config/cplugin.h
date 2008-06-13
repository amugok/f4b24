/*
 * cplugin.h
 *
 */

#include <windows.h>

/* CPDK�̃o�[�W���� */
#define CPDK_VER 0

/* �\���̐錾 */
typedef struct CONFIG_PLUGIN_INFO_TAG {
	struct CONFIG_PLUGIN_INFO_TAG *pNext;
	int nCPDKVer;

	DWORD (CALLBACK * GetConfigPageCount)(void);
	HPROPSHEETPAGE (CALLBACK * GetConfigPage)(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize);

} CONFIG_PLUGIN_INFO;

/* �C���^�[�t�F�[�X�̐錾 */
typedef CONFIG_PLUGIN_INFO *(CALLBACK *GetCPluginInfoFunc)(void);
