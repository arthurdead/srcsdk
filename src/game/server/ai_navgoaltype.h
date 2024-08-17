//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NAVGOALTYPE_H
#define AI_NAVGOALTYPE_H

#pragma once

// =======================================
//  Movement goals 
//		Used both to store the current
//		movment goal in m_routeGoalType
//		and to or/and with route
// =======================================
enum GoalType_t 
{
	GOALTYPE_NONE,
	GOALTYPE_TARGETENT,
	GOALTYPE_ENEMY,
	GOALTYPE_PATHCORNER,
	GOALTYPE_LOCATION,
#ifndef AI_USES_NAV_MESH
	GOALTYPE_LOCATION_NEAREST_NODE,
#else
	GOALTYPE_LOCATION_NEAREST_AREA,
#endif
	GOALTYPE_FLANK,
	GOALTYPE_COVER,
	
	GOALTYPE_INVALID
};

#endif // AI_NAVGOALTYPE_H
