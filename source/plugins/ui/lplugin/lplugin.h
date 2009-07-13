/*
 * lplugin.h
 *
 */

#include <windows.h>

/* LPDK�̃o�[�W���� */
#ifndef LPDK_VER
#define LPDK_VER 3
#elif LPDK_VER > 3
#error "Incorrect LPDK version."
#endif

/* �\���̐錾 */
typedef struct LX_PLUGIN_INFO_TAG {
	int nLPDKVer;

	/* v0 */
	int (CALLBACK * GetTypeNum)();
	int (CALLBACK * GetTypeCode)(int nIndex);
	LPCSTR (CALLBACK * GetTypeName)(int nIndex);

#if LPDK_VER >= 1
	/* v1 */

	BOOL (CALLBACK * IsSupported)(int nType);
	BOOL (CALLBACK *InitColumnOrder)(int nColumn, int nType);
	void (CALLBACK *AddColumn)(HWND hList, int nColumn, int nType);
	void (CALLBACK *GetColumnText)(LPVOID pFileInfo, int nRow, int nColumn, int nType, LPVOID pBuf, int nBufSize);
	int (CALLBACK *CompareColumnText)(LPVOID pFileInfoLeft, LPVOID pFileInfoRight, int nColumn, int nType);
	void (CALLBACK *OnSave)(HWND hList, int nColumn, int nType, int nWidth);

	HWND hWndMain;
	HMODULE hmodPlugin;
	F4B24LX_INTERFACE *plxif;

	LPVOID (CALLBACK *GetUserData)(LPVOID pFileInfo, LPVOID pGuid);
	void (CALLBACK *SetUserData)(LPVOID pFileInfo, LPVOID pGuid, LPVOID pUserData, void (CALLBACK *FreeProc)(LPVOID));
	int (CALLBACK * GetIniInt)(LPCSTR pSec, LPCSTR pKey, int nDefault);
	void (CALLBACK * SetIniInt)(LPCSTR pSec, LPCSTR pKey, int nValue);
#endif
} LX_PLUGIN_INFO;

/* �C���^�[�t�F�[�X�̐錾 */
typedef LX_PLUGIN_INFO *(CALLBACK *GetLXPluginInfoFunc)();

struct FILEINFOA{
	CHAR szFilePath[MAX_FITTLE_PATH];	// �t�@�C���p�X
	CHAR szSize[50];			// �T�C�Y
	CHAR szTime[50];			// �X�V����
	BOOL bPlayFlag;				// �Đ��ς݃`�F�b�N�t���O
	struct FILEINFOA *pNext;		// ���̃��X�g�ւ̃|�C���^
	LPVOID userdata;
};

struct FILEINFOW{
	WCHAR szFilePath[MAX_FITTLE_PATH];	// �t�@�C���p�X
	WCHAR szSize[50];			// �T�C�Y
	WCHAR szTime[50];			// �X�V����
	BOOL bPlayFlag;				// �Đ��ς݃`�F�b�N�t���O
	struct FILEINFOW *pNext;		// ���̃��X�g�ւ̃|�C���^
	LPVOID userdata;
};

typedef struct{
	CHAR szTitle[256];
	CHAR szArtist[256];
	CHAR szAlbum[256];
	CHAR szTrack[10];
	DWORD dwLength;
}TAGINFOA;

typedef struct{
	WCHAR szTitle[256];
	WCHAR szArtist[256];
	WCHAR szAlbum[256];
	WCHAR szTrack[10];
	DWORD dwLength;
}TAGINFOW;



