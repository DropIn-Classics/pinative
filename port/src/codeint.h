/* codeint.h - code of the original's image run by reading its bytes
 * (codeint.c): only the instruction forms of the balls' sprite routines.
 */
#ifndef PI_CODEINT_H
#define PI_CODEINT_H

#include <stdint.h>

/* a segment that is video memory (vga_read, vga_write) */
#define CI_VIDEO 0xFFFFFFFFu

typedef struct {
    uint32_t r[8];              /* EAX ECX EDX EBX ESP EBP ESI EDI */
    uint32_t seg[6];            /* ES CS SS DS FS GS: linear bases or CI_VIDEO */
    int zf, cf;
} CiCpu;

/* the routine at CODE offset `start` up to its RET, on `cpu` */
void code_run(uint32_t start, CiCpu *cpu);

#endif
