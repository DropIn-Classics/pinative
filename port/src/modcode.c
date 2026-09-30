/* modcode.c - the table modules' own routines (SOURCE\T00n.BPC; names
 * in src/T00n.hints), in C per table, called by their address in the
 * loaded module as the main program calls them; their calls back into
 * the main program go through the host vector in [CODE:0010]
 * (CODE:2CD10, docs/bpc-module.md).  A routine not translated stops the
 * port by its table and offset.
 */
#include <stdio.h>
#include "game.h"
#include "names.h"
#include "pmem.h"

static uint32_t sx16(uint16_t v)
{
    return (uint32_t)(int32_t)(int16_t)v;
}

/* the main program's routine at [[0010] + n], by its address */
static void host_call(uint32_t n)
{
    static char name[48];
    uint32_t a = rd(rd(0x0010) + n);

    switch (a) {
    case N_RECORD_DISPATCH:
        RECORD_DISPATCH();
        break;
    case N_DM_CLEAR:
        DM_CLEAR();
        break;
    case N_FRAMES_WAIT:
        FRAMES_WAIT();
        break;
    case N_DM_SCORE_DRAW:
        DM_SCORE_DRAW();
        break;
    case N_DM_TEXT_DRAW:
        DM_TEXT_DRAW();
        break;
    case N_DEC_TEXT:
        DEC_TEXT();
        break;
    case N_EVENT_QUEUE:
        EVENT_QUEUE();
        break;
    default:
        snprintf(name, sizeof name, "the host vector's +%Xh (CODE:%X)", (unsigned)n, (unsigned)a);
        pi_stop(name);
    }
}

/* ---- slot 32, the end-of-ball bonus (MOD_BALL_BONUS) ---- */

/* where each table's copy keeps its data (offsets in the module, from
 * build/T00n.ASM); the four copies are one routine but for the combos
 * (none on table 2) and the multiplier's lamp (table 3) */
struct bonus_data {
    uint16_t code;      /* MOD_BALL_BONUS */
    uint32_t audio;     /* the 12-byte audio record passed to +04h */
    uint16_t flag;      /* the blink toggle (C3D5C on table 1) */
    uint16_t count;     /* its eight rounds (C3D5D) */
    uint16_t digits;    /* four '0's and the number 1 (C3D5E) */
    uint16_t sum;       /* the combos' count, 12 digits ending at +8 */
    uint16_t text;      /* the combos' text record (C3D72) */
    uint16_t chars;     /* its characters (C3D7A) */
    uint16_t rec_bonus; /* the text records "BONUS", "TOTAL BONUS", */
    uint16_t rec_total; /* "NO BONUS" */
    uint16_t rec_none;
    uint16_t lamps;     /* the multiplier's text records, 4 each */
    uint16_t combos;    /* the players' combo counts, a word each (0: none) */
};

static const struct bonus_data bonus_tables[4] = {
    { 0x37AD, 0x11F75, 0x3D5C, 0x3D5D, 0x3D5E, 0x3D6A, 0x3D72, 0x3D7A,
      0x3D86, 0x3D94, 0x3DA8, 0x3DBA, 0x5750 },
    { 0x381D, 0x1A910, 0x3DBB, 0x3DBC, 0x3DBD, 0x3DC9, 0x3DD1, 0x3DD9,
      0x3DE6, 0x3DF4, 0x3E08, 0x3E1A, 0 },
    { 0x370D, 0x1EDD3, 0x3CB4, 0x3CB5, 0x3CB6, 0x3CC2, 0x3CCA, 0x3CD2,
      0x3CDE, 0x3CEC, 0x3D00, 0x3D12, 0x494C },
    { 0x36D1, 0x195B1, 0x3C05, 0x3C06, 0x3C07, 0x3C13, 0x3C1B, 0x3C23,
      0x3C30, 0x3C3E, 0x3C52, 0x3C64, 0x55FC },
};

/* the number [0000] points past drawn by host +10h at 9Eh, line 0Ah,
 * font 4, centred */
static void score_draw(void)
{
    ww(0x002C, 0x9E);
    wd(0x0030, 0x0A);
    wd(0x0034, 4);
    wd(0x0038, 2);
    host_call(0x10);
}

