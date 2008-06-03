#include "../fittle/src/aplugin.h"
#include <shlwapi.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../../extra/cabsdk/FDI.h"

#if defined(_MSC_VER)
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(linker, "/EXPORT:GetAPluginInfo=_GetAPluginInfo@0")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/OPT:NOWIN98")
#endif


static HMODULE hDLL = 0;

typedef HFDI (FAR DIAMONDAPI * LPFNFDICREATE)(PFNALLOC pfnalloc,PFNFREE pfnfree,PFNOPEN pfnopen,PFNREAD pfnread,PFNWRITE pfnwrite,PFNCLOSE pfnclose,PFNSEEK pfnseek,int cpuType,PERF perf);
typedef BOOL (FAR DIAMONDAPI * LPFNFDICOPY)(HFDI hfdi,char FAR *pszCabinet,char FAR *pszCabPath, int flags, PFNFDINOTIFY pfnfdin, PFNFDIDECRYPT pfnfdid, void FAR *pvUser);
typedef BOOL (FAR DIAMONDAPI * LPFNFDIDESTROY)(HFDI hfdi);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	(void)lpvReserved;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		hDLL = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
	}
	return TRUE;
}

static BOOL CALLBACK IsArchiveExt(char *pszExt){
	if(lstrcmpi(pszExt, "cab")==0){
		return TRUE;
	}
	return FALSE;
}

static char * CALLBACK CheckArchivePath(char *pszFilePath)
{
	char *p = StrStrI(pszFilePath, ".cab/");
	return p;
}

static FNALLOC(Xmalloc)
{
	return HeapAlloc(GetProcessHeap(), 0, cb);
}
static FNFREE(Xfree)
{
	if (pv) HeapFree(GetProcessHeap(), 0, pv);
}

class CFdiFile
{
private:
	HANDLE m_h;

	void *m_p;
	off_t m_c;
	off_t m_s;

	bool IsFile()
	{
		return m_h != INVALID_HANDLE_VALUE;
	}
	bool IsMemory()
	{
		return m_p != 0;
	}
	void Close()
	{
		if (IsFile())
		{
			CloseHandle(m_h);
			m_h = INVALID_HANDLE_VALUE;
		}
		if (m_p)
		{
			Xfree(m_p);
			m_p = 0;
		}
	}
public:
	bool Allocate(off_t s)
	{
		Close();
		m_p = Xmalloc(s);
		if (!m_p) return false;
		m_s = s;
		m_c = 0;
		return true;
	}
	void *Detatch()
	{
		void *ret = m_p;
		m_p = 0;
		m_s = 0;
		m_c = 0;
		return ret;
	}
	bool Open(char *pszFile, int oflag = O_RDONLY, int pmode = S_IREAD)
	{
		Close();
		if ((oflag & 15) == O_RDONLY)
		{
			m_h = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			return IsFile();
		}
		return false;
	}
	size_t Read(void *pv, size_t sz)
	{
		if (IsFile())
		{
			DWORD dwNumberOfBytesRead = 0;
			ReadFile(m_h, pv, sz, &dwNumberOfBytesRead, 0);
			return dwNumberOfBytesRead;
		}
		if (IsMemory())
		{
			if (sz > m_s - m_c) sz = m_s - m_c;
			CopyMemory(pv, ((char *)m_p) + m_c, sz);
			m_c += sz;
			return sz;
		}
		return 0;
	}
	size_t Write(void *pv, size_t sz)
	{
		if (IsFile())
		{
			return 0;
//			DWORD dwNumberOfBytesWritten = 0;
//			WriteFile(m_h, pv, sz, &dwNumberOfBytesWritten, 0);
//			return dwNumberOfBytesWritten;
		}
		if (IsMemory())
		{
			if (sz > m_s - m_c) sz = m_s - m_c;
			CopyMemory(((char *)m_p) + m_c, pv, sz);
			m_c += sz;
			return sz;
		}
		return 0;
	}
	long Seek(long dist, int seektype)
	{
		if (IsFile())
		{
			DWORD dwDistanceToMove = 0;
			LONG dwDistanceToMoveHigh = 0;
			dwDistanceToMove = SetFilePointer(m_h, dist, &dwDistanceToMoveHigh, seektype);
			return dwDistanceToMove | (dwDistanceToMoveHigh << 32);
		}
		if (IsMemory())
		{
			switch (seektype)
			{
			case /*SEEK_SET*/0:
				if (0 <= dist && dist < m_s)
				{
					m_c = dist;
					return m_c;
				}
				return -1L;
			case /*SEEK_CUR*/1:
				if (0 <= m_c + dist && m_c + dist < m_s)
				{
					m_c = m_c + dist;
					return m_c;
				}
				return -1L;
			case /*SEEK_END*/2:
				if (0 <= m_s + dist && m_s + dist < m_s)
				{
					m_c = m_s + dist;
					return m_c;
				}
				return -1L;
			}
		}
		return 0;
	}

