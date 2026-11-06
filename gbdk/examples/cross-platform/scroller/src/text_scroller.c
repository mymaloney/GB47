#include <gbdk/platform.h>
#include <gbdk/font.h>
#include <stdint.h>
#include <stdio.h>

// Positive 1 in valid y scroll coordinates
#define P1 1
// Negative 1 in valid y scroll coordinates
#define N1 (uint8_t)(DEVICE_SCREEN_BUFFER_HEIGHT*8 - 1)

const uint8_t shake_tbl[] = {0, P1, P1, P1, 0, N1, N1, N1};
const uint8_t scanline_offsets_tbl[] = {0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0};
const uint8_t * scanline_offsets = scanline_offsets_tbl;

#define SCROLL_POS 15u
#define SCROLL_HEIGHT 1u
#define SCROLL_PIX_HEIGHT (SCROLL_HEIGHT * 8u)
#define SCROLL_POS_PIX_START (((SCROLL_POS + DEVICE_SCREEN_Y_OFFSET) * 8u) - 2u)
#define SCROLL_POS_PIX_END (((SCROLL_POS + DEVICE_SCREEN_Y_OFFSET) * 8u) + SCROLL_PIX_HEIGHT - 1u)

inline void do_scroll(uint8_t x, uint8_t y) {
#if defined(NINTENDO) || defined(NINTENDO_NES)
    move_bkg(x, y);
#elif defined(SEGA)
    y;
    __WRITE_VDP_REG_UNSAFE(VDP_RSCX, -x);    
#endif
}

#if defined(SEGA)
uint8_t LYC_REG = 0;  // define the fake LYC_REG, we will use it as the interrupt routine state

void vblank_isr(void) {
    __WRITE_VDP_REG_UNSAFE(VDP_R10, SCROLL_POS_PIX_START);
    do_scroll(0, 0);
    LYC_REG = SCROLL_POS_PIX_START;
}
#endif

uint8_t scroller_x = 0;
void scanline_isr(void) {
    switch (LYC_REG) {
#if defined(NINTENDO) || defined(NINTENDO_NES)
        case 0: 
            do_scroll(0, 0);
            LYC_REG = SCROLL_POS_PIX_START;
            break;
#endif
        case SCROLL_POS_PIX_START:
            do_scroll(scroller_x, shake_tbl[(scroller_x >> 1) & 7]);
#if defined(SEGA)
            __WRITE_VDP_REG_UNSAFE(VDP_R10, R10_INT_OFF); // disable scanline interrupts after the next triggerimg
            while (VCOUNTER != SCROLL_POS_PIX_END);       // busywait for the end of the scanline effect
            do_scroll(0, 0);
#endif
            LYC_REG = SCROLL_POS_PIX_END;
            break;
        case SCROLL_POS_PIX_END:
#if defined(NINTENDO) || defined(NINTENDO_NES)
            do_scroll(0, 0);
            LYC_REG = 0;
#endif
        default:
            break;
    }
}

const uint8_t scroller_text[] = "This is a text scroller demo for GBDK-2020. You can use ideas, that are "\
"shown in this demo, to make different parallax effects, scrolling of tilemaps which are larger than 32x32 "\
"tiles and TEXT SCROLLERS, of course! Need to write something else to make this text longer than 256 characters. "\
"The quick red fox jumps over the lazy brown dog. 0123456789.          ";

const uint8_t * scroller_next_char = scroller_text;
uint8_t * scroller_vram_addr;
uint8_t * base, * limit;

void main(void) {
    DISPLAY_OFF;
    // Init font system / clear screen
    font_init();
    font_set(font_load(font_ibm));
    // Fill the screen background with '*'
    fill_bkg_rect(0, 0, DEVICE_SCREEN_WIDTH, DEVICE_SCREEN_HEIGHT, '*' - ' ');
    DISPLAY_ON;

    printf(" Scrolling %d chars", sizeof(scroller_text) - 1);

    CRITICAL {
        add_LCD(scanline_isr);
#if defined(NINTENDO)
        STAT_REG |= STATF_LYC;
#elif defined(SEGA)
        add_VBL(vblank_isr);
#endif
        LYC_REG = 0;
    }
#if defined(NINTENDO) || defined(NINTENDO_NES) || defined(SEGA)
    set_interrupts(VBL_IFLAG | LCD_IFLAG);
#endif
    HIDE_LEFT_COLUMN;    
    base = (uint8_t *)((uint16_t)get_bkg_xy_addr(0, SCROLL_POS) & (DEVICE_SCREEN_MAP_ENTRY_SIZE==1?0xffe0:0xffc0));
    limit = base + (DEVICE_SCREEN_BUFFER_WIDTH * DEVICE_SCREEN_MAP_ENTRY_SIZE);

    scroller_vram_addr = base + ((DEVICE_SCREEN_X_OFFSET + DEVICE_SCREEN_WIDTH) * DEVICE_SCREEN_MAP_ENTRY_SIZE);
    if (scroller_vram_addr >= limit) scroller_vram_addr = base;
    
    set_vram_byte(scroller_vram_addr, *scroller_next_char - 0x20);
    
    while (TRUE) {
        scroller_x++;
        if ((scroller_x & 0x07) == 0) {
            // next letter
            scroller_next_char++;
            if (*scroller_next_char == 0) scroller_next_char = scroller_text;
            
            // next vram position
            scroller_vram_addr += DEVICE_SCREEN_MAP_ENTRY_SIZE;
            if (scroller_vram_addr >= limit) scroller_vram_addr = base;
            
            // put next char
            set_vram_byte(scroller_vram_addr, *scroller_next_char - 0x20);
        }
        vsync();        
    }
}