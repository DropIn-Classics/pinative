/* display.c - the dot-matrix display's streams: DISPLAY_QUEUE
 * (CODE:2FEDB) puts a display record on the queue (or makes it the
 * background stream), DISPLAY_RUN (CODE:2F35C) runs them a frame, and
 * ANIMS_STEP (CODE:27A0E) the display's animations; the opcodes as the
 * port reaches them (docs/HANDOFF.md, "The attract mode's display").
 */
#include <stdio.h>
#include "game.h"
#include "names.h"
#include "pmem.h"

static uint32_t sx16(uint16_t v)
{
    return (uint32_t)(int32_t)(int16_t)v;
}

/* a display opcode's routine by its address */
static void display_op(uint32_t a)
{
    static char name[16];

    switch (a) {
    default:
        snprintf(name, sizeof name, "CODE:%X", (unsigned)a);
        pi_stop(name);
    }
}

/* the opcode word at [0004] of the record [0000]: its entry of
 * DISPLAY_OPTAB (routine, length) to [0020], [0024], the record's
 * position +4 moved on by the length; the routine's address */
static uint32_t op_fetch(uint32_t rec, uint16_t op)
{
    uint32_t e = N_DISPLAY_OPTAB + sx16(op) * 4;

    wd(0x0020, sx16(rw(e)));
    wd(0x0024, sx16(rw(e + 2)));
    ww(rec + 4, (uint16_t)(rw(rec + 4) + rw(e + 2)));
    return N_DISPLAY_OPCODES + sx16(rw(e));
}

/* the current player (state+0D72h) and its bit (+0D74h) to [0038],
 * [003C] */
static void player_cells(uint32_t st)
{
    wd(0x0038, sx16(rw(st + 0x0D72)));
    wd(0x003C, sx16(rw(st + 0x0D74)));
}

/* CODE:2F51A: no record running: the queue's priorities 0; unless an
 * animation plays, the background stream (state+2A58h) run from its
 * position to its end (opcode 0) in one go; with none, outside the
 * attract mode and while DISPLAY_BUSY, the idle display (CODE:2758B) */
static void background_run(uint32_t st)
{
    uint32_t rec, p;
    uint16_t op;

    wb(st + 0x2A3A, 0);
    wb(st + 0x2A3B, 0);
    if (rd(st + 0x2A50) != 0)
        return;
    rec = rd(st + 0x2A58);
    wd(0x0020, rec);
    if (rec == 0) {
        /* CODE:2F5E9 */
        if (rw(st + 0x8E) == 1 || rb(st + 0x2A74) != 0 || rb(N_DISPLAY_BUSY) != 0xFF)
            return;
        wb(N_DISPLAY_BUSY, 0);
        DM_CLEAR();
        pi_stop("CODE:2758B");
        return;
    }
    wd(0x0000, rec);
    for (;;) {
        st = rd(0x0014);
        player_cells(st);
        rec = rd(0x0000);
        p = rec + sx16(rw(rec + 4)) + 6;
        wd(0x0004, p);
        op = rw(p);
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | op);
        if (op == 0) {
            /* CODE:2F5D5 */
            ww(rd(0x0000) + 4, 0);
            wb(N_DISPLAY_BUSY, 0xFF);
            return;
        }
        display_op(op_fetch(rec, op));
        wd(0x0000, rec);
    }
}

void DISPLAY_RUN(void)
{
    uint32_t st = rd(0x0014), rec, ebx, e;
    uint16_t op;

    if (rw(st + 0x2A40) != 0) {
        ww(st + 0x2A40, (uint16_t)(rw(st + 0x2A40) - 1));
        wb(N_DISPLAY_BUSY, 0xFF);
        return;
    }
    if (rb(st + 0x2A3C) != 0) {
        if (rd(st + 0x2A50) != 0)
            return;
        if (rw(st + 0x2A3E) != 0) {
            ww(st + 0x2A3E, (uint16_t)(rw(st + 0x2A3E) - 1));
            wb(N_DISPLAY_BUSY, 0xFF);
            return;
        }
        wb(st + 0x2A3C, 0);
    }
    /* CODE:2F3BB */
    rec = rd(st + 0x2A2E);
    wd(0x0020, rec);
    if (rec == 0) {
        uint16_t bp = rw(st + 0x2A28);

        wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | bp);
        ebx = rd(st + 0x2A2A);
        e = ebx + sx16(bp) * 4;
        rec = rd(e);
        wd(0x0020, rec);
        if (rec == 0) {
            background_run(st);
            return;
        }
        wd(e, 0);
        ww(st + 0x2A28, (uint16_t)((rw(st + 0x2A28) + 1) & 0x3F));
        wd(st + 0x2A2E, rec);
        wd(0x0000, rec);
        ww(rec + 4, 0);
        DM_CLEAR();
        wd(0x0020, rec);
        wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | bp);
        wd(0x0000, rec);
    }
    /* CODE:2F453: one opcode of the current record */
    rec = rd(0x0020);
    wd(0x0000, rec);
    player_cells(rd(0x0014));
    e = rec + sx16(rw(rec + 4)) + 6;
    wd(0x0004, e);
    op = rw(e);
    wd(0x0020, (rec & 0xFFFF0000u) | op);
    if (op == 0) {
        /* CODE:2F4D5 */
        ww(rd(0x0000) + 4, 0);
        st = rd(0x0014);
        wd(st + 0x2A2E, 0);
        ww(st + 0x2A40, 0);
        ww(st + 0x2A3E, 0);
        wd(st + 0x2A50, 0);
        DM_CLEAR();
        wb(N_DISPLAY_BUSY, 0xFF);
        return;
    }
    display_op(op_fetch(rec, op));
    wb(N_DISPLAY_BUSY, 0xFF);
}

