//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "filters.h"
#include "entitylist.h"
#include "ai_squad.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ###################################################################
//	> BaseFilter
// ###################################################################
LINK_ENTITY_TO_CLASS(filter_base, CBaseFilter);

BEGIN_MAPENTITY( CBaseFilter, MAPENT_FILTERCLASS )

	DEFINE_KEYFIELD_AUTO( m_bNegated, "Negated" ),
	DEFINE_KEYFIELD_AUTO( m_bPassCallerWhenTested, "PassCallerWhenTested" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INPUT, "TestActivator", InputTestActivator ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "TestEntity", InputTestEntity ),
	DEFINE_INPUTFUNC( FIELD_INPUT, "SetField", InputSetField ),

	// Outputs
	DEFINE_OUTPUT( m_OnPass, "OnPass"),
	DEFINE_OUTPUT( m_OnFail, "OnFail"),

END_MAPENTITY()

//-----------------------------------------------------------------------------

bool CBaseFilter::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	return true;
}


bool CBaseFilter::PassesFilter( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	bool baseResult = PassesFilterImpl( pCaller, pEntity );
	return (m_bNegated) ? !baseResult : baseResult;
}

bool CBaseFilter::PassesDamageFilter(CBaseEntity *pCaller, const CTakeDamageInfo &info)
{
	bool baseResult = PassesDamageFilterImpl(pCaller, info);
	return (m_bNegated) ? !baseResult : baseResult;
}

bool CBaseFilter::PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
{
	//Tony; modified so it can check the inflictor or the attacker. We'll check the attacker first; which is normal if that fails, then check the inflictor.
	bool bResult = false;
	bResult = PassesFilterImpl( pCaller, info.GetAttacker() );

	if ( !bResult && info.GetInflictor() != NULL )
		bResult = PassesFilterImpl( pCaller, info.GetInflictor() );

	return bResult;//PassesFilterImpl( pCaller, info.GetAttacker() );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for testing the activator. If the activator passes the
//			filter test, the OnPass output is fired. If not, the OnFail output is fired.
//-----------------------------------------------------------------------------
void CBaseFilter::InputTestActivator( inputdata_t &&inputdata )
{
	if ( PassesFilter( inputdata.pCaller, inputdata.pActivator ) )
	{
		m_OnPass.FireOutput( inputdata.pActivator, m_bPassCallerWhenTested ? inputdata.pCaller : this );
	}
	else
	{
		m_OnFail.FireOutput( inputdata.pActivator, m_bPassCallerWhenTested ? inputdata.pCaller : this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for testing the activator. If the activator passes the
//			filter test, the OnPass output is fired. If not, the OnFail output is fired.
//-----------------------------------------------------------------------------
void CBaseFilter::InputTestEntity( inputdata_t &&inputdata )
{
	if ( !inputdata.value.Entity() )
	{
		// HACKHACK: Not firing OnFail in this case is intentional for the time being (activator shouldn't be null)
		return;
	}

	if ( PassesFilter( inputdata.pCaller, inputdata.value.Entity() ) )
	{
		m_OnPass.FireOutput( inputdata.value.Entity(), m_bPassCallerWhenTested ? inputdata.pCaller : this );
	}
	else
	{
		m_OnFail.FireOutput( inputdata.value.Entity(), m_bPassCallerWhenTested ? inputdata.pCaller : this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tries to set the filter's target since most filters use "filtername" anyway
//-----------------------------------------------------------------------------
void CBaseFilter::InputSetField( inputdata_t &&inputdata )
{
	KeyValue("filtername", inputdata.value.String());
	Activate();
}


// ###################################################################
//	> FilterMultiple
//
//   Allows one to filter through mutiple filters
// ###################################################################
#define MAX_FILTERS 10
enum filter_t
{
	FILTER_AND,
	FILTER_OR,
};

class CFilterMultiple : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterMultiple, CBaseFilter );
	DECLARE_MAPENTITY();

private:
	filter_t	m_nFilterType;
	string_t	m_iFilterName[MAX_FILTERS];
	EHANDLE		m_hFilter[MAX_FILTERS];

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );
	bool PassesDamageFilterImpl(CBaseEntity *pCaller, const CTakeDamageInfo &info);
	void Activate(void);

	bool BloodAllowed( CBaseEntity *pCaller, const CTakeDamageInfo &info );
	bool PassesFinalDamageFilter( CBaseEntity *pCaller, const CTakeDamageInfo &info );
	bool DamageMod( CBaseEntity *pCaller, CTakeDamageInfo &info );
};

LINK_ENTITY_TO_CLASS(filter_multi, CFilterMultiple);

BEGIN_MAPENTITY( CFilterMultiple, MAPENT_FILTERCLASS )

	// Keys
	DEFINE_KEYFIELD_AUTO( m_nFilterType, "FilterType" ),

	DEFINE_KEYFIELD(m_iFilterName[0], FIELD_STRING, "Filter01"),
	DEFINE_KEYFIELD(m_iFilterName[1], FIELD_STRING, "Filter02"),
	DEFINE_KEYFIELD(m_iFilterName[2], FIELD_STRING, "Filter03"),
	DEFINE_KEYFIELD(m_iFilterName[3], FIELD_STRING, "Filter04"),
	DEFINE_KEYFIELD(m_iFilterName[4], FIELD_STRING, "Filter05"),
	DEFINE_KEYFIELD(m_iFilterName[5], FIELD_STRING, "Filter06"),
	DEFINE_KEYFIELD(m_iFilterName[6], FIELD_STRING, "Filter07"),
	DEFINE_KEYFIELD(m_iFilterName[7], FIELD_STRING, "Filter08"),
	DEFINE_KEYFIELD(m_iFilterName[8], FIELD_STRING, "Filter09"),
	DEFINE_KEYFIELD(m_iFilterName[9], FIELD_STRING, "Filter10"),

END_MAPENTITY()



//------------------------------------------------------------------------------
// Purpose : Called after all entities have been loaded
//------------------------------------------------------------------------------
void CFilterMultiple::Activate( void )
{
	BaseClass::Activate();
	
	// We may reject an entity specified in the array of names, but we want the array of valid filters to be contiguous!
	int nNextFilter = 0;

	// Get handles to my filter entities
	for ( int i = 0; i < MAX_FILTERS; i++ )
	{
		if ( m_iFilterName[i] != NULL_STRING )
		{
			CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_iFilterName[i] );
			CBaseFilter *pFilter = dynamic_cast<CBaseFilter *>(pEntity);
			if ( pFilter == NULL )
			{
				Warning("filter_multi: Tried to add entity (%s) which is not a filter entity!\n", STRING( m_iFilterName[i] ) );
				continue;
			}
			else if ( pFilter == this )
			{
				Warning("filter_multi: Tried to add itself!\n");
				continue;
			}

			// Take this entity and increment out array pointer
			m_hFilter[nNextFilter] = pFilter;
			nNextFilter++;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity passes our filter, false if not.
// Input  : pEntity - Entity to test.
//-----------------------------------------------------------------------------
bool CFilterMultiple::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	// Test against each filter
	if (m_nFilterType == FILTER_AND)
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (!pFilter->PassesFilter( pCaller, pEntity ) )
				{
					return false;
				}
			}
		}
		return true;
	}
	else  // m_nFilterType == FILTER_OR
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (pFilter->PassesFilter( pCaller, pEntity ) )
				{
					return true;
				}
			}
		}
		return false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity passes our filter, false if not.
// Input  : pEntity - Entity to test.
//-----------------------------------------------------------------------------
bool CFilterMultiple::PassesDamageFilterImpl(CBaseEntity *pCaller, const CTakeDamageInfo &info)
{
	// Test against each filter
	if (m_nFilterType == FILTER_AND)
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (!pFilter->PassesDamageFilter(pCaller, info))
				{
					return false;
				}
			}
		}
		return true;
	}
	else  // m_nFilterType == FILTER_OR
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (pFilter->PassesDamageFilter(pCaller, info))
				{
					return true;
				}
			}
		}
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if blood should be allowed, false if not.
// Input  : pEntity - Entity to test.
//-----------------------------------------------------------------------------
bool CFilterMultiple::BloodAllowed( CBaseEntity *pCaller, const CTakeDamageInfo &info )
{
	// Test against each filter
	if (m_nFilterType == FILTER_AND)
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (!pFilter->BloodAllowed(pCaller, info))
				{
					return false;
				}
			}
		}
		return true;
	}
	else  // m_nFilterType == FILTER_OR
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (pFilter->BloodAllowed(pCaller, info))
				{
					return true;
				}
			}
		}
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the entity passes our filter, false if not.
// Input  : pEntity - Entity to test.
//-----------------------------------------------------------------------------
bool CFilterMultiple::PassesFinalDamageFilter( CBaseEntity *pCaller, const CTakeDamageInfo &info )
{
	// Test against each filter
	if (m_nFilterType == FILTER_AND)
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (!pFilter->PassesFinalDamageFilter(pCaller, info))
				{
					return false;
				}
			}
		}
		return true;
	}
	else  // m_nFilterType == FILTER_OR
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (pFilter->PassesFinalDamageFilter(pCaller, info))
				{
					return true;
				}
			}
		}
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if damage should be modded, false if not.
// Input  : pEntity - Entity to test.
//-----------------------------------------------------------------------------
bool CFilterMultiple::DamageMod( CBaseEntity *pCaller, CTakeDamageInfo &info )
{
	// Test against each filter
	if (m_nFilterType == FILTER_AND)
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (!pFilter->DamageMod(pCaller, info))
				{
					return false;
				}
			}
		}
		return true;
	}
	else  // m_nFilterType == FILTER_OR
	{
		for (int i=0;i<MAX_FILTERS;i++)
		{
			if (m_hFilter[i] != NULL)
			{
				CBaseFilter* pFilter = (CBaseFilter *)(m_hFilter[i].Get());
				if (pFilter->DamageMod(pCaller, info))
				{
					return true;
				}
			}
		}
		return false;
	}
}

