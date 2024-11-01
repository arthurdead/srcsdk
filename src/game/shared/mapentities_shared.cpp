//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Controls the loading, parsing and creation of the entities from the BSP.
//
//=============================================================================//


#include "cbase.h"
#include "mapentities_shared.h"
#include "stringregistry.h"
#include "datacache/imdlcache.h"
#include "toolframework/iserverenginetools.h"
#include "entitylist_base.h"
#include "wcedit.h"
#include "TemplateEntities.h"
#include "point_template.h"
#ifdef GAME_DLL
#include "soundent.h"
#include "lights.h"
#include "world.h"
#endif
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_MAPPARSE, "MapParse Server" );
#else
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_MAPPARSE, "MapParse Client" );
#endif

CUtlLinkedList<CMapEntityRef, unsigned short> g_MapEntityRefs;

extern bool IsStandardEntityClassname(const char *pClassname);
extern CSharedBaseEntity *GetSingletonOfClassname(const char *pClassname);

static CStringRegistry *g_pClassnameSpawnPriority = NULL;

static const char *s_BraceChars = "{}()\'";
static bool s_BraceCharacters[256];
static bool s_BuildReverseMap = true;

bool MapEntity_ExtractValue( const char *pEntData, const char *keyName, char Value[MAPKEY_MAXLENGTH] )
{
	char token[MAPKEY_MAXLENGTH];
	const char *inputData = pEntData;

	while ( inputData )
	{
		inputData = MapEntity_ParseToken( inputData, token );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		// is this the right key?
		if ( !strcmp(token, keyName) )
		{
			inputData = MapEntity_ParseToken( inputData, token );	// get value and return it
			Q_strncpy( Value, token, MAPKEY_MAXLENGTH );
			return true;
		}

		inputData = MapEntity_ParseToken( inputData, token );	// skip over value
	}

	return false;
}

int MapEntity_GetNumKeysInEntity( const char *pEntData )
{
	char token[MAPKEY_MAXLENGTH];
	const char *inputData = pEntData;
	int iNumKeys = 0;

	while ( inputData )
	{
		inputData = MapEntity_ParseToken( inputData, token );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		iNumKeys++;

		inputData = MapEntity_ParseToken( inputData, token );	// skip over value
	}

	return iNumKeys;
}


// skips to the beginning of the next entity in the data block
// returns NULL if no more entities
const char *MapEntity_SkipToNextEntity( const char *pMapData, char *pWorkBuffer )
{
	if ( !pMapData )
		return NULL;

	// search through the map string for the next matching '{'
	int openBraceCount = 1;
	while ( pMapData != NULL )
	{
		pMapData = MapEntity_ParseToken( pMapData, pWorkBuffer );

		if ( FStrEq(pWorkBuffer, "{") )
		{
			openBraceCount++;
		}
		else if ( FStrEq(pWorkBuffer, "}") )
		{
			if ( --openBraceCount == 0 )
			{
				// we've found the closing brace, so return the next character
				return pMapData;
			}
		}
	}

	// eof hit
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: parses a token out of a char data block
//			the token gets fully read no matter what the length, but only MAPKEY_MAXLENGTH 
//			characters are written into newToken
// Input  : char *data - the data to parse
//			char *newToken - the buffer into which the new token is written
//			char *braceChars - a string of characters that constitute braces.  this pointer needs to be
//			distince for each set of braceChars, since the usage is cached.
// Output : const char * - returns a pointer to the position in the data following the newToken
//-----------------------------------------------------------------------------
const char *MapEntity_ParseToken( const char *data, char *newToken )
{
	int             c;
	int             len;
		
	len = 0;
	newToken[0] = 0;
	
	if (!data)
		return NULL;

	// build the new table if we have to
	if ( s_BuildReverseMap )
	{
		s_BuildReverseMap = false; 

		Q_memset( s_BraceCharacters, 0, sizeof(s_BraceCharacters) );

		for ( const char *c = s_BraceChars; *c; c++ )
		{
			s_BraceCharacters[(unsigned)*c] = true;
		}
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while ( len < MAPKEY_MAXLENGTH )
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				newToken[len] = 0;
				return data;
			}
			newToken[len] = c;
			len++;
		}

		if ( len >= MAPKEY_MAXLENGTH )
		{
			len--;
			newToken[len] = 0;
		}
	}

