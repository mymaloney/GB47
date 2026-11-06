;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module main
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _RTC_START
	.globl _RTC_SET
	.globl _RTC_GET
	.globl _puts
	.globl _printf
	.globl _gotoxy
	.globl _vsync
	.globl _waitpadup
	.globl _joypad
	.globl _rtc
	.globl _RTC_VALUE_REG
	.globl _RTC_LATCH_REG
	.globl _RTC_SELECT_REG
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_RTC_SELECT_REG	=	0x4000
_RTC_LATCH_REG	=	0x6000
_RTC_VALUE_REG	=	0xa000
_rtc::
	.ds 5
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
;src/main.c:40: static void read_and_show_rtc_data(rtc_data_t * p_rtc) {
;	---------------------------------
; Function read_and_show_rtc_data
; ---------------------------------
_read_and_show_rtc_data:
	add	sp, #-8
	ld	c, e
	ld	b, d
;src/mbc3_rtc.h:29: inline void RTC_LATCH(void) { RTC_LATCH_REG = 0; RTC_LATCH_REG = 1; }
	xor	a, a
	ld	hl, #_RTC_LATCH_REG
	ld	(hl), a
	ld	(hl), #0x01
;src/main.c:44: p_rtc->day  = RTC_GET(RTC_VALUE_DAY);
	inc	sp
	inc	sp
	push	bc
	push	bc
	ld	a, #0x0b
	call	_RTC_GET
	ldhl	sp,	#8
	ld	a, c
	ld	(hl+), a
	ld	(hl), b
	pop	bc
	pop	de
	push	de
	ldhl	sp,	#6
	ld	a, (hl+)
	ld	(de), a
	inc	de
	ld	a, (hl)
	ld	(de), a
;src/main.c:45: p_rtc->hour = RTC_GET(RTC_VALUE_HOUR);
	ld	hl, #0x0002
	add	hl, bc
	push	hl
	ld	a, l
	ldhl	sp,	#8
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#7
	ld	(hl), a
	push	bc
	ld	a, #0x0a
	call	_RTC_GET
	ld	e, c
	pop	bc
	ld	a, e
	ldhl	sp,	#6
	ld	e, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, e
	ld	(hl), a
;src/main.c:46: p_rtc->min  = RTC_GET(RTC_VALUE_MIN);
	ld	e, c
	ld	d, b
	inc	de
	inc	de
	inc	de
	push	bc
	push	de
	ld	a, #0x09
	call	_RTC_GET
	ld	l, c
	pop	de
	pop	bc
	ld	a, l
	ld	(de), a
;src/main.c:47: p_rtc->sec  = RTC_GET(RTC_VALUE_SEC);
	inc	bc
	inc	bc
	inc	bc
	inc	bc
	push	bc
	push	de
	ld	a, #0x08
	call	_RTC_GET
	ld	l, c
	pop	de
	pop	bc
	ld	a, l
	ld	(bc), a
;src/main.c:49: gotoxy(0,6);
	push	bc
	push	de
	ld	hl, #0x600
	push	hl
	call	_gotoxy
	pop	hl
;src/main.c:50: printf("Elapsed RTC Time:\n");
	ld	de, #___str_1
	call	_puts
	pop	de
	pop	bc
;src/main.c:56: (uint16_t)p_rtc->sec);
	ld	a, (bc)
	ldhl	sp,	#2
	ld	(hl+), a
;src/main.c:55: (uint16_t)p_rtc->min,
	xor	a, a
	ld	(hl+), a
	ld	a, (de)
	ld	(hl+), a
;src/main.c:54: (uint16_t)p_rtc->hour,
	xor	a, a
	ld	(hl+), a
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	ld	d, a
	ld	a, (de)
	ld	(hl+), a
	ld	(hl), #0x00
;src/main.c:53: (uint16_t)p_rtc->day,
	pop	de
	push	de
	ld	a, (de)
	ld	c, a
	inc	de
	ld	a, (de)
	ld	b, a
;src/main.c:52: "Time: %u:%u:%u   \n",
	ldhl	sp,	#2
	ld	a, (hl+)
	inc	hl
	ld	e, a
	ld	d, #0x00
	push	de
	ld	a, (hl+)
	inc	hl
	ld	e, a
	ld	d, #0x00
	push	de
	ld	e, (hl)
	ld	d, #0x00
	push	de
	push	bc
	ld	de, #___str_2
	push	de
	call	_printf
