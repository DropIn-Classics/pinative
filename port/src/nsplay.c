/* nsplay.c - NOSOUND.SDR's module player and mixer (command 6's work), in
 * C.  Called by nosound.c with the driver's DS; the channel records, the
 * tick routines' offsets and the mix buffer are kept as the original keeps
 * them, so the driver's block compares with a run's.
 */
#include "game.h"
#include "gen/nosound.h"
#include "nosound.h"
#include "pmax.h"
#include "pmem.h"

enum {
#define X(seg, name, addr) D_##name = addr,
    NS_NAMES(X)
#undef X
};

/* the tick routines a channel's dword +0 holds (driver offsets) */
enum {
    T_ARPEGGIO = 0x16DA, T_PORTA = 0x178E, T_VIBRATO = 0x1824,
    T_VIB_VOLSLIDE = 0x188A, T_VOLSLIDE = 0x18CF, T_RETRIG = 0x19E9,
    T_CUT = 0x1A26, T_DELAY = 0x1A4B, T_NONE = 0x1B9E
};

/* channel record fields (CHANNELS) */
#define C_TICK 0x00
#define C_FRAC 0x04
#define C_POS 0x06
#define C_SEL 0x0A
#define C_VOL 0x0C
#define C_LOOP_START 0x0E
#define C_LOOP_END 0x10
#define C_END 0x12
#define C_STEP 0x16
#define C_STEP_FRAC 0x18
#define C_NOTE 0x1A
#define C_ARP1 0x1C
#define C_ARP2 0x20
#define C_VOLSLIDE 0x24
#define C_PERIOD 0x26
#define C_PORTA_SPEED 0x28
#define C_PORTA_TARGET 0x2A
#define C_OFFSET 0x2C
#define C_FINETUNE 0x2E
#define C_VIB_POS 0x30
#define C_VIB_SPEED 0x31
#define C_VIB_DEPTH 0x32
#define C_VOLUME 0x33
#define C_VIB_ROW 0x34
#define C_COUNT1 0x35
#define C_COUNT2 0x36
#define C_SFX 0x37
#define C_LOOP_ROW 0x38
#define C_LOOP_COUNT 0x3A
#define CH_SIZE 0x3B

/* the step for a period (the code repeated at CODE:1512 and elsewhere):
 * 361F0Fh / period, then over MIX_RATE as 16.16 */
static void step(uint16_t period, uint16_t *whole, uint16_t *frac)
{
    uint32_t q, rate = rw(D_MIX_RATE);

    if (period == 0 || 0x361F0Fu / period > 0xFFFF || rate == 0)
        pi_stop("NOSOUND: a divide error in a period's step");
    q = (uint16_t)(0x361F0Fu / period);
    *whole = (uint16_t)(q / rate);
    *frac = (uint16_t)(((q % rate) << 16) / rate);
}

static void step_to(uint32_t ch, uint16_t period, uint32_t at_whole, uint32_t at_frac)
{
    uint16_t w, f;

    step(period, &w, &f);
    ww(ch + at_whole, w);
    ww(ch + at_frac, f);
}

static uint16_t period_at(uint16_t index)
{
    return rw(D_PERIOD_TAB + index);
}

/* CODE:156A */
static void NOTE_SAMPLE(uint32_t ch, uint8_t sample)
{
    uint32_t s;
    uint16_t len;

    if (sample == 0)
        return;
    s = D_SLOTS + (uint32_t)(sample - 1) * 0x10 + rd(D_SLOT_OFF);
    ww(ch + C_SEL, rw(s));
    wb(ch + C_VOLUME, rb(s + 8));
    ww(ch + C_VOL, (uint16_t)(rb(s + 8) << 8));
    ww(ch + C_FINETUNE, rw(s + 0x0E));
    ww(ch + C_LOOP_START, rw(s + 2));
    ww(ch + C_OFFSET, 0);
    ww(ch + C_LOOP_END, rw(s + 4));
    len = rw(s + 4);
    if ((uint16_t)(rw(s + 4) - rw(s + 2)) <= 2) {
        ww(ch + C_LOOP_END, 1);
        len = rw(s + 6);
    }
    wd(ch + C_END, len);
}

