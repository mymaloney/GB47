;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module vsync_no_halt
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _vsync_no_halt
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
;src/vsync_no_halt.c:3: void vsync_no_halt(void) {
;	---------------------------------
; Function vsync_no_halt
; ---------------------------------
_vsync_no_halt::
;src/vsync_no_halt.c:7: if ((LCDC_REG & LCDCF_ON) == 0) return;
	ldh	a, (_LCDC_REG + 0)
	rlca
	ret	NC
;src/vsync_no_halt.c:10: VBL_DONE = 0;
	xor	a, a
	ldh	(__vbl_done + 0), a
;src/vsync_no_halt.c:11: while (VBL_DONE == 0);
00103$:
	ld	a, (#__vbl_done)
	or	a, a
	jr	Z, 00103$
;src/vsync_no_halt.c:12: }
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
