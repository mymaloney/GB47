;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module main
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _vsync_no_halt
	.globl _mbc7_accel_update
	.globl _mbc7_accel_init
	.globl _gotoxy
	.globl _puts
	.globl _printf
	.globl _set_sprite_data
	.globl _joypad
	.globl _joy
	.globl _old_joy
	.globl _sprite_tile
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
;src/main.c:58: void main(void)
;	---------------------------------
; Function main
; ---------------------------------
_main::
	add	sp, #-6
;src/main.c:60: bool do_set_origin = false;
	ldhl	sp,	#0
	ld	(hl), #0x00
;src/main.c:61: bool show_data     = false;
	ldhl	sp,	#5
	ld	(hl), #0x00
;src/main.c:63: HIDE_SPRITES;
	ldh	a, (_LCDC_REG + 0)
	and	a, #0xfd
	ldh	(_LCDC_REG + 0), a
;src/main.c:65: set_sprite_data(SPR_TILE_0, 1u, sprite_tile);
	ld	de, #_sprite_tile
	push	de
	xor	a, a
	inc	a
	push	af
	call	_set_sprite_data
	add	sp, #4
;c:\users\malon\downloads\gbdk-win64\gbdk\include\gb\gb.h:1887: shadow_OAM[nb].tile=tile;
	ld	hl, #(_shadow_OAM + 2)
	ld	(hl), #0x00
;src/main.c:68: printf("-B to Center\n");
	ld	de, #___str_1
	call	_puts
;src/main.c:69: printf("-SELECT toggles info");
	ld	de, #___str_2
	push	de
	call	_printf
	pop	hl
;src/main.c:71: mbc7_accel_init();
	call	_mbc7_accel_init
