/* main.c - Pinball Illusions: a native compatibility implementation requiring an
 * installed copy of the original game.
 *
 *     pinative [-game DIR | -gog FILE|FOLDER] [-cfg FILE] [-opt LETTERS] [-mem FILE] [-vram FILE] [-entry]
 *
 * DIR is the game's unpacked files: -game, else $PINATIVE_GAME, else the first
 * folder `game` holding ILLUSION.EXE beside the program, in the current
 * directory or in the data folder (sys_find_game).  When there is none,
 * the installed GOG release's image is unpacked into the data folder's
 * `game`, or, installed as a folder holding ILLUSION.EXE, that folder
 * copied there (cdimage.h; -gog names the image or the folder instead of
 * looking for it).
 * -cfg is the player's ILLUSION.CFG (default: the one in DIR, if there,
 * else the data folder's; none there: the options 0, no sound set-up, the
 * file made when the options are saved, pmax.h);
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

int main(int argc, char **argv)
{
    const char *given = NULL, *gog = NULL, *cfg = NULL;
    char game[SYS_PATH], path[SYS_PATH], err[256];
    int i, entry = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-game") && i + 1 < argc)
            given = argv[++i];
        else if (!strcmp(argv[i], "-gog") && i + 1 < argc)
            gog = argv[++i];
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
        else {
            fprintf(stderr, "usage: pinative [-game DIR | -gog FILE|FOLDER] [-cfg FILE] [-opt LETTERS] [-mem FILE] [-vram FILE] [-entry]\n");
            return 2;
        }
    }
    sys_set_app("Pinball Illusions", "pinative");
    sys_data_migrate(old_files);
    if (!plat_init("Pinball Illusions"))
        return 1;
    if (!get_game(given, gog, game, sizeof game)) {
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
    ENTRY();
    pi_end();
    return 0;
}
