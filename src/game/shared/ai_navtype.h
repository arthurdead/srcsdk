//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NAVTYPE_H
#define AI_NAVTYPE_H

#pragma once

// ---------------------------
//  Navigation Type Bits
// ---------------------------
enum Navigation_t 
{
	NAV_NONE = -1,	// error condition
	NAV_GROUND = 0,	// walk/run
	NAV_JUMP,		// jump/leap
	NAV_FLY,		// can fly, move all around
	NAV_CLIMB,		// climb ladders
	NAV_CRAWL,		// crawl
};


#endif // AI_NAVTYPE_H
