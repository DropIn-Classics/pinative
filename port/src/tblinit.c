/* tblinit.c - TABLE_LOAD2's last steps: the top four colours, the balls'
 * records and the flippers' start (BALLS_INIT, CODE:B797), the lights'
 * and drop targets' files (LIGHTS_LOAD, CODE:28C45), the flippers' data
 * and blocks (FLIPDAT_LOAD, CODE:15030).
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
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

/* CODE:BAB6: the same from TOP_COLOURS2 */
void TOP_COLOURS2_SET(void)
{
    int i;

    vga_outb(0x3C8, 0xFC);
    for (i = 0; i < 12; i++)
        vga_outb(0x3C9, (uint8_t)(rb(N_TOP_COLOURS2 + (uint32_t)i) >> 2));
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

/* a file loaded as the INT 94h AH=1 stubs do, its name at DS:`name`: its
 * selector to the word `sel`, its size to the dword `size`; 1 (CF) when
 * it is not there */
static int load(uint32_t name, uint32_t sel, uint32_t size)
{
    uint32_t n = 0;
    uint16_t s = pmax_load_ds(name, &n);

    ww(sel, s);
    wd(size, n);
    return !s;
}

/* CODE:28D64: drops.mgl; masks.mgl but on table 2 (its failure passed
 * over) */
static int DROPS_LOAD(void)
{
    if (load(N_DROPS_NAME, N_DROPS_SEL, N_DROPS_SIZE))
        return 1;
    if (rb(N_TABLE_NUM) != 2)
        load(N_MASKS_NAME, N_MASKS_SEL, N_MASKS_SIZE);
    return 0;
}

/* CODE:28C45 */
int LIGHTS_LOAD(void)
{
    uint32_t base = pmax_base(rw(N_HIDELIGHTS_SEL)), i;

    for (i = 0; i < 0xFF; i++) {
        wb(N_LIGHTS_ONE1 + i, 1);
        wb(N_LIGHTS_ONE2 + i, 1);
    }
    for (i = 0; i < 0x31380; i++)
        lwb(base + i, 0);
    if (load(N_LIGHTS_NAME, N_LIGHTS_SEL, N_LIGHTS_SIZE))
        return 1;
    wd(N_LIGHTS_POS, 0);
    return DROPS_LOAD();
}

/* a block's selector added to BLOCKS */
static void blocks_add(uint16_t sel)
{
    uint32_t p = rd(N_BLOCKS_END);

    ww(p, sel);
    wd(N_BLOCKS_END, p + 2);
}

/* CODE:150B0: for each flipper record (four; type 3 passed over), from
 * its angle +6 to +8 (by 1, or -1 when +0Ah is not 0, around 0..77h) a
 * rectangle of 4 words per angle into +F2h upwards (or +1EAh downwards):
 * x +2 less FLIP_SHAPES's x and 10h, y +4 less its y and 10h, +F0h, the
 * bottom (y less its y, plus its height); then two blocks of (the
 * heights' sum + 32h) x 40h bytes, cleared: "flipper gfx data" (its
 * selector to +1F2h) and "flipper mask data" (its DS offset to +28h, its
 * size to +2Ch), both added to BLOCKS */
static void FLIPPER_BLOCKS(void)
{
    uint32_t p = rd(rd(0x0014) + 0x28FE), n, at, i;
    int k;

    for (k = 0; k < 4; k++, p += 0x1F7) {
        uint32_t edx, di;
        uint16_t bx = 0;
        int step = 1;
        uint16_t sel;

        if (rb(p) == 3)
            continue;
        edx = rw(p + 6);
        di = p + 0xF2;
        if (rw(p + 0x0A) != 0) {
            di = p + 0x1EA;
            step = -1;
        }
        for (;;) {
            uint32_t sh = N_FLIP_SHAPES + edx * 4;
            uint16_t y = (uint16_t)(rw(p + 4) - rb(sh + 1));

            bx = (uint16_t)(bx + rb(sh + 2));
            ww(di, (uint16_t)(rw(p + 2) - rb(sh) - 0x10));
            ww(di + 2, (uint16_t)(y - 0x10));
            ww(di + 4, rw(p + 0xF0));
            ww(di + 6, (uint16_t)(y + rb(sh + 2)));
            di += (uint32_t)(step * 8);
            if ((uint16_t)edx == rw(p + 8))
                break;
            edx += (uint32_t)step;
            if ((int16_t)edx >= 0x78)
                edx = 0;
            if ((int16_t)edx < 0)
                edx = 0x77;
        }
        n = ((uint32_t)bx + 0x32) * 0x40;
        sel = pmax_alloc(n);
        ww(N_FLIP_BLOCK_SEL, sel);
        blocks_add(sel);
        ww(p + 0x1F2, sel);
        at = pmax_base(sel);
        for (i = 0; i < n; i++)
            lwb(at + i, 0);
        wd(p + 0x2C, n);
        sel = pmax_alloc(n);
        ww(N_FLIP_BLOCK_SEL, sel);
        blocks_add(sel);
        wd(p + 0x28, pmax_base(sel) - rd(N_TABLE_BASE));
        at = pmax_base(sel);
        for (i = 0; i < rd(p + 0x2C); i++)
            lwb(at + i, 0);
    }
}

/* CODE:15030: 1 (CF) when flipdat1.m is not there */
int FLIPDAT_LOAD(void)
{
    uint16_t sel = pmax_load_ds(N_FLIPDAT_NAME, NULL);

    if (!sel)
        return 1;
    ww(N_FLIPDAT_SEL, sel);
    blocks_add(sel);
    /* CODE:150A5 */
    FLIPPER_BLOCKS();
    FLIPPER_RENDER();
    return 0;       /* CODE:156C3 is a RET */
}

/* CODE:B0E3: the table's two multiball counts (MULTIBALL_COUNTS, two
 * dwords a table) each lowered to OPT_MULTIBALL's maximum (MULTIBALL_MAX)
 * and, where not 0, written to the word +2 of the records header slots
 * 43 and 44 point to (the module's +ACh, +B0h); 1 (CF) when the option is
 * not 0..3 */
int MULTIBALL_CAP(void)
{
    uint32_t t = (uint32_t)(uint8_t)(rb(N_TABLE_NUM) - 1) * 8, max, esi, edi, m;

    esi = rd(N_MULTIBALL_COUNTS + t);
    edi = rd(N_MULTIBALL_COUNTS + 4 + t);
    if (rb(N_OPT_MULTIBALL) >= 4)
        return 1;
    max = rd(N_MULTIBALL_MAX + 4 * (uint32_t)rb(N_OPT_MULTIBALL));
    if (max < esi)
        esi = max;
    if (max < edi)
        edi = max;
    m = rd(N_MODULE_BASE);
    if (esi)
        ww(rd(m + 0xAC) + 2, (uint16_t)esi);
    if (edi)
        ww(rd(m + 0xB0) + 2, (uint16_t)edi);
    return 0;
}
