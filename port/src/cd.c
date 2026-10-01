/* cd.c - the CD through MSCDEX: CD_INSTALLED (CODE:35B52), CD_LOCK
 * (CODE:35B7D), CD_READ_TOC (CODE:35C58), CD_STOP, CD_PLAY, CD_REQUEST (CODE:35B0D), and in
 * place of MSCDEX a drive D: with the tracks of the GOG release's cue sheet
 * played by doskit's cdaudio.h (cd_open), as dosrun has it with -cue; with
 * no sheet one data track, as dosrun has it without.
 */
#include <stdio.h>
#include "cdaudio.h"
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "platform.h"
#include "pmem.h"

/* the disc: tracks first..last, each track's start and the lead-out as
 * Red Book addresses (frame, second, minute, 0 as the dword's bytes) */
#define CD_LETTER 3                     /* D: */
static const uint32_t track1_start = 0x000200u;   /* 00:02:00 */
static const uint32_t lead_out = 0x2A4200u;       /* 66:42:00 */
static int cue_on;                      /* the tracks from a cue sheet */
static int cd_paused;                   /* a play stopped once, for resume */

int cd_open(const char *cue)
{
    char err[256];

    cue_on = cda_open(cue, err, sizeof err) == 0;
    if (!cue_on)
        fprintf(stderr, "CD audio: %s\n", err);
    return cue_on;
}

static uint32_t redbook(uint32_t f)
{
    return ((f / 4500) << 16) | (((f / 75) % 60) << 8) | (f % 75);
}

static uint32_t from_redbook(uint32_t r)
{
    return ((r >> 16) & 0xFF) * 4500 + ((r >> 8) & 0xFF) * 75 + (r & 0xFF);
}

/* INT 2Fh AX=1510h: the request at the real-mode address `rm` */
static void mscdex(uint32_t rm)
{
    uint8_t cmd = lrb(rm + 2);
    uint32_t xfer = ((uint32_t)lrw(rm + 0x10) << 4) + lrw(rm + 0x0E);

    if (cmd == 3) {             /* IOCTL input */
        switch (lrb(xfer)) {
        case 0x0A:              /* audio disk info */
            lwb(xfer + 1, 1);
            lwb(xfer + 2, cue_on ? (uint8_t)cda_tracks() : 1);
            lwd(xfer + 3, cue_on ? redbook(cda_leadout()) : lead_out);
            break;
        case 0x0B:              /* track info */
            if (cue_on) {
                int t = lrb(xfer + 1);

                if (t < 1 || t > cda_tracks()) {
                    lww(rm + 3, 0x8108);        /* done, sector not found */
                    return;
                }
                lwd(xfer + 2, redbook(cda_track_start(t - 1)));
                lwb(xfer + 6, cda_track_data(t - 1) ? 0x40 : 0x00);
                break;
            }
            lwd(xfer + 2, lrb(xfer + 1) == 1 ? track1_start : 0);
            lwb(xfer + 6, 0x40);        /* a data track (not read by the game here) */
            break;
        default:
            pi_stop("MSCDEX: an IOCTL input the port does not answer");
        }
    } else if (cmd == 0x0C) {   /* IOCTL output */
        if (lrb(xfer) != 1 && lrb(xfer) != 3)
            pi_stop("MSCDEX: an IOCTL output the port does not answer");
        /* 1: lock or unlock the door: nothing to do; 3: the audio
         * channels, input and volume for each output */
        if (lrb(xfer) == 3 && cue_on) {
            uint8_t in[4], vol[4];
            int i;

            for (i = 0; i < 4; i++) {
                in[i] = lrb(xfer + 1 + 2 * i);
                vol[i] = lrb(xfer + 2 + 2 * i);
            }
            plat_audio_lock();
            cda_channels(in, vol);
            plat_audio_unlock();
        }
    } else if (cmd == 0x85) {  /* stop: a play paused, else forgotten */
        if (cue_on) {
            plat_audio_lock();
            cd_paused = cda_playing();
            if (cd_paused)
                cda_stop();
            else
                cda_play(0, 0);
            plat_audio_unlock();
        }
    } else if (cmd == 0x84) {  /* play audio: frames from a start */
        if (cue_on) {
            uint32_t start = lrd(rm + 0x0E);

            if (lrb(rm + 0x0D))
                start = from_redbook(start);
            plat_audio_lock();
            cda_play(start, lrd(rm + 0x12));
            cd_paused = 0;
            plat_audio_unlock();
        }
        /* without a sheet: taken, nothing plays (the data track's frames
         * are not sound) */
    } else if (cmd == 0x88 && cue_on) {        /* resume */
        plat_audio_lock();
        if (cd_paused)
            cda_resume();
        cd_paused = 0;
        plat_audio_unlock();
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

/* CODE:35C09: IOCTL output 3 (audio channel control, CODE:35EF6: inputs
 * 0..3 to outputs 0..3) with the volume `bl` for outputs 0 and 1 */
void CD_VOLUME(uint8_t bl)
{
    wb(N_CD_CHANNELS + 4, bl);
    wb(N_CD_CHANNELS + 2, bl);
    wb(N_CD_IOCTL + 2, 0x0C);
    ww(N_CD_IOCTL + 0x12, 9);
    ww(N_CD_IOCTL + 0x10, pmax_rm_seg());
    ww(N_CD_IOCTL + 0x0E, (uint16_t)(N_CD_CHANNELS - N_CD_IOCTL));
    CD_REQUEST(N_CD_IOCTL);
}

/* CODE:35E4F: the request at CD_STOP_REQ (command 85h) */
void CD_STOP(void)
{
    CD_REQUEST(N_CD_STOP_REQ);
}

/* CODE:35E65: a Red Book address (frame, second, minute as the low
 * bytes) in frames; the second's word keeps the dword's top byte (MOV AH,0
 * after SHR 8), 0 in a Red Book address */
static uint32_t RB_FRAMES(uint32_t eax)
{
    return (eax >> 16) * 0x1194 + ((eax >> 8) & 0xFFFF00FFu) * 0x4B + (eax & 0xFF);
}

/* CODE:35DDC: track `ebx` played from its start (CD_TRACK_START) to the
 * next track's, or for 3:34 (32200h) when that is negative; the request at
 * CODE:35EA1 (command 84h, start at CD_PLAY_START, frames at CD_PLAY_LEN);
 * the frames back */
uint32_t CD_PLAY(uint32_t ebx)
{
    uint32_t ecx = RB_FRAMES(0x32200), eax;

    wd(N_CD_PLAY_START, rd(N_CD_TRACK_START + ebx * 4));
    eax = rd(N_CD_TRACK_START + 4 + ebx * 4);
    if ((int32_t)eax >= 0)
        ecx = RB_FRAMES(eax) - RB_FRAMES(rd(N_CD_TRACK_START + ebx * 4));
    wd(N_CD_PLAY_LEN, ecx);
    CD_REQUEST(N_CD_PLAY_REQ);
    return rd(N_CD_PLAY_LEN);
}
