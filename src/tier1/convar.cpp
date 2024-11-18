//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basetypes.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"
#include "tier1/characterset.h"
#include "tier1/utlbuffer.h"
#include "tier1/tier1.h"
#include "tier1/convar_serverbounded.h"
#include "icvar.h"
#include "tier0/dbg.h"
#include "Color.h"
#include "tier0/memdbgon.h"
#include "common/module_name.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_CONVAR, "ConVar" );

//-----------------------------------------------------------------------------
// Statically constructed list of ConCommandBases, 
// used for registering them with the ICVar interface
//-----------------------------------------------------------------------------

// ConVars add themselves to this list for the executable. 
// Then ConVar_Register runs through  all the console variables 
// and registers them into a global list stored in vstdlib.dll
static ConCommandBase		*s_pConCommandBases = NULL;

// ConVars in this executable use this 'global' to access values.
static IConCommandBaseAccessor	*s_pAccessor = NULL;

ConVar *developer=NULL;

static int s_nCVarFlag = 0;
static int s_nDLLIdentifier = -1;	// A unique identifier indicating which DLL this convar came from
static bool s_bRegistered = false;

bool CDefaultAccessor::RegisterConCommandBase( ConCommandBase *pVar )
{
#ifdef _DEBUG
	ConCommandBase *pOldVar = g_pCVar->FindCommandBase(pVar->GetName());
	AssertMsg( 
		!pOldVar ||
		(pOldVar->IsFlagSet(FCVAR_REPLICATED) && pVar->IsFlagSet(FCVAR_REPLICATED)) ||
		(!pOldVar->IsFlagSet(FCVAR_REPLICATED) && !pVar->IsFlagSet(FCVAR_REPLICATED)), 
		"%s dll tried to re-register con var/command named %s", modulename::dll, pVar->GetName()
	);
#endif

	// Link to engine's list instead
	g_pCVar->RegisterConCommand( pVar );

	char const *pValue = g_pCVar->GetCommandLineValue( pVar->GetName() );
	if( pValue && !pVar->IsCommand() )
	{
		( ( ConVar * )pVar )->SetValue( pValue );
	}

	return true;
}

CDefaultAccessor s_DefaultAccessor;

static void ModifyFlags(int &flags)
{
#ifndef _DEBUG
	if ( ( flags & ( FCVAR_ARCHIVE | FCVAR_RELEASE | FCVAR_USERINFO ) ) == 0 )
	{
		flags |= (FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY);
	}
#else
	flags &= ~(FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY);
#endif
}

//-----------------------------------------------------------------------------
// Called by the framework to register ConCommandBases with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( int nCVarFlag, IConCommandBaseAccessor *pAccessor )
{
	if( !pAccessor )
		pAccessor = &s_DefaultAccessor;

	if ( !g_pCVar || s_bRegistered )
		return;

	developer = g_pCVar->FindVar("developer");

	s_pAccessor = pAccessor;
	s_nCVarFlag = nCVarFlag;
	Assert( s_nDLLIdentifier < 0 );
	s_nDLLIdentifier = g_pCVar->AllocateDLLIdentifier();
#ifdef _DEBUG
	Log_Msg(LOG_CONVAR,"%s got dll identifier %i\n", modulename::dll, s_nDLLIdentifier);
#endif

	ConCommandBase *pCur, *pNext;

	pCur = s_pConCommandBases;
	while ( pCur )
	{
		pNext = pCur->m_pNext;
		pCur->AddFlags( nCVarFlag );
		pCur->Init( pAccessor );
		pCur = pNext;
	}

	g_pCVar->ProcessQueuedMaterialThreadConVarSets();

	s_bRegistered = true;
}

void ConVar_Unregister( )
{
	if ( !g_pCVar || !s_bRegistered )
		return;

	Assert( s_nDLLIdentifier >= 0 );
	g_pCVar->UnregisterConCommands( s_nDLLIdentifier );
	s_nCVarFlag = 0;
	s_nDLLIdentifier = -1;
	s_pAccessor = NULL;
	s_bRegistered = false;
}


