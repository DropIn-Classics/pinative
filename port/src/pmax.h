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
/* the program's CS (14h in the runs; its DS is PI_SEL_CODE); the table
 * runs on an alias of it (TABLE, CODE:A323: 0Ch in the runs), which it
 * sets here */
uint16_t pmax_code_sel(void);
void pmax_set_code_sel(uint16_t sel);

/* INT 94h AH=8: the configuration file.  The port reads the player's file
 * at `path`.  Missing (or NULL): the options all 0 and a header of the
 * port's own naming NOSOUND.SDR, so that the sound set-up is not called
 * (the original's header is all FFh then and the set-up runs; the port
 * sounds through doskit and needs none); the file is made at `path` by the
 * first AH=6. */
void pmax_cfg_open(const char *path);
/* INT 94h AH=7: its 200h bytes of options to `off` in CODE (at pm_ds) */
void pmax_cfg_read(uint32_t off);
/* INT 94h AH=6: the 200h bytes of options at `off` in CODE stored back,
 * chained as AH=7 reads them, the header left as it was (run 2026-10-01,
 * docs/HANDOFF.md, "The high scores' file"); with no file before, the
 * port's header with them (the original's file made by AH=8 is not looked
 * at).  A NULL path: nothing written. */
void pmax_cfg_write(uint32_t off);
/* INT 94h AH=5: its first 20h bytes to `off` in CODE; 0 (CF clear) */
int pmax_cfg_header(uint32_t off);

/* INT 94h AH=1: the archive's file `name` into a new block of memory; its
 * selector (the block's base: pmax_base), or 0 when it is not there.  The
 * file's size to *size unless `size` is NULL (EDX, as the run had it for
 * intro\INTROANI.ROY; docs/HANDOFF.md, "The chooser's files in a run") */
uint16_t pmax_load(const char *name, uint32_t *size);
/* the same with the name read from the program's memory at DS:`off` (the
 * table's file names, whose digit TABLE_DIGITS writes) */
uint16_t pmax_load_ds(uint32_t off, uint32_t *size);
/* the name (DS:ESI, its offset and selector) the next block's header
 * gets; the block allocated clears it (0:0 then) */
void pmax_name(uint32_t off, uint16_t sel);
/* INT 92h AH=4: a new block of `size` bytes (not cleared); its selector,
 * or 0 when there is no room */
uint16_t pmax_alloc(uint32_t size);
/* INT 92h AH=7: a new block of `size` bytes cut from the top of the heap
 * (the chooser's, run 2026-09-30: 5C08h bytes under FEFFF0h, whatever
 * policy was set; whether AH=7 always takes the top is not known); its
 * selector, 0 when there is no room */
uint16_t pmax_alloc_top(uint32_t size);
/* INT 92h AH=6: the same with no selector: its linear address (EAX), 0
 * when there is no room.  INT 92h AH=2 frees it by that address. */
uint32_t pmax_alloc_linear(uint32_t size);
void pmax_free_linear(uint32_t base);
/* INT 92h AH=9 (a name in ESI, which goes only into pMAX's header): the
 * same by the policy set, not from the top (the module's "DATALOAD 2",
 * docs/HANDOFF.md, "The module's load") */
uint32_t pmax_alloc_linear_here(uint32_t size);
/* INT 92h AH=8: the allocation policy (0 from the bottom of the heap, 1
 * from its top, 2 DOS memory); pmax_load and pmax_alloc follow it.  INT
 * 92h AH=0Ah (a block with a name) is pmax_alloc: the name goes only into
 * pMAX's header. */
void pmax_policy(uint8_t bl);
/* INT 93h AH=8: a second selector for the block of `sel` (the driver's
 * code selector, DX 409Ah); 0 when there is none */
uint16_t pmax_alias(uint16_t sel);
/* INT 93h AH=0Dh: a selector freed (an alias's; its memory stays) */
void pmax_free_sel(uint16_t sel);
/* a selector's linear base (0 for one the port did not give) */
uint32_t pmax_base(uint16_t sel);
/* INT 92h AH=5: the block freed */
void pmax_free(uint16_t sel);

#endif