// ###################################################################
//	> FilterName
// ###################################################################
class CFilterName : public CBaseFilter
{
	DECLARE_CLASS( CFilterName, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	string_t m_iFilterName;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (!pEntity)
			return false;
		
		// special check for !player as GetEntityName for player won't return "!player" as a name
		if (FStrEq(STRING(m_iFilterName), "!player"))
		{
			return pEntity->IsPlayer();
		}
		else
		{
			return pEntity->NameMatches( STRING(m_iFilterName) );
		}
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iFilterName = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_name, CFilterName );

BEGIN_MAPENTITY( CFilterName, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterName, "filtername" ),

END_MAPENTITY()

// ###################################################################
//	> FilterModel
// ###################################################################
class CFilterModel : public CBaseFilter
{
	DECLARE_CLASS( CFilterModel, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	string_t m_iFilterModel;
	string_t m_strFilterSkin;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (FStrEq(STRING(m_strFilterSkin), "-1") /*m_strFilterSkin == NULL_STRING|| FStrEq(STRING(m_strFilterSkin), "")*/)
			return Matcher_NamesMatch(STRING(m_iFilterModel), STRING(pEntity->GetModelName()));
		else if (pEntity->GetBaseAnimating())
		{
			//DevMsg("Skin isn't null\n");
			return Matcher_NamesMatch(STRING(m_iFilterModel), STRING(pEntity->GetModelName())) && Matcher_Match(STRING(m_strFilterSkin), pEntity->GetBaseAnimating()->GetSkin());
		}
		return false;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iFilterModel = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_model, CFilterModel );

BEGIN_MAPENTITY( CFilterModel, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterModel, "model" ),
	DEFINE_KEYFIELD_AUTO( m_iFilterModel, "filtermodel" ),
	DEFINE_KEYFIELD_AUTO( m_iFilterModel, "filtername" ),
	DEFINE_KEYFIELD_AUTO( m_strFilterSkin, "skin" ),

END_MAPENTITY()

// ###################################################################
//	> FilterContext
// ###################################################################
class CFilterContext : public CBaseFilter
{
	DECLARE_CLASS( CFilterContext, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	bool m_bAny;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		bool passes = false;
		ResponseContext_t curcontext;
		const char *contextvalue;
		for (int i = 0; i < GetContextCount(); i++)
		{
			curcontext = m_ResponseContexts[i];
			if (!pEntity->HasContext(STRING(curcontext.m_iszName), NULL))
			{
				if (m_bAny)
					continue;
				else
					return false;
			}

			contextvalue = pEntity->GetContextValue(STRING(curcontext.m_iszName));
			if (Matcher_NamesMatch(STRING(m_ResponseContexts[i].m_iszValue), contextvalue))
			{
				passes = true;
				if (m_bAny)
					break;
			}
			else if (!m_bAny)
			{
				return false;
			}
		}

		return passes;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		m_ResponseContexts.RemoveAll();
		AddContext(inputdata.value.String());
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_context, CFilterContext );

BEGIN_MAPENTITY( CFilterContext, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_bAny, "any" ),

END_MAPENTITY()

// ###################################################################
//	> FilterClass
// ###################################################################
class CFilterClass : public CBaseFilter
{
	DECLARE_CLASS( CFilterClass, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	string_t m_iFilterClass;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		return pEntity->ClassMatches( STRING(m_iFilterClass) );
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iFilterClass = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_class, CFilterClass );

BEGIN_MAPENTITY( CFilterClass, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterClass, "filterclass" ),

END_MAPENTITY()


// ###################################################################
//	> FilterTeam
// ###################################################################
class CFilterTeam : public CBaseFilter
{
	DECLARE_CLASS( CFilterTeam, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	int		m_iFilterTeam;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
	 	return ( pEntity != NULL && pEntity->GetTeamNumber() == m_iFilterTeam );
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_INTEGER);
		m_iFilterTeam = inputdata.value.Int();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_team, CFilterTeam );

BEGIN_MAPENTITY( CFilterTeam, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterTeam, "filterteam" ),

END_MAPENTITY()


// ###################################################################
//	> FilterMassGreater
// ###################################################################
class CFilterMassGreater : public CBaseFilter
{
	DECLARE_CLASS( CFilterMassGreater, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	float m_fFilterMass;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if ( pEntity->VPhysicsGetObject() == NULL )
			return false;

		return ( pEntity->VPhysicsGetObject()->GetMass() > m_fFilterMass );
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_FLOAT);
		m_fFilterMass = inputdata.value.Float();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_mass_greater, CFilterMassGreater );

BEGIN_MAPENTITY( CFilterMassGreater, MAPENT_FILTERCLASS )

// Keyfields
DEFINE_KEYFIELD_AUTO( m_fFilterMass, "filtermass" ),

END_MAPENTITY()


// ###################################################################
//	> FilterDamageType
// ###################################################################
class CFilterDamageType : public CBaseFilter
{
	DECLARE_CLASS( CFilterDamageType, CBaseFilter );
	DECLARE_MAPENTITY();

protected:

	bool PassesFilterImpl(CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		ASSERT( false );
	 	return true;
	}

	bool PassesDamageFilterImpl(CBaseEntity *pCaller, const CTakeDamageInfo &info)
	{
		switch (m_iFilterType)
		{
			case 1:		return (info.GetDamageType() & m_iDamageType) != 0;
			case 2:
			{
				uint64 iRecvDT = info.GetDamageType();
				uint64 iOurDT = m_iDamageType;
				while (iRecvDT)
				{
					if (iRecvDT & iOurDT)
						return true;

					iRecvDT >>= 1; iOurDT >>= 1;
				}
				return false;
			} break;
		}
		//Tony; these are bitflags. check them as so.
		return (((info.GetDamageType() & ~DMG_DIRECT) & m_iDamageType) == m_iDamageType);
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_INTEGER);
		m_iDamageType = inputdata.value.Int();
	}

	bool KeyValue( const char *szKeyName, const char *szValue )
	{
		if (FStrEq( szKeyName, "damageor" ) || FStrEq( szKeyName, "damagepresets" ))
		{
			m_iDamageType |= strtoull( szValue, NULL, 10 );
		}
		else
			return BaseClass::KeyValue( szKeyName, szValue );

		return true;
	}

	uint64 m_iDamageType;
	int m_iFilterType;
};

LINK_ENTITY_TO_CLASS( filter_damage_type, CFilterDamageType );

BEGIN_MAPENTITY( CFilterDamageType, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iDamageType, "damagetype" ),
	DEFINE_KEYFIELD_AUTO( m_iFilterType, "FilterType" ),

END_MAPENTITY()

// ###################################################################
//	> CFilterEnemy
// ###################################################################

#define SF_FILTER_ENEMY_NO_LOSE_AQUIRED	(1<<0)

class CFilterEnemy : public CBaseFilter
{
	DECLARE_CLASS( CFilterEnemy, CBaseFilter );
		// NOT SAVED	
		// m_iszPlayerName
	DECLARE_MAPENTITY();

public:

	virtual bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );
	virtual bool PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info );

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iszEnemyName = inputdata.value.StringID();
	}

private:

	bool	PassesNameFilter( CBaseEntity *pCaller );
	bool	PassesProximityFilter( CBaseEntity *pCaller, CBaseEntity *pEnemy );
	bool	PassesMobbedFilter( CBaseEntity *pCaller, CBaseEntity *pEnemy );

	string_t	m_iszEnemyName;				// Name or classname
	float		m_flRadius;					// Radius (enemies are acquired at this range)
	float		m_flOuterRadius;			// Outer radius (enemies are LOST at this range)
	int		m_nMaxSquadmatesPerEnemy;	// Maximum number of squadmates who may share the same enemy
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFilterEnemy::PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	if ( pCaller == NULL || pEntity == NULL )
		return false;

	// If asked to, we'll never fail to pass an already acquired enemy
	//	This allows us to use test criteria to initially pick an enemy, then disregard the test until a new enemy comes along
	if ( HasSpawnFlags( SF_FILTER_ENEMY_NO_LOSE_AQUIRED ) && ( pEntity == pCaller->GetEnemy() ) )
		return true;

	// This is a little weird, but it's saying that if we're not the entity we're excluding the filter to, then just pass it throughZ
	if ( PassesNameFilter( pEntity ) == false )
		return true;

	if ( PassesProximityFilter( pCaller, pEntity ) == false )
		return false;

	// NOTE: This can result in some weird NPC behavior if used improperly
	if ( PassesMobbedFilter( pCaller, pEntity ) == false )
		return false;

