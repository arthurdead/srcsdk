//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is an entity that represents a vgui screen
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUISCREEN_H
#define VGUISCREEN_H

#pragma once

#include "baseentity.h"

//-----------------------------------------------------------------------------
// This is an entity that represents a vgui screen
//-----------------------------------------------------------------------------
class CVGuiScreen : public CBaseEntity
{
public:
	DECLARE_CLASS( CVGuiScreen, CBaseEntity );
	
	DECLARE_SERVERCLASS();
	DECLARE_MAPENTITY();

	CVGuiScreen();

	virtual void Precache();
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void Spawn();
	virtual void Activate();

	const char *GetPanelName() const;

	// Sets the screen size + resolution
	void SetActualSize( float flWidth, float flHeight );

	// Activates/deactivates the screen
	bool IsActive() const;
	void SetActive( bool bActive );

	// Is this screen only visible to teammates?
	bool IsVisibleOnlyToTeammates() const;
	void MakeVisibleOnlyToTeammates( bool bActive );
	bool IsVisibleToTeam( int nTeam );

	// Sets the overlay material
	void SetOverlayMaterial( const char *pMaterial );

	void SetAttachedToViewModel( bool bAttached );
	bool IsAttachedToViewModel() const;

	void SetTransparency( bool bTransparent );

	virtual int UpdateTransmitState( void );
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo );

	void SetPlayerOwner( CBasePlayer *pPlayer, bool bOwnerOnlyInput = false );

private:
	void SetAttachmentIndex( int nIndex );
 	void SetPanelName( const char *pPanelName );
	void InputSetActive( inputdata_t &inputdata );
	void InputSetInactive( inputdata_t &inputdata );

	string_t m_strOverlayMaterial;

	CNetworkVar( float, m_flWidth ); 
	CNetworkVar( float, m_flHeight );
	CNetworkVar( int, m_nPanelName );	// The name of the panel 
	CNetworkVar( int, m_nAttachmentIndex );
	CNetworkVar( int, m_nOverlayMaterial );
	CNetworkVar( int, m_fScreenFlags );
	CNetworkHandle( CBasePlayer, m_hPlayerOwner );

	friend CVGuiScreen *CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex );
	friend CVGuiScreen *CreateVGuiScreenPredicted( const char *module, int line, const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex );
};

#define CREATE_PREDICTED_VGUISCREEN( classname, ...) \
	CreateVGuiScreenPredicted( __FILE__, __LINE__, classname __VA_OPT__(, __VA_ARGS__) )

void PrecacheVGuiScreen( const char *pScreenType );
void PrecacheVGuiScreenOverlayMaterial( const char *pMaterialName );

CVGuiScreen *CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex );
CVGuiScreen *CreateVGuiScreenPredicted( const char *module, int line, const char *pScreenClassname, const char *pScreenType, CBaseEntity *pAttachedTo, CBaseEntity *pOwner, int nAttachmentIndex );

void DestroyVGuiScreen( CVGuiScreen *pVGuiScreen );


#endif // VGUISCREEN_H
