/*
 * bass_tag.cpp
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#include "fittle.h"
#include "bass_tag.h"

static unsigned int GetSyncSafeInt(unsigned char* b){
	return (b[0]<<24) | (b[1]<<16) | (b[2]<<8) | (b[3]);
}

/*static unsigned GetSyncSafeInt(unsigned char* b){
    return ((b[0] & 0x7f)<<21) + ((b[1] & 0x7f)<<14)
             +((b[2] & 0x7f)<<7) + (b[3] & 0x7f);
}*/

static unsigned int GetSyncSafeInt22(unsigned char* b){
	return (b[0]<<16) | (b[1]<<8) | (b[2]);
}

/*static unsigned GetSyncSafeInt22(unsigned char* b){
    return ((b[0] & 0x7f)<<14) + ((b[1] & 0x7f)<<7) + (b[2] & 0x7f);
}*/

static BOOL UTF8toShift_Jis(LPSTR szOut, LPCSTR szIn){
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

	WideCharToMultiByte(CP_ACP, 0, wTmp, lstrlenW(wTmp), szOut, 256*sizeof(char), NULL, NULL);
	szOut[256-1] = TEXT('\0');
	HeapFree(GetProcessHeap(), 0, wTmp);

	return TRUE;
}

#define XMIN(x,y) ((x>y)?y:x)

static BOOL ID3V23_ReadFlame(LPCSTR pszFlame, int nFlameSize, LPTSTR pszTag){
	// 文字コード指定子を考慮
	switch(*(pszFlame + 10)){
		case 0:	// ANSI
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, 0, pszFlame + 11, nFlameSize, pszTag, 256);
#else
			lstrcpyn(pszTag, pszFlame + 11, XMIN(nFlameSize,256));
#endif
			break;
		case 1:	// UTF-16LE
			{
				int l = (nFlameSize - 2)/2;
				LPWSTR pw = (LPWSTR)(pszFlame + 11 + 2);
				if (pw[0] == 0xfeff){
					pw++;
					l--;
				}
	#ifdef UNICODE
				lstrcpyn(pszTag, pw, XMIN(l,256));
	#else
				WideCharToMultiByte(CP_ACP, 0, pw, l, pszTag, 256, NULL, NULL);
	#endif
			}
			break;
		case 2: // UTF-16BE
			{
				int l = (nFlameSize - 2)/2;
				LPWSTR pa = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * l);
				if (pa){
					int i;
					LPWSTR pw = pa;
					for (i = 0; i < l; i++){
						pw[i] = (((WORD)(unsigned char)*(pszFlame + 11 + 2 + i * 2 + 0)) << 8)
						      | (((WORD)(unsigned char)*(pszFlame + 11 + 2 + i * 2 + 1)) << 0);
					}
					if (pw[0] == 0xfeff){
						pw++;
						l--;
					}
#ifdef UNICODE
					lstrcpyn(pszTag, pw, XMIN(l,256));
#else
					WideCharToMultiByte(CP_ACP, 0, pw, l, pszTag, 256, NULL, NULL);
#endif
					HeapFree(GetProcessHeap(), 0, pa);
				}
			}
			break;
		case 3:	// UTF-8
#ifdef UNICODE
			MultiByteToWideChar(CP_UTF8, 0, pszFlame + 11, nFlameSize, pszTag, 256);
#else
			UTF8toShift_Jis(pszTag, pszFlame + 11);
#endif
			break;
	}
	OutputDebugString(pszTag);
	OutputDebugString(TEXT("\n"));
	return TRUE;
}

static BOOL ID3V22_ReadFlame(LPCSTR pszFlame, int nFlameSize, LPTSTR pszTag){
	// フレームサイズの後の1byteは文字コード指定子（$00ならANSI $01ならUnicode）
	if(*(pszFlame + 6)){
#ifdef UNICODE
		lstrcpyn(pszTag, (WCHAR *)(pszFlame + 7 + 2), XMIN((nFlameSize - 2)/2,256));
#else
		WideCharToMultiByte(CP_ACP, 0, (WCHAR *)(pszFlame + 7 + 2), (nFlameSize - 2)/2, pszTag, 256, NULL, NULL);
#endif
	}else{
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, 0, pszFlame + 7, nFlameSize, pszTag, 256);
#else
		lstrcpyn(pszTag, pszFlame + 7, nFlameSize);
#endif
	}
	OutputDebugString(pszTag);
	OutputDebugString(TEXT("\n"));
	return TRUE;
}

static BOOL ID3V1_ReadFlame(LPSTR pszFlame, LPTSTR pszTag){
	LPSTR p;

	// Ver.1.0は'\0'でなく' 'で埋めるためそれの対策
	for(p=&(pszFlame[29]);*p==' ';p--){
		*p = '\0';
	}
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, pszFlame, 30, pszTag, 256);
#else
	lstrcpyn(pszTag, pszFlame, 30);