	// The filter has been passed, meaning:
	//	- If we wanted all criteria to fail, they have
	//  - If we wanted all criteria to succeed, they have

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFilterEnemy::PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
{
	// NOTE: This function has no meaning to this implementation of the filter class!
	Assert( 0 );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Tests the enemy's name or classname
// Input  : *pEnemy - Entity being assessed
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFilterEnemy::PassesNameFilter( CBaseEntity *pEnemy )
{
	// If there is no name specified, we're not using it
	if ( m_iszEnemyName	== NULL_STRING )
		return true;

	if ( m_iszEnemyName == gm_isz_name_player )
	{
		if ( pEnemy->IsPlayer() )
		{
			if ( m_bNegated )
				return false;

			return true;
		}
	}

	// May be either a targetname or classname
	bool bNameOrClassnameMatches = ( pEnemy->NameMatches(STRING(m_iszEnemyName)) || pEnemy->ClassMatches(STRING(m_iszEnemyName)) );

	// We only leave this code block in a state meaning we've "succeeded" in any context
	if ( m_bNegated )
	{
		// We wanted the names to not match, but they did
		if ( bNameOrClassnameMatches )
			return false;
	}
	else
	{
		// We wanted them to be the same, but they weren't
		if ( bNameOrClassnameMatches == false )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Tests the enemy's proximity to the caller's position
// Input  : *pCaller - Entity assessing the target
//			*pEnemy - Entity being assessed
// Output : Returns true if potential enemy passes this filter stage
//-----------------------------------------------------------------------------
bool CFilterEnemy::PassesProximityFilter( CBaseEntity *pCaller, CBaseEntity *pEnemy )
{
	// If there is no radius specified, we're not testing it
	if ( m_flRadius <= 0.0f )
		return true;

	// We test the proximity differently when we've already picked up this enemy before
	bool bAlreadyEnemy = ( pCaller->GetEnemy() == pEnemy );

	// Get our squared length to the enemy from the caller
	float flDistToEnemySqr = ( pCaller->GetAbsOrigin() - pEnemy->GetAbsOrigin() ).LengthSqr();

	// Two radii are used to control oscillation between true/false cases
	// The larger radius is either specified or defaulted to be double or half the size of the inner radius
	float flLargerRadius = m_flOuterRadius;
	if ( flLargerRadius == 0 )
	{
		flLargerRadius = ( m_bNegated ) ? (m_flRadius*0.5f) : (m_flRadius*2.0f);
	}

	float flSmallerRadius = m_flRadius;
	if ( flSmallerRadius > flLargerRadius )
	{
		::V_swap( flLargerRadius, flSmallerRadius );
	}

	float flDist;	
	if ( bAlreadyEnemy )
	{
		flDist = ( m_bNegated ) ? flSmallerRadius : flLargerRadius;
	}
	else
	{
		flDist = ( m_bNegated ) ? flLargerRadius : flSmallerRadius;
	}

	// Test for success
	if ( flDistToEnemySqr <= (flDist*flDist) )
	{
		// We wanted to fail but didn't
		if ( m_bNegated )
			return false;

		return true;
	}
	
	// We wanted to succeed but didn't
	if ( m_bNegated == false )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to govern how many squad members can target any given entity
// Input  : *pCaller - Entity assessing the target
//			*pEnemy - Entity being assessed
// Output : Returns true if potential enemy passes this filter stage
//-----------------------------------------------------------------------------
bool CFilterEnemy::PassesMobbedFilter( CBaseEntity *pCaller, CBaseEntity *pEnemy )
{
	// Must be a valid candidate
	CAI_BaseNPC *pNPC = pCaller->MyNPCPointer();
	if ( pNPC == NULL || pNPC->GetSquad() == NULL )
		return true;

	// Make sure we're checking for this
	if ( m_nMaxSquadmatesPerEnemy <= 0 )
		return true;

	AISquadIter_t iter;
	int nNumMatchingSquadmates = 0;
	
	// Look through our squad members to see how many of them are already mobbing this entity
	for ( CAI_BaseNPC *pSquadMember = pNPC->GetSquad()->GetFirstMember( &iter ); pSquadMember != NULL; pSquadMember = pNPC->GetSquad()->GetNextMember( &iter ) )
	{
		// Disregard ourself
		if ( pSquadMember == pNPC )
			continue;

		// If the enemies match, count it
		if ( pSquadMember->GetEnemy() == pEnemy )
		{
			nNumMatchingSquadmates++;

			// If we're at or passed the max we stop
			if ( nNumMatchingSquadmates >= m_nMaxSquadmatesPerEnemy )
			{
				// We wanted to find more than allowed and we did
				if ( m_bNegated )
					return true;
				
				// We wanted to be less but we're not
				return false;
			}
		}
	}

	// We wanted to find more than the allowed amount but we didn't
	if ( m_bNegated )
		return false;

	return true;
}

LINK_ENTITY_TO_CLASS( filter_enemy, CFilterEnemy );

BEGIN_MAPENTITY( CFilterEnemy, MAPENT_FILTERCLASS )
	
	DEFINE_KEYFIELD_AUTO( m_iszEnemyName, "filtername" ),
	DEFINE_KEYFIELD_AUTO( m_flRadius, "filter_radius" ),
	DEFINE_KEYFIELD_AUTO( m_flOuterRadius, "filter_outer_radius" ),
	DEFINE_KEYFIELD_AUTO( m_nMaxSquadmatesPerEnemy, "filter_max_per_enemy" ),

END_MAPENTITY()

// ###################################################################
//	> CFilterSquad
// ###################################################################
class CFilterSquad : public CBaseFilter
{
	DECLARE_CLASS( CFilterSquad, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	string_t m_iFilterName;
	bool m_bAllowSilentSquadMembers;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pEntity && pNPC)
		{
			if (pNPC->GetSquad() && Matcher_NamesMatch(STRING(m_iFilterName), pNPC->GetSquad()->GetName()))
			{
				if (CAI_Squad::IsSilentMember(pNPC))
				{
					return m_bAllowSilentSquadMembers;
				}
				else
				{
					return true;
				}
			}
		}

		return false;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iFilterName = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_squad, CFilterSquad );

BEGIN_MAPENTITY( CFilterSquad, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterName, "filtername" ),
	DEFINE_KEYFIELD_AUTO( m_bAllowSilentSquadMembers, "allowsilentmembers" ),

END_MAPENTITY()

extern bool ReadUnregisteredKeyfields(CBaseEntity *pTarget, const char *szKeyName, variant_t *variant);

// ###################################################################
//	> CFilterKeyfield
// ###################################################################
class CFilterKeyfield : public CBaseFilter
{
	DECLARE_CLASS( CFilterKeyfield, CBaseFilter );
	DECLARE_MAPENTITY();

public:
	string_t m_iFilterKey;
	string_t m_iFilterValue;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		variant_t var;
		bool found = (pEntity->ReadKeyField(STRING(m_iFilterKey), &var) || ReadUnregisteredKeyfields(pEntity, STRING(m_iFilterKey), &var));
		return m_iFilterValue != NULL_STRING ? Matcher_Match(STRING(m_iFilterValue), var.String()) : found;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iFilterKey = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_keyfield, CFilterKeyfield );

BEGIN_MAPENTITY( CFilterKeyfield , MAPENT_FILTERCLASS)

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterKey, "keyname" ),
	DEFINE_KEYFIELD_AUTO( m_iFilterValue, "value" ),

END_MAPENTITY()

// ###################################################################
//	> CFilterRelationship
// ###################################################################
class CFilterRelationship : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterRelationship, CBaseFilter );
	DECLARE_MAPENTITY();

	Disposition_t m_iDisposition;
	string_t m_iszPriority; // string_t to support matchers
	bool m_bInvertTarget;
	bool m_bReciprocal;
	EHANDLE m_hTarget;

	bool RelationshipPasses(CBaseCombatCharacter *pBCC, CBaseEntity *pTarget)
	{
		if (!pBCC || !pTarget)
			return m_iDisposition == D_NU;

		Disposition_t disposition = pBCC->IRelationType(pTarget);
		int priority = pBCC->IRelationPriority(pTarget);

		bool passes = (disposition == m_iDisposition);
		if (!passes)
			return false;

		if (m_iszPriority != NULL_STRING)
		{
			passes = Matcher_Match(STRING(m_iszPriority), priority);
		}

		return passes;
	}

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CBaseEntity *pSubject = NULL;
		if (m_target != NULL_STRING)
		{
			if (!m_hTarget)
			{
				m_hTarget = gEntList.FindEntityGeneric(NULL, STRING(m_target), pCaller, pEntity, pCaller);
			}
			pSubject = m_hTarget;
		}

		if (!pSubject)
			pSubject = pCaller;

		// No subject or entity, cannot continue
		if (!pSubject || !pEntity)
			return m_iDisposition == D_NU;

		CBaseCombatCharacter *pBCC1 = !m_bInvertTarget ? pSubject->MyCombatCharacterPointer() : pEntity->MyCombatCharacterPointer();
		CBaseEntity *pTarget = m_bInvertTarget ? pSubject : pEntity;
		if (!pBCC1)
		{
			//Warning("Error: %s subject %s is not a character that uses relationships!\n", GetDebugName(), !m_bInvertTarget ? pSubject->GetDebugName() : pEntity->GetDebugName());
			return m_iDisposition == D_NU;
		}

		bool passes = RelationshipPasses(pBCC1, pTarget);
		if (m_bReciprocal)
			passes = RelationshipPasses(pTarget->MyCombatCharacterPointer(), pBCC1);

		return passes;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_target = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_relationship, CFilterRelationship );

BEGIN_MAPENTITY( CFilterRelationship, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iDisposition, "disposition" ),
	DEFINE_KEYFIELD_AUTO( m_iszPriority, "rank" ),
	DEFINE_KEYFIELD_AUTO( m_bInvertTarget, "inverttarget" ),
	DEFINE_KEYFIELD_AUTO( m_bReciprocal, "Reciprocal" ),
	DEFINE_FIELD( m_hTarget,			FIELD_EHANDLE ),

END_MAPENTITY()

// ###################################################################
//	> CFilterClassify
// ###################################################################
class CFilterClassify : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterClassify, CBaseFilter );
	DECLARE_MAPENTITY();