;src/main.c:73: while(1) {
00111$:
;src/main.c:76: INPUT_UPDATE;
	ld	a, (#_joy)
	ld	(#_old_joy),a
	call	_joypad
	ld	(#_joy),a
;src/main.c:77: switch (INPUT_BUTTONPRESS(J_B | J_SELECT)) {
	ld	a, (#_old_joy)
	ldhl	sp,	#3
	ld	(hl+), a
	xor	a, a
	ld	(hl-), a
	ld	a, (hl-)
	dec	hl
	cpl
	ld	(hl+), a
	xor	a, a
	cpl
	ld	(hl), a
	ld	a, (#_joy)
	ldhl	sp,	#4
	ld	(hl), a
	ld	a, (hl-)
	ld	(hl-), a
	dec	hl
	ld	a, (hl)
	ldhl	sp,	#4
	ld	(hl-), a
	ld	a, (hl+)
	and	a, (hl)
	and	a, #0x60
	cp	a, #0x20
	jr	Z, 00101$
	sub	a, #0x40
	jr	Z, 00102$
	jr	00103$
;src/main.c:78: case J_B: do_set_origin = true; break;
00101$:
	ldhl	sp,	#0
	ld	(hl), #0x01
	jr	00103$
;src/main.c:79: case J_SELECT: show_data ^= true; break;
00102$:
	ldhl	sp,	#5
	ld	a, (hl)
	xor	a, #0x01
	ld	(hl), a
;src/main.c:80: }
00103$:
;src/main.c:82: mbc7_accel_update(do_set_origin);
	ldhl	sp,	#0
	ld	a, (hl)
	call	_mbc7_accel_update
;src/main.c:85: if (do_set_origin) {
	ldhl	sp,	#0
	bit	0, (hl)
	jr	Z, 00105$
;src/main.c:86: do_set_origin = false;
	ld	(hl), #0x00
;src/main.c:88: gotoxy(0,3);
	ld	hl, #0x300
	push	hl
	call	_gotoxy
	pop	hl
;src/main.c:89: printf("origin x:%x\norigin y:%x", accel_x_origin, accel_y_origin);
	ld	a, (_accel_y_origin)
	ld	e, a
	ld	hl, #_accel_y_origin + 1
	ld	d, (hl)
	push	de
	ld	a, (_accel_x_origin)
	ld	e, a
	ld	hl, #_accel_x_origin + 1
	ld	d, (hl)
	push	de
	ld	de, #___str_3
	push	de
	call	_printf
	add	sp, #6
;src/main.c:90: SHOW_SPRITES;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x02
	ldh	(_LCDC_REG + 0), a
;c:\users\malon\downloads\gbdk-win64\gbdk\include\gb\gb.h:1973: OAM_item_t * itm = &shadow_OAM[nb];
	ld	hl, #_shadow_OAM
;c:\users\malon\downloads\gbdk-win64\gbdk\include\gb\gb.h:1974: itm->y=y, itm->x=x;
	ld	a, #0x54
	ld	(hl+), a
	ld	(hl), #0x54
;src/main.c:91: move_sprite(SPR_ID_CROSS, SCRN_CENTER_X, SCRN_CENTER_Y);
00105$:
;src/main.c:95: if (accel_origin_set) {
	ld	hl, #_accel_origin_set
	bit	0, (hl)
	jp	Z, 00109$
;src/main.c:98: scroll_sprite(SPR_ID_CROSS, accel_x_relative / ACCEL_DIVIDE_AMT, accel_y_relative / ACCEL_DIVIDE_AMT);
	ld	a, (_accel_y_relative)
	ld	c, a
	ld	hl, #_accel_y_relative + 1
	ld	b, (hl)
	ld	l, c
	ld	h, b
	bit	7, b
	jr	Z, 00118$
	ld	hl, #0x0007
	add	hl, bc
00118$:
	ld	c, l
	sra	h
	rr	c
	sra	h
	rr	c
	sra	h
	rr	c
	ld	a, (#_accel_x_relative)
	ldhl	sp,	#1
	ld	(hl), a
	ld	a, (#_accel_x_relative + 1)
	ldhl	sp,	#2
	ld	(hl-), a
	ld	a, (hl+)
	inc	hl
	ld	(hl-), a
	ld	a, (hl+)
	inc	hl
	ld	(hl-), a
	dec	hl
	bit	7, (hl)
	jr	Z, 00119$
	dec	hl
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	hl, #0x0007
	add	hl, de
	push	hl
	ld	a, l
	ldhl	sp,	#5
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#4
	ld	(hl), a
00119$:
	ldhl	sp,#3
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	sra	d
	rr	e
	sra	d
	rr	e
	sra	d
	rr	e
;c:\users\malon\downloads\gbdk-win64\gbdk\include\gb\gb.h:1989: OAM_item_t * itm = &shadow_OAM[nb];
;c:\users\malon\downloads\gbdk-win64\gbdk\include\gb\gb.h:1990: itm->y+=y, itm->x+=x;
	ld	hl, #_shadow_OAM
	ld	a, (hl)
	add	a, c
	ld	(hl), a
	ld	hl, #_shadow_OAM + 1
	ld	a, (hl)
	add	a, e
	ld	(hl), a
;src/main.c:100: if (show_data) {
	ldhl	sp,	#5
	bit	0, (hl)
	jr	Z, 00109$
;src/main.c:101: gotoxy(0,6);
	ld	hl, #0x600
	push	hl
	call	_gotoxy
	pop	hl
;src/main.c:105: "%d   \n",
	ld	a, (_accel_y_relative)
	ld	e, a
	ld	hl, #_accel_y_relative + 1
	ld	d, (hl)
	push	de
	ld	a, (_accel_x_relative)
	ld	e, a
	ld	hl, #_accel_x_relative + 1
	ld	d, (hl)
	push	de
	ld	a, (_accel_y)
	ld	e, a
	ld	hl, #_accel_y + 1
	ld	d, (hl)
	push	de
	ld	a, (_accel_x)
	ld	e, a
	ld	hl, #_accel_x + 1
	ld	d, (hl)
	push	de
	ld	de, #___str_4
	push	de
	call	_printf
	add	sp, #10
00109$:
;src/main.c:114: vsync_no_halt();
	call	_vsync_no_halt
	jp	00111$
;src/main.c:116: }
	add	sp, #6
	ret
_sprite_tile:
	.db #0x3c	; 60
	.db #0x18	; 24
	.db #0x7e	; 126
	.db #0x18	; 24
	.db #0xff	; 255
	.db #0x18	; 24
	.db #0xff	; 255
	.db #0xff	; 255
	.db #0xff	; 255
	.db #0xff	; 255
	.db #0xff	; 255
	.db #0x18	; 24
	.db #0x7e	; 126
	.db #0x18	; 24
	.db #0x3c	; 60
	.db #0x18	; 24
___str_1:
	.ascii "-B to Center"
	.db 0x00
___str_2:
	.ascii "-SELECT toggles info"
	.db 0x00
___str_3:
	.ascii "origin x:%x"
	.db 0x0a
	.ascii "origin y:%x"
	.db 0x00
___str_4:
	.ascii "%x"
	.db 0x0a
	.ascii "%x"
	.db 0x0a
	.ascii "%d   "
	.db 0x0a
	.ascii "%d   "
	.db 0x0a
	.db 0x00
	.area _CODE
	.area _INITIALIZER
__xinit__joy:
	.db #0x00	; 0
	.area _CABS (ABS)
