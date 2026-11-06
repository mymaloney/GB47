;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module mbc7_accelerometer
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _mbc7_accel_update
	.globl _mbc7_accel_setorigin
	.globl _mbc7_accel_init
	.globl _mbc7_accel_read
	.globl _delay
	.globl _accel_y_relative
	.globl _accel_x_relative
	.globl _accel_y_origin
	.globl _accel_x_origin
	.globl _accel_origin_set
	.globl _accel_y
	.globl _accel_x
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_accel_x::
	.ds 2
_accel_y::
	.ds 2
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
_accel_origin_set::
	.ds 1
_accel_x_origin::
	.ds 2
_accel_y_origin::
	.ds 2
_accel_x_relative::
	.ds 2
_accel_y_relative::
	.ds 2
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
;src/mbc7_accelerometer.c:70: void mbc7_accel_read(void) {
;	---------------------------------
; Function mbc7_accel_read
; ---------------------------------
_mbc7_accel_read::
;src/mbc7_accelerometer.c:72: accel_x = ((uint16_t)(rMBC7_ACCEL_X_HI) << 8) | (uint16_t)rMBC7_ACCEL_X_LO;
	ld	a, (_rMBC7_ACCEL_X_HI)
	ld	b, a
	ld	a, (_rMBC7_ACCEL_X_LO)
	ld	(_accel_x), a
	ld	a, b
	ld	(_accel_x + 1), a
;src/mbc7_accelerometer.c:73: accel_y = ((uint16_t)(rMBC7_ACCEL_Y_HI) << 8) | (uint16_t)rMBC7_ACCEL_Y_LO;
	ld	a, (_rMBC7_ACCEL_Y_HI)
	ld	b, a
	ld	a, (_rMBC7_ACCEL_Y_LO)
	ld	(_accel_y), a
	ld	a, b
	ld	(_accel_y + 1), a
;src/mbc7_accelerometer.c:79: rMBC7_LATCH_1 = MBC7_LATCH_ERASE;
	ld	hl, #_rMBC7_LATCH_1
	ld	(hl), #0x55
;src/mbc7_accelerometer.c:80: rMBC7_LATCH_2 = MBC7_LATCH_CAPTURE;
	ld	hl, #_rMBC7_LATCH_2
	ld	(hl), #0xaa
;src/mbc7_accelerometer.c:81: }
	ret
;src/mbc7_accelerometer.c:84: void mbc7_accel_init(void) {
;	---------------------------------
; Function mbc7_accel_init
; ---------------------------------
_mbc7_accel_init::
;src/mbc7_accelerometer.c:87: rMBC7_SRAM_ENABLE_1 = MBC7_SRAM_ENABLE_KEY_1;
	ld	hl, #_rMBC7_SRAM_ENABLE_1
	ld	(hl), #0x0a
;src/mbc7_accelerometer.c:88: rMBC7_SRAM_ENABLE_2 = MBC7_SRAM_ENABLE_KEY_2;
	ld	hl, #_rMBC7_SRAM_ENABLE_2
	ld	(hl), #0x40
;src/mbc7_accelerometer.c:91: mbc7_accel_read();
	call	_mbc7_accel_read
;src/mbc7_accelerometer.c:95: delay(2);
	ld	de, #0x0002
;src/mbc7_accelerometer.c:96: }
	jp	_delay
;src/mbc7_accelerometer.c:99: void mbc7_accel_setorigin(void) {
;	---------------------------------
; Function mbc7_accel_setorigin
; ---------------------------------
_mbc7_accel_setorigin::
;src/mbc7_accelerometer.c:100: accel_x_origin = accel_x;
	ld	a, (#_accel_x)
	ld	(#_accel_x_origin),a
	ld	a, (#_accel_x + 1)
	ld	(#_accel_x_origin + 1),a
;src/mbc7_accelerometer.c:101: accel_y_origin = accel_y;
	ld	a, (#_accel_y)
	ld	(#_accel_y_origin),a
	ld	a, (#_accel_y + 1)
	ld	(#_accel_y_origin + 1),a
;src/mbc7_accelerometer.c:102: accel_origin_set = true;
	ld	hl, #_accel_origin_set
	ld	(hl), #0x01
;src/mbc7_accelerometer.c:103: }
	ret
;src/mbc7_accelerometer.c:106: void mbc7_accel_update(bool do_set_origin) {
;	---------------------------------
; Function mbc7_accel_update
; ---------------------------------
_mbc7_accel_update::
	ld	c, a
;src/mbc7_accelerometer.c:108: mbc7_accel_read();
	push	bc
	call	_mbc7_accel_read
	pop	bc
;src/mbc7_accelerometer.c:110: if (do_set_origin) {
	bit	0, c
	jr	Z, 00102$
;src/mbc7_accelerometer.c:111: mbc7_accel_setorigin();
	call	_mbc7_accel_setorigin
00102$:
;src/mbc7_accelerometer.c:114: if (accel_origin_set) {
	ld	hl, #_accel_origin_set
	bit	0, (hl)
	ret	Z
;src/mbc7_accelerometer.c:116: accel_x_relative = (int16_t)(accel_x_origin - accel_x);
	ld	a, (#_accel_x_origin)
	ld	hl, #_accel_x
	sub	a, (hl)
	ld	c, a
	ld	a, (#_accel_x_origin + 1)
	ld	hl, #_accel_x + 1
	sbc	a, (hl)
	ld	b, a
	ld	hl, #_accel_x_relative
	ld	a, c
	ld	(hl+), a
	ld	(hl), b
;src/mbc7_accelerometer.c:117: accel_y_relative = (int16_t)(accel_y_origin - accel_y);
	ld	a, (#_accel_y_origin)
	ld	hl, #_accel_y
	sub	a, (hl)
	ld	c, a
	ld	a, (#_accel_y_origin + 1)
	ld	hl, #_accel_y + 1
	sbc	a, (hl)
	ld	b, a
	ld	hl, #_accel_y_relative
	ld	a, c
	ld	(hl+), a
	ld	(hl), b
;src/mbc7_accelerometer.c:119: }
	ret
	.area _CODE
	.area _INITIALIZER
__xinit__accel_origin_set:
	.db #0x00	;  0
__xinit__accel_x_origin:
	.dw #0x0000
__xinit__accel_y_origin:
	.dw #0x0000
__xinit__accel_x_relative:
	.dw #0x0000
__xinit__accel_y_relative:
	.dw #0x0000
	.area _CABS (ABS)
