//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//===========================================================================//

#ifndef CONVAR_H
#define CONVAR_H

#pragma once

#include "tier0/dbg.h"
#include "tier1/iconvar.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "icvar.h"
#include "Color.h"

DECLARE_LOGGING_CHANNEL( LOG_CONVAR );

#ifdef _WIN32
#define FORCEINLINE_CVAR FORCEINLINE
#elif POSIX
#define FORCEINLINE_CVAR inline
#else
#error "implement me"
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConVar;
class CCommand;
class ConCommand;
class ConCommandBase;
struct characterset_t;



//-----------------------------------------------------------------------------
// Any executable that wants to use ConVars need to implement one of
// these to hook up access to console variables.
//-----------------------------------------------------------------------------
class IConCommandBaseAccessor
{
public:
	// Flags is a combination of FCVAR flags in cvar.h.
	// hOut is filled in with a handle to the variable.
	virtual bool RegisterConCommandBase( ConCommandBase *pVar ) = 0;
};

class CDefaultAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase( ConCommandBase *pVar );
};

extern CDefaultAccessor s_DefaultAccessor;

//-----------------------------------------------------------------------------
// Called when a ConCommand needs to execute
//-----------------------------------------------------------------------------
typedef void ( *FnCommandCallbackVoid_t )( void );
typedef void ( *FnCommandCallback_t )( const CCommand &command );

#define COMMAND_COMPLETION_MAXITEMS		64
#define COMMAND_COMPLETION_ITEM_LENGTH	64

//-----------------------------------------------------------------------------
// Returns 0 to COMMAND_COMPLETION_MAXITEMS worth of completion strings
//-----------------------------------------------------------------------------
typedef int  ( *FnCommandCompletionCallback )( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] );


//-----------------------------------------------------------------------------
// Interface version
//-----------------------------------------------------------------------------
class ICommandCallback
{
public:
	virtual void CommandCallback( const CCommand &command ) = 0;
};

class ICommandCompletionCallback
{
public:
	virtual int  CommandCompletionCallback( const char *pPartial, CUtlVector< CUtlString > &commands ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
	friend class CCvar;
	friend class ConVar;
#ifdef __MINGW32__
	friend class ConVarBase;
#endif
	friend class ConCommand;
	friend void ConVar_Register( int nCVarFlag, IConCommandBaseAccessor *pAccessor );
	friend void ConVar_PublishToVXConsole();

	// FIXME: Remove when ConVar changes are done
	friend class CDefaultCvar;

protected:
	ConCommandBase( void );

public:
	ConCommandBase( const char *pName, const char *pHelpString = 0, 
		int flags = 0 ) = delete;

#ifdef __MINGW32__
private:
	virtual void __DTOR__();
protected:
	~ConCommandBase();
public:
#else
	virtual ~ConCommandBase( void );
#endif

	virtual	bool				IsCommand( void ) const;

	// Check flag
	virtual bool				IsFlagSet( int flag ) const;
	// Set flag
	virtual void				AddFlags( int flags );

	// Return name of cvar
	virtual const char			*GetName( void ) const;

	// Return help text for cvar
	virtual const char			*GetHelpText( void ) const;

	// Deal with next pointer
	const ConCommandBase		*GetNext( void ) const;
	ConCommandBase				*GetNext( void );
	
	virtual bool				IsRegistered( void ) const;

	// Returns the DLL identifier
	virtual CVarDLLIdentifier_t	GetDLLIdentifier() const;

	int GetFlags() const { return m_nFlags; }

protected:
	virtual void				CreateBase( const char *pName, const char *pHelpString = 0, 
									int flags = 0 );

	// Used internally by OneTimeInit to initialize/shutdown
	virtual void				Init();
	void				Init( IConCommandBaseAccessor *pAccessor );
	void						Shutdown();

	// Internal copy routine ( uses new operator from correct module )
	char						*CopyString( const char *from );

private:
	// Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, m_pNext is reset to point to the next
	// convar in the global list
	ConCommandBase				*m_pNext;

	// Has the cvar been added to the global list?
	bool						m_bRegistered;

	// Static data
	const char 					*m_pszName;
	const char 					*m_pszHelpString;
	
	// ConVar flags
	int							m_nFlags;
};


//-----------------------------------------------------------------------------
// Command tokenizer
//-----------------------------------------------------------------------------
class CCommand
{
public:
	CCommand();
	CCommand( int nArgC, const char **ppArgV );
	bool Tokenize( const char *pCommand, characterset_t *pBreakSet = NULL );
	void Reset();

