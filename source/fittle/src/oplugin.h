/*
 * oplugin.h
 *
 */

#include <windows.h>

/* OPDKのバージョン */
#ifndef OPDK_VER
#define OPDK_VER 0
#elif OPDK_VER > 0
#error "Incorrect OPDK version."
#endif

typedef enum {
	OUTPUT_PLUGIN_STATUS_STOP = 0,
	OUTPUT_PLUGIN_STATUS_PLAY = 1,
	OUTPUT_PLUGIN_STATUS_PAUSE = 3
} OUTPUT_PLUGIN_STATUS;

typedef struct OUTPUT_PLUGIN_INFO_TAG {
	int nOPDKVer;

	/* v0 */
	int (CALLBACK * GetDeviceCount)(void);
	DWORD (CALLBACK * GetDefaultDeviceID)(void);
	DWORD (CALLBACK * GetDeviceID)(int nIndex);
	BOOL (CALLBACK * GetDeviceNameA)(DWORD dwID, LPSTR szBuf, int nBufSize);
	int (CALLBACK * Init)(DWORD dwID);
	void (CALLBACK * Term)(void);
	BOOL (CALLBACK * Setup)(HWND hWnd);
	int (CALLBACK * GetStatus)(void);

	void (CALLBACK * Start)(void *pchinfo, float sVolume, BOOL fFloat);
	void (CALLBACK * End)(void);
	void (CALLBACK * Play)(void);
	void (CALLBACK * Pause)(void);
	void (CALLBACK * Stop)(void);
	void (CALLBACK * SetVolume)(float sVolume);
	void (CALLBACK * FadeIn)(float sVolume, DWORD dwTime);
	void (CALLBACK * FadeOut)(DWORD dwTime);
	BOOL (CALLBACK * IsSupportFloatOutput)(void);

	HWND hWnd;
	BOOL (CALLBACK * IsEndCue)(void);
	void (CALLBACK * PlayNext)(HWND hWnd);
	DWORD (CALLBACK * GetDecodeChannel)(float *pAmp);

#if OPDK_VER >= 1
#endif
/*
	int (CALLBACK * GetRate)(void);
*/
} OUTPUT_PLUGIN_INFO;

/* インターフェースの宣言 */
typedef OUTPUT_PLUGIN_INFO *(CALLBACK *GetOPluginInfoFunc)();
