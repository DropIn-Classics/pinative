/* modcode.c - the table modules' own routines (SOURCE\T00n.BPC; names
 * in src/T00n.hints), in C per table, called by their address in the
 * loaded module as the main program calls them; their calls back into
 * the main program go through the host vector in [CODE:0010]
 * (CODE:2CD10, docs/bpc-module.md).  A routine not translated stops the
 * port by its table and offset.
 */
#include <stdio.h>
#include "frame.h"
#include "game.h"
#include "names.h"
#include "pmax.h"
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

/* table 1 CODE:00B6, table 2 CODE:00BB (before its TUNE_COPY): with the
 * player's multiplier (word +12h) n not 0, the slot-16 counter at
 * `counter` set to n/2 in the player's words +6 and +16h, and the first
 * n/2 light states of the chain at `chain` (next +10h) given the
 * player's bit in byte +5 */
static void next_ball_lamps(uint32_t counter, uint32_t chain, unsigned table)
{
    uint32_t m = rd(N_MODULE_BASE), st = rd(0x0014), c, l;
    uint16_t n, h, p;
    static char why[96];

    n = rw(rd(st + 0x0D76) + 0x12);
    ww(0x0020, n);
    if (n == 0)
        return;
    if (n & 1) {
        snprintf(why, sizeof why, "MOD_NEXT_BALL: an odd multiplier, whose loop does not end (table %u)",
                 table);
        pi_stop(why);
    }
    h = (uint16_t)(n >> 1);
    ww(0x0024, h);
    p = rw(st + 0x0D72);
    wd(0x0038, sx16(p));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    c = m + counter;
    wd(0x0004, c);
    ww(c + sx16(p) * 2 + 6, h);
    ww(c + sx16(p) * 2 + 0x16, h);
    wd(0x0004, m + chain);
    do {
        l = rd(0x0004);
        wb(l + 5, (uint8_t)(rb(l + 5) | (uint8_t)rd(0x003C)));
        wd(0x0004, rd(l + 0x10));
        n = (uint16_t)(rw(0x0020) - 2);
        ww(0x0020, n);
    } while (n != 0);
}

/* Table 1 CODE:9A4E (SHOOT_START), the display game's opcode-14h
 * object: wait once, then clear the score and hit count, give it four
 * lives and put the crosshair in window 1.  Each of the four windows
 * gets its next delay from the module's 256-word delay table; the
 * table index at state+42h is kept and advanced as in the original. */
static void T1_SHOOT_START(void)
{
    uint32_t m = rd(N_MODULE_BASE), s = m + 0xA19F, w;
    uint16_t i, n;

    host_call(0x08);
    wd(0x0004, s);
    wb(s + 8, 0);
    ww(s, 0);
    wd(s + 0x0A, 0);
    wd(s + 0x0E, 0);
    ww(s + 4, 1);
    ww(s + 2, 4);
    wb(s + 6, 0);
    wb(s + 7, 0);

    w = s + 0x12;
    i = rw(s + 0x42);
    for (n = 0; n < 4; n++) {
        ww(w, rw(m + 0xA227 + (uint32_t)(i & 0xFF) * 2));
        ww(w + 2, 0);
        ww(w + 4, 0);
        w += 6;
        i = (uint16_t)((i + 1) & 0xFF);
    }
    ww(s + 0x42, i);
}

/* The module calls the main image's +28h callback before these blits;
 * that callback selects DM_ANIM for ES and VM_DATA for FS.  The port
 * keeps the same bytes in those two pMAX blocks. */
static void T1_SHOOT_PIC(uint32_t m, uint32_t picture, int transparent)
{
    uint32_t fs = pmax_base(rw(N_VM_DATA_SEL));
    uint32_t es = pmax_base(rw(N_DM_ANIM));
    uint32_t index = picture + 1;
    uint32_t src = fs + rd(fs + index * 4);
    uint32_t dst = es + rd(m + 0xA19B);
    uint32_t width = rd(m + 0xA193), height = rd(m + 0xA197), y, x;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            uint8_t c = lrb(src++);

            if (!transparent || c != 0)
                lwb(dst + x, (uint8_t)(c + 0xFC));
        }
        dst += 0xA0;
    }
}

/* A twelve-digit packed-BCD addition in the module, leaving the same
 * end pointers in the shared scratch cells as its DAA loop. */
static void T1_SHOOT_BCD_ADD(uint32_t dst, uint32_t src)
{
    wd(0x0000, dst);
    wd(0x000C, src);
    bcd12_add(dst, src);
    wd(0x0000, dst - 6);
    wd(0x000C, src - 6);
}

/* CODE:9E2D, the four windows. Their timer, step and picture number
 * occupy six bytes each at state+12h; pairs in A1F3 drive the animation. */
static void T1_SHOOT_WINDOWS(uint32_t m, uint32_t s)
{
    uint32_t w = s + 0x12, i;
    uint16_t count = 3;

    wd(0x0008, w);
    ww(0x0038, count);
    wd(0x003C, 0);
    for (i = 0; i < 4; i++, w += 6) {
        uint16_t look;

        if (rw(w) != 0) {
            ww(w, (uint16_t)(rw(w) - 1));
            look = rw(w + 4);
        } else {
            uint16_t step = (uint16_t)(rw(w + 2) + 1);

            if (step >= 10) {
                uint16_t at = rw(s + 0x42);

                ww(w + 2, 0);
                ww(w + 4, 0);
                ww(w, rw(m + 0xA227 + (uint32_t)(at & 0xFF) * 2));
                ww(s + 0x42, (uint16_t)((at + 1) & 0xFF));
                look = 0;
            } else {
                uint32_t pair = m + 0xA1F3 + (uint32_t)step * 4;

                ww(w + 2, step);
                ww(w, rw(pair));
                look = rw(pair + 2);
                if (look & 0x8000) {
                    ww(s + 2, (uint16_t)(rw(s + 2) - 1));
                    wd(0x0000, m + 0x11CA9);
                    host_call(0x04);
                    look &= 0x7FFF;
                }
                ww(w + 4, look);
            }
        }

        /* SHOOT_PIC uses the 1-based window frames after its own +1. */
        ww(0x0020, look);
        if (look != 0) {
            uint16_t cross = rw(s + 4);

            wd(0x0034, i * 0x300);
            if (look == 2 && i == cross) {
                uint16_t hits = (uint16_t)(rw(s) + 1);

                ww(s, hits);
                if (hits == 0x19)
                    wb(s + 8, 0xFF);
                wd(0x0000, m + 0x11CC3);
                host_call(0x04);
                T1_SHOOT_BCD_ADD(s + 0x12, m + 0xA447);
                /* A hit clears this window and starts its next delay now. */
                ww(w + 2, 0);
                ww(w + 4, 0);
                {
                    uint16_t at = rw(s + 0x42);
                    ww(w, rw(m + 0xA227 + (uint32_t)(at & 0xFF) * 2));
                    ww(s + 0x42, (uint16_t)((at + 1) & 0xFF));
                }
            } else {
                wd(m + 0xA19B, rw(m + 0xA1E3 + i * 2));
                wd(m + 0xA197, 0x10);
                wd(m + 0xA193, 0x20);
                T1_SHOOT_PIC(m, i * 3 + look - 1, 0);
            }
        }
        wd(0x0008, w + 6);
        wd(0x003C, i + 1);
        ww(0x0038, (uint16_t)(count - 1));
        count--;
    }
}

