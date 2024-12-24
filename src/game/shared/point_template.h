//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Point entity used to create templates out of other entities or groups of entities
//
//=============================================================================//

#ifndef POINT_TEMPLATE_H
#define POINT_TEMPLATE_H
#pragma once

#ifdef GAME_DLL
#include "baseentity.h"
#else
#include "c_baseentity.h"
#endif
#include "entityoutput.h"

#define MAX_NUM_TEMPLATES		16

struct template_t
{
	int			iTemplateIndex;
	VMatrix		matEntityToTemplate;
};

void PrecachePointTemplates();

#ifdef GAME_DLL
class CPointTemplate;
typedef CPointTemplate CSharedPointTemplate;
#else
class C_PointTemplate;
typedef C_PointTemplate CSharedPointTemplate;
#endif

#ifdef CLIENT_DLL
	#define CPointTemplate C_PointTemplate
#endif

enum PointTemplateSpawnFlags_t : unsigned char
{
	SF_POINTTEMPLATE_DONTREMOVETEMPLATEENTITIES = 0x0001,

// Level designers can suppress the uniquification of the spawned entity
// names with a spawnflag, provided they guarantee that only one instance
// of the entities will ever be spawned at a time.
	SF_POINTTEMPLATE_PRESERVE_NAMES = 0x0002,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointTemplate : public CSharedLogicalEntity
{
public:
	DECLARE_CLASS( CPointTemplate, CSharedLogicalEntity );

#ifdef CLIENT_DLL
	#undef CPointTemplate
#endif

	DECLARE_MAPENTITY();

	virtual void	Spawn( void );
	virtual void	Precache();

	DECLARE_SPAWNFLAGS( PointTemplateSpawnFlags_t )

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_spawnflags )

	// Template initialization
	void			StartBuildingTemplates( void );
	void			FinishBuildingTemplates( void );

	// Template Entity accessors
	int				GetNumTemplateEntities( void );
	CSharedBaseEntity		*GetTemplateEntity( int iTemplateNumber );
	void			AddTemplate( CSharedBaseEntity *pEntity, const char *pszMapData, int nLen );
	bool			ShouldRemoveTemplateEntities( void );
	bool			AllowNameFixup();
	bool			NameFixupExpanded() { return m_bFixupExpanded; }

	// Templates accessors
	int				GetNumTemplates( void );
	int				GetTemplateIndexForTemplate( int iTemplate );

	// Template instancing
	bool			CreateInstance( const Vector &vecOrigin, const QAngle &vecAngles, CUtlVector<CSharedBaseEntity*> *pEntities, CSharedBaseEntity *pEntityMaker = NULL, bool bCreateTime = false );
	bool			CreateSpecificInstance( int iTemplate, const Vector &vecOrigin, const QAngle &vecAngles, CSharedBaseEntity **pOutEntity );

	void			CreationComplete( const CUtlVector<CSharedBaseEntity*> &entities );

	// Inputs
	void			InputForceSpawn( inputdata_t &&inputdata );
	void			InputForceSpawnRandomTemplate( inputdata_t &&inputdata );

	virtual void	PerformPrecache();

private:
	string_t						m_iszTemplateEntityNames[MAX_NUM_TEMPLATES];

	// List of map entities this template targets. Built inside our Spawn().
	// It's only valid between Spawn() & Activate(), because the map entity parsing
	// code removes all the entities in it once it finishes turning them into templates.
	CUtlVector< CSharedBaseEntity * >		m_hTemplateEntities;

	// List of templates, generated from our template entities.
	CUtlVector< template_t >		m_hTemplates;

	COutputEvent					m_pOutputOnSpawned;
	COutputEHANDLE					m_pOutputOutEntity;

	// Allows name fixup to target all instances of a name in a keyvalue, including output parameters.
	// TODO: Support for multiple fixup modes?
	bool							m_bFixupExpanded;
};

#endif // POINT_TEMPLATE_H
