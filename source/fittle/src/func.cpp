/*
 * Func.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	�֗��Ȋ֐�(����܂�֗�����Ȃ�����)
//

#if defined(_MSC_VER) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#endif

#include "func.h"
#include "bass_tag.h"
#include "archive.h"

/* �t�@�C�����̃|�C���^���擾 */
LPTSTR GetFileName(LPTSTR szIn){
	LPTSTR p, q;

	p = q = szIn;
	if(IsURLPath(szIn)) return q;
	if(IsArchivePath(szIn)){
		LPTSTR r = GetArchiveItemFileName(szIn);
		if (r) return r;
	}
	while(*p){
#ifdef UNICODE
#else
		if(IsDBCSLeadByte(*p)){
			p += 2;
			continue;
		}
#endif
		if(*p==TEXT('\\') || *p==TEXT('/') /* || *p==TEXT('?')*/ ) q = p + 1;
		p++;
	}
	return q;
}

//�e�t�H���_���擾
int GetParentDir(LPCTSTR pszFilePath, LPTSTR pszParPath){
	TCHAR szLongPath[MAX_FITTLE_PATH] = {0};

	GetLongPathName(pszFilePath, szLongPath, MAX_FITTLE_PATH); //98�ȍ~�̂�
	if(!FILE_EXIST(pszFilePath)){
		//�t�@�C���A�f�B���N�g���ȊO
		return OTHERS;
	}else if(PathIsDirectory(pszFilePath)){
		//�f�B���N�g���������ꍇ
		lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
		return FOLDERS;
	}else{
		//�t�@�C���������ꍇ

		if(IsPlayListFast(szLongPath)){
			// ���X�g
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return LISTS;
		}else if(IsArchiveFast(szLongPath)){
			// �A�[�J�C�u
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			return ARCHIVES;
		}else{
			// ���y�t�@�C��
			lstrcpyn(pszParPath, szLongPath, MAX_FITTLE_PATH);
			*PathFindFileName(pszParPath) = TEXT('\0');
			return FILES;
		}
	}
}

/*�v���C���X�g���ǂ������f*/
BOOL IsPlayListFast(LPTSTR szFilePath){
	LPTSTR p;
	if((p = PathFindExtension(szFilePath)) == NULL || !*p) return FALSE;
	p++;
	if(lstrcmpi(p, TEXT("M3U"))==0 || lstrcmpi(p, TEXT("M3U8"))==0 || lstrcmpi(p, TEXT("PLS"))==0)
		return TRUE;
	else
		return FALSE;
}

/*�v���C���X�g���ǂ������f*/
BOOL IsPlayList(LPTSTR szFilePath){
	if(PathIsDirectory(szFilePath)) return FALSE;
	return IsPlayListFast(szFilePath);
}

void FormatDateTime(LPTSTR pszBuf, LPFILETIME pft){
	FILETIME lt;
	FileTimeToLocalFileTime(pft, &lt);
	FormatLocalDateTime(pszBuf, &lt);
}

