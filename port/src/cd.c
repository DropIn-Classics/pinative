/* cd.c - the CD through MSCDEX: CD_INSTALLED (CODE:35B52), CD_LOCK
 * (CODE:35B7D), CD_READ_TOC (CODE:35C58), CD_REQUEST (CODE:35B0D), and in
 * place of MSCDEX a drive D: with one data track, as dosrun has it without
 * a cue sheet (the GOG release's audio tracks come with the sound).
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* the disc: tracks first..last, each track's start and the lead-out as
 * Red Book addresses (frame, second, minute, 0 as the dword's bytes) */
#define CD_LETTER 3                     /* D: */
static const uint32_t track1_start = 0x000200u;   /* 00:02:00 */
static const uint32_t lead_out = 0x2A4200u;       /* 66:42:00 */

/* INT 2Fh AX=1510h: the request at the real-mode address `rm` */
static void mscdex(uint32_t rm)
{
    uint8_t cmd = lrb(rm + 2);
    uint32_t xfer = ((uint32_t)lrw(rm + 0x10) << 4) + lrw(rm + 0x0E);

    if (cmd == 3) {             /* IOCTL input */
        switch (lrb(xfer)) {
        case 0x0A:              /* audio disk info */
            lwb(xfer + 1, 1);
            lwb(xfer + 2, 1);
            lwd(xfer + 3, lead_out);
            break;
        case 0x0B:              /* track info */
            lwd(xfer + 2, lrb(xfer + 1) == 1 ? track1_start : 0);
            lwb(xfer + 6, 0x40);        /* a data track (not read by the game here) */
            break;
        default:
            pi_stop("MSCDEX: an IOCTL input the port does not answer");
        }
    } else if (cmd == 0x0C) {   /* IOCTL output */
        if (lrb(xfer) != 1)
            pi_stop("MSCDEX: an IOCTL output the port does not answer");
        /* 1: lock or unlock the door: nothing to do */
    } else {
        pi_stop("MSCDEX: a request the port does not answer");
    }
    lww(rm + 3, 0x0100);        /* done, no error */
}

/* CODE:35B0D: the header at EBX and what follows (64h bytes) to the
 * real-mode buffer (INT 93h AH=13h), INT 2Fh AX=1510h, its status */
static uint16_t CD_REQUEST(uint32_t ebx)
{
    uint32_t rm = (uint32_t)pmax_rm_seg() << 4, i;

    for (i = 0; i < 0x19 * 4; i++)
        lwb(rm + i, rb(ebx + i));
    mscdex(rm);
    return lrw(rm + 3);
}

int CD_INSTALLED(void)
{
    /* INT 2Fh AX=1500h: BX drives, CX the first */
    ww(N_CD_DRIVE, CD_LETTER);
    ww(N_CD_IOCTL + 0x10, pmax_rm_seg());
    return 0;
}

void CD_LOCK(uint8_t bl)
{
    wb(N_CD_IOCTL + 0x24, bl);
    wb(N_CD_IOCTL + 2, 0x0C);
    ww(N_CD_IOCTL + 0x12, 2);
    ww(N_CD_IOCTL + 0x10, pmax_rm_seg());
    ww(N_CD_IOCTL + 0x0E, 0x23);        /* CODE:35EFF, the control block */
    CD_REQUEST(N_CD_IOCTL);
}

int CD_READ_TOC(void)
{
    uint32_t rm = (uint32_t)pmax_rm_seg() << 4;
    uint16_t ax;
    uint8_t dl, dh;

    wb(N_CD_IOCTL + 2, 3);
    ww(N_CD_IOCTL + 0x12, 7);
    ww(N_CD_IOCTL + 0x10, pmax_rm_seg());
    ww(N_CD_IOCTL + 0x0E, 0x26);        /* CODE:35F02, audio disk info */
    ax = CD_REQUEST(N_CD_IOCTL);
    if (ax & 0x8000) {
        if ((ax & 0xFF) != 0x0F)
            return 1;
        ww(N_CD_IOCTL + 0x0E, 0x26);
        if (CD_REQUEST(N_CD_IOCTL) & 0x8000)
            return 1;
    }
    dl = lrb(rm + 0x27);
    dh = lrb(rm + 0x28);
    do {
        ww(N_CD_IOCTL + 0x0E, 0x2D);    /* CODE:35F09, track info */
        wb(N_CD_IOCTL + 0x2E, dl);
        CD_REQUEST(N_CD_IOCTL);
        wd(N_CD_TRACK_START + (uint32_t)dl * 4, lrd(rm + 0x2F));
        dl++;
    } while (dl <= dh);
    return 0;
}
