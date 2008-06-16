/*
 * bass_tag.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "fittle.h"
#include "bass_tag.h"

static unsigned int GetSyncSafeInt(LPBYTE b){
	return (b[0]<<21) | (b[1]<<14) | (b[2]<<7) | (b[3]);
}

static unsigned int GetSyncSafeInt22(LPBYTE b){
	return (b[0]<<14) | (b[1]<<7) | (b[2]);
}

static BOOL IsBomUTF8(LPBYTE pText, int nTextSize){
	return nTextSize >= 3 && pText[0] == 0xef && pText[1] == 0xbb && pText[2] == 0xbf;
}
static BOOL IsBomUTF16BE(LPBYTE pText, int nTextSize){
	return nTextSize >= 2 && pText[0] == 0xfe && pText[1] == 0xff;
}
static BOOL IsBomUTF16LE(LPBYTE pText, int nTextSize){
	return nTextSize >= 2 && pText[0] == 0xff && pText[1] == 0xfe;
}

static LPVOID HAlloc(DWORD dwSize){
	return HeapAlloc(GetProcessHeap(), 0, dwSize);
}

static void HFree(LPVOID lp){
	HeapFree(GetProcessHeap(), 0, lp);
}

static BOOL UTF8ToMultiByte(LPCSTR lpszSrc, int nSrcSize, LPSTR lpszDst, int nDstSize){
	int lw = MultiByteToWideChar(CP_UTF8, 0, lpszSrc, nSrcSize, 0, 0);
	LPWSTR pa;
	if (lw == 0) return FALSE;
	pa = (LPWSTR)HAlloc(lw * sizeof(WCHAR));
	if (!pa) return FALSE;
	MultiByteToWideChar(CP_UTF8, 0, lpszSrc, nSrcSize, pa, lw);
	WideCharToMultiByte(CP_ACP, 0, pa, lw, lpszDst, nDstSize, NULL, NULL);
	HFree(pa);
	return TRUE;
}

#define XMIN(x,y) ((x>y)?y:x)
static BOOL ID3V3_ReadFlameText(BYTE bType, LPBYTE pTextRaw, int nTextRawSize, LPTSTR pszBuf, int nBufSize){
	ZeroMemory(pszBuf, nBufSize * sizeof(TCHAR));
	if ((bType == 3) || (bType != 1 && bType != 2 && IsBomUTF8(pTextRaw, nTextRawSize))){
		/* UTF8 */
		if (IsBomUTF8(pTextRaw, nTextRawSize)) {
			pTextRaw += 3;
			nTextRawSize -= 3;
		}
#ifdef UNICODE
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pTextRaw, nTextRawSize, pszBuf, nBufSize);
#else
		UTF8ToMultiByte((LPCSTR)pTextRaw, nTextRawSize, pszBuf, nBufSize);
#endif
	} else if (bType == 1 || bType == 2){
		/* UTF16 */
		if (IsBomUTF16LE(pTextRaw, nTextRawSize)){
			pTextRaw += 2;
			nTextRawSize -= 2;
			bType = 1;
		} else if (IsBomUTF16BE(pTextRaw, nTextRawSize)){
			pTextRaw += 2;
			nTextRawSize -= 2;
			bType = 2;
		}
		if (bType == 2){
			LPBYTE pa = (LPBYTE)HAlloc(nTextRawSize);
			if (pa){
				int l = nTextRawSize >> 1;
				int i;
				for (i = 0; i < l; i++){
					pa[i * 2 + 0] = pTextRaw[i * 2 + 1];
					pa[i * 2 + 1] = pTextRaw[i * 2 + 0];
				}
				ID3V3_ReadFlameText(bType, pa, nTextRawSize, pszBuf, nBufSize);
				HFree(pa);
			}
		}
	#ifdef UNICODE
		CopyMemory(pszBuf, pTextRaw, XMIN(nTextRawSize >> 1, nBufSize) * sizeof(WCHAR));
	#else
		WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pTextRaw, nTextRawSize >> 1, pszBuf, nBufSize, NULL, NULL);
	#endif
	} else {
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pTextRaw, nTextRawSize, pszBuf, nBufSize);
#else
		CopyMemory(pszBuf, pTextRaw, XMIN(nTextRawSize, nBufSize) * sizeof(CHAR));
#endif
	}
	//OutputDebugString(pszBuf);
	//OutputDebugString(TEXT("\n"));
	return TRUE;
}

