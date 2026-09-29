/* nosound.c - NOSOUND.SDR, the sound driver for no sound (src/NOSOUND.hints),
 * in C: the port's stand-in for the driver the configuration names.  It
 * works in its own block as the original does (loaded by SOUND_START at
 * DRIVER_SEL), so its memory compares with a run's (memcmp.py
 * src/NOSOUND.hints ... --base of the block).  The commands one by one as
 * the game reaches them.
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

/* IN from the ports the driver saves, as dosrun answered in the run
 * (docs/HANDOFF.md, "The driver's command 0 in a run"); the port has no
 * PIC or speaker to ask */
static uint8_t port_in(uint16_t port)
{
    switch (port) {
    case 0x21: return 0xBA;
    case 0xA1: return 0xFF;
    case 0x61: return 0x30;
    }
    return 0xFF;
}

/* CODE:10BB */
static void PORTS_SAVE(void)
{
    wb(D_SAVED_61, port_in(0x61));
    wb(D_SAVED_IMR1, port_in(0x21));
    wb(D_SAVED_IMR2, port_in(0xA1));
    wb(D_IRQ_MASK1, 0xFF);
    wb(D_IRQ_MASK2, 0xFF);
}

/* the block of selector `sel` cleared, `n` bytes */
static void clear(uint16_t sel, uint32_t n)
{
    uint32_t base = pmax_base(sel), i;

    for (i = 0; i < n; i++)
        lwb(base + i, 0);
}

/* CODE:137A: 1 for CF */
static int CHAN_BUF_ALLOC(void)
{
    uint16_t bx;
    uint32_t k;

    if (HCB_ALLOC(0x800, &bx))
        return 1;
    ww(D_CHAN_BUF_SEL, bx);
    for (k = 0; k < 4; k++)
        ww(D_CHANNELS + k * 0x3B + 0x0A, bx);
    clear(bx, 0x800);
    return 0;
}

/* CODE:1214: 1 for CF */
static int DMA_ALLOC(void)
{
    uint16_t bp, bx;
    uint32_t lin, end;

    if (HCB_ALLOC_LOW(rw(D_DMA_SIZE), &bp))
        return 1;
    lin = HCB_LINEAR(bp);
    /* the buffer (from 20h bytes before it) must not cross a 64 KB line */
    end = (uint16_t)(lin - 0x20) + (uint32_t)rw(D_DMA_SIZE);
    if (end > 0xFFFF) {
        /* freed; a filler up to the line, the buffer again, the filler
         * freed (not reached in the runs) */
        if (bp)
            HCB_FREE(bp);
        if (HCB_ALLOC_LOW(rw(D_DMA_SIZE) - (end & 0xFFFF), &bx))
            return 1;
        ww(D_DMA_FILLER_SEL, bx);
        if (HCB_ALLOC_LOW(rw(D_DMA_SIZE), &bp))
            return 1;
        if (rw(D_DMA_FILLER_SEL)) {
            HCB_FREE(rw(D_DMA_FILLER_SEL));
            ww(D_DMA_FILLER_SEL, 0);
        }
        lin = HCB_LINEAR(bp);
    }
    ww(D_DMA_SEL, bp);
    wd(D_DMA_LINEAR, lin);
    clear(bp, rw(D_DMA_SIZE));
    return 0;
}

/* CODE:053D: 1 for CF */
static int VOLTAB_MAKE(void)
{
    uint16_t sel = pmax_alloc(0x8202);     /* INT 92h AH=0Ah, "Volume table" */
    uint32_t base, i = 0;
    int v, b;

    ww(D_VOLTAB_SEL, sel);
    if (!sel)
        pi_stop("VOLTAB_MAKE: no memory (NOSOUND CODE:1B9E)");
    base = pmax_base(sel);
    for (v = 0; v <= 0x40; v++)
        for (b = 0; b < 0x100; b++, i++)
            lww(base + i * 2, (uint16_t)((int8_t)b * v));
    return 0;
}

/* CODE:11C6: 1 for CF */
static int BUFFERS_ALLOC(void)
{
    uint16_t ax = rw(D_MIX_RATE) / 50;

    ww(D_FRAME_BYTES, ax);
    ww(D_MIX_SIZE, (uint16_t)(ax * rw(D_NBUF)));
    ax = (uint16_t)(ax << 1);
    ww(D_FRAME_BYTES2, ax);
    ww(D_DMA_SIZE, (uint16_t)(ax * rw(D_NBUF)));
    if (DMA_ALLOC())
        return 1;
    return VOLTAB_MAKE();
}