	int ArgC() const;
	const char **ArgV() const;
	const char *ArgS() const;					// All args that occur after the 0th arg, in string form
	const char *GetCommandString() const;		// The entire command in string form, including the 0th arg
	const char *operator[]( int nIndex ) const;	// Gets at arguments
	const char *Arg( int nIndex ) const;		// Gets at arguments
	
	// Helper functions to parse arguments to commands.
	const char* FindArg( const char *pName ) const;
	int FindArgInt( const char *pName, int nDefaultVal ) const;

	static int MaxCommandLength();
	static characterset_t* DefaultBreakSet();

private:
	enum
	{
		COMMAND_MAX_ARGC = 64,
		COMMAND_MAX_LENGTH = 512,
	};

	int		m_nArgc;
	int		m_nArgv0Size;
	char	m_pArgSBuffer[ COMMAND_MAX_LENGTH ];
	char	m_pArgvBuffer[ COMMAND_MAX_LENGTH ];
	const char*	m_ppArgv[ COMMAND_MAX_ARGC ];
};

inline int CCommand::MaxCommandLength()
{
	return COMMAND_MAX_LENGTH - 1;
}

inline int CCommand::ArgC() const
{
	return m_nArgc;
}

inline const char **CCommand::ArgV() const
{
	return m_nArgc ? (const char**)m_ppArgv : NULL;
}

inline const char *CCommand::ArgS() const
{
	return m_nArgv0Size ? &m_pArgSBuffer[m_nArgv0Size] : "";
}

inline const char *CCommand::GetCommandString() const
{
	return m_nArgc ? m_pArgSBuffer : "";
}

inline const char *CCommand::Arg( int nIndex ) const
{
	// FIXME: Many command handlers appear to not be particularly careful
	// about checking for valid argc range. For now, we're going to
	// do the extra check and return an empty string if it's out of range
	if ( nIndex < 0 || nIndex >= m_nArgc )
		return "";
	return m_ppArgv[nIndex];
}

inline const char *CCommand::operator[]( int nIndex ) const
{
	return Arg( nIndex );
}


//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
friend class CCvar;

public:
	typedef ConCommandBase BaseClass;

protected:
	ConCommand( void ) = default;

public:
	ConCommand( const char *pName, const char *pHelpString = 0, 
		int flags = 0 ) = delete;

