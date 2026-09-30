/* display.c - the dot-matrix display's streams: DISPLAY_QUEUE
 * (CODE:2FEDB) puts a display record on the queue (or makes it the
 * background stream), DISPLAY_RUN (CODE:2F35C) runs them a frame, and
 * ANIMS_STEP (CODE:27A0E) the display's animations; the opcodes as the
 * port reaches them (docs/HANDOFF.md, "The attract mode's display").
 */
#include <stdio.h>
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

static uint32_t sx16(uint16_t v)
{
    return (uint32_t)(int32_t)(int16_t)v;
}

/* CODE:2F9CF, display opcode 1 (an animation record, five words): the
 * record to state+2A50h, the words to its +6..+0Eh (the first shifted
 * right by 4 and doubled), +12h from +22h, +10h and +16h 0 */
static void OP_ANIM(void)
{
    uint32_t p = rd(0x0004), a = rd(p + 2), ecx;

    wd(0x0000, a);
    wd(rd(0x0014) + 0x2A50, a);
    ecx = sx16(rw(p + 6));
    wd(0x0024, sx16(rw(p + 8)));
    wd(0x0028, sx16(rw(p + 10)));
    wd(0x002C, sx16(rw(p + 12)));
    wd(0x0030, sx16(rw(p + 14)));
    ecx = (ecx & 0xFFFF0000u) | (uint16_t)((uint16_t)ecx >> 4 << 1);
    wd(0x0020, ecx);
    a = rd(0x0000);
    ww(a + 6, (uint16_t)ecx);
    ww(a + 8, (uint16_t)rd(0x0024));
    ww(a + 10, (uint16_t)rd(0x0028));
    ww(a + 12, (uint16_t)rd(0x002C));
    ww(a + 14, (uint16_t)rd(0x0030));
    ww(a + 0x12, rw(a + 0x22));
    wb(a + 0x10, 0);
    wd(a + 0x16, 0);
}

/* CODE:2FB6F, display opcode 7 (a word): a wait of the word times
 * FRAME_RATE (state+50h) frames, state+2A3Eh, and state+2A3Ch FFh */
static void OP_WAIT(void)
{
    uint32_t st = rd(0x0014);
    uint32_t edi = (uint32_t)rw(rd(0x0004) + 2) * rw(st + 0x50);

    wd(0x0020, edi);
    ww(st + 0x2A3E, (uint16_t)edi);
    wb(st + 0x2A3C, 0xFF);
}

