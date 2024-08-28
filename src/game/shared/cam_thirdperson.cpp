//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "cam_thirdperson.h"
#include "gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static Vector CAM_HULL_MIN(-CAM_HULL_OFFSET,-CAM_HULL_OFFSET,-CAM_HULL_OFFSET);
static Vector CAM_HULL_MAX( CAM_HULL_OFFSET, CAM_HULL_OFFSET, CAM_HULL_OFFSET);

CThirdPersonManager g_ThirdPersonManager;

#ifdef CLIENT_DLL

#include "input.h"


extern const ConVar *sv_cheats;



#endif

CThirdPersonManager::CThirdPersonManager( void )
{
}

void CThirdPersonManager::Init( void )
{
	m_iForced = FORCED_CAM_DISABLED;
	m_flUpFraction = 0.0f;
	m_flFraction = 1.0f;

	m_flUpLerpTime = 0.0f;
	m_flLerpTime = 0.0f;

	m_flUpOffset = CAMERA_UP_OFFSET;

	if ( input )
	{
		input->CAM_SetCameraThirdData( NULL, vec3_angle );
	}

	if ( !sv_cheats )
	{
		sv_cheats = cvar->FindVar( "sv_cheats" );
	}
}

void CThirdPersonManager::Update( void )
{
#ifdef CLIENT_DLL
	bool thirdperson_allowed = (
		(!sv_cheats || sv_cheats->GetBool()) &&
		(!GameRules() || GameRules()->AllowThirdPersonCamera())
	);

	// If cheats have been disabled, pull us back out of third-person view.
	if ( (m_iForced == FORCED_CAM_DISABLED) && !thirdperson_allowed && input->CAM_IsThirdPerson())
	{
		input->CAM_Set( CAM_FIRSTPERSON );
		return;
	}

	if ( m_iForced != FORCED_CAM_DISABLED )
	{
		if ( input->CAM_Get() != m_iForced )
		{
			input->CAM_Set( m_iForced );
		}
	}
#endif
}

Vector CThirdPersonManager::GetFinalCameraOffset( void )
{
	Vector vDesired = GetDesiredCameraOffset();

	if ( m_flUpFraction != 1.0f )
	{
		vDesired.z += m_flUpOffset;
	}

	return vDesired;

}

Vector CThirdPersonManager::GetDistanceFraction( void )
{
	float flFraction = m_flFraction;
	float flUpFraction = m_flUpFraction;

	float flFrac = RemapValClamped( gpGlobals->curtime - m_flLerpTime, 0, CAMERA_OFFSET_LERP_TIME, 0, 1 );

	flFraction = Lerp( flFrac, m_flFraction, m_flTargetFraction );

	if ( flFrac == 1.0f )
	{
		m_flFraction = m_flTargetFraction;
	}

	flFrac = RemapValClamped( gpGlobals->curtime - m_flUpLerpTime, 0, CAMERA_UP_OFFSET_LERP_TIME, 0, 1 );

	flUpFraction = 1.0f - Lerp( flFrac, m_flUpFraction, m_flTargetUpFraction );

	if ( flFrac == 1.0f )
	{
		m_flUpFraction = m_flTargetUpFraction;
	}

	return Vector( flFraction, flFraction, flUpFraction );
}

void CThirdPersonManager::PositionCamera( CBasePlayer *pPlayer, const QAngle& angles )
{
	if ( pPlayer )
	{
		trace_t trace;

		Vector camForward, camRight, camUp;

		// find our player's origin, and from there, the eye position
		Vector origin = pPlayer->GetLocalOrigin();
		origin += pPlayer->GetViewOffset();

		AngleVectors( angles, &camForward, &camRight, &camUp );
	
		Vector endPos = origin;

		Vector vecCamOffset = endPos + (camForward * - GetDesiredCameraOffset()[DIST_FORWARD]) + (camRight * GetDesiredCameraOffset()[ DIST_RIGHT ]) + (camUp * GetDesiredCameraOffset()[ DIST_UP ] );

		// use our previously #defined hull to collision trace
		CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
		UTIL_TraceHull( endPos, vecCamOffset, CAM_HULL_MIN, CAM_HULL_MAX, MASK_SOLID & ~CONTENTS_MONSTER, &traceFilter, &trace );
		
		if ( trace.fraction != m_flTargetFraction )
		{
			m_flLerpTime = gpGlobals->curtime;
		}

		m_flTargetFraction = trace.fraction;
		m_flTargetUpFraction = 1.0f;

		//If we're getting closer to a wall snap the fraction right away.
		if ( m_flTargetFraction < m_flFraction )
		{
			m_flFraction = m_flTargetFraction;
			m_flLerpTime = gpGlobals->curtime;
		}
	
		// move the camera closer if it hit something
		if( trace.fraction < 1.0  )
		{
			m_vecCameraOffset[ DIST ] *= trace.fraction;

			UTIL_TraceHull( endPos, endPos + (camForward * - GetDesiredCameraOffset()[DIST_FORWARD]), CAM_HULL_MIN, CAM_HULL_MAX, MASK_SOLID & ~CONTENTS_MONSTER, &traceFilter, &trace );

			if ( trace.fraction != 1.0f )
			{
				if ( trace.fraction != m_flTargetUpFraction )
				{
					m_flUpLerpTime = gpGlobals->curtime;
				}

				m_flTargetUpFraction = trace.fraction;

				if ( m_flTargetUpFraction < m_flUpFraction )
				{
					m_flUpFraction = trace.fraction;
					m_flUpLerpTime = gpGlobals->curtime;
				}
			}
		}
	}
}
