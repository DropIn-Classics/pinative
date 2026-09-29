/* game.h - the translated routines of ILLUSION.386, by the hints' names.
 */
#ifndef PI_GAME_H
#define PI_GAME_H

/* Where the port stops: a routine not translated yet.  With a memory file
 * set, the memory is written there first (for tools/memcmp.py against the
 * original stopped at the same routine); then the program ends. */
extern const char *pi_stop_mem;
void pi_stop(const char *name);

/* CODE:02A3, the program's start */
void ENTRY(void);

#endif