	ConCommand( const char *pName, FnCommandCallbackVoid_t callback, 
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( const char *pName, FnCommandCallback_t callback, 
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( const char *pName, ICommandCallback *pCallback, 
		const char *pHelpString = 0, int flags = 0, ICommandCompletionCallback *pCommandCompletionCallback = 0 );

#ifdef __MINGW32__
private:
	void __DTOR__();
#endif

public:
	VIRTUAL_OVERLOAD ~ConCommand( void );

	virtual	bool IsCommand( void ) const;

	virtual int AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands );

	virtual bool CanAutoComplete( void );

	// Invoke the function
	virtual void Dispatch( const CCommand &command );

protected:
	// NOTE: To maintain backward compat, we have to be very careful:
	// All public virtual methods must appear in the same order always
	// since engine code will be calling into this code, which *does not match*
	// in the mod code; it's using slightly different, but compatible versions
	// of this class. Also: Be very careful about adding new fields to this class.
	// Those fields will not exist in the version of this class that is instanced
	// in mod code.

	// Call this function when executing the command
	union
	{
		FnCommandCallbackVoid_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback;
		ICommandCallback *m_pCommandCallback; 
	};

	union
	{
		FnCommandCompletionCallback	m_fnCompletionCallback;
		ICommandCompletionCallback *m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingNewCommandCallback : 1;
	bool m_bUsingCommandCallbackInterface : 1;
};

class ConCommandLower : public ConCommand
{
public:
	ConCommandLower( void ) = delete;
	ConCommandLower( const char *pName, const char *pHelpString = 0, 
		int flags = 0 ) = delete;

	ConCommandLower( const char *pName, FnCommandCallbackVoid_t callback, 
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommandLower( const char *pName, FnCommandCallback_t callback, 
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommandLower( const char *pName, ICommandCallback *pCallback, 
		const char *pHelpString = 0, int flags = 0, ICommandCompletionCallback *pCommandCompletionCallback = 0 );

	virtual void				CreateBase( const char *pName, const char *pHelpString = 0, 
									int flags = 0 );

private:
	char m_szNameLower[64];
};

class ConCommandLinked : public ConCommand
{
public:
	ConCommandLinked( void ) = delete;
	ConCommandLinked( const char *pName, const char *pHelpString = 0, 
		int flags = 0 ) = delete;

	ConCommandLinked( const char *pName, FnCommandCallbackVoid_t callback, 
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommandLinked( const char *pName, FnCommandCallback_t callback, 
		const char *pHelpString = 0, int flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommandLinked( const char *pName, ICommandCallback *pCallback, 
		const char *pHelpString = 0, int flags = 0, ICommandCompletionCallback *pCommandCompletionCallback = 0 );

	virtual void				CreateBase( const char *pName, const char *pHelpString = 0, 
									int flags = 0 );

	virtual void Dispatch( const CCommand &command );

private:
	friend class ConCommandBase;

	ConCommand *m_pParent;
};

class ConVar_ServerBounded;

//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
#ifndef __MINGW32__
typedef ConVar ConVarBase;
#endif

#ifdef __MINGW32__
class ConVarBase : public ConCommandBase
#else
class ConVar : public ConCommandBase, public DO_NOT_USE_IConVar
#endif
{
friend class CCvar;
friend class ConVarRef;

public:
	typedef ConCommandBase BaseClass;

#ifdef __MINGW32__
	#define ConVar ConVarBase
#endif

#ifndef __MINGW32__
	ConVar( void ) = delete;

	ConVar( const char *pName, const char *pDefaultValue, int flags = 0);

	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString );
	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax );
	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, FnChangeCallback_t callback );
	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
		FnChangeCallback_t callback );
#else
protected:
	ConVarBase( void );
public:
#endif

	VIRTUAL_OVERLOAD ~ConVar( void );

#ifdef __MINGW32__
	#undef ConVar
#endif

#ifdef __MINGW32__
private:
	void __DTOR__();
#endif

public:
	VIRTUAL_OVERLOAD bool				IsFlagSet( int flag ) const;
	virtual const char*			GetHelpText( void ) const;
	virtual bool				IsRegistered( void ) const;
	VIRTUAL_OVERLOAD const char			*GetName( void ) const;
	virtual void				AddFlags( int flags );
	virtual	bool				IsCommand( void ) const;

#ifndef __MINGW32__
	// Install a change callback (there shouldn't already be one....)
	void InstallChangeCallback( FnChangeCallback_t callback );
#endif

	// Retrieve value
	float			GetBaseFloatValue( void ) const;
	int			GetBaseIntValue( void ) const;
	bool			GetBaseBoolValue( void ) const;
	char const	   *GetBaseStringValue( void ) const;

	float			GetFloat( void ) const;
	int			GetInt( void ) const;
	bool			GetBool() const;
	char const	   *GetString( void ) const;
	Color			GetColor( void ) const;

	void SetMin(float min);
	void SetMax(float max);

	// Any function that allocates/frees memory needs to be virtual or else you'll have crashes
	//  from alloc/free across dll/exe boundaries.
	
	// These just call into the IConCommandBaseAccessor to check flags and set the var (which ends up calling InternalSetValue).
	VIRTUAL_OVERLOAD void				SetValue( const char *value );
	VIRTUAL_OVERLOAD void				SetValue( float value );
	VIRTUAL_OVERLOAD void				SetValue( int value );
	void				SetValue( bool value );
	
	// Reset to default value
	void						Revert( void );

	// True if it has a min/max setting
	bool						GetMin( float& minVal ) const;
	bool						GetMax( float& maxVal ) const;
	const char					*GetDefault( void ) const;
	void						SetDefault( const char *pszDefault );

private:
	// Called by CCvar when the value of a var is changing.
	virtual void				InternalSetValue(const char *value);
	// For CVARs marked FCVAR_NEVER_AS_STRING
	virtual void				InternalSetFloatValue( float fNewValue );
	virtual void				InternalSetIntValue( int nValue );
	void				InternalSetBoolValue( bool nValue );

	virtual bool				ClampValue( float& value );
	virtual void				ChangeStringValue( const char *tempVal, float flOldValue )
#ifdef __MINGW32__
	= 0
#endif
	;

	virtual void				Create( const char *pName, const char *pDefaultValue, int flags = 0,
									const char *pHelpString = 0, bool bMin = false, float fMin = 0.0,
									bool bMax = false, float fMax = false, FnChangeCallback_t callback = 0 )
#ifdef __MINGW32__
	= 0
#endif
	;

	// Used internally by OneTimeInit to initialize.
	virtual void				Init();
	void				Init( IConCommandBaseAccessor *pAccessor );

public:
	ConVar_ServerBounded *GetServerBounded();
	const ConVar_ServerBounded *GetServerBounded() const { return const_cast<ConVarBase *>(this)->GetServerBounded(); }

	ConVarBase *GetTarget();
	const ConVarBase *GetTarget() const;

	bool IsRootVar() const;

	int GetFlags() const;

protected:
#ifdef __MINGW32__
	friend class ConVar;
#endif
	friend struct IConVarRef;

private:
#ifdef __MINGW32__
	DO_NOT_USE_IConVar *m_pIConVar;
#endif

protected:
	// This either points to "this" or it points to the original declaration of a ConVar.
	// This allows ConVars to exist in separate modules, and they all use the first one to be declared.
	// m_pParent->m_pParent must equal m_pParent (ie: m_pParent must be the root, or original, ConVar).
	ConVarBase						*m_pParent;

	// Static data
	const char					*m_pszDefaultValue;
	
	// Value
	// Dynamically allocated
	char						*m_pszString;
	int							m_StringLength;

	// Values
	float						m_fValue;
	int							m_nValue;

	// Min/Max values
	bool						m_bHasMin;
	float						m_fMinVal;
	bool						m_bHasMax;
	float						m_fMaxVal;
	
	// Call this function when ConVar changes
	FnChangeCallback_t			m_fnChangeCallback;
};

#ifdef __MINGW32__
class ConVar : public ConVarBase, private DO_NOT_USE_IConVar
{
public:
	typedef ConVarBase BaseClass;

	ConVar( void ) = delete;

	ConVar( const char *pName, const char *pDefaultValue, int flags = 0);

	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString );
	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax );
	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, FnChangeCallback_t callback );
	ConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
		FnChangeCallback_t callback );

	~ConVar( void );