static BOOL ID3V23_ReadFlame(LPBYTE pFlame, int nFlameSize, LPTSTR pszBuf, int nBufSize){
	return ID3V3_ReadFlameText(pFlame[10], pFlame + 10 + 1, nFlameSize - 1, pszBuf, nBufSize);
}

static BOOL ID3V22_ReadFlame(LPBYTE pFlame, int nFlameSize, LPTSTR pszBuf, int nBufSize){
	return ID3V3_ReadFlameText(pFlame[6], pFlame + 6 + 1, nFlameSize - 1, pszBuf, nBufSize);
}

static BOOL ID3V1_ReadFlame(LPCSTR pFlame, int nFlameSize, LPTSTR pszBuf, int nBufSize){
	while (nFlameSize > 0 && pFlame[nFlameSize - 1] == ' ') nFlameSize--;
	return ID3V3_ReadFlameText(0, (LPBYTE)pFlame, nFlameSize, pszBuf, nBufSize);
}

static BOOL Vorbis_ReadFlame(LPBYTE pFlame, LPCSTR pszFlameID, LPTSTR pszBuf, int nBufSize){
	int nIDLength = lstrlenA(pszFlameID);

	if(StrCmpNIA((LPCSTR)pFlame, pszFlameID, nIDLength)) return FALSE;

	ZeroMemory(pszBuf, nBufSize * sizeof(TCHAR));
#ifdef UNICODE
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)(pFlame + nIDLength), -1, pszBuf, nBufSize);
#else
	UTF8ToMultiByte((LPCSTR)(pFlame + nIDLength), -1, pszBuf, nBufSize);
#endif
	//OutputDebugString(pszBuf);
	//OutputDebugString(TEXT("\n"));

	return TRUE;
}

static BOOL Riff_ReadFlame(LPBYTE pFlame, LPCSTR pszFlameID, LPTSTR pszBuf, int nBufSize){
	int nIDLength = lstrlenA(pszFlameID);

	if(StrCmpNIA((LPCSTR)pFlame, pszFlameID, nIDLength)) return FALSE;

	ZeroMemory(pszBuf, nBufSize * sizeof(TCHAR));
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(pFlame + nIDLength), -1, pszBuf, nBufSize);
#else
	lstrcpyn(pszBuf, (LPCSTR)(pFlame + nIDLength), nBufSize);
#endif
	//OutputDebugString(pszBuf);
	//OutputDebugString(TEXT("\n"));

	return TRUE;
}

