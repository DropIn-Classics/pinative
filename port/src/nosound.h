/* nosound.h - the sound driver the port has (nosound.c), called as the game
 * calls its driver: CALL FWORD to DRIVER_ENTRY with the command in EAX.
 */
#ifndef PI_NOSOUND_H
#define PI_NOSOUND_H

#include <stdint.h>

/* the registers a command reads and returns */
typedef struct {
    uint32_t eax, ebx, ecx, edx, edi;
    uint16_t ds, es, fs;
} NsRegs;

/* DISPATCH (NOSOUND CODE:0933) through the code selector `cs`: command
 * r->eax; 1 for CF (then r->eax is the driver's error) */
int ns_call(uint16_t cs, NsRegs *r);

/* nsplay.c, with DS the driver's: CODE:1A98 and CODE:1057 */
void MIX_UPDATE(void);
void CHANNELS_RESET(void);
/* CODE:1A7B */
uint16_t MIX_ROOM(void);

#endif
