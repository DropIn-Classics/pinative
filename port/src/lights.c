/* lights.c - the light states of header slot 14: at the game's start
 * (LIGHTS_RESET, CODE:29AF9), a frame (LIGHTS_STEP, CODE:2EF7A) and their
 * flashing (FLASH_STEP, CODE:2ED15), and the drop targets' pictures they
 * draw (DROP_PIC, CODE:2912B; SPRITE4_DRAW, CODE:29265).
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

/* CODE:29155: a picture at LIGHTS_SEL:[SPR_SRC] of SPR_W x SPR_H bytes a
 * plane (the four planes one after the other, 0 transparent) at byte
 * SPR_X of line SPR_Y: into SPOOKY_SEL's planes 0..3, then into video
 * memory from 1500h where HIDELIGHTS_SEL's byte of the plane is 0 */
static void LIGHT_SPRITE(void)
{
    uint32_t fs = pmax_base(rw(N_LIGHTS_SEL)), src, di, ebx, r, c, k;
    uint32_t w = rd(N_SPR_W), h = rd(N_SPR_H);
    uint32_t spooky = pmax_base(rw(N_SPOOKY_SEL)), hide = pmax_base(rw(N_HIDELIGHTS_SEL));

    wd(N_SPR_SKIP, 0x54 - w);
    wd(N_SPR_AT, rd(N_SPR_Y) * 0x54 + rd(N_SPR_X));
    src = rd(N_SPR_SRC);
    for (k = 4; k > 0; k--) {
        di = rd(N_SPR_AT) + rd(N_SPR_PLANES + 4 * k);
        for (r = 0; r < h; r++, di += rd(N_SPR_SKIP))
            for (c = 0; c < w; c++, di++) {
                uint8_t al = lrb(fs + src++);
                if (al != 0)
                    lwb(spooky + di, al);
            }
    }
    src = rd(N_SPR_SRC);
    ebx = 0;
    for (k = 4; k > 0; k--, ebx += 0xC4E0) {
        di = rd(N_SPR_AT) + 0x1500;
        MAP_MASK((uint8_t)(0x10 >> k));
        for (r = 0; r < h; r++, di += rd(N_SPR_SKIP))
            for (c = 0; c < w; c++, di++) {
                if (lrb(hide + ebx + di - 0x1500)) {
                    src++;
                } else {
                    uint8_t al = lrb(fs + src++);
                    if (al != 0)
                        vga_write((uint16_t)di, al);
                }
            }
    }
}

/* CODE:2909F: LIGHTS_SEL's picture `n` (its offset in the dword table at
 * the block's start; x, y, width, height before the bytes) by
 * LIGHT_SPRITE */
static void LIGHT_PIC(uint32_t n)
{
    uint32_t fs = pmax_base(rw(N_LIGHTS_SEL)), at = lrd(fs + n * 4);

    wd(N_SPR_X, lrd(fs + at));
    wd(N_SPR_Y, lrd(fs + at + 4));
    wd(N_SPR_W, lrd(fs + at + 8));
    wd(N_SPR_H, lrd(fs + at + 12));
    wd(N_SPR_SRC, at + 16);
    LIGHT_SPRITE();
}

/* CODE:29033: from LIGHTS_DRAW_POS on, each light whose LIGHTS_ONE1 byte
 * differs from LIGHTS_ONE2 (then copied there) drawn: picture n when the
 * byte is not 0, n plus half the count otherwise (the count the first
 * dword of LIGHTS_SEL), and then the loop goes on from that picture's
 * number, passing over the lights between (presumably a slip); stopped
 * when the retrace has come (FRAME_DONE FFh), to go on from there the
 * next frame */
void LIGHTS_DRAW(void)
{
    uint32_t ecx = lrd(pmax_base(rw(N_LIGHTS_SEL))), edx = ecx >> 1;
    uint32_t esi = rd(N_LIGHTS_DRAW_POS);

    for (;;) {
        uint8_t al;

        if (rb(N_FRAME_DONE) == 0xFF)
            break;
        al = rb(N_LIGHTS_ONE1 + esi);
        if (al != rb(N_LIGHTS_ONE2 + esi)) {
            wb(N_LIGHTS_ONE2 + esi, al);
            /* the index itself moved on by half for a light off (ADD
             * ESI,EDX before PUSH ESI), so the loop goes on from there */
            if (al == 0)
                esi += edx;
            LIGHT_PIC(esi);
        }
        if (++esi >= ecx) {
            esi = 0;
            break;
        }
    }
    wd(N_LIGHTS_DRAW_POS, esi);
}

