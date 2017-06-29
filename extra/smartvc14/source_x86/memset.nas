	bits	32

	section	.text

	global	_memset

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
