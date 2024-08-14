#ifndef HEIST_PLAYERANIMSTATE_H
#define HEIST_PLAYERANIMSTATE_H

#pragma once

#include "multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_HeistPlayer;
#define CHeistPlayer C_HeistPlayer
#else
class CHeistPlayer;
#endif

class CHeistPlayerAnimState : public CMultiPlayerAnimState
{
public:
	DECLARE_CLASS(CHeistPlayerAnimState, CMultiPlayerAnimState);

	CHeistPlayerAnimState();
	CHeistPlayerAnimState(CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData);
	~CHeistPlayerAnimState();

	void InitHeistAnimState(CHeistPlayer *pPlayer);
	CHeistPlayer *GetHeistPlayer()
	{ return m_pHeistPlayer; }

	void ClearAnimationState() override;
	Activity TranslateActivity(Activity actDesired) override;
	void Update(float eyeYaw, float eyePitch) override;

	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0) override;

	bool HandleMoving(Activity &idealActivity) override;
	bool HandleJumping(Activity &idealActivity) override;
	bool HandleDucking(Activity &idealActivity) override;
	bool HandleSwimming(Activity &idealActivity) override;

	float GetCurrentMaxGroundSpeed() override;

private:
	bool SetupPoseParameters(CStudioHdr *pStudioHdr);
	void EstimateYaw() override;
	void ComputePoseParam_MoveYaw(CStudioHdr *pStudioHdr) override;
	void ComputePoseParam_AimPitch(CStudioHdr *pStudioHdr) override;
	void ComputePoseParam_AimYaw(CStudioHdr *pStudioHdr) override;

	CHeistPlayer *m_pHeistPlayer;
	bool m_bInAirWalk;
};

CHeistPlayerAnimState *CreateHeistPlayerAnimState(CHeistPlayer *pPlayer);

#endif