/* CODE:1512 */
static void NOTE_PERIOD(uint32_t ch, uint8_t note)
{
    uint16_t bx;

    if (note == 0)
        return;
    bx = (uint16_t)((uint8_t)(note - 1) * 2 + rw(ch + C_FINETUNE));
    ww(ch + C_NOTE, bx);
    ww(ch + C_PERIOD, period_at(bx));
    step_to(ch, period_at(bx), C_STEP, C_STEP_FRAC);
    wd(ch + C_POS, rw(ch + C_OFFSET));
    wb(ch + C_VIB_POS, 0);
}

/* CODE:18B5: the volume slide's speed from the parameter */
static void volslide_set(uint32_t ch, uint8_t param)
{
    int8_t al = (int8_t)(param >> 4);

    if (al == 0)
        al = (int8_t)-(param & 0x0F);
    ww(ch + C_VOLSLIDE, (uint16_t)(al * 0x100));
}

/* CODE:18CF */
static void T_volslide(uint32_t ch)
{
    int16_t ax = (int16_t)(rw(ch + C_VOL) + rw(ch + C_VOLSLIDE));

    if (ax < 0)
        ww(ch + C_VOL, 0);
    else if ((uint16_t)ax > 0x4000)
        ww(ch + C_VOL, 0x4000);
    else
        ww(ch + C_VOL, (uint16_t)ax);
}

/* CODE:178E */
static void T_porta(uint32_t ch)
{
    uint16_t target = rw(ch + C_PORTA_TARGET), ax;

    if (target == 0)
        return;
    ax = (uint16_t)(rw(ch + C_PERIOD) + rw(ch + C_PORTA_SPEED));
    if ((int16_t)rw(ch + C_PORTA_SPEED) >= 0) {
        if (ax >= target)
            ax = target;
    } else {
        if (ax <= target)
            ax = target;
    }
    ww(ch + C_PERIOD, ax);
    step_to(ch, ax, C_STEP, C_STEP_FRAC);
    ww(ch + C_ARP1, rw(ch + C_STEP));
    ww(ch + C_ARP2, rw(ch + C_STEP));
    ww(ch + C_ARP1 + 2, rw(ch + C_STEP_FRAC));
    ww(ch + C_ARP2 + 2, rw(ch + C_STEP_FRAC));
}

/* CODE:1824 */
static void T_vibrato(uint32_t ch)
{
    uint8_t bl = (uint8_t)(rb(ch + C_VIB_POS) + rb(ch + C_VIB_SPEED));
    uint16_t ax;

    wb(ch + C_VIB_POS, bl);
    ax = (uint16_t)(rb(D_SINE_TAB + ((bl >> 2) & 0x1F)) * rb(ch + C_VIB_DEPTH));
    ax = (uint16_t)((ax >> 7) & 0x1FF);         /* ROL 1, XCHG, AND AH,1 */
    if (rb(ch + C_VIB_POS) & 0x80)
        ax = (uint16_t)-ax;
    step_to(ch, (uint16_t)(rw(ch + C_PERIOD) + ax), C_STEP, C_STEP_FRAC);
}

/* CODE:16DA: the three steps rotated */
static void T_arpeggio(uint32_t ch)
{
    int k;

    for (k = 0; k < 4; k += 2) {
        uint16_t ax = rw(ch + C_STEP + k), t;

        t = rw(ch + C_ARP1 + k);
        ww(ch + C_ARP1 + k, ax);
        ax = t;
        t = rw(ch + C_ARP2 + k);
        ww(ch + C_ARP2 + k, ax);
        ww(ch + C_STEP + k, t);
    }
}

