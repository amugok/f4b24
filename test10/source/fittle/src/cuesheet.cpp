/*
 * CueSheet.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "cuesheet.h"
#include "list.h"
#include "func.h"

/* CUEシートかどうか判断 */
BOOL IsCueSheet(char *szFilePath){
	char *p;

	if(!PathIsDirectory(szFilePath)){
		if((p = PathFindExtension(szFilePath)) != NULL && *p){
			p++;
			if(lstrcmpi(p, "cue")==0){
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* CUEシートパスか判断 */
BOOL IsCueSheetPath(char *pszFilePath){
	char *p;
	BOOL b;

	p = StrStrI(pszFilePath, ".cue/");
	if(!p){
		return FALSE;
	}

	*(p+4) = '\0';
	b = FILE_EXIST(pszFilePath);
	*(p+4) = '/';
	return b;
}

/* xx:xx:xxという表記とハンドルから時間をQWORDで取得 */
QWORD GetByteFromSecStr(HCHANNEL hChan, char *pszSecStr){
	int min, sec;

	if(*pszSecStr=='\0'){
		return BASS_ChannelGetLength(hChan, BASS_POS_BYTE);
	}

	min = atoi(pszSecStr);

	for(;*pszSecStr!=':';pszSecStr++);

	sec = atoi(++pszSecStr);

	return BASS_ChannelSeconds2Bytes(hChan, (float)(min*60+sec));
}

/* 単語を取得 */
LPSTR GetWord(LPSTR pszLine, LPSTR szToken, int nSize){
	int j = 0;
	LPSTR p = pszLine;
	BOOL bInQuate = FALSE;

	// skip whitespace
	while (*p==' ' || *p=='\t'){
		p++;
	}

	// get word
	do {
		if (*p=='\"') bInQuate = !bInQuate;

		*(szToken + j++) = *p++;
		if(j>=nSize) return NULL;
	} while( bInQuate || !(*p==' ' || *p=='\t') && !*p=='\0');
	*(szToken + j) = '\0';
	return p;
}

/* ファイルから一行読む */
int ReadLine(HANDLE hFile, LPSTR pszBuff, int nSize){
	DWORD dwAccBytes = 1;
	int i=0;
	char c;

	while (dwAccBytes>0) {
        ReadFile(hFile, &c, 1, &dwAccBytes, NULL);
		if(dwAccBytes<=0){
			*(pszBuff+i) = '\0';
			break;
		}
		switch (c) {
			case '\r':
				i--;
				break;
			case '\n':
				*(pszBuff+i) = '\0';
				dwAccBytes = 0;
				break;
			default:
				*(pszBuff+i) = c;
				break;
		}
		if( i++ >= nSize) return -1;
	}

	return i;
}

/* CUEシートを読み込む */
int ReadCueSheet(struct FILEINFO **ppSub, char *pszFilePath){
	HANDLE hFile;
	char szLine[MAX_FITTLE_PATH] = {0};
	char szCuePath[MAX_FITTLE_PATH] = {0};
	char szToken[MAX_FITTLE_PATH] = {0};
	char szAudioPath[MAX_FITTLE_PATH] = {0};
	char szTitle[256] = {0};
	FILEINFO **ppTale = ppSub;

	// オープン
	hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return -1;

	// 一行ずつ読み込む
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH) > 0){
		LPSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, "FILE")){
			// 対応形式であるかチェック
			GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(!CheckFileType(szToken)) return -1;
			lstrcpyn(szAudioPath, szToken, MAX_FITTLE_PATH);
		}else if(!lstrcmp(szToken, "TRACK")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			wsprintf(szCuePath, "%s/%s.", pszFilePath, szToken);
			ppTale = AddList(ppTale, szCuePath, "-", "-");
			lstrcpyn(szTitle, PathFindFileName(szAudioPath), 256);
		}else if(!lstrcmp(szToken, "TITLE")){
			// タイトル
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			lstrcpyn(szTitle, szToken, 256);
		}else if(!lstrcmp(szToken, "INDEX")){
			// タイトルを連結
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, "01")){
				lstrcat((*ppTale)->szFilePath, szTitle);
			}
		}
	}

	// クローズ
	CloseHandle(hFile);

	return GetListCount(*ppSub);;
}

