/*
 * cplugin.h
 *
 */

#include <windows.h>

/* CPDKのバージョン */
#define CPDK_VER 0

/* 構造体宣言 */
typedef struct CONFIG_PLUGIN_INFO_TAG {
	struct CONFIG_PLUGIN_INFO_TAG *pNext;
	int nCPDKVer;

	DWORD (CALLBACK * GetConfigPageCount)(void);
	HPROPSHEETPAGE (CALLBACK * GetConfigPage)(int nIndex, int nLevel, char *pszConfigPath, int nConfigPathSize);

} CONFIG_PLUGIN_INFO;

/* インターフェースの宣言 */
typedef CONFIG_PLUGIN_INFO *(CALLBACK *GetCPluginInfoFunc)(void);
