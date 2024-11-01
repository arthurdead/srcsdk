//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef NETWORKVAR_H
#define NETWORKVAR_H
#pragma once


#include "tier0/dbg.h"
#include "convar.h"
#include "string_t.h"

#if defined( CLIENT_DLL ) || defined( GAME_DLL )
	#include "basehandle.h"
	#include "ehandle.h"
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL nullptr

namespace std
{
	using nullptr_t = decltype(nullptr);
}

#pragma warning( disable : 4284 ) // warning C4284: return type for 'CNetworkVarT<int>::operator ->' is 'int *' (ie; not a UDT or reference to a UDT.  Will produce errors if applied using infix notation)

#define MyOffsetOf( type, var ) offsetof(type, var)

#ifdef _DEBUG
	extern bool g_bUseNetworkVars;
	#define CHECK_USENETWORKVARS if(g_bUseNetworkVars)
#else
	#define CHECK_USENETWORKVARS // don't check for g_bUseNetworkVars
#endif

// network vars use memcmp when fields are set.  To ensure proper behavior your
// object's memory should be initialized to zero.  This happens for entities automatically
// use this for other classes.
class CMemZeroOnNew
{
public:
	void *operator new( size_t nSize )
	{
		void *pMem = MemAlloc_Alloc( nSize );
		V_memset( pMem, 0, nSize );
		return pMem;
	}

	void* operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
	{
		void *pMem = MemAlloc_Alloc( nSize, pFileName, nLine );
		V_memset( pMem, 0, nSize );
		return pMem;
	}

	void operator delete(void *pData)
	{
		if ( pData )
		{
			MemAlloc_Free( pData );
		}
	}

	void operator delete( void* pData, int nBlockUse, const char *pFileName, int nLine )
	{
		if ( pData )
		{
			MemAlloc_Free( pData, pFileName, nLine);
		}
	}
};

extern int InternalCheckDeclareClass( const char *pClassName, const char *pClassNameMatch, void *pTestPtr, void *pBasePtr );


template <typename T> 
inline int CheckDeclareClass_Access( T *, const char *pShouldBe )
{
	return T::CheckDeclareClass( pShouldBe );
}

#ifndef _STATIC_LINKED
#if defined(_DEBUG)
	#define VALIDATE_DECLARE_CLASS 1
#endif
#endif

#ifdef  VALIDATE_DECLARE_CLASS
	#define CHECK_DECLARE_CLASS_IMPL( DLLClassName, sendTable ) \
		template <> int CheckDeclareClass_Access<sendTable::ignored>(sendTable::ignored *, const char *pIgnored) \
		{ \
			return DLLClassName::CheckDeclareClass( V_STRINGIFY( DLLClassName ) ); \
		} \
		namespace sendTable \
		{ \
			int verifyDeclareClass = ::CheckDeclareClass_Access( (sendTable::ignored*)NULL, NULL ); \
		}

	#define CHECK_DECLARE_CLASS_DECL( className, baseClassName ) \
		template <typename X> friend int ::CheckDeclareClass_Access(X *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			InternalCheckDeclareClass( pShouldBe, V_STRINGIFY( className ), (ThisClass*)0xFFFFF, (BaseClass*)(ThisClass*)0xFFFFF ); \
			return CheckDeclareClass_Access( (BaseClass *)NULL, baseClassName::s_pClassnameStr ); \
		}

	#define CHECK_DECLARE_CLASS_GAMEROOT_DECL( className, baseClassName ) \
		template <typename X> friend int ::CheckDeclareClass_Access(X *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			return InternalCheckDeclareClass( pShouldBe, V_STRINGIFY( className ), (ThisClass*)0xFFFFF, (BaseClass*)(ThisClass*)0xFFFFF ); \
		}

	#define CHECK_DECLARE_CLASS_NOBASE_DECL( className ) \
		template <typename X> friend int ::CheckDeclareClass_Access(X *, const char *pShouldBe); \
		static int CheckDeclareClass( const char *pShouldBe ) \
		{ \
			return InternalCheckDeclareClass( pShouldBe, V_STRINGIFY( className ), NULL, NULL ); \
		} 