/* T001 CODE:9B25, once per frame of the shooting game. */
static int T1_SHOOT_UPDATE(void)
{
    uint32_t m = rd(N_MODULE_BASE), s = m + 0xA19F, main_state;
    uint16_t cross, hits;

    wd(0x0004, s);
    wd(m + 0xA197, 0x10);
    wd(m + 0xA193, 0xA0);
    wd(m + 0xA19B, 0);
    T1_SHOOT_PIC(m, 0, 0);
    T1_SHOOT_WINDOWS(m, s);

    main_state = rd(0x0014);
    if (rb(main_state + 0x2A7B)) {
        if (rb(s + 6) == 0) {
            wb(s + 7, 0);
            if (rw(s + 4) != 0) {
                ww(s + 4, (uint16_t)(rw(s + 4) - 1));
                wb(s + 6, 0x19);
                wb(s + 6, (uint8_t)(rb(s + 6) - 1));
            }
        } else {
            wb(s + 6, (uint8_t)(rb(s + 6) - 1));
        }
    } else if (rb(main_state + 0x2A7C)) {
        if (rb(s + 7) == 0) {
            wb(s + 6, 0);
            if (rw(s + 4) != 3) {
                ww(s + 4, (uint16_t)(rw(s + 4) + 1));
                wb(s + 7, 0x19);
                wb(s + 7, (uint8_t)(rb(s + 7) - 1));
            }
        } else {
            wb(s + 7, (uint8_t)(rb(s + 7) - 1));
        }
    } else {
        wb(s + 6, 0);
        wb(s + 7, 0);
    }

    cross = rw(s + 4);
    wd(m + 0xA197, 0x0F);
    wd(m + 0xA193, 0x10);
    wd(m + 0xA19B, rw(m + 0xA1EB + (uint32_t)cross * 2) >> 1);
    T1_SHOOT_PIC(m, 0x0D, 1);

    if ((int16_t)rw(s + 2) <= 0) {
        hits = rw(s);
        wb(m + 0x6061, '0');
        wb(m + 0x6062, '0');
        wd(0x0000, m + 0x6063);
        ww(0x0020, hits);
        host_call(0x18);
        /* DEC_TEXT clobbers the module-state pointer in CODE:[0004], so
         * the original misses the flag set at 25 hits. The optional fix
         * checks the saved state directly; with this fix off, the original
         * check is retained. */
        if ((pi_fix_shooting_extra_ball && rb(s + 8) == 0xFF) ||
            (!pi_fix_shooting_extra_ball && rb(rd(0x0004) + 8) == 0xFF))
            wd(0x0000, m + 0x5F96);
        else
            wd(0x0000, m + 0x5FA2);
        host_call(0x1C);
        wd(0x0020, 0xFFFFFFFF);
        return 1;
    }
    if (rw(s) >= 0x1E) {
        T1_SHOOT_BCD_ADD(s + 0x12, m + 0xA43F);
        T1_SHOOT_BCD_ADD(rd(main_state + 0x0D76) + 8, s + 0x12);
        wd(0x0000, m + 0x5FAE);
        host_call(0x1C);
        wd(0x0020, 0xFFFFFFFF);
        return 1;
    }
    wd(0x0020, 0);
    return 0;
}

/* table 2: TUNE_CHOICES (a word per player, the tune 0..2), TUNE_TEMPLATES
 * (three pointers, three audio records each) and the records 0..2 the
 * music is played from (CODE:1A760), offsets in the module */
#define T2_TUNE_CHOICES   0x9D88
#define T2_TUNE_TEMPLATES 0x9D9E
#define T2_TUNE_RECORDS   0x1A760
#define T2_TUNE_NAMES     0x9DAA
#define T2_TUNE_PICNAMES  0x9DFA
#define T2_TUNE_STREAM    0x4CBE

/* the 18 words at [0000] to [0008], [0020]'s low word counting down */
static void tune_words(void)
{
    uint16_t n;

    do {
        uint32_t q = rd(0x0000), e = rd(0x0008);

        ww(e, rw(q));
        wd(0x0000, q + 2);
        wd(0x0008, e + 2);
        n = rw(0x0020);
        ww(0x0020, (uint16_t)(n - 1));
    } while (n != 0);
}

/* table 2 CODE:9A55 (TUNE_COPY): the current player's template (three
 * audio records) over the records 0..2 */
static void T2_TUNE_COPY(void)
{
    uint32_t m = rd(N_MODULE_BASE), tc = m + T2_TUNE_CHOICES;
    uint16_t p, cx;

    wd(0x0004, tc);
    p = rw(rd(0x0014) + 0x0D72);
    ww(0x0038, p);
    cx = rw(tc + sx16(p) * 2);
    ww(0x0020, cx);
    wd(0x0000, rd(m + T2_TUNE_TEMPLATES + sx16(cx) * 4));
    wd(0x0008, m + T2_TUNE_RECORDS);
    ww(0x0020, 0x11);
    tune_words();
}

/* table 2 CODE:9ADA (TUNES_RESET, all of slot 40 MOD_GAME_START at
 * CODE:00B5): all eight players' choices 0, template 0 over the records
 * 0..2 */
static void T2_TUNES_RESET(void)
{
    uint32_t m = rd(N_MODULE_BASE), tc = m + T2_TUNE_CHOICES;
    uint16_t si;

    wd(0x0004, tc);
    ww(0x0038, 7);
    do {
        si = rw(0x0038);
        ww(tc + sx16(si) * 2, 0);
        ww(0x0038, (uint16_t)(si - 1));
    } while (si != 0);
    wd(0x0000, rd(m + T2_TUNE_TEMPLATES));
    wd(0x0008, m + T2_TUNE_RECORDS);
    ww(0x0020, 0x11);
    tune_words();
}

/* table 2 CODE:9A10 (TUNE_START, the opcode-14h object's +0): the
 * player's choice 0, the timers +10h, +11h (the flippers' repeat), +12h
 * (the picture) and +14h (its frames, 6) set, state+0F1h (the last key)
 * cleared */
static void T2_TUNE_START(void)
{
    uint32_t tc = rd(N_MODULE_BASE) + T2_TUNE_CHOICES, st = rd(0x0014);
    uint16_t p;

    wd(0x0004, tc);
    p = rw(st + 0x0D72);
    ww(0x0038, p);
    ww(tc + sx16(p) * 2, 0);
    wb(tc + 0x10, 0);
    wb(tc + 0x11, 0);
    ww(tc + 0x12, 0);
    ww(tc + 0x14, 6);
    wb(st + 0x00F1, 0);
}

/* table 2 CODE:9D4E (TUNE_PIC): picture ECX+1 of the VideoMode data's
 * table (FS, by host +28h, which keeps the registers) to DM_ANIM's block,
 * 160 x 16, 0FCh added to each byte */
