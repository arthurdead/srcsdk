//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BEAM_H
#define BEAM_H
#pragma once

#include "baseentity_shared.h"
#include "baseplayer_shared.h"
#if !defined( CLIENT_DLL )
#include "entityoutput.h"
#else
#include "c_pixel_visibility.h"
#endif

#include "beam_flags.h"

#define MAX_BEAM_WIDTH			102.3f
#define MAX_BEAM_SCROLLSPEED	100.0f
#define MAX_BEAM_NOISEAMPLITUDE		64

#define SF_BEAM_STARTON			0x0001
#define SF_BEAM_TOGGLE			0x0002
#define SF_BEAM_RANDOM			0x0004
#define SF_BEAM_RING			0x0008
#define SF_BEAM_SPARKSTART		0x0010
#define SF_BEAM_SPARKEND		0x0020
#define SF_BEAM_DECALS			0x0040
#define SF_BEAM_SHADEIN			0x0080
#define SF_BEAM_SHADEOUT		0x0100
#define	SF_BEAM_TAPEROUT		0x0200	// Tapers to zero
#define SF_BEAM_TEMPORARY		0x8000

#define ATTACHMENT_INDEX_BITS	5
#define ATTACHMENT_INDEX_MASK	((1 << ATTACHMENT_INDEX_BITS) - 1)

#if defined( CLIENT_DLL )
class C_Beam;
typedef C_Beam CSharedBeam;
#else
class CBeam;
typedef CBeam CSharedBeam;
#endif

#if defined( CLIENT_DLL )
#define CBeam C_Beam
#endif

class CBeam : public CSharedBaseEntity
{
public:
	DECLARE_CLASS( CBeam, CSharedBaseEntity );
	CBeam();

#if defined( CLIENT_DLL )
	#undef CBeam
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
#if !defined( CLIENT_DLL )
	DECLARE_MAPENTITY();
#endif

	virtual void SetModel( const char *szModelName );

	void	Spawn( void );
	void	Precache( void );
#if !defined( CLIENT_DLL )
	EntityCaps_t ObjectCaps( void );
	void	SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	EdictStateFlags_t UpdateTransmitState( void );
	int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
#endif

	virtual int DrawDebugTextOverlays(void);

	// These functions are here to show the way beams are encoded as entities.
	// Encoding beams as entities simplifies their management in the client/server architecture
	void SetType( int type );
	void SetBeamFlags( int flags );
	void SetBeamFlag( int flag );
	
	// NOTE: Start + End Pos are specified in *relative* coordinates 
	void SetStartPos( const Vector &pos );
	void SetEndPos( const Vector &pos );

	// This will change things so the abs position matches the requested spot
	void SetAbsStartPos( const Vector &pos );
	void SetAbsEndPos( const Vector &pos );

	const Vector &GetAbsStartPos( void ) const;
	const Vector &GetAbsEndPos( void ) const;

	void SetStartEntity( CSharedBaseEntity *pEntity );
	void SetEndEntity( CSharedBaseEntity *pEntity );

	void SetStartAttachment( int attachment );
	void SetEndAttachment( int attachment );

	void SetTexture( modelindex_t spriteIndex );
	void SetHaloTexture( modelindex_t spriteIndex );
	void SetHaloScale( float haloScale );
	void SetWidth( float width );
	void SetEndWidth( float endWidth );
	void SetFadeLength( float fadeLength );
	void SetNoise( float amplitude );
	void SetColor( int r, int g, int b );
	void SetBrightness( int brightness );
	void SetFrame( float frame );
	void SetScrollRate( float speed );
	void SetFireTime( float flFireTime );
	void SetFrameRate( float flFrameRate ) { m_flFrameRate = flFrameRate; }

	void SetMinDXLevel( int nMinDXLevel ) { m_nMinDXLevel = nMinDXLevel; }

	void TurnOn( void );
	void TurnOff( void );

	int	GetType( void ) const;
	int	GetBeamFlags( void ) const;
	CSharedBaseEntity* GetStartEntityPtr( void ) const;
	int	GetStartEntity( void ) const;
	CSharedBaseEntity* GetEndEntityPtr( void ) const;
	int	GetEndEntity( void ) const;
	int GetStartAttachment() const;
	int GetEndAttachment() const;