#else
	#define CHECK_DECLARE_CLASS_IMPL( DLLClassName, sendTable )

	#define CHECK_DECLARE_CLASS_DECL( className, baseClassName )

	#define CHECK_DECLARE_CLASS_GAMEROOT_DECL( className, baseClassName )

	#define CHECK_DECLARE_CLASS_NOBASE_DECL( className )
#endif

#define DECLARE_CLASS( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass; \
	static inline const char *s_pClassnameStr = V_STRINGIFY( className ); \
	CHECK_DECLARE_CLASS_DECL( className, baseClassName )

// Use this macro when you have a base class, but it's part of a library that doesn't use network vars
// or any of the things that use ThisClass or BaseClass.
#define DECLARE_CLASS_GAMEROOT( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass; \
	static inline const char *s_pClassnameStr = V_STRINGIFY( className ); \
	CHECK_DECLARE_CLASS_GAMEROOT_DECL( className, baseClassName )

#define DECLARE_CLASS_NOBASE( className ) \
	typedef className ThisClass; \
	static inline const char *s_pClassnameStr = V_STRINGIFY( className ); \
	CHECK_DECLARE_CLASS_NOBASE_DECL( className )

// Deprecated macro formerly used to work around VC++98 bug
#define DECLARE_CLASS_NOFRIEND( className, baseClassName ) \
	DECLARE_CLASS( className, baseClassName )


// All classes that contain CNetworkVars need a NetworkStateChanged() function. If the class is not an entity,
// it needs to forward the call to the entity it's in. These macros can help.
	
// These macros setup an entity pointer in your class. Use IMPLEMENT_NETWORKVAR_CHAIN before you do
// anything inside the class itself.

#ifdef GAME_DLL
class CBaseEntity;
typedef CBaseEntity CGameBaseEntity;
#elif defined CLIENT_DLL
class C_BaseEntity;
typedef C_BaseEntity CGameBaseEntity;
#else
class CGameBaseEntity;
#endif

class CAutoInitEntPtr
{
public:
	CAutoInitEntPtr()
	{
		m_pEnt = NULL;
	}
	CGameBaseEntity *m_pEnt;
};

//TODO: Currently, these don't get the benefit of tracking changes to individual vars.
// Would be nice if they did.
#define DECLARE_NETWORKVAR_CHAIN() \
	template <typename T> friend int ServerClassInit(T *);	\
	template <typename T> friend int ClientClassInit(T *); \
	CAutoInitEntPtr __m_pChainEntity; \
	virtual void NetworkStateChanged() { CHECK_USENETWORKVARS { if(__m_pChainEntity.m_pEnt) { __m_pChainEntity.m_pEnt->NetworkStateChanged(); } } } \
	virtual void NetworkStateChanged( void *pVar ) { CHECK_USENETWORKVARS { if(__m_pChainEntity.m_pEnt) { __m_pChainEntity.m_pEnt->NetworkStateChanged(); } } }

#define IMPLEMENT_NETWORKVAR_CHAIN( varName ) \
	(varName)->__m_pChainEntity.m_pEnt = this;



// Use this macro when you want to embed a structure inside your entity and have CNetworkVars in it.
template< class T >
static inline void DispatchNetworkStateChanged( T *pObj )
{
	CHECK_USENETWORKVARS pObj->NetworkStateChanged();
}
template< class T >
static inline void DispatchNetworkStateChanged( T *pObj, void *pVar )
{
	CHECK_USENETWORKVARS pObj->NetworkStateChanged( pVar );
}


#define DECLARE_EMBEDDED_NETWORKVAR() \
	template <typename T> friend int ServerClassInit(T *);	\
	template <typename T> friend int ClientClassInit(T *); \
	virtual void NetworkStateChanged() {} \
	virtual void NetworkStateChanged( void *pProp ) {}