private:
	void __DTOR__();

public:
	// Install a change callback (there shouldn't already be one....)
	void InstallChangeCallback( FnChangeCallback_t callback );

	virtual void				ChangeStringValue( const char *tempVal, float flOldValue );

	virtual void				Create( const char *pName, const char *pDefaultValue, int flags = 0,
									const char *pHelpString = 0, bool bMin = false, float fMin = 0.0,
									bool bMax = false, float fMax = false, FnChangeCallback_t callback = 0 );

	virtual bool				IsFlagSet( int flag ) const { return ConVarBase::IsFlagSet(flag); }
	virtual const char			*GetName( void ) const { return ConVarBase::GetName(); }

	virtual void				SetValue( const char *value ) { ConVarBase::SetValue(value); }
	virtual void				SetValue( float value ) { ConVarBase::SetValue(value); }
	virtual void				SetValue( int value ) { ConVarBase::SetValue(value); }
};
#endif

inline ConVarBase			*ICvar::FindVarBase ( const char *var_name ) { return (ConVarBase *)DO_NOT_USE_FindVar(var_name); }
inline const ConVarBase	*ICvar::FindVarBase ( const char *var_name ) const { return (const ConVarBase *)DO_NOT_USE_FindVar(var_name); }

inline ConVarBase *ConVarBase::GetTarget()
{
	return m_pParent ? (ConVarBase *)m_pParent : this;
}

inline const ConVarBase *ConVarBase::GetTarget() const
{
	return const_cast<ConVarBase *>(this)->GetTarget();
}

inline bool ConVarBase::IsRootVar() const
{
	return !m_pParent || (ConVarBase *)m_pParent == this;
}

inline int ConVarBase::GetFlags() const
{
	return m_pParent ? m_pParent->m_nFlags : m_nFlags;
}

FORCEINLINE_CVAR void ConVarBase::SetMin( float v )
{
	m_bHasMin = true;
	m_fMinVal = v;

	if(!IsRootVar()) {
		m_pParent->m_bHasMin = true;
		m_pParent->m_fMinVal = v;
	}
}

