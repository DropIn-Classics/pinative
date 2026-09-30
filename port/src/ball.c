/* ball.c - the balls on the screen a frame: taken off where they were
 * (BALLS_STEP, CODE:14DE2) and drawn where they are (BALLS_SHOW,
 * CODE:14C46), each with its mark in the hide-lights mask, by the
 * original's own sprite routines (codeint.c).
 */
#include "codeint.h"
#include "game.h"
#include "image.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* the segments as the table has them: DS, FS and GS the table's data
 * (the program's base), the rest as given */
static void cpu_init(CiCpu *c, uint32_t routine)
{
    int i;

    for (i = 0; i < 8; i++)
        c->r[i] = 0;
    for (i = 0; i < 6; i++)
        c->seg[i] = PI_IMAGE_BASE;
    c->r[0] = routine;          /* EAX: the routine's address (CALL EAX) */
    c->zf = c->cf = 0;
}

/* CODE:2690E: the ball [0010] drawn at +12h, +14h by the table's routine
 * of BALL_DRAWERS (DS video memory, GS the table's data, EBP +58h, EDI
 * 1500h); the DX and SI it gives back (the plane and mask, the video
 * address) to +6Eh and +70h */
static void BALL_DRAW(void)
{
    uint32_t b = rd(0x0010);
    CiCpu c;

    cpu_init(&c, rd(N_BALL_DRAWERS + 4 * (uint32_t)(uint8_t)(rb(N_TABLE_NUM) - 1)));
    c.r[2] = rw(b + 0x12);
    c.r[6] = rw(b + 0x14);
    c.r[7] = 0x1500;
    c.r[5] = rd(b + 0x58);
    c.seg[3] = CI_VIDEO;
    code_run(c.r[0], &c);
    ww(b + 0x6E, (uint16_t)c.r[2]);
    ww(b + 0x70, (uint16_t)c.r[6]);
}

/* CODE:26977: the ball [0010] taken off at +6Eh, +70h (not when +70h is
 * 0) by the table's routine of BALL_ERASERS (ES video memory, GS
 * SPOOKY_SEL's copy of the stage) */
static void BALL_ERASE(void)
{
    uint32_t b = rd(0x0010);
    CiCpu c;

    if (rw(b + 0x70) == 0)
        return;
    cpu_init(&c, rd(N_BALL_ERASERS + 4 * (uint32_t)(uint8_t)(rb(N_TABLE_NUM) - 1)));
    c.r[1] = rw(b + 0x6E);
    c.r[7] = rw(b + 0x70);
    c.seg[0] = CI_VIDEO;
    c.seg[5] = pmax_base(rw(N_SPOOKY_SEL));
    code_run(c.r[0], &c);
}

/* CODE:2600D: EAX written in the ball's shape into HIDELIGHTS_SEL's block
 * at pixel EDI (y x 150h + x) by one of the four routines of BALL_MASKS
 * (by EDI AND 3) at EDI / 4 */
static void BALL_MASK(uint32_t edi, uint32_t eax)
{
    CiCpu c;

    cpu_init(&c, eax);
    c.r[6] = edi & 3;
    c.r[7] = edi >> 2;
    c.seg[3] = pmax_base(rw(N_HIDELIGHTS_SEL));
    code_run(rd(N_BALL_MASKS + c.r[6] * 4), &c);
}

/* CODE:269B0: the ball's mark at +72h cleared */
static void BALL_UNMARK(void)
{
    BALL_MASK(rd(rd(0x0010) + 0x72), 0);
}

/* CODE:269D7: the ball's mark at its place (+72h = +14h x 150h + +12h),
 * 80808080h */
static void BALL_MARK(void)
{
    uint32_t b = rd(0x0010), edi = (uint32_t)rw(b + 0x14) * 0x150 + rw(b + 0x12);

    wd(b + 0x72, edi);
    BALL_MASK(edi, 0x80808080u);
}

/* CODE:14DE2: state+0D5Ah 0; each of the state+0D32h balls on the list at
 * state+1046h to [0010], taken off and its mark cleared (CODE:14DD6) */
/* CODE:14DD6: the ball [0010] taken off and its mark cleared */
void BALL_HIDE(void)
{
    BALL_ERASE();
    BALL_UNMARK();
}

void BALLS_STEP(void)
{
    uint32_t st = rd(0x0014);

    wd(0x0008, st + 0x1046);
    wd(0x003C, (rd(0x003C) & 0xFFFF0000u) | rw(st + 0x0D32));
    if (rw(st + 0x0D32) == 0)
        return;
    wd(st + 0x0D5A, 0);
    do {
        uint32_t esi = rd(0x0008);

        wd(0x0008, esi + 4);
        wd(0x0010, rd(esi));
        BALL_HIDE();
        ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    } while (rw(0x003C) != 0);
}

/* CODE:14C46: of the balls on the list, the lowest on the stage (the
 * largest y below 258h; with more than one ball only those whose byte
 * +0Bh is 0) to state+0D5Ah (x) and +0D5Ch (y), the first ball's place
 * to begin with; then each ball drawn and marked */
void BALLS_SHOW(void)
{
    uint32_t st = rd(0x0014), b, esi;

    wd(0x0000, st + 0x1046);
    wd(0x003C, (rd(0x003C) & 0xFFFF0000u) | rw(st + 0x0D32));
    if (rw(0x003C) == 0)
        return;
    b = rd(rd(0x0000));
    wd(0x0010, b);
    wd(st + 0x0D5A, rd(b + 0x12));
    do {
        uint16_t y;

        esi = rd(0x0000);
        b = rd(esi);
        wd(0x0000, esi + 4);
        wd(0x0010, b);
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | rw(b + 0x12));
        if (rw(b + 0x12) & 0x8000)
            wd(0x0020, 0);
        wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | rw(b + 0x14));
        if (rw(b + 0x14) & 0x8000)
            wd(0x0024, 0);
        y = rw(0x0024);
        if (y < 0x258 && (rw(st + 0x0D32) == 1 || rb(b + 0x0B) == 0) && y >= rw(st + 0x0D5C)) {
            ww(st + 0x0D5A, rw(0x0020));
            ww(st + 0x0D5C, y);
        }
        ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    } while (rw(0x003C) != 0);

    wd(0x0000, st + 0x1046);
    ww(0x003C, rw(st + 0x0D32));
    do {
        esi = rd(0x0000);
        wd(0x0000, esi + 4);
        wd(0x0010, rd(esi));
        BALL_DRAW();
        BALL_MARK();
        ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    } while (rw(0x003C) != 0);
}
