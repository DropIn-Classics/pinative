/* main.c - Pinball Illusions: a native compatibility implementation requiring an
 * installed copy of the original game.
 *
 *     pinative [-game DIR | -gog FILE|FOLDER] [-cue FILE] [-cfg FILE] [-opt LETTERS] [-mem FILE] [-vram FILE] [-entry] [-setup]
 *
 * DIR is the game's unpacked files: -game, else $PINATIVE_GAME, else the first
 * folder `game` holding ILLUSION.EXE beside the program, in the current
 * directory or in the data folder (sys_find_game).  When there is none,
 * the installed GOG release's image is unpacked into the data folder's
 * `game`, or, installed as a folder holding ILLUSION.EXE, that folder
 * copied there (cdimage.h; -gog names the image or the folder instead of
 * looking for it).
 * -cue is the cue sheet of the CD's audio tracks (default: the copy in the
 * data folder's `cd`, made the first time from the GOG release's cue
 * sheet, game.ins or game.inst, and the files it names; else the sheet
 * beside the release; "none", or none found: a CD of one data track, no
 * CD music, cd.c);
 * -cfg is the player's ILLUSION.CFG (default: the one in DIR, if there,
 * else the data folder's; none there: the options 0, no sound set-up, the
 * file made when the options are saved, pmax.h);
 * -setup shows the port's setup screen also in the headless build (the
 * window's build shows it always, but with -entry);
 * -opt the letters after the '/' of the command line (the GOG release's
 * ILLUSION.BAT passes its own arguments there: o the options screen, s the
 * sound set-up, r the options cleared, ? the help).
 *
 * ILLUSION.386 is unpacked from the player's ILLUSION.EXE and loaded as
 * pMAX loads it (image.h), then run from ENTRY by the translated routines
 * (game.h) up to the first one not translated, where the port stops; -mem
 * writes the memory there (doskit's pmem.h, for tools/memcmp.py), -vram
 * the video memory (as dosrun's -vram), -entry
 * stops at ENTRY itself.
 */
#include <stdio.h>
#include <string.h>
#include "cdimage.h"
#include "frame.h"
#include "game.h"
#include "image.h"
#include "pmax.h"
#include "platform.h"
#include "sys.h"
#include "textmode.h"

static const GogRelease release = {
    "Pinball Illusions",                 /* GOG's folder name: check on an installation */
    NULL,                       /* game.gog */
    NULL,                       /* the Mac release's path in /Applications: not known yet */
    "1207664113",               /* GOG's product ID (goggame-ID.info; given by the user
                                 * from a Windows installation) */
    "ILLUSION.EXE",             /* on the CD: images of other games are passed over */
};

/* what earlier versions wrote beside the program (sys_data_migrate) */
static const char *const old_files[] = { "game", NULL };

static uint8_t pixels[TM_WIDTH * TM_HEIGHT];
static uint32_t palette[256];

static void show(void)
{
    tm_render(pixels, palette);
    plat_present(pixels, TM_WIDTH, TM_HEIGHT, palette);
}

/* 1 once the player agreed to a copy from the GOG release (open_cd then
 * takes the CD along without asking again) */
static int agreed;
/* 1 if the player declined the game's files */
static int declined;

/* asks whether `what` may be copied from `from` into `to`: Y or Enter yes,
 * N or Esc no; 1 yes.  Not asked (1) when -gog named the release or the
 * build has no window to ask in. */
