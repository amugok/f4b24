;       copyright (c) microsoft corporation. all rights reserved.

	bits	32
	section	.text

	global	__aulldiv

__aulldiv:
	push	ebx
	push	esi
	mov	eax,[esp + 018h]
	or	eax,eax
	jne	.l22
	mov	ecx,[esp + 014h]
	mov	eax,[esp + 010h]
	xor	edx,edx
	div	ecx
	mov	ebx,eax
	mov	eax,[esp + 0ch]
	div	ecx
	mov	edx,ebx
	jmp	short .l63
.l22:
	mov	ecx,eax
	mov	ebx,[esp + 014h]
	mov	edx,[esp + 010h]
	mov	eax,[esp + 0ch]
.l30:
	shr	ecx,1
	rcr	ebx,1
	shr	edx,1
	rcr	eax,1
	or	ecx,ecx
	jne	.l30
	div	ebx
	mov	esi,eax
	mul	dword [esp + 018h]
	mov	ecx,eax
	mov	eax,[esp + 014h]
	mul	esi
	add	edx,ecx
	jb	.l5e
	cmp	edx,[esp + 010h]
	ja	.l5e
	jb	.l5f
	cmp	eax,[esp + 0ch]
	jbe	.l5f
.l5e:
	dec	esi
.l5f:
	xor	edx,edx
	mov	eax,esi
.l63:
	pop	esi
	pop	ebx
	ret	010h