// NOTE: Assignment operator is disabled because it doesn't call copy constructors of scalar types within the aggregate, so they are not marked changed
#define CNetworkVarEmbedded( type, name ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	static inline int GetOffset_##name() { return MyOffsetOf(ThisClass,name); } \
	typedef ThisClass ThisClass_##name; \
	class NetworkVar_##name final : public type \
	{ \
	private: \
		template< class T > \
		NetworkVar_##name& operator=( const T &val ) \
		{ *((type*)this) = val; return *this; } \
	public: \
		type *operator&() { return ((type *)this); } \
		const type *operator&() const { return ((type *)this); } \
		void CopyFrom( const type &src ) { *((type *)this) = src; NetworkStateChanged(); } \
		type & GetForModify( void ) { NetworkStateChanged(); return *((type *)this); } \
		virtual void NetworkStateChanged() \
		{ \
			DispatchNetworkStateChanged( (ThisClass_##name*)( ((char*)this) - GetOffset_##name() ) ); \
		} \
		virtual void NetworkStateChanged( void *pVar ) \
		{ \
			DispatchNetworkStateChanged( (ThisClass_##name*)( ((char*)this) - GetOffset_##name() ), pVar ); \
		} \
	}; \
	NetworkVar_##name name; 

template<typename T>
FORCEINLINE void NetworkVarConstruct( T &x ) { x = T(0); }
FORCEINLINE void NetworkVarConstruct( color32_s &x ) { x.r = x.g = x.b = x.a = 255; }
FORCEINLINE void NetworkVarConstruct( color24_s &x ) { x.r = x.g = x.b = 255; }

template< class Type, class Changer >
class CNetworkVarBaseImpl
{
public:
	inline CNetworkVarBaseImpl()
	{
		NetworkVarConstruct( m_Value );
	}

	inline explicit CNetworkVarBaseImpl( Type val )
	: m_Value( val )
	{
		NetworkStateChanged();
	}

	template< class C >
	CNetworkVarBaseImpl& operator=( const C &val ) 
	{ 
		Set( ( const Type )val ); 
		return *this;
	}
	
	template< class C >
	CNetworkVarBaseImpl& operator=( const CNetworkVarBaseImpl< C, Changer > &val ) 
	{ 
		Set( ( const Type )val.m_Value ); 
		return *this;
	}

	void SetDirect( const Type &val )
	{
		NetworkStateChanged();
		m_Value = val;
	}
	
	void Set( const Type &val )
	{
		if ( memcmp( &m_Value, &val, sizeof(Type) ) )
		{
			NetworkStateChanged();
			m_Value = val;
		}
	}
	
	Type& GetForModify()
	{
		NetworkStateChanged();
		return m_Value;
	}

	template< class C >
	CNetworkVarBaseImpl& operator+=( const C &val ) 
	{
		Set( m_Value + ( const Type )val ); 
		return *this;
	}

	template< class C >
	CNetworkVarBaseImpl& operator-=( const C &val ) 
	{
		Set( m_Value - ( const Type )val ); 
		return *this;
	}
	
	template< class C >
	CNetworkVarBaseImpl& operator/=( const C &val ) 
	{
		Set( m_Value / ( const Type )val ); 
		return *this;
	}
	
	template< class C >
	CNetworkVarBaseImpl& operator*=( const C &val ) 
	{
		Set( m_Value * ( const Type )val ); 
		return *this;
	}
	
	template< class C >
	CNetworkVarBaseImpl& operator^=( const C &val ) 
	{
		Set( m_Value ^ ( const Type )val ); 
		return *this;
	}

	template< class C >
	CNetworkVarBaseImpl& operator|=( const C &val ) 
	{
		Set( m_Value | ( const Type )val ); 
		return *this;
	}

	template< class C >
	bool operator==( const C &val ) const
	{ return (m_Value == val); }
	template< class C >
	bool operator!=( const C &val ) const
	{ return (m_Value != val); }
	template< class C >
	bool operator>( const C &val ) const
	{ return (m_Value > val); }
	template< class C >
	bool operator<( const C &val ) const
	{ return (m_Value < val); }
	template< class C >
	bool operator>=( const C &val ) const
	{ return (m_Value >= val); }
	template< class C >
	bool operator<=( const C &val ) const
	{ return (m_Value <= val); }

	const Type& operator++()
	{
		(*this += 1);
		return m_Value;
	}

	const Type& operator--()
	{
		(*this -= 1);
		return m_Value;
	}
	
	Type operator++( int ) // postfix version..
	{
		Type val = m_Value;
		(*this += 1);
		return val;
	}

	Type operator--( int ) // postfix version..
	{
		Type val = m_Value;
		(*this -= 1);
		return val;
	}
	
	// For some reason the compiler only generates type conversion warnings for this operator when used like 
	// CNetworkVarBase<unsigned char> = 0x1
	// (it warns about converting from an int to an unsigned char).
	template< class C >
	CNetworkVarBaseImpl& operator&=( const C &val ) 
	{	
		Set( m_Value & ( const Type )val ); 
		return *this;
	}

	operator const Type&() const 
	{
		return m_Value; 
	}
	
	const Type& Get() const 
	{
		return m_Value; 
	}
	
	const Type* operator->() const 
	{
		return &m_Value; 
	}

	Type m_Value;

protected:
	inline void NetworkStateChanged()
	{
		Changer::NetworkStateChanged( this );
	}
};

template< class Type, class Changer >
class CNetworkVarBase : public CNetworkVarBaseImpl<Type, Changer>
{
public:
	using CNetworkVarBaseImpl<Type, Changer>::CNetworkVarBaseImpl;
	using CNetworkVarBaseImpl<Type, Changer>::operator=;
};

template< class Type, class Changer >
class CNetworkColor32Base : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	inline void Init( byte rVal, byte gVal, byte bVal )
	{
		this->SetR( rVal );
		this->SetG( gVal );
		this->SetB( bVal );
	}
	inline void Init( byte rVal, byte gVal, byte bVal, byte aVal )
	{
		this->SetR( rVal );
		this->SetG( gVal );
		this->SetB( bVal );
		this->SetA( aVal );
	}

	CNetworkColor32Base& operator=( const Type &val ) 
	{ 
		this->Set( val ); 
		return *this;
	}

	CNetworkColor32Base& operator=( const CNetworkColor32Base<Type,Changer> &val ) 
	{ 
		this->Set( val.m_Value );
		return *this;
	}
	
	inline byte GetR() const { return this->m_Value.r; }
	inline byte GetG() const { return this->m_Value.g; }
	inline byte GetB() const { return this->m_Value.b; }
	inline byte GetA() const { return this->m_Value.a; }
	inline void SetR( byte val ) { this->SetVal( this->m_Value.r, val ); }
	inline void SetG( byte val ) { this->SetVal( this->m_Value.g, val ); }
	inline void SetB( byte val ) { this->SetVal( this->m_Value.b, val ); }
	inline void SetA( byte val ) { this->SetVal( this->m_Value.a, val ); }

protected:
	inline void SetVal( byte &out, const byte &in )
	{
		if ( out != in )
		{
			this->NetworkStateChanged();
			out = in;
		}
	}
};

template< class Type, class Changer >
class CNetworkColor24Base : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	inline void Init( byte rVal, byte gVal, byte bVal )
	{
		this->SetR( rVal );
		this->SetG( gVal );
		this->SetB( bVal );
	}

	CNetworkColor24Base& operator=( const Type &val ) 
	{ 
		this->Set( val ); 
		return *this;
	}

	CNetworkColor24Base& operator=( const CNetworkColor24Base<Type,Changer> &val ) 
	{ 
		this->Set( val.m_Value );
		return *this;
	}
	
	inline byte GetR() const { return this->m_Value.r; }
	inline byte GetG() const { return this->m_Value.g; }
	inline byte GetB() const { return this->m_Value.b; }
	inline void SetR( byte val ) { this->SetVal( this->m_Value.r, val ); }
	inline void SetG( byte val ) { this->SetVal( this->m_Value.g, val ); }
	inline void SetB( byte val ) { this->SetVal( this->m_Value.b, val ); }

protected:
	inline void SetVal( byte &out, const byte &in )
	{
		if ( out != in )
		{
			this->NetworkStateChanged();
			out = in;
		}
	}
};

// Network vector wrapper.
template< class Type, class Changer >
class CNetworkVectorBase : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	inline void Init( float ix=0, float iy=0, float iz=0 ) 
	{
		this->SetX( ix );
		this->SetY( iy );
		this->SetZ( iz );
	}
	
	CNetworkVectorBase& operator=( const Type &val ) 
	{ 
		base::Set( val ); 
		return *this;
	}

	CNetworkVectorBase& operator=( const CNetworkVectorBase<Type,Changer> &val ) 
	{ 
		base::Set( val.m_Value );
		return *this;
	}

	inline float GetX() const { return this->m_Value.x; }
	inline float GetY() const { return this->m_Value.y; }
	inline float GetZ() const { return this->m_Value.z; }
	inline float operator[]( int i ) const { return this->m_Value[i]; }

	inline void SetX( float val ) { this->DetectChange( this->m_Value.x, val ); }
	inline void SetY( float val ) { this->DetectChange( this->m_Value.y, val ); }
	inline void SetZ( float val ) { this->DetectChange( this->m_Value.z, val ); }
	inline void Set( int i, float val ) { this->DetectChange( this->m_Value[i], val ); }

	bool operator==( const Type &val ) const 
	{ 
		return this->m_Value == (Type)val; 
	}

	bool operator!=( const Type &val ) const 
	{
		return this->m_Value != (Type)val; 
	}

	Type operator+( const Type &val ) const 
	{
		return this->m_Value + val; 
	}

	Type operator-( const Type &val ) const
	{ 
		return this->m_Value - val; 
	}

	Type operator*( const Type &val ) const
	{
		return this->m_Value * val; 
	}

	CNetworkVectorBase& operator*=( float val )
	{
		base::Set( this->m_Value * val );
		return *this;
	}

	Type operator*( float val ) const
	{
		return this->m_Value * val; 
	}

	Type operator/( const Type &val ) const
	{
		return this->m_Value / val; 
	}

private:
	inline void DetectChange( float &out, float in ) 
	{
		if ( out != in ) 
		{
			this->NetworkStateChanged();
			out = in;
		}
	}
};


// Network vector wrapper.
template< class Type, class Changer >
class CNetworkQuaternionBase : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	inline void Init( float ix=0, float iy=0, float iz=0, float iw = 0 ) 
	{
		SetX( ix );
		SetY( iy );
		SetZ( iz );
		SetW( iw );
	}
	
	CNetworkQuaternionBase& operator=( const Type &val ) 
	{ 
		base::Set( val ); 
		return *this;
	}

	CNetworkQuaternionBase& operator=( const CNetworkQuaternionBase<Type,Changer> &val ) 
	{ 
		base::Set( val.m_Value );
		return *this;
	}

	inline float GetX() const { return this->m_Value.x; }
	inline float GetY() const { return this->m_Value.y; }
	inline float GetZ() const { return this->m_Value.z; }
	inline float GetW() const { return this->m_Value.w; }
	inline float operator[]( int i ) const { return this->m_Value[i]; }

	inline void SetX( float val ) { DetectChange( this->m_Value.x, val ); }
	inline void SetY( float val ) { DetectChange( this->m_Value.y, val ); }
	inline void SetZ( float val ) { DetectChange( this->m_Value.z, val ); }
	inline void SetW( float val ) { DetectChange( this->m_Value.w, val ); }
	inline void Set( int i, float val ) { DetectChange( this->m_Value[i], val ); }

	bool operator==( const Type &val ) const 
	{ 
		return this->m_Value == (Type)val; 
	}

	bool operator!=( const Type &val ) const 
	{
		return this->m_Value != (Type)val; 
	}

	Type operator+( const Type &val ) const 
	{
		return this->m_Value + val; 
	}

	Type operator-( const Type &val ) const
	{ 
		return this->m_Value - val; 
	}

	Type operator*( const Type &val ) const
	{
		return this->m_Value * val; 
	}

	CNetworkQuaternionBase& operator*=( float val )
	{
		base::Set( this->m_Value * val );
		return *this;
	}

	Type operator*( float val ) const
	{
		return this->m_Value * val; 
	}

	Type operator/( const Type &val ) const
	{
		return this->m_Value / val; 
	}

private:
	inline void DetectChange( float &out, float in ) 
	{
		if ( out != in ) 
		{
			this->NetworkStateChanged();
			out = in;
		}
	}
};