static int offer(const char *what, const char *from, const char *to, int named)
{
    const uint8_t attr = TM_ATTR(TM_LIGHTGREY, TM_BLUE), hi = TM_ATTR(TM_YELLOW, TM_BLUE);

    if (named || agreed || !plat_has_window())
        return 1;
    tm_clear(' ', attr);
    tm_text(2, 2, "Pinball Illusions was found (the GOG release):", TM_ATTR(TM_WHITE, TM_BLUE));
    tm_text(2, 3, from, hi);
    tm_text(2, 5, what, TM_ATTR(TM_WHITE, TM_BLUE));
    tm_text(2, 6, to, hi);
    tm_text(2, 8, "Do that now?  Y / N", hi);
    show();
    while (plat_pump()) {
        int b;

        while ((b = plat_read_scancode()) >= 0) {
            if (b == 0x15 || b == 0x1C)                 /* Y, Enter */
                return agreed = 1;
            if (b == 0x31 || b == 0x01)                 /* N, Esc */
                return 0;
        }
        plat_sleep_ms(15);
    }
    return 0;
}

/* the game's files: found, or from the GOG release (its CD image
 * unpacked, or an installed folder of the game's files copied); 1 if
 * there */
static int get_game(const char *given, const char *gog, char *out, size_t n)
{
    char from[SYS_PATH], data[SYS_PATH], err[256];
    int folder, r;

    if (sys_find_game(given, "PINATIVE_GAME", "ILLUSION.EXE", out, n))
        return 1;
    if (given)
        return 0;
    if (gog)
        snprintf(from, sizeof from, "%s", gog);
    else if (!gog_find(&release, from, sizeof from) &&
             !gog_find_folder(&release, from, sizeof from))
        return 0;
    folder = sys_is_dir(from);
    sys_data_dir(data, sizeof data);
    sys_join(out, n, data, "game");
    if (!offer(folder ? "Its game files can be copied into the data folder's"
                      : "Its CD image can be unpacked into the data folder's",
               from, out, gog != NULL)) {
        declined = 1;
        return 0;
    }
    tm_clear(' ', TM_ATTR(TM_LIGHTGREY, TM_BLUE));
    tm_text(2, 2, folder ? "Copying the game's files from" : "Unpacking the game's files from",
            TM_ATTR(TM_WHITE, TM_BLUE));
    tm_text(2, 3, from, TM_ATTR(TM_YELLOW, TM_BLUE));
    show();
    if (folder)
        r = gog_copy(from, out, "ILLUSION.EXE", NULL, NULL, err, sizeof err);
    else
        r = cd_unpack(from, out, "ILLUSION.EXE", NULL, NULL, err, sizeof err);
    if (r != 0) {
        plat_message(err);
        return 0;
    }
    return 1;
}

static const char *const cue_names[] = { "game.ins", "game.inst", NULL };

/* the cue sheet beside the GOG release's image (-gog's, or the one
 * found) into `cue`; 1 if there */
static int gog_cue(const char *gog, char *cue, size_t n)
{
    char img[SYS_PATH], dir[SYS_PATH];
    int i;

    if (gog)
        snprintf(img, sizeof img, "%s", gog);
    else if (!gog_find(&release, img, sizeof img))
        return 0;
    if (sys_is_dir(img))
        snprintf(dir, sizeof dir, "%s", img);
    else if (!sys_parent(img, dir, sizeof dir))
        return 0;
    for (i = 0; cue_names[i]; i++)
        if (sys_find(dir, cue_names[i], cue, n))
            return 1;
    return 0;
}

/* the CD: the copy in the data folder's `cd` (made from the GOG release's
 * cue sheet, its image and its audio tracks the first time, so that the
 * music does not need the installation afterwards), else the sheet beside
 * the release; 1 if one was read */
static int open_cd(const char *gog)
{
    char data[SYS_PATH], dir[SYS_PATH], cue[SYS_PATH], from[SYS_PATH], err[256];
    int i;

    sys_data_dir(data, sizeof data);
    sys_join(dir, sizeof dir, data, "cd");
    if (!sys_is_dir(dir) && gog_cue(gog, from, sizeof from) &&
        offer("Its CD image and music can be copied into the data folder's", from, dir,
              gog != NULL)) {
        tm_clear(' ', TM_ATTR(TM_LIGHTGREY, TM_BLUE));
        tm_text(2, 2, "Copying the CD's image and music from", TM_ATTR(TM_WHITE, TM_BLUE));
        tm_text(2, 3, from, TM_ATTR(TM_YELLOW, TM_BLUE));
        show();
        if (cd_copy_disc(from, dir, NULL, NULL, err, sizeof err) != 0)
            fprintf(stderr, "pinative: %s\n", err);
    }
    for (i = 0; cue_names[i]; i++)
        if (sys_find(dir, cue_names[i], cue, sizeof cue))
            return cd_open(cue);
    return gog_cue(gog, cue, sizeof cue) && cd_open(cue);
}

