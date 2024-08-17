//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ILAGCOMPENSATIONMANAGER_H
#define ILAGCOMPENSATIONMANAGER_H
#pragma once

class CBasePlayer;
class CUserCmd;

//-----------------------------------------------------------------------------
// Purpose: This is also an IServerSystem
//-----------------------------------------------------------------------------
abstract_class ILagCompensationManager
{
public:
	// Called during player movement to set up/restore after lag compensation
	virtual void	StartLagCompensation( CBasePlayer *player, CUserCmd *cmd ) = 0;
	virtual void	FinishLagCompensation( CBasePlayer *player ) = 0;
	virtual bool	IsCurrentlyDoingLagCompensation() const = 0;
#ifdef SM_AI_FIXES
	virtual void	RemoveNpcData(int index) = 0; 
#endif
};

extern ILagCompensationManager *lagcompensation;

#endif // ILAGCOMPENSATIONMANAGER_H
