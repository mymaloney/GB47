;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module gbc_hicolor
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _hicolor_palette_isr
	.globl _set_bkg_tiles
	.globl _set_bkg_data
	.globl _set_interrupts
	.globl _add_LCD
	.globl _remove_LCD
	.globl _hicolor_start
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_SP_SAVE:
	.ds 2
_STAT_SAVE:
	.ds 1
_p_hicolor_palettes:
	.ds 2
_hicolor_palettes_bank:
	.ds 1
_hicolor_last_scanline:
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
;src/gbc_hicolor.c:20: void hicolor_palette_isr(void) NONBANKED {
;	---------------------------------
; Function hicolor_palette_isr
; ---------------------------------
_hicolor_palette_isr::
;src/gbc_hicolor.c:116: __endasm;
	ldh	a, (__current_bank) ; switch ROM bank
	push	af
	ld	a, (_hicolor_palettes_bank)
	or	a
	jr	z, 3$
	ldh	(__current_bank), a
	ld	(_rROMB0), a
3$:
	ld	(_SP_SAVE), sp ; save SP
	ld	hl, #_p_hicolor_palettes ; load address of picture palettes buffer
	ld	a, (hl+)
	ld	d, (hl)
	ld	e, a
	ldh	a, (_SCY_REG)
	swap	a
	ld	l, a
	and	#0x0f
	ld	h, a
	ld	a, #0xf0
	and	l
	ld	l, a
	add	hl, hl
	add	hl, de ; offset address by SCY * (4 * 4 * 2)
	ld	sp, hl
	rlca	; compensate odd/even line
	and	#0x20 ; if odd then start from 4-th palette; offset == 32
	or	#0b10000000 ; set auto-increment
	ld	hl, #_BCPS_REG
	ld	(hl+), a ; HL now points to BCPD
	ld	c, #(8 * 4) ; read and set the the colors that come from previous lines
2$:
	pop	de
	ld	(hl), e
	ld	(hl), d
	dec	c
	jr	nz, 2$
0$:
	ldh	a, (_STAT_REG)
	and	#0b00000010
	jr	z, 0$ ; wait for mode 3
	ldh	a, (_STAT_REG)
	ld	(_STAT_SAVE), a
	ld	a, #0b00001000
	ldh	(_STAT_REG), a
	xor	a
	ldh	(_IF_REG), a
1$:
	pop	bc ; preload the first two colors
	pop	de
	xor	a
	ldh	(_IF_REG), a
	halt	; wait for mode 0
	ld	(hl), c ; set the first two colors
	ld	(hl), b
	ld	(hl), e
	ld	(hl), d
	.rept	(4*4)-2 ; read and set four palettes except the two previously set colors
	pop	de
	ld	(hl), e
	ld	(hl), d
	.endm
	ld	a, (_hicolor_last_scanline)
	ld	c, a
	ldh	a, (_LY_REG)
	cp	c
	jr	c, 1$ ; load the next 4 palettes
	ld	a, (_STAT_SAVE)
	ldh	(_STAT_REG), a
	xor	a
	ldh	(_IF_REG), a
	ld	sp, #_SP_SAVE
	pop	hl
	ld	sp, hl ; restore SP
	pop	af
	ldh	(__current_bank), a
	ld	(_rROMB0), a
	ret
;src/gbc_hicolor.c:117: }
	ret
;src/gbc_hicolor.c:125: void hicolor_start(const hicolor_data * p_hicolor, uint8_t hicolor_bank) NONBANKED {
;	---------------------------------
; Function hicolor_start
; ---------------------------------
_hicolor_start::
	add	sp, #-9
	ld	c, e
	ld	b, d
	ld	e, a
;src/gbc_hicolor.c:129: }
	di
;src/gbc_hicolor.c:128: remove_LCD(hicolor_palette_isr);
	push	bc
	push	de
	ld	de, #_hicolor_palette_isr
	call	_remove_LCD
	pop	de
	pop	bc
	ei
;src/gbc_hicolor.c:131: if (!p_hicolor) return;
	ld	a, b
	or	a, c
	jp	Z, 00107$
;src/gbc_hicolor.c:132: hicolor_palettes_bank = hicolor_bank;
	ld	hl, #_hicolor_palettes_bank
	ld	(hl), e
;src/gbc_hicolor.c:134: uint8_t bank_save = CURRENT_BANK;
	ldh	a, (__current_bank + 0)
	ldhl	sp,	#2
	ld	(hl), a
;src/gbc_hicolor.c:135: if (hicolor_bank) SWITCH_ROM(hicolor_bank);
	ld	a, e
	or	a, a
	jr	Z, 00104$
	ld	a, e
	ldh	(__current_bank + 0), a
	ld	hl, #_rROMB0
	ld	(hl), e
00104$:
;src/gbc_hicolor.c:138: p_hicolor_palettes = p_hicolor->p_palette;
	ld	hl, #0x0009
	add	hl, bc
	ld	e, l
	ld	d, h
	ld	a, (de)
	ld	hl, #_p_hicolor_palettes
	ld	(hl+), a
	inc	de
	ld	a, (de)
	ld	(hl), a
