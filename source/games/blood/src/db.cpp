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
#include "common_game.h"
#include "zstring.h"
#include "m_crc32.h"
#include "md4.h"
#include "automap.h"
#include "raze_sound.h"
#include "gamefuncs.h"
#include "hw_sections.h"
#include "sectorgeometry.h"

#include "blood.h"

BEGIN_BLD_NS

DBloodActor bloodActors[kMaxSprites];


bool gModernMap = false;
unsigned int gStatCount[kMaxStatus + 1];

int gVisibility;

void dbCrypt(char *pPtr, int nLength, int nKey)
{
    for (int i = 0; i < nLength; i++)
    {
        pPtr[i] = pPtr[i] ^ nKey;
        nKey++;
    }
}

void InsertSpriteSect(int nSprite, int nSector)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(validSectorIndex(nSector));
    int nOther = headspritesect[nSector];
    if (nOther >= 0)
    {
        prevspritesect[nSprite] = prevspritesect[nOther];
        nextspritesect[nSprite] = -1;
        nextspritesect[prevspritesect[nOther]] = nSprite;
        prevspritesect[nOther] = nSprite;
    }
    else
    {
        prevspritesect[nSprite] = nSprite;
        nextspritesect[nSprite] = -1;
        headspritesect[nSector] = nSprite;
    }
    sprite[nSprite].sectnum = nSector;
}

void RemoveSpriteSect(int nSprite)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    int nSector = sprite[nSprite].sectnum;
    assert(validSectorIndex(nSector));
    int nOther = nextspritesect[nSprite];
    if (nOther < 0)
    {
        nOther = headspritesect[nSector];
    }
    prevspritesect[nOther] = prevspritesect[nSprite];
    if (headspritesect[nSector] != nSprite)
    {
        nextspritesect[prevspritesect[nSprite]] = nextspritesect[nSprite];
    }
    else
    {
        headspritesect[nSector] = nextspritesect[nSprite];
    }
    sprite[nSprite].sectnum = MAXSECTORS;
}

void InsertSpriteStat(int nSprite, int nStat)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(nStat >= 0 && nStat <= kMaxStatus);
    int nOther = headspritestat[nStat];
    if (nOther >= 0)
    {
        prevspritestat[nSprite] = prevspritestat[nOther];
        nextspritestat[nSprite] = -1;
        nextspritestat[prevspritestat[nOther]] = nSprite;
        prevspritestat[nOther] = nSprite;
    }
    else
    {
        prevspritestat[nSprite] = nSprite;
        nextspritestat[nSprite] = -1;
        headspritestat[nStat] = nSprite;
    }
    sprite[nSprite].statnum = nStat;
    gStatCount[nStat]++;
}

void RemoveSpriteStat(int nSprite)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    int nStat = sprite[nSprite].statnum;
    assert(nStat >= 0 && nStat <= kMaxStatus);
    int nOther = nextspritestat[nSprite];
    if (nOther < 0)
    {
        nOther = headspritestat[nStat];
    }
    prevspritestat[nOther] = prevspritestat[nSprite];
    if (headspritestat[nStat] != nSprite)
    {
        nextspritestat[prevspritestat[nSprite]] = nextspritestat[nSprite];
    }
    else
    {
        headspritestat[nStat] = nextspritestat[nSprite];
    }
    sprite[nSprite].statnum = MAXSTATUS;
    gStatCount[nStat]--;
}

void qinitspritelists(void) // Replace
{
    for (int i = 0; i <= MAXSECTORS; i++)
    {
        headspritesect[i] = -1;
    }
    for (int i = 0; i <= kMaxStatus; i++)
    {
        headspritestat[i] = -1;
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        sprite[i].sectnum = -1;
        InsertSpriteStat(i, kMaxStatus);
    }
    memset(gStatCount, 0, sizeof(gStatCount));
    Numsprites = 0;
}

DBloodActor* InsertSprite(sectortype* pSector, int nStat)
{
    int nSprite = headspritestat[kMaxStatus];
    assert(nSprite < kMaxSprites);
    if (nSprite < 0)
    {
        I_Error("Out of sprites!"); // we cannot deal with this - and most of the calling code never checks...
        return nullptr;
    }
    RemoveSpriteStat(nSprite);
    DBloodActor* actor = &bloodActors[nSprite];
    actor->Clear();
    spritetype *pSprite = &actor->s();
    memset(pSprite, 0, sizeof(spritetype));
    InsertSpriteStat(nSprite, nStat);
    InsertSpriteSect(nSprite, sectnum(pSector));
    pSprite->cstat = 128;
    pSprite->clipdist = 32;
    pSprite->xrepeat = pSprite->yrepeat = 64;
    actor->SetOwner(nullptr);

    Numsprites++;

    sprite[nSprite].time = leveltimer++;
    return actor;
}


