/* events.c - a frame of play's rules (CODE:2B4B9's routines): the event
 * streams' runner (EVENT_RUN, CODE:2D080) with the opcodes translated so
 * far, the mode stream (MODE_RUN), the lit slot-15 records' list
 * (LIT_LIST_STEP), the timers of the slot-16 counters and of the objects,
 * the slot-26 counters and the balls' object hits (OBJECT_HITS).
 * Paths not translated stop the port by name.
 */
#include <stdio.h>
#include "game.h"
#include "names.h"
#include "pmem.h"

static uint32_t sx16(uint16_t v)
{
    return (uint32_t)(int32_t)(int16_t)v;
}

/* the low word of the cell `c` set to `w`, its high word kept */
static void cw(uint32_t c, uint16_t w)
{
    wd(c, (rd(c) & 0xFFFF0000u) | w);
}

/* the current player's bit number (the low three bits of [0038]) */
static unsigned pbit(void)
{
    return rd(0x0038) & 7;
}

static int bit_of(uint32_t a, unsigned b)
{
    return rb(a) >> b & 1;
}

/* CODE:2E82F: the lamp of the slot-15 record at [0008] (+4) off for the
 * player: its bit cleared in byte +0, byte +2 bits 0 and 1, +1 FFh, +3
 * and +4 0; [0004] and [0020] kept */
void LAMP_OFF(void)
{
    uint32_t keep4 = rd(0x0004), keep20 = rd(0x0020), l;

    l = rd(rd(0x0008) + 4);
    wd(0x0020, l);
    if (l != 0) {
        wd(0x0004, l);
        wb(l, (uint8_t)(rb(l) & ~(1u << pbit())));
        wb(l + 2, (uint8_t)(rb(l + 2) & 0xFC));
        wb(l + 1, 0xFF);
        wb(l + 3, 0);
        wb(l + 4, 0);
    }
    wd(0x0020, keep20);
    wd(0x0004, keep4);
}

/* CODE:3007A: the record at [0000] by its type (byte +0 AND 7): 2 a
 * sound record (SFX_PLAY), 4 an audio record (MUSIC_REQUEST), 5
 * CODE:9F88 (not translated), the others nothing; [0020] and [0024]
 * kept */
void RECORD_DISPATCH(void)
{
    uint32_t keep24 = rd(0x0024), keep20 = rd(0x0020);
    unsigned t = rb(rd(0x0000)) & 7;

    wd(0x0020, rw(0x300B9 + t * 2));
    if (t == 2)
        SFX_PLAY();
    else if (t == 4)
        MUSIC_REQUEST();
    else if (t == 5)
        SFX_NOTE();
    wd(0x0020, keep20);
    wd(0x0024, keep24);
}

/* CODE:2ECCE: the light state at [0008] flashed [0020]'s low word times
 * (LIGHT_FLASH_STEP), unless it is already (bit 0 of its byte +2): a
 * 6-byte entry (the count, the light) onto the stack at state+17B8h;
 * [0004] kept */
void LIGHT_QUEUE(void)
{
    uint32_t keep4 = rd(0x0004), l = rd(0x0008), st, p;

    if (!(rb(l + 2) & 1)) {
        wb(l + 2, (uint8_t)(rb(l + 2) | 1));
        st = rd(0x0014);
        p = rd(st + 0x17B8);
        ww(p, (uint16_t)rd(0x0020));
        wd(p + 2, l);
        wd(0x0004, p + 6);
        wd(st + 0x17B8, p + 6);
    }
    wd(0x0004, keep4);
}

/* CODE:2FD11: the 12-digit packed-BCD number [000C] points past added to
 * the player's +8 (as TAKE_PAY's second half); [000C] 6 back after,
 * [0000] and [0010] kept */
void SCORE_ADD(void)
{
    uint32_t keep10 = rd(0x0010), keep0 = rd(0x0000), pl, src, dst;
    int cf = 0, i;

    wb(N_DISPLAY_BUSY, 0xFF);
    pl = rd(rd(0x0014) + 0x0D76);
    wd(0x0010, pl);
    wd(0x0000, pl + 8);
    src = rd(0x000C);
    dst = rd(0x0000);
    for (i = 0; i < 4; i++)
        adc_daa(dst - 4 + (uint32_t)i, src - 4 + (uint32_t)i, &cf);
    for (i = 0; i < 2; i++)
        adc_daa(dst - 8 + (uint32_t)i, src - 8 + (uint32_t)i, &cf);
    wd(0x000C, rd(0x000C) - 6);
    wd(0x0000, rd(0x0000) - 6);
    wd(0x0000, keep0);
    wd(0x0010, keep10);
}

/* the lamp at `l` put on blinking for the player (byte +3 left) */
static void lamp_blink(uint32_t l)
{
    wb(l, (uint8_t)(rb(l) | (uint8_t)rd(0x003C)));
    wb(l + 2, (uint8_t)(rb(l + 2) | 2));
    wb(l + 4, 8);
}

/* [0020] -1 and the sign flag, as the routines that return by JS do */
static int neg(void)
{
    wd(0x0020, 0xFFFFFFFFu);
    return 1;
}

/* CODE:2DD8A: the slot-16 counter at [0000] against its thresholds (12
 * bytes each from +50h: a word, the players' bits +2, a lamp's light
 * state +8; a negative word ends them, FFFEh wraps round), by the
 * player's count +16h; 1 (SF) when none was reached */