static BOOL Riff_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_RIFF_INFO);
	while(p && *p){
		Riff_ReadFlame(p, "INAM=", pTagInfo->szTitle, 256);
		Riff_ReadFlame(p, "IART=", pTagInfo->szArtist, 256);
		Riff_ReadFlame(p, "IPRD=", pTagInfo->szAlbum, 256);
		p += lstrlenA((LPCSTR)p) + 1;
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL ID3V2_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_ID3V2);
	if(p && p[0] == 'I' && p[1] == 'D' && p[2] == '3'){
		char szFlameID[5];
		int nFlameSize;
		unsigned nTotal = 10;	// ヘッダサイズを足しておく
		unsigned nTagSize = GetSyncSafeInt(p + 6);
		// フレームを前から順に取得
		if(*(p + 3)>=3){	// バージョンの取得
			while(nTotal<nTagSize){
				// Ver.2.3以上
				lstrcpynA(szFlameID, (LPCSTR)(p + nTotal), 5); // フレームIDの取得
				nFlameSize = GetSyncSafeInt(p + nTotal + 4); // フレームサイズの取得
				if(lstrlenA(szFlameID)!=4) break;

				if(!lstrcmpA(szFlameID, "TIT2"))
					ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTitle, 256);
				else if(!lstrcmpA(szFlameID, "TPE1"))
					ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szArtist, 256);
				else if(!lstrcmpA(szFlameID, "TALB"))
					ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szAlbum, 256);
				else if(!lstrcmpA(szFlameID, "TRCK"))
					ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTrack, 10);

				nTotal += nFlameSize + 10;
			}
		}else{
			while(nTotal<nTagSize){
				// Ver.2.2以下
				lstrcpynA(szFlameID, (LPCSTR)(p + nTotal), 4); // フレームIDの取得
				nFlameSize = GetSyncSafeInt22(p + nTotal + 3); // フレームサイズの取得
				if(lstrlenA(szFlameID)!=3) break;

				if(!lstrcmpA(szFlameID, "TP1"))
					ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szArtist, 256);
				else if(!lstrcmpA(szFlameID, "TT2"))
					ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTitle, 256);
				else if(!lstrcmpA(szFlameID, "TAL"))
					ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szAlbum, 256);
				else if(!lstrcmpA(szFlameID, "TRK"))
					ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTrack, 10);

				nTotal += nFlameSize + 6;
			}
		}
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL ID3V1_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	ID3TAG *pID3Tag = (ID3TAG *)BASS_ChannelGetTags(handle, BASS_TAG_ID3);
	if (!pID3Tag) return FALSE;
	ID3V1_ReadFlame(pID3Tag->Title, 30, pTagInfo->szTitle, 256);
	ID3V1_ReadFlame(pID3Tag->Artist, 30, pTagInfo->szArtist, 256);
	ID3V1_ReadFlame(pID3Tag->Album, 30, pTagInfo->szAlbum, 256);
	ID3V1_ReadFlame(pID3Tag->Track, 2, pTagInfo->szTrack, 10);
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL WMA_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_WMA);
	while(p && *p){
		Vorbis_ReadFlame(p, "Title : ", pTagInfo->szTitle, 256);
		Vorbis_ReadFlame(p, "Author : ", pTagInfo->szArtist, 256);
		Vorbis_ReadFlame(p, "Album : ", pTagInfo->szAlbum, 256);
		Vorbis_ReadFlame(p, "WM/TrackNumber : ", pTagInfo->szTrack, 10);
		p += lstrlenA((LPCSTR)p) + 1;
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL ReadTagSub(LPBYTE p, TAGINFO *pTagInfo){
	while(p && *p){
		Vorbis_ReadFlame(p, "TITLE=", pTagInfo->szTitle, 256);
		Vorbis_ReadFlame(p, "ARTIST=", pTagInfo->szArtist, 256);
		Vorbis_ReadFlame(p, "ALBUM=", pTagInfo->szAlbum, 256);
		Vorbis_ReadFlame(p, "TRACKNUMBER=", pTagInfo->szTrack, 10);
		Vorbis_ReadFlame(p, "TRACKN=", pTagInfo->szTrack, 10);
		p += lstrlenA((LPCSTR)p) + 1;
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

BOOL BASS_TAG_Read(DWORD handle, TAGINFO *pTagInfo){
	BASS_CHANNELINFO bcinfo = {0};
	LPCSTR p = NULL;

	ZeroMemory(pTagInfo, sizeof(TAGINFO));

	BASS_ChannelGetInfo(handle, &bcinfo);
	switch(bcinfo.ctype){
		case BASS_CTYPE_STREAM_AIFF:
		case BASS_CTYPE_STREAM_WAV:
		case BASS_CTYPE_STREAM_WAV_PCM:  
		case BASS_CTYPE_STREAM_WAV_FLOAT:  
/*		case BASS_CTYPE_STREAM_WAV_MP3:*/ /* removed in BASS 2.4 */
			return Riff_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_MP1:
		case BASS_CTYPE_STREAM_MP2:
		case BASS_CTYPE_STREAM_MP3:
		case BASS_CTYPE_STREAM_TTA:
			/* bass_ape.dllを入れても読んでくれない */
			if (ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_APE), pTagInfo)) return TRUE;
			if (ID3V2_ReadTag(handle, pTagInfo)) return TRUE;
			return ID3V1_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_OFR:
		case BASS_CTYPE_STREAM_WV:
		case BASS_CTYPE_STREAM_WV_H:
		case BASS_CTYPE_STREAM_WV_L:
		case BASS_CTYPE_STREAM_WV_LH:
			/* bass_ape.dllを入れても読んでくれない */
			if (ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_APE), pTagInfo)) return TRUE;
			return ID3V1_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_WMA:
			return WMA_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_OGG:	// Vorbis comment
		case BASS_CTYPE_STREAM_FLAC:
		case BASS_CTYPE_STREAM_SPX:
			return ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_OGG), pTagInfo);

		case BASS_CTYPE_STREAM_MP4: // iTunes metadata
		case BASS_CTYPE_STREAM_ALAC:
		case BASS_CTYPE_STREAM_AAC:
			return ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_MP4), pTagInfo);

		case BASS_CTYPE_STREAM_APE:
		case BASS_CTYPE_STREAM_MPC:
			return ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_APE), pTagInfo);

		case BASS_CTYPE_STREAM_AC3:
			return FALSE;

		case BASS_CTYPE_STREAM_CD:
			return FALSE;

		case BASS_CTYPE_STREAM:
			p = BASS_ChannelGetTags(handle, BASS_TAG_ICY);
			return FALSE;

	}
	return FALSE;
}