// parse single characters
	if ( s_BraceCharacters[c]/*c=='{' || c=='}'|| c==')'|| c=='(' || c=='\''*/ )
	{
		newToken[len] = c;
		len++;
		newToken[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		newToken[len] = c;
		data++;
		len++;
		c = *data;
		if ( s_BraceCharacters[c] /*c=='{' || c=='}'|| c==')'|| c=='(' || c=='\''*/ )
			break;

		if ( len >= MAPKEY_MAXLENGTH )
		{
			len--;
			newToken[len] = 0;
		}

	} while (c>32);
	
	newToken[len] = 0;
	return data;
}


/* ================= CEntityMapData definition ================ */

bool CEntityMapData::ExtractValue( const char *keyName, char *value )
{
	return MapEntity_ExtractValue( m_pEntData, keyName, value );
}

bool CEntityMapData::GetFirstKey( char *keyName, char *value )
{
	m_pCurrentKey = m_pEntData; // reset the status pointer
	return GetNextKey( keyName, value );
}

const char *CEntityMapData::CurrentBufferPosition( void )
{
	return m_pCurrentKey;
}

bool CEntityMapData::GetNextKey( char *keyName, char *value )
{
	char token[MAPKEY_MAXLENGTH];

	// parse key
	char *pPrevKey = m_pCurrentKey;
	m_pCurrentKey = (char*)MapEntity_ParseToken( m_pCurrentKey, token );
	if ( token[0] == '}' )
	{
		// step back
		m_pCurrentKey = pPrevKey;
		return false;
	}

	if ( !m_pCurrentKey )
	{
		Warning( "CEntityMapData::GetNextKey: EOF without closing brace\n" );
		Assert(0);
		return false;
	}
	
	Q_strncpy( keyName, token, MAPKEY_MAXLENGTH );

	// fix up keynames with trailing spaces
	int n = strlen(keyName);
	while (n && keyName[n-1] == ' ')
	{
		keyName[n-1] = 0;
		n--;
	}

	// parse value	
	m_pCurrentKey = (char*)MapEntity_ParseToken( m_pCurrentKey, token );
	if ( !m_pCurrentKey )
	{
		Warning( "CEntityMapData::GetNextKey: EOF without closing brace\n" );
		Assert(0);
		return false;
	}
	if ( token[0] == '}' )
	{
		Warning( "CEntityMapData::GetNextKey: closing brace without data\n" );
		Assert(0);
		return false;
	}

	// value successfully found
	Q_strncpy( value, token, MAPKEY_MAXLENGTH );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: find the keyName in the endata and change its value to specified one
//-----------------------------------------------------------------------------
bool CEntityMapData::SetValue( const char *keyName, char *NewValue, int nKeyInstance )
{
	// If this is -1, the size of the string is unknown and cannot be safely modified!
	Assert( m_nEntDataSize != -1 );
	if ( m_nEntDataSize == -1 )
		return false;

	char token[MAPKEY_MAXLENGTH];
	char *inputData = (char *)m_pEntData;
	char *prevData;

	char newvaluebuf[ 1024 ];
	int nCurrKeyInstance = 0;

	int entLen = strlen(m_pEntData);

	while ( inputData )
	{
		inputData = (char*)MapEntity_ParseToken( inputData, token );	// get keyname
		if ( token[0] == '}' )									// end of entity?
			break;												// must not have seen the classname

		// is this the right key?
		if ( !strcmp(token, keyName) )
		{
			++nCurrKeyInstance;
			if ( nCurrKeyInstance > nKeyInstance )
			{
				// Find the start & end of the token we're going to replace
				char *postData = new char[entLen];
				prevData = inputData;
				inputData = (char*)MapEntity_ParseToken( inputData, token );	// get keyname
				Q_strncpy( postData, inputData, entLen );

				// Insert quotes if caller didn't
				if ( NewValue[0] != '\"' )
				{
					Q_snprintf( newvaluebuf, sizeof( newvaluebuf ), "\"%s\"", NewValue );
				}
				else
				{
					Q_strncpy( newvaluebuf, NewValue, sizeof( newvaluebuf ) );
				}

				int iNewValueLen = Q_strlen(newvaluebuf);
				int iPadding = iNewValueLen - Q_strlen( token ) - 2;	// -2 for the quotes (token doesn't have them)

				// prevData has a space at the start, seperating the value from the key.
				// Add 1 to prevData when pasting in the new Value, to account for the space.
				Q_strncpy( prevData+1, newvaluebuf, iNewValueLen+1 );	// +1 for the null terminator
				Q_strcat( prevData, postData, m_nEntDataSize - ((prevData-m_pEntData)+1) );

				m_pCurrentKey += iPadding;
				delete [] postData;
				return true;
			}
		}

		inputData = (char*)MapEntity_ParseToken( inputData, token );	// skip over value
	}

	return false;
}

// this is a hook for edit mode
void RememberInitialEntityPositions( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	for (int nEntity = 0; nEntity < nEntities; nEntity++)
	{
		CSharedBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

		if ( pEntity )
		{
			NWCEdit::RememberEntityPosition( pEntity );
		}
	}
}

void FreeContainingEntity( edict_t *ed )
{
	if ( ed )
	{
		CSharedBaseEntity *ent = GetContainingEntity( ed );
		if ( ent )
		{
			ed->SetEdict( NULL, false );
			CSharedBaseEntity::PhysicsRemoveTouchedList( ent );
			CSharedBaseEntity::PhysicsRemoveGroundList( ent );
			UTIL_RemoveImmediate( ent );
		}
	}
}

// parent name may have a , in it to include an attachment point
string_t ExtractParentName(string_t parentName)
{
	if ( !strchr(STRING(parentName), ',') )
		return parentName;

	char szToken[256];
	nexttoken(szToken, STRING(parentName), ',', sizeof(szToken));
	return AllocPooledString(szToken);
}

//-----------------------------------------------------------------------------
// Purpose: Callback function for qsort, used to sort entities by their depth
//			in the movement hierarchy.
// Input  : pEnt1 - 
//			pEnt2 - 
// Output : Returns -1, 0, or 1 per qsort spec.
//-----------------------------------------------------------------------------
static int CDECL CompareSpawnOrder(HierarchicalSpawn_t *pEnt1, HierarchicalSpawn_t *pEnt2)
{
	if (pEnt1->m_nDepth == pEnt2->m_nDepth)
	{
		if ( g_pClassnameSpawnPriority )
		{
			int o1 = pEnt1->m_pEntity ? g_pClassnameSpawnPriority->GetStringID( pEnt1->m_pEntity->GetClassname() ) : -1;
			int o2 = pEnt2->m_pEntity ? g_pClassnameSpawnPriority->GetStringID( pEnt2->m_pEntity->GetClassname() ) : -1;
			if ( o1 < o2 )
				return 1;
			if ( o2 < o1 )
				return -1;
		}
		return 0;
	}

	if (pEnt1->m_nDepth > pEnt2->m_nDepth)
		return 1;

	return -1;
}


//-----------------------------------------------------------------------------
// Computes the hierarchical depth of the entities to spawn..
//-----------------------------------------------------------------------------
static int ComputeSpawnHierarchyDepth_r( CSharedBaseEntity *pEntity )
{
	if ( !pEntity )
		return 1;

	if (pEntity->m_iParent == NULL_STRING)
		return 1;

	CSharedBaseEntity *pParent = g_pEntityList->FindEntityByName( NULL, ExtractParentName(pEntity->m_iParent) );
	if (!pParent)
		return 1;
	
	if (pParent == pEntity)
	{
		Log_Warning( LOG_MAPPARSE,"LEVEL DESIGN ERROR: Entity %s is parented to itself!\n", pEntity->GetDebugName() );
		return 1;
	}

	return 1 + ComputeSpawnHierarchyDepth_r( pParent );
}

static void ComputeSpawnHierarchyDepth( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	// NOTE: This isn't particularly efficient, but so what? It's at the beginning of time
	// I did it this way because it simplified the parent setting in hierarchy (basically
	// eliminated questions about whether you should transform origin from global to local or not)
	int nEntity;
	for (nEntity = 0; nEntity < nEntities; nEntity++)
	{
		CSharedBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;
		if (pEntity && !pEntity->IsDormant())
		{
			pSpawnList[nEntity].m_nDepth = ComputeSpawnHierarchyDepth_r( pEntity );
		}
		else
		{
			pSpawnList[nEntity].m_nDepth = 1;
		}
	}
}

static void SortSpawnListByHierarchy( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	MEM_ALLOC_CREDIT();
	g_pClassnameSpawnPriority = new CStringRegistry;
	// this will cause the entities to be spawned in the indicated order
	// Highest string ID spawns first.  String ID is spawn priority.
	// by default, anything not in this list has priority -1
	g_pClassnameSpawnPriority->AddString( "func_wall", 10 );
	g_pClassnameSpawnPriority->AddString( "scripted_sequence", 9 );
	g_pClassnameSpawnPriority->AddString( "phys_hinge", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_ballsocket", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_slideconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_constraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_pulleyconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_lengthconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_ragdollconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "info_mass_center", 8 ); // spawn these before physbox/prop_physics
	g_pClassnameSpawnPriority->AddString( "trigger_vphysics_motion", 8 ); // spawn these before physbox/prop_physics

	g_pClassnameSpawnPriority->AddString( "prop_physics", 7 );
	g_pClassnameSpawnPriority->AddString( "prop_ragdoll", 7 );
	// Sort the entities (other than the world) by hierarchy depth, in order to spawn them in
	// that order. This insures that each entity's parent spawns before it does so that
	// it can properly set up anything that relies on hierarchy.
#ifdef _WIN32
	qsort(&pSpawnList[0], nEntities, sizeof(pSpawnList[0]), (int (CDECL *)(const void *, const void *))CompareSpawnOrder);
#elif POSIX
	qsort(&pSpawnList[0], nEntities, sizeof(pSpawnList[0]), (int (*)(const void *, const void *))CompareSpawnOrder);
#endif
	delete g_pClassnameSpawnPriority;
	g_pClassnameSpawnPriority = NULL;
}

void SetupParentsForSpawnList( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	int nEntity;
	for (nEntity = nEntities - 1; nEntity >= 0; nEntity--)
	{
		CSharedBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;
		if ( pEntity )
		{
			if ( strchr(STRING(pEntity->m_iParent), ',') )
			{
				char szToken[256];
				const char *pAttachmentName = nexttoken(szToken, STRING(pEntity->m_iParent), ',', sizeof(szToken));
				pEntity->m_iParent = AllocPooledString(szToken);
				CSharedBaseEntity *pParent = g_pEntityList->FindEntityByName( NULL, pEntity->m_iParent );

				// setparent in the spawn pass instead - so the model will have been set & loaded
				pSpawnList[nEntity].m_pDeferredParent = pParent;
				pSpawnList[nEntity].m_pDeferredParentAttachment = pAttachmentName;
			}
			else
			{
				CSharedBaseEntity *pParent = g_pEntityList->FindEntityByName( NULL, pEntity->m_iParent );

				if (pParent != NULL)
				{
					pEntity->SetParent( pParent ); 
				}
			}
		}
	}
}


void SpawnAllEntities( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities )
{
	int nEntity;
	for (nEntity = 0; nEntity < nEntities; nEntity++)
	{
		VPROF( "MapEntity_ParseAllEntities_Spawn");
		CSharedBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

		if ( pSpawnList[nEntity].m_pDeferredParent )
		{
			// UNDONE: Promote this up to the root of this function?
			MDLCACHE_CRITICAL_SECTION();
			CSharedBaseEntity *pParent = pSpawnList[nEntity].m_pDeferredParent;
			if(pParent)
			{
				int iAttachment = -1;
				CSharedBaseAnimating *pAnim = pParent->GetBaseAnimating();
				if ( pAnim )
				{
					iAttachment = pAnim->LookupAttachment(pSpawnList[nEntity].m_pDeferredParentAttachment);
				}

				if(pEntity)
				{
					pEntity->SetParent( pParent, iAttachment );
				}
			}
		}
		if ( pEntity )
		{
			if (DispatchSpawn(pEntity) < 0)
			{
				for ( int i = nEntity+1; i < nEntities; i++ )
				{
					// this is a child object that will be deleted now
					if ( pSpawnList[i].m_pEntity && pSpawnList[i].m_pEntity->IsMarkedForDeletion() )
					{
						pSpawnList[i].m_pEntity = NULL;
					}
				}

				// Spawn failed.
				g_pEntityList->CleanupDeleteList();
				// Remove the entity from the spawn list
				pSpawnList[nEntity].m_pEntity = NULL;
			}
		}
	}

	if ( bActivateEntities )
	{
		VPROF( "MapEntity_ParseAllEntities_Activate");
		bool bAsyncAnims = g_pMDLCache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
		for (nEntity = 0; nEntity < nEntities; nEntity++)
		{
			CSharedBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

			if ( pEntity )
			{
				MDLCACHE_CRITICAL_SECTION();
				pEntity->Activate();
			}
		}
		g_pMDLCache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
	}
}

// --------------------------------------------------------------------------------------------------- //
// CMapEntitySpawner implementation.
// --------------------------------------------------------------------------------------------------- //

CMapEntitySpawner::CMapEntitySpawner()
{
	m_nEntities = 0;
	m_pSpawnMapData = new HierarchicalSpawnMapData_t[GAME_NUM_ENT_ENTRIES];
	m_pSpawnList = new HierarchicalSpawn_t[GAME_NUM_ENT_ENTRIES];
	m_bFoundryMode = false;
}

CMapEntitySpawner::~CMapEntitySpawner()
{
	delete [] m_pSpawnMapData;
	delete [] m_pSpawnList;
}

void CMapEntitySpawner::AddEntity( CSharedBaseEntity *pEntity, const char *pCurMapData, int iMapDataLength )
{
	if (pEntity->IsTemplate())
	{
		if ( m_bFoundryMode )
			Templates_RemoveByHammerID( pEntity->GetHammerID() );

		// It's a template entity. Squirrel away its keyvalue text so that we can
		// recreate the entity later via a spawner. pMapData points at the '}'
		// so we must add one to include it in the string.
		Templates_Add( pEntity, pCurMapData, iMapDataLength, pEntity->GetHammerID() );

		// Remove the template entity so that it does not show up in FindEntityXXX searches.
		UTIL_Remove(pEntity);
		PurgeRemovedEntities();
		return;
	}

	// To 
	if ( pEntity->IsWorld() )
	{
		Assert( !m_bFoundryMode );
		VPROF( "MapEntity_ParseAllEntities_SpawnWorld");

		pEntity->m_iParent = NULL_STRING;	// don't allow a parent on the first entity (worldspawn)

		DispatchSpawn(pEntity);
		return;
	}

#ifdef GAME_DLL
	if ( dynamic_cast<CLight*>(pEntity) )
	{
		VPROF( "MapEntity_ParseAllEntities_SpawnTransients");

		// We overflow the max edicts on large maps that have lots of entities.
		// Nodes & Lights remove themselves immediately on Spawn(), so dispatch their
		// spawn now, to free up the slot inside this loop.
		// NOTE: This solution prevents nodes & lights from being used inside point_templates.
		if (DispatchSpawn(pEntity) < 0)
		{
			PurgeRemovedEntities();
		}
		return;
	}
#endif

	// Build a list of all point_template's so we can spawn them before everything else
	CSharedPointTemplate *pTemplate = dynamic_cast< CSharedPointTemplate* >(pEntity);
	if ( pTemplate )
	{
		m_PointTemplates.AddToTail( pTemplate );
	}
	else
	{
		// Queue up this entity for spawning
		m_pSpawnList[m_nEntities].m_pEntity = pEntity;
		m_pSpawnList[m_nEntities].m_nDepth = 0;
		m_pSpawnList[m_nEntities].m_pDeferredParentAttachment = NULL;
		m_pSpawnList[m_nEntities].m_pDeferredParent = NULL;

		m_pSpawnMapData[m_nEntities].m_pMapData = pCurMapData;
		m_pSpawnMapData[m_nEntities].m_iMapDataLength = iMapDataLength;
		m_nEntities++;
	}
}

void MapEntity_ParseAllEntites_SpawnTemplates( CSharedPointTemplate **pTemplates, int iTemplateCount, CSharedBaseEntity **pSpawnedEntities, HierarchicalSpawnMapData_t *pSpawnMapData, int iSpawnedEntityCount )
{
	// Now loop through all our point_template entities and tell them to make templates of everything they're pointing to
	for ( int i = 0; i < iTemplateCount; i++ )
	{
		VPROF( "MapEntity_ParseAllEntities_SpawnTemplates");
		CSharedPointTemplate *pPointTemplate = pTemplates[i];

		// First, tell the Point template to Spawn
		if ( DispatchSpawn(pPointTemplate) < 0 )
		{
			UTIL_Remove(pPointTemplate);
			g_pEntityList->CleanupDeleteList();
			continue;
		}

		pPointTemplate->StartBuildingTemplates();

		// Now go through all it's templates and turn the entities into templates
		int iNumTemplates = pPointTemplate->GetNumTemplateEntities();
		for ( int iTemplateNum = 0; iTemplateNum < iNumTemplates; iTemplateNum++ )
		{
			// Find it in the spawn list
			CSharedBaseEntity *pEntity = pPointTemplate->GetTemplateEntity( iTemplateNum );
			for ( int iEntNum = 0; iEntNum < iSpawnedEntityCount; iEntNum++ )
			{
				if ( pSpawnedEntities[iEntNum] == pEntity )
				{
					// Give the point_template the mapdata
					pPointTemplate->AddTemplate( pEntity, pSpawnMapData[iEntNum].m_pMapData, pSpawnMapData[iEntNum].m_iMapDataLength );

					if ( pPointTemplate->ShouldRemoveTemplateEntities() )
					{
						// Remove the template entity so that it does not show up in FindEntityXXX searches.
						UTIL_Remove(pEntity);
						g_pEntityList->CleanupDeleteList();

						// Remove the entity from the spawn list
						pSpawnedEntities[iEntNum] = NULL;
					}
					break;
				}
			}
		}

		pPointTemplate->FinishBuildingTemplates();
	}
}

void CMapEntitySpawner::HandleTemplates()
{
	if( m_PointTemplates.Count() == 0 )
		return;

	CSharedBaseEntity **pSpawnedEntities = (CSharedBaseEntity **)stackalloc( sizeof(CSharedBaseEntity *) * m_nEntities );
	for( int i = 0; i != m_nEntities; ++i )
	{
		pSpawnedEntities[i] = m_pSpawnList[i].m_pEntity;
	}

	PurgeRemovedEntities();
	MapEntity_ParseAllEntites_SpawnTemplates( m_PointTemplates.Base(), m_PointTemplates.Count(), pSpawnedEntities, m_pSpawnMapData, m_nEntities );

	//copy the entity list back since some entities may have been removed and nulled out
	for( int i = 0; i != m_nEntities; ++i )
	{
		m_pSpawnList[i].m_pEntity = pSpawnedEntities[i]; 
	}
}


void CMapEntitySpawner::SpawnAndActivate( bool bActivateEntities )
{
	SpawnHierarchicalList( m_nEntities, m_pSpawnList, bActivateEntities );
}

void CMapEntitySpawner::PurgeRemovedEntities()
{
	// Walk through spawn list and NULL out any soon-to-be-stale pointers
	for ( int i = 0; i < m_nEntities; ++ i )
	{
		if ( m_pSpawnList[i].m_pEntity && m_pSpawnList[i].m_pEntity->IsMarkedForDeletion() )
		{
#ifdef _DEBUG
			// Catch a specific error that bit us
			if ( dynamic_cast< CSharedGameRulesProxy * >( m_pSpawnList[i].m_pEntity ) != NULL )
			{
				Log_Warning( LOG_MAPPARSE,"Map-placed game rules entity is being deleted; does the map contain more than one?\n" );
			}
#endif
			m_pSpawnList[i].m_pEntity = NULL;
		}
	}

	g_pEntityList->CleanupDeleteList();
}


//-----------------------------------------------------------------------------
// Purpose: Only called on BSP load. Parses and spawns all the entities in the BSP.
// Input  : pMapData - Pointer to the entity data block to parse.
//-----------------------------------------------------------------------------
void MapEntity_ParseAllEntities(const char *pMapData, IMapEntityFilter *pFilter, bool bActivateEntities)
{
	VPROF("MapEntity_ParseAllEntities");

	CMapEntitySpawner spawner;

	char szTokenBuffer[MAPKEY_MAXLENGTH];

	// Allow the tools to spawn different things
	if ( serverenginetools )
	{
		pMapData = serverenginetools->GetEntityData( pMapData );
	}

	//  Loop through all entities in the map data, creating each.
	for ( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) )
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken( pMapData, token );

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
			break;

		if (token[0] != '{')
		{
			Log_FatalError( LOG_MAPPARSE,"MapEntity_ParseAllEntities: found %s when expecting {", token);
			continue;
		}

		//
		// Parse the entity and add it to the spawn list.
		//
		CSharedBaseEntity *pEntity;
		const char *pCurMapData = pMapData;
		pMapData = MapEntity_ParseEntity(pEntity, pMapData, pFilter);
		if (pEntity == NULL)
			continue;

		spawner.AddEntity( pEntity, pCurMapData, pMapData - pCurMapData + 2 );
	}

	spawner.HandleTemplates();
	spawner.SpawnAndActivate( bActivateEntities );
}

