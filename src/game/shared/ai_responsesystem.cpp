//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "ai_responsesystem.h"
#include "igamesystem.h"
#include "ai_criteria.h"
#include "tier1/KeyValues.h"
#include "filesystem.h"
#include "utldict.h"
#ifdef GAME_DLL
#include "ai_speech.h"
#endif
#include "tier0/icommandline.h"
#include <ctype.h>
#include "utlbuffer.h"
#include "stringpool.h"
#include "fmtstr.h"
#include "characterset.h"
#include "responserules/response_host_interface.h"
#include "../../responserules/runtime/response_types_internal.h"
#ifdef GAME_DLL
#include "env_debughistory.h"
#endif

#include "scenefilecache/ISceneFileCache.h"

#ifdef GAME_DLL
#include "sceneentity.h"
#endif

#include "networkstringtabledefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined CLIENT_DLL || (defined GAME_DLL && defined DISABLE_DEBUG_HISTORY)
#define LocalScene_Printf Scene_Printf
#else
extern void LocalScene_Printf( const char *pFormat, ... );
#endif

using namespace ResponseRules;

extern ConVar rr_debugresponses; // ( "rr_debugresponses", "0", FCVAR_NONE, "Show verbose matching output (1 for simple, 2 for rule scoring, 3 for noisy). If set to 4, it will only show response success/failure for npc_selected NPCs." );
extern ConVar rr_debugrule; // ( "rr_debugrule", "", FCVAR_NONE, "If set to the name of the rule, that rule's score will be shown whenever a concept is passed into the response rules system.");
extern ConVar rr_dumpresponses; // ( "rr_dumpresponses", "0", FCVAR_NONE, "Dump all response_rules.txt and rules (requires restart)" );
extern ConVar rr_debugresponseconcept; // ( "rr_debugresponseconcept", "", FCVAR_NONE, "If set, rr_debugresponses will print only responses testing for the specified concept" );

extern ISceneFileCache *scenefilecache;
extern INetworkStringTable *g_pStringTableClientSideChoreoScenes;

static characterset_t	g_BreakSetIncludingColons;

// Simple class to initialize breakset
class CBreakInit
{
public:
	CBreakInit()
	{
		CharacterSetBuild( &g_BreakSetIncludingColons, "{}()':" );
	}
} g_BreakInit;

inline char rr_tolower( char c )
{
	if ( c >= 'A' && c <= 'Z' )
		return c - 'A' + 'a';
	return c;
}
// BUG BUG:  Note that this function doesn't check for data overruns!!!
// Also, this function lowercases the token as it parses!!!
inline const char *RR_Parse(const char *data, char *token )
{
	unsigned char    c;
	int             len;
	characterset_t	*breaks = &g_BreakSetIncludingColons;
	len = 0;
	token[0] = 0;

	if (!data)
		return NULL;

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
		while (1)
		{
			c = rr_tolower( *data++ );
			if (c=='\"' || !c)
			{
				token[len] = 0;
				return data;
			}
			token[len] = c;
			len++;
		}
	}

	// parse single characters
	if ( IN_CHARACTERSET( *breaks, c ) )
	{
		token[len] = c;
		len++;
		token[len] = 0;
		return data+1;
	}

	// parse a regular word
	do
	{
		token[len] = rr_tolower( c );
		data++;
		len++;
		c = rr_tolower( *data );
		if ( IN_CHARACTERSET( *breaks, c ) )
			break;
	} while (c>32);

	token[len] = 0;
	return data;
}

// A version of the above which preserves casing and supports escaped quotes
inline const char *RR_Parse_Preserve(const char *data, char *token )
{
	unsigned char    c;
	int             len;
	characterset_t	*breaks = &g_BreakSetIncludingColons;
	len = 0;
	token[0] = 0;

	if (!data)
		return NULL;

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
		bool escaped = false;
		data++;
		while (1)
		{
			c = *data++;
			if ((c=='\"' && !escaped) || !c)
			{
				token[len] = 0;
				return data;
			}
			else if (c != '\"' && escaped)
			{
				// Not an escape character, just a back slash
				token[len] = '\\';
				len++;
			}

			escaped = (c == '\\');
			if (!escaped)
			{
				token[len] = c;
				len++;
			}
		}
	}

	// parse single characters
	if ( IN_CHARACTERSET( *breaks, c ) )
	{
		token[len] = c;
		len++;
		token[len] = 0;
		return data+1;
	}

	// parse a regular word
	do
	{
		token[len] = c;
		data++;
		len++;
		c = *data;
		if ( IN_CHARACTERSET( *breaks, c ) )
			break;
	} while (c>32);

	token[len] = 0;
	return data;
}