/* CODE:14FC */
static void TICK_FX(void)
{
    uint32_t n = rw(D_NCHANNELS), ch = D_CHANNELS;

    for (; n; n--, ch += CH_SIZE) {
        switch (rd(ch + C_TICK)) {
        case T_ARPEGGIO: T_arpeggio(ch); break;
        case T_PORTA: T_porta(ch); break;
        case T_VIBRATO: T_vibrato(ch); break;
        case T_VIB_VOLSLIDE: T_vibrato(ch); T_volslide(ch); break;
        case T_VOLSLIDE: T_volslide(ch); break;
        case T_RETRIG:                          /* CODE:19E9 */
            wb(ch + C_COUNT2, (uint8_t)(rb(ch + C_COUNT2) - 1));
            if (rb(ch + C_COUNT2) == 0) {
                wb(ch + C_COUNT2, rb(ch + C_COUNT1));
                wd(ch + C_POS, 0);
            }
            break;
        case T_CUT:                             /* CODE:1A26 */
            wb(ch + C_COUNT2, (uint8_t)(rb(ch + C_COUNT2) - 1));
            if (rb(ch + C_COUNT2) == 0) {
                ww(ch + C_STEP, 0);
                ww(ch + C_STEP_FRAC, 0);
                wd(ch + C_TICK, T_NONE);
            }
            break;
        case T_DELAY:                           /* CODE:1A4B */
            wb(ch + C_COUNT2, (uint8_t)(rb(ch + C_COUNT2) - 1));
            if (rb(ch + C_COUNT2) == 0)
                wd(ch + C_TICK, T_NONE);
            break;
        case T_NONE:
            break;
        default:
            pi_stop("NOSOUND: a channel's tick routine not known");
        }
    }
}

/* CODE:16FB, 1713: portamento up and down */
static void porta_set(uint32_t ch, uint16_t target, int16_t speed)
{
    ww(ch + C_PORTA_TARGET, target);
    ww(ch + C_PORTA_SPEED, (uint16_t)speed);
    wd(ch + C_TICK, T_PORTA);
}

/* the effects at a row (ROW_FX, ROW_FX_SFX and ROW_FX_E, ROW_FX_E_SFX);
 * cl note, ch sample, dl effect, dh parameter */
