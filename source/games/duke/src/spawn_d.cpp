//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------
#include <utility>
#include "ns.h"
#include "global.h"
#include "sounds.h"
#include "names_d.h"
#include "dukeactor.h"

BEGIN_DUKE_NS


DDukeActor* spawninit_d(DDukeActor* actj, DDukeActor* act, TArray<DDukeActor*>* actors)
{
	if (actorflag(act, SFLAG2_TRIGGERRESPAWN))
	{
		act->spr.yint = act->spr.hitag;
		act->spr.hitag = -1;
	}

	if (iseffector(act))
	{
		// for in-game spawned SE's the init code must not run. The only type ever being spawned that way is SE128 - 
		// but we cannot check that here as the number has not been set yet.
		if (actj == 0) spawneffector(act, actors);
		return act;
	}

	if (act->GetClass() != RUNTIME_CLASS(DDukeActor))
	{
		if (!badguy(act) || commonEnemySetup(act, actj))
			CallInitialize(act);
		return act;
	}
	auto sectp = act->sector();


	switch (act->spr.picnum)
	{
	default:
		if (!badguy(act) || commonEnemySetup(act, actj))
			CallInitialize(act);
		break;
	case DTILE_APLAYER:
	{
		act->spr.scale = DVector2(0, 0);
		int j = ud.coop;
		if (j == 2) j = 0;

		if (ud.multimode < 2 || (ud.multimode > 1 && j != act->spr.lotag))
			ChangeActorStat(act, STAT_MISC);
		else
			ChangeActorStat(act, STAT_PLAYER);
		break;
	}
	}
	return act;
}

END_DUKE_NS
