/* lights.c - the light states of header slot 14 at the game's start
 * (LIGHTS_RESET, CODE:29AF9) and the drop targets' pictures they draw
 * (DROP_PIC, CODE:2912B; SPRITE4_DRAW, CODE:29265).
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

/* CODE:3D389 */
static void MAP_MASK(uint8_t ah)
{
    vga_outw(0x3C4, (uint16_t)(2 | ah << 8));
}

/* CODE:29265: a picture at DS:[SPR_SRC] of SPR_W x SPR_H pixels (bytes
 * in plane order, a quarter of the width a row, FFh transparent) at
 * SPR_X, SPR_Y: into the stage's copy in SPOOKY_SEL (planes from
 * SPR_PLANES, starting with x AND 3's, the next byte after the fourth),
 * then into video memory from 1500h where HIDELIGHTS_SEL's byte (the
 * same plane) is 0 */
static void SPRITE4_DRAW(void)
{
    uint32_t fs = pmax_base(rw(N_DROPS_SEL)), src, di, ebx, r, c, k;
    uint32_t w4 = rd(N_SPR_W) >> 2, h = rd(N_SPR_H), x = rd(N_SPR_X);
    uint32_t spooky = pmax_base(rw(N_SPOOKY_SEL)), hide = pmax_base(rw(N_HIDELIGHTS_SEL));

    wd(N_SPR_SKIP, 0x54 - w4);
    wd(N_SPR_AT, rd(N_SPR_Y) * 0x54 + (x >> 2));
    wb(N_SPR_PLANE0, (uint8_t)(x & 3));

    src = rd(N_SPR_SRC);
    wb(N_SPR_PLANE, rb(N_SPR_PLANE0));
    wd(N_SPR_CARRY, 0);
    for (k = 0; k < 4; k++) {
        wb(N_SPR_PLANE, (uint8_t)(rb(N_SPR_PLANE) + 1));
        if (rb(N_SPR_PLANE) == 5) {
            wb(N_SPR_PLANE, 1);
            wd(N_SPR_CARRY, 1);
        }
        di = rd(N_SPR_AT) + rd(N_SPR_PLANES + 4 * (uint32_t)(5 - rb(N_SPR_PLANE))) + rd(N_SPR_CARRY);
        for (r = 0; r < h; r++, di += rd(N_SPR_SKIP))
            for (c = 0; c < w4; c++, di++) {
                uint8_t al = lrb(fs + src++);
                if (al != 0xFF)
                    lwb(spooky + di, al);
            }
    }

    src = rd(N_SPR_SRC);
    ebx = 0;
    wb(N_SPR_PLANE, rb(N_SPR_PLANE0));
    wd(N_SPR_CARRY, 0);
    for (k = 0; k < 4; k++, ebx += 0xC4E0) {
        uint8_t n;

        wb(N_SPR_PLANE, (uint8_t)(rb(N_SPR_PLANE) + 1));
        if (rb(N_SPR_PLANE) == 5) {
            wb(N_SPR_PLANE, 1);
            wd(N_SPR_CARRY, 1);
        }
        di = rd(N_SPR_AT) + 0x1500 + rd(N_SPR_CARRY);
        n = (uint8_t)(5 - rb(N_SPR_PLANE));
        MAP_MASK((uint8_t)(0x10 >> n));
        for (r = 0; r < h; r++, di += rd(N_SPR_SKIP))
            for (c = 0; c < w4; c++, di++) {
                if (lrb(hide + ebx + di - 0x1500)) {
                    src++;
                } else {
                    uint8_t al = lrb(fs + src++);
                    if (al != 0xFF)
                        vga_write((uint16_t)di, al);
                }
            }
    }
}

/* CODE:2912B: DROPS_SEL's picture `n` (its offset in the table at the
 * block's start; width and height before the bytes) by SPRITE4_DRAW */
static void DROP_PIC(uint32_t n)
{
    uint32_t fs = pmax_base(rw(N_DROPS_SEL)), at = lrd(fs + n * 4);

    wd(N_SPR_W, lrd(fs + at));
    wd(N_SPR_H, lrd(fs + at + 4));
    wd(N_SPR_SRC, at + 8);
    SPRITE4_DRAW();
}

/* CODE:28FF8: the light [0000]: without bit 3 of +2 its LIGHTS_ONE1 byte
 * (+1Ch) 0, with it its drop target's picture 2 x +1Ch at +14h, +18h */
static void LIGHT_INIT(void)
{
    uint32_t b = rd(0x0000);

    if (!(rb(b + 2) & 8)) {
        wb(N_LIGHTS_ONE1 + rw(b + 0x1C), 0);
        return;
    }
    wd(N_SPR_X, rd(b + 0x14));
    wd(N_SPR_Y, rd(b + 0x18));
    DROP_PIC((uint32_t)rw(b + 0x1C) * 2);
}

/* CODE:29AF9: along header slot 14's groups (state+28DEh, a list of
 * pointers ended by 0; each group's byte +4 AND 6, its dword +0 the first
 * light, a light's +10h the next): each light not marked "not drawn"
 * (+2 bit 2) LIGHT_INIT, its byte in state+16B2h (by its running number)
 * 0, +3, +4, +0, +5 0, +1 FFh, +2 bits 0 and 1 cleared; the place
 * state+16AEh the first group, state+16ACh FFFFh; the lists at
 * state+17B8h and state+1822h emptied */
void LIGHTS_RESET(void)
{
    uint32_t st = rd(0x0014), g;

    wd(0x000C, rd(st + 0x28DE));
    wd(st + 0x16AE, rd(0x000C));
    ww(st + 0x16AC, 0xFFFF);
    wd(0x0038, 0);
    for (;;) {
        g = rd(rd(0x000C));
        wd(0x0020, g);
        wd(0x000C, rd(0x000C) + 4);
        if (!g)
            break;
        wb(g + 4, (uint8_t)(rb(g + 4) & 6));
        wd(0x0000, rd(g));
        for (;;) {
            uint32_t b = rd(0x0000), n, next;

            if (!(rb(b + 2) & 4))
                LIGHT_INIT();
            n = rd(0x0038);
            wb(rd(0x0014) + (uint32_t)(int32_t)(int16_t)n + 0x16B2, 0);
            b = rd(0x0000);
            wb(b + 3, 0);
            wb(b + 4, 0);
            wb(b, 0);
            wb(b + 5, 0);
            wb(b + 1, 0xFF);
            wb(b + 2, (uint8_t)(rb(b + 2) & 0xFC));
            wd(0x0038, (n & 0xFFFF0000u) | (uint16_t)(n + 1));
            next = rd(b + 0x10);
            wd(0x0020, next);
            if (!next)
                break;
            wd(0x0000, next);
        }
    }
    st = rd(0x0014);
    wd(st + 0x17B8, st + 0x17BC);
    wd(st + 0x1822, st + 0x1826);
}
