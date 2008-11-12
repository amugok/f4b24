#ifndef BASSLOAD_H_INC_
#define BASSLOAD_H_INC_

#ifdef _WIN32
#include <wtypes.h>
typedef unsigned __int64 QWORD;
#endif
#define BASSDEF(f) (WINAPI * f)

#define BASSVERSION 0x204

#define BASS_CONFIG_NET_TIMEOUT		11
#define BASS_SAMPLE_8BITS		1
#define BASS_SAMPLE_FLOAT		256
#define BASS_STREAM_BLOCK		0x100000
#define BASS_STREAM_DECODE		0x200000
#define BASS_STREAM_PRESCAN		0x20000
#define BASS_MUSIC_DECODE		BASS_STREAM_DECODE
#define BASS_MUSIC_PRESCAN		BASS_STREAM_PRESCAN
#define BASS_UNICODE			0x80000000

#define BASS_STREAMPROC_END		0x80000000

#define BASS_FILEPOS_END		2
#define BASS_SYNC_POS			0
#define BASS_SYNC_END			2
#define BASS_POS_BYTE			0

typedef struct {
	DWORD ctype;
	const char *name;
	const char *exts;
} BASS_PLUGINFORM;

typedef struct {
	DWORD version;
	DWORD formatc;
	const BASS_PLUGINFORM *formats;
} BASS_PLUGININFO;

typedef struct {
	DWORD freq;
	DWORD chans;
	DWORD flags;
	DWORD ctype;
	DWORD origres;
	DWORD plugin;
	DWORD sample;
	const char *filename;
} BASS_CHANNELINFO;

typedef void (CALLBACK SYNCPROC)(DWORD handle, DWORD channel, DWORD data, void *user);
typedef void (CALLBACK DOWNLOADPROC)(const void *buffer, DWORD length, void *user);

extern double BASSDEF(BASS_ChannelBytes2Seconds)(DWORD handle, QWORD pos);
extern BOOL BASSDEF(BASS_ChannelGetInfo)(DWORD handle, BASS_CHANNELINFO *info);
extern QWORD BASSDEF(BASS_ChannelGetLength)(DWORD handle, DWORD mode);
extern QWORD BASSDEF(BASS_ChannelGetPosition)(DWORD handle, DWORD mode);
extern const char *BASSDEF(BASS_ChannelGetTags)(DWORD handle, DWORD tags);
extern BOOL BASSDEF(BASS_ChannelPlay)(DWORD handle, BOOL restart);
extern QWORD BASSDEF(BASS_ChannelSeconds2Bytes)(DWORD handle, double pos);
extern BOOL BASSDEF(BASS_ChannelSetPosition)(DWORD handle, QWORD pos, DWORD mode);
extern DWORD BASSDEF(BASS_ChannelSetSync)(DWORD handle, DWORD type, QWORD param, SYNCPROC *proc, void *user);
extern BOOL BASSDEF(BASS_ChannelStop)(DWORD handle);
extern BOOL BASSDEF(BASS_Free)();
extern DWORD BASSDEF(BASS_GetVersion)();
extern BOOL BASSDEF(BASS_Init)(int device, DWORD freq, DWORD flags, HWND win, const GUID *dsguid);
extern BOOL BASSDEF(BASS_MusicFree)(DWORD handle);
extern DWORD BASSDEF(BASS_MusicLoad)(BOOL mem, const void *file, QWORD offset, DWORD length, DWORD flags, DWORD freq);
extern const BASS_PLUGININFO *BASSDEF(BASS_PluginGetInfo)(DWORD handle);
extern DWORD BASSDEF(BASS_PluginLoad)(const char *file, DWORD flags);
extern BOOL BASSDEF(BASS_SetConfig)(DWORD option, DWORD value);
extern DWORD BASSDEF(BASS_StreamCreateFile)(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags);
extern DWORD BASSDEF(BASS_StreamCreateURL)(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user);
extern BOOL BASSDEF(BASS_StreamFree)(DWORD handle);
extern QWORD BASSDEF(BASS_StreamGetFilePosition)(DWORD handle, DWORD mode);

extern BOOL LoadBASS(void);

#endif
