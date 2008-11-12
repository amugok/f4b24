/*
 * bass_tag.h
 *
 * Copyright(C) 2004-2008 Mallow <mallow at livedoor.com>
 * All Rights Reserved
 */

#ifndef _BASS_TAG_H_
#define _BASS_TAG_H_

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
	DWORD dwLength;
}TAGINFO;

BOOL BASS_TAG_Read(DWORD, TAGINFO *);

#endif