static void T2_TUNE_PIC(uint32_t ecx)
{
    uint32_t fs = pmax_base(rw(N_VM_DATA_SEL)), es = pmax_base(rw(N_DM_ANIM));
    uint32_t ebx = lrd(fs + (ecx + 1) * 4), i;

    for (i = 0; i < 160 * 16; i++)
        lwb(es + i, (uint8_t)(lrb(fs + ebx + i) + 0xFC));
}

/* table 2 CODE:9B68 (TUNE_UPDATE, the object's +4, each frame): the
 * display cleared (host +8), the turning picture (+12h, the next every 5
 * frames by +14h); the left flipper (state+2A7Bh) one tune down, the
 * right (2A7Ch) one up, again every 25 frames while held; the tune's
 * name (host +14h); Enter (state+0F1h 1Ch): its template over records
 * 0..2, event stream 4CBEh queued (host +1Ch) and 1 (ZF clear, done);
 * else 0 */
static int T2_TUNE_UPDATE(void)
{
    uint32_t m = rd(N_MODULE_BASE), tc = m + T2_TUNE_CHOICES, st;
    uint32_t keep20, keep38;
    uint16_t c, p;

    host_call(0x08);
    wd(0x0004, tc);
    wd(0x0008, m + T2_TUNE_PICNAMES);
    ww(0x0038, rw(rd(0x0014) + 0x0D72));
    T2_TUNE_PIC((rd(0x0020) & 0xFFFF0000u) | rw(tc + 0x12));
    tc = rd(0x0004);
    ww(tc + 0x14, (uint16_t)(rw(tc + 0x14) - 1));
    if (rw(tc + 0x14) & 0x8000) {
        ww(tc + 0x14, 4);
        ww(tc + 0x12, (uint16_t)(rw(tc + 0x12) + 1));
        if (rw(tc + 0x12) == 3)
            ww(tc + 0x12, 0);
    }
    p = rw(0x0038);                                     /* CODE:9BD1 */
    tc = rd(0x0004);
    c = rw(tc + sx16(p) * 2);
    ww(0x0020, c);
    st = rd(0x0014);
    if (rb(st + 0x2A7B) != 0 && (rb(tc + 0x10) != 0 || c != 0)) {
        if (rb(tc + 0x10) == 0) {
            c--;
            ww(0x0020, c);
            wb(tc + 0x10, 0x19);
        }
        wb(tc + 0x10, (uint8_t)(rb(tc + 0x10) - 1));
    } else
        wb(tc + 0x10, 0);
    c = rw(0x0020);                                     /* CODE:9C2B */
    if (rb(st + 0x2A7C) != 0 && (rb(tc + 0x11) != 0 || c < 2)) {
        if (rb(tc + 0x11) == 0) {
            c++;
            ww(0x0020, c);
            wb(tc + 0x11, 0x19);
        }
        wb(tc + 0x11, (uint8_t)(rb(tc + 0x11) - 1));
    } else
        wb(tc + 0x11, 0);
    keep38 = rd(0x0038);                                /* CODE:9C70 */
    keep20 = rd(0x0020);
    c = (uint16_t)keep20;
    ww(tc + sx16((uint16_t)keep38) * 2, c);
    wd(0x0000, rd(m + T2_TUNE_NAMES + sx16(c) * 4));
    host_call(0x14);
    wd(0x0020, keep20);
    wd(0x0038, keep38);
    st = rd(0x0014);
    if (rb(st + 0x00F1) != 0x1C) {
        wd(0x0020, 0);
        return 0;
    }
    wb(st + 0x00F1, 0);
    wd(0x0000, rd(m + T2_TUNE_TEMPLATES + sx16(c) * 4));
    wd(0x0008, m + T2_TUNE_RECORDS);
    ww(0x0020, 0x11);
    tune_words();
    wd(0x0000, m + T2_TUNE_STREAM);
    host_call(0x1C);
    wd(0x0020, 0xFFFFFFFFu);
    return 1;
}

/* table 4 CODE:97E5 (SEA_OBJECT), the sea game on the display
 * (docs/bpc-module.md, "Table 4's sea game"): the boat steered with the
 * flippers past rocks, picking up bonuses. Offsets in the module. */
#define T4_SEA_STATE    0xACB6

/* the PIT's counter 0 as table 4 CODE:A27B reads it (latched, low byte
 * then high). The port has no PIT behind its memory (its NOSOUND counts
 * the timer's IRQs per picture instead); here the same rate (the reload
 * the runs show, 18733, about 63.7 ticks a second) from the picture
 * count, so headless runs repeat exactly. The rocks come out elsewhere
 * than in a run, as with any other timer. */
static uint16_t sea_pit(void)
{
    double q = (double)frame_count() / 70.0 * 1193182.0 / 18733.0;

    q -= (double)(uint64_t)q;
    return (uint16_t)((1.0 - q) * 18733.0);
}

/* table 4 CODE:A2E1 (SEA_STEER): the running move on (the position by
 * the step, the lane when it ends), else the flippers move the boat a
 * lane (left down at once, right up when the position reaches 80h).
 * The lane wraps modulo 8, the position stays below 80h. */
static void T4_SEA_STEER(uint32_t m, uint32_t st)
{
    uint32_t ebx = rd(0x0028), ecx, edx;
    uint8_t bl = rb(m + 0xACCD), cl;
    int8_t ccd;

    ebx = (ebx & 0xFFFFFF00u) | bl;
    wd(0x0028, ebx);
    wb(m + 0xACD3, (uint8_t)(rb(m + 0xACD3) - bl));
    wd(0x0024, 0xFFFFFFFFu);
    ecx = rd(0x002C);
    cl = rb(m + 0xACCC);
    ecx = (ecx & 0xFFFFFF00u) | cl;
    wd(0x002C, ecx);
    edx = 0x7Fu & rb(m + 0xACD3);
    wd(0x0020, edx);
    if (edx != 0) {                                   /* still moving */
        wb(m + 0xACD3, (uint8_t)edx);
        return;
    }
    if (rb(m + 0xACCD) != 0) {                        /* the move's end */
        ccd = (int8_t)rb(m + 0xACCD);
        ebx = rd(0x0024);
        if (ccd < 0)
            ebx = (ebx & 0xFFFFFF00u) | ((ebx - 1) & 0xFF);
        bl = (uint8_t)(ebx + 2);
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x0024, ebx);
        wb(m + 0xACCD, 0);
        wb(m + 0xACD2, (uint8_t)(rb(m + 0xACD2) + bl));
        ww(m + 0xACD2, (uint16_t)(rw(m + 0xACD2) & 7));
        return;
    }
    if (rb(st + 0x2A7B)) {                            /* left */
        ebx = rd(0x0024);
        bl = (uint8_t)(ebx + 1);
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x0024, ebx);
        ecx = rd(0x002C);
        wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | (ecx & 0xFF));
    } else if (rb(st + 0x2A7C)) {                     /* right */
        wd(0x0024, 0xFFFFFFFEu);
        ecx = rd(0x002C);
        cl = (uint8_t)-(int8_t)(ecx & 0xFF);
        ecx = (ecx & 0xFFFFFF00u) | cl;
        wd(0x002C, ecx);
        ebx = rd(0x0024);
        bl = (uint8_t)(ebx + 1);
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x0024, ebx);
        ecx = rd(0x002C);
        wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | (ecx & 0xFF));
    }
    ebx = rd(0x0024);
    bl = (uint8_t)(ebx + 1);
    ebx = (ebx & 0xFFFFFF00u) | bl;
    wd(0x0024, ebx);
    ecx = rd(0x0020);
    wb(m + 0xACCD, (uint8_t)ecx);
    wb(m + 0xACD3, (uint8_t)(rb(m + 0xACD3) - (ecx & 0xFF)));
    wb(m + 0xACD2, (uint8_t)(rb(m + 0xACD2) - bl));
    ww(m + 0xACD2, (uint16_t)(rw(m + 0xACD2) & 0x7F07));
}