#endif
	OutputDebugString(pszTag);
	OutputDebugString(TEXT("\n"));
	return TRUE;
}

static BOOL Vorbis_ReadFlame(LPCSTR pszFlame, LPCSTR pszFlameID, LPTSTR pszTag){
	int nIDLength = lstrlenA(pszFlameID);

	if(StrCmpNIA(pszFlame, pszFlameID, nIDLength)) return FALSE;
	
#ifdef UNICODE
	MultiByteToWideChar(CP_UTF8, 0, pszFlame + nIDLength, -1, pszTag, 256);
#else
	UTF8toShift_Jis(pszTag, pszFlame + nIDLength);
#endif
	OutputDebugString(pszTag);
	OutputDebugString(TEXT("\n"));

	return TRUE;
}

BOOL Riff_ReadFlame(LPCSTR pszFlame, LPCSTR pszFlameID, LPTSTR pszTag){
	int nIDLength = lstrlenA(pszFlameID);

	if(StrCmpNIA(pszFlame, pszFlameID, nIDLength)) return FALSE;
	
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, 0, pszFlame + nIDLength, -1, pszTag, 256);
#else
	lstrcpyn(pszTag, pszFlame + nIDLength, 256);
#endif
	OutputDebugString(pszTag);
	OutputDebugString(TEXT("\n"));

	return TRUE;
}

BOOL BASS_TAG_Read(DWORD handle, TAGINFO *pTagInfo){
	BASS_CHANNELINFO bcinfo = {0};
	ID3TAG *pID3Tag = NULL;
	LPCSTR p = NULL;

	ZeroMemory(pTagInfo, sizeof(TAGINFO));

	BASS_ChannelGetInfo(handle, &bcinfo);
	switch(bcinfo.ctype){
		case BASS_CTYPE_STREAM_AIFF:
		case BASS_CTYPE_STREAM_WAV:
		case BASS_CTYPE_STREAM_WAV_PCM:  
		case BASS_CTYPE_STREAM_WAV_FLOAT:  
/*		case BASS_CTYPE_STREAM_WAV_MP3:*/ /* removed in BASS 2.4 */
			p = BASS_ChannelGetTags(handle, BASS_TAG_RIFF_INFO);
			while(p && *p){
				Riff_ReadFlame(p, "INAM=", pTagInfo->szTitle);
				Riff_ReadFlame(p, "IART=", pTagInfo->szArtist);
				Riff_ReadFlame(p, "IPRD=", pTagInfo->szAlbum);
				p += lstrlenA(p) + 1;
			}
			if(*pTagInfo->szTitle == 0 || *pTagInfo->szArtist == 0) return FALSE;
			return TRUE;

		case BASS_CTYPE_STREAM_MP1:
		case BASS_CTYPE_STREAM_MP2:
		case BASS_CTYPE_STREAM_MP3:
		case BASS_CTYPE_STREAM_TTA:
			unsigned int nTagSize, nTotal;
			char szFlameID[5];
			int nFlameSize;

			// まずはv2タグを読む
			p = BASS_ChannelGetTags(handle, BASS_TAG_ID3V2);
			if(p && p[0] == 'I' && p[1] == 'D' && p[2] == '3'){
				nTotal = 10;	// ヘッダサイズを足しておく
				nTagSize = GetSyncSafeInt((unsigned char *)(p + 6));
				// フレームを前から順に取得
				if(*(p + 3)>=3){	// バージョンの取得
					while(nTotal<nTagSize){
						// Ver.2.3以上
						lstrcpynA(szFlameID, p + nTotal, 5); // フレームIDの取得
						nFlameSize = GetSyncSafeInt((unsigned char *)(p + nTotal + 4)); // フレームサイズの取得
						if(lstrlenA(szFlameID)!=4) break;

						if(!lstrcmpA(szFlameID, "TIT2"))
							ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTitle);
						else if(!lstrcmpA(szFlameID, "TPE1"))
							ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szArtist);
						else if(!lstrcmpA(szFlameID, "TALB"))
							ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szAlbum);
						else if(!lstrcmpA(szFlameID, "TRCK"))
							ID3V23_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTrack);

						nTotal += nFlameSize + 10;
					}
				}else{
					while(nTotal<nTagSize){
						// Ver.2.2以下
						lstrcpynA(szFlameID, p + nTotal, 4); // フレームIDの取得
						nFlameSize = GetSyncSafeInt22((unsigned char *)(p + nTotal + 3)); // フレームサイズの取得
						if(lstrlenA(szFlameID)!=3) break;

						if(!lstrcmpA(szFlameID, "TP1"))
							ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szArtist);
						else if(!lstrcmpA(szFlameID, "TT2"))
							ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTitle);
						else if(!lstrcmpA(szFlameID, "TAL"))
							ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szAlbum);
						else if(!lstrcmpA(szFlameID, "TRK"))
							ID3V22_ReadFlame(p + nTotal, nFlameSize, pTagInfo->szTrack);

						nTotal += nFlameSize + 6;
					}
				}
			}	
			if(*pTagInfo->szTitle != 0 || *pTagInfo->szArtist != 0) return TRUE;
			// breakしないで下のcaseに行く

		case BASS_CTYPE_STREAM_OFR:
		case BASS_CTYPE_STREAM_WV:
		case BASS_CTYPE_STREAM_WV_H:
		case BASS_CTYPE_STREAM_WV_L:
		case BASS_CTYPE_STREAM_WV_LH:
			// ここからv1タグを読み込み
			pID3Tag = (ID3TAG *)BASS_ChannelGetTags(handle, BASS_TAG_ID3);
			if(!pID3Tag) return FALSE;

			ID3V1_ReadFlame(pID3Tag->Title, pTagInfo->szTitle);
			ID3V1_ReadFlame(pID3Tag->Artist, pTagInfo->szArtist);
			ID3V1_ReadFlame(pID3Tag->Album, pTagInfo->szAlbum);