namespace ResponseRules
{
	extern const char *ResponseCopyString( const char *in );
}

// Host functions required by the ResponseRules::IEngineEmulator interface
class CResponseRulesToEngineInterface : public ResponseRules::IEngineEmulator
{
	/// Given an input text buffer data pointer, parses a single token into the variable token and returns the new
	///  reading position
	virtual const char			*ParseFile( const char *data, char *token, int maxlen ) 
	{
		NOTE_UNUSED( maxlen );
		return RR_Parse( data, token );
	}

	/// (Optional) Same as ParseFile, but with casing preserved and escaped quotes supported
	virtual const char			*ParseFilePreserve( const char *data, char *token, int maxlen ) 
	{
		NOTE_UNUSED( maxlen );
		return RR_Parse_Preserve( data, token );
	}

	/// Return a pointer to an IFileSystem we can use to read and process scripts.
	virtual IFileSystem *GetFilesystem() 
	{
		return filesystem;
	}

	/// Return a pointer to an instance of an IUniformRandomStream
	virtual IUniformRandomStream *GetRandomStream() 
	{
		return random;
	}

	/// Return a pointer to a tier0 ICommandLine
	virtual ICommandLine *GetCommandLine() 
	{
		return CommandLine();
	}

	/// Emulates the server's UTIL_LoadFileForMe
	virtual byte *LoadFileForMe( const char *filename, int *pLength )
	{
		return UTIL_LoadFileForMe( filename, pLength );
	}

	/// Emulates the server's UTIL_FreeFile
	virtual void  FreeFile( byte *buffer ) 
	{
		return UTIL_FreeFile( buffer );
	}

};

CResponseRulesToEngineInterface g_ResponseRulesEngineWrapper;
IEngineEmulator *IEngineEmulator::s_pSingleton = &g_ResponseRulesEngineWrapper;

/// Add some game-specific code to the basic response system
/// (eg, the scene precacher, which requires the client and server
///  to work)

class CGameResponseSystem : public CResponseSystem
{
public:
	CGameResponseSystem();

	virtual void Precache();
	virtual void PrecacheResponses( bool bEnable )
	{
		m_bPrecache = bEnable;
	}
	bool		ShouldPrecache()	{ return m_bPrecache; }

protected:
	bool		m_bPrecache;	
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGameResponseSystem::CGameResponseSystem() : m_bPrecache(true)
{};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


class CScenePrecacheSystem : public CAutoGameSystem
{
public:
	CScenePrecacheSystem() : CAutoGameSystem( "CScenePrecacheSystem" ), m_RepeatCounts( 0, 0, DefLessFunc( int ) )
	{
	}

	// Level init, shutdown
	virtual void LevelShutdownPreEntity()
	{
		m_RepeatCounts.Purge();
	}

	bool ShouldPrecache( char const *pszScene )
	{
		int hash = HashStringCaselessConventional( pszScene );

		int slot = m_RepeatCounts.Find( hash );
		if ( slot != m_RepeatCounts.InvalidIndex() )
		{
			m_RepeatCounts[ slot ]++;
			return false;
		}

		m_RepeatCounts.Insert( hash, 0 );
		return true;
	}

private:

	CUtlMap< int, int > m_RepeatCounts;
};

static CScenePrecacheSystem g_ScenePrecacheSystem;

extern void PrecacheChoreoScene( CChoreoScene *scene );
extern void FreeSceneFileMemory( void *buffer );

class IChoreoEventCallback;

extern CChoreoScene *ChoreoLoadScene( 
	char const *filename,
	IChoreoEventCallback *callback, 
	ISceneTokenProcessor *tokenizer,
	void ( *pfn ) ( PRINTF_FORMAT_STRING const char *fmt, ... ) );

//-----------------------------------------------------------------------------
// Purpose: Used for precaching instanced scenes
// Input  : *pszScene - 
//-----------------------------------------------------------------------------
void PrecacheInstancedScene( char const *pszScene )
{
	static int nMakingReslists = -1;

	if ( !g_ScenePrecacheSystem.ShouldPrecache( pszScene ) )
		return;

	if ( nMakingReslists == -1 )
	{
		nMakingReslists = CommandLine()->FindParm( "-makereslists" ) > 0 ? 1 : 0;
	}

	if ( nMakingReslists == 1 )
	{
		// Just stat the file to add to reslist
		g_pFullFileSystem->Size( pszScene );
	}

	// verify existence, cache is pre-populated, should be there
	SceneCachedData_t sceneData;
	if ( scenefilecache->GetSceneCachedData( pszScene, &sceneData ) )
	{
		for ( int i = 0; i < sceneData.numSounds; ++i )
		{
			short stringId = scenefilecache->GetSceneCachedSound( sceneData.sceneId, i );
			CBaseEntity::PrecacheScriptSound( scenefilecache->GetSceneString( stringId ) );
		}
	}
	else
	{
		char loadfile[MAX_PATH];
		Q_strncpy( loadfile, pszScene, sizeof( loadfile ) );
		Q_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
		Q_FixSlashes( loadfile );

		// Attempt to precache manually
		void *pBuffer = NULL;
		if (filesystem->ReadFileEx( loadfile, "MOD", &pBuffer, true ))
		{
			g_TokenProcessor.SetBuffer((char*)pBuffer);
			CChoreoScene *pScene = ChoreoLoadScene( loadfile, NULL, &g_TokenProcessor, LocalScene_Printf );
			if (pScene)
			{
				PrecacheChoreoScene(pScene);
			}
			g_TokenProcessor.SetBuffer(NULL);
		}
		FreeSceneFileMemory( pBuffer );
	}

#ifdef GAME_DLL
	g_pStringTableClientSideChoreoScenes->AddString( true, pszScene );
#else
	g_pStringTableClientSideChoreoScenes->AddString( false, pszScene );
#endif
}

static void TouchFile( char const *pchFileName )
{
	IEngineEmulator::Get()->GetFilesystem()->Size( pchFileName );
}

void CGameResponseSystem::Precache()
{
	bool bTouchFiles = CommandLine()->FindParm( "-makereslists" ) != 0;

	// enumerate and mark all the scripts so we know they're referenced
	for ( int i = 0; i < (int)m_Responses.Count(); i++ )
	{
		ResponseGroup &group = m_Responses[i];

		for ( int j = 0; j < group.group.Count(); j++)
		{
			ParserResponse &response = group.group[j];

			switch ( response.type )
			{
			default:
				break;
			case RESPONSE_SCENE:
				{
					// fixup $gender references
					char file[_MAX_PATH];
					Q_strncpy( file, response.value, sizeof(file) );
					char *gender = strstr( file, "$gender" );
					if ( gender )
					{
						// replace with male & female
						const char *postGender = gender + strlen("$gender");
						*gender = 0;
						char genderFile[_MAX_PATH];
						// male
						Q_snprintf( genderFile, sizeof(genderFile), "%smale%s", file, postGender);

						PrecacheInstancedScene( genderFile );
						if ( bTouchFiles )
						{
							TouchFile( genderFile );
						}

						Q_snprintf( genderFile, sizeof(genderFile), "%sfemale%s", file, postGender);

						PrecacheInstancedScene( genderFile );
						if ( bTouchFiles )
						{
							TouchFile( genderFile );
						}
					}
					else
					{
						PrecacheInstancedScene( file );
						if ( bTouchFiles )
						{
							TouchFile( file );
						}
					}
				}
				break;
			case RESPONSE_SPEAK:
				{
					CBaseEntity::PrecacheScriptSound( response.value );
				}
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: A special purpose response system associated with a custom entity
//-----------------------------------------------------------------------------
class CInstancedResponseSystem : public CGameResponseSystem
{
	typedef CGameResponseSystem BaseClass;

public:
	CInstancedResponseSystem( const char *scriptfile ) :
	  m_pszScriptFile( 0 )
	  {
		  Assert( scriptfile );

		  int len = Q_strlen( scriptfile ) + 1;
		  m_pszScriptFile = new char[ len ];
		  Assert( m_pszScriptFile );
		  Q_strncpy( m_pszScriptFile, scriptfile, len );
	  }

	  ~CInstancedResponseSystem()
	  {
		  delete[] m_pszScriptFile;
	  }
	  virtual const char *GetScriptFile( void ) 
	  {
		  Assert( m_pszScriptFile );
		  return m_pszScriptFile;
	  }

	  // CAutoGameSystem
	  virtual bool Init()
	  {
		  const char *basescript = GetScriptFile();
		  LoadRuleSet( basescript );
		  return true;
	  }

	  virtual void LevelInitPostEntity()
	  {
	  #ifdef GAME_DLL
	  	if(gpGlobals->eLoadType != MapLoad_Transition)
	  #endif
		  ResetResponseGroups();
	  }

	  virtual void Release()
	  {
		  Clear();
		  delete this;
	  }
private:

	char *m_pszScriptFile;
};

//-----------------------------------------------------------------------------
// Purpose: The default response system for expressive AIs
//-----------------------------------------------------------------------------
class CDefaultResponseSystem : public CGameResponseSystem, public CAutoGameSystem
{
	typedef CAutoGameSystem BaseClass;

public:
	CDefaultResponseSystem() : CAutoGameSystem( "CDefaultResponseSystem" )
	{
	}

	virtual const char *GetScriptFile( void ) 
	{
		return "scripts/talker/response_rules.txt";
	}

	// CAutoServerSystem
	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPostEntity()
	{
		// CInstancedResponseSystem is not a CAutoGameSystem, so this needs to be called manually.
		// The same could've been accomplished by making CInstancedResponseSystem derive from CAutoGameSystem,
		// but their instanced nature would've complicated things a lot.
		int c = m_InstancedSystems.Count();
		for ( int i = c - 1 ; i >= 0; i-- )
		{
			m_InstancedSystems[i]->LevelInitPostEntity();
		}
	}

	virtual void Release()
	{
		Assert( 0 );
	}

	void AddInstancedResponseSystem( const char *scriptfile, CInstancedResponseSystem *sys )
	{
		m_InstancedSystems.Insert( scriptfile, sys );
	}

	CInstancedResponseSystem *FindResponseSystem( const char *scriptfile )
	{
		int idx = m_InstancedSystems.Find( scriptfile );
		if ( idx == m_InstancedSystems.InvalidIndex() )
			return NULL;
		return m_InstancedSystems[ idx ];
	}

	IResponseSystem *PrecacheCustomResponseSystem( const char *scriptfile )
	{
		COM_TimestampedLog( "PrecacheCustomResponseSystem %s - Start", scriptfile );
		CInstancedResponseSystem *sys = ( CInstancedResponseSystem * )FindResponseSystem( scriptfile );
		if ( !sys )
		{
			sys = new CInstancedResponseSystem( scriptfile );
			if ( !sys )
			{
				Error( "Failed to load response system data from %s", scriptfile );
			}

			if ( !sys->Init() )
			{
				Error( "CInstancedResponseSystem:  Failed to init response system from %s!", scriptfile );
			}

			AddInstancedResponseSystem( scriptfile, sys );
		}

		sys->Precache();

		COM_TimestampedLog( "PrecacheCustomResponseSystem %s - Finish", scriptfile );

		return ( IResponseSystem * )sys;
	}

	IResponseSystem *BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore );
	void DestroyCustomResponseSystems();

	virtual void LevelInitPreEntity()
	{
		// This will precache the default system
		// All user installed systems are init'd by PrecacheCustomResponseSystem which will call sys->Precache() on the ones being used

		// FIXME:  This is SLOW the first time you run the engine (can take 3 - 10 seconds!!!)
		if ( ShouldPrecache() )
		{
			Precache();
		}

	#ifdef GAME_DLL
		if(gpGlobals->eLoadType != MapLoad_Transition)
	#endif
			ResetResponseGroups();
	}

	void ReloadAllResponseSystems()
	{
		Clear();
		Init();

		int c = m_InstancedSystems.Count();
		for ( int i = c - 1 ; i >= 0; i-- )
		{
			CInstancedResponseSystem *sys = m_InstancedSystems[ i ];
			if ( !IsCustomManagable() )
			{
				sys->Clear();
				sys->Init();
			}
			else
			{
				// Custom reponse rules will manage/reload themselves - remove them.
				m_InstancedSystems.RemoveAt( i );
			}
		}

		// precache sounds in case we added new ones
		Precache();

	}

private:

	void ClearInstanced()
	{
		int c = m_InstancedSystems.Count();
		for ( int i = c - 1 ; i >= 0; i-- )
		{
			CInstancedResponseSystem *sys = m_InstancedSystems[ i ];
			sys->Release();
		}
		m_InstancedSystems.RemoveAll();
	}

	CUtlDict< CInstancedResponseSystem *, int > m_InstancedSystems;
	friend void CC_RR_DumpHashInfo( const CCommand &args );
};

IResponseSystem *CDefaultResponseSystem::BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore )
{
	// Create a instanced response system. 
	CInstancedResponseSystem *pCustomSystem = new CInstancedResponseSystem( pszCustomName );
	if ( !pCustomSystem )
	{
		Error( "BuildCustomResponseSystemGivenCriterea: Failed to create custom response system %s!", pszCustomName );
	}

	pCustomSystem->Clear();

	// Copy the relevant rules and data.
	/*
	int nRuleCount = m_Rules.Count();
	for ( int iRule = 0; iRule < nRuleCount; ++iRule )
	*/
	for ( ResponseRulePartition::tIndex iIdx = m_RulePartitions.First() ;
		m_RulePartitions.IsValid(iIdx) ;
		iIdx = m_RulePartitions.Next( iIdx ) )
	{
		Rule *pRule = &m_RulePartitions[iIdx];
		if ( pRule )
		{
			float flScore = 0.0f;

			int nCriteriaCount = pRule->m_Criteria.Count();
			for ( int iCriteria = 0; iCriteria < nCriteriaCount; ++iCriteria )
			{
				int iRuleCriteria = pRule->m_Criteria[iCriteria];

				flScore += LookForCriteria( criteriaSet, iRuleCriteria );
				if ( flScore >= flCriteriaScore )
				{
					CopyRuleFrom( pRule, iIdx, pCustomSystem );
					break;
				}
			}
		}
	}

	// Set as a custom response system.
	m_bCustomManagable = true;
	AddInstancedResponseSystem( pszCustomName, pCustomSystem );

	//	pCustomSystem->DumpDictionary( pszCustomName );

	return pCustomSystem;
}

void CDefaultResponseSystem::DestroyCustomResponseSystems()
{
	ClearInstanced();
}


static CDefaultResponseSystem defaultresponsesytem;
IResponseSystem *g_pResponseSystem = &defaultresponsesytem;

CON_COMMAND( rr_reloadresponsesystems, "Reload all response system scripts." )
{
#ifdef GAME_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	defaultresponsesytem.ReloadAllResponseSystems();
}

// Designed for extern magic, this gives the <, >, etc. of response system criteria to the outside world.
// Mostly just used for Matcher_Match in matchers.h.
bool ResponseSystemCompare( const char *criterion, const char *value )
{
	Criteria criteria;
	criteria.value = criterion;
	defaultresponsesytem.ComputeMatcher( &criteria, criteria.matcher );
	return defaultresponsesytem.CompareUsingMatcher( value, criteria.matcher, true );

	return false;
}

//-----------------------------------------------------------------------------
// CResponseFilePrecacher
//
// Purpose: Precaches a single talker file. That's it.
// 
// It copies from a bunch of the original Response System class and therefore it's really messy.
// Despite the horrors a normal programmer might find in here, I think it performs better than anything else I could've come up with.
//-----------------------------------------------------------------------------
/*
class CResponseFilePrecacher
{
public:

	// Stuff copied from the Response System.
	// Direct copy-pastes are very compact, to say the least.
	inline bool ParseToken( void )
	{
		if ( m_bUnget )
		{ m_bUnget = false; return true; }
		if ( m_ScriptStack.Count() <= 0 )
		{ return false; }

		m_ScriptStack[ 0 ].currenttoken = engine->ParseFile( m_ScriptStack[ 0 ].currenttoken, token, sizeof( token ) );
		m_ScriptStack[ 0 ].tokencount++;
		return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
	}

	CUtlVector< CResponseSystem::ScriptEntry >		m_ScriptStack;
	bool m_bUnget;
	char		token[ 1204 ];


	void PrecacheResponse( const char *response, byte type )
	{
		switch ( type )
		{
		default:
			break;
		case RESPONSE_SCENE:
			{
				DevMsg("Precaching scene %s...\n", response);

				// fixup $gender references
				char file[_MAX_PATH];
				Q_strncpy( file, response, sizeof(file) );
				char *gender = strstr( file, "$gender" );
				if ( gender )
				{
					// replace with male & female
					const char *postGender = gender + strlen("$gender");
					*gender = 0;
					char genderFile[_MAX_PATH];

					Q_snprintf( genderFile, sizeof(genderFile), "%smale%s", file, postGender);
					PrecacheInstancedScene( genderFile );

					Q_snprintf( genderFile, sizeof(genderFile), "%sfemale%s", file, postGender);
					PrecacheInstancedScene( genderFile );
				}
				else
				{
					PrecacheInstancedScene( file );
				}
			}
			break;
		case RESPONSE_SPEAK:
			{
				DevMsg("Precaching sound %s...\n", response);
				CBaseEntity::PrecacheScriptSound( response );
			}
			break;
		}
	}
	
	bool IsRootCommand()
	{
		if (!Q_stricmp( token, "#include" ) || !Q_stricmp( token, "response" )
			|| !Q_stricmp( token, "enumeration" ) || !Q_stricmp( token, "criteria" )
			|| !Q_stricmp( token, "criterion" ) || !Q_stricmp( token, "rule" ))
			return true;
		return false;
	}

	void ParseResponse( void )
	{
		// Must go to response group name
		ParseToken();

		while ( 1 )
		{
			ParseToken();

			if ( !Q_stricmp( token, "{" ) )
			{
				while ( 1 )
				{
					ParseToken();
					if ( !Q_stricmp( token, "}" ) )
						break;

					byte type = ComputeResponseType( token );
					if (type == RESPONSE_NONE)
						continue;

					ParseToken();
					char *value = CopyString( token );

					PrecacheResponse(value, type);
				}
				break;
			}

			byte type = ComputeResponseType( token );
			if (type == RESPONSE_NONE)
				break;

			ParseToken();
			char *value = CopyString( token );

			PrecacheResponse(value, type);

			break;
		}
	}

	bool LoadFromBuffer(const char *scriptfile, unsigned char *buffer, CStringPool &includedFiles)
	{
		includedFiles.Allocate( scriptfile );

		CResponseSystem::ScriptEntry e;
		e.name = filesystem->FindOrAddFileName( scriptfile );
		e.buffer = buffer;
		e.currenttoken = (char *)e.buffer;
		e.tokencount = 0;
		m_ScriptStack.AddToHead( e );

		while ( 1 )
		{
			ParseToken();
			if ( !token[0] )
			{
				break;
			}

			if ( !Q_stricmp( token, "response" ) )
			{
				ParseResponse();
			}
			else if ( !Q_stricmp( token, "#include" ) || !Q_stricmp( token, "#base" ) )
			{
				// Compacted version of ParseInclude(), including new changes.
				// Look at that if you want to read.
				char includefile[ 256 ];
				ParseToken();
				if (scriptfile) { size_t len = strlen(scriptfile)-1;
					for (size_t i = 0; i < len; i++)
					{ if (scriptfile[i] == CORRECT_PATH_SEPARATOR || scriptfile[i] == INCORRECT_PATH_SEPARATOR)
						{ len = i; }
					} Q_strncpy(includefile, scriptfile, len+1);
					if (len+1 != strlen(scriptfile))
					{ Q_snprintf(includefile, sizeof(includefile), "%s/%s", includefile, token); }
					else includefile[0] = '\0';
				} if (!includefile[0]) Q_snprintf( includefile, sizeof( includefile ), "scripts/%s", token );

				if ( includedFiles.Find( includefile ) == NULL )
				{
					MEM_ALLOC_CREDIT();

					// Try and load it
					CUtlBuffer buf;
					if ( filesystem->ReadFile( includefile, "GAME", buf ) )
					{
						LoadFromBuffer( includefile, (unsigned char *)buf.PeekGet(), includedFiles );
					}
				}
			}
		}

		if ( m_ScriptStack.Count() > 0 )
			m_ScriptStack.Remove( 0 );

		return true;
	}
};
*/

// Loads a file directly to the main response system
bool LoadResponseSystemFile(const char *scriptfile)
{
	CUtlBuffer buf;
	if ( !filesystem->ReadFile( scriptfile, "GAME", buf ) )
	{
		return false;
	}

	// This is a really messy and specialized system that precaches the responses and only the responses of a talker file.
	/*
	CStringPool includedFiles;
	CResponseFilePrecacher *rs = new CResponseFilePrecacher();
	if (!rs || !rs->LoadFromBuffer(scriptfile, (unsigned char *)buf.PeekGet(), includedFiles))
	{
		Warning( "Failed to load response system data from %s", scriptfile );
		delete rs;
		return false;
	}
	delete rs;
	*/

	// HACKHACK: This is not very efficient
	/*
	CInstancedResponseSystem *tempSys = new CInstancedResponseSystem( scriptfile );
	if ( tempSys && tempSys->Init() )
	{
		tempSys->Precache();

		for ( ResponseRulePartition::tIndex idx = tempSys->m_RulePartitions.First() ;
				tempSys->m_RulePartitions.IsValid(idx) ;
				idx = tempSys->m_RulePartitions.Next(idx) )
		{
			Rule &rule = tempSys->m_RulePartitions[idx];
			tempSys->CopyRuleFrom( &rule, idx, &defaultresponsesytem );
		}

		tempSys->Release();
	}
	*/

	// HACKHACK: This is even less efficient
	defaultresponsesytem.LoadFromBuffer( scriptfile, (const char *)buf.PeekGet() );
	defaultresponsesytem.Precache();

	return true;
}

// Called from Mapbase manifests to flush
void ReloadResponseSystem()
{
	defaultresponsesytem.ReloadAllResponseSystems();
}

//-----------------------------------------------------------------------------
// CResponseSystemSaveRestoreOps
//
// Purpose: Handles save and load for instanced response systems...
//
// BUGBUG:  This will save the same response system to file multiple times for "shared" response systems and 
//  therefore it'll restore the same data onto the same pointer N times on reload (probably benign for now, but we could
//  write code to save/restore the instanced ones by filename in the block handler above maybe?
//-----------------------------------------------------------------------------

class CResponseSystemSaveRestoreOps : public CDefCustomFieldOps
{
public:

	

} g_ResponseSystemFieldOps;

ICustomFieldOps *responseSystemFieldOps = &g_ResponseSystemFieldOps;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDefaultResponseSystem::Init()
{
	/*
	Warning( "sizeof( Response ) == %d\n", sizeof( Response ) );
	Warning( "sizeof( ResponseGroup ) == %d\n", sizeof( ResponseGroup ) );
	Warning( "sizeof( Criteria ) == %d\n", sizeof( Criteria ) );
	Warning( "sizeof( AI_ResponseParams ) == %d\n", sizeof( AI_ResponseParams ) );
	*/
	const char *basescript = GetScriptFile();

	LoadRuleSet( basescript );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDefaultResponseSystem::Shutdown()
{
	// Wipe instanced versions
	ClearInstanced();

	// Clear outselves
	Clear();
	// IServerSystem chain
	BaseClass::Shutdown();
}


//-----------------------------------------------------------------------------
// Purpose: Instance a custom response system
// Input  : *scriptfile - 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *PrecacheCustomResponseSystem( const char *scriptfile )
{
	return defaultresponsesytem.PrecacheCustomResponseSystem( scriptfile );
}

//-----------------------------------------------------------------------------
// Purpose: Instance a custom response system
// Input  : *scriptfile -
//			set - 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore )
{
	return defaultresponsesytem.BuildCustomResponseSystemGivenCriteria( pszBaseFile, pszCustomName, criteriaSet, flCriteriaScore );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DestroyCustomResponseSystems()
{
	defaultresponsesytem.DestroyCustomResponseSystems();
}