;src/gbc_hicolor.c:141: hicolor_last_scanline = (p_hicolor->height_in_tiles > DEVICE_SCREEN_HEIGHT) ? (DEVICE_SCREEN_PX_HEIGHT - 1) : ((p_hicolor->height_in_tiles << 3) - 1);
	ld	hl, #0x0002
	add	hl, bc
	push	hl
	ld	a, l
	ldhl	sp,	#5
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#4
	ld	(hl-), a
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	cp	a, #0x13
	jr	C, 00109$
	ld	a, #0x8f
	jr	00110$
00109$:
	add	a, a
	add	a, a
	add	a, a
	dec	a
00110$:
	ld	(#_hicolor_last_scanline),a
;src/gbc_hicolor.c:144: VBK_REG = VBK_BANK_0;
	xor	a, a
	ldh	(_VBK_REG + 0), a
;src/gbc_hicolor.c:145: set_bkg_data(0u, MIN(p_hicolor->tile_count, 256), p_hicolor->p_tiles);
	ld	hl, #0x0003
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
	ld	(hl+), a
	inc	de
	ld	a, (de)
	ld	(hl), a
	ldhl	sp,	#7
	ld	a, c
	ld	(hl+), a
	ld	a, b
	ld	(hl-), a
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ld	l, a
	inc	de
	ld	a, (de)
	sub	a, #0x01
	jr	NC, 00111$
	ld	a, l
	jr	00112$
00111$:
	xor	a, a
00112$:
	pop	de
	push	de
	push	de
	ld	h, a
	ld	l, #0x00
	push	hl
	call	_set_bkg_data
	add	sp, #4
;src/gbc_hicolor.c:146: set_bkg_tiles(0u, 0u, DEVICE_SCREEN_WIDTH, p_hicolor->height_in_tiles, p_hicolor->p_map);
	ld	hl, #0x0005
	add	hl, bc
	ld	e, l
	ld	d, h
	ld	a, (de)
	ldhl	sp,	#0
	ld	(hl+), a
	inc	de
	ld	a, (de)
	ld	(hl), a
	ldhl	sp,#3
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	pop	de
	push	de
	push	de
	ld	h, a
	ld	l, #0x14
	push	hl
	xor	a, a
	rrca
	push	af
	call	_set_bkg_tiles
	add	sp, #6
;src/gbc_hicolor.c:149: VBK_REG = VBK_BANK_1;
	ld	a, #0x01
	ldh	(_VBK_REG + 0), a
;src/gbc_hicolor.c:150: if (p_hicolor->tile_count > 256) set_bkg_data(0u, (p_hicolor->tile_count - 256), p_hicolor->p_tiles + (256 * 16));
	ldhl	sp,#7
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	ld	d, a
	ld	a, (de)
	ld	(hl+), a
	inc	de
	ld	a, (de)
	ld	(hl-), a
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	xor	a, a
	cp	a, e
	ld	a, #0x01
	sbc	a, d
	jr	NC, 00106$
	dec	hl
	dec	hl
	dec	hl
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ld	l, a
	inc	de
	ld	a, (de)
	ld	e, l
	add	a, #0x10
	ld	d, a
	ldhl	sp,	#7
	ld	a, (hl)
	push	de
	ld	h, a
	ld	l, #0x00
	push	hl
	call	_set_bkg_data
	add	sp, #4
00106$:
;src/gbc_hicolor.c:151: set_bkg_tiles(0, 0, DEVICE_SCREEN_WIDTH, p_hicolor->height_in_tiles, p_hicolor->p_attribute_map);
	ld	hl, #0x0007
	add	hl, bc
	ld	a, (hl+)
	ld	c, a
	ld	b, (hl)
	ldhl	sp,#3
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	push	bc
	ld	h, a
	ld	l, #0x14
	push	hl
	xor	a, a
	rrca
	push	af
	call	_set_bkg_tiles
	add	sp, #6
;src/gbc_hicolor.c:152: VBK_REG = VBK_BANK_0;
	xor	a, a
	ldh	(_VBK_REG + 0), a
;src/gbc_hicolor.c:154: SWITCH_ROM(bank_save);
	ldhl	sp,	#2
	ld	a, (hl)
	ldh	(__current_bank + 0), a
	ld	a, (hl)
	ld	(#_rROMB0),a
;src/gbc_hicolor.c:162: }
	di
;src/gbc_hicolor.c:158: LYC_REG = 152;
	ld	a, #0x98
	ldh	(_LYC_REG + 0), a
;src/gbc_hicolor.c:159: STAT_REG = STATF_LYC;
	ld	a, #0x40
	ldh	(_STAT_REG + 0), a
;src/gbc_hicolor.c:161: add_LCD(hicolor_palette_isr);
	ld	de, #_hicolor_palette_isr
	call	_add_LCD
	ei
;src/gbc_hicolor.c:163: set_interrupts(IE_REG | LCD_IFLAG);
	ldh	a, (_IE_REG + 0)
	set	1, a
	call	_set_interrupts
00107$:
;src/gbc_hicolor.c:164: }
	add	sp, #9
	ret
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
