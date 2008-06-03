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
#include "archive.h"
#include "cuesheet.h"

// ���[�J���֐�
void Merge(struct FILEINFO **, struct FILEINFO **);
int CompareNode(struct FILEINFO *, struct FILEINFO *, int);

// ���X�g�\���ɃZ����ǉ�
struct FILEINFO **AddList(struct FILEINFO **ptr, char *szFileName, char *pszFileSize, char *pszFileTime){
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
			MessageBox(NULL, "���X�g�̃C���f�b�N�X���s���ł��B", "Fittle", MB_OK | MB_ICONWARNING);
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
int GetIndexFromPath(struct FILEINFO *pRoot, char *szFilePath){
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
BOOL SearchFolder(struct FILEINFO **pRoot, char *pszSearchPath, BOOL bSubFlag){
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	char szSearchDirW[MAX_FITTLE_PATH];
	char szFullPath[MAX_FITTLE_PATH];
	char szSize[50], szTime[50];
	struct FILEINFO **pTale = pRoot;
	SYSTEMTIME sysTime;
	FILETIME locTime;
	
	if(!(GetFileAttributes(pszSearchPath) & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
	PathAddBackslash(pszSearchPath);
	wsprintf(szSearchDirW, "%s*.*", pszSearchPath);
	hFind = FindFirstFile(szSearchDirW, &wfd);
	if(hFind!=INVALID_HANDLE_VALUE){
		do{
			if(!(lstrcmp(wfd.cFileName, ".")==0 || lstrcmp(wfd.cFileName, "..")==0)){
				if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
					if(CheckFileType(wfd.cFileName)){
						wsprintf(szFullPath, "%s%s", pszSearchPath, wfd.cFileName); 
						wsprintf(szSize, "%d KB", wfd.nFileSizeLow/1024);
						FileTimeToLocalFileTime(&(wfd.ftLastWriteTime), &locTime);
						FileTimeToSystemTime(&locTime, &sysTime);
						wsprintf(szTime, "%02d/%02d/%02d %02d:%02d:%02d",
							sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
						pTale = AddList(pTale, szFullPath, szSize, szTime);
					}if(g_cfg.nZipSearch && bSubFlag && IsArchive(wfd.cFileName)){	// ZIP������
						wsprintf(szFullPath, "%s%s", pszSearchPath, wfd.cFileName);
						ReadArchive(pRoot, szFullPath);
					}
				}else{
					// �T�u�t�H���_������
					if(bSubFlag && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)){
						wsprintf(szFullPath, "%s%s", pszSearchPath, wfd.cFileName); 
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
int ReadM3UFile(struct FILEINFO **pSub, char *pszFilePath){
	HANDLE hFile;
	DWORD dwSize = 0L;
	char *lpszBuf;
	DWORD dwAccBytes;
	int i=0, j=0;
	char szTempPath[MAX_FITTLE_PATH];
	char szParPath[MAX_FITTLE_PATH];
	char szCombine[MAX_FITTLE_PATH];
	char szTime[50] = "-", szSize[50] = "-";
	struct FILEINFO **pTale = pSub;

#ifdef _DEBUG
		DWORD dTime;
		char szBuff[100];
		dTime = GetTickCount();
#endif

	if((hFile = CreateFile(pszFilePath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))==INVALID_HANDLE_VALUE) return -1;
	//GetParentDir(pszFilePath, szParPath);
	lstrcpyn(szParPath, pszFilePath, MAX_FITTLE_PATH);
	*(PathFindFileName(szParPath)-1) = '\0';
	dwSize = GetFileSize(hFile, NULL);
	lpszBuf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize+1);
	if(!lpszBuf) return -1;
	ReadFile(hFile, lpszBuf, dwSize+1, &dwAccBytes, NULL);
	lpszBuf[dwAccBytes] = '\0';

#ifdef _DEBUG
		wsprintf(szBuff, "ReadAlloc time: %d ms\n", GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

	// �ǂݍ��񂾃o�b�t�@������
	if(lpszBuf[0]!='\0'){ 
		do{
			if(lpszBuf[i]=='\0' || lpszBuf[i]=='\n'){
				szTempPath[j] = '\0';
				if(j!=0){
					// pls�`�F�b�N
					if(!StrCmpN(szTempPath, "File", 4)){
						lstrcpyn(szTempPath, StrStr(szTempPath, "=") + 1, MAX_FITTLE_PATH);
					}
					// URL�`�F�b�N
					if(IsURLPath(szTempPath)){
						 pTale = AddList(pTale, szTempPath, szSize, szTime);
					}else{
						// ���΃p�X�̏���
						//if(!(szTempPath[1] == ':' && szTempPath[2] == '\\')){
						if(PathIsRelative(szTempPath)){
							wsprintf(szCombine, "%s\\%s", szParPath, szTempPath);
							PathCanonicalize(szTempPath, szCombine);
						}
						if(!g_cfg.nExistCheck || (FILE_EXIST(szTempPath) || IsArchivePath(szTempPath) || IsCueSheet(szTempPath)))
						{
							if(g_cfg.nTimeInList){
								GetTimeAndSize(szTempPath, szSize, szTime);
							}
							pTale = AddList(pTale, szTempPath, szSize, szTime);
						}
					}
				}
				j=0;
			}else{
				if(lpszBuf[i]!='\r'){
					szTempPath[j] = lpszBuf[i];
					j++;
				}
			}
		}while(lpszBuf[i++]!='\0');
	}
	CloseHandle(hFile);
	HeapFree(GetProcessHeap(), 0, lpszBuf);

#ifdef _DEBUG
		wsprintf(szBuff, "ReadPlaylist time: %d ms\n", GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

	return 0;
}

// ���X�g��M3U�t�@�C���ŕۑ�
BOOL WriteM3UFile(struct FILEINFO *pRoot, char *szFile, int nMode){
	char *szBuff;
	FILEINFO *pTmp;
	HANDLE hFile;
	DWORD dwAccBytes;
	int nTotal;
	char szLine[MAX_FITTLE_PATH];

	// �������̊m��
	nTotal = GetListCount(pRoot);
	szBuff = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nTotal * (MAX_FITTLE_PATH + 2) + 1);
	if(!szBuff) return FALSE;

	// ������szBuff�ɕۑ����e��]��
	for(pTmp = pRoot;pTmp!=NULL;pTmp = pTmp->pNext){
		if(nMode==2){
			// ���΃p�X�̏���
			PathRelativePathTo(szLine, szFile, 0, pTmp->szFilePath, 0);
		}else{
			lstrcpyn(szLine, pTmp->szFilePath, MAX_FITTLE_PATH);
		}
		lstrcat(szBuff, szLine);
		lstrcat(szBuff, "\r\n"); // Windows���s�R�[�h������
	}
	lstrcat(szBuff, "\0");

	// �t�@�C���̏�������
	hFile = CreateFile(szFile, GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, szBuff, (DWORD)lstrlen(szBuff), &dwAccBytes, NULL);
	CloseHandle(hFile);

	// �������̊J��
	HeapFree(GetProcessHeap(), 0, szBuff);
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
			return lstrcmpi(IsURLPath(pLeft->szFilePath)?"URL":SafePathFindExtensionPlusOne(pLeft->szFilePath), StrStr(pRight->szFilePath, "://")?"URL":SafePathFindExtensionPlusOne(pRight->szFilePath));
		case -3:	// ��ޏ���
			return -1*lstrcmpi(IsURLPath(pLeft->szFilePath)?"URL":SafePathFindExtensionPlusOne(pLeft->szFilePath), StrStr(pRight->szFilePath, "://")?"URL":SafePathFindExtensionPlusOne(pRight->szFilePath));
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

int SearchFiles(struct FILEINFO **ppRoot, char *szFilePath, BOOL bSub){
	char szTime[50], szSize[50];

#ifdef _DEBUG
		DWORD dTime;
		char szBuff[100];
		dTime = GetTickCount();
#endif

	if(PathIsDirectory(szFilePath)){
		SearchFolder(ppRoot, szFilePath, bSub);	/* �t�H���_ */

#ifdef _DEBUG
		wsprintf(szBuff, "ReadFolder time: %d ms\n", GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

		return FOLDERS;
	}else if(IsPlayList(szFilePath)){
		ReadM3UFile(ppRoot, szFilePath);			/* �v���C���X�g */

//#ifdef _DEBUG
//		wsprintf(szBuff, "ReadPlaylist time: %d ms\n", GetTickCount() - dTime);
//		OutputDebugString(szBuff);
//#endif

		return LISTS;
	}else if(IsArchive(szFilePath)){
		ReadArchive(ppRoot, szFilePath);			/* �A�[�J�C�u */

#ifdef _DEBUG
		wsprintf(szBuff, "ReadArchive time: %d ms\n", GetTickCount() - dTime);
		OutputDebugString(szBuff);
#endif

		return ARCHIVES;
	}else if(IsCueSheet(szFilePath)){
		ReadCueSheet(ppRoot, szFilePath);			/* Cue Sheet */
		return CUESHEETS;
	}else if(FILE_EXIST(szFilePath) && CheckFileType(szFilePath)){
		GetTimeAndSize(szFilePath, szSize, szTime);		/* �P�̃t�@�C�� */
		AddList(ppRoot, szFilePath, szSize, szTime);
		return FILES;
	}else if(IsURLPath(szFilePath)){
		AddList(ppRoot, szFilePath, "-", "-");
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