static void row_fx(uint32_t ch, uint8_t cl, uint8_t csample, uint8_t dl, uint8_t dh)
{
    uint8_t fx = dl & 0x0F, ah = dh;
    int sfx = rb(ch + C_SFX) == 1;

    if (sfx && fx != 0x0B && fx != 0x0D && fx != 0x0E && fx != 0x0F)
        return;
    switch (fx) {
    case 0:                                     /* CODE:1648 */
        if (ah == 0)
            break;
        step_to(ch, period_at((uint16_t)(((ah & 0xF0) >> 3) + rw(ch + C_NOTE))), C_ARP1, C_ARP1 + 2);
        step_to(ch, period_at((uint16_t)(((ah & 0x0F) << 1) + rw(ch + C_NOTE))), C_ARP2, C_ARP2 + 2);
        wd(ch + C_TICK, T_ARPEGGIO);
        break;
    case 1:
        porta_set(ch, 0x71, (int16_t)-ah);
        break;
    case 2:
        porta_set(ch, 0x358, ah);
        break;
    case 3:                                     /* CODE:1728 */
        if (ah)
            ww(ch + C_PORTA_SPEED, ah);
        if (cl) {
            uint16_t bx = (uint16_t)((uint8_t)(cl - 1) * 2 + rw(ch + C_FINETUNE));

            ww(ch + C_NOTE, bx);
            ww(ch + C_PORTA_TARGET, period_at(bx));
            if (csample) {
                uint8_t v = rb(D_SLOTS + 8 + (uint32_t)(csample - 1) * 0x10 + rd(D_SLOT_OFF));

                wb(ch + C_VOLUME, v);
                ww(ch + C_VOL, (uint16_t)(v << 8));
            }
        }
        if ((int16_t)rw(ch + C_PORTA_SPEED) < 0)
            ww(ch + C_PORTA_SPEED, (uint16_t)-rw(ch + C_PORTA_SPEED));
        if (rw(ch + C_PERIOD) >= rw(ch + C_PORTA_TARGET))
            ww(ch + C_PORTA_SPEED, (uint16_t)-rw(ch + C_PORTA_SPEED));
        wd(ch + C_TICK, T_PORTA);
        break;
    case 4:                                     /* CODE:1804 */
        wb(ch + C_VIB_ROW, 0xFF);
        if (ah & 0xF0)
            wb(ch + C_VIB_SPEED, (uint8_t)((ah & 0xF0) >> 2));
        if (ah & 0x0F)
            wb(ch + C_VIB_DEPTH, ah & 0x0F);
        wd(ch + C_TICK, T_VIBRATO);
        break;
    case 6:                                     /* CODE:1882 */
        wd(ch + C_TICK, T_VIB_VOLSLIDE);
        volslide_set(ch, dh);
        break;
    case 9:                                     /* CODE:1891 */
        ww(ch + C_OFFSET, (uint16_t)(ah << 8));
        if (rb(D_ROW_SAMPLE)) {
            uint32_t eax = (uint32_t)ah << 8;

            if (eax > rd(ch + C_END))
                eax = rd(ch + C_END);
            wd(ch + C_POS, eax);
        }
        break;
    case 0x0A:                                  /* CODE:18AF */
        wd(ch + C_TICK, T_VOLSLIDE);
        volslide_set(ch, dh);
        break;
    case 0x0B:                                  /* CODE:18F4 */
        if (rb(D_BREAK_ROW))
            break;
        if (rb(D_C2D64) == 0xFF && rb(D_ORDER_POS) == ah) {
            wb(D_SONG_END, 0xFF);
            wb(D_SONG_END_CB, 0xFF);
            break;
        }
        wb(D_ORDER_POS, ah);
        wd(D_ORDER_POS, rd(D_ORDER_POS) - 1);
        wd(D_NEXT_SLOT, rd(D_SLOT_OFF));
        wb(D_BREAK_ROW, 1);
        break;
    case 0x0C:                                  /* CODE:1940 */
        if (ah > 0x40)
            ah = 0x40;
        wb(ch + C_VOLUME, ah);
        ww(ch + C_VOL, (uint16_t)(ah << 8));
        break;
    case 0x0D:                                  /* CODE:1951 */
        wb(D_BREAK_ROW, (uint8_t)(dh + 1));
        wd(D_NEXT_SLOT, rd(D_SLOT_OFF));
        break;
    case 0x0E:                                  /* CODE:1965 */
        if (sfx && (dh >> 4) != 6 && (dh >> 4) != 0x0E)
            break;
        switch (dh >> 4) {
        case 1:                                 /* CODE:197F */
            porta_set(ch, 0x71, (int16_t)-(ah & 0x0F));
            wd(ch + C_TICK, T_NONE);
            T_porta(ch);
            break;
        case 2:                                 /* CODE:1993 */
            porta_set(ch, 0x358, ah & 0x0F);
            wd(ch + C_TICK, T_NONE);
            T_porta(ch);
            break;
        case 6:                                 /* CODE:19A7 */
            ah &= 0x0F;
            if (ah == 0) {
                ww(ch + C_LOOP_ROW, rw(D_PAT_OFFSET));
                break;
            }
            if ((int8_t)rb(ch + C_LOOP_COUNT) < 0)
                wb(ch + C_LOOP_COUNT, ah);
            wb(ch + C_LOOP_COUNT, (uint8_t)(rb(ch + C_LOOP_COUNT) - 1));
            if ((int8_t)rb(ch + C_LOOP_COUNT) < 0)
                wb(ch + C_LOOP_COUNT, 0xFF);
            else
                ww(D_LOOP_TO, rw(ch + C_LOOP_ROW));
            break;
        case 9:                                 /* CODE:19D9 */
            wb(ch + C_COUNT1, ah & 0x0F);
            wb(ch + C_COUNT2, ah & 0x0F);
            wd(ch + C_TICK, T_RETRIG);
            break;
        case 0x0A:                              /* CODE:19FC */
        case 0x0B:                              /* CODE:1A09 */
            ww(ch + C_VOLSLIDE, (uint16_t)((dh >> 4) == 0x0A ? (ah & 0x0F) << 8 : -((ah & 0x0F) << 8)));
            T_volslide(ch);
            break;
        case 0x0C:                              /* CODE:1A19 */
            wb(ch + C_COUNT2, ah & 0x0F);
            wd(ch + C_TICK, T_CUT);
            break;
        case 0x0D:                              /* CODE:1A3E */
            wb(ch + C_COUNT2, ah & 0x0F);
            wd(ch + C_TICK, T_DELAY);
            break;
        case 0x0E:                              /* CODE:1A57 */
            wb(D_PAT_DELAY, ah & 0x0F);
            break;
        }
        break;
    case 0x0F:                                  /* CODE:1A61 */
        if (dh == 0) {
            wb(D_STOPPED, 1);
            dh = 1;
        }
        wb(D_SPEED, dh);
        wb(D_TICK_COUNT, dh);
        break;
    }
}