/* CODE:2FEDB: the display record [0000]: with bit 0 of its first word the
 * background stream (state+2A58h; the display cleared when no record
 * runs); else queued at state+2A26h in the ring of 64 at [state+2A2Ah]
 * when its priority (byte +2, then +3) is not below the queue's
 * (state+2A3Ah, 2A3Bh), the queue emptied first (and the display, the
 * top colours, the running record and animation reset) when it is
 * above, or when byte +2 is negative */
void DISPLAY_QUEUE(void)
{
    uint32_t esi = rd(0x0000), st, save4, q;
    uint8_t cl, bl;

    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | (rw(esi) & 1));
    if (rw(esi) & 1) {
        st = rd(0x0014);
        if (rd(st + 0x2A2E) == 0)
            DM_CLEAR();
        wd(st + 0x2A58, esi);
        wd(st + 0x2A5C, 0);
        return;
    }
    /* CODE:2FF23 */
    save4 = rd(0x0004);
    bl = rb(esi + 3);
    wd(0x0024, (rd(0x0024) & 0xFFFFFF00u) | bl);
    cl = rb(esi + 2);
    wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | cl);
    st = rd(0x0014);
    if (!(cl & 0x80)) {
        if (cl < rb(st + 0x2A3A))
            goto out;
        if (cl == rb(st + 0x2A3A)) {
            if (bl < rb(st + 0x2A3B))
                goto out;
            if (bl == rb(st + 0x2A3B))
                goto reset;
            goto add;
        }
    }
reset:
    /* CODE:2FF7A */
    wd(st + 0x2A2A, N_DISPLAY_QBUF);
    wd(rd(st + 0x2A2A), 0);
    ww(st + 0x2A26, 0);
    ww(st + 0x2A28, 0);
    {
        uint32_t c0 = rd(0x0000), c24 = rd(0x0024), c20 = rd(0x0020);

        DM_CLEAR();
        TOP_COLOURS_SET();
        wd(0x0020, c20);
        wd(0x0024, c24);
        wd(0x0000, c0);
    }
    st = rd(0x0014);
    wb(st + 0x2A3C, 0);
    wd(st + 0x2A2E, 0);
    ww(st + 0x2A40, 0);
    ww(st + 0x2A3E, 0);
    wd(st + 0x2A50, 0);
add:
    /* CODE:3000F */
    st = rd(0x0014);
    q = rd(st + 0x2A2A);
    wd(0x0004, q);
    wd(0x0028, (rd(0x0028) & 0xFFFF0000u) | rw(st + 0x2A26));
    wd(q + sx16(rw(st + 0x2A26)) * 4, rd(0x0000));
    wd(q + sx16(rw(st + 0x2A26)) * 4 + 4, 0);
    ww(st + 0x2A26, (uint16_t)((rw(st + 0x2A26) + 1) & 0x3F));
    wb(st + 0x2A3A, (uint8_t)rd(0x0020));
    wb(st + 0x2A3B, (uint8_t)rd(0x0024));
out:
    wd(0x0004, save4);
}

/* CODE:27A0E: ANIMS_BG_ONLY 0, [0008] 0; with no animation (state+2A50h)
 * ANIMS_BG_ONLY FFh and, while no record runs (the low word of
 * state+2A2Eh) and there is a background animation (state+2A5Ch), that
 * one; else the background animation's +12h from +22h and +16h cleared,
 * written through a null pointer when there is none (at CODE:0012 and
 * 0016: the CMP's flags are tested only after, and both ways lead to the
 * RET; presumably a slip) */
void ANIMS_STEP(void)
{
    uint32_t st = rd(0x0014), ebx;

    wb(N_ANIMS_BG_ONLY, 0);
    wd(0x0008, 0);
    wd(0x0020, rd(st + 0x2A50));
    if (rd(0x0020) != 0)
        pi_stop("CODE:27A7F");
    wb(N_ANIMS_BG_ONLY, 0xFF);
    if (rw(st + 0x2A2E) == 0) {
        wd(0x0020, rd(st + 0x2A5C));
        if (rd(0x0020) != 0)
            pi_stop("CODE:27ABD");
    }
    /* CODE:27A62 */
    ebx = rd(st + 0x2A5C);
    ww(ebx + 0x12, rw(ebx + 0x22));
    wd(ebx + 0x16, 0);
}
