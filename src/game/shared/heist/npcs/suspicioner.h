#ifndef SUSPICIONER_H
#define SUSPICIONER_H

#include "networkvar.h"
#include "ehandle.h"
#include "shareddefs.h"
#include "heist_player_shared.h"
#include "dt_shared.h"
#include "util_shared.h"

#ifdef CLIENT_DLL
#define CSuspicioner C_Suspicioner
#endif

class CSuspicioner
{
public:
	DECLARE_CLASS_NOBASE(CSuspicioner);
	DECLARE_EMBEDDED_NETWORKVAR();

	CSuspicioner();
	virtual ~CSuspicioner();

#ifdef GAME_DLL
	void SetSuspicion(CHeistPlayer *player, float value);
	void SetSuspicion(int idx, float value);
#endif

	float GetSuspicion(CHeistPlayer *player) const;
	float GetSuspicion(int idx) const;

	static void ClearAll()
	{ s_SuspicionerList.RemoveAll(); }

	static const CUtlVector<CSuspicioner *> &List()
	{ return s_SuspicionerList; }

	void Init(CBaseEntity *owner);

	EHANDLE GetOwner() const
	{ return m_hOwner; }

	void Update();

private:
	static CUtlVector<CSuspicioner *> s_SuspicionerList;

	EHANDLE m_hOwner;

	CNetworkArray(float, m_flSuspicion, MAX_PLAYERS);
};

#ifdef GAME_DLL
EXTERN_SEND_TABLE(DT_Suspicioner);
#else
EXTERN_RECV_TABLE(DT_Suspicioner);
#endif

#endif