/* a six-byte packed-BCD addition for the sea game's bonuses: the
 * number ending 4 past [0008h] added to the one ending 4 past [0010h]
 * (as the original's DAA loop does it); [0008h] and [0010h] both less
 * 6 after, the flags kept out (PUSHFD). */
static void T4_SEA_BCD6(void)
{
    uint32_t src = rd(0x0008) - 4, dst = rd(0x0010) - 4;
    int i, cf = 0;

    for (i = 0; i < 6; i++)
        adc_daa(dst + (uint32_t)i, src + (uint32_t)i, &cf);
    wd(0x0008, rd(0x0008) - 6);
    wd(0x0010, rd(0x0010) - 6);
}

/* table 4 CODE:9EC8 (SEA_CRASH, the boat met a rock): the crash script
 * and its audio record; the row loop goes on. */
static void T4_SEA_CRASH(uint32_t m)
{
    uint32_t keep10 = rd(0x0010);

    wb(m + 0xACE5, 0x36);
    wd(m + 0xACE6, m + 0x9AEC);
    wd(0x0000, m + 0x19557);
    wd(0x0010, rd(m + 0x9874));
    host_call(0x04);
    wd(0x0010, keep10);
    /* [0000h] left at the object, as the original's POP leaves it */
    wd(0x0000, rd(0x000C));
}

/* table 4 CODE:9F1D (SEA_EXTRA_BALL, kind 6): its stream queued and the
 * end flag set; the row loop goes on. */
static void T4_SEA_EXTRA_BALL(uint32_t m)
{
    uint32_t keep0 = rd(0x0000), keep10 = rd(0x0010);

    wd(0x0000, m + 0x90AE);
    wd(0x0010, rd(m + 0x9874));
    host_call(0x1C);
    wd(0x0010, keep10);
    wd(0x0000, keep0);
    wb(m + 0xACE4, 0xFF);
}

/* table 4 CODE:9F68/0xA053 (SEA_BONUS_5M/10M, kinds 2/4): the score and
 * the course a step on, its audio record; the row loop goes on. */
static void T4_SEA_BONUS(uint32_t m, uint32_t score)
{
    uint32_t keep0 = rd(0x0000), keep10 = rd(0x0010), pl;

    wd(0x0008, m + score);
    wb(m + 0xACD4, (uint8_t)(rb(m + 0xACD4) + 4));
    wd(0x0000, m + 0x194A1);
    wd(0x0010, rd(m + 0x9874));
    host_call(0x04);
    wd(0x0010, keep10);
    wd(0x0000, keep0);
    pl = rd(rd(0x0014) + 0x0D76) + 8;
    wd(0x0010, pl);
    T4_SEA_BCD6();
}

/* table 4 CODE:9C91 (SEA_ROW): every column a row down (rows 1..7 only
 * on even row counts, from 8 on every row); new objects from the
 * random number (a bonus of the course step's kind at row 1 when none
 * is on the water and its bits 12..13 are not both 0, else a rock at
 * row 1, but nothing while immunity runs and nothing past a column
 * above row 2); the boat met on leaving row 15 in its column. */
static void T4_SEA_ROW(uint32_t m)
{
    uint32_t p = m + 0xACBA, q = m + 0xACB9, ebx, ecx, edx, edi, ebp;
    uint8_t bl, cl;
    int8_t kind;

    wd(0x0000, p);
    wd(0x0004, q);
    wb(q, rb(p + 7));
    wd(0x0020, 0);
    wd(0x0038, 0);
    ebx = rd(0x0030);
    bl = (uint8_t)(rb(m + 0xACD2) + 1);
    ebx = (ebx & 0xFFFFFF00u) | bl;
    wd(0x0030, ebx);
    ecx = rd(0x002C);
    cl = rb(m + 0xACD3);
    ecx = (ecx & 0xFFFFFF00u) | cl;
    wd(0x002C, ecx);
    ebx = rd(0x002C);
    if ((ebx & 0xFF) >= 0x40) {
        ebx = rd(0x0030);
        ebx = (ebx & 0xFFFFFF00u) | ((ebx + 1) & 0xFF);
        wd(0x0030, ebx);
    }
    ecx = 7 - (rd(0x0030) & 7);
    wd(0x002C, ecx);
    edi = rd(0x0030);
    edi = (edi & 0xFFFF0000u) | rw(m + 0xACCE);
    wd(0x0030, edi);
    wd(0x003C, 7);
    for (;;) {
        p = rd(0x0000);
        ebx = rd(0x0020);
        bl = rb(p);
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x0020, ebx);
        if (bl != 0) {
            ecx = bl & 0xF;
            edi = (0xF - ecx) >> 3;
            wd(0x0034, edi);
            ecx = 0;
            ebp = edi & 0x1F;
            ecx = (uint32_t)1 << ebp;
            ecx = (~(0u - (ecx & 0xFFFF))) & 0xFFFF;
            ecx &= rw(m + 0xACCE);
            wd(0x0024, ecx);
            if (ecx == 0) {
                bl = (uint8_t)(bl + 1);
                ebx = (ebx & 0xFFFFFF00u) | bl;
                wd(0x0020, ebx);
                ecx = bl & 0xF;
                wd(0x0024, ecx);
                if (ecx != 0) {
                    p = rd(0x0000);
                    wb(p, bl);
                    wd(0x0000, p + 1);
                } else {
                    ebx = rd(0x002C);
                    ecx = rd(0x003C);
                    if ((ecx & 0xFF) == (ebx & 0xFF)) {
                        p = rd(0x0000);
                        wb(p, 0);
                        wd(0x0000, p + 1);
                        kind = (int8_t)((bl >> 4) & 0xF);
                        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) |
                           (kind == 1 ? 8 : kind == 3 ? 0xA8 :
                            kind == 5 ? 0x193 : kind == 7 ? 0x5D : 0));
                        if (kind == 1)
                            T4_SEA_CRASH(m);
                        else if (kind == 3)
                            T4_SEA_BONUS(m, 0xAD42);
                        else if (kind == 5)
                            T4_SEA_BONUS(m, 0xAD4A);
                        else if (kind == 7)
                            T4_SEA_EXTRA_BALL(m);
                        else
                            pi_stop("SEA_ROW: a meeting of an odd kind");
                    } else {
                        p = rd(0x0000);
                        wb(p, 0);
                        wd(0x0000, p + 1);
                    }
                }
            }
        } else {
            uint32_t t;
            edi = rd(0x0024);
            edi = (edi & 0xFFFF0000u) | rw(m + 0xACD6);
            t = (edi & 0xFFFF) + 0x5173;
            edi = (edi & 0xFFFF0000u) |
                  ((t + (rd(0x0038) & 0xFFFF) + (t > 0xFFFF ? 1 : 0)) & 0xFFFF);
            ww(m + 0xACD6, (uint16_t)edi);
            ebx = edi;
            bl = (uint8_t)(ebx & 7);
            ebx = (ebx & 0xFFFFFF00u) | bl;
            wd(0x0024, ebx);
            if (bl != 0) {
                p = rd(0x0000);
                ebx = rd(0x0020);
                wb(p, (uint8_t)ebx);
                wd(0x0000, p + 1);
            } else {
                edi = 0xF;
                q = rd(0x0004);
                edx = 0xFu & rb(q);
                wd(0x0028, edx);
                if ((edx & 0xFF) > 2) {
                    p = rd(0x0000);
                    ebx = rd(0x0020);
                    wb(p, (uint8_t)ebx);
                    wd(0x0000, p + 1);
                } else if (rb(m + 0xACCB) != 0) {
                    p = rd(0x0000);
                    ebx = rd(0x0020);
                    wb(p, (uint8_t)ebx);
                    wd(0x0000, p + 1);
                } else {
                    ebx = (ebx & 0xFFFF0000u) | (ebx & 0x3000);
                    wd(0x0024, ebx);
                    if ((ebx & 0xFFFF) == 0) {
                        p = rd(0x0000);
                        ebx = rd(0x0020);
                        wb(p, (uint8_t)ebx);
                        wd(0x0000, p + 1);
                    } else {
                        wb(m + 0xACB8, 0xFF);
                        ebx = 0x21 + rb(m + 0xACD5);
                        wd(0x0020, ebx);
                        p = rd(0x0000);
                        ebx = rd(0x0020);
                        wb(p, (uint8_t)ebx);
                        wd(0x0000, p + 1);
                    }
                }
            }
        }
        q = rd(0x0004) + 1;
        wd(0x0004, q);
        if (rw(0x003C) == 0)
            break;
        ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    }
    wb(m + 0xACB9, rb(q));
}