	Class_T m_iFilterClassify;

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		return pEntity->Classify() == m_iFilterClassify;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_INTEGER);
		m_iFilterClassify = (Class_T)inputdata.value.Int();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_classify, CFilterClassify );

BEGIN_MAPENTITY( CFilterClassify, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterClassify, "filterclassify" ),

END_MAPENTITY()

// ###################################################################
//	> CFilterCriteria
// ###################################################################
class CFilterCriteria : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterCriteria, CBaseFilter );
	DECLARE_MAPENTITY();

	bool m_bAny;
	bool m_bFull; // All criteria functions are gathered

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (!pEntity)
			return false;

		AI_CriteriaSet set;
		pEntity->ModifyOrAppendCriteria( set );
		if (m_bFull)
		{
			// Meeets the full wrath of the response criteria
			CBasePlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
			if( pPlayer )
				pPlayer->ModifyOrAppendPlayerCriteria( set );

			pEntity->ReAppendContextCriteria( set );
		}

		bool passes = false;
		const char *contextname;
		const char *contextvalue;
		const char *matchingvalue;
		for (int i = 0; i < set.GetCount(); i++)
		{
			contextname = set.GetName(i);
			contextvalue = set.GetValue(i);

			matchingvalue = GetContextValue(contextname);
			if (matchingvalue == NULL)
			{
				if (m_bAny)
					continue;
				else
					return false;
			}
			else
			{
				if (Matcher_NamesMatch(matchingvalue, contextvalue))
				{
					passes = true;
					if (m_bAny)
						break;
				}
				else if (!m_bAny)
				{
					return false;
				}
			}
		}

		return passes;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		m_ResponseContexts.RemoveAll();
		AddContext(inputdata.value.String());
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_criteria, CFilterCriteria );

BEGIN_MAPENTITY( CFilterCriteria, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_bAny, "any" ),
	DEFINE_KEYFIELD_AUTO( m_bFull, "full" ),

END_MAPENTITY()

extern bool TestEntityTriggerIntersection_Accurate( CBaseEntity *pTrigger, CBaseEntity *pEntity );

// ###################################################################
//	> CFilterInVolume
// Passes when the entity is within the specified volume.
// ###################################################################
class CFilterInVolume : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterInVolume, CBaseFilter );
	DECLARE_MAPENTITY();

	string_t m_iszVolumeTester;

	void Spawn()
	{
		BaseClass::Spawn();

		// Assume no string = use activator
		if (m_iszVolumeTester == NULL_STRING)
			m_iszVolumeTester = gm_isz_name_activator;
	}

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CBaseEntity *pVolume = gEntList.FindEntityByNameNearest(STRING(m_target), pEntity->GetLocalOrigin(), 0, this, pEntity, pCaller);
		if (!pVolume)
		{
			Msg("%s cannot find volume %s\n", GetDebugName(), STRING(m_target));
			return false;
		}

		CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, STRING(m_iszVolumeTester), this, pEntity, pCaller);
		if (pTarget)
			return TestEntityTriggerIntersection_Accurate(pVolume, pTarget);
		else
		{
			Msg("%s cannot find target entity %s, returning false\n", GetDebugName(), STRING(m_iszVolumeTester));
			return false;
		}
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iszVolumeTester = inputdata.value.StringID();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_involume, CFilterInVolume );

BEGIN_MAPENTITY( CFilterInVolume, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iszVolumeTester, "tester" ),

END_MAPENTITY()

// ###################################################################
//	> CFilterSurfaceProp
// ###################################################################
class CFilterSurfaceData : public CBaseFilter
{
public:
	DECLARE_CLASS( CFilterSurfaceData, CBaseFilter );
	DECLARE_MAPENTITY();

	string_t m_iFilterSurface;
	int m_iSurfaceIndex;

	enum
	{
		SURFACETYPE_SURFACEPROP,
		SURFACETYPE_GAMEMATERIAL,
	};

	// Gets the surfaceprop's game material and filters by that.
	int m_iSurfaceType;

	void ParseSurfaceIndex()
	{
		m_iSurfaceIndex = physprops->GetSurfaceIndex(STRING(m_iFilterSurface));

		switch (m_iSurfaceType)
		{
			case SURFACETYPE_GAMEMATERIAL:
			{
				const surfacedata_t *pSurfaceData = physprops->GetSurfaceData(m_iSurfaceIndex);
				if (pSurfaceData)
					m_iSurfaceIndex = pSurfaceData->game.material;
				else
					Warning("Can't get surface data for %s\n", STRING(m_iFilterSurface));
			} break;
		}
	}

	void Activate()
	{
		BaseClass::Activate();
		ParseSurfaceIndex();
	}

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (pEntity->VPhysicsGetObject())
		{
			int iMatIndex = pEntity->VPhysicsGetObject()->GetMaterialIndex();
			switch (m_iSurfaceType)
			{
				case SURFACETYPE_GAMEMATERIAL:
				{
					const surfacedata_t *pSurfaceData = physprops->GetSurfaceData(iMatIndex);
					if (pSurfaceData)
						return m_iSurfaceIndex == pSurfaceData->game.material;
				}
				default:
					return iMatIndex == m_iSurfaceIndex;
			}
		}

		return false;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		m_iFilterSurface = inputdata.value.StringID();
		ParseSurfaceIndex();
	}
};

LINK_ENTITY_TO_CLASS( filter_activator_surfacedata, CFilterSurfaceData );

BEGIN_MAPENTITY( CFilterSurfaceData, MAPENT_FILTERCLASS )

	// Keyfields
	DEFINE_KEYFIELD_AUTO( m_iFilterSurface, "filterstring" ),
	DEFINE_KEYFIELD_AUTO( m_iSurfaceType, "SurfaceType" ),

END_MAPENTITY()

