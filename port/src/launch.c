/* launch.c - the port's setup screen before the game (doskit's
 * launcher.h): which table to start, the game's options (stored in
 * ILLUSION.CFG as the game's own options screen stores them), the sound
 * (volume, doskit's audiofx), the keys and a controller's buttons; and
 * the volume keys in play (frame.h's hud).  The port's, not the game's:
 * none of it changes what the game does with the options it reads.
 *
 * The port's settings (all but the game's options) are kept in the data
 * folder's pinative.cfg. */
#include <stdio.h>
#include <string.h>
#include "audiofx.h"
#include "frame.h"
#include "game.h"
#include "launcher.h"
#include "pad.h"
#include "platform.h"
#include "pmax.h"
#include "sys.h"

int pi_start_table;             /* 1..4: that table at once; 0 the chooser */
int pi_skip_intro;

/* ---- the settings */

static int skip_intro, fullscreen = 0;
static int opt[6];              /* the game's options, OPTIONS+0..5 */
static int volume = 10, bass = 4, treble = 4, oomph = 0, headphone = 0;

/* the game's keys (docs/HANDOFF.md, "The keys"; state+0E46h + the code)
 * and the player's for them */
enum { K_LFLIP, K_RFLIP, K_PLUNGER, K_NUDGE_L, K_NUDGE_R, K_NUDGE_UP, K_START, K_PAUSE, K_COUNT };
static const int game_key[K_COUNT] = { 0x2A, 0x36, 0x1C, 0x38, 0xB8, 0x39, 0x3B, 0x19 };
static int player_key[K_COUNT] = { 0x2A, 0x36, 0x1C, 0x38, 0xB8, 0x39, 0x3B, 0x19 };

/* a controller's button: one of these */
enum { P_NONE, P_LFLIP, P_RFLIP, P_PLUNGER, P_NUDGE_L, P_NUDGE_R, P_NUDGE_UP, P_START, P_PAUSE,
       P_ESC, P_Y, P_N, P_UP, P_DOWN, P_LEFT, P_RIGHT, P_COUNT };
static const char *const pad_actions[] = {
    "nothing", "left flipper", "right flipper", "plunger / Enter", "nudge (Left Alt)",
    "nudge (Right Alt)", "nudge (Space)", "start a game (F1)", "pause (P)", "Esc", "Y", "N",
    "Up", "Down", "Left", "Right", NULL
};
static int pad_choice[PAD_BUTTONS] = {
    [PAD_A] = P_PLUNGER, [PAD_B] = P_ESC, [PAD_X] = P_NUDGE_UP, [PAD_Y] = P_Y,
    [PAD_BACK] = P_PAUSE, [PAD_START] = P_START, [PAD_LSTICK] = P_NUDGE_L,
    [PAD_RSTICK] = P_NUDGE_R, [PAD_LB] = P_LFLIP, [PAD_RB] = P_RFLIP, [PAD_LT] = P_LFLIP,
    [PAD_RT] = P_RFLIP, [PAD_UP] = P_UP, [PAD_DOWN] = P_DOWN, [PAD_LEFT] = P_LEFT,
    [PAD_RIGHT] = P_RIGHT,
};

static const char *const yesno[] = { "No", "Yes", NULL };
static const char *const window[] = { "Window", "Full Screen", NULL };
/* the game's options screen's lines and values, in the order of their
 * bytes (docs/HANDOFF.md, "The options"; the words are the port's) */
static const char *const balls[] = { "3", "5", NULL };
static const char *const angle[] = { "Normal", "High", "Very High", "Very Low", "Low", NULL };
static const char *const scrolling[] = { "Medium", "Smooth", "Fast", NULL };
static const char *const multiball[] = { "6", "3", "4", "5", NULL };
static const char *const tilt[] = { "Normal", "Earthquake (never tilts)", NULL };
static const char *const resolution[] = { "VGA 360x350", "SVGA 640x480", "SVGA 800x600",
                                          "VGA 320x240", NULL };