	virtual const Vector &WorldSpaceCenter( void ) const;

	modelindex_t GetTexture( void ) const;
	float GetWidth( void ) const;
	float GetEndWidth( void ) const;
	float GetFadeLength( void ) const;
	float GetNoise( void ) const;
	int GetBrightness( void ) const;
	float GetFrame( void ) const;
	float GetScrollRate( void ) const;
	float GetHDRColorScale( void ) const;
	void SetHDRColorScale( float flScale ) { m_flHDRColorScale = flScale; }


	// Call after you change start/end positions
	void		RelinkBeam( void );

	void		DoSparks( const Vector &start, const Vector &end );
	CSharedBaseEntity *RandomTargetname( const char *szName );
	void		BeamDamage( trace_t *ptr );
	// Init after BeamCreate()
	void		BeamInit( const char *pSpriteName, float width );
	void		PointsInit( const Vector &start, const Vector &end );
	void		PointEntInit( const Vector &start, CSharedBaseEntity *pEndEntity );
	void		EntsInit( CSharedBaseEntity *pStartEntity, CSharedBaseEntity *pEndEntity );
	void		LaserInit( CSharedBaseEntity *pStartEntity, CSharedBaseEntity *pEndEntity );
	void		HoseInit( const Vector &start, const Vector &direction );
	void		SplineInit( int nNumEnts, CSharedBaseEntity** pEntList, int *attachment  );

	// Input handlers

	static CSharedBeam *BeamCreate( const char *pSpriteName, float width );
	static CSharedBeam *BeamCreatePredictable( const char *module, int line, const char *pSpriteName, float width, CSharedBasePlayer *pOwner );

	void LiveForTime( float time );
	void BeamDamageInstant( trace_t *ptr, float damage );

// Only supported in TF2 right now
#if defined( INVASION_CLIENT_DLL )
	virtual bool	ShouldPredict( void )
	{
		return true;
	}
#endif

	virtual const char *GetDecalName( void ) { return "BigShot"; }

	// specify whether the beam should always go all the way to 
	// the end point, or clip against geometry, or clip against
	// geometry and NPCs. This is only used by env_beams at present, but
	// need to be in this CBeam because of the way it affects drawing.
	enum BeamClipStyle_t
	{
		kNOCLIP = 0, // don't clip (default)
		kGEOCLIP = 1,
		kMODELCLIP = 2,

		kBEAMCLIPSTYLE_NUMBITS = 2, //< number of bits needed to represent this object
	};

	inline BeamClipStyle_t GetClipStyle() const { return m_nClipStyle; }

#if defined( CLIENT_DLL )
// IClientEntity overrides.
public:
	virtual int			DrawModel( int flags, const RenderableInstance_t &instance );
	virtual RenderableTranslucencyType_t ComputeTranslucencyType();
	virtual bool		ShouldDraw();
	virtual void		OnDataChanged( DataUpdateType_t updateType );

	virtual bool		OnPredictedEntityRemove( bool isbeingremoved, C_BaseEntity *predicted );

	// Add beam to visible entities list?
	virtual bool		Simulate( void );
	virtual bool		ShouldReceiveProjectedTextures( int flags )
	{
		return false;
	}

// Beam Data Elements
private:
	// Computes the bounding box of a beam local to the origin of the beam
	void ComputeBounds( Vector& mins, Vector& maxs );

	friend void RecvProxy_Beam_ScrollSpeed( const CRecvProxyData *pData, void *pStruct, void *pOut );
	friend class CViewRenderBeams;

#endif

protected:
	CNetworkVar( float, m_flFrameRate );
	CNetworkVar( float, m_flHDRColorScale );
	float		m_flFireTime;
	float		m_flDamage;			// Damage per second to touchers.
	CNetworkVar( int, m_nNumBeamEnts );
#if defined( CLIENT_DLL )
	pixelvis_handle_t	m_queryHandleHalo;
#endif

private:
#if !defined( CLIENT_DLL )
	void InputNoise( inputdata_t &&inputdata );
 	void InputWidth( inputdata_t &&inputdata );
	void InputColorRedValue( inputdata_t &&inputdata );
	void InputColorBlueValue( inputdata_t &&inputdata );
	void InputColorGreenValue( inputdata_t &&inputdata );
#endif