// Network ehandle wrapper.
#if defined( CLIENT_DLL ) || defined( GAME_DLL )
inline void NetworkVarConstruct( CBaseHandle &x ) { x.Term(); }

template< class Type >
inline void NetworkVarConstruct( CHandle<Type> &x ) { x.Term(); }

template< class Type, class Changer >
class CNetworkHandleBaseImpl : public CNetworkVarBaseImpl< CHandle<Type>, Changer >
{
	typedef CNetworkVarBaseImpl< CHandle<Type>, Changer > base;

public:
	CNetworkHandleBaseImpl &operator=( const Type *val ) 
	{ 
		this->Set( val ); 
		return *this;
	}

	CNetworkHandleBaseImpl &operator=( Type *val ) 
	{ 
		this->Set( val ); 
		return *this;
	}
		
	CNetworkHandleBaseImpl& operator=( const CNetworkHandleBaseImpl &val ) 
	{ 
		this->Set( val.m_Value );
		return *this;
	}

	CNetworkHandleBaseImpl& operator=( const CHandle<Type> &val ) 
	{ 
		this->Set( val );
		return *this;
	}

	CNetworkHandleBaseImpl& operator=( std::nullptr_t ) 
	{ 
		this->Term();
		return *this;
	}

	bool operator !() const 
	{ 
		return ( this->m_Value.Get() == NULL ); 
	}