void FormatLocalDateTime(LPTSTR pszBuf, LPFILETIME pft){
	SYSTEMTIME st;
	FileTimeToSystemTime(pft, &st);
	wsprintf(pszBuf, TEXT("%02d/%02d/%02d %02d:%02d:%02d"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

BOOL GetTimeAndSize(LPCTSTR pszFilePath, LPTSTR pszFileSize, LPTSTR pszFileTime){
	WASTR szPath;
	BY_HANDLE_FILE_INFORMATION bhfi;
	HANDLE hFile;

	WAstrcpyT(&szPath, pszFilePath);
	hFile = WAOpenFile(&szPath);
	if(hFile==INVALID_HANDLE_VALUE || !GetFileInformationByHandle(hFile, &bhfi)){
		lstrcpy(pszFileSize, TEXT("-"));
		lstrcpy(pszFileTime, TEXT("-"));
		return FALSE;
	}

	FormatDateTime(pszFileTime, &bhfi.ftLastWriteTime);
	wsprintf(pszFileSize, TEXT("%d KB"), bhfi.nFileSizeLow / 1024);

	CloseHandle(hFile);

	return TRUE;
}

void SetOLECursor(int nIndex){
	static HMODULE hMod = NULL;
	HCURSOR hCur;

	if(!hMod){
		hMod = GetModuleHandle(TEXT("ole32"));
	}
	hCur = LoadCursor(hMod, MAKEINTRESOURCE(nIndex));
	SetCursor(hCur);
	return;
}

LPTSTR MyPathAddBackslash(LPTSTR pszPath){
	if(PathIsDirectory(pszPath)){
		return PathAddBackslash(pszPath);
	}else if(IsPlayListFast(pszPath) || IsArchiveFast(pszPath)){
		return pszPath;
	}else{
		return PathAddBackslash(pszPath);
	}
}

// �I����ԃN���A
void ListView_ClearSelect(HWND hLV){
	ListView_SetItemState(hLV, -1, 0, (LVIS_SELECTED | LVIS_FOCUSED));
}

static void ListView_SingleSelectViewSub(HWND hLV, int nIndex, int flag){
	ListView_ClearSelect(hLV);
	if (nIndex >= 0){
		ListView_SetItemState(hLV, nIndex, (LVIS_SELECTED | LVIS_FOCUSED), (LVIS_SELECTED | LVIS_FOCUSED));
		if (flag & 1) ListView_EnsureVisible(hLV, nIndex, TRUE);
		if (flag & 2) PostMessage(hLV, LVM_ENSUREVISIBLE, (WPARAM)nIndex, (LPARAM)TRUE);
	}
}

// �S�Ă̑I����Ԃ�������A�w��C���f�b�N�X�̃A�C�e����I��
void ListView_SingleSelect(HWND hLV, int nIndex){
	ListView_SingleSelectViewSub(hLV, nIndex, 0);
}

// �S�Ă̑I����Ԃ�������A�w��C���f�b�N�X�̃A�C�e����I���A�\��
void ListView_SingleSelectView(HWND hLV, int nIndex){
	ListView_SingleSelectViewSub(hLV, nIndex, 1);
}

// �S�Ă̑I����Ԃ�������A�w��C���f�b�N�X�̃A�C�e����I���A�\���\��
void ListView_SingleSelectViewP(HWND hLV, int nIndex) {
	ListView_SingleSelectViewSub(hLV, nIndex, 3);
}

#define ALTYPE 0
#if (ALTYPE == 1)
/* ���[�N�`�F�b�N */
void *HAlloc(DWORD dwSize){
	return malloc(dwSize);
}
void *HZAlloc(DWORD dwSize){
	return calloc(dwSize, 1);
}
void *HRealloc(LPVOID pPtr, DWORD dwSize){
	return realloc(pPtr, dwSize);
}
void HFree(LPVOID pPtr){
	free(pPtr);
}
#elif (ALTYPE == 2)
/* �J���|�C���^�A�N�Z�X�`�F�b�N */
typedef struct {
	DWORD dwSize;
} HW;
LPVOID HAlloc(DWORD dwSize){
	HW *p = (HW *)VirtualAlloc(0, sizeof(HW) + dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (!p) return NULL;
	p->dwSize = dwSize;
	ZeroMemory(p + 1, dwSize);
	return p + 1;
}
LPVOID HZAlloc(DWORD dwSize){
	return HAlloc(dwSize);
}
LPVOID HRealloc(LPVOID pPtr, DWORD dwSize){
	HW *o = ((HW *)pPtr) - 1;
	LPVOID n = HAlloc(dwSize);
	if (n) {
		CopyMemory(n, pPtr, o->dwSize);
		HFree(pPtr);
	}
	return n;
}
void HFree(LPVOID pPtr){
	DWORD dwOldProtect;
	HW *o = ((HW *)pPtr) - 1;
	VirtualProtect(o, o->dwSize, PAGE_NOACCESS, &dwOldProtect);
}
#else
void *HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}
void *HZAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
}
void *HRealloc(LPVOID pPtr, DWORD dwSize){
	return HeapReAlloc(GetProcessHeap(), 0, pPtr, dwSize);
}
void HFree(LPVOID pPtr){
	HeapFree(GetProcessHeap(), 0, pPtr);
}
#endif