/* table 4 CODE:A067 (SEA_BONUS_CHECK): 0 while no bonus is on the
 * water; with one, 0 in its column, else 6 or 18 by its side and 6
 * more by the distance round. */
static void T4_SEA_BONUS_CHECK(uint32_t m)
{
    uint32_t p = m + 0xACB9, ebx, ecx, edx, ebp;
    uint8_t bl, cl;

    wd(0x0000, p);
    wd(0x003C, 7);
    for (;;) {
        p = rd(0x0000);
        ebx = 0xFFFFFFF0u;
        bl = (uint8_t)(0xF0 & rb(p));
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x0020, ebx);
        wd(0x0000, p + 1);
        if (bl != 0)
            break;
        if (rw(0x003C) == 0) {
            wb(m + 0xACB8, 0);
            wb(m + T4_SEA_STATE, 0);
            return;
        }
        ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    }
    wb(m + 0xACB8, 0xFF);
    wd(0x002C, 0);
    ebp = 7;
    ebx = rd(0x0020);
    bl = (uint8_t)((rb(m + 0xACD2) + 2) & 7);
    ebx = (ebx & 0xFFFFFF00u) | bl;
    wd(0x0020, ebx);
    ecx = ebp;
    cl = (uint8_t)(ecx - bl);
    ecx = (ecx & 0xFFFFFF00u) | cl;
    wd(0x0024, ecx);
    edx = rd(0x003C);
    ecx = rd(0x0024);
    if ((edx & 0xFF) != (ecx & 0xFF)) {
        if ((int8_t)(edx & 0xFF) >= (int8_t)(ecx & 0xFF)) {
            /* XCHG DX,CX */
            uint32_t t = (edx & 0xFFFF0000u) | (ecx & 0xFFFF);
            ecx = (ecx & 0xFFFF0000u) | (edx & 0xFFFF);
            edx = t;
            wd(0x003C, edx);
            wd(0x0024, ecx);
            ecx = rd(0x002C) + 0xC;
            wd(0x002C, ecx);
        }
        ecx = rd(0x002C) + 6;
        wd(0x002C, ecx);
        ebx = rd(0x003C);
        ecx = rd(0x0028);
        cl = (uint8_t)(ebx + 8);
        ecx = (ecx & 0xFFFFFF00u) | cl;
        edx = rd(0x0024);
        cl = (uint8_t)(ecx - edx);
        ecx = (ecx & 0xFFFFFF00u) | cl;
        wd(0x0028, ecx);
        bl = (uint8_t)(ebx - edx);
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x003C, ebx);
        if (bl & 0x80) {
            bl = (uint8_t)-bl;
            ebx = (ebx & 0xFFFFFF00u) | bl;
            wd(0x003C, ebx);
        }
    }
    ebx = rd(0x0028);
    ecx = rd(0x003C);
    if ((int8_t)((ecx - ebx) & 0xFF) < 0) {
        ecx = rd(0x002C) + 6;
        wd(0x002C, ecx);
    }
    ww(m + T4_SEA_STATE, (uint16_t)rd(0x002C));
}

/* table 4 CODE:A27B (SEA_RANDOM): ACD6 plus the timer, doubled with
 * carry, XOR the word at ACBA, plus the row count. The shared work
 * cells written as the original leaves them. */
static void T4_SEA_RANDOM(uint32_t m)
{
    uint32_t edi = rd(0x0020), ebp = rd(0x0024), sum;
    uint16_t di;

    ebp = (ebp & 0xFFFF0000u) | rw(m + 0xACBA);
    wd(0x0024, ebp);
    sum = (uint32_t)rw(m + 0xACD6) + sea_pit();
    di = (uint16_t)(sum * 2 + (sum > 0xFFFF ? 1 : 0));
    wd(0x0020, (edi & 0xFFFF0000u) | di);
    di ^= rw(m + 0xACBA);
    wd(0x0020, (edi & 0xFFFF0000u) | di);
    di = (uint16_t)(di + (rd(m + 0xACCE) & 0xFFFF));
    wd(0x0020, (edi & 0xFFFF0000u) | di);
    ww(m + 0xACD6, di);
}

/* table 4 CODE:B1B1 (SEA_CLEAR_BUF): 1400h bytes at the display buffer
 * cleared. The original calls host +28h (which selects DM_ANIM for ES
 * and returns its selector in EAX); the port addresses the block as
 * the table-1/2 picture routines do. */
static void T4_SEA_CLEAR_BUF(void)
{
    uint32_t es = pmax_base(rw(N_DM_ANIM)), i;

    for (i = 0; i < 0x1400; i++)
        lwb(es + i, 0);
}

