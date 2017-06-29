
#ifndef FNAME_MAX32
#define FNAME_MAX32		512
#endif

#ifndef M_CHECK_FILENAME_ONLY
#define M_CHECK_FILENAME_ONLY	0x00000200L
#endif

typedef	HGLOBAL	HARC;

typedef struct {
	DWORD	dwOriginalSize;
	DWORD	dwCompressedSize;
	DWORD	dwCRC;
	UINT	uFlag;
	UINT	uOSType;
	WORD	wRatio;
	WORD	wDate;
	WORD	wTime;
	char	szFileName[FNAME_MAX32 + 1];
	char	dummy1[3];
	char	szAttribute[8];
	char	szMode[8];
} INDIVIDUALINFOA, *LPINDIVIDUALINFOA;
