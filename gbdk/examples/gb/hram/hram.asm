;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module hram
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _printf
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
;hram.c:7: void main(void) {
;	---------------------------------
; Function main
; ---------------------------------
_main::
;hram.c:8: my_hram_variable = 5;
	ld	a, #0x05
	ldh	(_my_hram_variable + 0), a
;hram.c:9: printf("value is: %hd at %x", (uint8_t)my_hram_variable, (uint16_t)&my_hram_variable);  
	ld	de, #_my_hram_variable
	push	de
	ldh	a, (_my_hram_variable + 0)
	push	af
	inc	sp
	ld	de, #___str_0
	push	de
	call	_printf
	add	sp, #5
;hram.c:10: }
	ret
___str_0:
	.ascii "value is: %hd at %x"
	.db 0x00
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
