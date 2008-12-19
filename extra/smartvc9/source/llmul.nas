;       copyright (c) microsoft corporation. all rights reserved.

	bits	32
	section	.text

	global	__allmul

__allmul:
		mov	eax,[esp + 8]
		mov	ecx,[esp + 010h]
		or	ecx,eax
		mov	ecx,[esp + 0ch]
		jne	.l19
		mov	eax,[esp + 4]
		mul	ecx
		ret	010h
.l19:
		push	ebx
		mul	ecx
		mov	ebx,eax
		mov	eax,[esp + 4 + 4]
		mul	dword [esp + 4 + 010h]
		add	ebx,eax
		mov	eax,[esp + 4 + 4]
		mul	ecx
		add	edx,ebx
		pop	ebx
		ret	010h