;src/main.c:57: }
	add	sp, #18
	ret
___str_1:
	.ascii "Elapsed RTC Time:"
	.db 0x00
___str_2:
	.ascii "Days: %u  "
	.db 0x0a
	.ascii "Time: %u:%u:%u   "
	.db 0x0a
	.db 0x00
;src/main.c:60: static void write_rtc_data(rtc_data_t * p_rtc) {
;	---------------------------------
; Function write_rtc_data
; ---------------------------------
_write_rtc_data:
	add	sp, #-8
	ld	c, e
	ld	b, d
;src/main.c:65: p_rtc->day     = 0u; // Supported range: 0-511
	inc	sp
	inc	sp
	push	bc
	pop	hl
	push	hl
	xor	a, a
	ld	(hl+), a
	ld	(hl), a
;src/main.c:66: p_rtc->hour    = 0u; // Supported range: 0-23
	ld	hl, #0x0002
	add	hl, bc
	push	hl
	ld	a, l
	ldhl	sp,	#8
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#7
	ld	(hl-), a
	ld	a, (hl+)
	ld	h, (hl)
	ld	l, a
	ld	(hl), #0x00
;src/main.c:67: p_rtc->min     = 0u; // Supported range: 0-59
	ld	hl, #0x0003
	add	hl, bc
	push	hl
	ld	a, l
	ldhl	sp,	#6
	ld	(hl), a
	pop	hl
	ld	a, h
	ldhl	sp,	#5
	ld	(hl-), a
	ld	a, (hl+)
	ld	h, (hl)
	ld	l, a
	ld	(hl), #0x00
;src/main.c:68: p_rtc->sec     = 0u; // Supported range: 0-59
	inc	bc
	inc	bc
	inc	bc
	inc	bc
	xor	a, a
	ld	(bc), a
;src/main.c:70: RTC_SET(RTC_VALUE_DAY,  p_rtc->day);
	pop	de
	push	de
	ld	a, (de)
	ld	l, a
	inc	de
	ld	a, (de)
	push	bc
	ld	e, l
	ld	d, a
	ld	a, #0x0b
	call	_RTC_SET
	pop	bc
;src/main.c:71: RTC_SET(RTC_VALUE_HOUR, p_rtc->hour);
	ldhl	sp,#6
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ld	d, #0x00
	push	bc
	ld	e, a
	ld	a, #0x0a
	call	_RTC_SET
	pop	bc
;src/main.c:72: RTC_SET(RTC_VALUE_MIN,  p_rtc->min);
	ldhl	sp,#4
	ld	a, (hl+)
	ld	e, a
	ld	d, (hl)
	ld	a, (de)
	ld	d, #0x00
	push	bc
	ld	e, a
	ld	a, #0x09
	call	_RTC_SET
	pop	bc
;src/main.c:73: RTC_SET(RTC_VALUE_SEC,  p_rtc->sec);
	ld	a, (bc)
	ld	d, #0x00
	push	bc
	ld	e, a
	ld	a, #0x08
	call	_RTC_SET
;src/main.c:75: gotoxy(0,11);
	ld	hl, #0xb00
	push	hl
	call	_gotoxy
	pop	hl
;src/main.c:76: printf("Set RTC to:\n");
	ld	de, #___str_4
	call	_puts
	pop	bc
;src/main.c:82: (uint16_t)p_rtc->sec);
	ld	a, (bc)
	ldhl	sp,	#2
	ld	(hl+), a
;src/main.c:81: (uint16_t)p_rtc->min,
	xor	a, a
	ld	(hl+), a
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	ld	d, a
	ld	a, (de)
	ld	(hl+), a
;src/main.c:80: (uint16_t)p_rtc->hour,
	xor	a, a
	ld	(hl+), a
	ld	a, (hl+)
	ld	e, a
	ld	a, (hl-)
	ld	d, a
	ld	a, (de)
	ld	(hl+), a
	ld	(hl), #0x00
;src/main.c:79: (uint16_t)p_rtc->day,
	pop	de
	push	de
	ld	a, (de)
	ld	c, a
	inc	de
	ld	a, (de)
	ld	b, a
