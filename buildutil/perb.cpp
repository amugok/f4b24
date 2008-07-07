/*
	PE ReBuilder
*/
#if _MSC_VER >= 1200
#pragma comment(linker, "/OPT:NOWIN98")
#endif
#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker,"/MERGE:.rdata=.text")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char ministub[0x40] = {
	0x4D, 0x5A, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00, 
	0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 
	0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0xB8, 0x00, 0x4C, 0xCD, 0x21, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

static void setwordle(unsigned char *buf, unsigned long value){
	buf[0] = (unsigned char)((value >> (0 * 8)) & 0xff);
	buf[1] = (unsigned char)((value >> (1 * 8)) & 0xff);
}

static void setdwordle(unsigned char *buf, unsigned long value){
	buf[0] = (unsigned char)((value >> (0 * 8)) & 0xff);
	buf[1] = (unsigned char)((value >> (1 * 8)) & 0xff);
	buf[2] = (unsigned char)((value >> (2 * 8)) & 0xff);
	buf[3] = (unsigned char)((value >> (3 * 8)) & 0xff);
}

static unsigned long getdwordle(const unsigned char *buf){
	return (((unsigned long)buf[0]) << (0 * 8)) | (((unsigned long)buf[1]) << (1 * 8)) | (((unsigned long)buf[2]) << (2 * 8)) | (((unsigned long)buf[3]) << (3 * 8));
}

static unsigned long getwordle(const unsigned char *buf){
	return (((unsigned long)buf[0]) << (0 * 8)) | (((unsigned long)buf[1]) << (1 * 8));
}

static unsigned char *readfile(const char *path, size_t *psz){
	FILE *rfp = 0;
	unsigned char *rbf = 0;
	size_t rsz;
	rfp = fopen(path, "rb");
	if (!rfp) return 0;
	fseek(rfp, 0, SEEK_END);
	rsz = ftell(rfp);
	if (rsz <= 0) return 0;
	fseek(rfp, 0, SEEK_SET);
	rbf = (unsigned char *)malloc(rsz);
	if (!rbf){
		fclose(rfp);
		return 0;
	}
	if (rsz != fread(rbf, 1, rsz, rfp)){
		free(rbf);
		fclose(rfp);
		return 0;
	}
	fclose(rfp);
	*psz = rsz;
	return rbf;
}

static int writefile(const char *path, const unsigned char *pbuf, size_t wsz){
	FILE *wfp = 0;
	wfp = fopen(path, "wb");
	if (!wfp) return 0;
	if (wsz != fwrite(pbuf, 1, wsz, wfp)){
		fclose(wfp);
		return 0;
	}
	fclose(wfp);
	return 1;
}

static unsigned long realign(unsigned long sz, int s){
	return ((sz + ((1 << s) - 1)) >> s) << s;
}

static void clearres(unsigned char *rdir, unsigned long rofs){
	int d;
	int nd = getwordle(rdir + rofs + 14);
	setdwordle(rdir + rofs + 4, 0);	/* time stamp */
	for (d = 0; d < nd; d++)
	{
		unsigned long sofs = getdwordle(rdir + rofs + 16 + 8 * d + 4);
		if (sofs & 0x80000000) clearres(rdir, sofs & 0x7fffffff);
	}
}

