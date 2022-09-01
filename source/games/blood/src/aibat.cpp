//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "build.h"

#include "blood.h"

BEGIN_BLD_NS

static void batThinkTarget(DBloodActor*);
static void batThinkSearch(DBloodActor*);
static void batThinkGoto(DBloodActor*);
static void batThinkPonder(DBloodActor*);
static void batMoveDodgeUp(DBloodActor*);
static void batMoveDodgeDown(DBloodActor*);
static void batThinkChase(DBloodActor*);
static void batMoveForward(DBloodActor*);
static void batMoveSwoop(DBloodActor*);
static void batMoveFly(DBloodActor*);
static void batMoveToCeil(DBloodActor*);


AISTATE batIdle = { kAiStateIdle, 0, -1, 0, NULL, NULL, batThinkTarget, NULL };
AISTATE batFlyIdle = { kAiStateIdle, 6, -1, 0, NULL, NULL, batThinkTarget, NULL };
AISTATE batChase = { kAiStateChase, 6, -1, 0, NULL, batMoveForward, batThinkChase, &batFlyIdle };
AISTATE batPonder = { kAiStateOther, 6, -1, 0, NULL, NULL, batThinkPonder, NULL };
AISTATE batGoto = { kAiStateMove, 6, -1, 600, NULL, batMoveForward, batThinkGoto, &batFlyIdle };
AISTATE batBite = { kAiStateChase, 7, nBatBiteClient, 60, NULL, NULL, NULL, &batPonder };
AISTATE batRecoil = { kAiStateRecoil, 5, -1, 0, NULL, NULL, NULL, &batChase };
AISTATE batSearch = { kAiStateSearch, 6, -1, 120, NULL, batMoveForward, batThinkSearch, &batFlyIdle };
AISTATE batSwoop = { kAiStateOther, 6, -1, 60, NULL, batMoveSwoop, batThinkChase, &batChase };
AISTATE batFly = { kAiStateMove, 6, -1, 0, NULL, batMoveFly, batThinkChase, &batChase };
AISTATE batTurn = { kAiStateMove, 6, -1, 60, NULL, aiMoveTurn, NULL, &batChase };
AISTATE batHide = { kAiStateOther, 6, -1, 0, NULL, batMoveToCeil, batMoveForward, NULL };
AISTATE batDodgeUp = { kAiStateMove, 6, -1, 120, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpRight = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeUpLeft = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeUp, 0, &batChase };
AISTATE batDodgeDown = { kAiStateMove, 6, -1, 120, NULL, batMoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownRight = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeDown, 0, &batChase };
AISTATE batDodgeDownLeft = { kAiStateMove, 6, -1, 90, NULL, batMoveDodgeDown, 0, &batChase };

void batBiteSeqCallback(int, DBloodActor* actor)
{
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	int dx = bcos(actor->int_ang());
	int dy = bsin(actor->int_ang());
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEINFO* pDudeInfoT = getDudeInfo(pTarget->spr.type);
	int height = (actor->spr.yrepeat * pDudeInfo->eyeHeight) << 2;
	int height2 = (pTarget->spr.yrepeat * pDudeInfoT->eyeHeight) << 2;
	actFireVector(actor, 0, 0, dx, dy, height2 - height, kVectorBatBite);
}

static void batThinkTarget(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
	if (pDudeExtraE->active && pDudeExtraE->thinkTime < 10)
		pDudeExtraE->thinkTime++;
	else if (pDudeExtraE->thinkTime >= 10 && pDudeExtraE->active)
	{
		pDudeExtraE->thinkTime = 0;
		actor->xspr.goalAng += 256;
		aiSetTarget(actor, actor->basePoint);
		aiNewState(actor, &batTurn);
		return;
	}
	if (Chance(pDudeInfo->alertChance))
	{
		for (int p = connecthead; p >= 0; p = connectpoint2[p])
		{
			PLAYER* pPlayer = &gPlayer[p];
			if (pPlayer->actor->xspr.health == 0 || powerupCheck(pPlayer, kPwUpShadowCloak) > 0)
				continue;
			auto ppos = pPlayer->actor->spr.pos;
			auto dvec = ppos - actor->spr.pos.XY();
			auto pSector = pPlayer->actor->sector();

			int nDist = approxDist(dvec);
			if (nDist > pDudeInfo->seeDist && nDist > pDudeInfo->hearDist)
				continue;
			double height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) * REPEAT_SCALE;
			if (!cansee(ppos, pSector, actor->spr.pos.plusZ(-height), actor->sector()))
				continue;
			int nDeltaAngle = getincangle(actor->int_ang(), getangle(dvec));
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, pPlayer->actor);
				aiActivateDude(actor);
			}
			else if (nDist < pDudeInfo->hearDist)
			{
				aiSetTarget(actor, ppos);
				aiActivateDude(actor);
			}
			else
				continue;
			break;
		}
	}
}

