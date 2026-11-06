;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module mbc5_rumble
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _rumble_set_duty_cycle
	.globl _rumble_mask
	.globl _rumble_intensity
	.globl _rumble_count
	.globl _rumble_masks
	.globl _rumble_start
	.globl _rumble_cancel
	.globl _rumble_intensity_set
	.globl _rumble_intensity_increase
	.globl _rumble_intensity_decrease
	.globl _rumble_update
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
_rumble_count::
	.ds 1
_rumble_intensity::
	.ds 1
_rumble_mask::
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
;src/mbc5_rumble.c:20: void rumble_start(uint8_t duration) {
;	---------------------------------
; Function rumble_start
; ---------------------------------
_rumble_start::
	ld	(#_rumble_count),a
;src/mbc5_rumble.c:21: rumble_count = duration;
;src/mbc5_rumble.c:22: }
	ret
_rumble_masks:
	.db #0x01	; 1
	.db #0x02	; 2
	.db #0x03	; 3
	.db #0xff	; 255
;src/mbc5_rumble.c:25: void rumble_cancel(void) {
;	---------------------------------
; Function rumble_cancel
; ---------------------------------
_rumble_cancel::
;src/mbc5_rumble.c:26: rumble_count = RUMBLE_COUNT_DONE;
;src/mbc5_rumble.c:27: MBC5_RUMBLE_OFF;
	xor	a, a
	ld	(#_rumble_count), a
	ld	(#_rRAMB),a
;src/mbc5_rumble.c:28: }
	ret
;src/mbc5_rumble.c:31: void rumble_set_duty_cycle(void) {
;	---------------------------------
; Function rumble_set_duty_cycle
; ---------------------------------
_rumble_set_duty_cycle::
;src/mbc5_rumble.c:32: rumble_mask = rumble_masks[rumble_intensity];
	ld	a, #<(_rumble_masks)
	ld	hl, #_rumble_intensity
	add	a, (hl)
	ld	c, a
	ld	a, #>(_rumble_masks)
	adc	a, #0x00
	ld	b, a
	ld	a, (bc)
	ld	(#_rumble_mask),a
;src/mbc5_rumble.c:33: }
	ret
;src/mbc5_rumble.c:36: void rumble_intensity_set(uint8_t intensity) {    
;	---------------------------------
; Function rumble_intensity_set
; ---------------------------------
_rumble_intensity_set::
;src/mbc5_rumble.c:38: if (intensity > RUMBLE_INTENSITY_MAX)
	cp	a, #0x04
	jr	C, 00102$
;src/mbc5_rumble.c:39: intensity = RUMBLE_INTENSITY_MAX;
	ld	a, #0x03
00102$:
;src/mbc5_rumble.c:40: rumble_intensity = intensity;
	ld	(#_rumble_intensity),a
;src/mbc5_rumble.c:41: rumble_set_duty_cycle();
;src/mbc5_rumble.c:42: }
	jp	_rumble_set_duty_cycle
;src/mbc5_rumble.c:45: uint8_t rumble_intensity_increase(void) {
;	---------------------------------
; Function rumble_intensity_increase
; ---------------------------------
_rumble_intensity_increase::
;src/mbc5_rumble.c:47: if (rumble_intensity <  RUMBLE_INTENSITY_MAX)
	ld	hl, #_rumble_intensity
	ld	a, (hl)
	sub	a, #0x03
	jr	NC, 00102$
;src/mbc5_rumble.c:48: rumble_intensity++;
	inc	(hl)
00102$:
;src/mbc5_rumble.c:49: rumble_set_duty_cycle();
	call	_rumble_set_duty_cycle
;src/mbc5_rumble.c:51: return rumble_intensity;
	ld	a, (_rumble_intensity)
;src/mbc5_rumble.c:52: }
	ret
;src/mbc5_rumble.c:55: uint8_t rumble_intensity_decrease(void) {
;	---------------------------------
; Function rumble_intensity_decrease
; ---------------------------------
_rumble_intensity_decrease::
;src/mbc5_rumble.c:57: if (rumble_intensity > RUMBLE_INTENSITY_MIN)
	ld	hl, #_rumble_intensity
	ld	a, (hl)
	or	a, a
	jr	Z, 00102$
;src/mbc5_rumble.c:58: rumble_intensity--;
	dec	(hl)
00102$:
;src/mbc5_rumble.c:59: rumble_set_duty_cycle();
	call	_rumble_set_duty_cycle
;src/mbc5_rumble.c:61: return rumble_intensity;
	ld	a, (_rumble_intensity)
;src/mbc5_rumble.c:62: }
	ret
;src/mbc5_rumble.c:66: void rumble_update(void) {
;	---------------------------------
; Function rumble_update
; ---------------------------------
_rumble_update::
;src/mbc5_rumble.c:68: if (rumble_count) {
	ld	hl, #_rumble_count
	ld	a, (hl)
	or	a, a
	ret	Z
;src/mbc5_rumble.c:69: rumble_count--;
	dec	(hl)
;src/mbc5_rumble.c:76: if (rumble_count & rumble_mask)
	ld	a, (hl)
	ld	hl, #_rumble_mask
	and	a, (hl)
	jr	Z, 00102$
;src/mbc5_rumble.c:77: MBC5_RUMBLE_ON;
	ld	hl, #_rRAMB
	ld	(hl), #0x08
	ret
00102$:
;src/mbc5_rumble.c:79: MBC5_RUMBLE_OFF;
	xor	a, a
	ld	(#_rRAMB),a
;src/mbc5_rumble.c:81: }
	ret
	.area _CODE
	.area _INITIALIZER
__xinit__rumble_count:
	.db #0x00	; 0
__xinit__rumble_intensity:
	.db #0x03	; 3
__xinit__rumble_mask:
	.db #0xff	; 255
	.area _CABS (ABS)