/* table 4 CODE:A16E (SEA_DRAW_PREP): the columns copied for the
 * drawing (so five columns from any lane read in one run of bytes),
 * the five columns' x by the eased position. */
static void T4_SEA_DRAW_PREP(uint32_t m)
{
    uint32_t p = m + 0xACB9, ebx, ecx, edx, eax, ebp, esi;
    uint8_t pos, eased;

    wd(0x0000, m + 0xACDA);
    wd(0x0004, p);
    wd(0x0008, m + 0xA25B);
    wd(0x0010, m + 0xAF4D);
    wd(0x0038, 0xF);
    ebx = rd(p + 1);
    wd(p + 9, ebx);
    ebx = rd(p + 5);
    wd(p + 0xD, ebx);
    pos = rb(m + 0xACD3);
    eased = rb(m + 0xAF4D + pos);
    wd(0x0020, rd(0x0020) - eased);
    wd(0x0024, rd(0x0024) - eased);
    wd(0x0028, rd(0x0028) - eased);
    wd(0x002C, rd(0x002C) - eased);
    ebp = rd(0x0030) - eased;
    wd(0x0030, ebp);
    edx = rd(0x0020);
    eax = rd(0x0024);
    ecx = rd(0x0028);
    ebx = rd(0x002C);
    ebp = rd(0x0030);
    wd(0x0030, ebp);
    esi = rd(0x0000);
    ww(esi, (uint16_t)edx);
    esi += 2;
    ww(esi, (uint16_t)eax);
    esi += 2;
    ww(esi, (uint16_t)ecx);
    esi += 2;
    ww(esi, (uint16_t)ebx);
    esi += 2;
    wd(0x0000, esi);
    ww(esi, (uint16_t)ebp);
}

/* the display selected for the sea game's direct blits (the
 * original's host +28h call, which the port does not model: it keeps
 * the same bytes in the two blocks). */
static uint32_t T4_SEA_DM(void)
{
    return pmax_base(rw(N_DM_ANIM));
}

static uint32_t T4_SEA_FS(uint32_t m)
{
    (void)m;
    return pmax_base(rw(N_VM_DATA_SEL));
}

/* table 4 CODE:AFD5 (SEA_PIC): a whole 160 x 16 picture (the scripts'
 * steps), 0FCh added to each byte. */
static void T4_SEA_PIC(uint32_t m, uint32_t ecx)
{
    uint32_t fs = T4_SEA_FS(m), es = T4_SEA_DM();
    uint32_t src = fs + lrd(fs + ecx * 4), i, n;

    for (n = 0; n < 16; n++)
        for (i = 0; i < 160; i++)
            lwb(es++, (uint8_t)(lrb(src++) + 0xFC));
}

/* table 4 CODE:B024 (SEA_WATER): the water band, 160 wide, all but the
 * first EDX of 16 lines, 0FCh added to each byte. */
static void T4_SEA_WATER(uint32_t m, uint32_t ecx, uint32_t edx)
{
    uint32_t fs = T4_SEA_FS(m), es = T4_SEA_DM();
    uint32_t src = fs + lrd(fs + ecx * 4), i, n;

    es += rd(m + 0xB1F3);
    for (n = 0; n < 16 - edx; n++)
        for (i = 0; i < 160; i++)
            lwb(es++, (uint8_t)(lrb(src++) + 0xFC));
}

/* table 4 CODE:B076 (SEA_SPRITE): picture ECX+1 from its byte
 * SPRITE_SKIP on, 16 lines of SPRITE_WIDTH to the display at x
 * SPRITE_X, line 0; 0 bytes and x not below 160 left out. */
static void T4_SEA_SPRITE(uint32_t m, uint32_t ecx)
{
    uint32_t fs = T4_SEA_FS(m), es = T4_SEA_DM();
    uint32_t src = fs + lrd(fs + ecx * 4) + rd(m + 0xB1F3);
    uint32_t dst = es + rd(m + 0xB1EF), width = rd(m + 0xB1E7), n;
    uint16_t x;

    if (width == 0)
        return;
    for (n = 0; n < 16; n++) {
        uint32_t left = width;
        x = (uint16_t)rd(m + 0xB1EF);
        while (left-- != 0) {
            uint8_t c = lrb(src++);
            if (c != 0 && x < 0xA0)
                lwb(dst, (uint8_t)(c + 0xFC));
            dst++;
            x++;
        }
        dst -= width;
        dst += 0xA0;
    }
}

/* table 4 CODE:B0FA (SEA_SPRITE2): as SEA_SPRITE, but SPRITE_SKIP added
 * to the display offset instead: the boat (500h, line 8). */
static void T4_SEA_SPRITE2(uint32_t m, uint32_t ecx)
{
    uint32_t fs = T4_SEA_FS(m), es = T4_SEA_DM();
    uint32_t src = fs + lrd(fs + ecx * 4);
    uint32_t dst = es + rd(m + 0xB1EF) + rd(m + 0xB1F3);
    uint32_t width = rd(m + 0xB1E7), n;
    uint16_t x;

    if (width == 0)
        return;
    for (n = 0; n < 16; n++) {
        uint32_t left = width;
        x = (uint16_t)rd(m + 0xB1EF);
        while (left-- != 0) {
            uint8_t c = lrb(src++);
            if (c != 0 && x < 0xA0)
                lwb(dst, (uint8_t)(c + 0xFC));
            dst++;
            x++;
        }
        dst -= width;
        dst += 0xA0;
    }
}

/* table 4 CODE:B17E (SEA_CLEAR): the display cleared to 0FCh, every
 * frame. */
static void T4_SEA_CLEAR(void)
{
    uint32_t es = T4_SEA_DM(), i;

    for (i = 0; i < 0xA00; i++)
        lwb(es + i, 0xFC);
}

/* table 4 CODE:9BC8 (SEA_ENTER_END, Enter's script's end): the rocks
 * cleared, 150 frames of immunity. 0 while the game goes on. */
static int T4_SEA_ENTER_END(uint32_t m)
{
    uint32_t p = m + 0xACBA, ebx, ecx;
    uint8_t bl;
    int n;

    wd(0x0000, p);
    wd(0x0020, 0xFFFFFFF0u);
    wd(0x003C, 7);
    for (n = 0; n < 8; n++) {
        p = rd(0x0000);
        ebx = rd(0x0024);
        bl = (uint8_t)(0xF0 & rb(p));
        ebx = (ebx & 0xFFFFFF00u) | bl;
        wd(0x0024, ebx);
        ecx = rd(0x0020);
        if (bl == 0) {
            wb(p, 0);
            ecx = (ecx & 0xFFFFFF00u) | 0;
            wd(0x0020, ecx);
        }
        wd(0x0000, p + 1);
        if (rw(0x003C) == 0)
            break;
        ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    }
    wb(m + 0xACCB, 0x96);
    wd(0x0020, 0);
    return rb(m + 0xACE4) != 0;
}