static void batThinkSearch(DBloodActor* actor)
{
	aiChooseDirection(actor, actor->xspr.goalAng);
	batThinkTarget(actor);
}

static void batThinkGoto(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	int nAngle = getangle(dvec);
	int nDist = approxDist(dvec);
	aiChooseDirection(actor, nAngle);
	if (nDist < 512 && abs(actor->int_ang() - nAngle) < pDudeInfo->periphery)
		aiNewState(actor, &batSearch);
	batThinkTarget(actor);
}

static void batThinkPonder(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	auto dvec = pTarget->spr.pos.XY() - actor->spr.pos.XY();
	aiChooseDirection(actor, getangle(dvec));
	if (pTarget->xspr.health == 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	int nDist = approxDist(dvec);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), getangle(dvec));
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		int height2 = (getDudeInfo(pTarget->spr.type)->eyeHeight * pTarget->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->spr.pos, pTarget->sector(), actor->spr.pos.plusZ(-height * zinttoworld), actor->sector()))
		{
			aiSetTarget(actor, actor->GetTarget());
			if (height2 - height < 0x3000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x5000 && nDist < 0x1800 && nDist > 0xc00 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeDown);
			else if (height2 - height < 0x2000 && nDist < 0x200 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x6000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeDown);
			else if (height2 - height < 0x2000 && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height < 0x2000 && abs(nDeltaAngle) < 85 && nDist > 0x1400)
				aiNewState(actor, &batDodgeUp);
			else if (height2 - height > 0x4000)
				aiNewState(actor, &batDodgeDown);
			else
				aiNewState(actor, &batDodgeUp);
			return;
		}
	}
	aiNewState(actor, &batGoto);
	actor->SetTarget(nullptr);
}

static void batMoveDodgeUp(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, DAngle::fromBuild(actor->xspr.goalAng));
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int dx = actor->__int_vel.X;
	int dy = actor->__int_vel.Y;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (actor->xspr.dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

	actor->set_int_bvel_x(DMulScale(t1, nCos, t2, nSin, 30));
	actor->__int_vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->__int_vel.Z = -0x52aaa;
}

static void batMoveDodgeDown(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, DAngle::fromBuild(actor->xspr.goalAng));
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	if (actor->xspr.dodgeDir == 0)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int dx = actor->__int_vel.X;
	int dy = actor->__int_vel.Y;
	int t1 = DMulScale(dx, nCos, dy, nSin, 30);
	int t2 = DMulScale(dx, nSin, -dy, nCos, 30);
	if (actor->xspr.dodgeDir > 0)
		t2 += pDudeInfo->sideSpeed;
	else
		t2 -= pDudeInfo->sideSpeed;

	actor->set_int_bvel_x(DMulScale(t1, nCos, t2, nSin, 30));
	actor->__int_vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->__int_vel.Z = 0x44444;
}