inline int DeleteSprite(DBloodActor* actor)
{
    auto sp = &actor->s();
    FVector3 pos = GetSoundPos(&actor->s().pos);
    soundEngine->RelinkSound(SOURCE_Actor, actor, nullptr, &pos);

    assert(sp->statnum >= 0 && sp->statnum < kMaxStatus);
    RemoveSpriteStat(actor->GetSpriteIndex());
    assert(sp->insector());
    RemoveSpriteSect(actor->GetSpriteIndex());
    InsertSpriteStat(actor->GetSpriteIndex(), kMaxStatus);
#ifdef NOONE_EXTENSIONS
    for (auto& ctrl : gPlayerCtrl) if (ctrl.qavScene.initiator == actor) ctrl.qavScene.initiator = nullptr;
#endif
    Numsprites--;

    return 0;
}

int ChangeSpriteSect(int nSprite, int nSector)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(validSectorIndex(nSector));
	assert(sprite[nSprite].insector());
    RemoveSpriteSect(nSprite);
    InsertSpriteSect(nSprite, nSector);
    return 0;
}

int qchangespritesect(short nSprite, short nSector)
{
    return ChangeSpriteSect(nSprite, nSector);
}

int ChangeSpriteStat(int nSprite, int nStatus)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(nStatus >= 0 && nStatus < kMaxStatus);
    assert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    assert(sprite[nSprite].insector());
    RemoveSpriteStat(nSprite);
    InsertSpriteStat(nSprite, nStatus);
    return 0;
}

void InitFreeList(unsigned short* pList, int nCount)
{
    for (int i = 1; i < nCount; i++)
    {
        pList[i] = i - 1;
    }
    pList[0] = nCount - 1;
}


void dbInit(void)
{
    initspritelists();
    for (int i = 0; i < kMaxSprites; i++)
    {
        sprite[i].cstat = 128;
    }
}

void PropagateMarkerReferences(void)
{
    BloodStatIterator it(kStatMarker);
    while (auto actor = it.Next())
    {
        switch (actor->s().type)  
        {
            case kMarkerOff:
            case kMarkerAxis:
            case kMarkerWarpDest: 
            {
                int nOwner = actor->s().owner;
                if (nOwner >= 0 && nOwner < numsectors) 
                {
                    if (sector[nOwner].hasX())
                    {
                        sector[nOwner].xs().marker0 = actor;
                        continue;
                    }
                }
            }
            break;
            case kMarkerOn: 
            {
                int nOwner = actor->s().owner;
                if (nOwner >= 0 && nOwner < numsectors)
                {
                    if (sector[nOwner].hasX())
                    {
                        sector[nOwner].xs().marker1 = actor;
                        continue;
                    }
                }
            }
            break;
        }
        
        DeleteSprite(actor);
    }
}

bool drawtile2048, encrypted;

MAPHEADER2 byte_19AE44;

unsigned int dbReadMapCRC(const char *pPath)
{
    encrypted = 0;

    FString mapname = pPath;
    DefaultExtension(mapname, ".map");
    auto fr = fileSystem.OpenFileReader(mapname);

    if (!fr.isOpen())
    {
        Printf("Error opening map file %s", pPath);
        return -1;
    }

    MAPSIGNATURE header;
    fr.Read(&header, 6);
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        I_Error("%s: Map file corrupted.", mapname.GetChars());
    }
    int ver = LittleShort(header.version);
    if ((ver & 0xff00) == 0x600)
    {
    }
    else if ((ver & 0xff00) == 0x700)
    {
        encrypted = 1;
    }
    else
    {
        I_Error("%s: Map file is wrong version.", mapname.GetChars());
    }
    fr.Seek(-4, FileReader::SeekEnd);
    return fr.ReadInt32();
}

int gMapRev, gMattId, gSkyCount;
//char byte_19AE44[128];
const int nXSectorSize = 60;
const int nXSpriteSize = 56;
const int nXWallSize = 24;