// ===================================================================
// Redirect filters
// 
// Redirects certain data to a specific filter.
// ===================================================================
class CBaseFilterRedirect : public CBaseFilter
{
public:
	DECLARE_CLASS( CBaseFilterRedirect, CBaseFilter );

	inline CBaseEntity *GetTargetFilter()
	{
		// Yes, this hijacks damage filter functionality.
		// It's not like it was using it before anyway.
		return m_hDamageFilter.Get();
	}

	bool RedirectToFilter( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (GetTargetFilter() && pEntity)
		{
			CBaseFilter *pFilter = static_cast<CBaseFilter*>(GetTargetFilter());
			return pFilter->PassesFilter(pCaller, pEntity);
		}

		return pEntity != NULL;
	}

	bool RedirectToDamageFilter( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (GetTargetFilter())
		{
			CBaseFilter *pFilter = static_cast<CBaseFilter*>(GetTargetFilter());
			return pFilter->PassesDamageFilter(pCaller, info);
		}

		return true;
	}

	virtual bool PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		return RedirectToDamageFilter( pCaller, info );
	}

	virtual bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		return RedirectToFilter( pCaller, pEntity );
	}

	virtual bool BloodAllowed( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (GetTargetFilter())
		{
			CBaseFilter *pFilter = static_cast<CBaseFilter*>(GetTargetFilter());
			return pFilter->BloodAllowed(pCaller, info);
		}

		return true;
	}

	virtual bool DamageMod( CBaseEntity *pCaller, CTakeDamageInfo &info )
	{
		if (GetTargetFilter())
		{
			CBaseFilter *pFilter = static_cast<CBaseFilter*>(GetTargetFilter());
			return pFilter->DamageMod( pCaller, info );
		}

		return true;
	}

	void InputSetField( inputdata_t &&inputdata )
	{
		inputdata.value.Convert(FIELD_STRING);
		InputSetDamageFilter(inputdata);
	}

	enum
	{
		REDIRECT_MUST_PASS_TO_DAMAGE_CALLER,	// Must pass to damage caller, if damage is allowed
		REDIRECT_MUST_PASS_TO_ACT,				// Must pass to do action
		REDIRECT_MUST_PASS_ACTIVATORS,			// Each activator must pass this filter
	};
};

// ###################################################################
//	> CFilterRedirectInflictor
// Uses the specified filter to filter by damage inflictor.
// ###################################################################
class CFilterRedirectInflictor : public CBaseFilterRedirect
{
	DECLARE_CLASS( CFilterRedirectInflictor, CBaseFilterRedirect );

public:
	bool PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		return RedirectToFilter(pCaller, info.GetInflictor());
	}
};

LINK_ENTITY_TO_CLASS( filter_redirect_inflictor, CFilterRedirectInflictor );

// ###################################################################
//	> CFilterRedirectWeapon
// Uses the specified filter to filter by either the entity's active weapon or the weapon causing damage,
// depending on the context.
// ###################################################################
class CFilterRedirectWeapon : public CBaseFilterRedirect
{
	DECLARE_CLASS( CFilterRedirectWeapon, CBaseFilterRedirect );

public:
	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		CBaseCombatCharacter *pBCC = pEntity->MyCombatCharacterPointer();
		if (pBCC && pBCC->GetActiveWeapon())
		{
			return RedirectToFilter( pCaller, pBCC->GetActiveWeapon() );
		}

		return false;
	}

	bool PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		// Pass any weapon found in the damage info
		if (info.GetWeapon())
		{
			return RedirectToFilter( pCaller, info.GetWeapon() );
		}

		// Check the attacker's active weapon instead
		if (info.GetAttacker())
		{
			return PassesFilterImpl( pCaller, info.GetAttacker() );
		}

		// No weapon to check
		return false;
	}
};

LINK_ENTITY_TO_CLASS( filter_redirect_weapon, CFilterRedirectWeapon );

