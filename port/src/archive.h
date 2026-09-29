/* archive.h - the resource archive appended to ILLUSION.EXE, as
 * tools/illfiles.py reads it (docs/reverse-engineering.md): a directory
 * of names, offsets and sizes after the MZ image, each file packed with
 * NLZW (LZW codes through an arithmetic coder).
 */
#ifndef PI_ARCHIVE_H
#define PI_ARCHIVE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char name[64];              /* as in the directory: DATA\S001\STAGE.M */
    uint32_t off, size, packed; /* off from the archive's start */
} ArcEntry;

typedef struct {
    uint8_t *data;              /* the whole of ILLUSION.EXE */
    size_t len, base;           /* base: where the archive starts */
    int count;
    ArcEntry *entries;
} Archive;

/* opens the archive in the program at `path`; 0, or -1 with a message */
int arc_open(Archive *a, const char *path, char *err, size_t n);
void arc_close(Archive *a);
/* the entry of that name (case and / or \ do not matter), or NULL */
const ArcEntry *arc_find(const Archive *a, const char *name);
/* the entry unpacked into a new buffer (malloc) of e->size bytes, or NULL
 * with a message */
uint8_t *arc_unpack(const Archive *a, const ArcEntry *e, char *err, size_t n);

#endif
