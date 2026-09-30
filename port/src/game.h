/* game.h - the translated routines of ILLUSION.386, by the hints' names.
 */
#ifndef PI_GAME_H
#define PI_GAME_H

#include <stdint.h>

/* Where the port stops: a routine not translated yet.  With a memory file
 * set (and a video memory file), the memory is written there first (for tools/memcmp.py against the
 * original stopped at the same routine); then the program ends. */
extern const char *pi_stop_mem, *pi_stop_vram;
void pi_stop(const char *name);
/* the program's end: the memory files written, the platform shut down */
void pi_end(void);

/* INT 21h AH=9: the '$'-ended text at the linear address `at` on the
 * console */
void pi_print_dos(uint32_t at);
/* a DOS text printed, and the program ended (INT 21h AH=4Ch) */
void pi_exit_text(const char *text);

/* CODE:02A3, the program's start; it returns where the original goes back
 * to pMAX (RETF) */
void ENTRY(void);
/* CODE:32F39: 1 (CF) when the program is to end */
int SETUP_ARGS(void);
/* CODE:0753 */
void SVGA_CHECK(void);
/* CODE:049A, CODE:046E */
void VGA_INIT(void);
void HISCORE_INIT(void);
/* CODE:757D (intro.c) */
void CHOOSER_LOAD(void);
/* chooser.c: CODE:4CFB, the table chosen (0..3) or FFh for Esc */
uint8_t CHOOSER_START(void);
/* CODE:7082 (sound.c): 1 (CF) when there is no CD or the driver fails */
int SOUND_START(void);
/* cd.c: CODE:35B52 and 35C58 return 1 for CF */
int CD_INSTALLED(void);
void CD_LOCK(uint8_t bl);
int CD_READ_TOC(void);
void CD_STOP(void);

/* hostcb.c: the host callbacks; 1 for CF */
int HCB_ALLOC(uint32_t size, uint16_t *bx);
int HCB_ALLOC_LOW(uint32_t size, uint16_t *bx);
void HCB_FREE(uint16_t bx);
uint32_t HCB_LINEAR(uint16_t bx);
void HCB_GETVEC(uint8_t bl, uint16_t *es, uint32_t *edx);
void HCB_SETVEC(uint8_t bl, uint16_t es, uint32_t edx);
int HCB_LOAD(uint16_t ds, uint32_t ebx);
void HCB_READ(uint16_t ds, uint32_t edi, uint32_t ecx);
void HCB_UNLOAD(void);
void HCB_SEEK(uint32_t edx);
/* 1: callbacks 6 to 9 keep the file in the table's cells */
void hcb_use_table(int table);

/* dotmatrix.c: CODE:2833E; 1 (CF) when a block or file failed */
int DM_LOAD(void);

/* module.c: CODE:B22D, 1 (CF) when a file failed; CODE:30368, host
 * vector +18h */
int TABLE_MODULE(void);
void DEC_TEXT(void);

/* tblvga.c: CODE:911F */
void TBL_VGA_INIT(void);

/* tblinit.c: CODE:BA9B, CODE:B797 */
void TOP_COLOURS_SET(void);
void BALLS_INIT(void);

/* table.c: CODE:A323, AL the table 1..4; 1 (CF) when a load failed */
int TABLE(uint8_t al);

#endif