// ###################################################################
//	> CFilterRedirectOwner
// Uses the specified filter to filter by owner entity.
// ###################################################################
class CFilterRedirectOwner : public CBaseFilterRedirect
{
	DECLARE_CLASS( CFilterRedirectOwner, CBaseFilterRedirect );

public:
	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (pEntity->GetOwnerEntity())
		{
			return RedirectToFilter(pCaller, pEntity->GetOwnerEntity());
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS( filter_redirect_owner, CFilterRedirectOwner );

// ###################################################################
//	> CFilterDamageTransfer
// Transfers damage to another entity.
// ###################################################################
class CFilterDamageTransfer : public CBaseFilterRedirect
{
	DECLARE_CLASS( CFilterDamageTransfer, CBaseFilterRedirect );
	DECLARE_MAPENTITY();

public:
	void Spawn()
	{
		BaseClass::Spawn();

		// Assume no string = use activator
		if (m_target == NULL_STRING)
			m_target = gm_isz_name_activator;

		// A number less than or equal to 0 is always synonymous with no limit
		if (m_iMaxEntities <= 0)
			m_iMaxEntities = MAX_EDICTS;
	}

	// Some secondary filter modes shouldn't be used in non-final filter passes
	// Always return true on non-standard secondary filter modes
	/*
	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		return true;
	}
	*/

	// A hack because of the way final damage filtering now works.
	bool BloodAllowed( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (!m_bCallerDamageAllowed)
			return false;
		else
			return m_iSecondaryFilterMode == REDIRECT_MUST_PASS_TO_DAMAGE_CALLER && GetTargetFilter() ? RedirectToDamageFilter(pCaller, info) : true;
	}

	// PassesFinalDamageFilter() was created for the express purpose of having filter_damage_transfer function without
	// passing damage on filter checks that don't actually lead to us taking damage in the first place.
	// PassesFinalDamageFilter() is only called in certain base entity functions where we DEFINITELY will take damage otherwise.
	bool PassesFinalDamageFilter( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (m_iSecondaryFilterMode == REDIRECT_MUST_PASS_TO_ACT)
		{
			// Transfer only if the secondary filter passes
			if (!RedirectToDamageFilter(pCaller, info))
			{
				// Otherwise just return the other flag
				return m_bCallerDamageAllowed;
			}
		}

		CBaseEntity *pTarget = gEntList.FindEntityGeneric(NULL, STRING(m_target), this, info.GetAttacker(), pCaller);
		int iNumDamaged = 0;
		while (pTarget)
		{
			// Avoid recursive loops!
			if (pTarget->m_hDamageFilter != this)
			{
				CTakeDamageInfo info2 = info;

				// Adjust damage position stuff
				if (m_bAdjustDamagePosition)
				{
					info2.SetDamagePosition(pTarget->GetAbsOrigin() + (pCaller->GetAbsOrigin() - info.GetDamagePosition()));

					if (pCaller->IsCombatCharacter() && pTarget->IsCombatCharacter())
						pTarget->MyCombatCharacterPointer()->SetLastHitGroup(pCaller->MyCombatCharacterPointer()->LastHitGroup());
				}

				if (m_iSecondaryFilterMode != REDIRECT_MUST_PASS_ACTIVATORS || RedirectToFilter(pCaller, pTarget))
				{
					pTarget->TakeDamage(info2);
					iNumDamaged++;
				}
			}

			if (iNumDamaged < m_iMaxEntities)
				pTarget = gEntList.FindEntityGeneric(pTarget, STRING(m_target), this, info.GetAttacker(), pCaller);
			else
				break;
		}

		// We've transferred the damage, now determine whether the caller should take damage.
		// Boolean surpasses all.
		if (!m_bCallerDamageAllowed)
			return false;
		else
			return m_iSecondaryFilterMode == REDIRECT_MUST_PASS_TO_DAMAGE_CALLER && GetTargetFilter() ? RedirectToDamageFilter(pCaller, info) : true;
	}

	/*
	void InputSetTarget( inputdata_t &&inputdata )
	{
		m_target = inputdata.value.StringID();
		m_hTarget = NULL;
	}
	*/

	inline CBaseEntity *GetTarget(CBaseEntity *pCaller, CBaseEntity *pActivator)
	{
		return gEntList.FindEntityGeneric(NULL, STRING(m_target), this, pActivator, pCaller);
	}

	//EHANDLE m_hTarget;

	bool m_bAdjustDamagePosition;

	// See CBaseRedirectFilter enum for more info
	int m_iSecondaryFilterMode;

	// If enabled, the caller can be damaged after the transfer. If disabled, the caller cannot.
	bool m_bCallerDamageAllowed;

	int m_iMaxEntities = MAX_EDICTS;
};

LINK_ENTITY_TO_CLASS( filter_damage_transfer, CFilterDamageTransfer );

BEGIN_MAPENTITY( CFilterDamageTransfer, MAPENT_FILTERCLASS )

	//DEFINE_FIELD( m_hTarget,	FIELD_EHANDLE ),
	DEFINE_KEYFIELD_AUTO( m_bAdjustDamagePosition, "AdjustDamagePosition" ),
	DEFINE_KEYFIELD_AUTO( m_iMaxEntities, "MaxEntities" ),
	DEFINE_KEYFIELD_AUTO( m_iSecondaryFilterMode, "SecondaryFilterMode" ),
	DEFINE_KEYFIELD_AUTO( m_bCallerDamageAllowed, "CallerDamageAllowed" ),

END_MAPENTITY()

// ###################################################################
//	> CFilterBloodControl
// Takes advantage of hacks created for filter_damage_transfer to control blood.
// ###################################################################
class CFilterBloodControl : public CBaseFilterRedirect
{
	DECLARE_CLASS( CFilterBloodControl, CBaseFilterRedirect );
	DECLARE_MAPENTITY();
public:
	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (GetTargetFilter() && m_bSecondaryFilterIsDamageFilter)
			return RedirectToFilter(pCaller, pEntity);

		return true;
	}

	bool BloodAllowed( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (m_bBloodDisabled)
			return false;

		return GetTargetFilter() ? RedirectToDamageFilter(pCaller, info) : true;
	}

	void InputDisableBlood( inputdata_t &&inputdata ) { m_bBloodDisabled = true; }
	void InputEnableBlood( inputdata_t &&inputdata ) { m_bBloodDisabled = false; }

	bool m_bBloodDisabled;

	// Uses the secondary filter as a damage filter instead of just a blood filter
	bool m_bSecondaryFilterIsDamageFilter;
};

LINK_ENTITY_TO_CLASS( filter_blood_control, CFilterBloodControl );

BEGIN_MAPENTITY( CFilterBloodControl, MAPENT_FILTERCLASS )

	DEFINE_KEYFIELD_AUTO( m_bBloodDisabled, "BloodDisabled" ),
	DEFINE_KEYFIELD_AUTO( m_bSecondaryFilterIsDamageFilter, "SecondaryFilterMode" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisableBlood", InputDisableBlood ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableBlood", InputEnableBlood ),

END_MAPENTITY()

// ###################################################################
//	> CFilterDamageMod
// Modifies damage.
// ###################################################################
class CFilterDamageMod : public CBaseFilterRedirect
{
public:
	DECLARE_CLASS( CFilterDamageMod, CBaseFilterRedirect );
	DECLARE_MAPENTITY();
	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (GetTargetFilter() && m_iSecondaryFilterMode == REDIRECT_MUST_PASS_TO_DAMAGE_CALLER)
			return RedirectToFilter(pCaller, pEntity);

		return true;
	}

	bool PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (GetTargetFilter() && m_iSecondaryFilterMode == REDIRECT_MUST_PASS_TO_DAMAGE_CALLER)
			return RedirectToDamageFilter( pCaller, info );

		return true;
	}

	bool DamageMod( CBaseEntity *pCaller, CTakeDamageInfo &info )
	{
		if (GetTargetFilter())
		{
			bool bPass = true;

			switch (m_iSecondaryFilterMode)
			{
				case REDIRECT_MUST_PASS_TO_DAMAGE_CALLER:
				case REDIRECT_MUST_PASS_TO_ACT:				bPass = (RedirectToDamageFilter( pCaller, info )); break;

				case REDIRECT_MUST_PASS_ACTIVATORS:			bPass = (info.GetAttacker() && RedirectToFilter(pCaller, info.GetAttacker())); break;
			}

			if (!bPass)
				return false;
		}

		if (m_flDamageMultiplier != 1.0f)
			info.ScaleDamage(m_flDamageMultiplier);
		if (m_flDamageAddend != 0.0f)
			info.AddDamage(m_flDamageAddend);

		if (m_iDamageBitsAdded != 0)
			info.AddDamageType(m_iDamageBitsAdded);
		if (m_iDamageBitsRemoved != 0)
			info.AddDamageType(~m_iDamageBitsRemoved);

		if (m_iszNewAttacker != NULL_STRING)
		{
			if (!m_hNewAttacker)
				m_hNewAttacker = gEntList.FindEntityByName(NULL, m_iszNewAttacker, this, info.GetAttacker(), pCaller);
			info.SetAttacker(m_hNewAttacker);
		}
		if (m_iszNewInflictor != NULL_STRING)
		{
			if (!m_hNewInflictor)
				m_hNewInflictor = gEntList.FindEntityByName(NULL, m_iszNewInflictor, this, info.GetAttacker(), pCaller);
			info.SetInflictor(m_hNewInflictor);
		}
		if (m_iszNewWeapon != NULL_STRING)
		{
			if (!m_hNewWeapon)
				m_hNewWeapon = gEntList.FindEntityByName(NULL, m_iszNewWeapon, this, info.GetAttacker(), pCaller);
			info.SetWeapon(m_hNewWeapon);
		}

		return true;
	}

	void InputSetNewAttacker( inputdata_t &&inputdata ) { m_iszNewAttacker = inputdata.value.StringID(); m_hNewAttacker = NULL; }
	void InputSetNewInflictor( inputdata_t &&inputdata ) { m_iszNewInflictor = inputdata.value.StringID(); m_hNewInflictor = NULL; }
	void InputSetNewWeapon( inputdata_t &&inputdata ) { m_iszNewWeapon = inputdata.value.StringID(); m_hNewWeapon = NULL; }

	float m_flDamageMultiplier	= 1.0f;
	float m_flDamageAddend;
	uint64 m_iDamageBitsAdded;
	uint64 m_iDamageBitsRemoved;

	string_t m_iszNewAttacker;		EHANDLE m_hNewAttacker;
	string_t m_iszNewInflictor;		EHANDLE m_hNewInflictor;
	string_t m_iszNewWeapon;		EHANDLE m_hNewWeapon;

	// See CBaseRedirectFilter enum for more info
	int m_iSecondaryFilterMode;
};

