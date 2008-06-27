/*
 * bass_tag.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _BASS_TAG_H_
#define _BASS_TAG_H_

#include "windows.h"
#include "fittle.h"

#define BASS_CTYPE_STREAM		0x10000
#define BASS_CTYPE_STREAM_OGG	0x10002
#define BASS_CTYPE_STREAM_MP1	0x10003
#define BASS_CTYPE_STREAM_MP2	0x10004
#define BASS_CTYPE_STREAM_MP3	0x10005
#define BASS_CTYPE_STREAM_AIFF	0x10006
#define BASS_CTYPE_STREAM_TAK	0x100007
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
#define BASS_CTYPE_STREAM_WAV_MP3	0x50055

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

typedef struct{
	char ID3[3];
	char Title[30];
	char Artist[30];
	char Album[30];
	char Year[4];
	char Comment[28];
	char Track[2];
	char Genre;
}ID3TAG;

typedef struct{
	TCHAR szTitle[256];
	TCHAR szArtist[256];
	TCHAR szAlbum[256];
	TCHAR szTrack[10];
}TAGINFO;

BOOL BASS_TAG_Read(DWORD, TAGINFO *);

#endif
