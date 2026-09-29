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

/* INT 94h AH=8: the configuration file.  The port reads the player's file
 * at `path` (NULL or missing: none, the options all 0; the original then
 * creates it and runs the sound set-up, which the port does not). */
void pmax_cfg_open(const char *path);
/* INT 94h AH=7: its 200h bytes of options to `off` in CODE (at pm_ds) */
void pmax_cfg_read(uint32_t off);

#endif