	explicit operator bool() const 
	{ 
		return ( this->m_Value.Get() != NULL );
	}
	
	operator Type*() const 
	{ 
		return static_cast< Type* >( this->m_Value.Get() );
	}

	explicit operator const Type*() const 
	{ 
		return static_cast< const Type* >( this->m_Value.Get() );
	}

	explicit operator IHandleEntity*() const 
	{ 
		return static_cast< IHandleEntity* >( this->m_Value.Get() );
	}

	explicit operator const IHandleEntity*() const 
	{ 
		return static_cast< const IHandleEntity* >( this->m_Value.Get() );
	}

	void Set( const Type *val )
	{
		if ( this->m_Value.Get() != val )
		{
			this->NetworkStateChanged();
			this->m_Value.Set( val );
		}
	}

	void Set( Type *val )
	{
		if ( this->m_Value.Get() != val )
		{
			this->NetworkStateChanged();
			this->m_Value.Set( val );
		}
	}

	void Set( const CHandle<Type> &val )
	{
		if ( this->m_Value.operator!=(val) )
		{
			this->NetworkStateChanged();
			this->m_Value.Set( val );
		}
	}

	void Set( std::nullptr_t )
	{
		if ( this->m_Value.Get() != NULL )
		{
			this->NetworkStateChanged();
			this->m_Value.Term();
		}
	}