static int COUNTER_LEVELS(void)
{
    uint32_t c, a, l;
    uint16_t di;
    unsigned b;

    for (;;) {
        c = rd(0x0000);
        cw(0x0020, rw(c + sx16(rw(0x0038)) * 2 + 0x16));
        wd(0x0004, c + 0x50);
        wd(0x002C, 0);
        if (rb(c) & 2) {
            for (;;) {                                  /* CODE:2DDCC */
                a = rd(0x0004);
                di = rw(a);
                cw(0x0024, di);
                if (di & 0x8000)
                    break;
                b = pbit();
                if (!bit_of(a + 2, b))
                    wd(0x002C, 0xFFFFFFFFu);
                if ((uint16_t)rd(0x0020) < di)
                    return neg();
                if ((uint16_t)rd(0x0020) == di) {       /* CODE:2DE6C */
                    if (bit_of(a + 2, b)) {
                        c = rd(0x0000);
                        ww(c + sx16(rw(0x0038)) * 2 + 0x16,
                           (uint16_t)(rw(c + sx16(rw(0x0038)) * 2 + 0x16) + 1));
                        cw(0x0020, (uint16_t)(rd(0x0020) + 1));
                        continue;
                    }
                    l = rd(a + 8);
                    wd(0x0028, l);
                    if (l != 0) {
                        wd(0x0008, l);
                        wb(l, (uint8_t)(rb(l) | 1u << b));
                        wb(l + 2, (uint8_t)(rb(l + 2) | 2));
                        wb(l + 3, 0);
                        wb(l + 4, 8);
                    }
                    wd(0x0020, 0);
                    return 0;
                }
                l = rd(a + 8);
                wd(0x0028, l);
                if (l != 0) {
                    wd(0x0008, l);
                    wb(l, (uint8_t)(rb(l) & ~(1u << b)));
                    wb(l + 2, (uint8_t)(rb(l + 2) & 0xFD));
                    wb(l + 1, 0xFF);
                    wb(l + 3, 0);
                    wb(l + 4, 0);
                }
                wd(0x0004, a + 0x0C);
            }
        } else {
            for (;;) {                                  /* CODE:2DFDA */
                a = rd(0x0004);
                di = rw(a);
                cw(0x0024, di);
                if (di & 0x8000)
                    break;
                b = pbit();
                if (!bit_of(a + 2, b))
                    wd(0x002C, 0xFFFFFFFFu);
                if ((uint16_t)rd(0x0020) < di)
                    return neg();
                if ((uint16_t)rd(0x0020) > di) {
                    wd(0x0004, rd(0x0004) + 0x0C);
                    continue;
                }
                if (!bit_of(a + 2, b)) {                /* CODE:2E035 */
                    wd(0x0020, 0);
                    return 0;
                }
                c = rd(0x0000);
                ww(c + sx16(rw(0x0038)) * 2 + 0x16,
                   (uint16_t)(rw(c + sx16(rw(0x0038)) * 2 + 0x16) + 1));
                cw(0x0020, (uint16_t)(rd(0x0020) + 1));
                wd(0x0004, rd(0x0004) + 0x0C);
            }
        }
        /* CODE:2DEEF: the list's end */
        if ((uint16_t)rd(0x0024) != 0xFFFE)
            return neg();
        {
            uint16_t bp = rw(rd(0x0004) + 2);
            uint32_t k;

            cw(0x0028, bp);
            c = rd(0x0000);
            k = c + sx16(rw(0x0038)) * 2 + 0x16;
            ww(k, (uint16_t)(rw(k) - bp));
        }
        if ((uint8_t)rd(0x002C) == 0)
            break;
    }
    /* every threshold passed: the counter's stream +4Ch, its step back
     * from +28h/+2Ch, the player's counts to +2, the value 0 and the
     * players' bits cleared */
    c = rd(0x0000);
    l = rd(c + 0x4C);
    wd(0x0020, l);
    if (l != 0) {
        wd(0x0008, c);
        wd(0x0000, l);
        EVENT_QUEUE();
        wd(0x0000, rd(0x0008));
    }
    c = rd(0x0000);                                     /* CODE:2DF5C */
    wd(0x0004, c + 0x50);
    wd(c + 0x30, rd(c + 0x28));
    wd(c + 0x34, rd(c + 0x2C));
    cw(0x0020, rw(c + 2));
    ww(c + sx16(rw(0x0038)) * 2 + 6, rw(c + 2));
    ww(c + sx16(rw(0x0038)) * 2 + 0x16, rw(c + 2));
    wd(c + 0x38, 0);
    wd(c + 0x3C, 0);
    for (;;) {                                          /* CODE:2DFAB */
        a = rd(0x0004);
        if (rw(a) & 0x8000)
            return neg();
        wb(a + 2, (uint8_t)(rb(a + 2) & ~(1u << pbit())));
        wd(0x0004, a + 0x0C);
    }
}

/* the start of take handlers 6 and 15h: the slot-16 counter at the
 * record's +34h counted up for the player (+6, and +16h), its stream +48h
 * when it reaches its top +4 (0: none), then COUNTER_LEVELS (its result,
 * 1 for none reached); -1 when the count was at the top already */
static int take_count(void)
{
    uint32_t c = rd(rd(0x0008) + 0x34), k;
    uint16_t cx, dx, si;

    wd(0x0000, c);
    k = c + sx16(rw(0x0038)) * 2;
    cx = rw(k + 6);
    cw(0x0020, cx);
    dx = rw(c + 4);
    cw(0x0028, dx);
    if (dx != 0 && cx == dx)
        return -1;
    si = (uint16_t)(cx + 1);
    cw(0x0020, si);
    ww(k + 6, si);
    ww(k + 0x16, (uint16_t)(rw(k + 0x16) + 1));
    if (dx != 0 && si >= dx) {
        uint32_t s = rd(c + 0x48);

        wd(0x0020, s);
        if (s != 0) {
            wd(0x0008, c);
            wd(0x0000, s);
            EVENT_QUEUE();
            wd(0x0000, rd(0x0008));
        }
    }
    return COUNTER_LEVELS();
}

