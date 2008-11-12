/* f4b24“à•”Šg’£IF */

#define F4B24LX_INTERFACE_VERSION 39
enum {
	F4B24LX_CHECK_URL = 0,
	F4B24LX_CHECK_CUE = 1,
	F4B24LX_CHECK_ARC = 2
};
typedef struct {
	LONG nAccessRight;
	int nVersion;
	int nUnicode;

	LPVOID (CALLBACK *LoadMusic)(LPVOID lpszPath);
	void (CALLBACK *FreeMusic)(LPVOID pMusic);
	BOOL (CALLBACK *GetTag)(LPVOID pMusic, LPVOID pTagInfo);
	void (CALLBACK *AddColumn)(HWND hList, int nColumn, LPVOID pLabel, int nWidth, int nFmt);
	LPVOID (CALLBACK *GetFileName)(LPVOID pFileInfo);
	BOOL (CALLBACK *CheckPath)(LPVOID pFileInfo, int nCheck);
	int (CALLBACK *StrCmp)(LPCVOID pStrLeft, LPCVOID pStrRight);

	void (CALLBACK *HookOnAlloc)(LPVOID pFileInfo);
	void (CALLBACK *HookOnFree)(LPVOID pFileInfo);
	BOOL (CALLBACK *HookInitColumnOrder)(int nColumn, int nType);
	void (CALLBACK *HookAddColumn)(HWND hList, int nColumn, int nType);
	void (CALLBACK *HookGetColumnText)(LPVOID pFileInfo, int nRow, int nColumn, int nType, LPVOID pBuf, int nBufSize);
	int (CALLBACK *HookCompareColumnText)(LPVOID pFileInfoLeft, LPVOID pFileInfoRight, int nColumn, int nType);
	void (CALLBACK *HookOnSave)(HWND hList, int nColumn, int nType, int nWidth);
} F4B24LX_INTERFACE;