	bool SetPos(off_t ofs)
	{
		if (ofs != Seek(ofs, 0)) return false;
		return true;
	}

	bool GetWORDLE(WORD &ret)
	{
		unsigned char buf[2];
		if (2 != Read(buf, 2)) return false;
		ret = (buf[1] << (1*8)) | (buf[0] << (0*8));
		return true;
	}

	bool GetDWORDLE(DWORD &ret)
	{
		unsigned char buf[4];
		if (4 != Read(buf, 4)) return false;
		ret = (buf[3] << (3*8)) | (buf[2] << (2*8)) | (buf[1] << (1*8)) | (buf[0] << (0*8));
		return true;
	}

private:
	void * operator new(size_t s)
	{
		return Xmalloc(s);
	}

	void operator delete(void *p)
	{
		Xfree(p);
	}

	static FNOPEN(Xopen)
	{
		CFdiFile *pThis = new CFdiFile;
		if (!pThis->Open(pszFile, oflag, pmode))
		{
			delete pThis;
			return -1;
		}
		return (INT_PTR)pThis;
	}
	static FNREAD(Xread)
	{
		return ((CFdiFile *)hf)->Read(pv, cb);
	}
	static FNWRITE(Xwrite)
	{
		return ((CFdiFile *)hf)->Write(pv, cb);
	}
	static FNCLOSE(Xclose)
	{
		CFdiFile *pThis = (CFdiFile *)hf;
		delete pThis;
		return 0;
	}
	static FNSEEK(Xseek)
	{
		return ((CFdiFile *)hf)->Seek(dist, seektype);
	}

public:
	CFdiFile() : m_h(INVALID_HANDLE_VALUE), m_p(0)
	{
	}

	~CFdiFile()
	{
		Close();
	}

	static HFDI FDICreateGate(LPFNFDICREATE lpfn, PERF perf)
	{
		return lpfn(Xmalloc, Xfree, Xopen, Xread, Xwrite, Xclose, Xseek, cpu80386, perf);
	}
};

class CFdi
{
private:

	/* cabinet.dll fdi.dll */
	HMODULE m_hfdidll;
	LPFNFDICREATE m_lpfncreate;
	LPFNFDICOPY m_lpfncopy;
	LPFNFDIDESTROY m_lpfndestroy;

	ERF m_erf;
	HFDI m_hfdi;
	char *m_pszFileName;
	CFdiFile m_fout;
	DWORD m_dwSize;
	void *m_pBuf;