/* CODE:09F0: 1 for CF, the result in r->eax */
static int CMD_INIT(NsRegs *r)
{
    if (rb(D_INITED) == 0xFF) {
        r->eax = 0;
        return 1;
    }
    ww(D_DRV_DS, r->ds);
    ww(D_HOST_CFG + 4, r->es);
    wd(D_HOST_CFG, r->ebx);
    ww(D_HOST_CB_SEL, r->fs);
    wd(D_HOST_CB_OFF, r->edi);
    wd(D_CMD11_PTR, 0xFFFFFFFFu);
    wb(D_INITED, 0xFF);
    PORTS_SAVE();
    if (CHAN_BUF_ALLOC() || BUFFERS_ALLOC()) {  /* the latter: DEV_INIT */
        r->eax = 1;
        return 1;
    }
    return 0;
}

/* CODE:2595: NOTE_IN to NOTE_OUT */
static void NOTE_CONVERT(void)
{
    uint16_t ax = (uint16_t)((rb(D_NOTE_IN) & 0x0F) << 8 | rb(D_NOTE_IN + 1)), bx;
    uint32_t edx = 0;

    if (ax != 0)
        for (;;) {
            bx = rw(D_PERIODS + edx);
            if (bx == 0) {
                ww(D_NOTE_OUT, 0);
                ww(D_NOTE_OUT + 2, 0);
                return;
            }
            edx += 2;
            if (ax == bx)
                break;
        }
    wb(D_NOTE_OUT, (uint8_t)(edx >> 1));
    wb(D_NOTE_OUT + 1, (uint8_t)((rb(D_NOTE_IN) & 0xF0) | rb(D_NOTE_IN + 2) >> 4));
    wb(D_NOTE_OUT + 2, rb(D_NOTE_IN + 2) & 0x0F);
    wb(D_NOTE_OUT + 3, rb(D_NOTE_IN + 3));
}

/* CODE:2614 */
static void PATTERNS_COUNT(void)
{
    uint32_t o = D_ORDERS + rd(D_SLOT_OFF), i;
    uint8_t ah = rb(o);

    for (i = 0; i < 0x80; i++)
        if (rb(o + i) > ah)
            ah = rb(o + i);
    wb(D_NPATTERNS, (uint8_t)(ah + 1));
}

/* a big-endian word of the file's */
static uint16_t be16(uint32_t off)
{
    return (uint16_t)(rb(off) << 8 | rb(off + 1));
}

/* MOD_LOAD's ends at CODE:2169 (unloaded, error 1) and CODE:215F (error 2);
 * CODE:2136's error 3 (a read failed) cannot come, callbacks 7 and 9 clear
 * CF always */
static int load_error(uint8_t err, int unload)
{
    if (unload)
        HCB_UNLOAD();
    wb(D_LOAD_ERROR, err);
    return 1;
}