static void batThinkChase(DBloodActor* actor)
{
	if (actor->GetTarget() == nullptr)
	{
		aiNewState(actor, &batGoto);
		return;
	}
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	if (!actor->ValidateTarget(__FUNCTION__)) return;
	auto pTarget = actor->GetTarget();
	auto dvec = pTarget->spr.pos.XY() - actor->spr.pos.XY();

	aiChooseDirection(actor, getangle(dvec));
	if (pTarget->xspr.health == 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	if (pTarget->IsPlayerActor() && powerupCheck(&gPlayer[pTarget->spr.type - kDudePlayer1], kPwUpShadowCloak) > 0)
	{
		aiNewState(actor, &batSearch);
		return;
	}
	int nDist = approxDist(dvec);
	if (nDist <= pDudeInfo->seeDist)
	{
		int nDeltaAngle = getincangle(actor->int_ang(), getangle(dvec));
		int height = (pDudeInfo->eyeHeight * actor->spr.yrepeat) << 2;
		// Should be dudeInfo[pTarget->spr.type-kDudeBase]
		int height2 = (pDudeInfo->eyeHeight * pTarget->spr.yrepeat) << 2;
		int top, bottom;
		GetActorExtents(actor, &top, &bottom);
		if (cansee(pTarget->spr.pos, pTarget->sector(), actor->spr.pos.plusZ(-height * zinttoworld), actor->sector()))
		{
			if (nDist < pDudeInfo->seeDist && abs(nDeltaAngle) <= pDudeInfo->periphery)
			{
				aiSetTarget(actor, actor->GetTarget());
				int floorZ = getflorzofslopeptr(actor->sector(), actor->spr.pos);
				if (height2 - height < 0x2000 && nDist < 0x200 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &batBite);
				else if ((height2 - height > 0x5000 || floorZ - bottom > 0x5000) && nDist < 0x1400 && nDist > 0x800 && abs(nDeltaAngle) < 85)
					aiNewState(actor, &batSwoop);
				else if ((height2 - height < 0x3000 || floorZ - bottom < 0x3000) && abs(nDeltaAngle) < 85)
					aiNewState(actor, &batFly);
				return;
			}
		}
		else
		{
			aiNewState(actor, &batFly);
			return;
		}
	}

	actor->SetTarget(nullptr);
	aiNewState(actor, &batHide);
}

static void batMoveForward(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, DAngle::fromBuild(actor->xspr.goalAng));
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > DAngle60)
		return;
	if (actor->GetTarget() == nullptr)
		actor->spr.angle += DAngle45;
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	int nDist = approxDist(dvec);
	if ((unsigned int)Random(64) < 32 && nDist <= 0x200)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->__int_vel.X;
	int vy = actor->__int_vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	if (actor->GetTarget() == nullptr)
		t1 += nAccel;
	else
		t1 += nAccel >> 1;
	actor->set_int_bvel_x(DMulScale(t1, nCos, t2, nSin, 30));
	actor->__int_vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
}

static void batMoveSwoop(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, DAngle::fromBuild(actor->xspr.goalAng));
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > DAngle60)
	{
		actor->xspr.goalAng = (actor->int_ang() + 512) & 2047;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	int nDist = approxDist(dvec);
	if (Chance(0x600) && nDist <= 0x200)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->__int_vel.X;
	int vy = actor->__int_vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->set_int_bvel_x(DMulScale(t1, nCos, t2, nSin, 30));
	actor->__int_vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->__int_vel.Z = 0x44444;
}

static void batMoveFly(DBloodActor* actor)
{
	assert(actor->spr.type >= kDudeBase && actor->spr.type < kDudeMax);
	DUDEINFO* pDudeInfo = getDudeInfo(actor->spr.type);
	auto nAng = deltaangle(actor->spr.angle, DAngle::fromBuild(actor->xspr.goalAng));
	auto nTurnRange = DAngle::fromQ16(pDudeInfo->angSpeed << 3);
	actor->spr.angle += clamp(nAng, -nTurnRange, nTurnRange);
	int nAccel = pDudeInfo->frontSpeed << 2;
	if (abs(nAng) > DAngle60)
	{
		actor->spr.angle += DAngle90;
		return;
	}
	auto dvec = actor->xspr.TargetPos.XY() - actor->spr.pos.X;
	int nDist = approxDist(dvec);
	if (Chance(0x4000) && nDist <= 0x200)
		return;
	int nCos = Cos(actor->int_ang());
	int nSin = Sin(actor->int_ang());
	int vx = actor->__int_vel.X;
	int vy = actor->__int_vel.Y;
	int t1 = DMulScale(vx, nCos, vy, nSin, 30);
	int t2 = DMulScale(vx, nSin, -vy, nCos, 30);
	t1 += nAccel >> 1;
	actor->set_int_bvel_x(DMulScale(t1, nCos, t2, nSin, 30));
	actor->__int_vel.Y = DMulScale(t1, nSin, -t2, nCos, 30);
	actor->__int_vel.Z = -0x2d555;
}

void batMoveToCeil(DBloodActor* actor)
{
	if (actor->spr.pos.Z - actor->xspr.TargetPos.Z < 0x10)
	{
		DUDEEXTRA_STATS* pDudeExtraE = &actor->dudeExtra.stats;
		pDudeExtraE->thinkTime = 0;
		actor->spr.flags = 0;
		aiNewState(actor, &batIdle);
	}
	else
		aiSetTarget(actor, DVector3(actor->spr.pos.XY(), actor->sector()->ceilingz));
}

END_BLD_NS
