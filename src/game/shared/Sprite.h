//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SPRITE_H
#define SPRITE_H
#pragma once

#include "predictable_entity.h"
#include "baseentity_shared.h"

#define SF_SPRITE_STARTON		0x0001
#define SF_SPRITE_ONCE			0x0002
#define SF_SPRITE_TEMPORARY		0x8000

class CBasePlayer;

#if defined( CLIENT_DLL )
class C_Sprite;
typedef C_Sprite CSharedSprite;
class C_SpriteOriented;
typedef C_SpriteOriented CSharedSpriteOriented;

#include "c_pixel_visibility.h"

class CEngineSprite;

class C_SpriteRenderer
{
public:
	//-----------------------------------------------------------------------------
	// Purpose: Sprite orientations
	// WARNING!  Change these in common/MaterialSystem/Sprite.cpp if you change them here!
	//-----------------------------------------------------------------------------
	typedef enum
	{
		SPR_VP_PARALLEL_UPRIGHT		= 0,
		SPR_FACING_UPRIGHT			= 1,
		SPR_VP_PARALLEL				= 2,
		SPR_ORIENTED				= 3,
		SPR_VP_PARALLEL_ORIENTED	= 4
	} SPRITETYPE;
	
	// Determine sprite orientation
	static void							GetSpriteAxes( SPRITETYPE type, 
										const Vector& origin,
										const QAngle& angles,
										Vector& forward, 
										Vector& right, 
										Vector& up );

	// Sprites can alter blending amount
	virtual float					GlowBlend( CEngineSprite *psprite, const Vector& entorigin, int rendermode, int renderfx, int alpha, float *scale );

	// Draws tempent as a sprite
	int								DrawSprite( 
										IClientEntity *entity,
										const model_t *model, 
										const Vector& origin, 
										const QAngle& angles,
										float frame,
										IClientEntity *attachedto,
										int attachmentindex,
										int rendermode,
										int renderfx,
										int alpha,
										int r, 
										int g, 
										int b,
										float scale,
										float flHDRColorScale = 1.0f
										);

protected:
	pixelvis_handle_t	m_queryHandle;
	float				m_flGlowProxySize;
	float				m_flHDRColorScale;
};

#else
class CSprite;
typedef CSprite CSharedSprite;
class CSpriteOriented;
typedef CSpriteOriented CSharedSpriteOriented;
#endif

#if defined( CLIENT_DLL )
	#define CSprite C_Sprite
#endif

class CSprite : public CSharedBaseEntity
#if defined( CLIENT_DLL )
	, public C_SpriteRenderer
#endif
{
public:
	DECLARE_CLASS( CSprite, CSharedBaseEntity );
	CSprite() : CSprite( 0 ) {}
	CSprite( uint64 iEFlags );

#if defined( CLIENT_DLL )
	#undef CSprite
#endif

	DECLARE_PREDICTABLE();
	DECLARE_NETWORKCLASS();

	virtual void SetModel( const char *szModelName );

#if defined( CLIENT_DLL )
	virtual bool IsSprite( void ) const
	{
		return true;
	}
#endif

	void Spawn( void );
	void Precache( void );
	virtual void ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	void SetGlowProxySize( float flSize ) { m_flGlowProxySize = flSize; }

#if !defined( CLIENT_DLL )

	virtual EdictStateFlags_t ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual EdictStateFlags_t UpdateTransmitState( void );
	
	void SetAsTemporary( void ) { AddSpawnFlags( SF_SPRITE_TEMPORARY ); }
	bool IsTemporary( void ) { return ( HasSpawnFlags( SF_SPRITE_TEMPORARY ) ); }
	
	int	ObjectCaps( void )
	{ 
		return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); 
	}
#endif

	void AnimateThink( void );
	void ExpandThink( void );
	void Animate( float frames );
	void Expand( float scaleSpeed, float fadeSpeed );
	void SpriteInit( const char *pSpriteName, const Vector &origin );

#if !defined( CLIENT_DLL )
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	// Input handlers
	void InputHideSprite( inputdata_t &&inputdata );
	void InputShowSprite( inputdata_t &&inputdata );
	void InputToggleSprite( inputdata_t &&inputdata );
	void InputColorRedValue( inputdata_t &&inputdata );
	void InputColorBlueValue( inputdata_t &&inputdata );
	void InputColorGreenValue( inputdata_t &&inputdata );