	void FreeDll()
	{
		if (m_hfdidll)
		{
			FreeLibrary(m_hfdidll);
			m_hfdidll = 0;
		}
	}
	bool LoadDll(char *pszName)
	{
		HMODULE hDll = LoadLibrary(pszName);
		if (hDll)
		{
			m_lpfncreate = (LPFNFDICREATE)GetProcAddress(hDll, "FDICreate");
			m_lpfncopy = (LPFNFDICOPY)GetProcAddress(hDll, "FDICopy");
			m_lpfndestroy = (LPFNFDIDESTROY)GetProcAddress(hDll, "FDIDestroy");
			if (m_lpfncreate && m_lpfncopy && m_lpfndestroy)
			{
				m_hfdidll = hDll;
				return true;
			}
			FreeLibrary(hDll);
		}
		return false;
	}
	bool LoadDll()
	{
		return m_hfdidll || LoadDll("cabinet.dll") || LoadDll("fdi.dll");
	}
	void Close()
	{
		if (m_hfdi)
		{
			m_lpfndestroy(m_hfdi);
			m_hfdi = 0;
		}
	}
	bool Init()
	{
		Close();
		if (!LoadDll()) return false;
		m_hfdi = CFdiFile::FDICreateGate(m_lpfncreate, &m_erf);
		return m_hfdi != 0;
	}
	INT_PTR Notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
	{
		switch (fdint)
		{
		case fdintCABINET_INFO:
			break;
		case fdintCOPY_FILE:
			if (lstrcmpi(m_pszFileName, pfdin->psz1) == 0)
			{
				m_dwSize =  pfdin->cb;
				if (!m_fout.Allocate(m_dwSize))
					return -1;
				return (INT_PTR)&m_fout;
			}
			break;
		case fdintCLOSE_FILE_INFO:
			if (m_pBuf)
			{
				Xfree(m_pBuf);
				m_pBuf = 0;
			}
			m_pBuf = m_fout.Detatch();
			return -1;
		}
		return 0;
	}
	static FNFDINOTIFY(Xnotify)
	{
		CFdi *pThis = (CFdi *)pfdin->pv;
		return pThis->Notify(fdint, pfdin);
	}
public:
	CFdi() : m_hfdi(0), m_pBuf(0), m_hfdidll(0)
	{
	}
	~CFdi()
	{
		Close();
		FreeDll();
	}
	bool Extract(char *pszArchivePath, char *pszFileName, void **ppBuf, DWORD *pSize)
	{
		if (!Init()) return false;
		m_pszFileName = pszFileName;
		m_pBuf = 0;
		m_lpfncopy(m_hfdi, pszArchivePath, "", 0, Xnotify, 0, this);
		if (m_pBuf)
		{
			*ppBuf = m_pBuf;
			*pSize = m_dwSize;
			m_pBuf = 0;
			return true;
		}
		return false;
	}
};

static BOOL CALLBACK EnumArchive(char *pszArchivePath, LPFNARCHIVEENUMPROC lpfnProc, void *pData)
{
	CFdiFile cab;
	if (!cab.Open(pszArchivePath)) return FALSE;
	DWORD dwSignature;
	DWORD dwCoffFiles;
	WORD wCFiles;
	if (!cab.GetDWORDLE(dwSignature) || dwSignature != 0x4643534d) return FALSE;
	if (!cab.SetPos(16) || !cab.GetDWORDLE(dwCoffFiles)) return FALSE;
	if (!cab.SetPos(24) || !cab.GetWORDLE(wCFiles)) return FALSE;
	DWORD dwFileOfs = dwCoffFiles;
	for (int i = 0; i < wCFiles; i++)
	{
		if (!cab.SetPos(dwFileOfs)) return FALSE;
		DWORD dwSize;
		if (!cab.GetDWORDLE(dwSize)) return FALSE;
		DWORD dwFolder;
		if (!cab.GetDWORDLE(dwFolder)) return FALSE;
		WORD wFolder;
		if (!cab.GetWORDLE(wFolder)) return FALSE;
		WORD wDate;
		if (!cab.GetWORDLE(wDate)) return FALSE;
		WORD wTime;
		if (!cab.GetWORDLE(wTime)) return FALSE;
		WORD wAttr;
		if (!cab.GetWORDLE(wAttr)) return FALSE;
		char name[MAX_PATH + 1];
		size_t l = cab.Read(name, MAX_PATH);
		if (l < MAX_PATH + 1) name[l] = 0;

		FILETIME ft;
		DosDateTimeToFileTime(wDate, wTime, &ft);
		lpfnProc(name, dwSize, ft, pData);

		dwFileOfs += 4 + 4 + 2 + 2 + 2 + 2 + lstrlen(name) + 1;
	}
	return TRUE;
}

static BOOL CALLBACK ExtractArchive(char *pszArchivePath, char *pszFileName, void **ppBuf, DWORD *pSize)
{
	CFdi fdi;
	if (!fdi.Extract(pszArchivePath, pszFileName, ppBuf, pSize)) return FALSE;
	return TRUE;
}

static ARCHIVE_PLUGIN_INFO apinfo = {
	0,
	APDK_VER,
	IsArchiveExt,
	CheckArchivePath,
	EnumArchive,
	ExtractArchive
};

static BOOL InitArchive(){	
	return TRUE;
}

extern "C" ARCHIVE_PLUGIN_INFO * CALLBACK GetAPluginInfo(void)
{
	return InitArchive() ? &apinfo : 0;
}
