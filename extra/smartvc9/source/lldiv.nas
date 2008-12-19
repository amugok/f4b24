;       Copyright (c) Microsoft Corporation. All rights reserved.

	bits	32
	section	.text

	global	__alldiv

__alldiv:
	push	edi
	push	esi
	push	ebx
	xor	edi,edi
	mov	eax,[esp + 014h]
	or	eax,eax
	jge	.l21
	inc	edi
	mov	edx,[esp + 010h]
	neg	eax
	neg	edx
	sbb	eax,0
	mov	[esp + 014h],eax
	mov	[esp + 010h],edx
.l21:
	mov	eax,[esp + 01ch]
	or	eax,eax
	jge	.l3d
	inc	edi
	mov	edx,[esp + 018h]
	neg	eax
	neg	edx
	sbb	eax,0
	mov	[esp + 01ch],eax
	mov	[esp + 018h],edx
.l3d:
	or	eax,eax
	jne	.l59
	mov	ecx,[esp + 018h]
	mov	eax,[esp + 014h]
	xor	edx,edx
	div	ecx
	mov	ebx,eax
	mov	eax,[esp + 010h]
	div	ecx
	mov	edx,ebx
	jmp	short .l9a
.l59:
	mov	ebx,eax
	mov	ecx,[esp + 018h]
	mov	edx,[esp + 014h]
	mov	eax,[esp + 010h]
.l67:
	shr	ebx,1
	rcr	ecx,1
	shr	edx,1
	rcr	eax,1
	or	ebx,ebx
	jne	.l67
	div	ecx
	mov	esi,eax
	mul	dword [esp + 01ch]
	mov	ecx,eax
	mov	eax,[esp + 018h]
	mul	esi
	add	edx,ecx
	jb	.l95
	cmp	edx,[esp + 014h]
	ja	.l95
	jb	.l96
	cmp	eax,[esp + 010h]
	jbe	.l96
.l95:
	dec	esi
.l96:
	xor	edx,edx
	mov	eax,esi
.l9a:
	dec	edi
	jne	.la4
	neg	edx
	neg	eax
	sbb	edx,0
.la4:
	pop	ebx
	pop	esi
	pop	edi
	ret	010h
