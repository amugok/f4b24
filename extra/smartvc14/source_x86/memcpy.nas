	bits	32

	section	.text

	global	_memcpy

_memcpy:
	cld
	push	edi
	push	esi
	mov	edi,[esp + 4 + 8]
	mov	esi,[esp + 8 + 8]
	mov	ecx,[esp + 0ch + 8]
	mov	edx,ecx
	mov	eax,edi
	shr	ecx,2
	rep movsd

	test	dl,2
	jz	.skipword
	movsw
.skipword:

	test	dl,1
	jz	.skipbyte
	movsb
.skipbyte:

	pop	esi
	pop	edi
	ret