/* CODE:15E5 */
static void NOTE_FX(uint32_t ch, uint8_t cl, uint8_t csample, uint8_t dl, uint8_t dh)
{
    if (rb(ch + C_SFX) != 1) {
        uint8_t was = rb(ch + C_VIB_ROW);

        wb(ch + C_VIB_ROW, 0);
        if (was == 0xFF)
            step_to(ch, rw(ch + C_PERIOD), C_STEP, C_STEP_FRAC);
    }
    wd(ch + C_TICK, T_NONE);
    row_fx(ch, cl, csample, dl, dh);
}

/* CODE:14DC */
static void NOTE_PLAY(uint32_t ch, uint8_t cl, uint8_t csample, uint8_t dl, uint8_t dh)
{
    if (rb(ch + C_SFX) != 1 && dl != 3 && dl != 5) {
        NOTE_SAMPLE(ch, csample);
        NOTE_PERIOD(ch, cl);
    }
    NOTE_FX(ch, cl, csample, dl, dh);
}

/* CODE:13DA */
static void ROW_PLAY(void)
{
    uint32_t pat, n, ch = D_CHANNELS, edi;
    uint16_t si;
    uint8_t al;

    if (rb(D_PAT_DELAY)) {
        wb(D_PAT_DELAY, (uint8_t)(rb(D_PAT_DELAY) - 1));
        return;
    }
    si = rw(D_PAT_OFFSET);
    pat = pmax_base(rw(rd(D_SLOT_OFF) + D_PATTERNS_SEL));
    for (n = rw(D_NCHANNELS); n; n--, ch += CH_SIZE, si = (uint16_t)(si + 4)) {
        uint8_t cl = lrb(pat + si), csample = lrb(pat + si + 1);

        wb(D_ROW_SAMPLE, csample);
        NOTE_PLAY(ch, cl, csample, lrb(pat + si + 2), lrb(pat + si + 3));
    }
    if (rw(D_LOOP_TO) != 0xFFFF) {
        ww(D_PAT_OFFSET, rw(D_LOOP_TO));
        ww(D_LOOP_TO, 0xFFFF);
        return;
    }
    if (rb(D_BREAK_ROW) == 0) {
        ww(D_PAT_OFFSET, si);
        if (si % 0x400)
            return;
    }
    /* CODE:1476: the next order */
    edi = rd(D_SLOT_OFF);
    wd(D_ORDER_POS, rd(D_ORDER_POS) + 1);
    while ((uint16_t)rd(D_ORDER_POS) >= rw(edi + D_SONG_LENGTH))
        wd(D_ORDER_POS, rb(D_RESTART_POS));
    wb(D_CUR_PATTERN, rb(edi + (uint16_t)rd(D_ORDER_POS) + D_ORDERS));
    al = (uint8_t)(rb(D_BREAK_ROW) - 1);
    if ((int8_t)al < 0)
        al = 0;
    ww(D_PAT_OFFSET, (uint16_t)(((uint16_t)rb(D_CUR_PATTERN) << 8 | (uint8_t)(al << 2)) * 4));
    wb(D_BREAK_ROW, 0);
}