/* CODE:2E08C, take handler 15h: the counter counted (take_count) */
static void TAKE_COUNT(void)
{
    take_count();
}

/* CODE:2DCA1, take handler 6: as 15h, and when a threshold was reached
 * its event stream (its +4) to EVENT_QUEUE, the player's bit [003C] set
 * in its +2 first when the counter's flag bit 2 is set */
static void TAKE_COUNT_STREAM(void)
{
    uint32_t a;

    if (take_count() != 0)
        return;
    a = rd(0x0004);
    if (rb(rd(0x0000)) & 4)
        wb(a + 2, (uint8_t)(rb(a + 2) | (uint8_t)rd(0x003C)));
    wd(0x0000, rd(a + 4));                              /* CODE:2DD75 */
    EVENT_QUEUE();
}

/* CODE:2E13D, take handler 0Bh: the counter at the record's +34h, its
 * value (12 BCD digits: the word +38h, the dword +3Ch) raised by its step
 * (+30h, +34h); then the value set to +40h, +44h when +40h is not
 * negative and both of the value's dwords (+38h, +3Ch) are at or above
 * +40h's and +44h's: a cap, presumably */
static void TAKE_RAISE(void)
{
    uint32_t c = rd(rd(0x0008) + 0x34), src, dst, v0, v1, m;
    int cf = 0, i;

    wd(0x0000, c);
    wd(0x0004, c + 0x38);
    wd(0x000C, c + 0x40);
    src = c + 0x38;
    dst = c + 0x40;
    for (i = 0; i < 4; i++)
        adc_daa(dst - 4 + (uint32_t)i, src - 4 + (uint32_t)i, &cf);
    for (i = 0; i < 2; i++)
        adc_daa(dst - 8 + (uint32_t)i, src - 8 + (uint32_t)i, &cf);
    wd(0x0004, rd(0x0004) - 6);
    wd(0x000C, rd(0x000C) - 6);
    v0 = rd(c + 0x38);
    wd(0x0020, v0);
    v1 = rd(c + 0x3C);
    wd(0x0024, v1);
    m = rd(c + 0x40);
    wd(0x0028, m);
    if ((m & 0x80000000u) || v0 < m || v1 < rd(c + 0x44))
        return;
    wd(c + 0x38, rd(c + 0x40));
    wd(c + 0x3C, rd(c + 0x44));
}

/* CODE:2E6CC, take handler 7: the counter's value (the word +38h, the
 * dword +3Ch) to the player's score (SCORE_ADD) */
static void TAKE_SCORE(void)
{
    uint32_t c = rd(rd(0x0008) + 0x34);

    wd(0x0000, c);
    wd(0x000C, c + 0x40);
    SCORE_ADD();
}

/* CODE:2E7C8, take handler 14h: the counter at the record's +34h, its
 * timer +26h = the record's word +38h times FRAME_RATE (state+50h), after
 * which COUNTER_TIMERS puts its step back and its value to 0 */
static void TAKE_TIMER(void)
{
    uint32_t r = rd(0x0008), c = rd(r + 0x34), v;

    wd(0x0000, c);
    v = (uint32_t)rw(r + 0x38) * rw(rd(0x0014) + 0x50);
    wd(0x0020, v);
    ww(c + 0x26, (uint16_t)v);
}

static void take_handler(uint32_t a)
{
    static char why[64];

    switch (a) {
    case 0x2DA7A:                                       /* handler 0, a RET */
        break;
    case 0x2E08C:
        TAKE_COUNT();
        break;
    case 0x2DCA1:
        TAKE_COUNT_STREAM();
        break;
    case 0x2E13D:
        TAKE_RAISE();
        break;
    case 0x2E6CC:
        TAKE_SCORE();
        break;
    case 0x2E7C8:
        TAKE_TIMER();
        break;
    case 0x2DC91:                                       /* handler 10h: 0Bh, 6, 7 */
        TAKE_RAISE();
        TAKE_COUNT_STREAM();
        TAKE_SCORE();
        break;
    default:
        snprintf(why, sizeof why, "RECORD_TAKE: take handler CODE:%X", (unsigned)a);
        pi_stop(why);
    }
}

/* CODE:2D916: the slot-15 record at [0008] taken for the player when lit
 * for him and not blocked: unlit, its points (TAKE_PAY from +2Ch), lit
 * again (flag bits 1, 3) or its lamp off, its display stream +18h, the
 * player's bit in byte +5 of the light state +8, the record +10h
 * (RECORD_DISPATCH), its handler +2Ch */
static void RECORD_TAKE(void)
{
    uint32_t r = rd(0x0008), x;
    unsigned b = pbit();
    uint16_t h;

    if (bit_of(r + 2, b) || !bit_of(r + 1, b))
        return;
    wb(r + 1, (uint8_t)(rb(r + 1) & ~(1u << b)));
    wd(0x000C, r + 0x2C);
    TAKE_PAY();
    r = rd(0x0008);
    wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | (rb(r) & 0x0A));
    if (rb(r) & 0x0A)
        wb(r + 1, (uint8_t)(rb(r + 1) | (uint8_t)rd(0x003C)));
    else
        LAMP_OFF();
    r = rd(0x0008);
    x = rd(r + 0x18);
    wd(0x0020, x);
    if (x != 0) {
        wd(0x0000, x);
        DISPLAY_QUEUE();
    }
    r = rd(0x0008);
    x = rd(r + 8);
    wd(0x0020, x);
    if (x != 0) {
        wd(0x0000, x);
        wb(x + 5, (uint8_t)(rb(x + 5) | (uint8_t)rd(0x003C)));
    }
    r = rd(0x0008);
    x = rd(r + 0x10);
    wd(0x0020, x);
    if (x != 0) {
        wd(0x0000, x);
        RECORD_DISPATCH();
    }
    h = rw(rd(0x0008) + 0x2C);                          /* CODE:2D9DE */
    cw(0x0020, h);
    if (h == 0)
        return;
    take_handler(rd(N_TAKE_HANDLERS + sx16(h) * 4));
}