FORCEINLINE_CVAR void ConVarBase::SetMax( float v )
{
	m_bHasMax = true;
	m_fMaxVal = v;

	if(!IsRootVar()) {
		m_pParent->m_bHasMax = true;
		m_pParent->m_fMaxVal = v;
	}
}

struct IConVarRef
{
public:
	IConVarRef() = delete;

	IConVarRef( ConVar *var_ )
	{
	#ifdef __MINGW32__
		var = var_->m_pIConVar;
	#else
		var = static_cast<DO_NOT_USE_IConVar *>(var_);
	#endif
	}

	~IConVarRef() {}

	operator ConVar *() const = delete;
	operator const ConVar *() const = delete;

#ifdef __MINGW32__
	operator ConVarBase *() const = delete;
	operator const ConVarBase *() const = delete;
#endif

	void SetValue( const char *pValue ) { var->SetValue(pValue); }
	void SetValue( float flValue ) { var->SetValue(flValue); }
	void SetValue( int nValue ) { var->SetValue(nValue); }

	// Return name of command
	const char *GetName( void ) const { return var->GetName(); }

	// Accessors.. not as efficient as using GetState()/GetInfo()
	// if you call these methods multiple times on the same IConVar
	bool IsFlagSet( int nFlag ) const { return var->IsFlagSet(nFlag); }

	// Get/Set value
	float GetFloat( void ) const { return GetLinkedConVarBase()->GetFloat(); }
	int GetInt( void ) const { return GetLinkedConVarBase()->GetInt(); }
	bool GetBool() const { return GetLinkedConVarBase()->GetBool(); }
	const char *GetString( void ) const { return GetLinkedConVarBase()->GetString(); }
	Color GetColor(void) const { return GetLinkedConVarBase()->GetColor(); }

	void SetValue( bool bValue ) { GetLinkedConVarBase()->SetValue( bValue); }

	const char *GetDefault() const { return GetLinkedConVarBase()->GetDefault(); }

private:
	ConVar *GetLinkedConVar()
	{ return ((ConVar *)var); }
	const ConVar *GetLinkedConVar() const
	{ return ((ConVar *)var); }
	ConVarBase *GetLinkedConVarBase()
	{ return ((ConVarBase *)GetLinkedConVar()); }
	const ConVarBase *GetLinkedConVarBase() const
	{ return ((ConVarBase *)GetLinkedConVar()); }

	DO_NOT_USE_IConVar *var;
};

COMPILE_TIME_ASSERT(sizeof(IConVarRef) == sizeof(DO_NOT_USE_IConVar *));

class CEmptyConVar : public ConVar
{
private:
	CEmptyConVar( const char *pName, const char *pDefaultValue, int flags = 0) = delete;

	CEmptyConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString ) = delete;
	CEmptyConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax ) = delete;
	CEmptyConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, FnChangeCallback_t callback ) = delete;
	CEmptyConVar( const char *pName, const char *pDefaultValue, int flags, 
		const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
		FnChangeCallback_t callback ) = delete;

public:
	CEmptyConVar()
		: ConVar( "", "", FCVAR_UNREGISTERED|FCVAR_HIDDEN|FCVAR_DEVELOPMENTONLY, NULL, true, 0.0f, true, 0.0f )
	{
	}

	// Used for optimal read access
	virtual void InternalSetValue(const char *value) {}
	virtual void InternalSetFloatValue( float fNewValue ) {}
	virtual void InternalSetIntValue( int nValue ) {}
	virtual void SetValue( const char *pValue ) {}
	virtual void SetValue( float flValue ) {}
	virtual void SetValue( int nValue ) {}

	virtual bool ClampValue( float& value )
	{ value = 0.0f; return true; }
	virtual void ChangeStringValue( const char *tempVal, float flOldValue ) {}

	virtual bool IsFlagSet( int flag ) const
	{ return (flag & (FCVAR_UNREGISTERED|FCVAR_HIDDEN|FCVAR_DEVELOPMENTONLY)) != 0; }

	virtual void AddFlags( int flags )
	{ ConVar::AddFlags(flags & (FCVAR_UNREGISTERED|FCVAR_HIDDEN|FCVAR_DEVELOPMENTONLY)); }
};

extern CEmptyConVar s_EmptyConVar;

