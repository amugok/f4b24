	bits	32

	section	.data

	global	__fltused

__fltused:
	dd	1

	section	.text

	global	_memset
	global	_memcpy

_memset:
	cld
	push	edi
	mov	edi,[esp + 4 + 4]
	movzx	eax,byte [esp + 8 + 4]
	mov	ecx,[esp + 0ch + 4]
	cmp     ecx,12
	jb	.skipdword

	; - i386/486 Pentium/wMMX/M(Dothan or later)/4 Core/2/i7
	mov	ah,al
	; avoiding for partial register stall - PentiumPro/2/3/M(Tualatin or before)
	;mov	edx,eax
	;shl	edx,8
	;or	eax,edx

	mov	edx,eax
	shl	edx,16
	or	eax,edx

	mov	edx,ecx
	and	edx,3
	shr	ecx,2
	rep stosd

	mov	ecx,edx

.skipdword:
	rep stosb

	mov	eax,[esp + 4 + 4]
	pop	edi

	ret

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