static const char *const vol_values[] = { "0 (off)", "1", "2", "3", "4", "5", "6", "7", "8",
                                          "9", "10", NULL };
static const char *const db_values[] = { "-12 dB", "-9 dB", "-6 dB", "-3 dB", "0 dB", "+3 dB",
                                          "+6 dB", "+9 dB", "+12 dB", NULL };
static const char *const oomph_values[] = { "off", "+3 dB", "+6 dB", "+9 dB", "+12 dB", NULL };

enum { A_CHOOSER = 1, A_TABLE1, A_TABLE2, A_TABLE3, A_TABLE4, A_QUIT };

static LauncherItem start_items[] = {
    { LI_HEAD, "Play", NULL, NULL, NULL, 0, NULL },
    { LI_ACTION, "Start", NULL, NULL, NULL, A_CHOOSER,
      "Start from the intro." },
    { LI_ACTION, "Law 'n Justice", NULL, NULL, NULL, A_TABLE1, "Start at this table." },
    { LI_ACTION, "Babewatch", NULL, NULL, NULL, A_TABLE2, "Start at this table." },
    { LI_ACTION, "Extreme Sports", NULL, NULL, NULL, A_TABLE3, "Start at this table." },
    { LI_ACTION, "The Vikings", NULL, NULL, NULL, A_TABLE4, "Start at this table." },
    { LI_HEAD, "", NULL, NULL, NULL, 0, NULL },
    { LI_CHOICE, "Skip the intro", "skipintro", yesno, &skip_intro, 0, NULL },
    { LI_CHOICE, "Display", "fullscreen", window, &fullscreen, 0, "Alt+Enter switches too." },
    { LI_HEAD, "", NULL, NULL, NULL, 0, NULL },
    { LI_ACTION, "Quit", NULL, NULL, NULL, A_QUIT, NULL },
};

static LauncherItem game_items[] = {
    { LI_HEAD, "Game Options", NULL, NULL, NULL, 0, NULL },
    { LI_CHOICE, "Balls per game", NULL, balls, &opt[0], 0, NULL },
    { LI_CHOICE, "Table angle", NULL, angle, &opt[1], 0, "Higher angle applies higher pull to the ball." },
    { LI_CHOICE, "Scrolling", NULL, scrolling, &opt[2], 0, NULL },
    { LI_CHOICE, "Multiball maximum", NULL, multiball, &opt[3], 0, NULL },
    { LI_CHOICE, "Tilt sensitivity", NULL, tilt, &opt[4], 0, NULL },
    { LI_CHOICE, "Resolution", NULL, resolution, &opt[5], 0,
      "SVGA shows more of the table's height." },
};

static LauncherItem sound_items[] = {
    { LI_HEAD, "Sound", NULL, NULL, NULL, 0, NULL },
    { LI_CHOICE, "Volume", "volume", vol_values, &volume, 0, NULL },
    { LI_CHOICE, "Bass", "bass", db_values, &bass, 0, NULL },
    { LI_CHOICE, "Treble", "treble", db_values, &treble, 0, NULL },
    { LI_CHOICE, "Oomph (deep bass)", "oomph", oomph_values, &oomph, 0, NULL },
    { LI_CHOICE, "Headphones", "headphone", yesno, &headphone, 0,
      "A wider stereo picture for headphones." },
};

static LauncherItem key_items[] = {
    { LI_HEAD, "Keyboard Configuration", NULL, NULL, NULL, 0, NULL },
    { LI_KEY, "Left flipper", "key_leftflipper", NULL, &player_key[K_LFLIP], 0,
      "Default: Left Shift or Left Ctrl." },
    { LI_KEY, "Right flipper", "key_rightflipper", NULL, &player_key[K_RFLIP], 0,
      "Default: Right Shift or Right Ctrl." },
    { LI_KEY, "Plunger", "key_plunger", NULL, &player_key[K_PLUNGER], 0, "Default: Enter." },
    { LI_KEY, "Nudge Left (Default: Left Alt)", "key_nudgeleft", NULL, &player_key[K_NUDGE_L], 0, NULL },
    { LI_KEY, "Nudge Right (Default: Right Alt)", "key_nudgeright", NULL, &player_key[K_NUDGE_R], 0, NULL },
    { LI_KEY, "Nudge Up (Default: Space)", "key_nudgeup", NULL, &player_key[K_NUDGE_UP], 0, NULL },
    { LI_KEY, "Pause", "key_pause", NULL, &player_key[K_PAUSE], 0, "Default: P." },
};

