[bits 16]

mzhead:


.mz00_Magic:
	db	'MZ'
.mz02_Number_of_bytes_in_last_page:
	dw	(mzsize - mzhead) & 511
.mz04_Total_number_of_pages_in_executable:
	dw	(mzsize - mzhead + 511) / 512
.mz06_Number_of_relocation_entries:
	dw	0
.mz08_Header_size_in_paragraphs:
	dw	3
.mz0a_Minimum_paragraphs_of_memory_allocated_in_addition_to_the_code_size:
	dw	0100h
.mz0c_Maximum_number_of_paragraphs_allocated_in_addition_to_the_code_size:
	dw	0100h
.mz0e_Initial_SS_relative_to_start_of_executable:
	dw	0
.mz10_Initial_SP:
	dw	mzsize - mzbody + 1000h
.mz12_Checksum:
	dw	0
.mz14_Entry_point:
	dw	entry - mzbody
	dw	0
.mz18_Offset_of_relocation_table:
	dw	040h
.mz1a_Overlay_number:
	dw	0
	times 10 dw 0

mzbody:
entry:

	mov	ax,4C00h
	int	21h

	align 64,db 0



mzsize:


