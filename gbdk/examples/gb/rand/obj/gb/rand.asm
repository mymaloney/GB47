;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.1 #15267 (MINGW64)
;--------------------------------------------------------
	.module rand
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _memset
	.globl _puts
	.globl _line
	.globl _plot
	.globl _arand
	.globl _initarand
	.globl _rand
	.globl _waitpadup
	.globl _waitpad
	.globl _accub
	.globl _accut
	.globl _accua
	.globl _accu
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
	.area _HRAM
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
_accu::
	.ds 40
_accua::
	.ds 40
_accut::
	.ds 40
_accub::
	.ds 40
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
;src/rand.c:36: void main(void)
;	---------------------------------
; Function main
; ---------------------------------
_main::
	add	sp, #-4
;src/rand.c:40: memset(accu, 0, sizeof(accu));
	ld	de, #0x0028
	push	de
	ld	de, #0x0000
	push	de
	ld	de, #_accu
	push	de
	call	_memset
	add	sp, #6
;src/rand.c:41: memset(accua, 0, sizeof(accua));
	ld	de, #0x0028
	push	de
	ld	de, #0x0000
	push	de
	ld	de, #_accua
	push	de
	call	_memset
	add	sp, #6
;src/rand.c:42: memset(accut, 0, sizeof(accut));
	ld	bc, #_accut
	ld	de, #0x0028
	push	de
	ld	de, #0x0000
	push	de
	push	bc
	call	_memset
	add	sp, #6
;src/rand.c:43: memset(accut, 0, sizeof(accub));
	ld	de, #0x0028
	push	de
	ld	de, #0x0000
	push	de
	push	bc
	call	_memset
	add	sp, #6
;src/rand.c:47: puts("Push any key (1)");
	ld	de, #___str_3
	call	_puts
;src/rand.c:48: waitpad(0xFF);
	ld	a, #0xff
	call	_waitpad
;src/rand.c:49: waitpadup();
	call	_waitpadup
;src/rand.c:50: seed = DIV_REG;
	ldh	a, (_DIV_REG + 0)
	ld	c, a
;src/rand.c:51: puts("Push any key (2)");
	push	bc
	ld	de, #___str_2
	call	_puts
	pop	bc
;src/rand.c:52: waitpad(0xFF);
	ld	a, #0xff
	call	_waitpad
;src/rand.c:53: waitpadup();
	call	_waitpadup
;src/rand.c:54: seed |= (UWORD)DIV_REG << 8;
	ldh	a, (_DIV_REG + 0)
	ld	b, a
;src/rand.c:57: initarand(seed);
	push	bc
	call	_initarand
	pop	hl
;src/rand.c:60: line(RANGE_SIZE * 1, 0, RANGE_SIZE * 1, 143);
	ld	hl, #0x8f28
	push	hl
	ld	hl, #0x28
	push	hl
	call	_line
	add	sp, #4
;src/rand.c:61: line(RANGE_SIZE * 2, 0, RANGE_SIZE * 2, 143);
	ld	hl, #0x8f50
	push	hl
	ld	hl, #0x50
	push	hl
	call	_line
	add	sp, #4
;src/rand.c:62: line(RANGE_SIZE * 3, 0, RANGE_SIZE * 3, 143);
	ld	hl, #0x8f78
	push	hl
	ld	hl, #0x78
	push	hl
	call	_line
	add	sp, #4