/* CODE:1057 */
void CHANNELS_RESET(void)
{
    uint32_t n = rw(D_NCHANNELS), ch = D_CHANNELS;

    for (; n; n--, ch += CH_SIZE) {
        if (rb(D_PLAYING) != 0xFF)
            wb(ch + C_SFX, 0);
        if (rb(ch + C_SFX) == 1)
            continue;
        wd(ch + C_POS, 0);
        wd(ch + C_END, 1);
        ww(ch + C_LOOP_START, 0);
        ww(ch + C_LOOP_END, 1);
        ww(ch + C_OFFSET, 0);
        ww(ch + C_FINETUNE, 0);
        ww(ch + C_VOL, 0);
        ww(ch + C_STEP, 0);
        ww(ch + C_STEP_FRAC, 0);
    }
    wb(D_BREAK_ROW, 0);
}

/* CODE:1019 */
static void SONG_RESTORE(void)
{
    wb(D_SONG_END, 0);
    wb(D_C2D64, 0);
    wd(D_SLOT_OFF, rd(D_SAVED_SLOT));
    ww(D_PAT_OFFSET, rw(0x21BB));
    wd(D_ORDER_POS, rd(0x21BD));
    wb(D_SPEED, rb(D_SAVED_SPEED));
    CHANNELS_RESET();
}

/* CODE:00E4 */
static void MIX(void)
{
    uint32_t dma = pmax_base(rw(D_DMA_SEL)), vol = pmax_base(rw(D_VOLTAB_SEL));
    uint32_t n = rw(D_NCHANNELS), k, ch = D_CHANNELS;

    for (k = 0; k < n; k++, ch += CH_SIZE) {
        uint32_t edi = (uint32_t)rw(D_MIX_POS) * 2 + rd(D_MIX_ROUTINES + 4 + k * 8);
        uint32_t fn = rd(D_MIX_ROUTINES + k * 8), cnt = rw(D_MIX_LEN);
        uint32_t esi = rd(ch + C_POS), ebp = (uint32_t)rw(ch + C_STEP_FRAC) << 16;
        uint32_t edx = rw(ch + C_STEP), eax, ebx, smp;

        wd(0x00E0, fn);                         /* the routine CALL takes it from */

        if (rd(ch + C_TICK) == T_DELAY)
            ebp = edx = 0;
        smp = pmax_base(rw(ch + C_SEL));
        eax = rw(ch + C_VOL);
        if (rb(ch + C_SFX) != 1)
            eax = (uint32_t)((eax * rw(D_MASTER_VOL)) >> 16) << 8;
        if ((int32_t)esi > (int32_t)rd(ch + C_END)) {
            ww(ch + C_VOL, 0);
            ebp = edx = 0;
            esi = 0;
        }
        ebx = (uint32_t)rw(ch + C_FRAC) << 16;
        if (cnt == 0)
            pi_stop("NOSOUND: MIX with no samples");
        if (fn != D_MIX_SET && fn != D_MIX_ADD)
            pi_stop("NOSOUND: a mix routine not known");
        /* MIX_SET, MIX_ADD: the unrolled loops, one sample a step */
        for (; cnt; cnt--, edi += 2) {
            uint16_t cx;
            uint32_t sum;

            eax = (eax & 0xFFFFFF00u) | lrb(smp + esi);
            sum = ebx + ebp;
            cx = lrw(vol + eax * 2);
            esi = esi + edx + (sum < ebx);
            ebx = sum;
            if (fn == D_MIX_SET)
                lww(dma + edi, cx);
            else
                lww(dma + edi, (uint16_t)(lrw(dma + edi) + cx));
        }
        ww(ch + C_FRAC, (uint16_t)(ebx >> 16));
        if (esi < rd(ch + C_END)) {
            wd(ch + C_POS, esi);
            continue;
        }
        if (rb(ch + C_SFX) != 0) {
            /* a jingle's end: the music's channel back */
            wb(ch + C_SFX, 0);
            wd(ch + C_POS, rd(0x2D26));
            ww(ch + C_SEL, rw(0x2D2A));
            wd(ch + C_END, rd(0x2D2C));
            ww(ch + C_LOOP_START, rw(0x2D30));
            ww(ch + C_LOOP_END, rw(0x2D32));
            ww(ch + C_STEP, rw(0x2D34));
            ww(ch + C_STEP_FRAC, rw(0x2D36));
            ww(ch + C_VOL, 0);
            continue;
        }
        if (rw(ch + C_LOOP_END) < 2) {
            esi = rd(ch + C_END);
        } else {
            uint16_t len = (uint16_t)(rw(ch + C_LOOP_END) - rw(ch + C_LOOP_START));

            if (len == 0)
                pi_stop("NOSOUND: MIX's loop of length 0");
            esi = rw(ch + C_LOOP_START) + (uint16_t)((uint16_t)(esi - rw(ch + C_LOOP_START)) % len);
        }
        wd(ch + C_POS, esi);
    }
}

