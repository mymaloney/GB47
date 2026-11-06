;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module main
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _hicolor_start
	.globl _cpu_fast
	.globl _fill_bkg_rect
	.globl _set_bkg_data
	.globl _display_off
	.globl _vsync
	.globl _joypad
	.globl _buttons_prev
	.globl _buttons
	.globl _hicolors
	.globl _blank_tile
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_buttons::
	.ds 1
_buttons_prev::
	.ds 1
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
;src/main.c:36: void main(void) {
;	---------------------------------
; Function main
; ---------------------------------
_main::
	add	sp, #-10
;src/main.c:38: uint8_t  img_select = 0;
	ldhl	sp,	#9
	ld	(hl), #0x00
;src/main.c:39: bool     first_pass = true;
	ldhl	sp,	#1
	ld	(hl), #0x01
;src/main.c:40: uint8_t  scroll_limit = 0;
	ldhl	sp,	#6
	ld	(hl), #0x00
;src/main.c:44: SHOW_BKG;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x01
	ldh	(_LCDC_REG + 0), a
;src/main.c:47: if (_cpu == CGB_TYPE) {
	ld	a, (#__cpu)
	sub	a, #0x11
	jp	NZ, 00128$
;src/main.c:49: cpu_fast();
	call	_cpu_fast
;src/main.c:51: while(true) {
00124$:
;src/main.c:53: vsync();
	call	_vsync
;src/main.c:54: UPDATE_BUTTONS();
	ld	a, (#_buttons)
	ld	(#_buttons_prev),a
	call	_joypad
	ldhl	sp,#8
	ld	(hl), a
	ld	a, (hl)
	ld	(#_buttons),a
;src/main.c:57: if (BUTTON_TOGGLED(J_A | J_B) || first_pass) {
	ld	a, (#_buttons_prev)
	ldhl	sp,	#7
	ld	(hl+), a
	xor	a, a
	ld	(hl-), a
	ld	a, (hl)
	cpl
	ldhl	sp,	#2
	ld	(hl+), a
	xor	a, a
	cpl
	ld	(hl), a
	ld	a, (#_buttons)
	ldhl	sp,	#4
	ld	(hl+), a
	xor	a, a
	ld	(hl-), a
	ld	a, (hl-)
	dec	hl
	and	a, (hl)
	ldhl	sp,	#7
	ld	(hl+), a
	xor	a, a
	ld	(hl-), a
	ld	a, (hl)
	and	a, #0x30
	jr	NZ, 00119$
	ldhl	sp,	#1
	bit	0, (hl)
	jp	Z, 00120$
00119$:
;src/main.c:59: vsync();
	call	_vsync
;src/main.c:60: DISPLAY_OFF;
	call	_display_off
;src/main.c:63: hicolor_bank = hicolors[img_select].bank;
	ldhl	sp,	#9
	ld	c, (hl)
	ld	b, #0x00
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, bc
	ld	c, l
	ld	b, h
	ld	hl, #_hicolors
	add	hl, bc
	push	hl
	ld	a, l
	ldhl	sp,	#7
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#6
	ld	(hl-), a
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ldhl	sp,	#0
	ld	(hl), a
;src/main.c:64: p_hicolor = (const hicolor_data *)hicolors[img_select].ptr;
	ldhl	sp,#5
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	l, e
	ld	h, d
	inc	hl
	push	hl
	ld	a, l
	ldhl	sp,	#9
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#8
	ld	(hl-), a
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	ld	d, a
	ld	a, (de)
	ld	(hl+), a
	inc	de
	ld	a, (de)
	ld	(hl-), a
	ld	a, (hl)
	ldhl	sp,	#1
	ld	(hl), a
	ldhl	sp,	#8
	ld	a, (hl)
	ldhl	sp,	#2
;src/main.c:66: uint8_t bank_save = CURRENT_BANK;
	ld	(hl+), a
	ldh	a, (__current_bank + 0)
	ld	(hl), a
;src/main.c:67: if (hicolor_bank) SWITCH_ROM(hicolor_bank);
	ldhl	sp,	#0
	ld	a, (hl)
	or	a, a
	jr	Z, 00102$
	ld	a, (hl)
	ldh	(__current_bank + 0), a
	ld	a, (hl)
	ld	(#_rROMB0),a
00102$:
;src/main.c:70: SCY_REG = 0u;
	xor	a, a
	ldh	(_SCY_REG + 0), a
;src/main.c:71: if ((p_hicolor->height_in_tiles * 8u) > DEVICE_SCREEN_PX_HEIGHT)
	ldhl	sp,#1
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	hl, #0x0002
	add	hl, de
	push	hl
	ld	a, l
	ldhl	sp,	#6
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#5
;src/main.c:81: if ((p_hicolor->height_in_tiles * 8u) < DEVICE_SCREEN_PX_HEIGHT) {
	ld	(hl-), a
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl+)
	ld	d, a
	ld	a, (de)
	ld	(hl), a
;src/main.c:71: if ((p_hicolor->height_in_tiles * 8u) > DEVICE_SCREEN_PX_HEIGHT)
	ld	a, (hl+)
	ld	(hl+), a
	ld	(hl), #0x00
	ld	a, #0x03
00211$:
	ldhl	sp,	#7
	sla	(hl)
	inc	hl
	rl	(hl)
	dec	a
	jr	NZ, 00211$
	dec	hl
	ld	a, #0x90
	sub	a, (hl)
	inc	hl
	ld	a, #0x00
	sbc	a, (hl)
	jr	NC, 00104$
;src/main.c:72: scroll_limit = ((p_hicolor->height_in_tiles * 8u) - DEVICE_SCREEN_PX_HEIGHT);
	ldhl	sp,	#6
	ld	a, (hl)
	add	a, a
	add	a, a
	add	a, a
	add	a, #0x70
	ld	(hl), a
	jr	00105$
00104$:
;src/main.c:73: else scroll_limit = 0;
	ldhl	sp,	#6
	ld	(hl), #0x00
00105$:
;src/main.c:81: if ((p_hicolor->height_in_tiles * 8u) < DEVICE_SCREEN_PX_HEIGHT) {
	ldhl	sp,	#7
	ld	a, (hl+)
	sub	a, #0x90
	ld	a, (hl)
	sbc	a, #0x00
	jr	NC, 00107$
;src/main.c:82: VBK_REG = VBK_BANK_1;
	ld	a, #0x01
	ldh	(_VBK_REG + 0), a
;src/main.c:83: set_bkg_data(BG_LAST_TILE, 1u, blank_tile);
	ld	de, #_blank_tile
	push	de
	ld	hl, #0x1ff
	push	hl
	call	_set_bkg_data
	add	sp, #4
;src/main.c:84: fill_bkg_rect(0u, (p_hicolor->height_in_tiles), DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, BKGF_BANK1);
	ldhl	sp,#4
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ld	h, #0x08
	push	hl
	inc	sp
	ld	h, #0x12
	push	hl
	inc	sp
	ld	h, #0x14
	push	hl
	inc	sp
	ld	h, a
	ld	l, #0x00
	push	hl
	call	_fill_bkg_rect
	add	sp, #5
;src/main.c:85: VBK_REG = VBK_BANK_0;
	xor	a, a
	ldh	(_VBK_REG + 0), a
;src/main.c:86: fill_bkg_rect(0u, (p_hicolor->height_in_tiles), DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, BG_LAST_TILE);
	ldhl	sp,#4
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ld	h, #0xff
	push	hl
	inc	sp
	ld	h, #0x12
	push	hl
	inc	sp
	ld	h, #0x14
	push	hl
	inc	sp
	ld	h, a
	ld	l, #0x00
	push	hl
	call	_fill_bkg_rect
	add	sp, #5
00107$:
;src/main.c:89: SWITCH_ROM(bank_save);
	ldhl	sp,	#3
	ld	a, (hl)
	ldh	(__current_bank + 0), a
	ld	a, (hl)
	ld	(#_rROMB0),a
;src/main.c:92: hicolor_start(p_hicolor, hicolor_bank);
	ldhl	sp,	#0
	ld	a, (hl+)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	call	_hicolor_start
;src/main.c:94: DISPLAY_ON;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x80
	ldh	(_LCDC_REG + 0), a
;src/main.c:97: img_select++;
	ldhl	sp,	#9
	inc	(hl)
;src/main.c:98: if (img_select == ARRAY_LEN(hicolors)) img_select = 0;
	ld	a, (hl)
	sub	a, #0x03
	jr	NZ, 00109$
	ldhl	sp,	#9
	ld	(hl), #0x00
00109$:
;src/main.c:100: first_pass = false;
	ldhl	sp,	#1
	ld	(hl), #0x00
	jp	00124$
00120$:
;src/main.c:103: else if (BUTTON_PRESSED(J_UP)) {
	ld	a, (_buttons)
	bit	2, a
	jr	Z, 00117$
;src/main.c:104: if (SCY_REG) SCY_REG--;
	ld	a, (#_SCY_REG)
	or	a, a
	jp	Z, 00124$
	ldh	a, (_SCY_REG + 0)
	dec	a
	ldh	(_SCY_REG + 0), a
	jp	00124$
00117$:
;src/main.c:105: } else if (BUTTON_PRESSED(J_DOWN)) {
	bit	3, a
	jp	Z, 00124$
;src/main.c:106: if (SCY_REG < scroll_limit) SCY_REG++;
	ldh	a, (_SCY_REG + 0)
	ldhl	sp,	#6
	sub	a, (hl)
	jp	NC, 00124$
	ldh	a, (_SCY_REG + 0)
	inc	a
	ldh	(_SCY_REG + 0), a
	jp	00124$
00128$:
;src/main.c:110: }
	add	sp, #10
	ret
_blank_tile:
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
	.db #0x00	; 0
_hicolors:
	.byte ___bank_test_pattern_tall
	.dw _test_pattern_tall_data
	.byte ___bank_example_image
	.dw _example_image_data
	.byte ___bank_test_pattern_short
	.dw _test_pattern_short_data
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
