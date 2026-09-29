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
    default:
        pi_stop("NOSOUND: a command not translated yet");
        cf = 1;
    }
    if (cf)
        wd(D_RESULT, r->eax);
    pm_ds = save;
    return cf;
}