/* CODE:28F41: from DROPS_UPD_POS on, the 64h entries of DROP_PIECES (8
 * bytes: the state drawn, the state, x, y words) whose state changed
 * (then copied): DROPS_SEL's picture 2n, or 2n - 1 when the state is not
 * 0, at x, y (the words go into SPR_X and SPR_Y through AX, EAX's high
 * word 0 there: 6 from the driver's command 6, then SPRITE4_DRAW's);
 * stopped as LIGHTS_DRAW */
void DROPS_UPDATE(void)
{
    uint32_t esi = rd(N_DROPS_UPD_POS), e;

    for (;;) {
        uint8_t dl;

        if (rb(N_FRAME_DONE) == 0xFF)
            break;
        e = N_DROP_PIECES + esi * 8;
        dl = rb(e + 1);
        if (rb(e) != dl) {
            wb(e, dl);
            wd(N_SPR_X, rw(e + 2));
            wd(N_SPR_Y, rw(e + 4));
            DROP_PIC(dl != 0 ? esi * 2 - 1 : esi * 2);
        }
        if (++esi >= 0x64) {
            esi = 0;
            break;
        }
    }
    wd(N_DROPS_UPD_POS, esi);
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

/* CODE:28FBA: the light [0000] on: without bit 3 of +2 its LIGHTS_ONE1
 * byte FFh; with it the drop target's picture 2 x +1Ch - 1 (DROP_PIC) */
static void LIGHT_ON(void)
{
    uint32_t b = rd(0x0000);

    if (!(rb(b + 2) & 8)) {
        wb(N_LIGHTS_ONE1 + rw(b + 0x1C), 0xFF);
        return;
    }
    wd(N_SPR_X, rd(b + 0x14));
    wd(N_SPR_Y, rd(b + 0x18));
    DROP_PIC((uint32_t)rw(b + 0x1C) * 2 - 1);
}

/* CODE:2FE8E: [0000] into the ring of 64 dwords at [state+2A1Eh], the
 * index state+2A1Ah, the next entry 0 */
void EVENT_QUEUE(void)
{
    uint32_t st = rd(0x0014), ring = rd(st + 0x2A1E);
    int16_t n = (int16_t)rw(st + 0x2A1A);

    wd(0x0004, ring);
    ww(0x0028, (uint16_t)n);
    wd(ring + (uint32_t)(n * 4), rd(0x0000));
    wd(ring + (uint32_t)(n * 4) + 4, 0);
    ww(st + 0x2A1A, (uint16_t)((rw(st + 0x2A1A) + 1) & 0x3F));
}

/* CODE:2EF17: the group [0008] onto the list at state+1822h (a word 10h,
 * the group), bit 0 of each light's +2 set */
static void GROUP_FLASH(void)
{
    uint32_t save = rd(0x0004), st = rd(0x0014), p = rd(st + 0x1822), l;

    ww(p, 0x10);
    wd(p + 2, rd(0x0008));
    wd(0x0004, p + 6);
    wd(st + 0x1822, p + 6);
    wd(0x0000, rd(rd(0x0008)));
    for (;;) {
        l = rd(0x0000);
        wb(l + 2, (uint8_t)(rb(l + 2) | 1));
        wd(0x0020, rd(l + 0x10));
        if (!rd(0x0020))
            break;
        wd(0x0000, rd(0x0020));
    }
    wd(0x0004, save);
}

/* CODE:2EF7A: the lights of header slot 14's groups a frame, from where
 * the last call stopped (state+16AEh, the light's number state+16ACh):
 * up to 20h lights visited and 8 drawn; blinking counted; a group whose
 * lights are all shown gets its stream queued and flashes, once (see the
 * hints); then, at a new press of a flipper key, the lane change */
void LIGHTS_STEP(void)
{
    uint32_t st = rd(0x0014), p, g, l, nx;
    int16_t n;

    wd(0x000C, rd(st + 0x16AE));
    ww(0x0038, rw(st + 0x16AC));
    wd(0x0030, 0x20);
    ww(0x0034, rw(st + 0x0D72));
    wd(0x003C, 8);
    for (;;) {                                          /* CODE:2EFC6 */
        ww(st + 0x16AC, rw(0x0038));
        if ((int16_t)rw(0x0030) < 0)
            goto stop;
        wd(0x002C, 0);
        p = rd(0x000C);
        wd(0x0020, rd(p));
        wd(0x000C, p + 4);
        if (!rd(0x0020)) {
            p = rd(st + 0x28DE);
            wd(0x0038, 0xFFFFFFFFu);
            ww(st + 0x16AC, 0xFFFF);
            wd(0x0020, rd(p));
            wd(0x000C, p + 4);
            if (!rd(0x0020))
                goto back;
        }
        /* CODE:2F044 */
        wd(0x0010, rd(0x0020));
        wd(0x0000, rd(rd(0x0020)));
        for (;;) {                                      /* CODE:2F05A */
            int shown = 0;

            ww(0x0038, (uint16_t)(rw(0x0038) + 1));
            l = rd(0x0000);
            if (rb(l + 2) & 2) {
                wd(0x002C, 0xFFFFFFFFu);
                wb(l + 3, (uint8_t)(rb(l + 3) - 1));
                if (rb(l + 3) & 0x80) {
                    wb(l + 1, (uint8_t)~rb(l + 1));
                    wb(l + 3, rb(l + 4));
                }
            }
            if (rb(l + 1) != 0) {
                wb(0x0020, (uint8_t)(rb(l) | rb(l + 5)));
                shown = rd(0x0020) >> (rd(0x0034) & 0x1F) & 1;
            }
            n = (int16_t)rw(0x0038);
            if (shown) {
                if (rb(st + 0x16B2 + (uint32_t)(int32_t)n) != 0)
                    goto next;
                wb(st + 0x16B2 + (uint32_t)(int32_t)n, 0xFF);
                if (!(rb(l + 2) & 4))
                    LIGHT_ON();
            } else {                                    /* CODE:2F0F6 */
                wd(0x002C, 0xFFFFFFFFu);
                if (rb(st + 0x16B2 + (uint32_t)(int32_t)n) == 0)
                    goto next;
                wb(st + 0x16B2 + (uint32_t)(int32_t)n, 0);
                if (!(rb(rd(0x0000) + 2) & 4))
                    LIGHT_INIT();
            }
            /* CODE:2F139 */
            ww(0x003C, (uint16_t)(rw(0x003C) - 1));
            if (rw(0x003C) == 0)
                goto back;
        next:                                           /* CODE:2F14F */
            ww(0x0030, (uint16_t)(rw(0x0030) - 1));
            nx = rd(rd(0x0000) + 0x10);
            wd(0x0020, nx);
            if (!nx)
                break;
            wd(0x0000, nx);
        }
        /* CODE:2F17D: every light shown, none blinking */
        if (rb(0x002C) & 0x80)
            continue;
        g = rd(0x0010);
        if (rb(g + 4) & 2)
            continue;
        if (rb(g + 4) & 1)
            continue;
        wb(g + 4, (uint8_t)(rb(g + 4) | 1));
        wd(0x0020, rd(g + 6));
        if (rd(0x0020)) {
            wd(0x0000, rd(0x0020));
            EVENT_QUEUE();
        }
        wd(0x0008, rd(0x0010));
        GROUP_FLASH();
    }
back:                                                   /* CODE:2F1D6 */
    wd(0x000C, rd(0x000C) - 4);
stop:                                                   /* CODE:2F1E5 */
    wd(st + 0x16AE, rd(0x000C));
    wb(0x0020, (uint8_t)(rb(st + 0x2A7C) | rb(st + 0x2A7B)));
    if (rb(0x0020) == 0) {
        wb(st + 0x2A7A, 0);
        return;
    }
    if (rb(st + 0x2A7A) != 0)
        return;
    wb(st + 0x2A7A, 0xFF);
    /* the lane change: the groups listed backwards before slot 14's */
    wd(0x0000, rd(st + 0x28DE));
    for (;;) {                                          /* CODE:2F235 */
        wd(0x0000, rd(0x0000) - 4);
        g = rd(rd(0x0000));
        wd(0x0020, g);
        if (!g)
            return;
        wd(0x0004, rd(g));
        wb(0x0020, rb(rd(g)));
        for (;;) {                                      /* CODE:2F268 */
            l = rd(0x0004);
            nx = rd(l + 0x10);
            wd(0x0024, nx);
            if (!nx)
                break;
            wd(0x0008, nx);
            wb(l, rb(nx));
            wd(0x0004, nx);
        }
        wb(rd(0x0004), rb(0x0020));
    }
}

/* CODE:2ED20: the flashing light state+17B2h (count state+17B6h: +1 0
 * while bit 1 of the count is set, else FFh; at 0 done, bit 0 of +2
 * cleared); with none, the next from the stack of 6-byte entries (a
 * word count, the light) below state+17B8h */
static void LIGHT_FLASH_STEP(void)
{
    uint32_t st = rd(0x0014), p;
    uint16_t di;

    wd(0x0020, rd(st + 0x17B2));
    if (!rd(0x0020)) {                                  /* CODE:2ED7C */
        p = rd(st + 0x17B8);
        wd(0x0000, p);
        if (p == 0xE2FA)                                /* state+17BCh */
            return;
        wd(st + 0x17B2, rd(p - 4));
        wd(0x0000, p - 6);
        ww(st + 0x17B6, rw(p - 6));
        wd(st + 0x17B8, rd(st + 0x17B8) - 6);
        return;
    }
    wd(0x0004, rd(0x0020));
    ww(0x0020, rw(st + 0x17B6));
    if (rw(0x0020) == 0) {                              /* CODE:2EDBC */
        wd(st + 0x17B2, 0);
        p = rd(0x0004);
        wb(p + 2, (uint8_t)(rb(p + 2) & ~1));
        return;
    }
    di = (uint16_t)(rw(0x0020) & 2);
    ww(0x0020, di);
    wb(rd(0x0004) + 1, di ? 0 : 0xFF);
    ww(st + 0x17B6, (uint16_t)(rw(st + 0x17B6) - 1));
}

/* CODE:2EDD8: the same for the flashing group state+181Ch (count
 * state+1820h, all its lights' +1; the stack below state+1822h); at the
 * count's end the group's +4 bit 0 cleared and its lights' +0 and +5
 * lose the player's bits (state+0D74h), +2 bit 0 cleared */
static void GROUP_FLASH_STEP(void)
{
    uint32_t st = rd(0x0014), p, l;
    uint8_t v;

    wd(0x0020, rd(st + 0x181C));
    if (!rd(0x0020)) {                                  /* CODE:2EE73 */
        p = rd(st + 0x1822);
        wd(0x0000, p);
        if (p == 0xE364)                                /* state+1826h */
            return;
        wd(st + 0x181C, rd(p - 4));
        wd(0x0000, p - 6);
        ww(st + 0x1820, rw(p - 6));
        wd(st + 0x1822, rd(st + 0x1822) - 6);
        return;
    }
    wd(0x0004, rd(0x0020));
    ww(0x0020, rw(st + 0x1820));
    if (rw(0x0020) == 0) {                              /* CODE:2EEB3 */
        wd(st + 0x181C, 0);
        p = rd(0x0004);
        wb(p + 4, (uint8_t)(rb(p + 4) & ~1));
        wd(0x0008, rd(p));
        ww(0x0024, (uint16_t)~rw(st + 0x0D74));
        for (;;) {
            l = rd(0x0008);
            wb(l, (uint8_t)(rb(l) & rb(0x0024)));
            wb(l + 5, (uint8_t)(rb(l + 5) & rb(0x0024)));
            wb(l + 2, (uint8_t)(rb(l + 2) & ~1));
            wd(0x0020, rd(l + 0x10));
            if (!rd(0x0020))
                return;
            wd(0x0008, rd(0x0020));
        }
    }
    wd(0x0008, rd(rd(0x0004)));
    ww(0x0020, (uint16_t)(rw(0x0020) & 2));
    v = rw(0x0020) ? 0 : 0xFF;
    for (;;) {
        l = rd(0x0008);
        wb(l + 1, v);
        wd(0x0020, rd(l + 0x10));
        if (!rd(0x0020))
            break;
        wd(0x0008, rd(0x0020));
    }
    ww(st + 0x1820, (uint16_t)(rw(st + 0x1820) - 1));
}

/* CODE:2ED15 */
void FLASH_STEP(void)
{
    LIGHT_FLASH_STEP();
    GROUP_FLASH_STEP();
}