	void Term()
	{
		if ( this->m_Value.Get() != NULL )
		{
			this->NetworkStateChanged();
			this->m_Value.Term();
		}
	}
	
	Type* Get() const 
	{ 
		return static_cast< Type* >( this->m_Value.Get() );
	}

	Type* operator->() const 
	{ 
		return static_cast< Type* >( this->m_Value.Get() );
	}

	bool operator==( const Type *val ) const 
	{
		return this->m_Value.Get() == val; 
	}

	bool operator!=( const Type *val ) const 
	{
		return this->m_Value.Get() != val;
	}

	bool operator==( Type *val ) const 
	{
		return this->m_Value.Get() == val; 
	}

	bool operator!=( Type *val ) const 
	{
		return this->m_Value.Get() != val;
	}

	bool operator==( const IHandleEntity *val ) const 
	{
		return this->m_Value.Get() == val; 
	}

	bool operator!=( const IHandleEntity *val ) const 
	{
		return this->m_Value.Get() != val;
	}

	bool operator==( IHandleEntity *val ) const 
	{
		return this->m_Value.Get() == val; 
	}

	bool operator!=( IHandleEntity *val ) const 
	{
		return this->m_Value.Get() != val;
	}

	bool operator==( std::nullptr_t ) const 
	{
		return this->m_Value.Get() == NULL; 
	}

	bool operator!=( std::nullptr_t ) const 
	{
		return this->m_Value.Get() != NULL;
	}
};

template< class Type, class Changer >
class CNetworkHandleBase : public CNetworkHandleBaseImpl<Type, Changer>
{
public:
	using CNetworkHandleBaseImpl<Type, Changer>::CNetworkHandleBaseImpl;
	using CNetworkHandleBaseImpl<Type, Changer>::operator=;
	using CNetworkHandleBaseImpl<Type, Changer>::operator==;
	using CNetworkHandleBaseImpl<Type, Changer>::operator!=;
	using CNetworkHandleBaseImpl<Type, Changer>::Set;

	bool operator==( const CGameBaseEntity *val ) const 
	{
		return this->m_Value.Get() == val; 
	}