	// Beam Data Elements
	CNetworkModelIndex( m_nHaloIndex );
	CNetworkVar( int, m_nBeamType );
	CNetworkVar( int, m_nBeamFlags );
	CNetworkArray( EHANDLE, m_hAttachEntity, MAX_BEAM_ENTS );
	CNetworkArray( int, m_nAttachIndex, MAX_BEAM_ENTS );
	CNetworkVar( float, m_fWidth );
	CNetworkVar( float, m_fEndWidth );
	CNetworkVar( float, m_fFadeLength );
	CNetworkVar( float, m_fHaloScale );
	CNetworkVar( float, m_fAmplitude );
	CNetworkVar( float, m_fStartFrame );
	CNetworkVar( float, m_fSpeed );
	CNetworkVar( int, m_nMinDXLevel );
	CNetworkVar( float, m_flFrame );
	CNetworkVar( BeamClipStyle_t, m_nClipStyle );

	CNetworkVector( m_vecEndPos );

	EHANDLE		m_hEndEntity;

#if !defined( CLIENT_DLL )
	int			m_nDissolveType;
#endif

public:
#ifdef PORTAL
	CNetworkVar( bool, m_bDrawInMainRender );
	CNetworkVar( bool, m_bDrawInPortalRender );
#endif //#ifdef PORTAL
};

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Inline methods 
//-----------------------------------------------------------------------------
inline int CSharedBeam::ObjectCaps( void )
{ 
	return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); 
}
#endif

inline void	CSharedBeam::SetFireTime( float flFireTime )		
{ 
	m_flFireTime = flFireTime; 
}

//-----------------------------------------------------------------------------
// NOTE: Start + End Pos are specified in *relative* coordinates 
//-----------------------------------------------------------------------------
inline void CSharedBeam::SetStartPos( const Vector &pos ) 
{ 
#if defined( CLIENT_DLL )
	SetNetworkOrigin( pos );
#endif
	SetLocalOrigin( pos );
}

inline void CSharedBeam::SetEndPos( const Vector &pos ) 
{ 
	m_vecEndPos = pos; 
}
	 
 // center point of beam
inline const Vector &CSharedBeam::WorldSpaceCenter( void ) const 
{
	Vector &vecResult = AllocTempVector();
	VectorAdd( GetAbsStartPos(), GetAbsEndPos(), vecResult );
	vecResult *= 0.5f;
	return vecResult;
}

inline void CSharedBeam::SetStartAttachment( int attachment )	
{
	Assert( (attachment & ~ATTACHMENT_INDEX_MASK) == 0 );
	m_nAttachIndex.Set( 0, attachment );
}

inline void CSharedBeam::SetEndAttachment( int attachment )		
{ 
	Assert( (attachment & ~ATTACHMENT_INDEX_MASK) == 0 );
	m_nAttachIndex.Set( m_nNumBeamEnts-1, attachment );
}

inline void CSharedBeam::SetTexture( modelindex_t spriteIndex )		
{ 
	SetModelIndex( spriteIndex ); 
}

inline void CSharedBeam::SetHaloTexture( modelindex_t spriteIndex )	
{ 
	m_nHaloIndex = spriteIndex; 
}

inline void CSharedBeam::SetHaloScale( float haloScale )		
{ 
	m_fHaloScale = haloScale; 
}

inline void CSharedBeam::SetWidth( float width )				
{
	Assert( width <= MAX_BEAM_WIDTH );
	m_fWidth = MIN( MAX_BEAM_WIDTH, width );
}

inline void CSharedBeam::SetEndWidth( float endWidth )		
{ 
	Assert( endWidth <= MAX_BEAM_WIDTH );
	m_fEndWidth	= MIN( MAX_BEAM_WIDTH, endWidth );
}

inline void CSharedBeam::SetFadeLength( float fadeLength )	
{ 
	m_fFadeLength = fadeLength; 
}

