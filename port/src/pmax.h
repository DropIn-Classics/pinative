/* pmax.h - what the port does in place of pMAX's services (INT 92h, 93h,
 * 94h), one by one as the translated routines need them.
 */
#ifndef PI_PMAX_H
#define PI_PMAX_H

#include <stddef.h>
#include <stdint.h>

/* the command tail the program gets: the GOG release's ILLUSION.BAT
 * starts `D:\ILLUSION.EXE C:\ILLUSION.CFG /` (docs/HANDOFF.md) */
extern const char *pmax_tail;
/* room for a tail with the player's options after the '/' (main.c, -opt) */
extern char pmax_tail_buf[160];

/* INT 93h AH=15h: the country, two letters as AX ('SV' 5356h Swedish,
 * 'GR' or 'SG' German); the port says neither, as the runs had it */
uint16_t pmax_country(void);

/* INT 93h AH=13h: the real-mode segment of pMAX's transfer buffer
 * (0B3Eh in the runs) */
uint16_t pmax_rm_seg(void);
/* INT 93h AH=5: the selector of video memory (48h in the runs) */
uint16_t pmax_video_sel(void);

/* INT 94h AH=8: the configuration file.  The port reads the player's file
 * at `path` (NULL or missing: none, the options all 0 and the header all
 * FFh; the original then creates it and runs the sound set-up, which the
 * port does not). */
void pmax_cfg_open(const char *path);
/* INT 94h AH=7: its 200h bytes of options to `off` in CODE (at pm_ds) */
void pmax_cfg_read(uint32_t off);
/* INT 94h AH=5: its first 20h bytes to `off` in CODE; 0 (CF clear) */
int pmax_cfg_header(uint32_t off);

/* INT 94h AH=1: the archive's file `name` into a new block of memory; its
 * selector (the block's base: pmax_base), or 0 when it is not there */
uint16_t pmax_load(const char *name);
/* INT 92h AH=4: a new block of `size` bytes (not cleared); its selector,
 * or 0 when there is no room */
uint16_t pmax_alloc(uint32_t size);
/* a block's linear base by its selector */
uint32_t pmax_base(uint16_t sel);
/* INT 92h AH=5: the block freed */
void pmax_free(uint16_t sel);

#endif