	bool operator!=( const CGameBaseEntity *val ) const 
	{
		return this->m_Value.Get() != val;
	}

	bool operator==( CGameBaseEntity *val ) const 
	{
		return this->m_Value.Get() == val; 
	}

	bool operator!=( CGameBaseEntity *val ) const 
	{
		return this->m_Value.Get() != val;
	}
};

template< class Changer >
class CNetworkHandleBase<CGameBaseEntity, Changer> : public CNetworkHandleBaseImpl<CGameBaseEntity, Changer>
{
public:
	using CNetworkHandleBaseImpl<CGameBaseEntity, Changer>::CNetworkHandleBaseImpl;
	using CNetworkHandleBaseImpl<CGameBaseEntity, Changer>::operator=;
	using CNetworkHandleBaseImpl<CGameBaseEntity, Changer>::operator==;
	using CNetworkHandleBaseImpl<CGameBaseEntity, Changer>::operator!=;
	using CNetworkHandleBaseImpl<CGameBaseEntity, Changer>::Set;
};

template< class Type, class Changer >
class CNetworkVarBase<CHandle<Type>, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkVarBase<CBaseHandle, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

#define CNetworkHandle( type, name ) CNetworkHandleInternal( type, name, NetworkStateChanged )

#define CNetworkHandleInternal( type, name, stateChangedFn ) \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkHandleBase, stateChangedFn )

#endif

template< class Type, class Changer >
class CNetworkStringTBaseImpl : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	using CNetworkVarBaseImpl<Type, Changer>::CNetworkVarBaseImpl;
	using CNetworkVarBaseImpl<Type, Changer>::operator=;
	using CNetworkVarBaseImpl<Type, Changer>::operator==;
	using CNetworkVarBaseImpl<Type, Changer>::operator!=;
	using CNetworkVarBaseImpl<Type, Changer>::Set;
};

template< class Changer >
class CNetworkVarBase<string_t, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

#define CNetworkStringT( name ) \
	NETWORK_VAR_START( string_t, name ) \
	NETWORK_VAR_END( string_t, name, CNetworkStringTBaseImpl, NetworkStateChanged )

// Use this macro to define a network variable.
#define CNetworkVar( type, name ) \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkVarBase, NetworkStateChanged )


// Use this macro when you have a base class with a variable, and it doesn't have that variable in a SendTable,
// but a derived class does. Then, the entity is only flagged as changed when the variable is changed in
// an entity that wants to transmit the variable.
#define CNetworkVarForDerived( type, name ) \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( void *pVar ) {} \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkVarBase, NetworkStateChanged_##name )

