//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Point entity used to create templates out of other entities or groups of entities
//
//=============================================================================//

#ifndef POINT_TEMPLATE_H
#define POINT_TEMPLATE_H
#pragma once

#include "baseentity.h"

#define MAX_NUM_TEMPLATES		16

struct template_t
{
	int			iTemplateIndex;
	VMatrix		matEntityToTemplate;
};

void PrecachePointTemplates();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointTemplate : public CLogicalEntity
{
	DECLARE_CLASS( CPointTemplate, CLogicalEntity );
public:
	DECLARE_MAPENTITY();

	virtual void	Spawn( void );
	virtual void	Precache();

	// Template initialization
	void			StartBuildingTemplates( void );
	void			FinishBuildingTemplates( void );

	// Template Entity accessors
	int				GetNumTemplateEntities( void );
	CBaseEntity		*GetTemplateEntity( int iTemplateNumber );
	void			AddTemplate( CBaseEntity *pEntity, const char *pszMapData, int nLen );
	bool			ShouldRemoveTemplateEntities( void );
	bool			AllowNameFixup();
	bool			NameFixupExpanded() { return m_bFixupExpanded; }

	// Templates accessors
	int				GetNumTemplates( void );
	int				GetTemplateIndexForTemplate( int iTemplate );

	// Template instancing
	bool			CreateInstance( const Vector &vecOrigin, const QAngle &vecAngles, CUtlVector<CBaseEntity*> *pEntities, CBaseEntity *pEntityMaker = NULL, bool bCreateTime = false );
	bool			CreateSpecificInstance( int iTemplate, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity **pOutEntity );

	void			CreationComplete( const CUtlVector<CBaseEntity*> &entities );

	// Inputs
	void			InputForceSpawn( inputdata_t &inputdata );
	void			InputForceSpawnRandomTemplate( inputdata_t &inputdata );

	virtual void	PerformPrecache();

private:
	string_t						m_iszTemplateEntityNames[MAX_NUM_TEMPLATES];

	// List of map entities this template targets. Built inside our Spawn().
	// It's only valid between Spawn() & Activate(), because the map entity parsing
	// code removes all the entities in it once it finishes turning them into templates.
	CUtlVector< CBaseEntity * >		m_hTemplateEntities;

	// List of templates, generated from our template entities.
	CUtlVector< template_t >		m_hTemplates;

	COutputEvent					m_pOutputOnSpawned;
	COutputEHANDLE					m_pOutputOutEntity;

	// Allows name fixup to target all instances of a name in a keyvalue, including output parameters.
	// TODO: Support for multiple fixup modes?
	bool							m_bFixupExpanded;
};

#endif // POINT_TEMPLATE_H
