;       Copyright (c) Microsoft Corporation. All rights reserved.

	bits	32
	section	.text

	global	__alloca_probe
	global	__chkstk

__alloca_probe:
__chkstk:

	push	ecx
	cmp	eax,01000h
	lea	ecx,[esp + 8]
	jb	.l20
.lc:
	sub	ecx,01000h
	sub	eax,01000h
	test	[ecx],eax
	cmp	eax,01000h
	jae	.lc
.l20:
	sub	ecx,eax
	mov	eax,esp
	test	[ecx],eax
	mov	esp,ecx
	mov	ecx,[eax]
	mov	eax,[eax + 4]
	push	eax
	ret