/* CODE:1C28: 1 for CF */
static int MOD_LOAD(uint16_t es, uint32_t edx)
{
    uint32_t slot = rd(D_SLOT_OFF), base, n, i, k;
    uint16_t ds = rw(D_DRV_DS), sel;
    uint8_t cl;

    if (HCB_LOAD(es, edx))
        return load_error(2, 0);
    ww(D_MOD_HANDLE, (uint16_t)edx);  /* BX, which callback 6 leaves */

    /* the signature at 438h: 31 samples with one, 15 without */
    HCB_SEEK(0x438);
    HCB_READ(ds, D_DMA_FILLER_SEL, 4);
    ww(D_SAMPLE_HDRS_SIZE, 0x3A2);
    ww(D_SIG_SIZE, 4);
    ww(D_NSAMPLES, 0x1F);
    if (rd(D_DMA_FILLER_SEL) != 0x2E4B2E4Du && rd(D_DMA_FILLER_SEL) != 0x34544C46u) {
        ww(D_SAMPLE_HDRS_SIZE, 0x1C2);
        ww(D_SIG_SIZE, 0);
        ww(D_NSAMPLES, 0x0F);
    }
    HCB_SEEK(0);
    HCB_READ(ds, D_SONG_TITLE, 0x14);
    HCB_READ(ds, D_SAMPLE_HDRS, rw(D_SAMPLE_HDRS_SIZE));
    HCB_READ(ds, D_DMA_FILLER_SEL, 2);
    cl = rb(D_DMA_FILLER_SEL + 1) & 0x7F;
    if (cl >= rb(D_DMA_FILLER_SEL))
        cl = 0;
    wb(D_RESTART_POS, cl);
    ww(D_SONG_LENGTH + slot, rb(D_DMA_FILLER_SEL));
    HCB_READ(ds, D_ORDERS + slot, 0x80);
    PATTERNS_COUNT();

    /* the patterns, each note converted */
    HCB_SEEK((uint16_t)(rw(D_SAMPLE_HDRS_SIZE) + 0x96 + rw(D_SIG_SIZE)));
    n = rb(D_NPATTERNS) * 0x400u;
    if (HCB_ALLOC(n, &sel))
        return load_error(1, 1);
    ww(D_PATTERNS_SEL + slot, sel);
    HCB_READ(sel, 0, n);
    base = pmax_base(sel);
    for (i = 0; i < n; i += 4) {
        wd(D_NOTE_IN, lrd(base + i));
        NOTE_CONVERT();
        lwd(base + i, rd(D_NOTE_OUT));
    }
    HCB_SEEK(n + rw(D_SAMPLE_HDRS_SIZE) + rw(D_SIG_SIZE) + 0x96);

    /* the sample records from the headers */
    for (k = 0; k < rw(D_NSAMPLES); k++) {
        uint32_t h = D_SAMPLE_HDRS + k * 0x1E, r = D_SLOTS + slot + k * 0x10;
        uint16_t ax, bx;

        ww(r + 0x0E, (uint16_t)(rb(h + 0x18) * 0x48));
        ww(r + 8, rb(h + 0x19));
        ax = (uint16_t)(be16(h + 0x16) << 1);
        ww(r + 6, ax);
        ww(r + 0x0A, ax);
        if (ax < 3) {
            /* none: the channels' buffer, one byte */
            ww(r, rw(D_CHAN_BUF_SEL));
            ww(r + 6, 1);
            ww(r + 2, 0);
            ww(r + 4, 1);
            ww(r + 0x0A, 1);
            ww(r + 8, 0);
            continue;
        }
        ax = be16(h + 0x1A);
        bx = be16(h + 0x1C);
        if (rw(D_SIG_SIZE) != 0) {
            ax = (uint16_t)(ax << 1);
            bx = (uint16_t)(bx << 1);
        }
        if (bx == 0)
            bx = 1;
        ww(r + 2, ax);
        ww(r + 0x0C, bx);
        bx = (uint16_t)(bx + ax);
        ww(r + 4, bx);
        if (bx > 2)
            ww(h + 6, bx);          /* into the header's name, as the original */
    }

    /* the samples, each followed by 800h bytes of its loop (0 without) */
    for (k = 0; k < rw(D_NSAMPLES); k++) {
        uint32_t r = D_SLOTS + slot + k * 0x10, edi, ebx;
        uint8_t ah;

        if (rw(r + 6) < 3)
            continue;
        if (HCB_ALLOC(rw(r + 0x0A) + 0x800u, &sel))
            return load_error(1, 1);
        ww(r, sel);
        HCB_READ(sel, 0, rw(r + 0x0A));
        base = pmax_base(sel);
        edi = rw(r + 6);
        ebx = rw(r + 2);
        ah = rw(r + 4) >= 3 ? 0xFF : 0;
        for (i = 0; i < 0x800; i++) {
            lwb(base + edi++, lrb(base + ebx++) & ah);
            if ((uint16_t)ebx >= rw(r + 4))
                ebx = rw(r + 2);
        }
    }

    wd(D_ORDER_POS, 0);
    ww(D_PAT_OFFSET, (uint16_t)(rb(D_ORDERS + slot) << 10));
    HCB_UNLOAD();
    wb(D_SPEED, 6);
    return 0;
}

/* CODE:0D2B: 1 for CF, the result in r->eax */
static int CMD_LOAD_MODULE(NsRegs *r)
{
    uint32_t slot;

    if (rb(D_PLAYING) != 0) {
        r->eax = 0;
        return 1;
    }
    slot = (r->ebx & 0xFF) * 0x275;
    wd(D_SLOT_OFF, slot);
    if (rb(D_SLOT_LOADED + slot) == 0xFF) {
        r->eax = 0;
        return 1;
    }
    wb(D_SLOT_LOADED + slot, 0xFF);
    wb(D_C2D64, 0);
    if (MOD_LOAD(r->es, r->edx)) {
        r->eax = rb(D_LOAD_ERROR);
        return 1;
    }
    return 0;
}

/* CODE:0D9F */
static int CMD_MIX(NsRegs *r)
{
    (void)r;                    /* SAMPLE_POS_GET's ECX not used */
    MIX_UPDATE();
    if (rb(D_SONG_END_CB) == 0xFF) {
        wb(D_SONG_END_CB, 0);
        if (rd(D_CMD11_PTR) != 0xFFFFFFFFu)
            pi_stop("NOSOUND: command 11h's pointer called (CMD_MIX)");
    }
    return 0;
}

int ns_call(uint16_t cs, NsRegs *r)
{
    uint32_t save = pm_ds;
    int cf;

    /* DISPATCH: DS as the caller has it for command 0, else DRV_DS */
    pm_ds = pmax_base(cs);
    if (r->eax != 0)
        r->ds = rw(D_DRV_DS);
    pm_ds = pmax_base(r->ds);
    switch (r->eax) {
    case 0:
        cf = CMD_INIT(r);
        break;
    case 4:
        cf = CMD_LOAD_MODULE(r);
        break;
    case 6:
        cf = CMD_MIX(r);
        break;
    default:
        pi_stop("NOSOUND: a command not translated yet");
        cf = 1;
    }
    if (cf)
        wd(D_RESULT, r->eax);
    pm_ds = save;
    return cf;
}