#pragma pack(push, 1)
// This is the on-disk format. Only Blood still needs this for its retarded encryption that has to read this in as a block so that it can be decoded.
// Keep it local so that the engine's sprite type is no longer limited by file format restrictions.
struct spritetypedisk
{
    int32_t x, y, z;
    uint16_t cstat;
    int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, detail;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner;
    int16_t index, yvel, inittype;
    int16_t type;
    int16_t hitag;
    int16_t extra;
};

struct sectortypedisk
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, fogpal;
    int16_t type;
    int16_t hitag;
    int16_t extra;
};

struct walltypedisk
{
    int32_t x, y;
    int16_t point2, nextwall, nextsector;
    uint16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t type;
    int16_t hitag;
    int16_t extra;
};

#pragma pack(pop)


void dbLoadMap(const char* pPath, int* pX, int* pY, int* pZ, short* pAngle, sectortype** pSector, unsigned int* pCRC)
{
    int16_t tpskyoff[256];
    ClearAutomap();
#ifdef NOONE_EXTENSIONS
    gModernMap = false;
#endif

#ifdef NOONE_EXTENSIONS
    for (auto& ctrl : gPlayerCtrl) ctrl.qavScene.initiator = nullptr;
#endif


#ifdef USE_OPENGL
    Polymost::Polymost_prepare_loadboard();
#endif

    FString mapname = pPath;
    DefaultExtension(mapname, ".map");
    auto fr = fileSystem.OpenFileReader(mapname);

    if (!fr.isOpen())
    {
        I_Error("Error opening map file %s", mapname.GetChars());
    }
    MAPSIGNATURE header;
    fr.Read(&header, 6);
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        I_Error("%s: Map file corrupted", mapname.GetChars());
    }
    encrypted = 0;
    if ((LittleShort(header.version) & 0xff00) == 0x700) {
        encrypted = 1;

#ifdef NOONE_EXTENSIONS
        // indicate if the map requires modern features to work properly
        // for maps wich created in PMAPEDIT BETA13 or higher versions. Since only minor version changed,
        // the map is still can be loaded with vanilla BLOOD / MAPEDIT and should work in other ports too.
        if ((header.version & 0x00ff) == 0x001) gModernMap = true;
#endif

    }
    else {
        I_Error("%s: Map file is wrong version", mapname.GetChars());
    }

    MAPHEADER mapHeader;
    fr.Read(&mapHeader, 37/* sizeof(mapHeader)*/);
    if (mapHeader.mattid != 0 && mapHeader.mattid != 0x7474614d && mapHeader.mattid != 0x4d617474) {
        dbCrypt((char*)&mapHeader, sizeof(mapHeader), 0x7474614d);
    }

    mapHeader.x = LittleLong(mapHeader.x);
    mapHeader.y = LittleLong(mapHeader.y);
    mapHeader.z = LittleLong(mapHeader.z);
    mapHeader.ang = LittleShort(mapHeader.ang);
    mapHeader.sect = LittleShort(mapHeader.sect);
    mapHeader.pskybits = LittleShort(mapHeader.pskybits);
    mapHeader.visibility = LittleLong(mapHeader.visibility);
    mapHeader.mattid = LittleLong(mapHeader.mattid);
    mapHeader.revision = LittleLong(mapHeader.revision);
    mapHeader.numsectors = LittleShort(mapHeader.numsectors);
    mapHeader.numwalls = LittleShort(mapHeader.numwalls);
    mapHeader.numsprites = LittleShort(mapHeader.numsprites);

    *pX = mapHeader.x;
    *pY = mapHeader.y;
    *pZ = mapHeader.z;
    *pAngle = mapHeader.ang;
    gVisibility = g_visibility = mapHeader.visibility;
    gMattId = mapHeader.mattid;
    if (encrypted)
    {
        if (mapHeader.mattid == 0x7474614d || mapHeader.mattid == 0x4d617474)
        {
            drawtile2048 = 1;
        }
        else if (!mapHeader.mattid)
        {
            drawtile2048 = 0;
        }
        else
        {
            I_Error("%s: Corrupted Map file", mapname.GetChars());
        }
    }
    else if (mapHeader.mattid)
    {
        I_Error("%s: Corrupted Map file", mapname.GetChars());
    }
    parallaxtype = mapHeader.parallax;
    gMapRev = mapHeader.revision;
    numsectors = mapHeader.numsectors;
    numwalls = mapHeader.numwalls;
    allocateMapArrays(mapHeader.numsprites);