//-----------------------------------------------------------------------------
// Used to read/write convars that already exist (replaces the FindVar method)
//-----------------------------------------------------------------------------
class ConVarRef
{
public:
	ConVarRef( const char *pName ) { Init( pName ); }
	ConVarRef( const char *pName, bool bIgnoreMissing ) { Init( pName, bIgnoreMissing ); }
	ConVarRef( const char *pName, const char * ) = delete;
	ConVarRef( ConVarBase *pConVar );

	void Init( const char *pName, bool bIgnoreMissing );
	void Init( const char *pName ) { Init( pName, false ); }
	bool IsValid() const;
	bool IsFlagSet( int nFlags ) const;
	ConVarBase *GetLinkedConVar();

	// Get/Set value
	float GetFloat( void ) const;
	int GetInt( void ) const;
	bool GetBool() const;
	const char *GetString( void ) const;
	Color GetColor(void) const;

	void SetValue( const char *pValue );
	void SetValue( float flValue );
	void SetValue( int nValue );
	void SetValue( bool bValue );

	const char *GetName() const;

	const char *GetDefault() const;

private:
	// High-speed method to read convar data
	ConVarBase *m_pConVar;
};


//-----------------------------------------------------------------------------
// Did we find an existing convar of that name?
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR bool ConVarRef::IsFlagSet( int nFlags ) const
{
	return ( m_pConVar->IsFlagSet( nFlags ) != 0 );
}

FORCEINLINE_CVAR ConVarBase *ConVarRef::GetLinkedConVar()
{
	return m_pConVar;
}

FORCEINLINE_CVAR const char *ConVarRef::GetName() const
{
	return m_pConVar->GetName();
}