#endif

	inline void SetAttachment( CSharedBaseEntity *pEntity, int attachment )
	{
		if ( pEntity )
		{
			m_hAttachedToEntity = pEntity;
			m_nAttachment = attachment;
			FollowEntity( pEntity );
		}
	}

	void TurnOff( void );
	void TurnOn( void );
	bool IsOn() { return !IsEffectActive( EF_NODRAW ); }

	inline float Frames( void ) { return m_flMaxFrame; }
	inline void SetTransparency( RenderMode_t rendermode, byte r, byte g, byte b, byte a, RenderFx_t fx )
	{
		SetRenderMode( (RenderMode_t)rendermode );
		SetColor( r, g, b );
		SetBrightness( a );
		SetRenderFX( (RenderFx_t)fx );
	}
	inline void SetTexture( modelindex_t spriteIndex ) { SetModelIndex( spriteIndex ); }
	inline void SetColor( byte r, byte g, byte b ) { SetRenderColor( r, g, b ); }
	
	void SetBrightness( int brightness, float duration = 0.0f );
	void SetScale( float scale, float duration = 0.0f );
	void SetSpriteScale( float scale );
	void EnableWorldSpaceScale( bool bEnable );

	float GetScale( void ) { return m_flSpriteScale; }
	int	GetBrightness( void ) { return m_nBrightness; }
	float GetHDRColorScale( void ) { return m_flHDRColorScale; }

	inline void FadeAndDie( float duration ) 
	{ 
		SetBrightness( 0, duration );
		SetThink(&CSharedSprite::AnimateUntilDead); 
		m_flDieTime = gpGlobals->curtime + duration; 
		SetNextThink( gpGlobals->curtime );  
	}

	inline void AnimateAndDie( float framerate ) 
	{ 
		SetThink(&CSharedSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + (m_flMaxFrame / m_flSpriteFramerate); 
		SetNextThink( gpGlobals->curtime ); 
	}

	inline void AnimateForTime( float framerate, float time ) 
	{ 
		SetThink(&CSharedSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + time;
		SetNextThink( gpGlobals->curtime ); 
	}

	// FIXME: This completely blows.
	// Surely there's gotta be a better way.
	void FadeOutFromSpawn( )
	{
		SetThink(&CSharedSprite::BeginFadeOutThink); 
		SetNextThink( gpGlobals->curtime + 0.01f ); 
	}

	void BeginFadeOutThink( )
	{
		FadeAndDie( 0.25f ); 
	}

	void AnimateUntilDead( void );
#if !defined( CLIENT_DLL )
	DECLARE_MAPENTITY();

	static CSharedSprite *SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate );
#endif
	static CSharedSprite *SpriteCreatePredictable( const char *module, int line, const char *pSpriteName, const Vector &origin, bool animate );

#if defined( CLIENT_DLL )
	virtual float	GetRenderScale( void );
	virtual int		GetRenderBrightness( void );

	virtual int		DrawModel( int flags, const RenderableInstance_t &instance );
	virtual const	Vector& GetRenderOrigin();
	virtual void	GetRenderBounds( Vector &vecMins, Vector &vecMaxs );
	virtual float	GlowBlend( CEngineSprite *psprite, const Vector& entorigin, int rendermode, int renderfx, int alpha, float *scale );
	virtual void	GetToolRecordingState( KeyValues *msg );

// Only supported in TF2 right now
#if defined( INVASION_CLIENT_DLL )
	virtual bool	ShouldPredict( void )
	{
		return true;
	}
#endif

	virtual void	BrightnessThink( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

#endif
public:
	CNetworkHandle( CSharedBaseEntity, m_hAttachedToEntity );
	CNetworkVar( int, m_nAttachment );
	CNetworkVar( float, m_flSpriteFramerate );
	CNetworkVar( float, m_flFrame );
#ifdef PORTAL
	CNetworkVar( bool, m_bDrawInMainRender );
	CNetworkVar( bool, m_bDrawInPortalRender );
#endif

	float		m_flDieTime;

private:

	CNetworkVar( int, m_nBrightness );
	CNetworkVar( float, m_flBrightnessTime );
	
	CNetworkVar( float, m_flSpriteScale );
	CNetworkVar( float, m_flScaleTime );
	CNetworkVar( bool, m_bWorldSpaceScale );
	CNetworkVar( float, m_flGlowProxySize );
	CNetworkVar( float, m_flHDRColorScale );

	float		m_flLastTime;
	float		m_flMaxFrame;

	float		m_flStartScale;
	float		m_flDestScale;		//Destination scale
	float		m_flScaleTimeStart;	//Real time for start of scale
	int			m_nStartBrightness;
	int			m_nDestBrightness;		//Destination brightness
	float		m_flBrightnessTimeStart;//Real time for brightness
};

#ifdef CLIENT_DLL
typedef C_ClientOnlyWrapper<C_Sprite> C_ClientOnlySprite;
#endif

#if defined( CLIENT_DLL )
	#define CSpriteOriented C_SpriteOriented
#endif

class CSpriteOriented : public CSharedSprite
{
public:
	DECLARE_CLASS( CSpriteOriented, CSharedSprite );

#if defined( CLIENT_DLL )
	#undef CSpriteOriented
#endif

#if !defined( CLIENT_DLL )
	DECLARE_SERVERCLASS();
	void Spawn( void );
#else
	DECLARE_CLIENTCLASS();
	virtual RenderableTranslucencyType_t ComputeTranslucencyType();
#endif
};

#ifdef CLIENT_DLL
typedef C_ClientOnlyWrapper<C_SpriteOriented> C_ClientOnlySpriteOriented;
#endif

// Macro to wrap creation
#define SPRITE_CREATE_PREDICTABLE( ... ) \
	CSharedSprite::SpriteCreatePredictable( __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__) )


#endif // SPRITE_H