#if 1 // bad, bad hack, just for making Polymost happy...
	PolymostAllocFakeSector();
#endif
    * pSector = &sector[mapHeader.sect];

    dbInit();
    if (encrypted)
    {
        fr.Read(&byte_19AE44, 128);
        dbCrypt((char*)&byte_19AE44, 128, numwalls);

        byte_19AE44.numxsprites = LittleLong(byte_19AE44.numxsprites);
        byte_19AE44.numxwalls = LittleLong(byte_19AE44.numxwalls);
        byte_19AE44.numxsectors = LittleLong(byte_19AE44.numxsectors);
    }
    else
    {
        memset(&byte_19AE44, 0, 128);
    }
    gSkyCount = 1 << mapHeader.pskybits;
    fr.Read(tpskyoff, gSkyCount * sizeof(tpskyoff[0]));
    if (encrypted)
    {
        dbCrypt((char*)tpskyoff, gSkyCount * sizeof(tpskyoff[0]), gSkyCount * 2);
    }

    psky_t* pSky = tileSetupSky(DEFAULTPSKY);
    pSky->horizfrac = 65536;
    pSky->lognumtiles = mapHeader.pskybits;
    for (int i = 0; i < ClipHigh(gSkyCount, MAXPSKYTILES); i++)
    {
        pSky->tileofs[i] = LittleShort(tpskyoff[i]);
    }

    for (int i = 0; i < numsectors; i++)
    {
        sectortype* pSector = &sector[i];
        sectortypedisk load;
        fr.Read(&load, sizeof(sectortypedisk));
        if (encrypted)
        {
            dbCrypt((char*)&load, sizeof(sectortypedisk), gMapRev * sizeof(sectortypedisk));
        }
        pSector->wallptr = LittleShort(load.wallptr);
        pSector->wallnum = LittleShort(load.wallnum);
        pSector->ceilingz = LittleLong(load.ceilingz);
        pSector->floorz = LittleLong(load.floorz);
        pSector->ceilingstat = LittleShort(load.ceilingstat);
        pSector->floorstat = LittleShort(load.floorstat);
        pSector->ceilingpicnum = LittleShort(load.ceilingpicnum);
        pSector->ceilingheinum = LittleShort(load.ceilingheinum);
        pSector->floorpicnum = LittleShort(load.floorpicnum);
        pSector->floorheinum = LittleShort(load.floorheinum);
        pSector->type = LittleShort(load.type);
        pSector->hitag = LittleShort(load.hitag);
        pSector->extra = LittleShort(load.extra);
        pSector->ceilingshade = load.ceilingshade;
        pSector->ceilingpal = load.ceilingpal;
        pSector->ceilingxpan_ = load.ceilingxpanning;
        pSector->ceilingypan_ = load.ceilingypanning;
        pSector->floorshade = load.floorshade;
        pSector->floorpal = load.floorpal;
        pSector->floorxpan_ = load.floorxpanning;
        pSector->floorypan_ = load.floorypanning;
        pSector->visibility = load.visibility;
        pSector->slopewallofs = load.fogpal;
        pSector->dirty = 255;
        pSector->exflags = 0;
        pSector->fogpal = 0;

        if (pSector->extra > 0)
        {
            char pBuffer[nXSectorSize];
            pSector->allocX();
            XSECTOR* pXSector = &pSector->xs();
            int nCount;
            if (!encrypted)
            {
                nCount = nXSectorSize;
            }
            else
            {
                nCount = byte_19AE44.numxsectors;
            }
            assert(nCount <= nXSectorSize);
            fr.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            /*pXSector->reference =*/ bitReader.readSigned(14);
            pXSector->state = bitReader.readUnsigned(1);
            pXSector->busy = bitReader.readUnsigned(17);
            pXSector->data = bitReader.readUnsigned(16);
            pXSector->txID = bitReader.readUnsigned(10);
            pXSector->busyWaveA = bitReader.readUnsigned(3);
            pXSector->busyWaveB = bitReader.readUnsigned(3);
            pXSector->rxID = bitReader.readUnsigned(10);
            pXSector->command = bitReader.readUnsigned(8);
            pXSector->triggerOn = bitReader.readUnsigned(1);
            pXSector->triggerOff = bitReader.readUnsigned(1);
            pXSector->busyTimeA = bitReader.readUnsigned(12);
            pXSector->waitTimeA = bitReader.readUnsigned(12);
            pXSector->restState = bitReader.readUnsigned(1);
            pXSector->interruptable = bitReader.readUnsigned(1);
            pXSector->amplitude = bitReader.readSigned(8);
            pXSector->freq = bitReader.readUnsigned(8);
            pXSector->reTriggerA = bitReader.readUnsigned(1);
            pXSector->reTriggerB = bitReader.readUnsigned(1);
            pXSector->phase = bitReader.readUnsigned(8);
            pXSector->wave = bitReader.readUnsigned(4);
            pXSector->shadeAlways = bitReader.readUnsigned(1);
            pXSector->shadeFloor = bitReader.readUnsigned(1);
            pXSector->shadeCeiling = bitReader.readUnsigned(1);
            pXSector->shadeWalls = bitReader.readUnsigned(1);
            pXSector->shade = bitReader.readSigned(8);
            pXSector->panAlways = bitReader.readUnsigned(1);
            pXSector->panFloor = bitReader.readUnsigned(1);
            pXSector->panCeiling = bitReader.readUnsigned(1);
            pXSector->Drag = bitReader.readUnsigned(1);
            pXSector->Underwater = bitReader.readUnsigned(1);
            pXSector->Depth = bitReader.readUnsigned(3);
            pXSector->panVel = bitReader.readUnsigned(8);
            pXSector->panAngle = bitReader.readUnsigned(11);
            pXSector->unused1 = bitReader.readUnsigned(1);
            pXSector->decoupled = bitReader.readUnsigned(1);
            pXSector->triggerOnce = bitReader.readUnsigned(1);
            pXSector->isTriggered = bitReader.readUnsigned(1);
            pXSector->Key = bitReader.readUnsigned(3);
            pXSector->Push = bitReader.readUnsigned(1);
            pXSector->Vector = bitReader.readUnsigned(1);
            pXSector->Reserved = bitReader.readUnsigned(1);
            pXSector->Enter = bitReader.readUnsigned(1);
            pXSector->Exit = bitReader.readUnsigned(1);
            pXSector->Wallpush = bitReader.readUnsigned(1);
            pXSector->color = bitReader.readUnsigned(1);
            /*pXSector->unused2 =*/ bitReader.readUnsigned(1);
            pXSector->busyTimeB = bitReader.readUnsigned(12);
            pXSector->waitTimeB = bitReader.readUnsigned(12);
            pXSector->stopOn = bitReader.readUnsigned(1);
            pXSector->stopOff = bitReader.readUnsigned(1);
            pXSector->ceilpal = bitReader.readUnsigned(4);
            pXSector->offCeilZ = bitReader.readSigned(32);
            pXSector->onCeilZ = bitReader.readSigned(32);
            pXSector->offFloorZ = bitReader.readSigned(32);
            pXSector->onFloorZ = bitReader.readSigned(32);
            /*pXSector->marker0 =*/ bitReader.readUnsigned(16);
            /*pXSector->marker1 =*/ bitReader.readUnsigned(16);
            pXSector->Crush = bitReader.readUnsigned(1);
            pSector->ceilingxpan_ += bitReader.readUnsigned(8) / 256.f;
            pSector->ceilingypan_ += bitReader.readUnsigned(8) / 256.f;
            pSector->floorxpan_ += bitReader.readUnsigned(8) / 256.f;
            pXSector->damageType = bitReader.readUnsigned(3);
            pXSector->floorpal = bitReader.readUnsigned(4);
            pSector->floorypan_ += bitReader.readUnsigned(8) / 256.f;
            pXSector->locked = bitReader.readUnsigned(1);
            pXSector->windVel = bitReader.readUnsigned(10);
            pXSector->windAng = bitReader.readUnsigned(11);
            pXSector->windAlways = bitReader.readUnsigned(1);
            pXSector->dudeLockout = bitReader.readUnsigned(1);
            pXSector->bobTheta = bitReader.readUnsigned(11);
            pXSector->bobZRange = bitReader.readUnsigned(5);
            pXSector->bobSpeed = bitReader.readSigned(12);
            pXSector->bobAlways = bitReader.readUnsigned(1);
            pXSector->bobFloor = bitReader.readUnsigned(1);
            pXSector->bobCeiling = bitReader.readUnsigned(1);
            pXSector->bobRotate = bitReader.readUnsigned(1);
            pXSector->busy = IntToFixed(pXSector->state);

        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        walltype* pWall = &wall[i];
        walltypedisk load;
        fr.Read(&load, sizeof(walltypedisk));
        if (encrypted)
        {
            dbCrypt((char*)&load, sizeof(walltypedisk), (gMapRev * sizeof(sectortypedisk)) | 0x7474614d);
        }
        pWall->x = LittleLong(load.x);
        pWall->y = LittleLong(load.y);
        pWall->point2 = LittleShort(load.point2);
        pWall->nextwall = LittleShort(load.nextwall);
        pWall->nextsector = LittleShort(load.nextsector);
        pWall->cstat = LittleShort(load.cstat);
        pWall->picnum = LittleShort(load.picnum);
        pWall->overpicnum = LittleShort(load.overpicnum);
        pWall->type = LittleShort(load.type);
        pWall->hitag = LittleShort(load.hitag);
        pWall->extra = LittleShort(load.extra);
        pWall->shade = load.shade;
        pWall->pal = load.pal;
        pWall->xrepeat = load.xrepeat;
        pWall->xpan_ = load.xpanning;
        pWall->yrepeat = load.yrepeat;
        pWall->ypan_ = load.ypanning;

        if (pWall->extra > 0)
        {
            char pBuffer[nXWallSize];
            pWall->allocX();
            XWALL* pXWall = &pWall->xw();
            int nCount;
            if (!encrypted)
            {
                nCount = nXWallSize;
            }
            else
            {
                nCount = byte_19AE44.numxwalls;
            }
            assert(nCount <= nXWallSize);
            fr.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            /*pXWall->reference =*/ bitReader.readSigned(14);
            pXWall->state = bitReader.readUnsigned(1);
            pXWall->busy = bitReader.readUnsigned(17);
            pXWall->data = bitReader.readSigned(16);
            pXWall->txID = bitReader.readUnsigned(10);
            bitReader.readUnsigned(6);
            pXWall->rxID = bitReader.readUnsigned(10);
            pXWall->command = bitReader.readUnsigned(8);
            pXWall->triggerOn = bitReader.readUnsigned(1);
            pXWall->triggerOff = bitReader.readUnsigned(1);
            pXWall->busyTime = bitReader.readUnsigned(12);
            pXWall->waitTime = bitReader.readUnsigned(12);
            pXWall->restState = bitReader.readUnsigned(1);
            pXWall->interruptable = bitReader.readUnsigned(1);
            pXWall->panAlways = bitReader.readUnsigned(1);
            pXWall->panXVel = bitReader.readSigned(8);
            pXWall->panYVel = bitReader.readSigned(8);
            pXWall->decoupled = bitReader.readUnsigned(1);
            pXWall->triggerOnce = bitReader.readUnsigned(1);
            pXWall->isTriggered = bitReader.readUnsigned(1);
            pXWall->key = bitReader.readUnsigned(3);
            pXWall->triggerPush = bitReader.readUnsigned(1);
            pXWall->triggerVector = bitReader.readUnsigned(1);
            pXWall->triggerTouch = bitReader.readUnsigned(1);
            bitReader.readUnsigned(2);
            pWall->xpan_ += bitReader.readUnsigned(8) / 256.f;
            pWall->ypan_ += bitReader.readUnsigned(8) / 256.f;
            pXWall->locked = bitReader.readUnsigned(1);
            pXWall->dudeLockout = bitReader.readUnsigned(1);
            bitReader.readUnsigned(4);
            bitReader.readUnsigned(32);
            pXWall->busy = IntToFixed(pXWall->state);

        }
    }
    initspritelists();
    leveltimer = mapHeader.numsprites;
    for (int i = 0; i < mapHeader.numsprites; i++)
    {
        RemoveSpriteStat(i);
        spritetypedisk load;
        auto actor = &bloodActors[i];
        actor->Clear();
        spritetype* pSprite = &actor->s();
        fr.Read(&load, sizeof(spritetypedisk)); // load into an intermediate buffer so that spritetype is no longer bound by file formats.
        if (encrypted) // What were these people thinking? :(
        {
            dbCrypt((char*)&load, sizeof(spritetypedisk), (gMapRev * sizeof(spritetypedisk)) | 0x7474614d);
        }

        pSprite->x = LittleLong(load.x);
        pSprite->y = LittleLong(load.y);
        pSprite->z = LittleLong(load.z);
        pSprite->cstat = LittleShort(load.cstat);
        pSprite->picnum = LittleShort(load.picnum);
        pSprite->sectnum = LittleShort(load.sectnum);
        pSprite->statnum = LittleShort(load.statnum);
        pSprite->ang = LittleShort(load.ang);
        pSprite->owner = LittleShort(load.owner);
        pSprite->xvel = LittleShort(load.index);
        pSprite->yvel = LittleShort(load.yvel);
        pSprite->inittype = LittleShort(load.inittype);
        pSprite->type = LittleShort(load.type);
        pSprite->flags = LittleShort(load.hitag);
        pSprite->extra = LittleShort(load.extra);
        pSprite->pal = load.pal;
        pSprite->clipdist = load.clipdist;
        pSprite->xrepeat = load.xrepeat;
        pSprite->yrepeat = load.yrepeat;
        pSprite->xoffset = load.xoffset;
        pSprite->yoffset = load.yoffset;
        pSprite->detail = load.detail;
        pSprite->shade = load.shade;
        pSprite->blend = 0;
        pSprite->time = i;
        ValidateSprite(*pSprite);

        InsertSpriteSect(i, pSprite->sectnum);
        InsertSpriteStat(i, pSprite->statnum);
        Numsprites++;
        if (pSprite->extra > 0)
        {
            char pBuffer[nXSpriteSize];
            actor->addX();
            XSPRITE* pXSprite = &actor->x();
            int nCount;
            if (!encrypted)
            {
                nCount = nXSpriteSize;
            }
            else
            {
                nCount = byte_19AE44.numxsprites;
            }
            assert(nCount <= nXSpriteSize);
            fr.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            /*pXSprite->reference =*/ bitReader.readSigned(14);
            pXSprite->state = bitReader.readUnsigned(1);
            pXSprite->busy = bitReader.readUnsigned(17);
            pXSprite->txID = bitReader.readUnsigned(10);
            pXSprite->rxID = bitReader.readUnsigned(10);
            pXSprite->command = bitReader.readUnsigned(8);
            pXSprite->triggerOn = bitReader.readUnsigned(1);
            pXSprite->triggerOff = bitReader.readUnsigned(1);
            pXSprite->wave = bitReader.readUnsigned(2);
            pXSprite->busyTime = bitReader.readUnsigned(12);
            pXSprite->waitTime = bitReader.readUnsigned(12);
            pXSprite->restState = bitReader.readUnsigned(1);
            pXSprite->Interrutable = bitReader.readUnsigned(1);
            pXSprite->unused1 = bitReader.readUnsigned(2);
            pXSprite->respawnPending = bitReader.readUnsigned(2);
            pXSprite->unused2 = bitReader.readUnsigned(1);
            pXSprite->lT = bitReader.readUnsigned(1);
            pXSprite->dropMsg = bitReader.readUnsigned(8);
            pXSprite->Decoupled = bitReader.readUnsigned(1);
            pXSprite->triggerOnce = bitReader.readUnsigned(1);
            pXSprite->isTriggered = bitReader.readUnsigned(1);
            pXSprite->key = bitReader.readUnsigned(3);
            pXSprite->Push = bitReader.readUnsigned(1);
            pXSprite->Vector = bitReader.readUnsigned(1);
            pXSprite->Impact = bitReader.readUnsigned(1);
            pXSprite->Pickup = bitReader.readUnsigned(1);
            pXSprite->Touch = bitReader.readUnsigned(1);
            pXSprite->Sight = bitReader.readUnsigned(1);
            pXSprite->Proximity = bitReader.readUnsigned(1);
            pXSprite->unused3 = bitReader.readUnsigned(2);
            pXSprite->lSkill = bitReader.readUnsigned(5);
            pXSprite->lS = bitReader.readUnsigned(1);
            pXSprite->lB = bitReader.readUnsigned(1);
            pXSprite->lC = bitReader.readUnsigned(1);
            pXSprite->DudeLockout = bitReader.readUnsigned(1);
            pXSprite->data1 = bitReader.readSigned(16);
            pXSprite->data2 = bitReader.readSigned(16);
            pXSprite->data3 = bitReader.readSigned(16);
            pXSprite->goalAng = bitReader.readUnsigned(11);
            pXSprite->dodgeDir = bitReader.readSigned(2);
            pXSprite->locked = bitReader.readUnsigned(1);
            pXSprite->medium = bitReader.readUnsigned(2);
            pXSprite->respawn = bitReader.readUnsigned(2);
            pXSprite->data4 = bitReader.readUnsigned(16);
            pXSprite->unused4 = bitReader.readUnsigned(6);
            pXSprite->lockMsg = bitReader.readUnsigned(8);
            pXSprite->health = bitReader.readUnsigned(12);
            pXSprite->dudeDeaf = bitReader.readUnsigned(1);
            pXSprite->dudeAmbush = bitReader.readUnsigned(1);
            pXSprite->dudeGuard = bitReader.readUnsigned(1);
            pXSprite->dudeFlag4 = bitReader.readUnsigned(1);
            /*pXSprite->target_i = */ bitReader.readSigned(16);
            pXSprite->targetX = bitReader.readSigned(32);
            pXSprite->targetY = bitReader.readSigned(32);
            pXSprite->targetZ = bitReader.readSigned(32);
            pXSprite->burnTime = bitReader.readUnsigned(16);
            /*pXSprite->burnSource =*/ bitReader.readSigned(16);
            pXSprite->height = bitReader.readUnsigned(16);
            pXSprite->stateTimer = bitReader.readUnsigned(16);
            pXSprite->aiState = NULL;
            bitReader.skipBits(32);
            pXSprite->busy = IntToFixed(pXSprite->state);
            if (!encrypted) {
                pXSprite->lT |= pXSprite->lB;
            }

#ifdef NOONE_EXTENSIONS
            // indicate if the map requires modern features to work properly
            // for maps wich created in different editors (include vanilla MAPEDIT) or in PMAPEDIT version below than BETA13
            if (!gModernMap && pXSprite->rxID == kChannelMapModernize && pXSprite->rxID == pXSprite->txID && pXSprite->command == kCmdModernFeaturesEnable)
                gModernMap = true;
#endif
        }
        if ((sprite[i].cstat & 0x30) == 0x30)
        {
            sprite[i].cstat &= ~0x30;
        }
    }
    unsigned int nCRC = fr.ReadUInt32();

    fr.Seek(0, FileReader::SeekSet);
    auto buffer = fr.Read();
    uint8_t md4[16];
    md4once(buffer.Data(), buffer.Size(), md4);
    G_LoadMapHack(mapname, md4);

    if (CalcCRC32(buffer.Data(), buffer.Size() - 4) != nCRC)
    {
        I_Error("%s: Map File does not match CRC", mapname.GetChars());
    }
    if (pCRC)
        *pCRC = nCRC;
    PropagateMarkerReferences();
    if (encrypted)
    {
        if (gMattId == 0x7474614d || gMattId == 0x4d617474)
        {
            drawtile2048 = 1;
        }
        else if (!gMattId)
        {
            drawtile2048 = 0;
        }
        else
        {
            I_Error("%s: Corrupted Map file", mapname.GetChars());
        }
    }
    else if (gMattId != 0)
    {
        I_Error("%s: Corrupted Map file", mapname.GetChars());
    }

    if ((header.version & 0xff00) == 0x600)
    {
        switch (header.version & 0xff)
        {
        case 0:
            for (int i = 0; i < numsectors; i++)
            {
                sectortype* pSector = &sector[i];
                if (pSector->hasX())
                {
                    XSECTOR* pXSector = &pSector->xs();
                    pXSector->busyTimeB = pXSector->busyTimeA;
                    if (pXSector->busyTimeA > 0)
                    {
                        if (!pXSector->restState)
                        {
                            pXSector->reTriggerA = 1;
                        }
                        else
                        {
                            pXSector->waitTimeB = pXSector->busyTimeA;
                            pXSector->waitTimeA = 0;
                            pXSector->reTriggerB = 1;
                        }
                    }
                }
            }
            [[fallthrough]];
        case 1:
            for (int i = 0; i < numsectors; i++)
            {
                sectortype* pSector = &sector[i];
                if (pSector->hasX())
                {
                    XSECTOR* pXSector = &pSector->xs();
                    pXSector->freq >>= 1;
                }
            }
            [[fallthrough]];
        case 2:
            for (int i = 0; i < kMaxSprites; i++)
            {
            }
            break;

        }
    }

    setWallSectors();
    hw_BuildSections();
    sectorGeometry.SetSize(numsections);
    wallbackup = wall;
    sectorbackup = sector;
}


END_BLD_NS

// only used by the backup loader.
void qloadboard(const char* filename, char flags, vec3_t* dapos, int16_t* daang)
{
    sectortype* sp;
    Blood::dbLoadMap(filename, &dapos->x, &dapos->y, &dapos->z, daang, &sp, NULL);
    Blood::dbInit();    // clean up immediately.
}
    