/* a display opcode's routine by its address */
static void display_op(uint32_t a)
{
    static char name[16];

    switch (a) {
    case 0x2F9CF:
        OP_ANIM();
        break;
    case 0x2FB6F:
        OP_WAIT();
        break;
    case 0x2FA55:               /* opcode 11h, a RET */
    case 0x2FBA4:               /* opcodes 4, 15h, 16h, 17h, RETs */
    case 0x2FBA5:
    case 0x2FBA6:
    case 0x2FBA7:
        break;
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

/* CODE:27C2A: the animation `ebp`'s next frame into DM_ANIM's block (a
 * new start, +16h 0, takes frame +24h, or 1 when that is not below the
 * frame count, the first dword of DM_ANIMS_SEL; its header words width,
 * height, count to +1Ah, +1Eh, +22h and +12h): from x +6 x 4, 0A0h bytes a
 * line, each data byte up to 4 a pixel of colour FCh + byte, above 4
 * that many less 4 pixels passed over; the data's end back */
static uint32_t ANIM_FRAME(uint32_t ebp)
{
    uint32_t fs = pmax_base(rw(N_DM_ANIMS_SEL)), es, edi, ebx, ecx, edx;

    if (rd(ebp + 0x16) == 0) {
        /* CODE:27C98 */
        ebx = rd(ebp + 0x24);
        if (ebx >= lrd(fs))
            ebx = 1;
        ebx = lrd(fs + ebx * 4);
        wd(ebp + 0x16, ebx + 6);
        wd(ebp + 0x1A, lrw(fs + ebx));
        wd(ebp + 0x1E, lrw(fs + ebx + 2));
        ww(ebp + 0x22, lrw(fs + ebx + 4));
        ww(ebp + 0x12, lrw(fs + ebx + 4));
    }
    es = pmax_base(rw(N_DM_ANIM));
    edi = (uint32_t)rw(ebp + 6) << 2;
    ebx = rd(ebp + 0x16);
    ecx = rd(ebp + 0x1E);
    do {
        edx = rd(ebp + 0x1A);
        do {
            uint8_t al = lrb(fs + ebx++);

            if (al <= 4) {
                lwb(es + edi++, (uint8_t)(al + 0xFC));
                edx--;
            } else {
                edi += (uint32_t)(al - 4);
                edx -= (uint32_t)(al - 4);
            }
        } while (edx != 0);
        edi = edi - rd(ebp + 0x1A) + 0xA0;
    } while (--ecx != 0);
    wd(0x0004, ebx);
    return ebx;
}

/* CODE:27ABD: from the animation [0020] along the list (+0 the next,
 * [0008] the one before): one waiting (+10h, from +11h after each frame)
 * ends the call; else its next frame (ANIM_FRAME, the data's position to
 * +16h), and after +12h frames (from +22h) it starts again while +0Eh
 * counts down, else it leaves the list (+0 cleared; state+2A50h/2A54h,
 * the list's head and last, mended).  The background animation
 * (ANIMS_BG_ONLY) starts again for ever */
static void anims_play(uint32_t st)
{
    uint32_t ebp;

    wd(0x0000, rd(0x0020));
    ebp = rd(0x0000);
    for (;;) {
        if (rb(ebp + 0x10) != 0) {
            wb(ebp + 0x10, (uint8_t)(rb(ebp + 0x10) - 1));
            return;
        }
        wb(ebp + 0x10, rb(ebp + 0x11));
        wd(0x0020, rd(ebp + 0x16));
        wd(ebp + 0x16, ANIM_FRAME(ebp));
        if (rb(N_ANIMS_BG_ONLY) != 0) {
            ww(ebp + 0x12, (uint16_t)(rw(ebp + 0x12) - 1));
            if (rw(ebp + 0x12) == 0) {
                ww(ebp + 0x12, rw(ebp + 0x22));
                wd(ebp + 0x16, 0);
            }
            return;
        }
        /* CODE:27B39 */
        ww(ebp + 0x12, (uint16_t)(rw(ebp + 0x12) - 1));
        if (rw(ebp + 0x12) == 0) {
            int leave = 1;

            if (rw(ebp + 0x0E) != 0) {
                uint32_t old16 = rd(ebp + 0x16);

                ww(ebp + 0x0E, (uint16_t)(rw(ebp + 0x0E) - 1));
                ww(ebp + 0x12, rw(ebp + 0x22));
                wd(ebp + 0x16, 0);
                leave = old16 == 0;
            }
            if (leave) {
                /* CODE:27B6F */
                wd(0x0020, rd(ebp));
                if (rd(0x0020) != 0) {
                    if (rd(0x0008) != 0)
                        wd(rd(0x0008), rd(0x0020));
                    else
                        wd(st + 0x2A50, rd(0x0020));
                } else if (rd(0x0008) != 0) {
                    wd(rd(0x0008), 0);
                    wd(st + 0x2A54, rd(0x0008));
                } else {
                    wd(st + 0x2A50, rd(0x0008));
                    wd(st + 0x2A54, rd(0x0008));
                }
                wd(ebp, 0);
            }
        }
        /* CODE:27BF2 */
        wd(0x0008, rd(0x0000));
        wd(0x0020, rd(ebp));
        if (rd(0x0020) == 0)
            return;
        wd(0x0000, rd(0x0020));
        ebp = rd(0x0000);
    }
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
    if (rd(0x0020) != 0) {
        /* CODE:27A7F: the background animation set back to its start */
        wd(0x0024, rd(st + 0x2A5C));
        if (rd(0x0024) != 0) {
            wd(0x0000, rd(0x0024));
            ebx = rd(0x0000);
            ww(ebx + 0x12, rw(ebx + 0x22));
            wd(ebx + 0x16, 0);
        }
        anims_play(st);
        return;
    }
    wb(N_ANIMS_BG_ONLY, 0xFF);
    if (rw(st + 0x2A2E) == 0) {
        wd(0x0020, rd(st + 0x2A5C));
        if (rd(0x0020) != 0) {
            anims_play(st);
            return;
        }
    }
    /* CODE:27A62 */
    ebx = rd(st + 0x2A5C);
    ww(ebx + 0x12, rw(ebx + 0x22));
    wd(ebx + 0x16, 0);
}