#ifdef UNICODE
			MultiByteToWideChar(CP_ACP, 0, pID3Tag->Track, 2, pTagInfo->szTrack, 256);
#else
			lstrcpyn(pTagInfo->szTrack, pID3Tag->Track, 2);
#endif

			if(*pTagInfo->szTitle == 0 || *pTagInfo->szArtist == 0) return FALSE;	// 両方とも空白だったら
			return TRUE;

		case BASS_CTYPE_STREAM_WMA:
			p = BASS_ChannelGetTags(handle, BASS_TAG_WMA);
			while(p && *p){
				Vorbis_ReadFlame(p, "Title : ", pTagInfo->szTitle);
				Vorbis_ReadFlame(p, "Author : ", pTagInfo->szArtist);
				Vorbis_ReadFlame(p, "Album : ", pTagInfo->szAlbum);
				Vorbis_ReadFlame(p, "WM/TrackNumber : ", pTagInfo->szTrack);
				p += lstrlenA(p) + 1;
			}
			if(*pTagInfo->szTitle == 0 || *pTagInfo->szArtist == 0) return FALSE;
			return TRUE;

		case BASS_CTYPE_STREAM_OGG:	// Vorbis comment
		case BASS_CTYPE_STREAM_FLAC:
		case BASS_CTYPE_STREAM_SPX:
			p = BASS_ChannelGetTags(handle, BASS_TAG_OGG);
			while(p && *p){
				Vorbis_ReadFlame(p, "TITLE=", pTagInfo->szTitle);
				Vorbis_ReadFlame(p, "ARTIST=", pTagInfo->szArtist);
				Vorbis_ReadFlame(p, "ALBUM=", pTagInfo->szAlbum);
				Vorbis_ReadFlame(p, "TRACKNUMBER=", pTagInfo->szTrack);
				p += lstrlenA(p) + 1;
			}
			if(*pTagInfo->szTitle == 0 || *pTagInfo->szArtist == 0) return FALSE;
			return TRUE;

		case BASS_CTYPE_STREAM_MP4: // iTunes metadata
		case BASS_CTYPE_STREAM_ALAC:
		case BASS_CTYPE_STREAM_AAC:
			p = BASS_ChannelGetTags(handle, BASS_TAG_MP4);
			while(p && *p){
				Vorbis_ReadFlame(p, "title=", pTagInfo->szTitle);
				Vorbis_ReadFlame(p, "artist=", pTagInfo->szArtist);
				Vorbis_ReadFlame(p, "album=", pTagInfo->szAlbum);
				Vorbis_ReadFlame(p, "track=", pTagInfo->szTrack);
				p += lstrlenA(p) + 1;
			}
			if(*pTagInfo->szTitle == 0 || *pTagInfo->szArtist == 0) return FALSE;
			return TRUE;

		case BASS_CTYPE_STREAM_APE:
		case BASS_CTYPE_STREAM_MPC:
			// APEタグには未対応
			p = BASS_ChannelGetTags(handle, BASS_TAG_APE);
			while(p && *p){
				Vorbis_ReadFlame(p, "Title=", pTagInfo->szTitle);
				Vorbis_ReadFlame(p, "Artist=", pTagInfo->szArtist);
				Vorbis_ReadFlame(p, "Album=", pTagInfo->szAlbum);
				Vorbis_ReadFlame(p, "Track=", pTagInfo->szTrack);
				p += lstrlenA(p) + 1;
			}
			if(*pTagInfo->szTitle == 0 || *pTagInfo->szArtist == 0) return FALSE;
			return TRUE;

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
	
	
	
	
	
	
	
	