/* table 4 CODE:9C44 (SEA_CRASH_END, the crash's end): stream 90BAh
 * queued (the music, the "ITEM COLLECTED"/"FISH" display), the game
 * over. */
static int T4_SEA_CRASH_END(uint32_t m)
{
    uint32_t keep0 = rd(0x0000), keep10 = rd(0x0010);

    wd(0x0000, m + 0x90BA);
    wd(0x0010, rd(m + 0x9874));
    host_call(0x1C);
    wd(0x0010, keep10);
    wd(0x0000, keep0);
    wd(0x0020, 0xFFFFFFFFu);
    return 1;
}

/* table 4 CODE:9A20 (SEA_SCRIPT_STEP): a running script's next picture;
 * at its end the script's routine (its first dword). 1 when it ends
 * the game then, else 0. */
static int T4_SEA_SCRIPT_STEP(uint32_t m)
{
    uint32_t script = rd(0x0020), routine;
    uint16_t count = rb(m + 0xACE5);

    wd(0x0000, script);
    wb(m + 0xACE5, (uint8_t)(count - 1));
    wd(0x0020, (uint32_t)(uint16_t)(count * 2));
    if (count == 0) {
        routine = rd(script);
        wd(0x0000, routine);
        wd(m + 0xACE6, 0);
        if (routine == m + 0x9BC8)
            return T4_SEA_ENTER_END(m);
        if (routine == m + 0x9C44)
            return T4_SEA_CRASH_END(m);
        pi_stop("SEA_SCRIPT_STEP: the script's end");
        return 1;
    }
    wd(0x0000, script + (uint32_t)(uint16_t)(count * 4));
    T4_SEA_PIC(m, rd(script + (uint32_t)(uint16_t)(count * 4)));
    wd(0x0020, 0);
    return 0;
}

/* one of the five columns around the boat (SEA_DRAW's repeated block
 * at EBP+`off`, its x from the word at CACDAh+2*`off`): the column's
 * object by its kind and row, in perspective, into the display. */
static void T4_SEA_COLUMN(uint32_t m, uint32_t ebp, unsigned off)
{
    uint32_t esi, edi = m + 0xAA98, ebx, ecx = m + 0xAA54, edx, eax;
    uint8_t dl;
    uint16_t dx;

    edx = 0;
    dl = rb(ebp + off);

    wb(m + 0x97E4, dl);
    edx = (uint32_t)(uint16_t)(dl * 4);
    wd(0x0024, edx);
    eax = edx;
    esi = rd(edi + eax);
    wd(0x0028, esi);
    if (esi == 0)
        return;
    edx &= 0x3C;
    wd(0x0024, edx);
    eax = edx;
    ebx = ecx + eax;
    ebp = (uint32_t)(int32_t)(int16_t)rw(ebx);
    ebp >>= 1;
    wd(0x002C, ebp);
    edi = (uint32_t)(int32_t)(int16_t)rw(ebx + 2);
    wd(0x0030, edi);
    edi -= 8;
    wd(m + 0xB1E7, edi);
    wd(0x0034, edi);
    ww(0x003C, (uint16_t)edi);
    ecx = rd(0x0024);
    ecx = (ecx & 0xFFFF0000u) | ((ecx >> 4) & 0xFFFF);
    wd(0x0024, ecx);
    eax = 3;
    eax = (eax & 0xFFFF0000u) | ((eax - (ecx & 0xFFFF)) & 0xFFFF);
    wd(0x0020, eax);
    ebp = 0xF;
    ebx = rd(0x0014);
    edx = 0xF;
    edx = (edx & 0xFFFFFF00u) | ((edx - rb(ebx)) & 0xFF);
    ecx = eax;
    ecx = (ecx & 0xFFFFFF00u) | (ecx & 0xFF);
    dx = (uint16_t)(edx & 0xFFFF);
    dx = (uint16_t)(dx >> (ecx & 31));
    edx = dx;
    ebp = rd(0x003C);
    edi = ebp & 0xFFFF;
    edx = (uint32_t)((int32_t)edx * (int32_t)edi);
    wd(m + 0xB1F3, edx);
    ebx = (uint32_t)(int32_t)(int16_t)rw(m + 0xCACDA + 2 * off);
    dx = (uint16_t)(rb(m + 0x97E4) & 0xF);
    eax = ebx;
    edx = (uint32_t)dx + 0x12;
    {
        int32_t prod = (int32_t)(int16_t)eax * (int16_t)edx;
        eax = (uint32_t)(int32_t)(int16_t)(prod / 62);
    }
    eax += 0x30;
    eax += rd(0x002C);
    wd(m + 0xB1EF, eax);
    ecx = rd(0x0028);
    wd(m + 0xB1EB, 0x10);
    T4_SEA_SPRITE(m, ecx);
}

/* table 4 CODE:A3EC (SEA_DRAW): the water band, five columns, the
 * boat, the arrow. */
static void T4_SEA_DRAW(uint32_t m)
{
    uint32_t st = rd(0x0014), saved = rd(m + 0xA3EC), ebp;
    uint32_t esi, edi, ebx, ecx, edx, eax;
    uint8_t h;

    wd(0x0004, m + 0xA3EC);
    esi = rd(0x0014);
    wd(0x0014, st);
    edi = m + 0xAD4A;
    ebp = rw(m + 0xACD0);
    edi += ebp & 0xFFFF;
    wd(0x0014, edi);
    ebx = 6;
    h = rb(edi);
    ebx = (ebx & 0xFFFFFF00u) | ((ebx - h) & 0xFF);
    if ((int8_t)(ebx & 0xFF) > 0) {
        ebx = 0u - ebx;
        ebx += 0x10;
        wd(0x0028, ebx);
        ecx = (rd(m + 0xACCE) & 3) + 0x2C;
        eax = 0xA0;
        eax = (eax * (ebx & 0xFFFF)) & 0xFFFF;
        wd(m + 0xB1F3, eax);
        edx = rd(0x0028);
        T4_SEA_WATER(m, ecx, edx);
    }
    esi = rd(0x000C);
    ebx = rd(0x0020);
    edi = m + 0xAA98;
    wd(0x0000, edi);
    ecx = m + 0xAA54;
    wd(0x0010, ecx);
    ebx = rd(0x0020);
    ebx = (ebx & 0xFFFFFF00u) | rb(m + 0xACD2);
    wd(0x0020, ebx);
    ebx = (ebx & 0xFFFF0000u) | (uint32_t)(uint16_t)(int8_t)(ebx & 0xFF);
    ebp = m + 0xACB9;
    ebp += ebx & 0xFFFF;
    wd(0x0008, ebp);
    T4_SEA_COLUMN(m, ebp, 0);
    T4_SEA_COLUMN(m, ebp, 1);
    T4_SEA_COLUMN(m, ebp, 3);
    T4_SEA_COLUMN(m, ebp, 4);
    T4_SEA_COLUMN(m, ebp, 2);
    wd(0x0024, m + 0x97E5);
    wd(0x0014, st);
    ebp = rd(st + 0x2A0E);
    wd(0x0004, ebp);
    wd(m + 0xB1EF, 0x48);
    wd(m + 0xB1E7, 0x10);
    wd(m + 0xB1EB, 8);
    ecx = 0x11;
    wd(m + 0xB1F3, 0x500);
    T4_SEA_SPRITE2(m, ecx);
    ecx = 3;
    edx = rd(0x000C);
    ecx = (ecx & 0xFFFFFF00u) | ((ecx - 0) & 0xFF);
    ecx = (ecx & 0xFFFFFF00u) | (rb(m + 0xACCA) & 0xFF);
    ecx &= 3;
    wd(0x0020, ecx);
    if (ecx != 0) {
        ecx = rw(m + T4_SEA_STATE);
        wd(0x0020, ecx);
        if (ecx != 0) {
            ebx = 0;
            esi = m + 0xAC98 + ecx;
            edi = rd(esi);
            wd(0x0000, edi);
            esi += 4;
            ebx = rw(esi);
            wd(m + 0xB1EF, ebx);
            wd(m + 0xB1E7, 0x10);
            wd(m + 0xB1EB, 0x10);
            ecx = edi;
            wd(m + 0xB1F3, 0);
            T4_SEA_SPRITE(m, ecx);
            wd(0x0008, ebx);
            esi = edi;
            wd(0x0010, esi);
        }
    }
    esi = rd(0x0004);
    esi -= 4;
    wd(0x0004, esi);
    wd(esi, saved);
}