inline void CSharedBeam::SetNoise( float amplitude )			
{ 
	m_fAmplitude = amplitude; 
}

inline void CSharedBeam::SetColor( int r, int g, int b )		
{ 
	SetRenderColor( r, g, b );
}

inline void CSharedBeam::SetBrightness( int brightness )		
{ 
	SetRenderAlpha( brightness ); 
}

inline void CSharedBeam::SetFrame( float frame )				
{ 
	m_fStartFrame = frame; 
}

inline void CSharedBeam::SetScrollRate( float speed )			
{ 
	m_fSpeed = speed; 
}

inline CSharedBaseEntity* CSharedBeam::GetStartEntityPtr( void ) const 
{ 
	return m_hAttachEntity[0].Get(); 
}

inline int CSharedBeam::GetStartEntity( void ) const 
{ 
	CSharedBaseEntity *pEntity = m_hAttachEntity[0].Get();
	return pEntity ? pEntity->entindex() : 0; 
}

inline CSharedBaseEntity* CSharedBeam::GetEndEntityPtr( void ) const 
{ 
	return m_hAttachEntity[1].Get(); 
}

inline int CSharedBeam::GetEndEntity( void ) const	
{ 
	CSharedBaseEntity *pEntity = m_hAttachEntity[m_nNumBeamEnts-1].Get();
	return pEntity ? pEntity->entindex() : 0; 
}

inline int CSharedBeam::GetStartAttachment() const
{
	return m_nAttachIndex[0] & ATTACHMENT_INDEX_MASK;
}

inline int CSharedBeam::GetEndAttachment() const
{
	return m_nAttachIndex[m_nNumBeamEnts-1] & ATTACHMENT_INDEX_MASK;
}

inline modelindex_t CSharedBeam::GetTexture( void ) const		
{ 
	return GetModelIndex(); 
}

inline float CSharedBeam::GetWidth( void ) const		
{
	return m_fWidth; 
}

inline float CSharedBeam::GetEndWidth( void ) const	
{ 
	return m_fEndWidth; 
}

inline float CSharedBeam::GetFadeLength( void ) const	
{ 
	return m_fFadeLength; 
}

inline float CSharedBeam::GetNoise( void ) const		
{ 
	return m_fAmplitude; 
}

inline int CSharedBeam::GetBrightness( void ) const	
{ 
	return GetRenderAlpha();
}

inline float CSharedBeam::GetFrame( void ) const		
{ 
	return m_fStartFrame; 
}

inline float CSharedBeam::GetScrollRate( void ) const	
{
	return m_fSpeed; 
}

inline float CSharedBeam::GetHDRColorScale( void ) const
{
	return m_flHDRColorScale;
}

inline void CSharedBeam::LiveForTime( float time ) 
{ 
	SetThink(&CSharedBeam::SUB_Remove); 
	SetNextThink( gpGlobals->curtime + time ); 
}

inline void	CSharedBeam::BeamDamageInstant( trace_t *ptr, float damage ) 
{ 
	m_flDamage = damage; 
	m_flFireTime = gpGlobals->curtime - 1;
	BeamDamage(ptr); 
}

bool IsStaticPointEntity( CSharedBaseEntity *pEnt );

// Macro to wrap creation
#define BEAM_CREATE_PREDICTABLE( ... ) \
	CSharedBeam::BeamCreatePredictable( __FILE__, __LINE__, __VA_ARGS__ )

#define BEAM_CREATE_PREDICTABLE_AT( ... ) \
	CSharedBeam::BeamCreatePredictable( file, line, __VA_ARGS__ )

// Start/End Entity is encoded as 12 bits of entity index, and 4 bits of attachment (4:12)
#define BEAMENT_ENTITY(x)		((x)&0xFFF)
#define BEAMENT_ATTACHMENT(x)	(((x)>>12)&0xF)


// Beam types, encoded as a byte
enum 
{
	BEAM_POINTS = 0,
	BEAM_ENTPOINT,
	BEAM_ENTS,
	BEAM_HOSE,
	BEAM_SPLINE,
	BEAM_LASER,
	NUM_BEAM_TYPES
};


#endif // BEAM_H
