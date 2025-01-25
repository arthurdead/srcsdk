//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "foundryhelpers_server.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_FOUNDRY, "Foundry Server" );

//-----------------------------------------------------------------------------
// Purpose: This just marshalls certain FoundryHelpers_ calls to the client.
//-----------------------------------------------------------------------------
class CTEFoundryHelpers_AddHighlight : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFoundryHelpers_AddHighlight, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEFoundryHelpers_AddHighlight( const char *pName ) :
		CBaseTempEntity( pName )
	{
	}

public:
	CNetworkHandle( CBaseEntity, m_hEntity );	// -1 means turn the effect off for all entities.
};

IMPLEMENT_SERVERCLASS_ST( CTEFoundryHelpers_AddHighlight, DT_TEFoundryHelpers_AddHighlight )
	DEFINE_SEND_FIELD( m_hEntity ),
END_SEND_TABLE()

// Singleton to fire TEMuzzleFlash objects
static CTEFoundryHelpers_AddHighlight g_TEFoundryHelpers_AddHighlight( "FoundryHelpers_AddHighlight" );

class CTEFoundryHelpers_ClearHighlights : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEFoundryHelpers_ClearHighlights, CBaseTempEntity );
	DECLARE_SERVERCLASS();

	CTEFoundryHelpers_ClearHighlights( const char *pName ) :
		CBaseTempEntity( pName )
	{
	}
};

IMPLEMENT_SERVERCLASS_ST( CTEFoundryHelpers_ClearHighlights, DT_TEFoundryHelpers_ClearHighlights )
END_SEND_TABLE()

static CTEFoundryHelpers_ClearHighlights g_TEFoundryHelpers_ClearHighlights( "FoundryHelpers_ClearHighlights" );

void FoundryHelpers_ClearEntityHighlightEffects()
{
	CBroadcastRecipientFilter filter;
	g_TEFoundryHelpers_ClearHighlights.Create( filter, 0 );
}

void FoundryHelpers_AddEntityHighlightEffect( CBaseEntity *pEnt )
{
	g_TEFoundryHelpers_AddHighlight.m_hEntity = pEnt->GetRefEHandle();
	
	CBroadcastRecipientFilter filter;
	g_TEFoundryHelpers_AddHighlight.Create( filter, 0 );
}


bool CheckInFoundryMode()
{
	if ( !serverfoundry )
	{
		Log_Warning( LOG_FOUNDRY, "Not in Foundry mode.\n" );
		return false;
	}

	return true;
}


void GetCrosshairOrNamedEntities( const CCommand &args, CUtlVector<CBaseEntity*> &entities )
{
	if ( args.ArgC() < 2 )
	{
		CBasePlayer *pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,
			MASK_SHOT_HULL|CONTENTS_GRATE|CONTENTS_DEBRIS, pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.DidHit() && !tr.DidHitWorld() )
		{
			entities.AddToTail( tr.m_pEnt );
		}
	}
	else
	{
		CBaseEntity *pEnt = NULL;
		while ((pEnt = gEntList.FindEntityGeneric( pEnt, args[1] ) ) != NULL)
		{
			entities.AddToTail( pEnt );
		}
	}
}


CON_COMMAND( foundry_update_entity, "Updates the entity's position/angles when in edit mode" )
{
	if(!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if ( !CheckInFoundryMode() )
		return;

	CUtlVector<CBaseEntity*> entities;
	GetCrosshairOrNamedEntities( args, entities );

	for ( int i=0; i < entities.Count(); i++ )
	{
		CBaseEntity *pEnt = entities[i];
		serverfoundry->MoveEntityTo( pEnt->GetHammerID(), pEnt->GetAbsOrigin(), pEnt->GetAbsAngles() );
	}
}

CON_COMMAND( foundry_sync_hammer_view, "Move Hammer's 3D view to the same position as the engine's 3D view." )
{
	if(!UTIL_IsCommandIssuedByServerAdmin())
		return;

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	Vector vPos = pPlayer->EyePosition();
	QAngle vAngles = pPlayer->pl.v_angle;
	serverfoundry->MoveHammerViewTo( vPos, vAngles );
}

CON_COMMAND( foundry_engine_get_mouse_control, "Give the engine control of the mouse." )
{
	if(!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if ( !CheckInFoundryMode() )
		return;

	serverfoundry->EngineGetMouseControl();
}


CON_COMMAND( foundry_engine_release_mouse_control, "Give the control of the mouse back to Hammer." )
{
	if(!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if ( !CheckInFoundryMode() )
		return;

	serverfoundry->EngineReleaseMouseControl();
}

CON_COMMAND( foundry_select_entity, "Select the entity under the crosshair or select entities with the specified name." )
{
	if(!UTIL_IsCommandIssuedByServerAdmin())
		return;

	CUtlVector<CBaseEntity*> entities;
	GetCrosshairOrNamedEntities( args, entities );

	CUtlVector<int> hammerIDs;
	for ( int i=0; i < entities.Count(); i++ )
	{
		CBaseEntity *pEnt = entities[i];
		hammerIDs.AddToTail( pEnt->GetHammerID() );
	}

	if ( hammerIDs.Count() == 0 )
	{
		CBasePlayer *pPlayer = UTIL_GetCommandClient();
		if ( !pPlayer )
			return;

		Vector vPos = pPlayer->EyePosition();
		QAngle vAngles = pPlayer->pl.v_angle;
		serverfoundry->SelectionClickInCenterOfView( vPos, vAngles );
	}
	else
	{
		serverfoundry->SelectEntities( hammerIDs.Base(), hammerIDs.Count() );
	}
}

