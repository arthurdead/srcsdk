//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "animation.h"
#include "baseviewmodel.h"
#include "player.h"
#include <KeyValues.h>
#include "studio.h"
#include "vguiscreen.h"
#include "hltvdirector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SendProxy_AnimTime( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_SequenceChanged( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

int CBaseViewModel::UpdateTransmitState()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}

	return SetTransmitState( FL_EDICT_FULLCHECK );
}

int CBaseViewModel::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// check if receipient owns this weapon viewmodel
	CBasePlayer *pOwner = ToBasePlayer( m_hOwner );

	if ( pOwner && pOwner->edict() == pInfo->m_pClientEnt )
	{
		return FL_EDICT_ALWAYS;
	}

	// check if recipient spectates the own of this viewmodel
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	if ( pRecipientEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pRecipientEntity );

		if ( pPlayer->IsHLTV() || pPlayer->IsReplay() )
		{
			// if this is the HLTV client, transmit all viewmodels in our PVS
			return FL_EDICT_PVSCHECK;
		}

		if ( (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE)  && (pPlayer->GetObserverTarget() == pOwner) )
		{
			return FL_EDICT_ALWAYS;
		}
	}

	// Don't send to anyone else except the local player or his spectators
	return FL_EDICT_DONTSEND;
}

void CBaseViewModel::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Force our screens to be sent too.
	for ( int i=0; i < m_hScreens.Count(); i++ )
	{
		CVGuiScreen *pScreen = m_hScreens[i].Get();
		if ( pScreen )
			pScreen->SetTransmit( pInfo, bAlways );
	}
}
