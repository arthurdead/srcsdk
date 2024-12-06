//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#ifndef ENGINECALLBACK_H
#define ENGINECALLBACK_H

#pragma once

#include "eiface.h"

class IFileSystem;				// include filesystem.h
class IUniformRandomStream;		// include vstdlib/random.h
class IEngineSound;				// include engine/IEngineSound.h
class IVEngineServer;			
class IVoiceServer;
class IStaticPropMgrServer;
class ISpatialPartition;
class IVModelInfo;
class IEngineTrace;
class IGameEventManager2;
class IDataCache;
class IMDLCache;
#ifndef SWDS
class IServerFoundry;
class IServerEngineTools;
#endif
#ifndef SWDS
class IGameLoopback;
class IGameServerLoopback;
class IGameClientLoopback;
#endif

#ifndef SWDS
extern bool g_bTextMode;
extern bool g_bDedicatedServer;
#endif

extern IVEngineServer			*engine;
extern IVoiceServer				*g_pVoiceServer;
extern IStaticPropMgrServer		*staticpropmgr;
extern ISpatialPartition		*partition;
extern IEngineSound				*enginesound;
extern IVModelInfo				*modelinfo;
extern IEngineTrace				*enginetrace;
extern IGameEventManager2		*gameeventmanager;
#ifndef SWDS
extern IServerEngineTools		*serverenginetools;
extern IServerFoundry			*serverfoundry;
#endif
#ifndef SWDS
extern IGameLoopback* g_pGameLoopback;
extern IGameServerLoopback* g_pGameServerLoopback;
inline IGameServerLoopback* GetGameServerLoopback()
{ return g_pGameServerLoopback; }
extern IGameClientLoopback* g_pGameClientLoopback;
#endif

//-----------------------------------------------------------------------------
// Precaches a material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material index into a string
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nMaterialIndex );


//-----------------------------------------------------------------------------
// Precache-related methods for particle systems
//-----------------------------------------------------------------------------
int PrecacheParticleSystem( const char *pParticleSystemName );
int GetParticleSystemIndex( const char *pParticleSystemName );
const char *GetParticleSystemNameFromIndex( int nIndex );

//-----------------------------------------------------------------------------
// Precache-related methods for effects (used by DispatchEffect)
//-----------------------------------------------------------------------------
void PrecacheEffect( const char *pParticleSystemName );
int GetEffectIndex( const char *pParticleSystemName );
const char *GetEffectNameFromIndex( int nIndex );

class IRecipientFilter;
void EntityMessageBegin( CBaseEntity * entity, bool reliable = false );
void UserMessageBegin( IRecipientFilter& filter, const char *messagename );
void MessageEnd( void );

// bytewise
void MessageWriteByte( int iValue);
void MessageWriteBytes(const void *pBuf, int nBytes);
void MessageWriteChar( int iValue);
void MessageWriteShort( int iValue);
void MessageWriteWord( int iValue );
void MessageWriteLong( long iValue);
void MessageWriteFloat( float flValue);
void MessageWriteAngle( float flValue);
void MessageWriteCoord( float flValue);
void MessageWriteVec3Coord( const Vector& rgflValue);
void MessageWriteVec3Normal( const Vector& rgflValue);
void MessageWriteAngles( const QAngle& rgflValue);
void MessageWriteString( const char *sz );
void MessageWriteEntity( int iValue);
void MessageWriteEHandle( CBaseEntity *pEntity ); //encoded as a long
void MessageWriteBitVecIntegral( const Vector& vecValue );

inline void MessageWriteModelIndex( modelindex_t iValue )
{ MessageWriteWord( iValue.GetRaw() ); }

inline void MessageWriteRGBA( color32 iValue )
{
	MessageWriteByte( iValue.r() );
	MessageWriteByte( iValue.g() );
	MessageWriteByte( iValue.b() );
	MessageWriteByte( iValue.a() );
}

// bitwise
void MessageWriteBool( bool bValue );
void MessageWriteUBitLong( unsigned int data, int numbits );
void MessageWriteSBitLong( int data, int numbits );
void MessageWriteBits( const void *pIn, int nBits );

/// Returns Steam ID, given player index.   Returns an invalid SteamID upon
/// failure
extern bool GetSteamIDForPlayerIndex( int iPlayerIndex, CSteamID &steamid );


// Bytewise
#define WRITE_BYTE		(MessageWriteByte)
#define WRITE_BYTES		(MessageWriteBytes)
#define WRITE_CHAR		(MessageWriteChar)
#define WRITE_SHORT		(MessageWriteShort)
#define WRITE_WORD		(MessageWriteWord)
#define WRITE_LONG		(MessageWriteLong)
#define WRITE_FLOAT		(MessageWriteFloat)
#define WRITE_ANGLE		(MessageWriteAngle)
#define WRITE_COORD		(MessageWriteCoord)
#define WRITE_VEC3COORD	(MessageWriteVec3Coord)
#define WRITE_VEC3NORMAL (MessageWriteVec3Normal)
#define WRITE_ANGLES	(MessageWriteAngles)
#define WRITE_STRING	(MessageWriteString)
#define WRITE_ENTITY	(MessageWriteEntity)
#define WRITE_EHANDLE	(MessageWriteEHandle)
#define WRITE_MODELINDEX		(MessageWriteModelIndex)
#define WRITE_RGBA		(MessageWriteRGBA)

// Bitwise
#define WRITE_BOOL		(MessageWriteBool)
#define WRITE_UBITLONG	(MessageWriteUBitLong)
#define WRITE_SBITLONG	(MessageWriteSBitLong)
#define WRITE_BITS		(MessageWriteBits)
#define WRITE_VEC3_INTEGRAL  (MessageWriteBitVecIntegral)

#endif		//ENGINECALLBACK_H
