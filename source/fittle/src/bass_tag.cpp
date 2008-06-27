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

static unsigned int GetNonSyncSafeInt23(LPBYTE b){
	return (b[0]<<24) | (b[1]<<16) | (b[2]<<8) | (b[3]);
}

static unsigned int GetNonSyncSafeInt22(LPBYTE b){
	return (b[0]<<16) | (b[1]<<8) | (b[2]);
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
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
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
static BOOL ID3_ReadFrameText(BYTE bType, LPBYTE pTextRaw, int nTextRawSize, LPTSTR pszBuf, int nBufSize){
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
				ID3_ReadFrameText(bType, pa, nTextRawSize, pszBuf, nBufSize);
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

static LPBYTE Unsync(LPBYTE pSrc, unsigned nSrcLen, unsigned nDstLmt, unsigned *pnDstLen) {
	LPBYTE lpUnsync = (LPBYTE)HAlloc(nSrcLen ? nSrcLen : nDstLmt);
	if (lpUnsync) {
		unsigned i, j;
		for (i = j = 0; (nSrcLen ? (i < nSrcLen) : (j < nDstLmt)); i++)
		{
			BYTE b = pSrc[i];
			lpUnsync[j++] = b; 
			if (b == 0xff && pSrc[i + 1] == 0) i++;
		}
		*pnDstLen = j;
	}
	return lpUnsync;
}

static BOOL ID3V23_ReadFrame(int nFlag, LPBYTE pRaw, unsigned nFrameSize, LPTSTR pszBuf, int nBufSize){
	BOOL bRet = FALSE;
	unsigned nRawSize = nFrameSize;
	if (nFlag & 1)
	{
		nRawSize -= 4;
		pRaw += 4;
	}
	if (nFlag & 2){
		unsigned nUnsynced;
		LPBYTE lpUnsync = Unsync(pRaw + 1, nRawSize - 1, 0, &nUnsynced);
		if (lpUnsync) {
			bRet = ID3_ReadFrameText(pRaw[0], lpUnsync, nUnsynced, pszBuf, nBufSize);
			HFree(lpUnsync);
		}
	} else {
		bRet = ID3_ReadFrameText(pRaw[0], pRaw + 1, nRawSize - 1, pszBuf, nBufSize);
	}
	return bRet;
}

static BOOL ID3V1_ReadFrame(LPCSTR pFrame, int nFrameSize, LPTSTR pszBuf, int nBufSize){
	while (nFrameSize > 0 && pFrame[nFrameSize - 1] == ' ') nFrameSize--;
	return ID3_ReadFrameText(0, (LPBYTE)pFrame, nFrameSize, pszBuf, nBufSize);
}

static BOOL Vorbis_ReadFrame(LPBYTE pFrame, LPCSTR pszFrameID, LPTSTR pszBuf, int nBufSize){
	int nIDLength = lstrlenA(pszFrameID);

	if(StrCmpNIA((LPCSTR)pFrame, pszFrameID, nIDLength)) return FALSE;

	ZeroMemory(pszBuf, nBufSize * sizeof(TCHAR));
#ifdef UNICODE
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)(pFrame + nIDLength), -1, pszBuf, nBufSize);
#else
	UTF8ToMultiByte((LPCSTR)(pFrame + nIDLength), -1, pszBuf, nBufSize);
#endif
	//OutputDebugString(pszBuf);
	//OutputDebugString(TEXT("\n"));

	return TRUE;
}

static BOOL Riff_ReadFrame(LPBYTE pFrame, LPCSTR pszFrameID, LPTSTR pszBuf, int nBufSize){
	int nIDLength = lstrlenA(pszFrameID);

	if(StrCmpNIA((LPCSTR)pFrame, pszFrameID, nIDLength)) return FALSE;

	ZeroMemory(pszBuf, nBufSize * sizeof(TCHAR));
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(pFrame + nIDLength), -1, pszBuf, nBufSize);
#else
	lstrcpyn(pszBuf, (LPCSTR)(pFrame + nIDLength), nBufSize);