static int perb(const char *path, int s, int dctmin, const char *v){
	unsigned char *rbf = 0;
	unsigned char *wbf = 0;
	size_t rsz;
	size_t wsz;

	unsigned long rpeofs;
	unsigned long wstubsz;
	unsigned long wcoffofs, rcoffofs;
	unsigned long woptofs, roptofs;
	unsigned long woptsz, roptsz;
	unsigned long wdctnum, rdctnum;
	unsigned long rsecnum;
	unsigned long rsec;
	unsigned long wsecofs, rsecofs;
	unsigned long whdsz;
	unsigned long wdofs;

	unsigned long erva = 0;
	unsigned long irva = 0;
	unsigned long rrva = 0;

	rbf = readfile(path, &rsz);
	if (!rbf) {
		fprintf(stderr, "ファイルを読み込めません(%s)\n", path);
		return 1;
	}

	wsz = rsz * 2;
	wbf = (unsigned char *)malloc(wsz);
	if (!wbf) {
		fprintf(stderr, "メモリが不足しています\n");
		free(rbf);
		return 2;
	}
	memset(wbf, 0, wsz);
	
	wstubsz = sizeof(ministub);
	memcpy(wbf, ministub, wstubsz);
	if (v) memcpy(wbf + 0x20, v, strlen(v));
	setdwordle(wbf + 0x3c, wstubsz);	/* peofs */
	memcpy(wbf + wstubsz, "PE\0\0", 4);
	wcoffofs = wstubsz + 4;

	rpeofs = getdwordle(rbf + 0x3c);
	rcoffofs = rpeofs + 4;

	memcpy(wbf + wcoffofs, rbf + rcoffofs, 20);
	setdwordle(wbf + wcoffofs + 4, 0); /* timestamp */

	woptofs = wcoffofs + 20;
	roptofs = rcoffofs + 20;
	roptsz = getwordle(rbf + rcoffofs + 16);
	rdctnum = getdwordle(rbf + roptofs + 92);

	if (0){
		woptsz = roptsz;
		wdctnum = rdctnum;
	}else{
		for (wdctnum = rdctnum; wdctnum > 0; wdctnum--)
		{
			unsigned long rva = getdwordle(rbf + roptofs + 96 + (wdctnum - 1) * 8);
			unsigned long size = getdwordle(rbf + roptofs + 96 + (wdctnum - 1) * 8 + 4);
			if (rva != 0 || size != 0) break;
		}
		if (wdctnum == 0) wdctnum = 1;
		woptsz = 96 + wdctnum * 8;
	}

	memcpy(wbf + woptofs, rbf + roptofs, woptsz);
	if (wdctnum < dctmin) wdctnum = dctmin;	/* 16より小さいとアイコンを読めない場合がある */
	woptsz = 96 + wdctnum * 8;

	setdwordle(wbf + woptofs + 64, 0);	/* checksum */
	setwordle(wbf + wcoffofs + 16, woptsz);
	setdwordle(wbf + woptofs + 92, wdctnum);

	rsecofs = roptofs + roptsz;
	wsecofs = woptofs + woptsz;
	rsecnum = getwordle(rbf + rcoffofs + 2);

	whdsz = realign(wsecofs + rsecnum * 40, s);
	setdwordle(wbf + woptofs + 36, 1 << s);
	setdwordle(wbf + woptofs + 60, whdsz);

	wdofs = whdsz;

	if (wdctnum >= 1){
		erva = getdwordle(rbf + roptofs + 96 + 0 * 8);
	}
	if (wdctnum >= 2){
		irva = getdwordle(rbf + roptofs + 96 + 1 * 8);
	}
	if (wdctnum >= 3){
		rrva = getdwordle(rbf + roptofs + 96 + 2 * 8);
	}

	for (rsec = 0; rsec < rsecnum; rsec++){
		unsigned long rdva = getdwordle(rbf + rsecofs + rsec * 40 + 12);
		unsigned long rdsz = getdwordle(rbf + rsecofs + rsec * 40 + 16);
		unsigned long rdofs = getdwordle(rbf + rsecofs + rsec * 40 + 20);
		unsigned long wdsz = realign(rdsz, s);
		memcpy(wbf + wsecofs + rsec * 40, rbf + rsecofs + rsec * 40, 40);
		setdwordle(wbf + wsecofs + rsec * 40 + 20, wdofs);

		while (wdsz >= (1 << s)) {
			int c = 0;
			unsigned long csz = realign(wdsz - (1 << s), s);
			unsigned long o;
			for (o = 0; o < (1 << s); o++){
				if (rbf[rdofs + csz + o] != 0) {
					c = 1;
					break;
				}
			}
			if (c) break;
			wdsz = csz;
			setdwordle(wbf + wsecofs + rsec * 40 + 16, wdsz);
		}

		if (wsz < wdofs + wdsz) {
			size_t nwsz = wdofs + wdsz;
			unsigned char *nwbf = (unsigned char *)realloc(wbf, nwsz); 
			if (!nwbf){
				fprintf(stderr, "メモリが不足しています\n");
				free(wbf);
				return 2;
			}
			memset(nwbf + wsz, 0, nwsz - wsz); /* 増分をクリア */
			wsz = nwsz;
			wbf = nwbf;
		}
		memcpy(wbf + wdofs, rbf + rdofs, rdsz);
		if (erva && rdva <= erva && erva < rdva + rdsz){
			setdwordle(wbf + wdofs + erva - rdva + 4, 0);	/* time stamp */
		}
		if (irva && rdva <= irva && irva < rdva + rdsz){
			unsigned long widt;
			for (widt = wdofs + irva - rdva; getdwordle(wbf + widt); widt += 20){
				setdwordle(wbf + widt + 4, 0);	/* time stamp */
			}
		}
		if (rrva && rdva <= rrva && rrva < rdva + rdsz){
			clearres(wbf + wdofs + rrva - rdva, 0);
		}
		wdofs += wdsz;
	}

	if (!writefile(path, wbf, wdofs)){
		fprintf(stderr, "ファイルが書き込めません(%s)\n", path);
		free(wbf);
		free(rbf);
		return 3;
	}
	free(wbf);
	free(rbf);
	return 0;
}

int main(int argc, char **argv){
	int i = 1;
	int s = 9;
	int d = 16;
	const char *v = 0;
	for (; i < argc; i++) {
		if (strcmpi(argv[i], "/s") == 0 && i + 1 < argc){
			s = atoi(argv[++i]);
			if (!s) s = 9;
			continue;
		} else if (strcmpi(argv[i], "/d") == 0 && i + 1 < argc){
			d = atoi(argv[++i]);
			if (!d) d = 16;
			continue;
		} else if (strcmpi(argv[i], "/v") == 0 && i + 1 < argc){
			v = argv[++i];
			continue;
		} else {
			perb(argv[i], s, d, v);
		}
	}
	return 0;
}

