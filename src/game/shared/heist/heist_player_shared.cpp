#include "cbase.h"
#include "heist_player_shared.h"
#include "movevars_shared.h"

#ifdef CLIENT_DLL
#include "prediction.h"
#include "c_recipientfilter.h"
#define CRecipientFilter C_RecipientFilter
#endif

#ifdef CLIENT_DLL
#include "c_heist_player.h"
#else
#include "heist_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

Vector CHeistPlayer::GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget)
{
	if(pWeapon) {
		return pWeapon->GetBulletSpread(WEAPON_PROFICIENCY_PERFECT);
	}

	return VECTOR_CONE_15DEGREES;
}

void CHeistPlayer::PlayStepSound(Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force)
{
	if(gpGlobals->maxClients > 1 && !sv_footsteps.GetFloat()) {
		return;
	}

#ifdef CLIENT_DLL
	if(!prediction->IsFirstTimePredicted()) {
		return;
	}
#endif

	if(GetFlags() & FL_DUCKING) {
		return;
	}

	m_Local.m_nStepside = !m_Local.m_nStepside;

	char szStepSound[128];
	if(m_Local.m_nStepside) {
		Q_snprintf(szStepSound, sizeof( szStepSound ), "Player.RunFootstepLeft");
	} else {
		Q_snprintf(szStepSound, sizeof( szStepSound ), "Player.RunFootstepRight");
	}

	CSoundParameters params;
	if(GetParametersForSound(szStepSound, params, NULL) == false) {
		return;
	}

	CRecipientFilter filter;
	filter.AddRecipientsByPAS(vecOrigin);

#ifndef CLIENT_DLL
	if(gpGlobals->maxClients > 1) {
		filter.RemoveRecipientsByPVS(vecOrigin);
	}
#endif

	EmitSound_t ep;
	ep.m_nChannel = CHAN_BODY;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = fvol;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound(filter, entindex(), ep);
}