void SpawnHierarchicalList( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities )
{
	// Compute the hierarchical depth of all entities hierarchically attached
	ComputeSpawnHierarchyDepth( nEntities, pSpawnList );

	// Sort the entities (other than the world) by hierarchy depth, in order to spawn them in
	// that order. This insures that each entity's parent spawns before it does so that
	// it can properly set up anything that relies on hierarchy.
	SortSpawnListByHierarchy( nEntities, pSpawnList );

	// save off entity positions if in edit mode
	if ( engine->IsInEditMode() )
	{
		RememberInitialEntityPositions( nEntities, pSpawnList );
	}

	// Set up entity movement hierarchy in reverse hierarchy depth order. This allows each entity
	// to use its parent's world spawn origin to calculate its local origin.
	SetupParentsForSpawnList( nEntities, pSpawnList );

	// Spawn all the entities in hierarchy depth order so that parents spawn before their children.
	SpawnAllEntities( nEntities, pSpawnList, bActivateEntities );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntData - 
//-----------------------------------------------------------------------------
void MapEntity_PrecacheEntity( const char *pEntData, int &nStringSize )
{
	CEntityMapData entData( (char*)pEntData, nStringSize );
	char className[MAPKEY_MAXLENGTH];
	
	if (!entData.ExtractValue("classname", className))
	{
		Log_FatalError( LOG_MAPPARSE,"classname missing from entity!\n" );
	}

	// Construct via the LINK_ENTITY_TO_CLASS factory.
	CSharedBaseEntity *pEntity = CreateEntityByName(className);

	//
	// Set up keyvalues, which can set the model name, which is why we don't just do UTIL_PrecacheOther here...
	//
	if ( pEntity != NULL )
	{
		pEntity->ParseMapData(&entData);
		pEntity->Precache();
		UTIL_RemoveImmediate( pEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Takes a block of character data as the input
// Input  : pEntity - Receives the newly constructed entity, NULL on failure.
//			pEntData - Data block to parse to extract entity keys.
// Output : Returns the current position in the entity data block.
//-----------------------------------------------------------------------------
const char *MapEntity_ParseEntity(CSharedBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter)
{
	CEntityMapData entData( (char*)pEntData );
	char className[MAPKEY_MAXLENGTH];
	
	if (!entData.ExtractValue("classname", className))
	{
		Log_FatalError( LOG_MAPPARSE,"classname missing from entity!\n" );
	}

	pEntity = GetSingletonOfClassname( className );
	if( !pEntity )
	{
		//
		// Construct via the LINK_ENTITY_TO_CLASS factory.
		//
		if ( !pFilter )
		{
			pEntity = CreateEntityByName(className);
		}
		else if ( pFilter->ShouldCreateEntity( className ) )
		{
			pEntity = pFilter->CreateNextEntity( className );
		}
	}

	//
	// Set up keyvalues.
	//
	if (pEntity != NULL)
	{
		pEntity->ParseMapData(&entData);
	}
	else
	{
		// Just skip past all the keys.
		char keyName[MAPKEY_MAXLENGTH];
		char value[MAPKEY_MAXLENGTH];
		if ( entData.GetFirstKey(keyName, value) )
		{
			do 
			{
			} 
			while ( entData.GetNextKey(keyName, value) );
		}
	}

	//
	// Return the current parser position in the data block
	//
	return entData.CurrentBufferPosition();
}

bool CMapLoadEntityFilter::ShouldCreateEntity( const char *pClassname )
{
	if(IsStandardEntityClassname(pClassname))
		return false;

	return true;
}

CSharedBaseEntity* CMapLoadEntityFilter::CreateNextEntity( const char *pClassname )
{
	CSharedBaseEntity *pRet = CreateEntityByName( pClassname );

	CMapEntityRef ref;

	if ( pRet )
	{
		ref.m_iEdict = pRet->entindex();
		ref.m_iSerialNumber = pRet->entserial();
	}
	else
	{
		ref.m_iEdict = -1;
		ref.m_iSerialNumber = -1;
	}

	g_MapEntityRefs.AddToTail( ref );
	return pRet;
}