/* CODE:1A7B */
uint16_t MIX_ROOM(void)
{
    uint16_t cx = (uint16_t)rd(D_SAMPLE_POS);

    if (cx < rw(D_MIX_POS))
        cx = (uint16_t)(cx + rw(D_MIX_SIZE));
    return (uint16_t)(cx - rw(D_MIX_POS));
}

/* CODE:1A98 */
void MIX_UPDATE(void)
{
    uint16_t fb = rw(D_FRAME_BYTES);

    ww(D_MIX_LEFT, fb);
    if (rb(D_PLAYING) != 0) {
        uint16_t cx = MIX_ROOM();

        if (cx < 1)
            return;
        ww(D_MIX_LEFT, cx > fb ? fb : cx);
    }
    do {
        ww(D_MIX_LEN, rw(D_MIX_LEFT));
        if (rw(D_MIX_POS) % fb) {
            uint16_t dx = (uint16_t)((uint16_t)(rw(D_MIX_POS) + rw(D_MIX_LEN)) % fb);

            if (dx < rw(D_MIX_LEN))
                ww(D_MIX_LEN, (uint16_t)(rw(D_MIX_LEN) - dx));
        } else {
            wd(D_TICKS, rd(D_TICKS) + 1);
            if (rb(D_STOPPED) == 0) {
                wb(D_TICK_COUNT, (uint8_t)(rb(D_TICK_COUNT) - 1));
                if (rb(D_TICK_COUNT) != 0) {
                    TICK_FX();
                } else {
                    wb(D_TICK_COUNT, rb(D_SPEED));
                    if (rb(D_SONG_END) == 0xFF)
                        SONG_RESTORE();
                    ROW_PLAY();
                }
            }
        }
        MIX();
        ww(D_MIX_POS, (uint16_t)(rw(D_MIX_POS) + rw(D_MIX_LEN)));
        if (rw(D_MIX_POS) >= rw(D_MIX_SIZE))
            ww(D_MIX_POS, 0);
        ww(D_MIX_LEFT, (uint16_t)(rw(D_MIX_LEFT) - rw(D_MIX_LEN)));
    } while (rw(D_MIX_LEFT) != 0);
}
