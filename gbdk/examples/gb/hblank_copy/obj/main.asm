;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module main
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _LCD_ISR
	.globl _hblank_copy_vram
	.globl _fill_bkg_rect
	.globl _set_bkg_tile_xy
	.globl _set_vram_byte
	.globl _display_off
	.globl _vsync
	.globl _set_interrupts
	.globl _joypad
	.globl _add_LCD
	.globl _animation_counter
	.globl _animation_speed
	.globl _animation_direction
	.globl _animation
	.globl _joy
	.globl _current_frame
	.globl _old_joy
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_LCD_ISR_odd_even_frame_10000_141:
	.ds 1
_old_joy::
	.ds 1
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
_current_frame::
	.ds 2
_joy::
	.ds 1
_animation::
	.ds 1
_animation_direction::
	.ds 1
_animation_speed::
	.ds 1
_animation_counter::
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
;src/main.c:16: static bool odd_even_frame = false;
	xor	a, a
	ld	hl, #_LCD_ISR_odd_even_frame_10000_141
	ld	(hl), a
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
;src/main.c:15: void LCD_ISR(void) {
;	---------------------------------
; Function LCD_ISR
; ---------------------------------
_LCD_ISR::
	dec	sp
;src/main.c:17: uint8_t _save = CURRENT_BANK;
	ldh	a, (__current_bank + 0)
	ldhl	sp,	#0
	ld	(hl), a
;src/main.c:18: SWITCH_ROM(current_frame->bank);
	ld	a, (_current_frame)
	ld	hl, #_current_frame + 1
	ld	b, (hl)
	ld	c, a
	inc	bc
	inc	bc
	ld	a, (bc)
	ldh	(__current_bank + 0), a
	ld	a, (bc)
	ld	(#_rROMB0),a
;src/main.c:20: if (odd_even_frame = !odd_even_frame) {
	ld	a, (#_LCD_ISR_odd_even_frame_10000_141)
	xor	a, #0x01
	ld	l, a
	ld	(_LCD_ISR_odd_even_frame_10000_141), a
	bit	0, l
	jr	Z, 00102$
;src/main.c:21: LCDC_REG &= ~LCDCF_BG8000;
	ldh	a, (_LCDC_REG + 0)
	and	a, #0xef
	ldh	(_LCDC_REG + 0), a
;src/main.c:22: hblank_copy_destination = _VRAM8000;
	ld	hl, #_hblank_copy_destination
	ld	a, #<(__VRAM8000)
	ld	(hl+), a
	ld	(hl), #>(__VRAM8000)
;src/main.c:23: hblank_copy_vram(current_frame->tiles, MAP_WIDTH * MAP_HEIGHT);
	ld	hl, #_current_frame
	ld	a,	(hl+)
	ld	h, (hl)
	ld	l, a
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
	ld	a, #0x7e
	ld	e, c
	ld	d, b
	call	_hblank_copy_vram
	jr	00103$
00102$:
;src/main.c:25: LCDC_REG |= LCDCF_BG8000;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x10
	ldh	(_LCDC_REG + 0), a
;src/main.c:26: hblank_copy_destination = _VRAM9000;
	ld	hl, #_hblank_copy_destination
	ld	a, #<(__VRAM9000)
	ld	(hl+), a
	ld	(hl), #>(__VRAM9000)
;src/main.c:27: hblank_copy_vram(current_frame->tiles, MAP_WIDTH * MAP_HEIGHT);
	ld	hl, #_current_frame
	ld	a,	(hl+)
	ld	h, (hl)
	ld	l, a
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
	ld	a, #0x7e
	ld	e, c
	ld	d, b
	call	_hblank_copy_vram
00103$:
;src/main.c:29: SWITCH_ROM(_save);
	ldhl	sp,	#0
	ld	a, (hl)
	ldh	(__current_bank + 0), a
	ld	a, (hl)
	ld	(#_rROMB0),a
;src/main.c:30: }
	inc	sp
	ret
;src/main.c:44: void main(void) {
;	---------------------------------
; Function main
; ---------------------------------
_main::
;src/main.c:45: DISPLAY_OFF;
	call	_display_off
;src/main.c:50: }
	di
;src/main.c:48: LYC_REG = 0, STAT_REG |= STATF_LYC;
	xor	a, a
	ldh	(_LYC_REG + 0), a
	ldh	a, (_STAT_REG + 0)
	or	a, #0x40
	ldh	(_STAT_REG + 0), a
;src/main.c:49: add_LCD(LCD_ISR);
	ld	de, #_LCD_ISR
	call	_add_LCD
	ei
;src/main.c:51: set_interrupts(IE_REG | LCD_IFLAG);
	ldh	a, (_IE_REG + 0)
	set	1, a
	call	_set_interrupts
;src/main.c:54: BGP_REG = DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK);
	ld	a, #0xe4
	ldh	(_BGP_REG + 0), a
;src/main.c:57: fill_bkg_rect(0, 0, 32, 32, 0);
	xor	a, a
	ld	h, a
	ld	l, #0x20
	push	hl
	ld	a, #0x20
	push	af
	inc	sp
	xor	a, a
	rrca
	push	af
	call	_fill_bkg_rect
	add	sp, #5
;src/main.c:58: uint8_t v = 0;
;src/main.c:59: for (uint8_t y = POSITION_Y; y != POSITION_Y + MAP_HEIGHT; y++) {
	ld	bc, #0x400
00141$:
	ld	a, b
	sub	a, #0x0d
	jr	Z, 00102$
;src/main.c:60: uint8_t * addr = set_bkg_tile_xy(POSITION_X, y, v++);
	ld	a, c
	inc	c
	push	bc
	push	af
	inc	sp
	ld	e, b
	ld	a, #0x03
	call	_set_bkg_tile_xy
	ld	e, c
	ld	d, b
	pop	bc
;src/main.c:61: for (uint8_t x = 0; x != MAP_WIDTH - 1; x++) set_vram_byte(++addr, v++);
	ld	l, #0x00
00138$:
	ld	a, l
	sub	a, #0x0d
	jr	Z, 00162$
	ld	a, c
	inc	c
	inc	de
	push	hl
	push	de
	call	_set_vram_byte
	pop	de
	pop	hl
	inc	l
	jr	00138$
00162$:
;src/main.c:59: for (uint8_t y = POSITION_Y; y != POSITION_Y + MAP_HEIGHT; y++) {
	inc	b
	jr	00141$
00102$:
;src/main.c:64: SHOW_BKG;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x01
	ldh	(_LCDC_REG + 0), a
;src/main.c:65: DISPLAY_ON;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x80
	ldh	(_LCDC_REG + 0), a
;src/main.c:67: bool step_animation = false;
	ld	c, #0x00
;src/main.c:69: while (true) {
00131$:
;src/main.c:70: vsync();
	call	_vsync
;src/main.c:34: old_joy = joy, joy = joypad();
	ld	a, (#_joy)
	ld	(#_old_joy),a
	call	_joypad
	ld	hl, #_joy
	ld	(hl), a
;src/main.c:73: if (joy & J_LEFT) {
	ld	a, (hl)
	bit	1, a
	jr	Z, 00106$
;src/main.c:74: animation = false;
;src/main.c:75: animation_direction = false;
	xor	a, a
	ld	(#_animation), a
	ld	(#_animation_direction),a
;src/main.c:76: step_animation = true;
	ld	c, #0x01
	jr	00107$
00106$:
;src/main.c:77: } else if (joy & J_RIGHT) {
	rrca
	jr	NC, 00107$
;src/main.c:78: animation = false;
	xor	a, a
	ld	(#_animation),a
;src/main.c:79: animation_direction = true;
	ld	hl, #_animation_direction
	ld	(hl), #0x01
;src/main.c:80: step_animation = true;
	ld	c, #0x01
00107$:
;src/main.c:37: return ((joy & ~old_joy) & key);
	ld	a, (#_old_joy)
	cpl
	ld	hl, #_joy
	and	a, (hl)
	ld	b, a
	bit	2, b
	jr	Z, 00115$
;src/main.c:83: if (KEY_PRESSED(J_UP)) {
;src/main.c:84: if (animation_speed) animation_speed--;
	ld	hl, #_animation_speed
	ld	a, (hl)
	or	a, a
	jr	Z, 00116$
	dec	(hl)
	jr	00116$
00115$:
;src/main.c:37: return ((joy & ~old_joy) & key);
	bit	3, b
	jr	Z, 00116$
;src/main.c:85: } else if (KEY_PRESSED(J_DOWN)) {
;src/main.c:86: if (++animation_speed > 10) animation_speed = 10;
	ld	hl, #_animation_speed
	inc	(hl)
	ld	a, #0x0a
	sub	a, (hl)
	jr	NC, 00116$
	ld	(hl), #0x0a
00116$:
;src/main.c:37: return ((joy & ~old_joy) & key);
	bit	7, b
	jr	Z, 00118$
;src/main.c:89: if (KEY_PRESSED(J_START)) animation  = !animation;
	ld	hl, #_animation
	ld	a, (hl)
	xor	a, #0x01
	ld	(hl), a
00118$:
;src/main.c:92: if ((step_animation) || ((animation) && (++animation_counter > animation_speed))) {
	bit	0, c
	jr	NZ, 00126$
	ld	hl, #_animation
	bit	0, (hl)
	jr	Z, 00131$
	ld	hl, #_animation_counter
	inc	(hl)
	ld	a, (#_animation_speed)
	ld	hl, #_animation_counter
	sub	a, (hl)
	jr	NC, 00131$
00126$:
;src/main.c:93: animation_counter = 0;
	xor	a, a
	ld	(#_animation_counter),a
;src/main.c:94: if (animation_direction) CRITICAL {
	ld	hl, #_animation_direction
	bit	0, (hl)
	jr	Z, 00124$
;src/main.c:96: } else CRITICAL {
	di
;src/main.c:95: if (++current_frame == (frames + ANIMATION_FRAME_COUNT)) current_frame = frames;
	ld	hl, #_current_frame
	ld	a, (hl)
	add	a, #0x03
	ld	(hl+), a
	ld	a, (hl)
	adc	a, #0x00
	ld	(hl-), a
	ld	a, (hl)
	sub	a, #<((_frames + 192))
	jr	NZ, 00120$
	inc	hl
	ld	a, (hl)
	sub	a, #>((_frames + 192))
	jr	NZ, 00120$
	ld	hl, #_current_frame
	ld	(hl), #<(_frames)
	inc	hl
	ld	(hl), #>(_frames)
00120$:
	ei
	jr	00125$
00124$:
;src/main.c:98: }
	di
;src/main.c:97: if (--current_frame < frames) current_frame = frames + (ANIMATION_FRAME_COUNT - 1);
	ld	hl, #_current_frame
	ld	a, (hl)
	add	a, #0xfd
	ld	(hl+), a
	ld	a, (hl)
	adc	a, #0xff
	ld	(hl-), a
	ld	a, (hl+)
	sub	a, #<(_frames)
	ld	a, (hl)
	sbc	a, #>(_frames)
	jr	NC, 00122$
	dec	hl
	ld	(hl), #<((_frames + 189))
	inc	hl
	ld	(hl), #>((_frames + 189))
00122$:
	ei
00125$:
;src/main.c:99: step_animation = false;
	ld	c, #0x00
;src/main.c:102: }
	jp	00131$
	.area _CODE
	.area _INITIALIZER
__xinit__current_frame:
	.dw _frames
__xinit__joy:
	.db #0x00	; 0
__xinit__animation:
	.db #0x01	;  1
__xinit__animation_direction:
	.db #0x01	;  1
__xinit__animation_speed:
	.db #0x01	; 1
__xinit__animation_counter:
	.db #0x00	; 0
	.area _CABS (ABS)