/* the slot-15 record at [0008] onto the lit list (flag bit 2, once; the
 * last state+2A36h, linked by +30h) */
static void lit_list_add(void)
{
    uint32_t r = rd(0x0008), st, x;

    if (rb(r) & 4)
        return;
    wb(r, (uint8_t)(rb(r) | 4));
    wd(r + 0x30, 0);
    st = rd(0x0014);
    x = rd(st + 0x2A36);
    wd(0x0020, x);
    if (x == 0) {
        wd(st + 0x2A32, r);
        wd(st + 0x2A36, r);
        return;
    }
    wd(0x0000, x);
    wd(x + 0x30, r);
    wd(rd(0x0014) + 0x2A36, r);
}

/* CODE:2D491, event opcode 2 (a slot-15 record, a word): lit for the
 * player for the word's seconds unless blocked: its lamp blinking (not
 * while a mode runs and flag bit 4 is set), its record +0Ch
 * (RECORD_DISPATCH) and display stream +14h, the timer
 * +2Eh, and onto the lit list (flag bit 2) */
static void OP_LIGHT_TIMED(void)
{
    uint32_t r = rd(rd(0x0004) + 2), st = rd(0x0014), x, e;
    unsigned b = pbit();

    wd(0x0008, r);
    if (bit_of(r + 2, b))
        return;
    wb(r + 1, (uint8_t)(rb(r + 1) | 1u << b));
    if (!(rd(st + 0x0D5E) != 0 && (rb(r) & 0x10))) {
        x = rd(r + 4);                                  /* CODE:2D4D7 */
        wd(0x0020, x);
        if (x != 0) {
            wd(0x0000, x);
            wb(x, (uint8_t)(rb(x) | (uint8_t)rd(0x003C)));
            wb(x + 2, (uint8_t)(rb(x + 2) | 2));
            wb(x + 3, 0);
            wb(x + 4, 8);
        }
    }
    r = rd(0x0008);                                     /* CODE:2D50B */
    x = rd(r + 0x0C);
    wd(0x0020, x);
    if (x != 0) {
        wd(0x0000, x);
        RECORD_DISPATCH();
    }
    x = rd(r + 0x14);
    wd(0x0020, x);
    if (x != 0) {
        wd(0x0000, x);
        DISPLAY_QUEUE();
    }
    st = rd(0x0014);                                    /* CODE:2D547 */
    e = (uint32_t)rw(rd(0x0004) + 6) * rw(st + 0x50);
    wd(0x0020, e);
    ww(rd(0x0008) + 0x2E, (uint16_t)e);
    lit_list_add();
}

/* CODE:2D36E, event opcode 1 (a slot-15 record): lit for the player
 * unless lit already or blocked: its lamp blinking (not while a mode
 * runs and flag bit 4 is set), its record +0Ch (RECORD_DISPATCH) and
 * display stream +14h; then no timer (+2Eh FFFFh) and
 * onto the lit list */
static void OP_LIGHT(void)
{
    uint32_t r = rd(rd(0x0004) + 2), x;
    unsigned b = pbit();
    int was;

    wd(0x0008, r);
    was = bit_of(r + 1, b);
    wb(r + 1, (uint8_t)(rb(r + 1) | 1u << b));
    if (!was && !bit_of(r + 2, b)) {
        if (!(rd(rd(0x0014) + 0x0D5E) != 0 && (rb(r) & 0x10))) {
            x = rd(r + 4);                              /* CODE:2D3BC */
            wd(0x0020, x);
            if (x != 0) {
                wd(0x0000, x);
                wb(x, (uint8_t)(rb(x) | (uint8_t)rd(0x003C)));
                wb(x + 2, (uint8_t)(rb(x + 2) | 2));
                wb(x + 3, 0);
                wb(x + 4, 8);
            }
        }
        r = rd(0x0008);                                 /* CODE:2D3F0 */
        x = rd(r + 0x0C);
        wd(0x0020, x);
        if (x != 0) {
            wd(0x0000, x);
            RECORD_DISPATCH();
        }
        x = rd(r + 0x14);
        wd(0x0020, x);
        if (x != 0) {
            wd(0x0000, x);
            DISPLAY_QUEUE();
        }
    }
    ww(rd(0x0008) + 0x2E, 0xFFFF);                      /* CODE:2D42C */
    lit_list_add();
}

/* CODE:2D23E, event opcode 4 (an object, a word): a drop target's (word
 * +0 1) +0Bh cleared and the word to DROP_SET */
static void OP_DROP(void)
{
    uint32_t c = rd(0x0004), o = rd(c + 2);

    wd(0x0000, o);
    cw(0x0020, rw(c + 6));
    if (rw(o) != 1)
        return;
    wb(o + 0x0B, 0);
    DROP_SET();
}

