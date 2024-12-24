//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SCENEENTITY_SHARED_H
#define SCENEENTITY_SHARED_H
#pragma once

#if defined( CLIENT_DLL )
class C_BaseFlex;
typedef C_BaseFlex CSharedBaseFlex;
class C_SceneEntity;
typedef C_SceneEntity CSharedSceneEntity;
#else
class CBaseFlex;
typedef CBaseFlex CSharedBaseFlex;
class CSceneEntity;
typedef CSceneEntity CSharedSceneEntity;
#endif

#include "iscenetokenprocessor.h"
#include "ehandle.h"
#include "tier1/convar.h"
#include "scenetokenprocessor.h"

class CChoreoEvent;
class CChoreoScene;
class CChoreoActor;
struct flexsettinghdr_t;

//-----------------------------------------------------------------------------
// Purpose: One of a number of currently playing scene events for this actor
//-----------------------------------------------------------------------------
// FIXME: move this, it's only used in in baseflex and baseactor
class CSceneEventInfo
{
public:
	CSceneEventInfo();

	// The event handle of the current scene event
	CChoreoEvent	*m_pEvent;

	// Current Scene
	CChoreoScene	*m_pScene;

	// Current actor
	CChoreoActor	*m_pActor;

	CHandle< CSharedSceneEntity >	m_hSceneEntity;

	// Set after the first time the event has been configured ( allows
	//  bumping markov index only at start of event playback, not every frame )
	bool			m_bStarted;

public:
	//	EVENT local data...
	// FIXME: Evil, make accessors or figure out better place
	// FIXME: This won't work, scenes don't save and restore...
	animlayerindex_t						m_iLayer;
	int						m_iPriority;
	sequence_t						m_nSequence;
	bool					m_bIsGesture;
	float					m_flWeight; // used for suppressions of posture while moving

	// movement, faceto targets?
	EHANDLE					m_hTarget;
	bool					m_bIsMoving;
	bool					m_bHasArrived;
	float					m_flInitialYaw;
	float					m_flTargetYaw;
	float					m_flFacingYaw;

	// generic AI events
	int						m_nType;
	float					m_flNext;

	// is this event only client side?
	bool					m_bClientSide; 

	// cached flex file
	const flexsettinghdr_t *m_pExpHdr; 

	void					InitWeight( CSharedBaseFlex *pActor );
	float					UpdateWeight( CSharedBaseFlex *pActor );
};

void Scene_Printf( PRINTF_FORMAT_STRING const char *pFormat, ... );
extern ConVar scene_clientflex;

#endif // SCENEENTITY_SHARED_H