int main(int argc, char **argv)
{
    const char *given = NULL, *gog = NULL, *cfg = NULL, *cue = NULL;
    char game[SYS_PATH], path[SYS_PATH], err[256];
    int i, entry = 0, setup = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-game") && i + 1 < argc)
            given = argv[++i];
        else if (!strcmp(argv[i], "-gog") && i + 1 < argc)
            gog = argv[++i];
        else if (!strcmp(argv[i], "-cue") && i + 1 < argc)
            cue = argv[++i];
        else if (!strcmp(argv[i], "-cfg") && i + 1 < argc)
            cfg = argv[++i];
        else if (!strcmp(argv[i], "-opt") && i + 1 < argc) {
            snprintf(pmax_tail_buf, sizeof pmax_tail_buf, "%s%s", pmax_tail, argv[++i]);
            pmax_tail = pmax_tail_buf;
        } else if (!strcmp(argv[i], "-mem") && i + 1 < argc)
            pi_stop_mem = argv[++i];
        else if (!strcmp(argv[i], "-vram") && i + 1 < argc)
            pi_stop_vram = argv[++i];
        else if (!strcmp(argv[i], "-entry"))
            entry = 1;
        else if (!strcmp(argv[i], "-setup"))
            setup = 1;
        else {
            fprintf(stderr, "usage: pinative [-game DIR | -gog FILE|FOLDER] [-cue FILE] [-cfg FILE] [-opt LETTERS] [-mem FILE] [-vram FILE] [-entry] [-setup]\n");
            return 2;
        }
    }
    sys_set_app("Pinball Illusions", "pinative");
    sys_data_migrate(old_files);
    if (!plat_init("Pinball Illusions"))
        return 1;
    if (!get_game(given, gog, game, sizeof game)) {
        if (declined)
            plat_message("The game's files were not copied, so nothing can be played. "
                         "Start pinative again to be asked again, or use -game with "
                         "the folder of the game's files.");
        else
            plat_message("The game's files were not found. This program needs an installed "
                         "copy of Pinball Illusions (the GOG release), or -game with its folder.");
        plat_shutdown();
        return 1;
    }
    if (pi_load_image(game, err, sizeof err) != 0) {
        plat_message(err);
        plat_shutdown();
        return 1;
    }
    if (!cue)
        open_cd(gog);
    else if (strcmp(cue, "none") != 0)
        cd_open(cue);
    if (entry)
        pi_stop("ENTRY");
    if (!cfg) {
        sys_join(path, sizeof path, game, "ILLUSION.CFG");
        if (!sys_is_file(path)) {
            char data[SYS_PATH];

            sys_data_dir(data, sizeof data);
            sys_join(path, sizeof path, data, "ILLUSION.CFG");
        }
        cfg = path;
    }
    pmax_cfg_open(cfg);
    /* the port's setup screen (not in a headless run, which runs the game
     * as the original starts) */
    if (pi_launch(setup || (plat_has_window() && !entry)) != 0) {
        plat_shutdown();
        return 0;
    }
    /* the start address taken at the retrace, the picture at the frame's end:
     * the chooser draws its caption into the page on show after the retrace
     * (docs/HANDOFF.md, "CHOOSER_WAIT's loop") */
    frame_set_scanout_end(1);
    ENTRY();
    pi_end();
    return 0;
}