//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float ConVarRef::GetFloat( void ) const
{
	return m_pConVar->GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int ConVarRef::GetInt( void ) const 
{
	return m_pConVar->GetInt();
}

FORCEINLINE_CVAR bool ConVarRef::GetBool( void ) const 
{
	return m_pConVar->GetBool();
}

FORCEINLINE_CVAR Color ConVarRef::GetColor() const
{
	return m_pConVar->GetColor();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR const char *ConVarRef::GetString( void ) const 
{
	Assert( !IsFlagSet( FCVAR_NEVER_AS_STRING ) );
	return m_pConVar->GetString();
}


FORCEINLINE_CVAR void ConVarRef::SetValue( const char *pValue )
{
	m_pConVar->SetValue( pValue );
}

FORCEINLINE_CVAR void ConVarRef::SetValue( float flValue )
{
	m_pConVar->SetValue( flValue );
}

FORCEINLINE_CVAR void ConVarRef::SetValue( int nValue )
{
	m_pConVar->SetValue( nValue );
}

FORCEINLINE_CVAR void ConVarRef::SetValue( bool bValue )
{
	m_pConVar->SetValue( bValue );
}

FORCEINLINE_CVAR const char *ConVarRef::GetDefault() const
{
	return m_pConVar->GetDefault();
}


//-----------------------------------------------------------------------------
// Called by the framework to register ConCommands with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( int nCVarFlag = 0, IConCommandBaseAccessor *pAccessor = NULL );
void ConVar_Unregister( );


//-----------------------------------------------------------------------------
// Utility methods 
//-----------------------------------------------------------------------------
void ConVar_PrintFlags( const ConCommandBase *var );
void ConVar_PrintDescription( const ConCommandBase *pVar );


//-----------------------------------------------------------------------------
// Purpose: Utility class to quickly allow ConCommands to call member methods
//-----------------------------------------------------------------------------
#pragma warning (disable : 4355 )

template< class T >
class CConCommandMemberAccessor : public ConCommand, public ICommandCallback, public ICommandCompletionCallback
{
	typedef ConCommand BaseClass;
	typedef void ( T::*FnMemberCommandCallback_t )( const CCommand &command );
	typedef int  ( T::*FnMemberCommandCompletionCallback_t )( const char *pPartial, CUtlVector< CUtlString > &commands );

public:
	CConCommandMemberAccessor( T* pOwner, const char *pName, FnMemberCommandCallback_t callback, const char *pHelpString = 0,
		int flags = 0, FnMemberCommandCompletionCallback_t completionFunc = 0 ) :
		BaseClass( pName, this, pHelpString, flags, ( completionFunc != 0 ) ? this : NULL )
	{
		m_pOwner = pOwner;
		m_Func = callback;
		m_CompletionFunc = completionFunc;
	}

	~CConCommandMemberAccessor()
	{
		Shutdown();
	}

	void SetOwner( T* pOwner )
	{
		m_pOwner = pOwner;
	}

	virtual void CommandCallback( const CCommand &command )
	{
		Assert( m_pOwner && m_Func );
		(m_pOwner->*m_Func)( command );
	}

	virtual int  CommandCompletionCallback( const char *pPartial, CUtlVector< CUtlString > &commands )
	{
		Assert( m_pOwner && m_CompletionFunc );
		return (m_pOwner->*m_CompletionFunc)( pPartial, commands );
	}

private:
	T* m_pOwner;
	FnMemberCommandCallback_t m_Func;
	FnMemberCommandCompletionCallback_t m_CompletionFunc;
};

#pragma warning ( default : 4355 )


//-----------------------------------------------------------------------------
// Purpose: Utility macros to quicky generate a simple console command
//-----------------------------------------------------------------------------
#define CON_COMMAND( name, description ) \
   static void name##_callback( const CCommand &args ); \
   static ConCommand name##_command( #name, name##_callback, description ); \
   static void name##_callback( const CCommand &args )

#define CON_COMMAND_LINKED( name, description ) \
   static void name##_callback( const CCommand &args ); \
   static ConCommandLinked name##_command( #name, name##_callback, description ); \
   static void name##_callback( const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_SHARED( name, description ) \
		static void name( const CCommand &args ); \
		static ConCommand name##_command_client( V_STRINGIFY( name##_client ), name, description ); \
		static void name( const CCommand &args )
#else
	#define CON_COMMAND_SHARED( name, description ) \
		static void name( const CCommand &args ); \
		static ConCommand name##_command( #name, name, description ); \
		static void name( const CCommand &args )
#endif

#define CON_COMMAND_F( name, description, flags ) \
   static void name##_callback( const CCommand &args ); \
   static ConCommand name##_command( #name, name##_callback, description, flags ); \
   static void name##_callback( const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_F_SHARED( name, description, flags ) \
		static void name( const CCommand &args ); \
		static ConCommand name##_command_client( V_STRINGIFY( name##_client ), name, description, flags ); \
		static void name( const CCommand &args )
#else
	#define CON_COMMAND_F_SHARED( name, description, flags ) \
		static void name( const CCommand &args ); \
		static ConCommand name##_command( #name, name, description, flags ); \
		static void name( const CCommand &args )
#endif

#define CON_COMMAND_F_COMPLETION( name, description, flags, completion ) \
	static void name##_callback( const CCommand &args ); \
	static ConCommand name##_command( #name, name##_callback, description, flags, completion ); \
	static void name##_callback( const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_F_COMPLETION_SHARED( name, description, flags, completion ) \
		static void name( const CCommand &args ); \
		static ConCommand name##_command_client( V_STRINGIFY( name##_client ), name, description, flags, completion ); \
		static void name( const CCommand &args )
#else
	#define CON_COMMAND_F_COMPLETION_SHARED( name, description, flags, completion ) \
		static void name( const CCommand &args ); \
		static ConCommand name##_command( #name, name, description, flags, completion ); \
		static void name( const CCommand &args )
#endif

#define CON_COMMAND_EXTERN( name, _funcname, description ) \
	void _funcname( const CCommand &args ); \
	static ConCommand name##_command( #name, _funcname, description ); \
	void _funcname( const CCommand &args )

#define CON_COMMAND_EXTERN_F( name, _funcname, description, flags ) \
	void _funcname( const CCommand &args ); \
	static ConCommand name##_command( #name, _funcname, description, flags ); \
	void _funcname( const CCommand &args )

#define CON_COMMAND_MEMBER_F( _thisclass, name, _funcname, description, flags ) \
	void _funcname( const CCommand &args );						\
	friend class CCommandMemberInitializer_##_funcname;			\
	class CCommandMemberInitializer_##_funcname					\
	{															\
	public:														\
		CCommandMemberInitializer_##_funcname() : m_ConCommandAccessor( NULL, name, &_thisclass::_funcname, description, flags )	\
		{														\
			m_ConCommandAccessor.SetOwner( GET_OUTER( _thisclass, m_##_funcname##_register ) );	\
		}														\
	private:													\
		CConCommandMemberAccessor< _thisclass > m_ConCommandAccessor;	\
	};															\
																\
	CCommandMemberInitializer_##_funcname m_##_funcname##_register;		\


#endif // CONVAR_H
