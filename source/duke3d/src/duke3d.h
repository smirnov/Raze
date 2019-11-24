//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef duke3d_h_
#define duke3d_h_

// JBF
#include "a.h"
#include "baselayer.h"
#include "build.h"
#include "cache1d.h"
#include "compat.h"
#include "fx_man.h"
#include "keyboard.h"
#include "pragmas.h"
#include "polymost.h"
#include "gamecvars.h"


#define HEAD2                   APPNAME

#ifdef EDUKE32_STANDALONE
    #define VOLUMEALL           (1)
    #define PLUTOPAK            (1)
    #define VOLUMEONE           (0)
#else
    #define VOLUMEALL           (g_Shareware == 0)
    #define PLUTOPAK            (g_scriptVersion >= 14)
    #define VOLUMEONE           (g_Shareware == 1)
#endif

# define BYTEVERSION_EDUKE32      339

#define BYTEVERSION         (BYTEVERSION_EDUKE32+(PLUTOPAK?1:(VOLUMEONE<<1)))

#define NUMPAGES            1

#define RECSYNCBUFSIZ       2520   //2520 is the (LCM of 1-8)*3
#define MOVEFIFOSIZ         2

// KEEPINSYNC lunatic/con_lang.lua
#define MAXVOLUMES          7
#define MAXLEVELS           64
#define MAXGAMETYPES        16

enum {
    MUS_FIRST_SPECIAL = MAXVOLUMES*MAXLEVELS,

    MUS_INTRO = MUS_FIRST_SPECIAL,
    MUS_BRIEFING = MUS_FIRST_SPECIAL + 1,
    MUS_LOADING = MUS_FIRST_SPECIAL + 2,
    MUS_USERMAP = MUS_FIRST_SPECIAL + 3,
};

////////// TIMING CONSTANTS //////////
// The number of 'totalclock' increments per second:
#define TICRATE             120
// The number of game state updates per second:
#define REALGAMETICSPERSEC  30
// The number of 'totalclock' increments per game state update:
// NOTE: calling a game state update a 'frame' is really weird.
// (This used to be TICRATE/GAMETICSPERSEC, which was 120/26 = 4.615~ truncated
// to 4 by integer division.)
#define TICSPERFRAME        (TICRATE/REALGAMETICSPERSEC)
// Used as a constant to satisfy all of the calculations written with ticrate =
// 26 in mind:
#define GAMETICSPERSEC      26


#define PACKBUF_SIZE        32768

#define TILE_SAVESHOT       (MAXTILES-1)
#define TILE_LOADSHOT       (MAXTILES-3)
#define TILE_TILT           (MAXTILES-2)
#define TILE_ANIM           (MAXTILES-4)
#define TILE_VIEWSCR        (MAXTILES-5)
// Reserved: TILE_VIEWSCR_1 (MAXTILES-6)
// Reserved: TILE_VIEWSCR_2 (MAXTILES-7)
EDUKE32_STATIC_ASSERT(7 <= MAXTILES-MAXUSERTILES);

// sprites with these statnums should be considered for fixing
#define ROTFIXSPR_STATNUMP(k) ((k)==STAT_DEFAULT || (k)==STAT_STANDABLE || (k)==STAT_FX || \
                            (k)==STAT_FALLER || (k)==STAT_LIGHT)
#define ROTFIXSPR_MAGIC 0x18190000

// JBF 20040604: sync is a function on some platforms
#define sync                dsync

// Uncomment the following to remove calls to a.nasm functions with the GL renderers
// so that debugging with valgrind --smc-check=none is possible:
//#define DEBUG_VALGRIND_NO_SMC

#include "actors.h"
#include "common_game.h"
#include "config.h"
#include "control.h"
#include "gamecontrol.h"
#include "game.h"
#include "gamedef.h"
#include "gameexec.h"
#include "gamevars.h"
#include "global.h"
#include "inv.h"
#include "macros.h"
#include "music.h"
#include "namesdyn.h"
#include "network.h"
#include "player.h"
#include "quotes.h"
#include "rts.h"
#include "screentext.h"
#include "sector.h"
#include "sounds.h"
#include "soundsdyn.h"

BEGIN_DUKE_NS

static inline int32_t G_HaveActor(int spriteNum)
{
    return g_tile[spriteNum].execPtr!=NULL;
}

static inline int32_t G_DefaultActorHealth(int spriteNum)
{
    return G_HaveActor(spriteNum) ? g_tile[spriteNum].execPtr[0] : 0;
}


struct GameInterface : ::GameInterface
{
	int app_main() override;
	bool validate_hud(int) override;
	void set_hud_layout(int size) override;
	void set_hud_scale(int size) override;
	bool mouseInactiveConditional(bool condition) override;
	FString statFPS() override;
	GameStats getStats() override;
	// Access to the front end specific menu code. Use is restricted to the main menu, the ingame menu and the skill/episode selection.
	// Everything else is either custom screens or will use the generic option menu style.
	void DrawNativeMenuText(int fontnum, int state, int xpos, int ypos, float fontscale, const char* text, int orientation) override;
	void MenuOpened() override;
	void MenuSelectSound() override;
	void MenuChooseSound() override;
	bool CanSave() override;

};

END_DUKE_NS

#endif