/* CODE:2D274, event opcode 0Dh (a slot-16 counter): reset for the
 * player: the step from +28h, +2Ch, the counts +6, +16h to +2, the value
 * 0, the thresholds' bits cleared and their lamps off */
static void OP_COUNTER_RESET(void)
{
    uint32_t c = rd(rd(0x0004) + 2), a, l;
    unsigned b;

    wd(0x0008, c);
    wd(c + 0x30, rd(c + 0x28));
    wd(c + 0x34, rd(c + 0x2C));
    cw(0x0020, rw(c + 2));
    ww(c + sx16(rw(0x0038)) * 2 + 6, rw(c + 2));
    ww(c + sx16(rw(0x0038)) * 2 + 0x16, rw(c + 2));
    wd(c + 0x38, 0);
    wd(c + 0x3C, 0);
    wd(0x0004, c + 0x50);
    for (;;) {                                          /* CODE:2D2CC */
        a = rd(0x0004);
        if (rw(a) & 0x8000)
            return;
        b = pbit();
        wb(a + 2, (uint8_t)(rb(a + 2) & ~(1u << b)));
        l = rd(a + 8);
        wd(0x0020, l);
        if (l != 0) {
            wd(0x0000, l);
            wb(l, (uint8_t)(rb(l) & ~(1u << b)));
            wb(l + 2, (uint8_t)(rb(l + 2) & 0xFD));
            wb(l + 1, 0xFF);
            wb(l + 3, 0);
            wb(l + 4, 0);
        }
        wd(0x0004, rd(0x0004) + 0x0C);
    }
}

/* CODE:2D326, event opcode 0Eh (a slot-15 record): unless flag bit 1 is
 * set, unlit for the player and its lamp off */
static void OP_UNLIGHT(void)
{
    uint32_t r = rd(rd(0x0004) + 2);

    wd(0x0008, r);
    if (rb(r) & 2)
        return;
    wb(r + 1, (uint8_t)(rb(r + 1) & ~(1u << pbit())));
    LAMP_OFF();
}

/* CODE:2D8C2, event opcode 17h (a slot-15 record, a position): unless
 * the record is lit for the player and not blocked, the stream [0000]
 * goes on at the position */
static void OP_UNLESS_LIT(void)
{
    uint32_t c = rd(0x0004), r = rd(c + 2);
    unsigned b = pbit();

    wd(0x0008, r);
    if (!bit_of(r + 2, b) && bit_of(r + 1, b))
        return;
    ww(rd(0x0000) + 2, rw(c + 6));
}

/* CODE:2D785, event opcode 8 (a hole): bit 1 of its byte +0 cleared,
 * +34h 0, pushed on the stack at state+2A64h for HOLE_EJECT_STEP */
static void OP_HOLE(void)
{
    uint32_t st = rd(0x0014), h = rd(rd(0x0004) + 2), p;

    wd(0x0008, h);
    wb(h, (uint8_t)(rb(h) & ~2));
    wd(h + 0x34, 0);
    p = rd(st + 0x2A64) - 4;
    wd(0x0000, p);
    wd(p, h);
    wd(st + 0x2A64, p);
}

/* CODE:2D7BE, event opcode 18h (a hole, a second hole): as opcode 8 with
 * bit 1 set and the second hole in +34h, where the ball comes out */
static void OP_HOLE2(void)
{
    uint32_t c = rd(0x0004), h = rd(c + 2), st, p;

    wd(0x0000, h);
    wb(h, (uint8_t)(rb(h) | 2));
    wd(h + 0x34, rd(c + 6));
    st = rd(0x0014);
    p = rd(st + 0x2A64) - 4;
    wd(0x0004, p);
    wd(p, h);
    wd(st + 0x2A64, p);
}

/* one command of a stream: its handler by its address (CODE:2D193 plus
 * the table's offset) */
static void event_op(uint32_t a)
{
    static char why[64];

    switch (a) {
    case 0x2D36E:
        OP_LIGHT();
        break;
    case 0x2D491:
        OP_LIGHT_TIMED();
        break;
    case 0x2D23E:
        OP_DROP();
        break;
    case 0x2D274:
        OP_COUNTER_RESET();
        break;
    case 0x2D326:
        OP_UNLIGHT();
        break;
    case 0x2D8C2:
        OP_UNLESS_LIT();
        break;
    case 0x2D785:
        OP_HOLE();
        break;
    case 0x2D7BE:
        OP_HOLE2();
        break;
    case 0x2D358:                   /* opcode 19h: CODE:B927 (a RET) when RES_CODE is 5 */
        break;
    case 0x2D907:                   /* opcode 5: a slot-15 record taken */
        wd(0x0008, rd(rd(0x0004) + 2));
        RECORD_TAKE();
        break;
    case 0x2D5E0:                   /* opcode 13h: an audio record */
        wd(0x0000, rd(rd(0x0004) + 2));
        MUSIC_REQUEST();
        break;
    default:
        snprintf(why, sizeof why, "EVENT_RUN: the opcode at CODE:%X", (unsigned)a);
        pi_stop(why);
    }
}

/* the command at the stream [0000]'s position +2 (from +4): [0004] the
 * command, [0020] the opcode, then its handler and size from
 * EVENT_OPCODES (the size added to the position first); 0 when the
 * opcode is 0 (the stream's end) */
static int stream_step(void)
{
    uint32_t s = rd(0x0000), cmd, t;
    uint16_t op;

    cmd = s + sx16(rw(s + 2)) + 4;
    wd(0x0004, cmd);
    op = rw(cmd);
    cw(0x0020, op);
    if (op == 0)
        return 0;
    t = N_EVENT_OPCODES + sx16(op) * 4;
    wd(0x0020, sx16(rw(t)));
    wd(0x0024, sx16(rw(t + 2)));
    ww(s + 2, (uint16_t)(rw(s + 2) + rw(t + 2)));
    event_op(0x2D193 + sx16(rw(t)));
    return 1;
}