#endif
	//OutputDebugString(pszBuf);
	//OutputDebugString(TEXT("\n"));

	return TRUE;
}

static BOOL Riff_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_RIFF_INFO);
	while(p && *p){
		Riff_ReadFrame(p, "INAM=", pTagInfo->szTitle, 256);
		Riff_ReadFrame(p, "IART=", pTagInfo->szArtist, 256);
		Riff_ReadFrame(p, "IPRD=", pTagInfo->szAlbum, 256);
		p += lstrlenA((LPCSTR)p) + 1;
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL ID3V2_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_ID3V2);
	if(p && p[0] == 'I' && p[1] == 'D' && p[2] == '3'){
		char szFrameID[5];
		unsigned nFrameSize;
		unsigned nTotal = 0;	// ヘッダサイズを足しておく
		unsigned nTagSize = GetSyncSafeInt(p + 6);
		unsigned nVersion = *(p + 3);
		unsigned nFlag = *(p + 5);
		LPBYTE pUnsync = NULL;
		if (nFlag & 0x80)
			p = pUnsync = Unsync(p + 10, 0, nTagSize, &nTagSize);
		else
			p += 10;
		if (!p) return FALSE;

		// フレームを前から順に取得
		if(nVersion >=3){	// バージョンの取得
			if (nFlag & 0x40) {
				/* 拡張ヘッダ */
				nTotal += (nVersion == 3) ? (4 + GetNonSyncSafeInt23(p + nTotal)) : GetSyncSafeInt(p + nTotal);
			}
			while(nTotal<nTagSize){
				int nFrameFlag = 0;
				int nLenID;
				// Ver.2.3以上
				lstrcpynA(szFrameID, (LPCSTR)(p + nTotal), 5); // フレームIDの取得
				if (nVersion == 3)
					nFrameSize = GetNonSyncSafeInt23(p + nTotal + 4); // フレームサイズの取得
				else
					nFrameSize = GetSyncSafeInt(p + nTotal + 4); // フレームサイズの取得
				nLenID = lstrlenA(szFrameID);
				if (nLenID > 0 && nLenID < 4 && nTotal > 4){
					/* broken tag */
					lstrcpynA(szFrameID, (LPCSTR)(p + nTotal - 4 + nLenID), 5); // フレームIDの取得
					if (lstrlenA(szFrameID) == 4){
						nTotal += - 4 + nLenID;
						continue;
					}
				}
				if(nLenID !=4) break;
				if (nVersion == 4)
					nFrameFlag = p[nTotal+9] & 3;
				if(!lstrcmpA(szFrameID, "TIT2"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 10, nFrameSize, pTagInfo->szTitle, 256);
				else if(!lstrcmpA(szFrameID, "TPE1"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 10, nFrameSize, pTagInfo->szArtist, 256);
				else if(!lstrcmpA(szFrameID, "TALB"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 10, nFrameSize, pTagInfo->szAlbum, 256);
				else if(!lstrcmpA(szFrameID, "TRCK"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 10, nFrameSize, pTagInfo->szTrack, 10);

				nTotal += nFrameSize + 10;
			}
		}else{
			while(nTotal<nTagSize){
				int nFrameFlag = 0;
				// Ver.2.2以下
				lstrcpynA(szFrameID, (LPCSTR)(p + nTotal), 4); // フレームIDの取得
				nFrameSize = GetNonSyncSafeInt22(p + nTotal + 3); // フレームサイズの取得
				if(lstrlenA(szFrameID)!=3) break;

				if(!lstrcmpA(szFrameID, "TP1"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 6, nFrameSize, pTagInfo->szArtist, 256);
				else if(!lstrcmpA(szFrameID, "TT2"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 6, nFrameSize, pTagInfo->szTitle, 256);
				else if(!lstrcmpA(szFrameID, "TAL"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 6, nFrameSize, pTagInfo->szAlbum, 256);
				else if(!lstrcmpA(szFrameID, "TRK"))
					ID3V23_ReadFrame(nFrameFlag, p + nTotal + 6, nFrameSize, pTagInfo->szTrack, 10);

				nTotal += nFrameSize + 6;
			}
		}

		if (pUnsync) HFree(pUnsync);
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL ID3V1_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	ID3TAG *pID3Tag = (ID3TAG *)BASS_ChannelGetTags(handle, BASS_TAG_ID3);
	if (!pID3Tag) return FALSE;
	ID3V1_ReadFrame(pID3Tag->Title, 30, pTagInfo->szTitle, 256);
	ID3V1_ReadFrame(pID3Tag->Artist, 30, pTagInfo->szArtist, 256);
	ID3V1_ReadFrame(pID3Tag->Album, 30, pTagInfo->szAlbum, 256);
	ID3V1_ReadFrame(pID3Tag->Track, 2, pTagInfo->szTrack, 10);
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL WMA_ReadTag(DWORD handle, TAGINFO *pTagInfo){
	LPBYTE p = (LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_WMA);
	while(p && *p){
		Vorbis_ReadFrame(p, "Title=", pTagInfo->szTitle, 256);
		Vorbis_ReadFrame(p, "Author=", pTagInfo->szArtist, 256);
		Vorbis_ReadFrame(p, "WM/AlbumTitle=", pTagInfo->szAlbum, 256);
		Vorbis_ReadFrame(p, "WM/Track=", pTagInfo->szTrack, 10);
		Vorbis_ReadFrame(p, "WM/TrackNumber=", pTagInfo->szTrack, 10);
		p += lstrlenA((LPCSTR)p) + 1;
	}
	if(*pTagInfo->szTitle || *pTagInfo->szArtist) return TRUE;
	return FALSE;
}

static BOOL ReadTagSub(LPBYTE p, TAGINFO *pTagInfo){
	while(p && *p){
		Vorbis_ReadFrame(p, "TITLE=", pTagInfo->szTitle, 256);
		Vorbis_ReadFrame(p, "ARTIST=", pTagInfo->szArtist, 256);
		Vorbis_ReadFrame(p, "ALBUM=", pTagInfo->szAlbum, 256);
		Vorbis_ReadFrame(p, "TRACKNUMBER=", pTagInfo->szTrack, 10);
		Vorbis_ReadFrame(p, "TRACKN=", pTagInfo->szTrack, 10);
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
		case BASS_CTYPE_STREAM_WAV_MP3: /* removed in BASS 2.4 */
			return Riff_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_MP1:
		case BASS_CTYPE_STREAM_MP2:
		case BASS_CTYPE_STREAM_MP3:
		case BASS_CTYPE_STREAM_APE:
		case BASS_CTYPE_STREAM_MPC:
		case BASS_CTYPE_STREAM_TTA:
		case BASS_CTYPE_STREAM_OFR:
		case BASS_CTYPE_STREAM_WV:
		case BASS_CTYPE_STREAM_WV_H:
		case BASS_CTYPE_STREAM_WV_L:
		case BASS_CTYPE_STREAM_WV_LH:
		case BASS_CTYPE_STREAM_TAK:
			/* bass_ape.dllを入れても読んでくれない */
			if (ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_APE), pTagInfo)) return TRUE;
			if (ID3V2_ReadTag(handle, pTagInfo)) return TRUE;
			return ID3V1_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_WMA:
		case BASS_CTYPE_STREAM_WMA_MP3:
			return WMA_ReadTag(handle, pTagInfo);

		case BASS_CTYPE_STREAM_OGG:	// Vorbis comment
		case BASS_CTYPE_STREAM_FLAC:
		case BASS_CTYPE_STREAM_FLAC_OGG:
		case BASS_CTYPE_STREAM_SPX:
			return ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_OGG), pTagInfo);

		case BASS_CTYPE_STREAM_MP4: // iTunes metadata
		case BASS_CTYPE_STREAM_ALAC:
		case BASS_CTYPE_STREAM_AAC:
			return ReadTagSub((LPBYTE)BASS_ChannelGetTags(handle, BASS_TAG_MP4), pTagInfo);

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
