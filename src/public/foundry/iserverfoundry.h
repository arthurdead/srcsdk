//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ISERVERFOUNDRY_H
#define ISERVERFOUNDRY_H
#pragma once

#include "interface.h"
#include "mathlib/vector.h"
#include "tier1/utlvector.h"

#ifndef SWDS

DECLARE_LOGGING_CHANNEL( LOG_FOUNDRY );

//-----------------------------------------------------------------------------
// Purpose: exposed from Foundry (Hammer) to game DLL
//-----------------------------------------------------------------------------
class IServerFoundry : public IBaseInterface
{
public:
	// This is used when you load a savegame and use certain Hammer entities on top of the savegame data.
	virtual bool GetRestoredEntityReplacementData( int iHammerID, CUtlVector<char> &data ) = 0;
	virtual void OnFinishedRestoreSavegame() = 0;

	// Move Hammer's version of the entity.
	virtual void MoveEntityTo( int nHammerID, const Vector &vPos, const QAngle &vAngles ) = 0;

	// Move Hammer's 3D view to the specified position.
	virtual void MoveHammerViewTo( const Vector &vPos, const QAngle &vAngles ) = 0;

	// If the game is controlling the input, release it.
	virtual void EngineGetMouseControl() = 0;
	virtual void EngineReleaseMouseControl() = 0;

	// Select the specified entities in Hammer.
	virtual void SelectEntities( int *pHammerIDs, int nIDs ) = 0;

	// Simulate a mouse click in the center of the 3D view.
	virtual void SelectionClickInCenterOfView( const Vector &vPos, const QAngle &vAngles ) = 0;
};

#define VSERVERFOUNDRY_INTERFACE_VERSION "VSERVERFOUNDRY001"

#endif

#endif // ISERVERFOUNDRY_H
