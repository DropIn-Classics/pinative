/* tblinit.c - TABLE_LOAD2's last steps: the top four colours, the balls'
 * records and the flippers' start (BALLS_INIT, CODE:B797).
 */
#include "game.h"
#include "names.h"
#include "pmem.h"
#include "vga.h"

/* CODE:BA9B: the twelve bytes at TOP_COLOURS, each shifted right 2, to the
 * DAC from colour FCh */
void TOP_COLOURS_SET(void)
{
    int i;

    vga_outb(0x3C8, 0xFC);
    for (i = 0; i < 12; i++)
        vga_outb(0x3C9, (uint8_t)(rb(N_TOP_COLOURS + (uint32_t)i) >> 2));
}

/* CODE:2C414 and CODE:2C467: the six pointers at state+`from` into the
 * ball at [0010]'s +50h..+64h, its byte +8 to `level` */
static void ball_bundle(uint32_t from, uint8_t level)
{
    uint32_t st = rd(0x0014), ball = rd(0x0010), i;

    for (i = 0; i < 6; i++)
        wd(ball + 0x50 + 4 * i, rd(st + from + 4 * i));
    wb(ball + 8, level);
}

/* CODE:29A5A: the ball at [0010] put at the plunger: its words +12h, +14h
 * to 11Ch, 1FEh (plus their low three bits) and shifted by 0Ah into +1Eh,
 * +22h, the high bytes of its words +0Eh, +10h to 2, byte +1 0, the
 * second bundle */
static void BALL_PLACE(void)
{
    uint32_t b = rd(0x0010), edi;

    ww(b + 0x12, (uint16_t)((rw(b + 0x12) & 7) + 0x11C));
    edi = (uint32_t)rw(b + 0x12) << 10;
    wd(b + 0x1E, edi);
    ww(b + 0x14, (uint16_t)((rw(b + 0x14) & 7) + 0x1FE));
    /* MOV DI keeps EDI's high word from the line before */
    edi = ((edi & 0xFFFF0000u) | rw(b + 0x14)) << 10;
    wd(0x0020, edi);
    wd(b + 0x22, edi);
    wd(b + 0x0E, (rd(b + 0x0E) & 0x00FF00FFu) + 0x02000200u);
    wb(b + 1, 0);
    ball_bundle(0x28BE, 0xFF);
}

/* CODE:B797 */
void BALLS_INIT(void)
{
    uint32_t st = rd(0x0014), p, ebx;
    int i;

    /* the 13 records at state+10AEh, 76h bytes each: the first bundle,
     * SLOPE_X and SLOPE_Y to +3Ch, +3Eh */
    wd(0x0010, st + 0x10AE);
    for (i = 0; i < 13; i++) {
        uint32_t b;

        ball_bundle(0x28A6, 0);
        b = rd(0x0010);
        st = rd(0x0014);
        ww(b + 0x3C, rw(st + 0x0E40));
        ww(b + 0x3E, rw(st + 0x0E3A));
        wd(0x0010, b + 0x76);
    }

    /* each listed at state+1046h and state+107Ah, numbered 1..13 in its
     * byte +0Ah and put at the plunger */
    st = rd(0x0014);
    wb(st + 0x0E34, 0);
    wb(st + 0x0E35, 0);
    wd(0x0000, st + 0x1046);
    wd(0x0004, st + 0x107A);
    wd(0x0010, st + 0x10AE);
    wd(0x003C, (rd(0x003C) & 0xFFFF0000u) | 0x0C);
    wd(0x0038, 1);
    do {
        uint32_t b = rd(0x0010), eax;

        wd(rd(0x0000), b);
        wd(0x0000, rd(0x0000) + 4);
        wd(rd(0x0004), b);
        wd(0x0004, rd(0x0004) + 4);
        wb(b + 0x0A, (uint8_t)rd(0x0038));
        BALL_PLACE();
        eax = rd(0x0038);
        wd(0x0038, (eax & 0xFFFFFF00u) | (uint8_t)(eax + 1));
        wd(0x0010, rd(0x0010) + 0x76);
        ebx = rd(0x003C);
        wd(0x003C, (ebx & 0xFFFF0000u) | (uint16_t)(ebx - 1));
    } while (ebx & 0xFFFF);
    wd(rd(0x0014) + 0x0D76, 0x0D7A);

    /* the flippers (header slot 22, 1F7h bytes each, type 0 ends them,
     * type 3 passed over): from the word +6 to +8 in steps of 1 (-1 when
     * the word +0Ah is 1) around 0..77h, the steps counted in +14h; then
     * +14h the steps times 40h less 1, +12h 0, or for -1 both the steps
     * times -40h and +1Ah FFFFh */
    for (p = rd(N_MODULE_HEADER + 0x58); rb(p) != 0; p += 0x1F7) {
        int16_t ax, bx;

        if (rb(p) == 3)
            continue;
        ax = (int16_t)rw(p + 6);
        bx = rw(p + 0x0A) == 1 ? -1 : 1;
        while ((uint16_t)ax != rw(p + 8)) {
            ww(p + 0x14, (uint16_t)(rw(p + 0x14) + 1));
            ax = (int16_t)(ax + bx);
            if (ax < 0)
                ax = (int16_t)(ax + 0x78);
            if (ax == 0x78)
                ax = 0;
        }
        ax = (int16_t)(rw(p + 0x14) << 6);
        ww(p + 0x14, (uint16_t)(ax - 1));
        ww(p + 0x12, 0);
        if (rw(p + 0x0A) == 1) {
            ax = (int16_t)-ax;
            ww(p + 0x12, (uint16_t)ax);
            ww(p + 0x14, (uint16_t)ax);
            ww(p + 0x1A, 0xFFFF);
        }
    }
}
