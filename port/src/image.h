/* image.h - ILLUSION.386 in the port's memory (doskit's pmem.h), where
 * pMAX puts it: unpacked from the archive in the player's ILLUSION.EXE,
 * checked against the file the hints describe (gen/names.h), loaded at the
 * linear address and with the selector a run showed (docs/HANDOFF.md,
 * "The image in memory at its entry").
 */
#ifndef PI_IMAGE_H
#define PI_IMAGE_H

#include <stddef.h>
#include "archive.h"
#include "pmem.h"

/* the image's linear address and descriptor 0's selector under pMAX, as
 * dosrun's run of ILLUSION.EXE has them */
#define PI_IMAGE_BASE 0x100F30u
#define PI_SEL_CODE 0x1Cu

extern PmImage pi_image;
/* the archive in the player's ILLUSION.EXE, kept open for the files the
 * program loads through pMAX (pmax.h) */
extern Archive pi_archive;

/* ILLUSION.386 from `game`'s ILLUSION.EXE into memory, pm_ds at CODE,
 * pi_archive open;
 * 0, or -1 with a message */
int pi_load_image(const char *game, char *err, size_t n);

#endif