#define CNetworkVectorForDerived( name ) \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( void *pVar ) {} \
	CNetworkVectorInternal( Vector, name, NetworkStateChanged_##name )
	
#define CNetworkHandleForDerived( type, name ) \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( void *pVar ) {} \
	CNetworkHandleInternal( type, name, NetworkStateChanged_##name )
	
#define CNetworkArrayForDerived( type, name, count ) \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( void *pVar ) {} \
	CNetworkArrayInternal( type, name, count, NetworkStateChanged_##name )

#define IMPLEMENT_NETWORK_VAR_FOR_DERIVED( name ) \
	virtual void NetworkStateChanged_##name() { CHECK_USENETWORKVARS NetworkStateChanged(); } \
	virtual void NetworkStateChanged_##name( void *pVar ) { CHECK_USENETWORKVARS NetworkStateChanged( pVar ); }


// This virtualizes the change detection on the variable, but it is ON by default.
// Use this when you have a base class in which MOST of its derived classes use this variable
// in their SendTables, but there are a couple that don't (and they
// can use DISABLE_NETWORK_VAR_FOR_DERIVED).
#define CNetworkVarForDerived_OnByDefault( type, name ) \
	virtual void NetworkStateChanged_##name() { CHECK_USENETWORKVARS NetworkStateChanged(); } \
	virtual void NetworkStateChanged_##name( void *pVar ) { CHECK_USENETWORKVARS NetworkStateChanged( pVar ); } \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkVarBase, NetworkStateChanged_##name )

#define DISABLE_NETWORK_VAR_FOR_DERIVED( name ) \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( void *pVar ) {}



// Vectors + some convenient helper functions.
#define CNetworkVector( name ) CNetworkVectorInternal( Vector, name, NetworkStateChanged )
#define CNetworkQAngle( name ) CNetworkVectorInternal( QAngle, name, NetworkStateChanged )

#define CNetworkVectorInternal( type, name, stateChangedFn ) \
	NETWORK_VAR_START( type, name ) \
	NETWORK_VAR_END( type, name, CNetworkVectorBase, stateChangedFn )

#define CNetworkQuaternion( name ) \
	NETWORK_VAR_START( Quaternion, name ) \
	NETWORK_VAR_END( Quaternion, name, CNetworkQuaternionBase, NetworkStateChanged )

// Helper for color32's. Contains GetR(), SetR(), etc.. functions.
#define CNetworkColor32( name ) \
	NETWORK_VAR_START( color32, name ) \
	NETWORK_VAR_END( color32, name, CNetworkColor32Base, NetworkStateChanged )

#define CNetworkColor24( name ) \
	NETWORK_VAR_START( color24, name ) \
	NETWORK_VAR_END( color24, name, CNetworkColor24Base, NetworkStateChanged )

#define CNetworkString( name, length ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	class NetworkVar_##name \
	{ \
	public: \
		NetworkVar_##name() { m_Value[0] = '\0'; } \
		operator const char*() const { return m_Value; } \
		const char* Get() const { return m_Value; } \
		char* GetForModify() \
		{ \
			NetworkStateChanged(); \
			return m_Value; \
		} \
	protected: \
		inline void NetworkStateChanged() \
		{ \
		CHECK_USENETWORKVARS ((ThisClass*)(((char*)this) - MyOffsetOf(ThisClass,name)))->NetworkStateChanged(); \
		} \
	private: \
		char m_Value[length]; \
	}; \
	NetworkVar_##name name;




// Use this to define networked arrays.
// You can access elements for reading with operator[], and you can set elements with the Set() function.
#define CNetworkArrayInternal( type, name, count, stateChangedFn ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	class NetworkVar_##name \
	{ \
	public: \
		inline NetworkVar_##name() \
		{ \
			for ( int i = 0 ; i < count ; ++i ) \
				NetworkVarConstruct( m_Value[i] ); \
		} \
		template <typename T> friend int ServerClassInit(T *);	\
		const type& operator[]( int i ) const \
		{ \
			return Get( i ); \
		} \
		\
		const type& Get( int i ) const \
		{ \
			Assert( i >= 0 && i < count ); \
			return m_Value[i]; \
		} \
		\
		type& GetForModify( int i ) \
		{ \
			Assert( i >= 0 && i < count ); \
			NetworkStateChanged( i ); \
			return m_Value[i]; \
		} \
		\
		void Set( int i, const type &val ) \
		{ \
			Assert( i >= 0 && i < count ); \
			if( memcmp( &m_Value[i], &val, sizeof(type) ) ) \
			{ \
				NetworkStateChanged( i ); \
			       	m_Value[i] = val; \
			} \
		} \
		const type* Base() const { return m_Value; } \
		type* Base() { return m_Value; } \
		static int Count() { return count; } \
	protected: \
		inline void NetworkStateChanged( int index ) \
		{ \
			CHECK_USENETWORKVARS ((ThisClass*)(((char*)this) - MyOffsetOf(ThisClass,name)))->stateChangedFn( &m_Value[index] ); \
		} \
	public: \
		type m_Value[count]; \
	}; \
	NetworkVar_##name name;


#define CNetworkArray( type, name, count )  CNetworkArrayInternal( type, name, count, NetworkStateChanged )


// Internal macros used in definitions of network vars.
#define NETWORK_VAR_START( type, name ) \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass MakeANetworkVar_##name; \
	class NetworkVar_##name \
	{ \
	public: \
		template <typename T> friend int ServerClassInit(T *);


#define NETWORK_VAR_END( type, name, base, stateChangedFn ) \
	public: \
		static inline void NetworkStateChanged( void *ptr ) \
		{ \
			CHECK_USENETWORKVARS ((ThisClass*)(((char*)ptr) - MyOffsetOf(ThisClass,name)))->stateChangedFn( ptr ); \
		} \
	}; \
	base< type, NetworkVar_##name > name;



#endif // NETWORKVAR_H
