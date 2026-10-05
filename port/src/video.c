/* video.c - VGA_INIT (CODE:049A): mode 13h with the game's CRTC settings
 * and the "Loading" picture; HISCORE_INIT (CODE:046E).
 */
#include "frame.h"
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

void VGA_INIT(void)
{
    uint16_t bx, sel;
    uint32_t si, di, end, base;
    uint8_t mark;
    int i;

    vga_set_mode(0x13);
    vga_outw(0x3D4, 0x1E13);    /* offset 1Eh: lines of 240 bytes */
    vga_outw(0x3D4, 0x4009);    /* one scan line a row (bit 6 cleared again below) */
    /* the line compare 108h: bits 0-7 to CR 18h, bit 8 to CR 7 bit 4,
     * bit 9 to CR 9 bit 6 */
    bx = (uint16_t)(0x0101 & 0x201) << 4;           /* BH = BL = 1 (108h >> 8) */
    bx = (uint16_t)((bx & 0xFF) | (uint16_t)((bx >> 8) << 1 & 0xFF) << 8);
    vga_outw(0x3D4, 0x0818);
    vga_outb(0x3D4, 7);
    vga_outw(0x3D4, (uint16_t)(7 | ((vga_inb(0x3D5) & 0xEF) | (bx & 0xFF)) << 8));
    vga_outb(0x3D4, 9);
    vga_outw(0x3D4, (uint16_t)(9 | ((vga_inb(0x3D5) & 0xBF) | (bx >> 8)) << 8));
    /* 15 vertical retraces waited */
    for (i = 0; i < 15; i++)
        frame_wait();

    /* LOADING_PIC's run-length data (byte +5 the mark: mark, count,
     * value) into a block of 13EAh bytes (INT 92h AH=4) */
    pmax_name(0, 0x1C);              /* ESI 0 */
    sel = pmax_alloc(0x13EA);
    if (!sel)
        pi_stop("VGA_INIT (no memory)");
    base = pmax_base(sel);
    mark = rb(N_LOADING_PIC + 5);
    si = N_LOADING_PIC + 6;
    di = 0;
    end = 0x13EA;
    while (di < end) {
        uint8_t al = rb(si++);
        if (al != mark) {
            lwb(base + di++, al);
        } else {
            uint8_t n = rb(si++), v = rb(si++);
            while (n--)
                lwb(base + di++, v);
        }
    }
    /* its palette (+0Ah) to the DAC, its 240x18 pixels (+30Ah) to video
     * memory at A916h (INT 93h AH=5's selector: A0000h) */
    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x300; i++)
        vga_outb(0x3C9, lrb(base + 0x0A + (uint32_t)i));
    for (i = 0; i < 0x438 * 4; i++)
        vga_write((uint16_t)(0xA916 + i), lrb(base + 0x30A + (uint32_t)i));
    pmax_free(sel);
    frame_wait();               /* present the picture before the table loads */
}

void HISCORE_INIT(void)
{
    uint32_t i;

    if (rb(N_HISCORES) != 0 && rb(N_HISCORES) != 0xFF)
        return;
    for (i = 0; i < 0x32 * 4; i++)
        wb(N_HISCORES + i, rb(N_HISCORE_DEFAULTS + i));
}