/* CUEシートをパースし、実際のファイルと時間を取得 */
BOOL GetCueSheetRealPath(LPCSTR pszCuePath, LPSTR pszAudioPath, LPSTR pszStart, LPSTR pszEnd){
	HANDLE hFile;
	char szLine[MAX_FITTLE_PATH] = {0};
	char szCuePath[MAX_FITTLE_PATH] = {0};
	char szToken[MAX_FITTLE_PATH] = {0};
	int nIndex, nCurrent = -1;

	*pszStart = '\0';
	*pszEnd = '\0';

	lstrcpyn(szCuePath, pszCuePath, MAX_FITTLE_PATH);
	LPSTR pExt = StrStrI(szCuePath, ".cue/");
	nIndex = atoi(pExt + 5);
	*(pExt+4) = '\0';

	// オープン
	hFile = CreateFile(szCuePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return -1;

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH) > 0){
		LPSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, "FILE")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(PathIsRelative(szToken)){
				lstrcpyn(pszAudioPath, szCuePath, MAX_FITTLE_PATH);
				lstrcpyn(PathFindFileName(pszAudioPath), szToken, MAX_FITTLE_PATH);
			}else{
				lstrcpyn(pszAudioPath, szToken, MAX_FITTLE_PATH);
			}
		}else if(!lstrcmp(szToken, "TRACK")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = atoi(szToken);
		}else if(!lstrcmp(szToken, "INDEX")){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			if(!lstrcmp(szToken, "01")){
				GetWord(p, szToken, MAX_FITTLE_PATH);
				if(nIndex==nCurrent){
					lstrcpyn(pszStart, szToken, 100);
				}else if(nIndex+1==nCurrent && *pszEnd=='\0'){
					lstrcpyn(pszEnd, szToken, 100);
				}
			}else if(!lstrcmp(szToken, "00")){
				GetWord(p, szToken, MAX_FITTLE_PATH);
				if(nIndex+1==nCurrent){
					lstrcpyn(pszEnd, szToken, 100);
				}
			}
		}
	}

	// クローズ
	CloseHandle(hFile);
	return TRUE;
}


/* CUEシートをパースし、タグ情報を取得 */
BOOL GetCueSheetTagInfo(LPCSTR pszCuePath, TAGINFO *pTagInfo){
	HANDLE hFile;
	char szLine[MAX_FITTLE_PATH] = {0};
	char szCuePath[MAX_FITTLE_PATH] = {0};
	char szToken[MAX_FITTLE_PATH] = {0};
	int nIndex, nCurrent = -1;

	if(!IsCueSheetPath((LPSTR)pszCuePath)) return FALSE;

		ZeroMemory(pTagInfo, sizeof(TAGINFO));

	lstrcpyn(szCuePath, pszCuePath, MAX_FITTLE_PATH);
	LPSTR pExt = StrStrI(szCuePath, ".cue/");
	nIndex = atoi(pExt + 5);
	*(pExt+4) = '\0';

	wsprintf(pTagInfo->szTrack, "%d", nIndex);

	// オープン
	hFile = CreateFile(szCuePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return -1;

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH) > 0){
		LPSTR p = GetWord(szLine, szToken, MAX_FITTLE_PATH);
		if(!lstrcmp(szToken, "TRACK")){
			GetWord(p, szToken, MAX_FITTLE_PATH);
			nCurrent = atoi(szToken);
		}else if(!lstrcmp(szToken, "TITLE")){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(nCurrent==-1){
				lstrcpyn(pTagInfo->szAlbum, szToken, 256);
			}else if(nIndex==nCurrent){
				lstrcpyn(pTagInfo->szTitle, szToken, 256);
			}
		}else if(!lstrcmp(szToken, "PERFORMER")){
			p = GetWord(p, szToken, MAX_FITTLE_PATH);
			PathUnquoteSpaces(szToken);
			if(nCurrent==-1 || nIndex==nCurrent){
				lstrcpyn(pTagInfo->szArtist, szToken, 256);
			}
		}
	}

	// クローズ
	CloseHandle(hFile);

	return (lstrlen(pTagInfo->szTitle) > 0);
}