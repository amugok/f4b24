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

