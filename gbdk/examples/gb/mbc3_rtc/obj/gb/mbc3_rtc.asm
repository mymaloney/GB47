;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module mbc3_rtc
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl __current_ram_bank
	.globl _RTC_VALUE_REG
	.globl _RTC_LATCH_REG
	.globl _RTC_SELECT_REG
	.globl _RTC_GET
	.globl _RTC_SET
	.globl _RTC_START
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
__current_ram_bank::
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
;src/mbc3_rtc.c:7: uint16_t RTC_GET(const rtc_dateparts_e part) {
;	---------------------------------
; Function RTC_GET
; ---------------------------------
_RTC_GET::
	ld	e, a
;src/mbc3_rtc.h:27: inline void SWITCH_RAM_BANK(uint8_t bank, uint8_t mask) { SWITCH_RAM(_current_ram_bank = ((_current_ram_bank & ~mask) | (bank & mask))); }
	ld	hl, #__current_ram_bank
	ld	a, (hl)
	and	a, #0xf0
	ld	c, a
	ld	a, e
	and	a, #0x0f
	or	a, c
	ld	(hl), a
	ld	(#_rRAMB),a
;src/mbc3_rtc.c:10: v = RTC_VALUE_REG;
	ld	a, (_RTC_VALUE_REG)
	ld	c, a
	ld	b, #0x00
;src/mbc3_rtc.c:11: if (part == RTC_VALUE_DAY) {
	ld	a, e
	sub	a, #0x0b
	ret	NZ
;src/mbc3_rtc.h:27: inline void SWITCH_RAM_BANK(uint8_t bank, uint8_t mask) { SWITCH_RAM(_current_ram_bank = ((_current_ram_bank & ~mask) | (bank & mask))); }
	ld	hl, #__current_ram_bank
	ld	a, (hl)
	and	a, #0xf0
	or	a, #0x0c
	ld	(hl), a
	ld	(#_rRAMB),a
;src/mbc3_rtc.c:13: if (RTC_VALUE_REG & 0x01) v |= 0x0100u;
	ld	a, (_RTC_VALUE_REG)
	rrca
	ret	NC
	ld	b, #0x01
;src/mbc3_rtc.c:15: return v;
;src/mbc3_rtc.c:16: }
	ret
;src/mbc3_rtc.c:18: void RTC_SET(const rtc_dateparts_e part, const uint16_t v) {
;	---------------------------------
; Function RTC_SET
; ---------------------------------
_RTC_SET::
	ld	c, a
;src/mbc3_rtc.h:27: inline void SWITCH_RAM_BANK(uint8_t bank, uint8_t mask) { SWITCH_RAM(_current_ram_bank = ((_current_ram_bank & ~mask) | (bank & mask))); }
	ld	hl, #__current_ram_bank
	ld	a, (hl)
	and	a, #0xf0
	ld	b, a
	ld	a, c
	and	a, #0x0f
	or	a, b
	ld	(hl), a
	ld	(#_rRAMB),a
;src/mbc3_rtc.c:20: RTC_VALUE_REG = v;
	ld	hl, #_RTC_VALUE_REG
	ld	(hl), e
;src/mbc3_rtc.c:21: if (part == RTC_VALUE_DAY) {
	ld	a, c
	sub	a, #0x0b
	ret	NZ
;src/mbc3_rtc.h:27: inline void SWITCH_RAM_BANK(uint8_t bank, uint8_t mask) { SWITCH_RAM(_current_ram_bank = ((_current_ram_bank & ~mask) | (bank & mask))); }
	ld	hl, #__current_ram_bank
	ld	a, (hl)
	and	a, #0xf0
	or	a, #0x0c
	ld	(hl), a
	ld	(#_rRAMB),a
;src/mbc3_rtc.c:23: RTC_VALUE_REG = (RTC_VALUE_REG & 0x0e) | (uint8_t)((v >> 8) & 0x01);
	ld	a, (_RTC_VALUE_REG)
	and	a, #0x0e
	ld	c, a
	ld	a, d
	and	a, #0x01
	or	a, c
	ld	(#_RTC_VALUE_REG),a
;src/mbc3_rtc.c:25: }
	ret
;src/mbc3_rtc.c:27: void RTC_START(const uint8_t start) {
;	---------------------------------
; Function RTC_START
; ---------------------------------
_RTC_START::
	ld	c, a
	ld	hl, #__current_ram_bank
	ld	a, (hl)
	and	a, #0xf0
	or	a, #0x0c
	ld	(hl), a
	ld	(#_rRAMB),a
;src/mbc3_rtc.c:29: if (start) RTC_VALUE_REG &= ~RTC_TIMER_STOP; else RTC_VALUE_REG |= RTC_TIMER_STOP;
	ld	a, c
	or	a, a
	jr	Z, 00102$
	ld	hl, #_RTC_VALUE_REG
	ld	a, (hl)
	and	a, #0xbf
	ld	(hl), a
	ret
00102$:
	ld	hl, #_RTC_VALUE_REG
	ld	a, (hl)
	or	a, #0x40
	ld	(hl), a
;src/mbc3_rtc.c:30: }
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
