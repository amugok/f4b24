/*
 * List.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

//
//	FILEINFO�\���̂̃��X�g�\���𑀍�
//

#include "list.h"
#include "fittle.h"
#include "func.h"
#include "bass_tag.h"
#include "archive.h"

// ���[�J���֐�
static void Merge(struct FILEINFO **, struct FILEINFO **);
static int CompareNode(struct FILEINFO *, struct FILEINFO *, int);

// ���X�g�\���ɃZ����ǉ�
struct FILEINFO **AddList(struct FILEINFO **ptr, LPTSTR szFileName, LPTSTR pszFileSize, LPTSTR pszFileTime){
	struct FILEINFO *NewFileInfo;

	if(*ptr==NULL){
		NewFileInfo = (struct FILEINFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct FILEINFO));
		lstrcpyn(NewFileInfo->szFilePath, szFileName, MAX_FITTLE_PATH);
		lstrcpyn(NewFileInfo->szSize, pszFileSize, 50);
		lstrcpyn(NewFileInfo->szTime, pszFileTime, 50);
		NewFileInfo->bPlayFlag = FALSE;
		NewFileInfo->pNext = *ptr;
		*ptr = NewFileInfo;
		return ptr;
	}else{
		return AddList(&((*ptr)->pNext), szFileName, pszFileSize, pszFileTime);
	}
}

// ��̃Z�����폜
int DeleteAList(struct FILEINFO *pDel, struct FILEINFO **pRoot){
	struct FILEINFO **pTmp;

	for(pTmp = pRoot;*pTmp!=NULL;pTmp=&(*pTmp)->pNext){
		if(*pTmp==pDel){
			*pTmp = pDel->pNext; // �폜�Z�����΂��ĘA��
			HeapFree(GetProcessHeap(), 0, pDel);
			break;
		}
	}
	return 0;
}

// ���X�g�\���̃Z����S�č폜
int DeleteAllList(struct FILEINFO **pRoot){
	struct FILEINFO *pNxt;
	struct FILEINFO *pTmp;
	HANDLE hPH = GetProcessHeap();

	pTmp = *pRoot;
	while(pTmp){
		pNxt = pTmp->pNext;
		HeapFree(hPH, 0, pTmp);
		pTmp = pNxt;
	}
	*pRoot = NULL;
	return 0;
}

// ���X�g�\���̃Z���̑������擾
int GetListCount(struct FILEINFO *pRoot){
	int i = 0;

	while(pRoot){
		i++;
		pRoot = pRoot->pNext;
	}
	return i;
}

// �C���f�b�N�X -> �|�C���^
struct FILEINFO *GetPtrFromIndex(struct FILEINFO *pRoot, int nIndex){
	struct FILEINFO *pTmp;
	int i;

	if(nIndex==-1) return NULL;
	pTmp = pRoot;
	for(i=1;i<=nIndex;i++){
		if(!pTmp){
			MessageBox(NULL, TEXT("���X�g�̃C���f�b�N�X���s���ł��B"), TEXT("Fittle"), MB_OK | MB_ICONWARNING);
			break;
		}
		pTmp = pTmp->pNext;
	}
	return pTmp;
}

// �|�C���^ -> �C���f�b�N�X
int GetIndexFromPtr(struct FILEINFO *pRoot, struct FILEINFO *pTarget){
	struct FILEINFO *pTmp;
	int i = 0;
	
	if(!pTarget) return -1;
	for(pTmp=pRoot;pTmp!=NULL;pTmp=pTmp->pNext){
		if(pTmp==pTarget) return i;
		i++;
	}
	return 0; // ������Ȃ������ꍇ
}

// �p�X -> �C���f�b�N�X
int GetIndexFromPath(struct FILEINFO *pRoot, LPTSTR szFilePath){
	struct FILEINFO *pTmp;
	int i = -1;

	for(pTmp=pRoot;pTmp!=NULL;pTmp=pTmp->pNext){
		i++;
		if(lstrcmpi(pTmp->szFilePath, szFilePath)==0) return i;
	}
	return -1;
}

// ���ׂĖ��Đ��ɂ���
int SetUnPlayed(struct FILEINFO *pRoot){
	struct FILEINFO *pTmp = NULL;
	int i = 0;

	for(pTmp=pRoot;pTmp!=NULL;pTmp=pTmp->pNext){
		pTmp->bPlayFlag = FALSE;
		i++;
	}
	return i;
}

// ���Đ��C���f�b�N�X -> ���ʂ̃C���f�b�N�X
int GetUnPlayedIndex(struct FILEINFO *pRoot, int nIndex){
	struct FILEINFO *pTmp;
	int i = 0, j= 0;

	for(pTmp = pRoot;pTmp!=NULL;pTmp = pTmp->pNext){
		if(!pTmp->bPlayFlag)
			if(i++==nIndex) return j;
		j++;
	}
	return 0;
}

// ���Đ��̃t�@�C���̐����擾
int GetUnPlayedFile(struct FILEINFO *ptr){
	int i = 0;

	for(;ptr!=NULL;ptr = ptr->pNext)
		if(ptr->bPlayFlag==FALSE) i++;
	return i;
}

// ���X�g�\���Ƀt�H���_�ȉ��̃t�@�C����ǉ�
BOOL SearchFolder(struct FILEINFO **pRoot, LPTSTR pszSearchPath, BOOL bSubFlag){
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szSearchDirW[MAX_FITTLE_PATH];
	TCHAR szFullPath[MAX_FITTLE_PATH];
	TCHAR szSize[50], szTime[50];
	struct FILEINFO **pTale = pRoot;
	SYSTEMTIME sysTime;
	FILETIME locTime;
	
	if(!(GetFileAttributes(pszSearchPath) & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
	PathAddBackslash(pszSearchPath);
	wsprintf(szSearchDirW, TEXT("%s*.*"), pszSearchPath);
	hFind = FindFirstFile(szSearchDirW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			if(!(lstrcmp(wfd.cFileName, TEXT("."))==0 || lstrcmp(wfd.cFileName, TEXT(".."))==0)){
				if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
					if(CheckFileType(wfd.cFileName)){
						wsprintf(szFullPath, TEXT("%s%s"), pszSearchPath, wfd.cFileName); 
						wsprintf(szSize, TEXT("%d KB"), wfd.nFileSizeLow/1024);
						FileTimeToLocalFileTime(&(wfd.ftLastWriteTime), &locTime);
						FileTimeToSystemTime(&locTime, &sysTime);
						wsprintf(szTime, TEXT("%02d/%02d/%02d %02d:%02d:%02d"),
							sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
						pTale = AddList(pTale, szFullPath, szSize, szTime);
					}if(g_cfg.nZipSearch && bSubFlag && IsArchive(wfd.cFileName)){	// ZIP������
						wsprintf(szFullPath, TEXT("%s%s"), pszSearchPath, wfd.cFileName);
						ReadArchive(pRoot, szFullPath);
					}
				}else{
					// �T�u�t�H���_������
					if(bSubFlag && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)){
						wsprintf(szFullPath, TEXT("%s%s"), pszSearchPath, wfd.cFileName); 
						SearchFolder(pRoot, szFullPath, TRUE);
					}
				}
			}
		}while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return TRUE;
}

// M3U�t�@�C����ǂݍ��݁A���X�g�r���[�ɒǉ�
int ReadM3UFile(struct FILEINFO **pSub, LPTSTR pszFilePath){
	HANDLE hFile;
	DWORD dwSize = 0L;
	LPSTR lpszBuf;
	DWORD dwAccBytes;
	BOOL fUTF8 = FALSE;
	int i=0, j=0;
	CHAR szTempPath[MAX_FITTLE_PATH];

	TCHAR szCombine[MAX_FITTLE_PATH];
	TCHAR szParPath[MAX_FITTLE_PATH];

	WCHAR szWideTemp[MAX_FITTLE_PATH];
#ifdef UNICODE
#else
	CHAR szAnsiTemp[MAX_FITTLE_PATH];
#endif
	TCHAR szTstrTemp[MAX_FITTLE_PATH];

	TCHAR szTime[50] = TEXT("-"), szSize[50] = TEXT("-");
	struct FILEINFO **pTale = pSub;

#ifdef _DEBUG
		DWORD dTime;
		TCHAR szBuff[100];
		dTime = GetTickCount();
#endif

	if((hFile = CreateFile(pszFilePath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))==INVALID_HANDLE_VALUE) return -1;
	//GetParentDir(pszFilePath, szParPath);

	lstrcpyn(szParPath, pszFilePath, MAX_FITTLE_PATH);
	*(PathFindFileName(szParPath)-1) = '\0';

	dwSize = GetFileSize(hFile, NULL);
	lpszBuf = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + sizeof(CHAR));
	if(!lpszBuf) return -1;
	ReadFile(hFile, lpszBuf, dwSize, &dwAccBytes, NULL);
	lpszBuf[dwAccBytes] = '\0';

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadAlloc time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

	// �ǂݍ��񂾃o�b�t�@������
	if(lpszBuf[0] != '\0'){
		// UTF-8 BOM
		if (lpszBuf[0] == '\xef' && lpszBuf[1] == '\xbb' && lpszBuf[2] == '\xbf'){
			fUTF8 = TRUE;
			i = 3;
		}
		do {
			if(lpszBuf[i]=='\0' || lpszBuf[i]=='\n' || lpszBuf[i]=='\r'){
				szTempPath[j] = '\0';
				if(j != 0){
					// pls�`�F�b�N
					if(!StrCmpNA(szTempPath, "File", 4) && StrStrA(szTempPath, "=")){
						lstrcpynA(szTempPath, StrStrA(szTempPath, "=") + 1, MAX_FITTLE_PATH);
					}
					MultiByteToWideChar(fUTF8 ? CP_UTF8 : CP_ACP, 0, szTempPath, -1, szWideTemp, MAX_FITTLE_PATH);
					// URL�`�F�b�N
					if(IsURLPathA(szTempPath)){
						lstrcpy(szSize, TEXT("-"));
						lstrcpy(szTime, TEXT("-"));
#ifdef UNICODE
						pTale = AddList(pTale, szWideTemp, szSize, szTime);
#else
						WideCharToMultiByte(CP_ACP, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
						pTale = AddList(pTale, szAnsiTemp, szSize, szTime);
#endif
					}else{
						// ���΃p�X�̏���
						//if(!(szTempPath[1] == TEXT(':') && szTempPath[2] == TEXT('\\'))){
						if(PathIsRelativeA(szTempPath)){
#ifdef UNICODE
							wsprintfW(szTstrTemp, L"%s\\%s", szParPath, szWideTemp);
#else
							WideCharToMultiByte(CP_ACP, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
							wsprintfA(szTstrTemp, "%s\\%s", szParPath, szAnsiTemp);
#endif
							PathCanonicalize(szCombine, szTstrTemp);
						}else{
#ifdef UNICODE
							lstrcpyn(szCombine, szWideTemp, MAX_FITTLE_PATH);
#else
							WideCharToMultiByte(CP_ACP, 0, szWideTemp, -1, szCombine, MAX_FITTLE_PATH, NULL, NULL);
#endif
						}
						if(!g_cfg.nExistCheck || (FILE_EXIST(szCombine) || IsArchivePath(szCombine)))
						{
							if(g_cfg.nTimeInList){
								GetTimeAndSize(szCombine, szSize, szTime);
							}
							pTale = AddList(pTale, szCombine, szSize, szTime);
						}
					}
				}
				j=0;
			}else{
				szTempPath[j] = lpszBuf[i];
				j++;
			}
		}while(lpszBuf[i++] != '\0');
	}
	CloseHandle(hFile);
	HeapFree(GetProcessHeap(), 0, lpszBuf);

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadPlaylist time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

	return 0;
}

// ���X�g��M3U�t�@�C���ŕۑ�
BOOL WriteM3UFile(struct FILEINFO *pRoot, LPTSTR szFile, int nMode){
	FILEINFO *pTmp;
	HANDLE hFile;
	DWORD dwAccBytes;
	TCHAR szLine[MAX_FITTLE_PATH];
#ifdef UNICODE
#else
	WCHAR szWideTemp[MAX_FITTLE_PATH];
#endif
	CHAR szAnsiTemp[MAX_FITTLE_PATH * 3];

		// �t�@�C���̏�������
	hFile = CreateFile(szFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if(nMode==3 || nMode==4){
		WriteFile(hFile, "\xef\xbb\xbf", 3, &dwAccBytes, NULL);
	}

	// ������szBuff�ɕۑ����e��]��
	for(pTmp = pRoot; pTmp!=NULL; pTmp = pTmp->pNext){
		if(nMode==2 || nMode==4){
			// ���΃p�X�̏���
			PathRelativePathTo(szLine, szFile, 0, pTmp->szFilePath, 0);
		}else{
			lstrcpyn(szLine, pTmp->szFilePath, MAX_FITTLE_PATH);
		}
		if(nMode==3 || nMode==4){
#ifdef UNICODE
			WideCharToMultiByte(CP_UTF8, 0, szLine, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
#else
			MultiByteToWideChar(CP_ACP, 0, szLine, -1, szWideTemp, MAX_FITTLE_PATH);
			WideCharToMultiByte(CP_UTF8, 0, szWideTemp, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
#endif
			WriteFile(hFile, szAnsiTemp, lstrlenA(szAnsiTemp), &dwAccBytes, NULL);
		}else{
#ifdef UNICODE
			WideCharToMultiByte(CP_ACP, 0, szLine, -1, szAnsiTemp, MAX_FITTLE_PATH, NULL, NULL);
			WriteFile(hFile, szAnsiTemp, lstrlenA(szAnsiTemp), &dwAccBytes, NULL);
#else
			WriteFile(hFile, szLine, lstrlenA(szLine), &dwAccBytes, NULL);
#endif
		}

		WriteFile(hFile, "\r\n", 2, &dwAccBytes, NULL);
	}
	CloseHandle(hFile);
	return TRUE;
}

static LPTSTR SafePathFindExtensionPlusOne(LPTSTR lpszPath){
	static TCHAR safezero[4] = { 0,0,0,0 };
	LPTSTR p = PathFindExtension(lpszPath);
	return (p && *p) ? p + 1 : safezero;
}

int CompareNode(struct FILEINFO *pLeft, struct FILEINFO *pRight, int nSortType){
	switch(nSortType){
		case 0:		// �t���p�X
			return lstrcmpi(pLeft->szFilePath, pRight->szFilePath);
		case 1:		// �t�@�C��������
			return lstrcmpi(GetFileName(pLeft->szFilePath), GetFileName(pRight->szFilePath));
		case -1:	// �t�@�C�����~��
			return -1*lstrcmpi(GetFileName(pLeft->szFilePath), GetFileName(pRight->szFilePath));
		case 2:		// �T�C�Y����
			return StrToInt(pLeft->szSize) - StrToInt(pRight->szSize);
		case -2:	// �T�C�Y�~��
			return StrToInt(pRight->szSize) - StrToInt(pLeft->szSize);
		case 3:		// ��ޏ���
			return lstrcmpi(IsURLPath(pLeft->szFilePath)?TEXT("URL"):SafePathFindExtensionPlusOne(pLeft->szFilePath), StrStr(pRight->szFilePath, TEXT("://"))?TEXT("URL"):SafePathFindExtensionPlusOne(pRight->szFilePath));
		case -3:	// ��ޏ���
			return -1*lstrcmpi(IsURLPath(pLeft->szFilePath)?TEXT("URL"):SafePathFindExtensionPlusOne(pLeft->szFilePath), StrStr(pRight->szFilePath, TEXT("://"))?TEXT("URL"):SafePathFindExtensionPlusOne(pRight->szFilePath));
		case 4:		// �X�V��������
			return lstrcmp(pLeft->szTime, pRight->szTime);
		case -4:	// �X�V�����~��
			return -1*lstrcmp(pLeft->szTime, pRight->szTime);
		default:
			return 0;
	}
}

// ��̃��X�g���}�[�W�BppLeft���}�[�W��̃��[�g�ɂȂ�
void Merge(struct FILEINFO **ppLeft, struct FILEINFO **ppRight, int nSortType){
	struct FILEINFO *pNewRoot;
	struct FILEINFO *pNow;

	// ���̏���
	if(!(*ppRight)
	|| ((*ppLeft) && CompareNode(*ppLeft, *ppRight, nSortType)<=0)){
		pNewRoot = pNow = *ppLeft;
		*ppLeft = (*ppLeft)->pNext;
	}else{
		pNewRoot = pNow = *ppRight;
		*ppRight = (*ppRight)->pNext;
	}

	// ����ȍ~�̏���
	while(*ppLeft || *ppRight){
		if(!(*ppRight)
		|| ((*ppLeft) && CompareNode(*ppLeft, *ppRight, nSortType)<=0)){
			pNow->pNext = *ppLeft;	// pNow�ɘA��
			pNow = *ppLeft;			// pNow���ڍs
			*ppLeft = (*ppLeft)->pNext;	// ppLeft���폜
		}else{
			pNow->pNext = *ppRight;	// pNow�ɘA��
			pNow = *ppRight;			// pNow���ڍs
			*ppRight = (*ppRight)->pNext;	// ppLeft���폜
		}
	}

	// ppLeft�ɕԂ��Ă��
	*ppLeft = pNewRoot;	
	return;
}

// ���X�g�\�����}�[�W�\�[�g
void MergeSort(struct FILEINFO **ppRoot, int nSortType){
	struct FILEINFO *pLeft;
	struct FILEINFO *pRight;
	struct FILEINFO *pMiddle;
	int nCount;

	nCount = GetListCount(*ppRoot);
	if(nCount<=1) return;

	// ���X�g��񕪊�
	pLeft = *ppRoot;
	pMiddle = GetPtrFromIndex(*ppRoot, nCount/2-1);
	pRight = pMiddle->pNext;
	pMiddle->pNext = NULL;

	// �ċA�I�Ăяo��
	MergeSort(&pLeft, nSortType);
	MergeSort(&pRight, nSortType);

	// ��̃��X�g���}�[�W
	Merge(&pLeft, &pRight, nSortType);
	*ppRoot = pLeft;

	return;
}

int SearchFiles(struct FILEINFO **ppRoot, LPTSTR szFilePath, BOOL bSub){
	TCHAR szTime[50], szSize[50];

#ifdef _DEBUG
		DWORD dTime;
		TCHAR szBuff[100];
		dTime = GetTickCount();
#endif

	if(PathIsDirectory(szFilePath)){
		SearchFolder(ppRoot, szFilePath, bSub);	/* �t�H���_ */

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadFolder time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

		return FOLDERS;
	}else if(IsPlayList(szFilePath)){
		ReadM3UFile(ppRoot, szFilePath);			/* �v���C���X�g */

//#ifdef _DEBUG
//		wsprintf(szBuff, TEXT("ReadPlaylist time: %d ms\n"), GetTickCount() - dTime);
//		OutputDebugString(szBuff);
//#endif

		return LISTS;
	}else if(IsArchive(szFilePath)){
		ReadArchive(ppRoot, szFilePath);			/* �A�[�J�C�u */

#ifdef _DEBUG
		wsprintf(szBuff, TEXT("ReadArchive time: %d ms\n"), GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

		return ARCHIVES;
	}else if(FILE_EXIST(szFilePath) && CheckFileType(szFilePath)){
		GetTimeAndSize(szFilePath, szSize, szTime);		/* �P�̃t�@�C�� */
		AddList(ppRoot, szFilePath, szSize, szTime);
		return FILES;
	}else if(IsURLPath(szFilePath)){
		AddList(ppRoot, szFilePath, TEXT("-"), TEXT("-"));
		return URLS;
	}
	return OTHERS;
}

int LinkCheck(struct FILEINFO **ppRoot){
	struct FILEINFO *pTmp;
	struct FILEINFO *pDel;
	int i=0;

	for(pTmp = *ppRoot;pTmp;){
		if(!FILE_EXIST(pTmp->szFilePath) && !IsArchivePath(pTmp->szFilePath) && !IsURLPath(pTmp->szFilePath)){
			pDel = pTmp;
			pTmp = pTmp->pNext;
			DeleteAList(pDel, ppRoot);
		}else{
			pTmp = pTmp->pNext;
			i++;
		}
	}
	return i;
}