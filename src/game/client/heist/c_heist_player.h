#ifndef C_HEIST_PLAYER_H
#define C_HEIST_PLAYER_H

#pragma once

#include "c_baseplayer.h"
#include "heist_playeranimstate.h"
#include "heist_player_shared.h"
#include "beamdraw.h"

class C_HeistPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS(C_HeistPlayer, C_BasePlayer);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_HeistPlayer();
	~C_HeistPlayer();

	static C_HeistPlayer *GetLocalHeistPlayer();

	void ThinkIDTarget();

	int DrawModel(int flags, const RenderableInstance_t &instance) override;

	Vector GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL);

	ShadowType_t ShadowCastType() override;
	const QAngle &GetRenderAngles() override;
	bool ShouldDraw() override;
	void OnDataChanged(DataUpdateType_t type) override;
	float GetFOV() override;
	CStudioHdr *OnNewModel() override;
	void TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator) override;
	void ItemPreFrame() override;
	void ItemPostFrame() override;
	float GetMinFOV() const override { return 5.0f; }
	Vector GetAutoaimVector(float flDelta) override;
	void NotifyShouldTransmit(ShouldTransmitState_t state) override;

	bool CreateLightEffects() override
	{
		return true;
	}

	bool ShouldReceiveProjectedTextures(int flags) override;
	void PostDataUpdate(DataUpdateType_t updateType) override;
	void PlayStepSound(const Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force) override;
	void PreThink() override;
	void DoImpactEffect(trace_t &tr, int nDamageType) override;
	void CalcView(Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov) override;
	const QAngle &EyeAngles() override;

	void UpdateLookAt();
	void Initialize();
	int GetIDTarget() const;
	void UpdateIDTarget();

	HeistPlayerState State_Get() const;

	void UpdateClientSideAnimation() override;

	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);

	void CalculateIKLocks(float currentTime) override;

	float GetServerIntendedCycle() override
	{ return m_flServerCycle; }
	void SetServerIntendedCycle(float cycle) override
	{ m_flServerCycle = cycle; }

	bool IsSpotted() const;

private:
	C_HeistPlayer(const C_HeistPlayer &);

	static void RecvProxy_CycleLatch(const CRecvProxyData *pData, void *pStruct, void *pOut);

	void ReleaseFlashlight();

	CNetworkVar(HeistPlayerState, m_iPlayerState);

	CHeistPlayerAnimState *m_PlayerAnimState;

	QAngle m_angEyeAngles;
	CInterpolatedVar<QAngle> m_iv_angEyeAngles;

	int m_iSpawnInterpCounter;
	int m_iSpawnInterpCounterCache;

	int m_cycleLatch;
	float m_flServerCycle;

	int	m_headYawPoseParam;
	int	m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;

	bool m_isInit;
	Vector m_vLookAtTarget;

	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;

	int m_iIDEntIndex;

	Beam_t *m_pFlashlightBeam;

	bool m_bSpotted;
};

inline C_HeistPlayer *ToHeistPlayer(CBaseEntity *pEntity)
{
	if(!pEntity || !pEntity->IsPlayer()) {
		return NULL;
	}

	return dynamic_cast<C_HeistPlayer *>(pEntity);
}

#endif