#define PAD_ITEM(b, label) \
    { LI_CHOICE, label, NULL, pad_actions, &pad_choice[b], 0, NULL }
static LauncherItem pad_items[] = {
    { LI_HEAD, "Controller Configuration", NULL, NULL, NULL, 0, NULL },
    PAD_ITEM(PAD_A, "A"), PAD_ITEM(PAD_B, "B"), PAD_ITEM(PAD_X, "X"), PAD_ITEM(PAD_Y, "Y"),
    PAD_ITEM(PAD_LB, "Left shoulder"), PAD_ITEM(PAD_RB, "Right shoulder"),
    PAD_ITEM(PAD_LT, "Left trigger"), PAD_ITEM(PAD_RT, "Right trigger"),
    PAD_ITEM(PAD_LSTICK, "Left stick (pressed)"), PAD_ITEM(PAD_RSTICK, "Right stick (pressed)"),
    PAD_ITEM(PAD_START, "Start"), PAD_ITEM(PAD_BACK, "Back"),
    PAD_ITEM(PAD_UP, "D-pad up"), PAD_ITEM(PAD_DOWN, "D-pad down"),
    PAD_ITEM(PAD_LEFT, "D-pad left"), PAD_ITEM(PAD_RIGHT, "D-pad right"),
};

static LauncherPage pages[] = {
    { "Start", start_items, sizeof start_items / sizeof start_items[0] },
    { "Game", game_items, sizeof game_items / sizeof game_items[0] },
    { "Sound", sound_items, sizeof sound_items / sizeof sound_items[0] },
    { "Keys", key_items, sizeof key_items / sizeof key_items[0] },
    { "Controller", pad_items, sizeof pad_items / sizeof pad_items[0] },
};
#define NPAGES (int)(sizeof pages / sizeof pages[0])

/* the controllers' names in the file (pad.h's) */
static void pad_names(void)
{
    static char names[PAD_BUTTONS][24];
    size_t i, k;

    for (i = 0; i < sizeof pad_items / sizeof pad_items[0]; i++) {
        LauncherItem *it = &pad_items[i];

        if (it->kind != LI_CHOICE)
            continue;
        k = (size_t)(it->value - pad_choice);
        snprintf(names[k], sizeof names[k], "pad_%s", pad_button_name((int)k));
        it->name = names[k];
    }
}

/* ---- applied */

static unsigned char keymap[256];
static PadKeys play_keys;

static void apply_keys(void)
{
    static const int action_key[P_COUNT] = {
        0, 0x2A, 0x36, 0x1C, 0x38, 0xB8, 0x39, 0x3B, 0x19, 0x01, 0x15, 0x31, 0xC8, 0xD0, 0xCB, 0xCD
    };
    int i;

    for (i = 0; i < 256; i++)
        keymap[i] = (unsigned char)i;
    for (i = 0; i < K_COUNT; i++)
        if (player_key[i] && player_key[i] != game_key[i])
            keymap[player_key[i]] = (unsigned char)game_key[i];
    frame_set_keymap(keymap);

    memset(play_keys, 0, sizeof play_keys);
    for (i = 0; i < PAD_BUTTONS; i++) {
        int a = pad_choice[i], code = a > 0 && a < P_COUNT ? action_key[a] : 0, k;

        /* the player's key for a game key, which the keymap turns into it */
        for (k = 0; k < K_COUNT; k++)
            if (code == game_key[k] && player_key[k])
                code = player_key[k];
        play_keys[i][0] = (unsigned char)code;
    }
    pad_set_keys(&play_keys);
}

