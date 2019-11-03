//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#pragma once

#ifndef keyboard_public_h_
#define keyboard_public_h_

#include "baselayer.h"	// for the keyboard stuff
#include "scancodes.h"

extern kb_scancode KB_LastScan;

#define KB_GetLastScanCode() (KB_LastScan)
#define KB_SetLastScanCode(scancode) \
    {                                \
        KB_LastScan = (scancode);    \
    }
#define KB_ClearLastScanCode()       \
    {                                \
        KB_SetLastScanCode(sc_None); \
    }
#define KB_GetCh keyGetChar
#define KB_KeyWaiting keyBufferWaiting
#define KB_FlushKeyboardQueue keyFlushChars
#define KB_FlushKeyboardQueueScans keyFlushScans

static inline void KB_KeyEvent(int32_t scancode, int32_t keypressed)
{
    if (keypressed)
        KB_LastScan = scancode;
}

void KB_Startup(void);
void KB_Shutdown(void);
const char *  KB_ScanCodeToString( int scancode ); // convert scancode into a string
int KB_StringToScanCode( const char * string );  // convert a string into a scancode

#endif