/* CODE:2D080: one command of the running event stream (state+2A22h),
 * else of the next one on the queue (the ring at [state+2A1Eh], index
 * state+2A1Ch); at its end the stream is done */
void EVENT_RUN(void)
{
    uint32_t st = rd(0x0014), s = rd(st + 0x2A22);

    wd(0x0020, s);
    if (s == 0) {
        uint16_t bp = rw(st + 0x2A1C);
        uint32_t q = rd(st + 0x2A1E) + sx16(bp) * 4;

        cw(0x0024, bp);
        s = rd(q);
        wd(0x0020, s);
        if (s == 0)
            return;
        wd(q, 0);
        ww(st + 0x2A1C, (uint16_t)((rw(st + 0x2A1C) + 1) & 0x3F));
        wd(st + 0x2A22, s);
        wd(0x0000, s);
        ww(s + 2, 0);
    }
    s = rd(0x0020);                                     /* CODE:2D0F7 */
    wd(0x0000, s);
    wd(0x0038, sx16(rw(st + 0x0D72)));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    if (!stream_step()) {
        ww(rd(0x0000) + 2, 0);
        wd(rd(0x0014) + 0x2A22, 0);
    }
}

/* CODE:2CD3C: the mode stream (state+0D5Eh), one command a call; the
 * module object's routine (state+2A74h) and opcode 1Ch's wait (state+
 * 0D50h) are not translated */
void MODE_RUN(void)
{
    uint32_t st = rd(0x0014), m;

    wd(0x0038, sx16(rw(st + 0x0D72)));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    if (rb(st + 0x2A74) != 0)
        pi_stop("MODE_RUN: the module object (CODE:2CFC8)");
    if (rb(st + 0x0D2E) != 0) {
        uint16_t cx = (uint16_t)(rw(st + 0x0D3A) + rw(st + 0x0D32));

        cw(0x0020, cx);
        if (cx <= 1) {
            wb(st + 0x0D2E, 0);
            wb(st + 0x0D51, 0xFF);
        }
    }
    st = rd(0x0014);                                    /* CODE:2CD9F */
    if (rb(st + 0x0D50) != 0)
        pi_stop("MODE_RUN: opcode 1Ch's wait (CODE:2CDB1)");
    m = rd(st + 0x0D5E);                                /* CODE:2CED1 */
    wd(0x0020, m);
    if (m != 0) {
        wd(0x0000, m);
        wb(st + 0x0D50, 0);                             /* CODE:2CEF1 */
        wd(st + 0x0D66, 0);
        if (stream_step())
            return;
        ww(rd(0x0000) + 2, 0);                          /* CODE:2CF60 */
        st = rd(0x0014);
        wd(st + 0x0D5E, 0);
        wd(st + 0x2A5C, 0);
        wd(st + 0x2A58, 0);
    }
    st = rd(0x0014);                                    /* CODE:2CF90 */
    wd(st + 0x0D66, 0);
    ww(st + 0x0D62, 0);
    ww(st + 0x0D6A, 0);
    wb(st + 0x0D50, 0);
    wb(st + 0x0D4F, 0);
    wb(st + 0x0D51, 0);
}

/* CODE:2E8CD: the lit slot-15 records' list (state+2A32h, the last
 * state+2A36h, linked by +30h) for the player: one no longer lit for him
 * or whose timer +2Eh ran out leaves it (its lamp off, flag bit 2
 * cleared); a timer counted down, the lamp off in its last second, else
 * blinking (unless blocked, or in a mode with flag bit 4) */
void LIT_LIST_STEP(void)
{
    uint32_t st = rd(0x0014), r, n, prev, l;
    unsigned b;

    if (rd(st + 0x2A32) == 0) {
        wd(0x0020, 0);
        return;
    }
    wd(0x0020, rd(st + 0x2A32));
    cw(0x0034, rw(st + 0x50));
    wd(0x0038, sx16(rw(st + 0x0D72)));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    wd(0x0004, 0);
    for (;;) {                                          /* CODE:2E91B */
        r = rd(0x0020);
        wd(0x0000, r);
        b = pbit();
        if (bit_of(r + 1, b)) {
            uint16_t t = rw(r + 0x2E);

            if (t & 0x8000)
                goto blink;
            if (t != 0) {                               /* CODE:2EA0C */
                ww(r + 0x2E, (uint16_t)(t - 1));
                if ((uint16_t)rd(0x0034) < rw(r + 0x2E))
                    goto blink;
                wd(0x0008, r);
                if (!(rd(st + 0x0D5E) != 0 && (rb(r) & 0x10)))
                    LAMP_OFF();
                goto next;
            }
            wb(r + 1, (uint8_t)(rb(r + 1) & ~(1u << b)));
        }
        /* CODE:2E956: off the list */
        r = rd(0x0000);
        ww(r + 0x2E, 0);
        wd(0x0008, r);
        LAMP_OFF();
        r = rd(0x0000);
        wb(r, (uint8_t)(rb(r) & ~4));
        n = rd(r + 0x30);
        wd(0x0020, n);
        prev = rd(0x0004);
        if (n != 0) {
            if (prev != 0) {
                wd(prev + 0x30, n);
                wd(r + 0x30, 0);
            } else {
                wd(rd(0x0014) + 0x2A32, n);
                wd(rd(0x0000) + 0x30, 0);
            }
            continue;
        }
        if (prev == 0) {                                /* CODE:2E9C4 */
            wd(rd(0x0014) + 0x2A32, 0);
            wd(rd(0x0014) + 0x2A36, 0);
        } else {
            wd(prev + 0x30, 0);
            wd(rd(0x0014) + 0x2A36, prev);
        }
        return;
blink:                                                  /* CODE:2EA47 */
        r = rd(0x0000);
        if (!bit_of(r + 2, pbit()) && !(rd(st + 0x0D5E) != 0 && (rb(r) & 0x10))) {
            l = rd(r + 4);                              /* CODE:2EA7A */
            wd(0x0020, l);
            if (l != 0) {
                wd(0x0008, l);
                lamp_blink(l);
            }
        }
next:                                                   /* CODE:2EAA9 */
        r = rd(0x0000);
        wd(0x0004, r);
        n = rd(r + 0x30);
        wd(0x0020, n);
        if (n == 0)
            return;
    }
}

