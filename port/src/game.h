/* game.h - the translated routines of ILLUSION.386, by the hints' names.
 */
#ifndef PI_GAME_H
#define PI_GAME_H

#include <stdint.h>

#include "launcher.h"

/* Where the port stops: a routine not translated yet.  With a memory file
 * set (and a video memory file), the memory is written there first (for tools/memcmp.py against the
 * original stopped at the same routine); then the program ends. */
extern const char *pi_stop_mem, *pi_stop_vram;
void pi_stop(const char *name);
/* the program's end: the memory files written, the platform shut down */
void pi_end(void);

/* launch.c: the port as doskit's launcher.h names it (the setup screen's
 * title bar, the dialogs about the game's files) */
extern const LauncherApp pi_app;

/* launch.c, the port's own: the setup screen (shown when `show`) and its
 * settings applied; 0, or -1 when the player quit there */
int pi_launch(int show);
/* the table to start at once (1..4; 0: the chooser), the intro skipped */
extern int pi_start_table, pi_skip_intro;
/* the volume, 0..1 */
float pi_volume_gain(void);

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
/* the GOG release's cue sheet for the CD's audio tracks (cd.c); 1 if read */
int cd_open(const char *cue);
int CD_INSTALLED(void);
void CD_LOCK(uint8_t bl);
int CD_READ_TOC(void);
void CD_STOP(void);
/* CODE:35DDC: the track's frames back (ECX) */
uint32_t CD_PLAY(uint32_t ebx);
/* CODE:35C0D: the CD's volume for both outputs */
void CD_VOLUME(uint8_t bl);

/* hostcb.c: the host callbacks; 1 for CF */
int HCB_ALLOC(uint16_t ds, uint32_t size, uint16_t *bx);
int HCB_ALLOC_LOW(uint16_t ds, uint32_t size, uint16_t *bx);
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

/* tblinit.c: CODE:29A5A, the ball at [0010] put at the plunger;
 * CODE:2C414, 2C467, the level's six pointers into it */
void BALL_PLACE(void);
void BALL_BUNDLE1(void);
void BALL_BUNDLE2(void);

/* dotmatrix.c: CODE:2833E, 1 (CF) when a block or file failed; CODE:2FBA8 */
int DM_LOAD(void);
void DM_CLEAR(void);
void DM_TEXT_CLEAR(void);
void DM_ANIM_CLEAR(void);
void DM_SAVE(void);
void DM_RESTORE(void);
int DM_TEXT_DRAW(void);
/* CODE:275D7, 275E4 (1: CF), 2758B */
int DM_HISCORE_DRAW(void);
int DM_SCORE_DRAW(void);
void BIN_BCD(void);
void DM_SCORE_IDLE(void);

/* module.c: CODE:B22D, 1 (CF) when a file failed; CODE:30368, host
 * vector +18h */
int TABLE_MODULE(void);
/* ADC AL,[src] and DAA into the byte at `dst`, *cf the carry */
void adc_daa(uint32_t dst, uint32_t src, int *cf);
void sbb_das(uint32_t dst, uint32_t src, int *cf);
void bcd12_add(uint32_t dst, uint32_t src);
void DEC_TEXT(void);

/* tblvga.c: CODE:911F */
void TBL_VGA_INIT(void);

/* tblinit.c: CODE:BA9B, CODE:B797 */
void TOP_COLOURS_SET(void);
void TOP_COLOURS2_SET(void);
void BALLS_INIT(void);
/* CODE:28C45, CODE:15030: 1 (CF) when a file failed */
int LIGHTS_LOAD(void);
int FLIPDAT_LOAD(void);
/* CODE:B0E3: 1 (CF) when OPT_MULTIBALL is not 0..3 */
int MULTIBALL_CAP(void);

/* flipper.c: CODE:1527F, CODE:156C4 */
void FLIPPER_RENDER(void);
void FLIPPERS_DRAW(void);
/* CODE:1048B; CODE:144AD, CODE:104A5 */
void FLIPPERS_STEP(void);
void FLIPPERS_MOVE(void);
void FLIPPER_SOUNDS(void);
/* phys.c: CODE:1023E */
void BALLS_PHYSICS(void);

/* lights.c: CODE:29AF9, CODE:2EF7A, CODE:2ED15, CODE:2FE8E */
void LIGHTS_RESET(void);
/* lights.c: CODE:29BE1 */
void LIGHTS_BALL_RESET(void);
void LIGHTS_STEP(void);
void FLASH_STEP(void);
/* lights.c: CODE:29033, CODE:28F41 */
void LIGHTS_DRAW(void);
void DROPS_UPDATE(void);
void DROPS_DRAW(void);
/* ball.c: CODE:14DE2, CODE:14C46 */
void BALLS_STEP(void);
void BALLS_SHOW(void);
void BALL_HIDE(void);
void EVENT_QUEUE(void);
/* display.c: CODE:2F35C, CODE:2FEDB, CODE:27A0E */
void DISPLAY_RUN(void);
void DISPLAY_QUEUE(void);
void ANIMS_STEP(void);
/* play.c: CODE:B928, the game; CODE:2F85C, CODE:2FBBF */
void TABLE_GAME(void);
void MUSIC_REQUEST(void);
void TAKE_PAY(void);
/* play.c: CODE:2C037, host vector +0Ch */
void FRAMES_WAIT(void);
/* modcode.c: the table module's routine at DS:`at` (a slot of its
 * header), in C per table; a RET returns, another stops the port */
void MOD_CALL(uint32_t at);
/* sound.c: CODE:7182, the chooser's driver loaded again after a table */
void DRIVER_RELOAD(void);
/* play.c: CODE:A704, CODE:B02B */
void FADE_MIX(uint32_t src, uint32_t dst, uint8_t cl);
void FADE_PAL_SET(void);
/* modcode.c: the module object's +4 at DS:`at`, 1 when it is done */
int MOD_UPDATE(uint32_t at);
/* play.c: CODE:28E5D */
void DROP_SET(void);
/* events.c: CODE:2E82F, 2D080, 2CD3C, 2E8CD, 2EAC9, 2CB69, 2CBCF, 2C7FC */
void LAMP_OFF(void);
/* events.c: CODE:2ECCE, CODE:2FD11 */
void LIGHT_QUEUE(void);
void SCORE_ADD(void);
/* events.c: CODE:3007A */
void RECORD_DISPATCH(void);
void EVENT_RUN(void);
void MODE_RUN(void);
void LIT_LIST_STEP(void);
void BCD_COUNTERS_STEP(void);
void COUNTER_TIMERS(void);
void OBJECT_TIMERS(void);
void OBJECT_HITS(void);

/* table.c: CODE:9F2C, the sound record at [CODE:0000] */
void SFX_PLAY(void);
/* table.c: CODE:9F88, the record at [CODE:0000] as a module's note */
void SFX_NOTE(void);
/* table.c: CODE:9B92, the driver's retrace routines and the table's
 * music started; CODE:B99C and CODE:B9B9, those routines */
void GAME_SOUND(void);
void SOUND_PAUSE(void);
void SOUND_RESUME(void);
void DRV_TICK(void);
void JINGLE_END_CB(void);
void DRV_FRAME(void);
/* table.c: CODE:298EF, MUSIC_UPDATE and the driver's command 6 */
void FRAME_MUSIC(void);
/* table.c: CODE:A323, AL the table 1..4; 1 (CF) when a load failed */
int TABLE(uint8_t al);

#endif