/* table 1 CODE:37AD (2 381D, 3 370D, 4 36D1): the audio record to +04h;
 * the bonus (the number ending at state+2AC8h) copied to the total
 * (ending at state+2AC8h - 8, state+2AC0h); if not 0, eight rounds of
 * "BONUS" over the player's bonus, every other round the multiplier's
 * text (the player's word +12h: its half - 1, on table 3 it - 2, picks
 * the record, drawn at x 28h and 118h), 0.2 s each; the player's combos
 * (not on table 2) counted into the total and shown ("n COMBO(S)",
 * 2 s); then "TOTAL BONUS" and the total 2 s, or "NO BONUS" 3 s */
static void MOD_BALL_BONUS(const struct bonus_data *d, unsigned table)
{
    uint32_t m = rd(N_MODULE_BASE), st, pl, e, q, bl;
    uint16_t n, c;

    wd(0x0000, m + d->audio);
    host_call(0x04);
    st = rd(0x0014);
    wd(st + 0x2AC0, 0);
    wd(st + 0x2AC4, 0);
    wb(m + d->flag, 0);
    wb(m + d->count, 8);
    wd(st + 0x2AC0, rd(st + 0x2AC8));
    wd(st + 0x2AC4, rd(st + 0x2ACC));
    if (rd(st + 0x2AC8) != 0 || rd(st + 0x2ACC) != 0) {
        do {
            host_call(0x08);
            wd(0x0000, rd(rd(0x0014) + 0x0D76) + 0x10);
            score_draw();
            wd(0x0000, m + d->rec_bonus);
            host_call(0x14);
            if (rb(m + d->flag) == 0) {
                pl = rd(rd(0x0014) + 0x0D76);
                wd(0x0000, pl);
                n = rw(pl + 0x12);
                n = table == 3 ? (uint16_t)(n - 2) : (uint16_t)((n >> 1) - 1);
                ww(0x003C, n);
                if (!(n & 0x8000)) {
                    e = rd(m + d->lamps + sx16(n) * 4);
                    wd(0x0000, e);
                    ww(e, 0x28);
                    host_call(0x14);
                    wd(0x0000, e);
                    ww(e, 0x118);
                    host_call(0x14);
                }
            }
            ww(0x0020, 0x0A);
            host_call(0x0C);
            wb(m + d->flag, (uint8_t)~rb(m + d->flag));
            wb(m + d->count, (uint8_t)(rb(m + d->count) - 1));
        } while (rb(m + d->count) != 0);
    }
    host_call(0x08);
    st = rd(0x0014);
    ww(0x0020, rw(st + 0x0D72));
    if (d->combos) {
        c = rw(m + d->combos + sx16(rw(st + 0x0D72)) * 2);
        ww(0x0020, c);
    } else {
        wd(0x0020, 0);
        c = 0;
    }
    if (c != 0) {
        wd(m + d->sum, 0);
        wd(m + d->sum + 4, 0);
        ww(0x0024, (uint16_t)(c - 1));
        do {
            wd(0x0004, m + d->text);
            wd(0x0008, m + d->sum);
            bcd12_add(rd(0x0004), rd(0x0008));
            wd(0x0008, rd(0x0008) - 6);
            wd(0x0004, rd(0x0004) - 6);
            n = rw(0x0024);
            ww(0x0024, (uint16_t)(n - 1));
        } while (n != 0);
        wd(0x0004, rd(0x0014) + 0x2AC8);
        wd(0x0008, m + d->text);
        bcd12_add(rd(0x0004), rd(0x0008));
        wd(0x0008, rd(0x0008) - 6);
        wd(0x0004, rd(0x0004) - 6);
        /* the count's digits, at most four, without leading zeros */
        wd(m + d->digits, 0x30303030);
        wd(0x0000, m + d->digits + 4);
        host_call(0x18);
        wd(0x0000, m + d->digits);
        wd(0x0004, m + d->chars);
        wd(0x0024, 3);
        for (;;) {
            q = rd(0x0000);
            bl = rb(q);
            wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | bl);
            wd(0x0000, q + 1);
            if (bl != 0x30)
                break;
            n = rw(0x0024);
            ww(0x0024, (uint16_t)(n - 1));
            if (n == 0)
                goto word;
        }
        q = rd(0x0004);
        wb(q, (uint8_t)bl);
        wd(0x0004, q + 1);
        n = rw(0x0024);
        ww(0x0024, (uint16_t)(n - 1));
        if (n != 0) {
            do {
                q = rd(0x0000);
                e = rd(0x0004);
                wb(e, rb(q));
                wd(0x0000, q + 1);
                wd(0x0004, e + 1);
                n = rw(0x0024);
                ww(0x0024, (uint16_t)(n - 1));
            } while (n != 0);
        }
    word:
        /* " COMBO", and "S" unless the count is 1 */
        q = rd(0x0004);
        wb(q, 0x20);
        wb(q + 1, 0x43);
        wb(q + 2, 0x4F);
        wb(q + 3, 0x4D);
        wb(q + 4, 0x42);
        wb(q + 5, 0x4F);
        q += 6;
        wd(0x0004, q);
        st = rd(0x0014);
        ww(0x0020, rw(st + 0x0D72));
        if (rw(m + d->combos + sx16(rw(st + 0x0D72)) * 2) != 1) {
            wb(q, 0x53);
            wd(0x0004, ++q);
        }
        wb(q, 0);
        wd(0x0004, q + 1);
        wd(0x0000, m + d->text);
        host_call(0x14);
        wd(0x0000, m + d->text);
        score_draw();
        ww(0x0020, 0x64);
        host_call(0x0C);
    }
    host_call(0x08);
    st = rd(0x0014);
    if (rd(st + 0x2AC0) != 0 || rd(st + 0x2AC4) != 0) {
        wd(0x0000, m + d->rec_total);
        host_call(0x14);
        wd(0x0000, rd(0x0014) + 0x2AC8);
        score_draw();
        ww(0x0020, 0x64);
        host_call(0x0C);
    } else {
        wd(0x0000, m + d->rec_none);
        host_call(0x14);
        ww(0x0020, 0x96);
        host_call(0x0C);
    }
}

