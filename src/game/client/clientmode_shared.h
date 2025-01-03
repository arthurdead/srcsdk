//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( CLIENTMODE_NORMAL_H )
#define CLIENTMODE_NORMAL_H
#pragma once

#include "iclientmode.h"
#include "GameEventListener.h"
#include <baseviewport.h>
#include "networkvar.h"
#include "cdll_int.h"
#include "ehandle.h"
#include "c_postprocesscontroller.h"
#include "util_shared.h"

class CBaseHudChat;
class CBaseHudWeaponSelection;
class CHudVote;
class CViewSetup;
class CViewSetupEx;
class C_BaseEntity;
class C_BasePlayer;
class C_ColorCorrection;

namespace vgui
{
class Panel;
}

//=============================================================================
// HPE_BEGIN:
// [tj] Moved this from the .cpp file so derived classes could access it
//=============================================================================
 
#define ACHIEVEMENT_ANNOUNCEMENT_MIN_TIME 10
 
//=============================================================================
// HPE_END
//=============================================================================

class CReplayReminderPanel;

#define USERID2PLAYER(i) ToBasePlayer( ClientEntityList().GetEnt( engine->GetPlayerForUserID( i ) ) )	

extern IClientMode *GetClientModeNormal(); // must be implemented
extern IClientMode *GetFullscreenClientMode();

// This class implements client mode functionality common to HL2 and TF2.
class ClientModeShared : public IClientMode, public CGameEventListener
{
// IClientMode overrides.
public:
	DECLARE_CLASS_NOBASE( ClientModeShared );

					ClientModeShared();
	virtual			~ClientModeShared();
	
	virtual void	Init();
	virtual void	InitViewport();
	virtual void	VGui_Shutdown();
	virtual void	Shutdown();

	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );

	virtual void	Enable();
	virtual void	EnableWithRootPanel( vgui::VPANEL pRoot );
	virtual void	Disable();
	virtual void	Layout(bool bForce = false);

	virtual void	ReloadScheme( bool flushLowLevel );
	virtual void	ReloadSchemeWithRoot( vgui::VPANEL pRoot, bool flushLowLevel );
	virtual void	OverrideView( CViewSetupEx *pSetup );
	virtual void	OverrideAudioState( AudioState_t *pAudioState ) { return; }
	virtual bool	ShouldDrawDetailObjects( );
	virtual bool	ShouldDrawEntity(C_BaseEntity *pEnt);
	virtual bool	ShouldDrawLocalPlayer( C_BasePlayer *pPlayer );
	virtual bool	ShouldDrawViewModel();
	virtual bool	ShouldDrawParticles( );
	virtual bool	ShouldDrawCrosshair( void );
	virtual bool	ShouldBlackoutAroundHUD() OVERRIDE;
	virtual void	AdjustEngineViewport( int& x, int& y, int& width, int& height );
	virtual void	PreRender(CViewSetupEx *pSetup);
	virtual void	PostRender();
	virtual void	PostRenderVGui();
	virtual void	ProcessInput(bool bActive);
	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *cmd );
	virtual void	Update();
	virtual void	OnColorCorrectionWeightsReset( void );
	virtual float	GetColorCorrectionScale( void ) const;
	virtual void	ClearCurrentColorCorrection() { m_pCurrentColorCorrection = NULL; }
	virtual void	SetBlurFade( float scale ) {}
	virtual float	GetBlurFade( void ) { return 0.0f; }

	// Input
	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual void	OverrideMouseInput( float *x, float *y );
	virtual void	StartMessageMode( int iMessageModeType );
	virtual vgui::Panel *GetMessagePanel();

	virtual void	ActivateInGameVGuiContext( vgui::Panel *pPanel );
	virtual void	DeactivateInGameVGuiContext();

	// The mode can choose to not draw fog
	virtual bool	ShouldDrawFog( void );
	
	virtual float	GetViewModelFOV( void );
	virtual vgui::Panel* GetViewport() { return m_pViewport; }
	virtual vgui::Panel *GetPanelFromViewport( const char *pchNamePath );
	// Gets at the viewports vgui panel animation controller, if there is one...
	virtual vgui::AnimationController *GetViewportAnimationController()
		{ return m_pViewport->GetAnimationController(); }
	
	virtual void FireGameEvent( IGameEvent *event );

	virtual bool CanRecordDemo( char *errorMsg, int length ) const { return true; }

	virtual int HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void InitChatHudElement( void );
	virtual void InitWeaponSelectionHudElement( void );
	virtual void InitVoteHudElement( void );

	virtual void	ComputeVguiResConditions( KeyValues *pkvConditions ) OVERRIDE;

	//=============================================================================
	// HPE_BEGIN:
	// [menglish] Save server information shown to the client in a persistent place
	//=============================================================================
	 
	virtual wchar_t* GetServerName() { return NULL; }
	virtual void SetServerName(wchar_t* name) {};
	virtual wchar_t* GetMapName() { return NULL; }
	virtual void SetMapName(wchar_t* name) {};
	 
	//=============================================================================
	// HPE_END
	//=============================================================================

	virtual bool	DoPostScreenSpaceEffects( const CViewSetupEx *pSetup );

	virtual void	DisplayReplayMessage( const char *pLocalizeName, float flDuration, bool bUrgent,
										  const char *pSound, bool bDlg );

	virtual bool	IsInfoPanelAllowed() OVERRIDE { return true; }
	virtual void	InfoPanelDisplayed() OVERRIDE { }
	virtual bool	IsHTMLInfoPanelAllowed() OVERRIDE { return true; }

	CBaseHudWeaponSelection *GetWeaponSelection()
	{ return m_pWeaponSelection; }

protected:
	CBaseViewport			*m_pViewport;

	void			DisplayReplayReminder();

private:
	virtual void	UpdateReplayMessages();

	void			ClearReplayMessageList();

#if defined( REPLAY_ENABLED )
	float					m_flReplayStartRecordTime;
	float					m_flReplayStopRecordTime;
	CReplayReminderPanel	*m_pReplayReminderPanel;
#endif

	// Message mode handling
	// All modes share a common chat interface
	CBaseHudChat			*m_pChatElement;
	vgui::HCursor			m_CursorNone;
	CBaseHudWeaponSelection *m_pWeaponSelection;
	CHudVote *m_pHudVote;
	int						m_nRootSize[2];

	void UpdatePostProcessingEffects();

	const C_PostProcessController* m_pCurrentPostProcessController;
	PostProcessParameters_t m_CurrentPostProcessParameters;
	PostProcessParameters_t m_LerpStartPostProcessParameters, m_LerpEndPostProcessParameters;
	CountdownTimer m_PostProcessLerpTimer;

	CHandle<C_ColorCorrection> m_pCurrentColorCorrection;
};

#endif // CLIENTMODE_NORMAL_H

