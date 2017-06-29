;       Copyright (c) Microsoft Corporation. All rights reserved.

	bits	32
	section	.text

	global	__ftol2
	global	__ftol2_sse

__ftol2:
__ftol2_sse:

	push	ebp
	mov	ebp,esp
	sub	esp,020h
	and	esp,0fffffff0h
	fld	st0
	fst	dword [esp + 018h]
	fistp	qword [esp + 010h]
	fild	qword [esp+ 010h]
	mov	edx,[esp + 018h]
	mov	eax,[esp + 010h]
	test	eax,eax
	je	.l95
.l59:
	fsubp	st1
	test	edx,edx
	jns	.l7d
	fstp	dword [esp]
	mov	ecx,[esp]
	xor	ecx,080000000h
	add	ecx,07fffffffh
	adc	eax,0
	mov	edx,[esp + 014h]
	adc	edx,0
	jmp	short .la9
.l7d:
	fstp	dword [esp]
	mov	ecx,[esp]
	add	ecx,07fffffffh
	sbb	eax,0
	mov	edx,[esp + 014h]
	sbb	edx,0
	jmp	short .la9
.l95:
	mov	edx,[esp + 014h]
	test	edx,07fffffffh
	jne	.l59
	fstp	dword [esp + 018h]
	fstp	dword [esp + 018h]
.la9:
	leave
	ret