//-----------------------------------------------------------------------------
// Purpose: Default constructor
//-----------------------------------------------------------------------------
ConCommandBase::ConCommandBase( void )
{
	m_bRegistered   = false;
	m_pszName       = NULL;
	m_pszHelpString = NULL;

	m_nFlags = 0;
	m_pNext  = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConCommandBase::~ConCommandBase( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsCommand( void ) const
{ 
//	Assert( 0 ); This can't assert. . causes a recursive assert in Sys_Printf, etc.
	return true;
}


//-----------------------------------------------------------------------------
// Returns the DLL identifier
//-----------------------------------------------------------------------------
CVarDLLIdentifier_t ConCommandBase::GetDLLIdentifier() const
{
	return s_nDLLIdentifier;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//			callback - 
//			*pHelpString - 
//			flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::CreateBase( const char *pName, const char *pHelpString /*= 0*/, int flags /*= 0*/ )
{
	m_bRegistered = false;

	// Name should be static data
	Assert( pName );
	m_pszName = pName;
	m_pszHelpString = pHelpString ? pHelpString : "";

	m_nFlags = flags;
	ModifyFlags( m_nFlags );

	m_pNext = s_pConCommandBases;
	s_pConCommandBases = this;

	// If s_pAccessor is already set (this ConVar is not a global variable),
	//  register it.
	if ( s_bRegistered )
	{
		AddFlags( s_nCVarFlag );
		Init( s_pAccessor );
	}
}

void ConCommandLower::CreateBase( const char *pName, const char *pHelpString, int flags )
{
	V_strncpy(m_szNameLower, pName, sizeof(m_szNameLower));
	V_strlower(m_szNameLower);

	ConCommand::CreateBase(m_szNameLower, pHelpString, flags);
}

//-----------------------------------------------------------------------------
// Purpose: Used internally by OneTimeInit to initialize.
//-----------------------------------------------------------------------------
void ConCommandBase::Init()
{
	IConCommandBaseAccessor *pAccessor = s_pAccessor;
	if( !pAccessor )
		pAccessor = &s_DefaultAccessor;

	Init( pAccessor );
}

void ConCommandLinked::CreateBase( const char *pName, const char *pHelpString, int flags )
{
	m_pParent = NULL;

	ConCommand::CreateBase(pName, pHelpString, FCVAR_REPLICATED|flags);
}

void ConCommandLinked::Dispatch( const CCommand &command )
{
	ConCommand::Dispatch( command );

	if( m_pParent )
		m_pParent->ConCommand::Dispatch( command );
}

void ConCommandBase::Init( IConCommandBaseAccessor *pAccessor )
{
	Assert( pAccessor );

	bool should_reg = !IsFlagSet(FCVAR_UNREGISTERED);
	bool is_cmd = IsCommand();
	bool is_repl = IsFlagSet(FCVAR_REPLICATED);

	if( is_cmd && is_repl )
	{
		Assert( IsFlagSet(FCVAR_GAMEDLL) || IsFlagSet(FCVAR_CLIENTDLL) );

		ConCommand *pOther = g_pCVar->FindCommand( GetName() );
		if( pOther )
		{
			pOther->m_nFlags |= (m_nFlags & FCVAR_GAMEDLL);
			pOther->m_nFlags |= (m_nFlags & FCVAR_CLIENTDLL);

			ConCommandLinked *pLink = dynamic_cast<ConCommandLinked *>(pOther);
			if( pLink )
				pLink->m_pParent = (ConCommand *)this;

			should_reg = false;
			m_nFlags |= FCVAR_UNREGISTERED;
		}
		else
		{
			if( should_reg )
				m_nFlags &= ~FCVAR_REPLICATED;
		}
	}

	if ( should_reg )
	{
		pAccessor->RegisterConCommandBase( this );

		if( is_cmd && is_repl )
		{
			m_nFlags |= FCVAR_REPLICATED;
		}
	}
}

void ConCommandBase::Shutdown()
{
	if ( g_pCVar )
	{
		g_pCVar->UnregisterConCommand( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Return name of the command/var
// Output : const char
//-----------------------------------------------------------------------------
const char *ConCommandBase::GetName( void ) const
{
	return m_pszName;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flag - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsFlagSet( int flag ) const
{
	return ( flag & GetFlags() ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags( int flags )
{
	m_nFlags |= flags;
	ModifyFlags( m_nFlags );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const ConCommandBase
//-----------------------------------------------------------------------------
const ConCommandBase *ConCommandBase::GetNext( void ) const
{
	return m_pNext;
}

ConCommandBase *ConCommandBase::GetNext( void )
{
	return m_pNext;
}


//-----------------------------------------------------------------------------
// Purpose: Copies string using local new/delete operators
// Input  : *from - 
// Output : char
//-----------------------------------------------------------------------------
char *ConCommandBase::CopyString( const char *from )
{
	int		len;
	char	*to;

	len = V_strlen( from );
	if ( len <= 0 )
	{
		to = new char[1];
		to[0] = 0;
	}
	else
	{
		to = new char[len+1];
		Q_strncpy( to, from, len+1 );
	}
	return to;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConCommandBase::GetHelpText( void ) const
{
	return m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Has this cvar been registered
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConCommandBase::IsRegistered( void ) const
{
	return m_bRegistered;
}


//-----------------------------------------------------------------------------
//
// Con Commands start here
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global methods
//-----------------------------------------------------------------------------
static characterset_t s_BreakSet;
static bool s_bBuiltBreakSet = false;


//-----------------------------------------------------------------------------
// Tokenizer class
//-----------------------------------------------------------------------------
CCommand::CCommand()
{
	if ( !s_bBuiltBreakSet )
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild( &s_BreakSet, "{}()':" );
	}

	Reset();
}

CCommand::CCommand( int nArgC, const char **ppArgV )
{
	Assert( nArgC > 0 );

	if ( !s_bBuiltBreakSet )
	{
		s_bBuiltBreakSet = true;
		CharacterSetBuild( &s_BreakSet, "{}()':" );
	}

	Reset();

	char *pBuf = m_pArgvBuffer;
	char *pSBuf = m_pArgSBuffer;
	m_nArgc = nArgC;
	for ( int i = 0; i < nArgC; ++i )
	{
		m_ppArgv[i] = pBuf;
		int nLen = Q_strlen( ppArgV[i] );
		memcpy( pBuf, ppArgV[i], nLen+1 );
		if ( i == 0 )
		{
			m_nArgv0Size = nLen;
		}
		pBuf += nLen+1;

		bool bContainsSpace = strchr( ppArgV[i], ' ' ) != NULL;
		if ( bContainsSpace )
		{
			*pSBuf++ = '\"';
		}
		memcpy( pSBuf, ppArgV[i], nLen );
		pSBuf += nLen;
		if ( bContainsSpace )
		{
			*pSBuf++ = '\"';
		}

		if ( i != nArgC - 1 )
		{
			*pSBuf++ = ' ';
		}
	}
}

void CCommand::Reset()
{
	m_nArgc = 0;
	m_nArgv0Size = 0;
	m_pArgSBuffer[0] = 0;
}

characterset_t* CCommand::DefaultBreakSet()
{
	return &s_BreakSet;
}

bool CCommand::Tokenize( const char *pCommand, characterset_t *pBreakSet )
{
	Reset();
	if ( !pCommand )
		return false;

	// Use default break set
	if ( !pBreakSet )
	{
		pBreakSet = &s_BreakSet;
	}

	// Copy the current command into a temp buffer
	// NOTE: This is here to avoid the pointers returned by DequeueNextCommand
	// to become invalid by calling AddText. Is there a way we can avoid the memcpy?
	int nLen = Q_strlen( pCommand );
	if ( nLen >= COMMAND_MAX_LENGTH - 1 )
	{
		Warning( "CCommand::Tokenize: Encountered command which overflows the tokenizer buffer.. Skipping!\n" );
		return false;
	}

	memcpy( m_pArgSBuffer, pCommand, nLen + 1 );

	// Parse the current command into the current command buffer
	CUtlBuffer bufParse( m_pArgSBuffer, nLen, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY ); 
	int nArgvBufferSize = 0;
	while ( bufParse.IsValid() && ( m_nArgc < COMMAND_MAX_ARGC ) )
	{
		char *pArgvBuf = &m_pArgvBuffer[nArgvBufferSize];
		int nMaxLen = COMMAND_MAX_LENGTH - nArgvBufferSize;
		int nStartGet = bufParse.TellGet();
		int	nSize = bufParse.ParseToken( pBreakSet, pArgvBuf, nMaxLen );
		if ( nSize < 0 )
			break;

		// Check for overflow condition
		if ( nMaxLen == nSize )
		{
			Reset();
			return false;
		}

		if ( m_nArgc == 1 )
		{
			// Deal with the case where the arguments were quoted
			m_nArgv0Size = bufParse.TellGet();
			bool bFoundEndQuote = m_pArgSBuffer[m_nArgv0Size-1] == '\"';
			if ( bFoundEndQuote )
			{
				--m_nArgv0Size;
			}
			m_nArgv0Size -= nSize;
			Assert( m_nArgv0Size != 0 );

			// The StartGet check is to handle this case: "foo"bar
			// which will parse into 2 different args. ArgS should point to bar.
			bool bFoundStartQuote = ( m_nArgv0Size > nStartGet ) && ( m_pArgSBuffer[m_nArgv0Size-1] == '\"' );
			Assert( bFoundEndQuote == bFoundStartQuote );
			if ( bFoundStartQuote )
			{
				--m_nArgv0Size;
			}
		}

		m_ppArgv[ m_nArgc++ ] = pArgvBuf;
		if( m_nArgc >= COMMAND_MAX_ARGC )
		{
			Warning( "CCommand::Tokenize: Encountered command which overflows the argument buffer.. Clamped!\n" );
		}

		nArgvBufferSize += nSize + 1;
		Assert( nArgvBufferSize <= COMMAND_MAX_LENGTH );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Helper function to parse arguments to commands.
//-----------------------------------------------------------------------------
const char* CCommand::FindArg( const char *pName ) const
{
	int nArgC = ArgC();
	for ( int i = 1; i < nArgC; i++ )
	{
		if ( !Q_stricmp( Arg(i), pName ) )
			return (i+1) < nArgC ? Arg( i+1 ) : "";
	}
	return 0;
}

int CCommand::FindArgInt( const char *pName, int nDefaultVal ) const
{
	const char *pVal = FindArg( pName );
	if ( pVal )
		return atoi( pVal );
	else
		return nDefaultVal;
}


//-----------------------------------------------------------------------------
// Default console command autocompletion function 
//-----------------------------------------------------------------------------
int DefaultCompletionFunc( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Constructs a console command
//-----------------------------------------------------------------------------
//ConCommand::ConCommand()
//{
//	m_bIsNewConCommand = true;
//}

ConCommand::ConCommand( const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallbackV1 = callback;
	m_bUsingNewCommandCallback = false;
	m_bUsingCommandCallbackInterface = false;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;

	// Setup the rest
	BaseClass::CreateBase( pName, pHelpString, flags );
}

ConCommand::ConCommand( const char *pName, FnCommandCallback_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallback = callback;
	m_bUsingNewCommandCallback = true;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCallbackInterface = false;

	// Setup the rest
	BaseClass::CreateBase( pName, pHelpString, flags );
}

ConCommand::ConCommand( const char *pName, ICommandCallback *pCallback, const char *pHelpString /*= 0*/, int flags /*= 0*/, ICommandCompletionCallback *pCompletionCallback /*= 0*/ )
{
	// Set the callback
	m_pCommandCallback = pCallback;
	m_bUsingNewCommandCallback = false;
	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = ( pCompletionCallback != 0 );
	m_bUsingCommandCallbackInterface = true;

	// Setup the rest
	BaseClass::CreateBase( pName, pHelpString, flags );
}

ConCommandLinked::ConCommandLinked( const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallbackV1 = callback;
	m_bUsingNewCommandCallback = false;
	m_bUsingCommandCallbackInterface = false;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;

	// Setup the rest
	CreateBase( pName, pHelpString, flags );
}

ConCommandLinked::ConCommandLinked( const char *pName, FnCommandCallback_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallback = callback;
	m_bUsingNewCommandCallback = true;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCallbackInterface = false;

	// Setup the rest
	CreateBase( pName, pHelpString, flags );
}

ConCommandLinked::ConCommandLinked( const char *pName, ICommandCallback *pCallback, const char *pHelpString /*= 0*/, int flags /*= 0*/, ICommandCompletionCallback *pCompletionCallback /*= 0*/ )
{
	// Set the callback
	m_pCommandCallback = pCallback;
	m_bUsingNewCommandCallback = false;
	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = ( pCompletionCallback != 0 );
	m_bUsingCommandCallbackInterface = true;

	// Setup the rest
	CreateBase( pName, pHelpString, flags );
}

ConCommandLower::ConCommandLower( const char *pName, FnCommandCallbackVoid_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallbackV1 = callback;
	m_bUsingNewCommandCallback = false;
	m_bUsingCommandCallbackInterface = false;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;

	// Setup the rest
	CreateBase( pName, pHelpString, flags );
}

ConCommandLower::ConCommandLower( const char *pName, FnCommandCallback_t callback, const char *pHelpString /*= 0*/, int flags /*= 0*/, FnCommandCompletionCallback completionFunc /*= 0*/ )
{
	// Set the callback
	m_fnCommandCallback = callback;
	m_bUsingNewCommandCallback = true;
	m_fnCompletionCallback = completionFunc ? completionFunc : DefaultCompletionFunc;
	m_bHasCompletionCallback = completionFunc != 0 ? true : false;
	m_bUsingCommandCallbackInterface = false;

	// Setup the rest
	CreateBase( pName, pHelpString, flags );
}

ConCommandLower::ConCommandLower( const char *pName, ICommandCallback *pCallback, const char *pHelpString /*= 0*/, int flags /*= 0*/, ICommandCompletionCallback *pCompletionCallback /*= 0*/ )
{
	// Set the callback
	m_pCommandCallback = pCallback;
	m_bUsingNewCommandCallback = false;
	m_pCommandCompletionCallback = pCompletionCallback;
	m_bHasCompletionCallback = ( pCompletionCallback != 0 );
	m_bUsingCommandCallbackInterface = true;

	// Setup the rest
	CreateBase( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConCommand::~ConCommand( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
//-----------------------------------------------------------------------------
bool ConCommand::IsCommand( void ) const
{ 
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Invoke the function if there is one
//-----------------------------------------------------------------------------
void ConCommand::Dispatch( const CCommand &command )
{
	if ( m_bUsingNewCommandCallback )
	{
		if ( m_fnCommandCallback )
		{
			( *m_fnCommandCallback )( command );
			return;
		}
	}
	else if ( m_bUsingCommandCallbackInterface )
	{
		if ( m_pCommandCallback )
		{
			m_pCommandCallback->CommandCallback( command );
			return;
		}
	}
	else
	{
		if ( m_fnCommandCallbackV1 )
		{
			( *m_fnCommandCallbackV1 )();
			return;
		}
	}

	// Command without callback!!!
	AssertMsg( 0, "Encountered ConCommand '%s' without a callback!\n", GetName() );
}


//-----------------------------------------------------------------------------
// Purpose: Calls the autocompletion method to get autocompletion suggestions
//-----------------------------------------------------------------------------
int	ConCommand::AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands )
{
	if ( m_bUsingCommandCallbackInterface )
	{
		if ( !m_pCommandCompletionCallback )
			return 0;
		return m_pCommandCompletionCallback->CommandCompletionCallback( partial, commands );
	}

	Assert( m_fnCompletionCallback );
	if ( !m_fnCompletionCallback )
		return 0;

	char rgpchCommands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ];
	int iret = ( m_fnCompletionCallback )( partial, rgpchCommands );
	for ( int i = 0 ; i < iret; ++i )
	{
		CUtlString str = rgpchCommands[ i ];
		commands.AddToTail( str );
	}
	return iret;
}


//-----------------------------------------------------------------------------
// Returns true if the console command can autocomplete 
//-----------------------------------------------------------------------------
bool ConCommand::CanAutoComplete( void )
{
	return m_bHasCompletionCallback;
}



//-----------------------------------------------------------------------------
//
// Console Variables
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Various constructors
//-----------------------------------------------------------------------------
ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags /* = 0 */ )
{
	Create( pName, pDefaultValue, flags );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString )
{
	Create( pName, pDefaultValue, flags, pHelpString );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, false, 0.0, false, 0.0, callback );
}

ConVar::ConVar( const char *pName, const char *pDefaultValue, int flags, const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t callback )
{
	Create( pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax, callback );
}


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar( void )
{
	if ( m_pszString )
	{
		delete[] m_pszString;
		m_pszString = NULL;
	}
}


//-----------------------------------------------------------------------------
// Install a change callback (there shouldn't already be one....)
//-----------------------------------------------------------------------------
void ConVar::InstallChangeCallback( FnChangeCallback_t callback )
{
	Assert( !m_fnChangeCallback || !callback );
	m_fnChangeCallback = callback;

	if(!m_pParent || m_pParent == this) {
		if ( m_fnChangeCallback )
		{
			// Call it immediately to set the initial value...
			m_fnChangeCallback( this, m_pszString, m_fValue );
		}
	} else {
		if ( m_fnChangeCallback )
		{
			// Call it immediately to set the initial value...
			m_fnChangeCallback( m_pParent, m_pParent->m_pszString, m_pParent->m_fValue );
		}
	}
}

bool ConVar::IsFlagSet( int flag ) const
{
	return ( flag & GetFlags() ) ? true : false;
}

const char *ConVar::GetHelpText( void ) const
{
	if(m_pParent)
		return m_pParent->m_pszHelpString;
	else
		return m_pszHelpString;
}

void ConVar::AddFlags( int flags )
{
	m_nFlags |= flags;
	ModifyFlags( m_nFlags );

	if(m_pParent && m_pParent != this) {
		m_pParent->m_nFlags |= flags;
		ModifyFlags( m_pParent->m_nFlags );
	}
}

bool ConVar::IsRegistered( void ) const
{
	return m_bRegistered;
}

const char *ConVar::GetName( void ) const
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ConVar::IsCommand( void ) const
{ 
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
//-----------------------------------------------------------------------------
void ConVar::Init()
{
	BaseClass::Init();
}

void ConVar::Init( IConCommandBaseAccessor *pAccessor )
{
	BaseClass::Init( pAccessor );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetValue( const char *value )
{
	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, value );
			return;
		}
	}

	float fNewValue;
	char  tempVal[ 32 ];
	char  *val;

	float flOldValue = (m_pParent ? m_pParent->m_fValue : m_fValue);

	val = (char *)value;
	if ( !value )
		fNewValue = 0.0f;
	else
		fNewValue = ( float )atof( value );

	if ( ClampValue( fNewValue ) )
	{
		Q_snprintf( tempVal,sizeof(tempVal), "%f", fNewValue );
		val = tempVal;
	}

	// Redetermine value
	m_fValue		= fNewValue;
	m_nValue		= ( int )( fNewValue );

	if(m_pParent && m_pParent != this) {
		m_pParent->m_fValue		= m_fValue;
		m_pParent->m_nValue		= m_nValue;
	}

	if ( !IsFlagSet(FCVAR_NEVER_AS_STRING ) )
	{
		ChangeStringValue( val, flOldValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *tempVal - 
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue( const char *tempVal, float flOldValue )
{
	Assert( !IsFlagSet(FCVAR_NEVER_AS_STRING ) );

 	char* pszOldValue = (char*)stackalloc( m_pParent ? m_pParent->m_StringLength : m_StringLength );
	memcpy( pszOldValue, (m_pParent ? m_pParent->m_pszString : m_pszString), (m_pParent ? m_pParent->m_StringLength : m_StringLength) );

	if ( tempVal )
	{
		int len = Q_strlen(tempVal) + 1;

		if ( len > m_StringLength)
		{
			if (m_pszString)
			{
				delete[] m_pszString;
			}

			m_pszString	= new char[len];
			m_StringLength = len;
		}

		memcpy( m_pszString, tempVal, len );

		if(m_pParent && m_pParent != this) {
			if ( len > m_pParent->m_StringLength)
			{
				if (m_pParent->m_pszString)
				{
					delete[] m_pParent->m_pszString;
				}

				m_pParent->m_pszString	= new char[len];
				m_pParent->m_StringLength = len;
			}

			memcpy( m_pParent->m_pszString, tempVal, len );
		}
	}
	else 
	{
		*m_pszString = 0;

		if(m_pParent && m_pParent != this) {
			*m_pParent->m_pszString = 0;
		}
	}

	if(!m_pParent || m_pParent == this) {
		// If nothing has changed, don't do the callbacks.
		if (V_strcmp(pszOldValue, m_pszString) != 0)
		{
			// Invoke any necessary callback function
			if ( m_fnChangeCallback )
			{
				m_fnChangeCallback( this, pszOldValue, flOldValue );
			}

			/*
			engine calls IClientRenderTargets->InitClientRenderTargets before IBaseClientDLL::Init

			which leads to:

			engine ->
				IClientRenderTargets->InitClientRenderTargets ->
					IClientShadowMgr::InitRenderTargets ->
						r_flashlightdepthres.SetValue -> crash because convars arent registered yet
			*/
			if(IsRegistered()) {
				g_pCVar->CallGlobalChangeCallbacks( this, pszOldValue, flOldValue );
			}
		}
	} else {
		if (V_strcmp(pszOldValue, m_pParent->m_pszString) != 0)
		{
			// Invoke any necessary callback function
			if ( m_fnChangeCallback )
			{
				m_fnChangeCallback( m_pParent, pszOldValue, flOldValue );
			}

			if ( m_pParent->m_fnChangeCallback && m_pParent->m_fnChangeCallback != m_fnChangeCallback )
			{
				m_pParent->m_fnChangeCallback( m_pParent, pszOldValue, flOldValue );
			}

			g_pCVar->CallGlobalChangeCallbacks( m_pParent, pszOldValue, flOldValue );
		}
	}

	stackfree( pszOldValue );
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp
// Input  : value - 
// Output : Returns true if value changed
//-----------------------------------------------------------------------------
bool ConVar::ClampValue( float& value )
{
	if(m_pParent) {
		if ( m_pParent->m_bHasMin && ( value < m_pParent->m_fMinVal ) )
		{
			value = m_pParent->m_fMinVal;
			return true;
		}
		
		if ( m_pParent->m_bHasMax && ( value > m_pParent->m_fMaxVal ) )
		{
			value = m_pParent->m_fMaxVal;
			return true;
		}
	} else {
		if ( m_bHasMin && ( value < m_fMinVal ) )
		{
			value = m_fMinVal;
			return true;
		}
		
		if ( m_bHasMax && ( value > m_fMaxVal ) )
		{
			value = m_fMaxVal;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetFloatValue( float fNewValue )
{
	if ( fNewValue == (m_pParent ? m_pParent->m_fValue : m_fValue) )
		return;

	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, fNewValue );
			return;
		}
	}

	// Check bounds
	ClampValue( fNewValue );

	// Redetermine value
	float flOldValue = (m_pParent ? m_pParent->m_fValue : m_fValue);
	m_fValue		= fNewValue;
	m_nValue		= ( int )m_fValue;

	if(m_pParent && m_pParent != this) {
		m_pParent->m_fValue		= m_fValue;
		m_pParent->m_nValue		= m_nValue;
	}

	if ( !IsFlagSet(FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal), "%f", (m_pParent ? m_pParent->m_fValue : m_fValue) );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetIntValue( int nValue )
{
	if ( nValue == (m_pParent ? m_pParent->m_nValue : m_nValue) )
		return;

	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, nValue );
			return;
		}
	}

	float fValue = (float)nValue;
	if ( ClampValue( fValue ) )
	{
		nValue = ( int )( fValue );
	}

	// Redetermine value
	float flOldValue = (m_pParent ? m_pParent->m_fValue : m_fValue);
	m_fValue		= fValue;
	m_nValue		= nValue;

	if(m_pParent && m_pParent != this) {
		m_pParent->m_fValue = m_fValue;
		m_pParent->m_nValue = m_nValue;
	}

	if ( !IsFlagSet(FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal ), "%d", (m_pParent ? m_pParent->m_nValue : m_nValue) );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

void ConVar::InternalSetBoolValue( bool nValue )
{
	if ( nValue == (m_pParent ? m_pParent->m_nValue : m_nValue) )
		return;

	if ( IsFlagSet( FCVAR_MATERIAL_THREAD_MASK ) )
	{
		if ( g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed() )
		{
			g_pCVar->QueueMaterialThreadSetValue( this, nValue );
			return;
		}
	}

	float fValue = nValue ? 1.0f : 0.0f;
	if ( ClampValue( fValue ) )
	{
		if(fValue > 0.0f)
			nValue = true;
		else
			nValue = false;
	}

	// Redetermine value
	float flOldValue = (m_pParent ? m_pParent->m_fValue : m_fValue);
	m_fValue		= fValue;
	m_nValue		= nValue;

	if(m_pParent && m_pParent != this) {
		m_pParent->m_fValue = m_fValue;
		m_pParent->m_nValue = m_nValue;
	}

	if ( !IsFlagSet(FCVAR_NEVER_AS_STRING ) )
	{
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal ), "%d", (m_pParent ? m_pParent->m_nValue : m_nValue) );
		ChangeStringValue( tempVal, flOldValue );
	}
	else
	{
		Assert( !m_fnChangeCallback );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Private creation
//-----------------------------------------------------------------------------
void ConVar::Create( const char *pName, const char *pDefaultValue, int flags /*= 0*/,
	const char *pHelpString /*= NULL*/, bool bMin /*= false*/, float fMin /*= 0.0*/,
	bool bMax /*= false*/, float fMax /*= false*/, FnChangeCallback_t callback /*= NULL*/ )
{
	m_pParent = this;

	// Name should be static data
	SetDefault( pDefaultValue );

	m_StringLength = V_strlen( m_pszDefaultValue ) + 1;
	m_pszString = new char[m_StringLength];
	memcpy( m_pszString, m_pszDefaultValue, m_StringLength );

	m_bHasMin = bMin;
	m_fMinVal = fMin;
	m_bHasMax = bMax;
	m_fMaxVal = fMax;

	m_fnChangeCallback = callback;

	m_fValue = ( float )atof( m_pszString );
	m_nValue = atoi( m_pszString ); // dont convert from float to int and lose bits

	// Bounds Check, should never happen, if it does, no big deal
	if ( m_bHasMin && ( m_fValue < m_fMinVal ) )
	{
		Assert( 0 );
	}

	if ( m_bHasMax && ( m_fValue > m_fMaxVal ) )
	{
		Assert( 0 );
	}

	BaseClass::CreateBase( pName, pHelpString, flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char *value)
{
	InternalSetValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( float value )
{
	InternalSetFloatValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue( int value )
{
	InternalSetIntValue( value );
}

void ConVar::SetValue( bool value )
{
	InternalSetBoolValue( value );
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value
//-----------------------------------------------------------------------------
void ConVar::Revert( void )
{
	// Force default value again
	SetValue( m_pParent ? m_pParent->m_pszDefaultValue : m_pszDefaultValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : minVal - 
// Output : true if there is a min set
//-----------------------------------------------------------------------------
bool ConVar::GetMin( float& minVal ) const
{
	if(m_pParent) {
		minVal = m_pParent->m_fMinVal;
		return m_pParent->m_bHasMin;
	} else {
		minVal = m_fMinVal;
		return m_bHasMin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : maxVal - 
//-----------------------------------------------------------------------------
bool ConVar::GetMax( float& maxVal ) const
{
	if(m_pParent) {
		maxVal = m_pParent->m_fMaxVal;
		return m_pParent->m_bHasMax;
	} else {
		maxVal = m_fMaxVal;
		return m_bHasMax;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *ConVar::GetDefault( void ) const
{
	if(m_pParent)
		return m_pParent->m_pszDefaultValue;
	else
		return m_pszDefaultValue;
}

void ConVar::SetDefault( const char *pszDefault ) 
{ 
	m_pszDefaultValue = pszDefault ? pszDefault : "";
	Assert( m_pszDefaultValue );

	if(m_pParent && m_pParent != this) {
		m_pParent->m_pszDefaultValue = m_pszDefaultValue;
	}
}

ConVar_ServerBounded *ConVar::GetServerBounded()
{
	ConVar_ServerBounded *pServer = NULL;

	if( (m_nFlags & FCVAR_SERVERBOUNDED) != 0 ) {
		pServer = reinterpret_cast<ConVar_ServerBounded *>(this);
	} else if(m_pParent && m_pParent != this) {
		if( (m_pParent->m_nFlags & FCVAR_SERVERBOUNDED) != 0 ) {
			pServer = reinterpret_cast<ConVar_ServerBounded *>(this);
		}
	}

	if(!pServer) {
		pServer = dynamic_cast<ConVar_ServerBounded *>(this);
		if(!pServer && m_pParent && m_pParent != this) {
			pServer = dynamic_cast<ConVar_ServerBounded *>(m_pParent);
		}
	}

	return pServer;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetBaseFloatValue( void ) const
{
	if(m_pParent)
		return m_pParent->m_fValue;
	else
		return m_fValue;
}

float ConVar::GetFloat( void ) const
{
	const ConVar_ServerBounded *pServer = GetServerBounded();
	if(pServer)
		return pServer->GetFloat();
	else
		return GetBaseFloatValue();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetBaseIntValue( void ) const 
{
	if(m_pParent)
		return m_pParent->m_nValue;
	else
		return m_nValue;
}

int ConVar::GetInt( void ) const 
{
	const ConVar_ServerBounded *pServer = GetServerBounded();
	if(pServer)
		return pServer->GetInt();
	else
		return GetBaseIntValue();
}

bool ConVar::GetBaseBoolValue( void ) const 
{
	if(m_pParent)
		return m_pParent->m_nValue != 0;
	else
		return m_nValue != 0;
}

bool ConVar::GetBool( void ) const 
{
	const ConVar_ServerBounded *pServer = GetServerBounded();
	if(pServer)
		return pServer->GetBool();
	else
		return GetBaseBoolValue();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
// Output : const char *
//-----------------------------------------------------------------------------
const char *ConVar::GetBaseStringValue( void ) const 
{
	if ( IsFlagSet( FCVAR_NEVER_AS_STRING ) )
		return "FCVAR_NEVER_AS_STRING";

	if(m_pParent)
		return m_pParent->m_pszString ? m_pParent->m_pszString : "";
	else
		return m_pszString ? m_pszString : "";
}

const char *ConVar::GetString( void ) const 
{
	if ( IsFlagSet( FCVAR_NEVER_AS_STRING ) )
		return "FCVAR_NEVER_AS_STRING";

	const ConVar_ServerBounded *pServer = GetServerBounded();
	if(pServer) {
		char tempVal[ 32 ];
		Q_snprintf( tempVal, sizeof( tempVal ), "%f", pServer->GetFloat() );

		int len = Q_strlen(tempVal) + 1;

		if ( len > m_StringLength)
		{
			if (m_pszString)
			{
				delete[] m_pszString;
			}

			const_cast<ConVar *>(this)->m_pszString	= new char[len];
			const_cast<ConVar *>(this)->m_StringLength = len;
		}

		memcpy( m_pszString, tempVal, len );

		if(m_pParent && m_pParent != this) {
			if ( len > m_pParent->m_StringLength)
			{
				if (m_pParent->m_pszString)
				{
					delete[] m_pParent->m_pszString;
				}

				m_pParent->m_pszString	= new char[len];
				m_pParent->m_StringLength = len;
			}

			memcpy( m_pParent->m_pszString, tempVal, len );
		}
	}

	return GetBaseStringValue();
}

void CONVAR_StringToColor( Color &color, const char *pString )
{
	char tempString[128];
	int	j;

	V_strcpy_safe( tempString, pString );
	const char *pfront;
	const char* pstr = pfront = tempString;

	for ( j = 0; j < 4; j++ )
	{
		color[j] = atoi( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if ( !*pstr )
			break;
		pstr++;
		pfront = pstr;
	}

	for ( j++; j < 4; j++ )
	{
		color[j] = 0;
	}
}

Color ConVar::GetColor() const
{
	Color clr;
	CONVAR_StringToColor( clr, GetString() );
	return clr;
}

//-----------------------------------------------------------------------------
// This version is simply used to make reading convars simpler.
// Writing convars isn't allowed in this mode
//-----------------------------------------------------------------------------
CEmptyConVar s_EmptyConVar;

void ConVarRef::Init( const char *pName, bool bIgnoreMissing )
{
	m_pConVar = g_pCVar ? g_pCVar->FindVar( pName ) : &s_EmptyConVar;
	if ( !m_pConVar )
	{
		m_pConVar = &s_EmptyConVar;
	}
	if( !IsValid() )
	{
	#if 0
		static bool bFirst = true;
		if ( bFirst )
		{
			if ( !bIgnoreMissing )
			{
				Log_Warning( LOG_CONVAR,"ConVarRef %s doesn't point to an existing ConVar\n", pName );
			}
			bFirst = false;
		}
	#else
		Log_Warning( LOG_CONVAR,"ConVarRef %s doesn't point to an existing ConVar\n", pName );
	#endif
	}
}

ConVarRef::ConVarRef( IConVar *pConVar )
{
	m_pConVar = (ConVar *)(pConVar ? pConVar : static_cast<IConVar *>(&s_EmptyConVar));
}

ConVarRef::ConVarRef( ConVar *pConVar )
{
	m_pConVar = pConVar ? pConVar : &s_EmptyConVar;
}

bool ConVarRef::IsValid() const
{
	return m_pConVar != &s_EmptyConVar;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintFlags( const ConCommandBase *var )
{
	bool any = false;
	if ( var->IsFlagSet( FCVAR_GAMEDLL ) )
	{
		Log_Msg( LOG_CONVAR," game" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CLIENTDLL ) )
	{
		Log_Msg( LOG_CONVAR, " client" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_ARCHIVE ) )
	{
		Log_Msg( LOG_CONVAR, " archive" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_NOTIFY ) )
	{
		Log_Msg( LOG_CONVAR, " notify" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_NOT_CONNECTED ) )
	{
		Log_Msg( LOG_CONVAR, " notconnected" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CHEAT ) )
	{
		Log_Msg( LOG_CONVAR, " cheat" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_REPLICATED ) )
	{
		Log_Msg( LOG_CONVAR, " replicated" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_SERVER_CAN_EXECUTE ) )
	{
		Log_Msg( LOG_CONVAR, " server_can_execute" );
		any = true;
	}

	if ( var->IsFlagSet( FCVAR_CLIENTCMD_CAN_EXECUTE ) )
	{
		Log_Msg( LOG_CONVAR, " clientcmd_can_execute" );
		any = true;
	}

	if ( any )
	{
		Log_Msg( LOG_CONVAR, "\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription( const ConCommandBase *pVar )
{
	bool bMin, bMax;
	float fMin, fMax;
	const char *pStr;

	Assert( pVar );

	if ( !pVar->IsCommand() )
	{
		ConVar *var = ( ConVar * )pVar;
		const ConVar_ServerBounded *pBounded = dynamic_cast<const ConVar_ServerBounded*>( var );

		bMin = var->GetMin( fMin );
		bMax = var->GetMax( fMax );

		const char *value = NULL;
		char tempVal[ 32 ];

		if ( pBounded || var->IsFlagSet( FCVAR_NEVER_AS_STRING ) )
		{
			value = tempVal;
			
			int intVal = pBounded ? pBounded->GetInt() : var->GetBaseIntValue();
			float floatVal = pBounded ? pBounded->GetFloat() : var->GetBaseFloatValue();

			if ( fabs( (float)intVal - floatVal ) < 0.000001 )
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%d", intVal );
			}
			else
			{
				Q_snprintf( tempVal, sizeof( tempVal ), "%f", floatVal );
			}
		}
		else
		{
			value = var->GetString();
		}

		if ( value )
		{
			Log_Msg( LOG_CONVAR, "\"%s\" = \"%s\"", var->GetName(), value );

			if ( stricmp( value, var->GetDefault() ) )
			{
				Log_Msg( LOG_CONVAR, " ( def. \"%s\" )", var->GetDefault() );
			}
		}

		if ( bMin )
		{
			Log_Msg( LOG_CONVAR, " min. %f", fMin );
		}
		if ( bMax )
		{
			Log_Msg( LOG_CONVAR, " max. %f", fMax );
		}

		Log_Msg( LOG_CONVAR, "\n" );

		// Handled virtualized cvars.
		if ( pBounded && fabs( pBounded->GetFloat() - pBounded->GetBaseFloatValue() ) > 0.0001f )
		{
			Log_Warning( LOG_CONVAR, "** NOTE: The real value is %.3f but the server has temporarily restricted it to %.3f **\n",
				pBounded->GetBaseFloatValue(), pBounded->GetFloat() );
		}
	}
	else
	{
		ConCommand *var = ( ConCommand * )pVar;

		Log_Msg( LOG_CONVAR, "\"%s\"\n", var->GetName() );
	}

	ConVar_PrintFlags( pVar );

	pStr = pVar->GetHelpText();
	if ( pStr && pStr[0] )
	{
		Log_Msg( LOG_CONVAR, " - %s\n", pStr );
	}
}