/* table 4 CODE:9878 (SEA_UPDATE, the object's +4), once a frame: a
 * running script's step, else Enter's script once, else immunity and
 * the frame count down, the scroll on, the random number stirred, the
 * steering, the bonus check, a row with the course step's values when
 * its frames run out, then the drawing. 1 when the end flag is set. */
static int T4_SEA_UPDATE(void)
{
    uint32_t m = rd(N_MODULE_BASE), st = rd(0x0014);
    uint32_t esi, edi, ebx;
    uint8_t cl;

    wd(0x000C, m + 0x97E5);
    esi = rd(0x000C);
    edi = rd(m + 0xACE6);
    wd(0x0020, edi);
    if (edi != 0)
        return T4_SEA_SCRIPT_STEP(m);
    if (rb(st + 0xF1) == 0x1C && rb(m + 0xACD8) == 0) {
        uint32_t keep0 = rd(0x0000), keep10 = rd(0x0010);

        wb(m + 0xACD8, 0xFF);
        wb(m + 0xACE5, 0x18);
        wd(m + 0xACE6, m + 0x9A88);
        wd(0x0000, m + 0x1953D);
        wd(0x0010, rd(m + 0x9874));
        host_call(0x04);
        wd(0x0010, keep10);
        wd(0x0000, keep0);
    } else {
        T4_SEA_CLEAR();
        if (rb(m + 0xACCB) != 0)
            wb(m + 0xACCB, (uint8_t)(rb(m + 0xACCB) - 1));
        wb(m + 0xACCA, (uint8_t)(rb(m + 0xACCA) + 1));
        ebx = rd(0x0020);
        ebx = (ebx & 0xFFFFFF00u) | rb(m + 0xACCC);
        ebx = (ebx & 0xFFFF0000u) | (((ebx & 0xFF) >> 1) & 0xFFFF);
        wd(0x0020, ebx);
        ebx = (rd(m + 0xACD0) + (ebx & 0xFFFF)) & 0xFFFF;
        ww(m + 0xACD0, (uint16_t)ebx);
        ww(m + 0xACD0, (uint16_t)(rw(m + 0xACD0) & 0x1FF));
        T4_SEA_RANDOM(m);
        T4_SEA_STEER(m, st);
        T4_SEA_BONUS_CHECK(m);
        cl = (uint8_t)(rb(m + 0xACD9) - 1);
        wb(m + 0xACD9, cl);
        if (cl == 0) {
            edi = m + 0xACFA;
            ebx = rb(m + 0xACD4);
            edi += ebx & 0xFFFF;
            wd(0x0020, ebx);
            cl = rb(edi);
            wb(m + 0xACD9, cl);
            edi++;
            cl = rb(edi);
            wb(m + 0xACCC, cl);
            edi++;
            wd(0x0000, edi);
            cl = rb(edi);
            wb(m + 0xACD5, cl);
            wd(m + 0xACCE, rd(m + 0xACCE) + 1);
            T4_SEA_ROW(m);
        }
        T4_SEA_DRAW_PREP(m);
        T4_SEA_DRAW(m);
    }
    wd(0x0020, rb(m + 0xACE4) != 0);
    return rb(m + 0xACE4) != 0;
}

/* table 4 CODE:97ED (SEA_START, the object's +0): the host vector kept,
 * the 28 bytes ACBA..ACD5 cleared, ACBA's word to [0000h], no Enter yet,
 * no bonus on the water, no end, a row each frame, the random number
 * stirred, the display buffer cleared. ACD6 (and the last key) kept. */
static void T4_SEA_START(void)
{
    uint32_t m = rd(N_MODULE_BASE), p = m + 0xACBA;
    int n;

    host_call(0x08);
    wd(0x000C, m + 0x97E5);
    wd(m + 0x9874, rd(0x0010));
    wd(0x0000, p);
    for (n = 0; n < 7; n++) {
        wd(p, 0);
        p += 4;
    }
    ww(0x003C, 0xFFFF);
    wb(m + 0xACD8, 0);
    wb(m + 0xACB8, 0);
    wb(m + 0xACE4, 0);
    wb(m + 0xACD9, 1);
    T4_SEA_RANDOM(m);
    wd(0x0020, 0);
    T4_SEA_CLEAR_BUF();
}

/* the module object's +4 at DS:`at` (CODE:2CFC8 calls it): 1 when it
 * returns with ZF clear (done), 0 with ZF set (called again) */
int MOD_UPDATE(uint32_t at)
{
    static char name[64];
    unsigned table = rb(N_TABLE_NUM);
    uint32_t off = at - rd(N_MODULE_BASE);

    if (table == 2 && off == 0x9B68)
        return T2_TUNE_UPDATE();
    if (table == 1 && off == 0x9B25)
        return T1_SHOOT_UPDATE();
    if (table == 4 && off == 0x9878)
        return T4_SEA_UPDATE();
    snprintf(name, sizeof name, "table %u's module at CODE:%X", table, (unsigned)off);
    pi_stop(name);
    return 1;
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
        next_ball_lamps(0x4502, 0x942E, 1);
        return;
    }
    if (table == 2 && off == 0x00B5) {
        T2_TUNES_RESET();
        return;
    }
    if (table == 2 && off == 0x00BB) {
        next_ball_lamps(0x40AA, 0x994A, 2);
        T2_TUNE_COPY();
        return;
    }
    if (table == 2 && off == 0x9A10) {
        T2_TUNE_START();
        return;
    }
    if (table == 1 && off == 0x9A4E) {
        T1_SHOOT_START();
        return;
    }
    if (table == 4 && off == 0x97ED) {
        T4_SEA_START();
        return;
    }
    snprintf(name, sizeof name, "table %u's module at CODE:%X", table, (unsigned)off);
    pi_stop(name);
}
