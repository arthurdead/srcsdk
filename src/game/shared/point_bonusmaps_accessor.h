//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef POINT_BONUSMAPS_ACCESSOR_H
#define POINT_BONUSMAPS_ACCESSOR_H

#pragma once


void BonusMapChallengeUpdate( const char *pchFileName, const char *pchMapName, const char *pchChallengeName, int iBest );
void BonusMapChallengeNames( char *pchFileName, char *pchMapName, char *pchChallengeName );
void BonusMapChallengeObjectives( int &iBronze, int &iSilver, int &iGold );


#endif		// POINT_BONUSMAPS_ACCESSOR_H
