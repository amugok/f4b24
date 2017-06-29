;       Copyright (c) Microsoft Corporation. All rights reserved.

	bits	32
	section	.text

	global	__ftol

__ftol:
	push	ebp
	mov	ebp,esp
	sub	esp,0ch
	wait
	fstcw	[ebp - 2]
	wait
	mov	ax,[ebp - 2]
	or	ah,0ch
	mov	[ebp - 4],ax
	fldcw	[ebp - 4]
	fistp	qword [ebp - 0ch]
	fldcw	[ebp - 2]
	mov	eax,[ebp - 0ch]
	mov	edx,[ebp - 8]
	leave
	ret
