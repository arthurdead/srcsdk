//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Helper methods + classes for choreo
//
//===========================================================================//

#ifndef CHOREOUTILS_H
#define CHOREOUTILS_H

#pragma once


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CChoreoScene;
class CChoreoEvent;
class CStudioHdr;


//-----------------------------------------------------------------------------
// Finds sound files associated with events
//-----------------------------------------------------------------------------
const char *GetSoundForEvent( const CChoreoEvent *pEvent, const CStudioHdr *pStudioHdr );


//-----------------------------------------------------------------------------
// Fixes up the duration of a choreo scene based on wav files + animations
// Returns true if a change needed to be made
//-----------------------------------------------------------------------------
bool AutoAddGestureKeys( CChoreoEvent *e, const CStudioHdr *pStudioHdr, const float *pPoseParameters, bool bCheckOnly );
bool UpdateGestureLength( CChoreoEvent *e, const CStudioHdr *pStudioHdr, const float *pPoseParameters, bool bCheckOnly );
bool UpdateSequenceLength( CChoreoEvent *e, const CStudioHdr *pStudioHdr, const float *pPoseParameters, bool bCheckOnly, bool bVerbose );


#endif // CHOREOUTILS_H

