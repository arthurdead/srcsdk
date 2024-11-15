//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_VGUISCREEN_H
#define C_VGUISCREEN_H

#pragma once


#include <vgui_controls/EditablePanel.h>
#include "c_baseentity.h"
#include "panelmetaclassmgr.h"

class KeyValues;


//-----------------------------------------------------------------------------
// Helper macro to make overlay factories one line of code. Use like this:
//	DECLARE_VGUI_SCREEN_FACTORY( CVguiScreenPanel, "image" );
//-----------------------------------------------------------------------------
struct VGuiScreenInitData_t
{
	C_BaseEntity *m_pEntity;

	VGuiScreenInitData_t() : m_pEntity(NULL) {}
	VGuiScreenInitData_t( C_BaseEntity *pEntity ) : m_pEntity(pEntity) {}
};

#define DECLARE_VGUI_SCREEN_FACTORY( _PanelClass, _nameString )	\
	DECLARE_PANEL_FACTORY( _PanelClass, VGuiScreenInitData_t, _nameString )


//-----------------------------------------------------------------------------
// Base class for vgui screen panels
//-----------------------------------------------------------------------------
class CVGuiScreenPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS( CVGuiScreenPanel, vgui::EditablePanel );
	CVGuiScreenPanel( vgui::Panel *parent, const char *panelName );
	CVGuiScreenPanel( vgui::Panel *parent, const char *panelName, vgui::HScheme hScheme );
	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	vgui::Panel *CreateControlByName(const char *controlName);
	virtual void OnCommand( const char *command );

protected:
	C_BaseEntity *GetEntity() const { return m_hEntity.Get(); }

private:
	EHANDLE	m_hEntity;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_VGuiScreen : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_VGuiScreen, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_VGuiScreen();
	~C_VGuiScreen();

	virtual void PreDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual int DrawModel( int flags, const RenderableInstance_t &instance );
	virtual bool ShouldDraw( void );
	virtual void InputThink( );
	virtual void GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pOrigin, QAngle *pAngles );
	virtual bool IsVisibleToPlayer( C_BasePlayer *pViewingPlayer );
	virtual RenderableTranslucencyType_t ComputeTranslucencyType();

	const char *PanelName() const;

	// The view screen has the cursor pointing at it
	void GainFocus( );
	void LoseFocus();

	// Button state...
	void SetButtonState( uint64 nButtonState );

	// Is the screen backfaced given a view position?
	bool IsBackfacing( const Vector &viewOrigin );

	// Return intersection point of ray with screen in barycentric coords
	bool IntersectWithRay( const Ray_t &ray, float *u, float *v, float *t );

	// Is the screen turned on?
	bool IsActive() const;
	void SetActive( bool bActive );

	// Sets the screen size + resolution
	void SetActualSize( float flWidth, float flHeight );

	void MakeVisibleOnlyToTeammates( bool bActive );
	// Are we only visible to teammates?
	bool IsVisibleOnlyToTeammates() const;

	// Are we visible to someone on this team?
	bool IsVisibleToTeam( int nTeam );

	void SetAttachedToViewModel( bool bAttached );
	bool IsAttachedToViewModel() const;

	bool AcceptsInput() const;
	void SetAcceptsInput( bool acceptsinput );

	C_BasePlayer *GetPlayerOwner( void );
	bool IsInputOnlyToOwner( void );

	void GetSize( float &width, float &height ) const { width = m_flWidth; height = m_flHeight; }
	void GetPixelSize( int &width, int &height ) const { width = m_nPixelWidth; height = m_nPixelHeight; }
	void SetWidth( float flWidth ) { m_flWidth = flWidth; }
	void SetHeight( float flHeight ) { m_flHeight = flHeight; }
	void SetPixelWidth( int nWidth ) { m_nPixelWidth = nWidth; }
	void SetPixelHeight( int nHeight ) { m_nPixelHeight = nHeight; }

private:
	// Vgui screen management
	void CreateVguiScreen( const char *pTypeName );
	void DestroyVguiScreen( );

	//  Computes the panel to world transform
	void ComputePanelToWorld();

	// Computes control points of the quad describing the screen
	void ComputeEdges( Vector *pUpperLeft, Vector *pUpperRight, Vector *pLowerLeft );

	// Writes the z buffer
	void DrawScreenOverlay();

private:
	void SetAttachmentIndex( int nIndex );
	void SetPanelName( const char *pPanelName );

	int m_nPixelWidth; 
	int m_nPixelHeight;
	float m_flWidth; 
	float m_flHeight;
	int m_nPanelName;	// The name of the panel 
	uint64	m_nButtonState;
	uint64 m_nButtonPressed;
	uint64 m_nButtonReleased;
	int m_nOldPx;
	int m_nOldPy;
	uint64 m_nOldButtonState;
	int m_nAttachmentIndex;
	int m_nOverlayMaterial;
	int m_fScreenFlags;

	int	m_nOldPanelName;
	int m_nOldOverlayMaterial;

	bool m_bLoseThinkNextFrame;

	bool	m_bAcceptsInput;

	CMaterialReference	m_WriteZMaterial;
	CMaterialReference	m_OverlayMaterial;

	VMatrix	m_PanelToWorld;

	CPanelWrapper m_PanelWrapper;

	CHandle<C_BasePlayer> m_hPlayerOwner;

	friend C_VGuiScreen *CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, C_BaseEntity *pAttachedTo, C_BaseEntity *pOwner, int nAttachmentIndex );
	friend C_VGuiScreen *CreatePredictedVGuiScreen( const char *module, int line, const char *pScreenClassname, const char *pScreenType, C_BaseEntity *pAttachedTo, C_BaseEntity *pOwner, int nAttachmentIndex );
};

#define CREATE_PREDICTED_VGUISCREEN(...) \
	CreatePredictedVGuiScreen( __FILE__, __LINE__, __VA_ARGS__ )

//-----------------------------------------------------------------------------
// Returns an entity that is the nearby vgui screen; NULL if there isn't one
//-----------------------------------------------------------------------------
C_BaseEntity *FindNearbyVguiScreen( const Vector &viewPosition, const QAngle &viewAngle, int nTeam = -1 );

C_VGuiScreen *CreateVGuiScreen( const char *pScreenClassname, const char *pScreenType, C_BaseEntity *pAttachedTo, C_BaseEntity *pOwner, int nAttachmentIndex );
C_VGuiScreen *CreatePredictedVGuiScreen( const char *module, int line, const char *pScreenClassname, const char *pScreenType, C_BaseEntity *pAttachedTo, C_BaseEntity *pOwner, int nAttachmentIndex );

void DestroyVGuiScreen( C_VGuiScreen *pVGuiScreen );

//-----------------------------------------------------------------------------
// Activates/Deactivates vgui screen
//-----------------------------------------------------------------------------
void ActivateVguiScreen( C_BaseEntity *pVguiScreen );
void DeactivateVguiScreen( C_BaseEntity *pVguiScreen );


//-----------------------------------------------------------------------------
// Updates vgui screen button state
//-----------------------------------------------------------------------------
void SetVGuiScreenButtonState( C_BaseEntity *pVguiScreen, uint64 nButtonState );


// Called at shutdown.
void ClearKeyValuesCache();


#endif // C_VGUISCREEN_H
  
