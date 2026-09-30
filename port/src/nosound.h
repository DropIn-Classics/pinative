/* nosound.h - the sound driver the port has (nosound.c), called as the game
 * calls its driver: CALL FWORD to DRIVER_ENTRY with the command in EAX.
 */
#ifndef PI_NOSOUND_H
#define PI_NOSOUND_H

#include <stdint.h>

/* the registers a command reads and returns */
typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi;
    uint16_t ds, es, fs;
} NsRegs;

/* DISPATCH (NOSOUND CODE:0933) through the code selector `cs`: command
 * r->eax; 1 for CF (then r->eax is the driver's error) */
int ns_call(uint16_t cs, NsRegs *r);

/* the game's routine at sel:off, which the driver calls (CALL FWORD) with
 * DS its own: the port's C for it (chooser.c) */
void ns_far_call(uint16_t sel, uint32_t off);

/* the retrace the driver waits for (CODE:08F4), when it is waiting: its
 * callback now instead of at the next picture's start.  For the table's
 * frame step, whose retrace comes shortly after the command-0Fh callback
 * the frame waited for (docs/HANDOFF.md, "The balls' sprites") */
void ns_retrace(void);

/* the master PIC's mask (port 21h), which the port keeps for the driver
 * to read back */
uint8_t pic_in21(void);
void pic_out21(uint8_t al);

/* nsplay.c, with DS the driver's: CODE:1A98 and CODE:1057 */
void MIX_UPDATE(void);
void CHANNELS_RESET(void);
/* CODE:0E2A, command 9 */
int CMD_SFX(NsRegs *r);
/* CODE:1A7B */
uint16_t MIX_ROOM(void);

#endif
