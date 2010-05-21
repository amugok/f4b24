#ifndef READTAG_H_INC_
#define READTAG_H_INC_

#define BASS_CTYPE_STREAM		0x10000
#define BASS_CTYPE_STREAM_OGG	0x10002
#define BASS_CTYPE_STREAM_MP1	0x10003
#define BASS_CTYPE_STREAM_MP2	0x10004
#define BASS_CTYPE_STREAM_MP3	0x10005
#define BASS_CTYPE_STREAM_AIFF	0x10006
#define BASS_CTYPE_STREAM_CD	0x10200
#define BASS_CTYPE_STREAM_WMA	0x10300
#define BASS_CTYPE_STREAM_WMA_MP3	0x10301
#define BASS_CTYPE_STREAM_WINAMP	0x10400
#define BASS_CTYPE_STREAM_WV	0x10500 // WavPack Lossless
#define BASS_CTYPE_STREAM_WV_H	0x10501 // WavPack Hybrid Lossless
#define BASS_CTYPE_STREAM_WV_L	0x10502 // WavPack Lossy
#define BASS_CTYPE_STREAM_WV_LH	0x10503 // WavPack Hybrid Lossy
#define BASS_CTYPE_STREAM_OFR	0x10600
#define BASS_CTYPE_STREAM_APE	0x10700
#define BASS_CTYPE_STREAM_FLAC	0x10900
#define BASS_CTYPE_STREAM_FLAC_OGG	0x10901
#define BASS_CTYPE_STREAM_MPC	0x10a00
#define BASS_CTYPE_STREAM_AAC	0x10b00 // AAC
#define BASS_CTYPE_STREAM_MP4	0x10b01 // MP4
#define BASS_CTYPE_STREAM_SPX	0x10c00
#define BASS_CTYPE_STREAM_MIDI	0x10d00
#define BASS_CTYPE_STREAM_ALAC	0x10e00
#define BASS_CTYPE_STREAM_TTA	0x10f00
#define BASS_CTYPE_STREAM_AC3	0x11000
#define BASS_CTYPE_STREAM_TAK	0x12000
#define BASS_CTYPE_STREAM_WAV	0x40000
#define BASS_CTYPE_STREAM_WAV_PCM	0x50001
#define BASS_CTYPE_STREAM_WAV_FLOAT	0x50003
#define BASS_CTYPE_STREAM_WAV_MP3	0x50055

#define BASS_CTYPE_MUSIC_MOD	0x20000
#define BASS_CTYPE_MUSIC_MTM	0x20001
#define BASS_CTYPE_MUSIC_S3M	0x20002
#define BASS_CTYPE_MUSIC_XM		0x20003
#define BASS_CTYPE_MUSIC_IT		0x20004
#define BASS_CTYPE_MUSIC_MO3	0x00100 // MO3 flag

// AC-3—p
#define BASS_CONFIG_AC3_DOWNMIX		0x10000
#define BASS_AC3_DOWNMIX_2		0x200	// downmix to stereo

// BASS_StreamGetTags types : what's returned

#define BASS_TAG_ID3		0	// ID3v1 tags : 128 byte block
#define BASS_TAG_ID3V2		1	// ID3v2 tags : variable length block
#define BASS_TAG_OGG		2	// OGG comments : series of null-terminated UTF-8 strings
#define BASS_TAG_HTTP		3	// HTTP headers : series of null-terminated ANSI strings
#define BASS_TAG_ICY		4	// ICY headers : series of null-terminated ANSI strings
#define BASS_TAG_META		5	// ICY metadata : ANSI string
#define BASS_TAG_APE		6	// APE tags
#define BASS_TAG_MP4 		7	// MP4/iTunes metadata
#define BASS_TAG_WMA		8	// WMA header tags : series of null-terminated UTF-8 strings
#define BASS_TAG_VENDOR		9	// OGG encoder : UTF-8 string
#define BASS_TAG_LYRICS3	10	// Lyric3v2 tag : ASCII string
#define BASS_TAG_WMA_META	11	// WMA mid-stream tag : UTF-8 string
#define BASS_TAG_RIFF_INFO	0x100 // RIFF/WAVE tags : series of null-terminated ANSI strings
#define BASS_TAG_MUSIC_NAME		0x10000	// MOD music name : ANSI string
#define BASS_TAG_MUSIC_MESSAGE	0x10001	// MOD message : ANSI string
#define BASS_TAG_MUSIC_INST		0x10100	// + instrument #, MOD instrument name : ANSI string
#define BASS_TAG_MUSIC_SAMPLE	0x10300	// + sample #, MOD sample name : ANSI string
#define BASS_TAG_MIDI_TRACK	0x11000 	// + track #, track text : array of null-terminated ANSI strings

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

#ifdef UNICODE
#else
static BOOL UTF8ToMultiByte(LPCSTR lpszSrc, int nSrcSize, LPSTR lpszDst, int nDstSize){
	int lw = MultiByteToWideChar(CP_UTF8, 0, lpszSrc, nSrcSize, 0, 0);
	LPWSTR pa;
	if (lw == 0) return FALSE;
	pa = (LPWSTR)HZAlloc(lw * sizeof(WCHAR));
	if (!pa) return FALSE;
	MultiByteToWideChar(CP_UTF8, 0, lpszSrc, nSrcSize, pa, lw);
	WideCharToMultiByte(CP_ACP, 0, pa, lw, lpszDst, nDstSize, NULL, NULL);
	HFree(pa);
	return TRUE;
}
#endif

#define XMIN(x,y) ((x>y)?y:x)
static BOOL ID3_ReadFrameText(BYTE bType, LPBYTE pTextRaw, int nTextRawSize, LPTSTR pszBuf, int nBufSize){
	ZeroMemory(pszBuf, nBufSize * sizeof(TCHAR));
	if ((bType == 3) || (bType != 1 && bType != 2 && IsBomUTF8(pTextRaw, nTextRawSize))){
		/* UTF8 */
		if (IsBomUTF8(pTextRaw, nTextRawSize)){
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
			LPBYTE pa = (LPBYTE)HZAlloc(nTextRawSize);
			if (pa){
				int l = nTextRawSize >> 1;
				int i;
				for (i = 0; i < l; i++){
					pa[i * 2 + 0] = pTextRaw[i * 2 + 1];
					pa[i * 2 + 1] = pTextRaw[i * 2 + 0];
				}
				ID3_ReadFrameText(1, pa, nTextRawSize, pszBuf, nBufSize);
				HFree(pa);
				return TRUE;
			}
			return FALSE;
		}
#ifdef UNICODE
		CopyMemory(pszBuf, pTextRaw, XMIN((nTextRawSize >> 1), nBufSize) * sizeof(WCHAR));
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
	LPBYTE lpUnsync = (LPBYTE)HZAlloc(nSrcLen ? nSrcLen : nDstLmt);
	if (lpUnsync) {
		unsigned i, j;
		for (i = j = 0; (nSrcLen ? (i < nSrcLen) : (j < nDstLmt)); i++){
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
	if (nFlag & 1){
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

#endif