;src/rand.c:64: while (1) {
00110$:
;src/rand.c:70: uint8_t r = RAND_RANGE_8BIT(rand(), RANGE_SIZE);
	call	_rand
	ld	c, e
	ld	b, #0x00
	ld	l, c
	ld	h, b
	add	hl, hl
	add	hl, hl
	add	hl, bc
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	c, h
;src/rand.c:73: uint8_t ra = RAND_RANGE_8BIT(arand(), RANGE_SIZE);
	call	_arand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	add	hl, hl
	ld	b, h
	ldhl	sp,	#0
	ld	(hl), b
;src/rand.c:76: uint8_t rt1 = RAND_RANGE_8BIT(rand(), HALF_RANGE_SIZE);
	call	_rand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	ld	b, h
;src/rand.c:77: uint8_t rt2 = RAND_RANGE_8BIT(rand(), HALF_RANGE_SIZE);
	call	_rand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
;src/rand.c:78: uint8_t rt = HALF_RANGE_SIZE + (rt1 - rt2);
	ld	a, b
	sub	a, h
	add	a, #0x14
	ldhl	sp,	#1
	ld	(hl), a
;src/rand.c:81: uint8_t rb1 = RAND_RANGE_8BIT(rand(), HALF_RANGE_SIZE / 2);
	call	_rand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	ld	b, h
	srl	b
	ldhl	sp,	#2
	ld	(hl), b
;src/rand.c:82: uint8_t rb2 = RAND_RANGE_8BIT(rand(), HALF_RANGE_SIZE / 2);
	call	_rand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	srl	h
	ld	b, h
;src/rand.c:83: uint8_t rb3 = RAND_RANGE_8BIT(rand(), HALF_RANGE_SIZE / 2);
	call	_rand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	srl	h
	ld	a, h
	ldhl	sp,	#3
	ld	(hl), a
;src/rand.c:84: uint8_t rb4 = RAND_RANGE_8BIT(rand(), HALF_RANGE_SIZE / 2);
	call	_rand
	ld	l, e
	ld	h, #0x00
	ld	e, l
	ld	d, h
	add	hl, hl
	add	hl, hl
	add	hl, de
	add	hl, hl
	add	hl, hl
	srl	h
	ld	d, h
;src/rand.c:85: uint8_t rb = HALF_RANGE_SIZE + (rb1 - rb2) + (rb3 - rb4);
	ldhl	sp,	#2
	ld	a, (hl+)
	sub	a, b
	add	a, #0x14
	ld	e, a
	ld	a, (hl)
	sub	a, d
	add	a, e
	ld	(hl), a
;src/rand.c:92: uint8_t r_bucket_height = ++accu[r];
	ld	hl, #_accu
	ld	b, #0x00
	add	hl, bc
	inc	(hl)
	ld	a, (hl)
;src/rand.c:93: if (r_bucket_height > 144) break;
	cp	a, #0x91
	jp	NC, 00112$
;src/rand.c:94: plot(r  + (RANGE_SIZE * 0), 144-r_bucket_height, LTGREY, SOLID);
	ld	b, a
	ld	a, #0x90
	sub	a, b
	ld	h, #0x00
	push	hl
	inc	sp
	ld	h, #0x01
	push	hl
	inc	sp
	ld	b, a
	push	bc
	call	_plot
	add	sp, #4
;src/rand.c:96: uint8_t ra_bucket_height = ++accua[ra];
	ld	de, #_accua
	ldhl	sp,	#0
	ld	l, (hl)
	ld	h, #0x00
	add	hl, de
	ld	c, l
	ld	b, h
	ld	a, (bc)
	inc	a
	ld	(bc), a
;src/rand.c:97: if (ra_bucket_height > 144) break;
	cp	a, #0x91
	jr	NC, 00112$
;src/rand.c:98: plot(ra + (RANGE_SIZE * 1), 144 - ra_bucket_height, DKGREY, SOLID);
	ld	c, a
	ld	a, #0x90
	sub	a, c
	ld	b, a
	ldhl	sp,	#0
	ld	a, (hl)
	add	a, #0x28
	ld	h, #0x00
	push	hl
	inc	sp
	ld	h, #0x02
	ld	l, b
	push	hl
	push	af
	inc	sp
	call	_plot
	add	sp, #4
;src/rand.c:100: uint8_t rt_bucket_height = ++accut[rt];
	ld	de, #_accut
	ldhl	sp,	#1
	ld	l, (hl)
	ld	h, #0x00
	add	hl, de
	ld	c, l
	ld	b, h
	ld	a, (bc)
	inc	a
	ld	(bc), a
;src/rand.c:101: if (rt_bucket_height > 144) break;
	cp	a, #0x91
	jr	NC, 00112$
;src/rand.c:102: plot(rt + (RANGE_SIZE * 2), 144 - rt_bucket_height, BLACK, SOLID);
	ld	c, a
	ld	a, #0x90
	sub	a, c
	ld	b, a
	ldhl	sp,	#1
	ld	a, (hl)
	add	a, #0x50
	ld	h, #0x00
	push	hl
	inc	sp
	ld	h, #0x03
	ld	l, b
	push	hl
	push	af
	inc	sp
	call	_plot
	add	sp, #4
;src/rand.c:104: uint8_t rb_bucket_height = ++accub[rb];
	ld	de, #_accub
	ldhl	sp,	#3
	ld	l, (hl)
	ld	h, #0x00
	add	hl, de
	ld	c, l
	ld	b, h
	ld	a, (bc)
	inc	a
	ld	(bc), a
;src/rand.c:105: if (rb_bucket_height > 144) break;
	cp	a, #0x91
	jr	NC, 00112$
;src/rand.c:106: plot(rb + (RANGE_SIZE * 3), 144 - rb_bucket_height, BLACK, SOLID);
	ld	c, a
	ld	a, #0x90
	sub	a, c
	ld	b, a
	ldhl	sp,	#3
	ld	a, (hl)
	add	a, #0x78
	ld	h, #0x00
	push	hl
	inc	sp
	ld	h, #0x03
	ld	l, b
	push	hl
	push	af
	inc	sp
	call	_plot
	add	sp, #4
	jp	00110$
00112$:
;src/rand.c:108: }
	add	sp, #4
	ret
___str_2:
	.ascii "Push any key (2)"
	.db 0x00
___str_3:
	.ascii "Getting seed"
	.db 0x0a
	.ascii "Push any key (1)"
	.db 0x00
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