/* ---- slot 41, at the next ball (MOD_NEXT_BALL) ---- */

/* table 1 CODE:00B6: with the player's multiplier (word +12h) n not 0,
 * the slot-16 counter at 4502h set to n/2 in the player's words +6 and
 * +16h, and the first n/2 light states of the chain at 942Eh (next +10h)
 * given the player's bit in byte +5 */
static void T1_NEXT_BALL(void)
{
    uint32_t m = rd(N_MODULE_BASE), st = rd(0x0014), c, l;
    uint16_t n, h, p;

    n = rw(rd(st + 0x0D76) + 0x12);
    ww(0x0020, n);
    if (n == 0)
        return;
    if (n & 1)
        pi_stop("MOD_NEXT_BALL: an odd multiplier, whose loop does not end (table 1, CODE:0123)");
    h = (uint16_t)(n >> 1);
    ww(0x0024, h);
    p = rw(st + 0x0D72);
    wd(0x0038, sx16(p));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    c = m + 0x4502;
    wd(0x0004, c);
    ww(c + sx16(p) * 2 + 6, h);
    ww(c + sx16(p) * 2 + 0x16, h);
    wd(0x0004, m + 0x942E);
    do {
        l = rd(0x0004);
        wb(l + 5, (uint8_t)(rb(l + 5) | (uint8_t)rd(0x003C)));
        wd(0x0004, rd(l + 0x10));
        n = (uint16_t)(rw(0x0020) - 2);
        ww(0x0020, n);
    } while (n != 0);
}

void MOD_CALL(uint32_t at)
{
    static char name[64];
    unsigned table = rb(N_TABLE_NUM);
    uint32_t off = at - rd(N_MODULE_BASE);

    if (rb(at) == 0xC3)
        return;
    if (table >= 1 && table <= 4 && off == bonus_tables[table - 1].code) {
        MOD_BALL_BONUS(&bonus_tables[table - 1], table);
        return;
    }
    if (table == 1 && off == 0x00B6) {
        T1_NEXT_BALL();
        return;
    }
    snprintf(name, sizeof name, "table %u's module at CODE:%X", table, (unsigned)off);
    pi_stop(name);
}