static int hud_until;           /* the picture count until which the volume shows */

static void apply_sound(void)
{
    plat_audio_lock();
    audiofx_set(bass * 3 - 12, treble * 3 - 12, oomph * 3, headphone);
    plat_audio_unlock();
}

float pi_volume_gain(void)
{
    return (float)volume / 10.0f;
}

static void changed(const LauncherItem *it)
{
    if (it->value == &fullscreen)
        plat_set_fullscreen(fullscreen);
    else if (it->value == &bass || it->value == &treble || it->value == &oomph ||
             it->value == &headphone)
        apply_sound();
}

/* the volume in play: a bar at the top left for two seconds */
static void hud_draw(VgaFrame *f)
{
    int x, y, best = 0, bestv = -1, i;

    if ((long)frame_count() >= hud_until)
        return;
    for (i = 0; i < 256; i++) {
        uint32_t c = f->palette[i];
        int v = (int)((c >> 16 & 0xFF) + (c >> 8 & 0xFF) + (c & 0xFF));
        if (v > bestv) {
            bestv = v;
            best = i;
        }
    }
    for (y = 4; y < 10 && y < f->height; y++)
        for (x = 0; x < 10; x++) {
            int bx = 4 + x * 8, k;
            for (k = 0; k < 6 && bx + k < f->width; k++)
                f->pixels[y * f->width + bx + k] =
                    (uint8_t)(x < volume || y == 4 || y == 9 || k == 0 || k == 5 ? best : 0);
        }
}

static void hud_control(int c)
{
    static int before_mute = 10;

    if (c == PLAT_VOLUME_UP && volume < 10)
        volume++;
    else if (c == PLAT_VOLUME_DOWN && volume > 0)
        volume--;
    else if (c == PLAT_MUTE) {
        if (volume) {
            before_mute = volume;
            volume = 0;
        } else
            volume = before_mute;
    }
    hud_until = (int)frame_count() + 140;
}

static void settings_path(char *out, size_t n)
{
    char data[SYS_PATH];

    sys_data_dir(data, sizeof data);
    sys_join(out, n, data, "pinative.cfg");
}

void pi_settings_save(void)
{
    char path[SYS_PATH];

    settings_path(path, sizeof path);
    launcher_save(path, "Pinball Illusions' port: its settings, written by its setup screen",
                  pages, NPAGES);
}

int pi_launch(int show)
{
    char path[SYS_PATH];
    uint8_t o[6], before[6];
    int i, r = A_CHOOSER;

    if (!show)
        return 0;               /* headless: the game as the original starts */
    pad_names();
    settings_path(path, sizeof path);
    launcher_load(path, pages, NPAGES);
    pmax_cfg_options(o, 6);
    for (i = 0; i < 6; i++)
        opt[i] = o[i];
    memcpy(before, o, 6);
    plat_set_fullscreen(fullscreen);
    apply_sound();
    {
        r = launcher_run("Pinball Illusions",
                         NULL,
                         pages, NPAGES, changed);
        pi_settings_save();
        for (i = 0; i < 6; i++)
            o[i] = (uint8_t)opt[i];
        if (memcmp(o, before, 6)) {
            /* the options screen sets the SVGA mode 0 when the resolution
             * changed (CODE:32D5A), so SVGA_CHECK looks for it again */
            uint8_t o7[7];

            pmax_cfg_options(o7, 7);
            memcpy(o7, o, 6);
            if (o[5] != before[5])
                o7[6] = 0;
            pmax_cfg_set_options(o7, 7);
        }
    }
    if (r == LAUNCHER_QUIT || r == A_QUIT)
        return -1;
    pi_start_table = r >= A_TABLE1 && r <= A_TABLE4 ? r - A_TABLE1 + 1 : 0;
    pi_skip_intro = skip_intro || pi_start_table != 0;
    apply_keys();
    frame_set_hud(hud_draw, hud_control);
    return 0;
}
