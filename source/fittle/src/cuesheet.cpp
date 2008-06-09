/*
 * CueSheet.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "cuesheet.h"
#include "list.h"
#include "func.h"

typedef enum {CUECHAR_DEFAULT,CUECHAR_UTF8,CUECHAR_UTF16LE,CUECHAR_UTF16BE,CUECHAR_NA} CueCharset;

static BYTE UTF8BOM[] = {0xEF,0xBB,0xBF};
static BYTE UTF16LEBOM[] = {0xFF,0xFE};
static BYTE UTF16BEBOM[] = {0xFE,0xFF};

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

// Detect CUE-Sheet Charset
static CueCharset DetectCharset(HANDLE hFile){
	BYTE buf[3];
	DWORD readed;
	ReadFile(hFile,buf,3,&readed,NULL);
	SetFilePointer(hFile,0,0,FILE_BEGIN);
	if(readed != 3)return CUECHAR_NA;
	if(memcmp(buf,UTF8BOM,3)==0){
		return CUECHAR_UTF8;
	}else if(memcmp(buf,UTF16LEBOM,2)==0 || buf[1] == 0){
		return CUECHAR_UTF16LE;
	}else if(memcmp(buf,UTF16BEBOM,2)==0 || buf[0] == 0){
		return CUECHAR_UTF16BE;
	}else{
		return CUECHAR_DEFAULT;
	}
}

/* xx:xx:xxという表記とハンドルから時間をQWORDで取得 */
QWORD GetByteFromSecStr(HCHANNEL hChan, char *pszSecStr){
	int min, sec, frame;

	if(*pszSecStr=='\0'){
		return BASS_ChannelGetLength(hChan, BASS_POS_BYTE);
	}

	min = atoi(pszSecStr);
	sec = 0;
	frame = 0;

	while (*pszSecStr && *pszSecStr!=':') pszSecStr++;

	if (*pszSecStr){
		sec = atoi(++pszSecStr);
		while (*pszSecStr && *pszSecStr!=':') pszSecStr++;
		if (*pszSecStr){
			frame = atoi(++pszSecStr);
		}
	}

	return BASS_ChannelSeconds2Bytes(hChan, (float)((min * 60 + sec) * 75 + frame) / 75.0);
}


// copied from bass_tag.cpp
static BOOL UTF8toShift_Jisn(LPSTR szOut, LPCSTR szIn, int cbOutBytes){
	int nSize;
	wchar_t *wTmp;

	// サイズ取得
	nSize = MultiByteToWideChar(CP_UTF8, 0, szIn, -1, NULL, 0);
	if(!nSize) return FALSE;

	// 領域確保
	wTmp = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(wchar_t)*(nSize+1));
	if(!wTmp) return FALSE;
	wTmp[nSize] = L'\0';

	MultiByteToWideChar(CP_UTF8, 0, szIn, -1, wTmp, nSize);
	if(!nSize){
		HeapFree(GetProcessHeap(), 0, wTmp);
		return FALSE;
	}

	WideCharToMultiByte(CP_ACP, 0, wTmp, lstrlenW(wTmp), szOut, cbOutBytes*sizeof(char), NULL, NULL);
	szOut[cbOutBytes-1] = '\0';
	HeapFree(GetProcessHeap(), 0, wTmp);

	return TRUE;
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

// convert string to SJIS
static void ConverttoShift_Jis(LPSTR pszOut, LPCSTR pszIn, int cbOutBytes, CueCharset Charset){
	int i;
	LPSTR tmp = NULL;
	ZeroMemory(pszOut,cbOutBytes);
	switch(Charset){
		case CUECHAR_UTF8:
			UTF8toShift_Jisn(pszOut,pszIn,cbOutBytes);
			break;
		case CUECHAR_UTF16LE:
			WideCharToMultiByte(CP_ACP,0,(LPWSTR)(pszIn+(pszIn[0]==0?1:0)),-1,pszOut,cbOutBytes,0,0);
			break;
		case CUECHAR_UTF16BE:
			tmp = (LPSTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(lstrlenW((LPCWSTR)pszIn)+1)*2);
			for(i=0;i<lstrlenW((LPCWSTR)pszIn);i++){
				tmp[i*2] = pszIn[i*2+1];
				tmp[i*2+1] = pszIn[i*2];
			}
			WideCharToMultiByte(CP_ACP,0,(LPWSTR)tmp,-1,pszOut,cbOutBytes,0,0);
			HeapFree(GetProcessHeap(),0,tmp);
			break;
		default:
			memcpy(pszOut,pszIn,cbOutBytes);
			break;
	}

}

int ReadLine(HANDLE hFile, LPSTR pszBuff, int nSize, CueCharset charset){
	DWORD bpc = (charset==CUECHAR_UTF16BE||charset==CUECHAR_UTF16LE?2:1);
	DWORD dwAccBytes = bpc;
	char buf[MAX_FITTLE_PATH];
	int i=0;
	char c[2] = {0};

	while (dwAccBytes>0) {
        ReadFile(hFile, c, bpc, &dwAccBytes, NULL);
		if(dwAccBytes != bpc){
			memset(buf+i,0,bpc);
			break;
		}
		if( ( (charset==CUECHAR_UTF16BE) && (c[0]=='\0') ) ||
			( (charset==CUECHAR_UTF16LE) && (c[1]=='\0') ) ||
			(bpc==1) ){
			switch(c[0]|c[1]){
				case '\r':
					i-=bpc; // cancel \r
					break;
				case '\n':
					*(buf+i) = '\0'; 
					if(bpc==2){*(buf+i+1) = '\0';}
					dwAccBytes = 0;
					break;
				default:
					*(buf+i) = c[0]; 
					if(bpc==2){*(buf+i+1) = c[1];}
					break;
			}
		}else{
			*(WCHAR*)(buf+i)=*(WCHAR*)c;
		}
		i+=bpc;
		if( i+bpc >= MAX_FITTLE_PATH) return -1;
	}
	ConverttoShift_Jis(pszBuff,buf,nSize,charset);

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
	CueCharset charset = CUECHAR_NA;
	FILEINFO **ppTale = ppSub;

	// オープン
	hFile = CreateFile(pszFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return -1;
	charset = DetectCharset(hFile);

	// 一行ずつ読み込む
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
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
	CueCharset charset = CUECHAR_NA;

	*pszStart = '\0';
	*pszEnd = '\0';

	lstrcpyn(szCuePath, pszCuePath, MAX_FITTLE_PATH);
	LPSTR pExt = StrStrI(szCuePath, ".cue/");
	nIndex = atoi(pExt + 5);
	*(pExt+4) = '\0';

	// オープン
	hFile = CreateFile(szCuePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile==INVALID_HANDLE_VALUE) return -1;
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
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
	CueCharset charset = CUECHAR_NA;

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
	charset = DetectCharset(hFile);

	// 一行ずつ処理
	while(ReadLine(hFile, szLine, MAX_FITTLE_PATH,charset) > 0){
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