/* CODE:2EAC9: the slot-26 BCD counters (state+290Eh, 0 ends): one that
 * runs (byte +0) is stepped toward its end (not translated) */
void BCD_COUNTERS_STEP(void)
{
    uint32_t p = rd(rd(0x0014) + 0x290E), e;

    wd(0x0000, p);
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            return;
        wd(0x0004, e);
        if (rb(e) != 0)
            pi_stop("BCD_COUNTERS_STEP: a counter running (CODE:2EAFD)");
    }
}

/* CODE:2CB69: the slot-16 counters' timers (state+28E6h, 0 ends): a word
 * +26h above 0 counted down; at 0 the step (+30h, +34h) back from +28h,
 * +2Ch and the value (+38h, +3Ch) 0, and the rest of the list is left
 * for this frame */
void COUNTER_TIMERS(void)
{
    uint32_t p = rd(rd(0x0014) + 0x28E6), e;

    wd(0x0000, p);
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            return;
        wd(0x0004, e);
        if (rw(e + 0x26) == 0)
            continue;
        ww(e + 0x26, (uint16_t)(rw(e + 0x26) - 1));
        if (rw(e + 0x26) != 0)
            continue;
        wd(e + 0x30, rd(e + 0x28));
        wd(e + 0x34, rd(e + 0x2C));
        wd(e + 0x38, 0);
        wd(e + 0x3C, 0);
        return;
    }
}

/* the byte +k of each record of a list of word offsets from its start
 * (0 ends) counted down to 0 */
static void offsets_down(uint32_t base)
{
    uint32_t p;
    uint16_t di;

    wd(0x0000, base);
    wd(0x0004, base);
    for (;;) {
        p = rd(0x0000);
        di = rw(p);
        cw(0x0020, di);
        wd(0x0000, p + 2);
        if (di == 0)
            return;
        p = rd(0x0004) + sx16(di) + 1;
        if (rb(p) != 0)
            wb(p, (uint8_t)(rb(p) - 1));
    }
}

/* the byte +2 of each object of type 0 of a pointer list (0 ends)
 * counted down to 0 */
static void objects_down(uint32_t list)
{
    uint32_t p, e;

    wd(0x0000, list);
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            return;
        wd(0x0004, e);
        if (rw(e) != 0 || rb(e + 2) == 0)
            continue;
        wb(e + 2, (uint8_t)(rb(e + 2) - 1));
    }
}

/* CODE:2CBCF: the byte +1 of the records of header slots 12 and 13
 * (state+28D6h, 28DAh: word offsets) and the byte +2 of the type-0
 * objects of slots 4 and 10 (state+28B6h, 28CEh) counted down a frame */
void OBJECT_TIMERS(void)
{
    uint32_t st = rd(0x0014);

    offsets_down(rd(st + 0x28D6));
    offsets_down(rd(rd(0x0014) + 0x28DA));
    objects_down(rd(rd(0x0014) + 0x28B6));
    objects_down(rd(rd(0x0014) + 0x28CE));
}

/* CODE:2C8C2: object type 0, [0000] the object, [0020]'s low word its
 * index: below 20h only when its timer (byte +2) has run out, then set to
 * 6 (OBJECT_TIMERS counts it down). With a light state at +4 the player's
 * bit set in it; when it was set already the light flashed 8 times and
 * the points of the record at +0Ch to the score only (SCORE_ADD from
 * +1Ah), else flashed 0Ch times and, as without a light, its points
 * (TAKE_PAY from +12h); the object's record +8 (RECORD_DISPATCH), the
 * record's event stream +1Ah and the object's +10h */
static void OBJECT_TYPE0(void)
{
    uint32_t o = rd(0x0000), r;

    if (rw(0x0020) < 0x20) {
        if (rb(o + 2) != 0)
            return;
        wb(o + 2, 6);
    }
    wd(0x0004, rd(o + 0x0C));
    r = rd(o + 4);
    wd(0x0020, r);
    if (r != 0) {
        uint16_t cx = rw(rd(0x0014) + 0x0D72);
        unsigned b = cx & 7;
        int was = rb(r) >> b & 1;

        wd(0x0008, r);
        wd(0x0020, (r & 0xFFFF0000u) | cx);
        wb(r, (uint8_t)(rb(r) | 1u << b));
        if (was) {
            wd(0x0020, (r & 0xFFFF0000u) | 8);
            LIGHT_QUEUE();
            wd(0x000C, rd(0x0004) + 0x1A);
            SCORE_ADD();
            goto rest;
        }
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | 0x0C);
        LIGHT_QUEUE();
    }
    wd(0x000C, rd(0x0004) + 0x12);
    TAKE_PAY();