;src/main.c:78: "Time: %u:%u:%u   \n",
	ldhl	sp,	#2
	ld	a, (hl+)
	inc	hl
	ld	e, a
	ld	d, #0x00
	push	de
	ld	a, (hl+)
	inc	hl
	ld	e, a
	ld	d, #0x00
	push	de
	ld	e, (hl)
	ld	d, #0x00
	push	de
	push	bc
	ld	de, #___str_5
	push	de
	call	_printf
;src/main.c:83: }
	add	sp, #18
	ret
___str_4:
	.ascii "Set RTC to:"
	.db 0x00
___str_5:
	.ascii "Days: %u  "
	.db 0x0a
	.ascii "Time: %u:%u:%u   "
	.db 0x0a
	.db 0x00
;src/main.c:86: static void clear_rtc_send_message(void) {
;	---------------------------------
; Function clear_rtc_send_message
; ---------------------------------
_clear_rtc_send_message:
;src/main.c:88: gotoxy(0,11);
	ld	hl, #0xb00
	push	hl
	call	_gotoxy
	pop	hl
;src/main.c:91: "                  \n");
	ld	de, #___str_10
;src/main.c:92: }
	jp	_puts
___str_10:
	.ascii "                  "
	.db 0x0a
	.ascii "                  "
	.db 0x0a
	.ascii "                  "
	.db 0x00
;src/main.c:95: void main(void) {
;	---------------------------------
; Function main
; ---------------------------------
_main::
	dec	sp
;src/main.c:98: ENABLE_RAM;
	ld	hl, #_rRAMG
	ld	(hl), #0x0a
;src/main.c:99: RTC_START(true);
	ld	a, #0x01
	call	_RTC_START
;src/main.c:103: SPRITES_8x8;
	ldh	a, (_LCDC_REG + 0)
	and	a, #0xfb
	ldh	(_LCDC_REG + 0), a
;src/main.c:104: SHOW_SPRITES;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x02
	ldh	(_LCDC_REG + 0), a
;src/main.c:105: SHOW_BKG;
	ldh	a, (_LCDC_REG + 0)
	or	a, #0x01
	ldh	(_LCDC_REG + 0), a
;src/main.c:110: "  set new RTC info.\n");
	ld	de, #___str_12
	call	_puts
;src/main.c:112: uint8_t clear_notice_count = 0;
	ldhl	sp,	#0
	ld	(hl), #0x00
;src/main.c:113: while(1) {
00111$:
;src/main.c:116: vsync();
	call	_vsync
;src/main.c:117: gamepad = joypad();
	call	_joypad
;src/main.c:121: if (gamepad & J_SELECT) {
	bit	6, a
	jr	Z, 00108$
;src/main.c:122: write_rtc_data(&rtc);
	ld	de, #_rtc
	call	_write_rtc_data
;src/main.c:124: clear_notice_count = 6;
	ldhl	sp,	#0
	ld	(hl), #0x06
;src/main.c:127: waitpadup();
	call	_waitpadup
	jr	00111$
00108$:
;src/main.c:131: if ((sys_time & 0x1Fu) == 0u) {
	ld	a, (_sys_time)
	ld	hl, #_sys_time + 1
	ld	c, (hl)
	and	a, #0x1f
	jr	NZ, 00111$
;src/main.c:132: read_and_show_rtc_data(&rtc);
	ld	de, #_rtc
	call	_read_and_show_rtc_data
;src/main.c:134: if (clear_notice_count) {
	ldhl	sp,	#0
	ld	a, (hl)
	or	a, a
	jr	Z, 00111$
;src/main.c:135: clear_notice_count--;
	dec	(hl)
	ld	a, (hl)
	jr	NZ, 00111$
;src/main.c:136: if (clear_notice_count == 0) {
;src/main.c:137: clear_rtc_send_message();
	call	_clear_rtc_send_message
	jr	00111$
;src/main.c:145: }
	inc	sp
	ret
___str_12:
	.ascii "RTC Date/Time Demo"
	.db 0x0a
	.db 0x0a
	.ascii "* Press SELECT to "
	.db 0x0a
	.ascii "  set new RTC info."
	.db 0x00
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
