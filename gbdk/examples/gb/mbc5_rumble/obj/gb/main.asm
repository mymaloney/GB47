;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module main
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _rumble_update
	.globl _rumble_intensity_decrease
	.globl _rumble_intensity_increase
	.globl _rumble_intensity_set
	.globl _rumble_cancel
	.globl _rumble_start
	.globl _gotoxy
	.globl _printf
	.globl _vsync
	.globl _joypad
	.globl _add_VBL
	.globl _joy
	.globl _old_joy
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_old_joy::
	.ds 1
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
_joy::
	.ds 1
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
;src/main.c:33: static void display_rumble_intensity(uint8_t value) {
;	---------------------------------
; Function display_rumble_intensity
; ---------------------------------
_display_rumble_intensity:
	ld	b, a
;src/main.c:34: gotoxy(INTENS_PRNT_X, INTENS_PRNT_Y);
	push	bc
	ld	hl, #0xc12
	push	hl
	call	_gotoxy
	pop	hl
;src/main.c:35: printf("%hu", (uint8_t)value);
	inc	sp
	ld	de, #___str_0
	push	de
	call	_printf
	add	sp, #3
;src/main.c:36: }
	ret
___str_0:
	.ascii "%hu"
	.db 0x00
;src/main.c:39: void main(void)
;	---------------------------------
; Function main
; ---------------------------------
_main::
	dec	sp
	dec	sp
;src/main.c:44: }
	di
;src/main.c:43: add_VBL(rumble_update);
	ld	de, #_rumble_update
	call	_add_VBL
	ei
;src/main.c:59: "Rumble intensity:");
	ld	de, #___str_1
	push	de
	call	_printf
	pop	hl
;src/main.c:62: rumble_intensity_set(RUMBLE_INTENSITY_MAX);
	ld	a, #0x03
	call	_rumble_intensity_set
;src/main.c:63: display_rumble_intensity(RUMBLE_INTENSITY_MAX);
	ld	a, #0x03
	call	_display_rumble_intensity
;src/main.c:65: INPUT_UPDATE;
	ld	a, (#_joy)
	ld	(#_old_joy),a
	call	_joypad
	ld	(#_joy),a
;src/main.c:66: while(1) {
00109$:
;src/main.c:68: INPUT_UPDATE;
	ld	a, (#_joy)
	ld	(#_old_joy),a
	call	_joypad
	ld	(#_joy),a
;src/main.c:70: switch(INPUT_BUTTONS_PRESSED) {
	ld	a, (_old_joy)
	ld	c, #0x00
	cpl
	ldhl	sp,	#0
	ld	(hl+), a
	ld	a, c
	cpl
	ld	(hl-), a
	ld	a, (_joy)
	ld	c, (hl)
	and	a, c
	cp	a, #0x04
	jr	Z, 00104$
	cp	a, #0x08
	jr	Z, 00105$
	cp	a, #0x10
	jr	Z, 00102$
	cp	a, #0x20
	jr	Z, 00103$
	cp	a, #0x40
	jr	Z, 00106$
	sub	a, #0x80
	jr	NZ, 00107$
;src/main.c:71: case J_START:  rumble_start(RUMBLE_COUNT_HALF_SEC); break;
	ld	a, #0x1f
	call	_rumble_start
	jr	00107$
;src/main.c:72: case J_A:      rumble_start(RUMBLE_COUNT_QUARTER_SEC); break;
00102$:
	ld	a, #0x10
	call	_rumble_start
	jr	00107$
;src/main.c:73: case J_B:      rumble_start(RUMBLE_COUNT_EIGHTH_SEC); break;
00103$:
	ld	a, #0x08
	call	_rumble_start
	jr	00107$
;src/main.c:75: case J_UP:     display_rumble_intensity( rumble_intensity_increase() );
00104$:
	call	_rumble_intensity_increase
	call	_display_rumble_intensity
;src/main.c:76: break;
	jr	00107$
;src/main.c:77: case J_DOWN:   display_rumble_intensity( rumble_intensity_decrease() );
00105$:
	call	_rumble_intensity_decrease
	call	_display_rumble_intensity
;src/main.c:78: break;
	jr	00107$
;src/main.c:80: case J_SELECT: rumble_cancel(); break;
00106$:
	call	_rumble_cancel
;src/main.c:81: }
00107$:
;src/main.c:83: vsync();
	call	_vsync
	jr	00109$
;src/main.c:85: }
	inc	sp
	inc	sp
	ret
___str_1:
	.ascii "MBC5 Rumble Example"
	.db 0x0a
	.db 0x0a
	.ascii "Press:"
	.db 0x0a
	.ascii "-ST for 1/2 second"
	.db 0x0a
	.ascii "-A  for 1/4 second"
	.db 0x0a
	.ascii "-B  for 1/8 second"
	.db 0x0a
	.db 0x0a
	.ascii "-SEL to stop early"
	.db 0x0a
	.db 0x0a
	.ascii "-Up more intensity"
	.db 0x0a
	.ascii "-Dn less intensity"
	.db 0x0a
	.db 0x0a
	.ascii "Rumble intensity:"
	.db 0x00
	.area _CODE
	.area _INITIALIZER
__xinit__joy:
	.db #0x00	; 0
	.area _CABS (ABS)