rest:                                                   /* CODE:2C973 */
    o = rd(0x0000);
    wd(0x000C, o);
    r = rd(o + 8);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        RECORD_DISPATCH();
    }
    r = rd(rd(0x0004) + 0x1A);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        EVENT_QUEUE();
    }
    r = rd(rd(0x000C) + 0x10);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        EVENT_QUEUE();
    }
}

/* CODE:2C9D4: a drop target hit, [0000] the target, its bank at +22h: the
 * player's bit in the light state +2, the bank's points (TAKE_PAY from
 * +16h), the sound record +6, the target down (+0Bh FFh) and queued for
 * DROP_SET (state+2A60h); when every target of the bank (a chain by +1Eh
 * from the bank's +0) is down, the bank's event stream +16h, and unless
 * the bank's +4 bit 0 is set the bank onto the stack at state+2AD0h to
 * be raised (DROPS_RAISE_STEP) */
static void DROP_HIT(void)
{
    uint32_t o = rd(0x0000), r, st, p;

    wd(0x0004, rd(o + 0x22));
    r = rd(o + 2);
    wd(0x0020, r);
    if (r != 0) {
        uint16_t cx = rw(rd(0x0014) + 0x0D72);

        wd(0x0008, r);
        wd(0x0020, (r & 0xFFFF0000u) | cx);
        wb(r, (uint8_t)(rb(r) | 1u << (cx & 7)));
    }
    wd(0x000C, rd(0x0004) + 0x16);
    TAKE_PAY();
    o = rd(0x0000);
    r = rd(o + 6);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        SFX_PLAY();
        wd(0x0000, o);
    }
    o = rd(0x0000);                                     /* CODE:2CA4D */
    wb(o + 0x0B, 0xFF);
    st = rd(0x0014);
    p = rd(st + 0x2A60) - 4;
    wd(p, o);
    p -= 2;
    wd(0x0008, p);
    ww(p, 1);
    wd(st + 0x2A60, p);
    wd(0x0020, rd(rd(0x0004)));
    for (;;) {                                          /* CODE:2CA8E */
        uint32_t t = rd(0x0020);

        wd(0x0008, t);
        if (rb(t + 0x0B) == 0)
            return;
        wd(0x0020, rd(t + 0x1E));
        if (rd(0x0020) == 0)
            break;
    }
    r = rd(0x0004);
    wd(0x000C, r);
    wd(0x0020, rd(r + 0x16));
    if (rd(0x0020) != 0) {
        wd(0x0000, rd(0x0020));
        EVENT_QUEUE();
    }
    r = rd(0x000C);                                     /* CODE:2CAD4 */
    if (rb(r + 4) & 1)
        return;
    st = rd(0x0014);
    p = rd(st + 0x2AD0) - 4;
    wd(0x0008, p);
    wd(p, r);
    wd(st + 0x2AD0, p);
}

/* CODE:2CB03: object type 2, [0000] the object: when the slot-15 record
 * at +2 is lit for the player (its byte +1), the ball's speed (+0Eh,
 * +10h) set to the object's words +6, +8 and the record taken
 * (RECORD_TAKE) */
static void OBJECT_TYPE2(void)
{
    uint32_t o = rd(0x0000), r = rd(o + 2), st = rd(0x0014), b;

    wd(0x0008, r);
    wd(0x0038, (uint32_t)sx16(rw(st + 0x0D72)));
    wd(0x003C, (uint32_t)sx16(rw(st + 0x0D74)));
    if (!bit_of(r + 1, pbit()))
        return;
    wd(0x0020, (uint32_t)sx16(rw(o + 6)));
    wd(0x0024, (uint32_t)sx16(rw(o + 8)));
    b = rd(0x0010);
    ww(b + 0x0E, rw(o + 6));
    ww(b + 0x10, rw(o + 8));
    RECORD_TAKE();
}

/* CODE:2C7FC: each ball in play whose +6Ch picks an object in the table
 * at its +60h: the object handled by its type (the switch at CODE:2C8BC;
 * [0008] kept across) */
void OBJECT_HITS(void)
{
    uint32_t st = rd(0x0014), b, o, keep8;
    uint16_t n = rw(st + 0x0D32), bp, t;
    static char why[64];

    wd(0x0008, st + 0x1046);
    ww(st + 0x0D34, n);
    if (n == 0)
        return;
    do {
        b = rd(rd(0x0008));
        wd(0x0010, b);
        wd(0x0008, rd(0x0008) + 4);
        if (rb(b + 9) == 0) {
            bp = rw(b + 0x6C);
            cw(0x0020, bp);
            if (!(bp & 0x8000)) {
                wd(0x0000, rd(b + 0x60));
                o = rd(rd(b + 0x60) + sx16(bp) * 4);
                wd(0x0024, o);
                if (o != 0) {
                    wd(0x0000, o);
                    t = rw(o);
                    cw(0x0028, rw(0x2C8BC + (uint32_t)t * 2));  /* the switch's entry */
                    keep8 = rd(0x0008);
                    if (t == 0)
                        OBJECT_TYPE0();
                    else if (t == 1)
                        DROP_HIT();
                    else if (t == 2)
                        OBJECT_TYPE2();
                    else {
                        snprintf(why, sizeof why, "OBJECT_HITS: an object of type %u (CODE:2C8BC)",
                                 (unsigned)t);
                        pi_stop(why);
                    }
                    wd(0x0008, keep8);
                }
            }
        }
        st = rd(0x0014);
        ww(st + 0x0D34, (uint16_t)(rw(st + 0x0D34) - 1));
    } while (rw(st + 0x0D34) != 0);
}