LINK_ENTITY_TO_CLASS( filter_damage_mod, CFilterDamageMod );

BEGIN_MAPENTITY( CFilterDamageMod, MAPENT_FILTERCLASS )

	DEFINE_KEYFIELD_AUTO( m_iszNewAttacker, "NewAttacker" ),
	DEFINE_KEYFIELD_AUTO( m_iszNewInflictor, "NewInflictor" ),
	DEFINE_KEYFIELD_AUTO( m_iszNewWeapon, "NewWeapon" ),
	DEFINE_FIELD( m_hNewAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hNewInflictor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hNewWeapon, FIELD_EHANDLE ),

	DEFINE_INPUT( m_flDamageMultiplier,	FIELD_FLOAT, "SetDamageMultiplier" ),
	DEFINE_INPUT( m_flDamageAddend,		FIELD_FLOAT, "SetDamageAddend" ),
	DEFINE_INPUT( m_iDamageBitsAdded,	FIELD_INTEGER64, "SetDamageBitsAdded" ),
	DEFINE_INPUT( m_iDamageBitsRemoved,	FIELD_INTEGER64, "SetDamageBitsRemoved" ),

	DEFINE_KEYFIELD_AUTO( m_iSecondaryFilterMode, "SecondaryFilterMode" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetNewAttacker", InputSetNewAttacker ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetNewInflictor", InputSetNewInflictor ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetNewWeapon", InputSetNewWeapon ),

END_MAPENTITY()

// ###################################################################
//	> CFilterDamageLogic
// Fires outputs from damage information.
// ###################################################################
class CFilterDamageLogic : public CBaseFilterRedirect
{
	DECLARE_CLASS( CFilterDamageLogic, CBaseFilterRedirect );
	DECLARE_MAPENTITY();
public:
	bool PassesFinalDamageFilter( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		bool bPassesFilter = !GetTargetFilter() || RedirectToDamageFilter( pCaller, info );
		if (!bPassesFilter)
		{
			if (m_iSecondaryFilterMode == 2)
				return true;
			else if (m_iSecondaryFilterMode != 1)
				return false;
		}

		CBaseEntity *pActivator = info.GetAttacker();

		m_OutInflictor.Set( info.GetInflictor(), pActivator, pCaller );
		m_OutAttacker.Set( info.GetAttacker(), pActivator, pCaller );
		m_OutWeapon.Set( info.GetWeapon(), pActivator, pCaller );

		m_OutDamage.Set( info.GetDamage(), pActivator, pCaller );
		m_OutMaxDamage.Set( info.GetMaxDamage(), pActivator, pCaller );
		m_OutBaseDamage.Set( info.GetBaseDamage(), pActivator, pCaller );

		m_OutDamageType.Set( info.GetDamageType(), pActivator, pCaller );
		m_OutDamageCustom.Set( info.GetDamageCustom(), pActivator, pCaller );
		m_OutDamageStats.Set( info.GetDamageStats(), pActivator, pCaller );
		m_OutAmmoType.Set( info.GetAmmoType(), pActivator, pCaller );

		m_OutDamageForce.Set( info.GetDamageForce(), pActivator, pCaller );
		m_OutDamagePosition.Set( info.GetDamagePosition(), pActivator, pCaller );

		m_OutForceFriendlyFire.Set( info.IsForceFriendlyFire() ? 1 : 0, pActivator, pCaller );

		return bPassesFilter;
	}

	bool PassesDamageFilterImpl( CBaseEntity *pCaller, const CTakeDamageInfo &info )
	{
		if (GetTargetFilter() && m_iSecondaryFilterMode != 2)
			return RedirectToDamageFilter( pCaller, info );

		return true;
	}

	bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity )
	{
		if (GetTargetFilter() && m_iSecondaryFilterMode != 2)
			return RedirectToFilter( pCaller, pEntity );

		return true;
	}

	// 0 = Use as a regular damage filter. If it doesn't pass, damage won't be outputted.
	// 1 = Fire outputs even if the secondary filter doesn't pass.
	// 2 = Only use the secondary filter for whether to output damage, other damage is actually dealt.
	int m_iSecondaryFilterMode;

	// Outputs
	COutputEHANDLE	m_OutInflictor;
	COutputEHANDLE	m_OutAttacker;
	COutputEHANDLE	m_OutWeapon;

	COutputFloat	m_OutDamage;
	COutputFloat	m_OutMaxDamage;
	COutputFloat	m_OutBaseDamage;

	COutputInt64		m_OutDamageType;
	COutputInt		m_OutDamageCustom;
	COutputInt		m_OutDamageStats;
	COutputInt		m_OutAmmoType;

	COutputVector	m_OutDamageForce;
	COutputPositionVector	m_OutDamagePosition;

	COutputInt		m_OutForceFriendlyFire;
};

LINK_ENTITY_TO_CLASS( filter_damage_logic, CFilterDamageLogic );

BEGIN_MAPENTITY( CFilterDamageLogic, MAPENT_FILTERCLASS )

	DEFINE_KEYFIELD_AUTO( m_iSecondaryFilterMode, "SecondaryFilterMode" ),

	// Outputs
	DEFINE_OUTPUT( m_OutInflictor, "OutInflictor" ),
	DEFINE_OUTPUT( m_OutAttacker, "OutAttacker" ),
	DEFINE_OUTPUT( m_OutWeapon, "OutWeapon" ),

	DEFINE_OUTPUT( m_OutDamage, "OutDamage" ),
	DEFINE_OUTPUT( m_OutMaxDamage, "OutMaxDamage" ),
	DEFINE_OUTPUT( m_OutBaseDamage, "OutBaseDamage" ),

	DEFINE_OUTPUT( m_OutDamageType, "OutDamageType" ),
	DEFINE_OUTPUT( m_OutDamageCustom, "OutDamageCustom" ),
	DEFINE_OUTPUT( m_OutDamageStats, "OutDamageStats" ),
	DEFINE_OUTPUT( m_OutAmmoType, "OutAmmoType" ),

	DEFINE_OUTPUT( m_OutDamageForce, "OutDamageForce" ),
	DEFINE_OUTPUT( m_OutDamagePosition, "OutDamagePosition" ),

	DEFINE_OUTPUT( m_OutForceFriendlyFire, "OutForceFriendlyFire" ),

END_MAPENTITY()
