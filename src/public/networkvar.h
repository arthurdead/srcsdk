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

#include "engine/ivmodelinfo.h"
#include "studio.h"

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

abstract_class INetworkableObject
{
protected:
#ifndef CLIENT_DLL
	virtual void NetworkStateChanged() = 0;
	virtual void NetworkStateChanged( unsigned short offset ) = 0;
#endif
};

// Use this macro when you want to embed a structure inside your entity and have CNetworkVars in it.
#ifndef CLIENT_DLL
template< class T >
static inline void DispatchNetworkStateChanged( T *pObj )
{
	CHECK_USENETWORKVARS pObj->NetworkStateChanged();
}
template< class T >
static inline void DispatchNetworkStateChanged( T *pObj, unsigned short offset )
{
	CHECK_USENETWORKVARS pObj->NetworkStateChanged( offset );
}
#endif

#ifndef CLIENT_DLL
#define DECLARE_EMBEDDED_NETWORKVAR() \
	template <typename T> friend int ServerClassInit(T *);	\
	template <typename T> friend int ClientClassInit(T *); \
	virtual void NetworkStateChanged() override = 0; \
	virtual void NetworkStateChanged( unsigned short offset ) override = 0;
#else
#define DECLARE_EMBEDDED_NETWORKVAR() \
	template <typename T> friend int ServerClassInit(T *);	\
	template <typename T> friend int ClientClassInit(T *);
#endif

#define DECLARE_EMBEDDED_NETWORKVAR_NOCHECK() \
	template <typename T> friend int ServerClassInit(T *);	\
	template <typename T> friend int ClientClassInit(T *);

template<typename T>
FORCEINLINE void NetworkVarConstruct( T &x ) { Construct(&x); }
FORCEINLINE void NetworkVarConstruct( color32 &x ) { x.SetColor( 255, 255, 255, 255 ); }
FORCEINLINE void NetworkVarConstruct( color24 &x ) { x.SetColor( 255, 255, 255 ); }
FORCEINLINE void NetworkVarConstruct( modelindex_t &x ) { x = INVALID_MODEL_INDEX; }
template<typename T>
FORCEINLINE void NetworkVarConstruct( CUtlVector<T> &x ) { x.Purge(); }

template<typename T>
struct networkvar_type_traits
{
	using base_type = T;
	using const_ref = const T &;

	static base_type &GetRef(base_type &val)
	{
		return val;
	}
	static const base_type &GetRef(const base_type &val)
	{
		return val;
	}

	static base_type *GetPtr(base_type &val)
	{
		return &val;
	}
	static const base_type *GetPtr(const base_type &val)
	{
		return &val;
	}
};

template<typename T>
struct networkvar_type_traits<T *>
{
	using base_type = T;
	using const_ref = const T *;

	static base_type &GetRef(base_type *val)
	{
		return *val;
	}
	static const base_type &GetRef(const base_type *val)
	{
		return *val;
	}

	static base_type *GetPtr(base_type *val)
	{
		return val;
	}
	static const base_type *GetPtr(const base_type *val)
	{
		return val;
	}
};

template< class Type, class Changer >
class CNetworkVarBaseImpl
{
public:
	using BaseElemType_t = Type;

	inline CNetworkVarBaseImpl()
	{
		NetworkVarConstruct( this->GetValue_location() );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	inline explicit CNetworkVarBaseImpl( const Type &val )
	{
		CopyConstruct( &this->GetValue_location(), val );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	inline explicit CNetworkVarBaseImpl( Type &&val )
	{
		MoveConstruct( &this->GetValue_location(), Move( val ) );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	template< class C >
	inline explicit CNetworkVarBaseImpl( const C &val )
	{
		CopyConstruct( &this->GetValue_location(), val );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	template< class C >
	inline explicit CNetworkVarBaseImpl( C &&val )
	{
		MoveConstruct( &this->GetValue_location(), Move( val ) );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	inline CNetworkVarBaseImpl( const CNetworkVarBaseImpl &other )
	{
		CopyConstruct( &this->GetValue_location(), other.GetValue_location() );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	inline CNetworkVarBaseImpl( CNetworkVarBaseImpl &&other )
	{
		MoveConstruct( &this->GetValue_location(), Move( other.GetValue_location() ) );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
		other.CallOuterNetworkStateChanged();
	#endif
	}

	template< class C, class H >
	inline CNetworkVarBaseImpl( const CNetworkVarBaseImpl<C, H> &other )
	{
		CopyConstruct( &this->GetValue_location(), other.GetValue_location() );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
	}

	template< class C, class H >
	inline CNetworkVarBaseImpl( CNetworkVarBaseImpl<C, H> &&other )
	{
		MoveConstruct( &this->GetValue_location(), Move( other.GetValue_location() ) );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
		other.CallOuterNetworkStateChanged();
	#endif
	}

	CNetworkVarBaseImpl& operator=( const CNetworkVarBaseImpl &val ) 
	{ 
		this->Set( val.GetValue_location() );
		return *this;
	}

	CNetworkVarBaseImpl& operator=( CNetworkVarBaseImpl &&val ) 
	{ 
		this->Set( Move( val ) );
		return *this;
	}

	template< class C, class H >
	CNetworkVarBaseImpl& operator=( const CNetworkVarBaseImpl<C, H> &val ) 
	{ 
		this->Set( val.GetValue_location() );
		return *this;
	}

	template< class C, class H >
	CNetworkVarBaseImpl& operator=( CNetworkVarBaseImpl<C, H> &&val ) 
	{ 
		this->Set( Move( val ) );
		return *this;
	}

	template< class C >
	CNetworkVarBaseImpl& operator=( const C &val ) 
	{ 
		this->Set( val );
		return *this;
	}

	template< class C >
	CNetworkVarBaseImpl& operator=( C &&val ) 
	{ 
		this->Set( Move( val ) );
		return *this;
	}

	template< class C >
	void SetDirect( const C &val )
	{
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		this->GetValue_location() = val;
	}

	template< class C >
	void SetDirect( C &&val )
	{
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		this->GetValue_location() = Move(val);
	}

	template< class C, class H >
	void SetDirect( CNetworkVarBaseImpl<C, H> &&val ) 
	{ 
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
		val.CallOuterNetworkStateChanged();
	#endif
		this->GetValue_location() = Move(val.GetValue_location());
	}

	template< class C >
	void Set( const C &val )
	{
	#ifndef CLIENT_DLL
		if ( this->GetValue_location() != val )
		{
			this->SetDirect( val );
		}
	#else
		this->SetDirect( val );
	#endif
	}

	template< class C >
	void Set( C &&val )
	{
	#ifndef CLIENT_DLL
		if ( this->GetValue_location() != val )
		{
			this->SetDirect( Move( val ) );
		}
	#else
		this->SetDirect( Move( val ) );
	#endif
	}

	bool operator==( const Type &val ) const
	{ return (this->GetValue_location() == val); }
	bool operator!=( const Type &val ) const
	{ return (this->GetValue_location() != val); }

	bool operator==( const CNetworkVarBaseImpl &val ) const
	{ return (this->GetValue_location() == val.GetValue_location()); }
	bool operator!=( const CNetworkVarBaseImpl &val ) const
	{ return (this->GetValue_location() != val.GetValue_location()); }

	template< class C, class H >
	bool operator==( const CNetworkVarBaseImpl<C, H> &val ) const
	{ return (this->GetValue_location() == val.GetValue_location()); }
	template< class C, class H >
	bool operator!=( const CNetworkVarBaseImpl<C, H> &val ) const
	{ return (this->GetValue_location() != val.GetValue_location()); }

	template< class C >
	bool operator==( const C &val ) const
	{ return (this->GetValue_location() == val); }
	template< class C >
	bool operator!=( const C &val ) const
	{ return (this->GetValue_location() != val); }

	operator typename networkvar_type_traits<Type>::const_ref() const 
	{
		return this->GetValue_location();
	}

	const typename networkvar_type_traits<Type>::base_type *operator&() const
	{
		return networkvar_type_traits<Type>::GetPtr(this->GetValue_location());
	}

	const typename networkvar_type_traits<Type>::base_type* operator->() const 
	{
		return networkvar_type_traits<Type>::GetPtr(this->GetValue_location()); 
	}

	const typename networkvar_type_traits<Type>::base_type &Get() const
	{
		return networkvar_type_traits<Type>::GetRef(this->GetValue_location()); 
	}

	typename networkvar_type_traits<Type>::const_ref GetRaw() const
	{
		return this->GetValue_location(); 
	}

	typename networkvar_type_traits<Type>::base_type& GetForModify()
	{
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return networkvar_type_traits<Type>::GetRef(this->GetValue_location());
	}

	Type& GetForModifyRaw()
	{
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return this->GetValue_location();
	}

	typename networkvar_type_traits<Type>::base_type& GetWithoutModify()
	{
		return networkvar_type_traits<Type>::GetRef(this->GetValue_location());
	}

	Type& GetWithoutModifyRaw()
	{
		return this->GetValue_location();
	}

protected:
	Type &GetValue_location()
	{
		return Changer::GetValue_location_impl( this );
	}

	const Type &GetValue_location() const
	{
		return Changer::GetValue_location_impl( this );
	}

#ifndef CLIENT_DLL
	void CallOuterNetworkStateChanged()
	{
		Changer::CallOuterNetworkStateChanged_impl( this );
	}
#endif
};

template< class Type, class Changer >
class CNetworkVarEmbeddedPtrBase : public CNetworkVarBaseImpl< Type *, Changer >
{
	typedef CNetworkVarBaseImpl< Type *, Changer > base;

public:
	using CNetworkVarBaseImpl<Type *, Changer>::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;
};

template< class Type, class Changer >
class CNetworkVarEmbeddedBase : public Type
{
public:
	CNetworkVarEmbeddedBase() = default;

	CNetworkVarEmbeddedBase& operator=( const CNetworkVarEmbeddedBase &val ) = delete;
	CNetworkVarEmbeddedBase& operator=( CNetworkVarEmbeddedBase &&val ) = delete;
	CNetworkVarEmbeddedBase& operator=( const Type &val ) = delete;
	CNetworkVarEmbeddedBase& operator=( Type &&val ) = delete;
	template< class T >
	CNetworkVarEmbeddedBase& operator=( const T &val ) = delete;
	template< class T >
	CNetworkVarEmbeddedBase& operator=( T &&val ) = delete;

	CNetworkVarEmbeddedBase( const Type &val ) = delete;
	CNetworkVarEmbeddedBase( Type &&val ) = delete;
	CNetworkVarEmbeddedBase( const CNetworkVarEmbeddedBase &val ) = delete;
	CNetworkVarEmbeddedBase( CNetworkVarEmbeddedBase &&val ) = delete;

	using Type::Type;

protected:
#ifndef CLIENT_DLL
	void NetworkStateChanged() override
	{
		Changer::CallOuterNetworkStateChanged_impl( this );
	}

	void NetworkStateChanged( unsigned short offset ) override
	{
		Changer::CallOuterNetworkStateChanged_impl( this, offset );
	}
#endif
};

template< class Type, class Changer >
class CNetworkVarEmbeddedCopyableBase : public Type
{
public:
	CNetworkVarEmbeddedCopyableBase() = default;
	CNetworkVarEmbeddedCopyableBase(const CNetworkVarEmbeddedCopyableBase &) = default;
	CNetworkVarEmbeddedCopyableBase(CNetworkVarEmbeddedCopyableBase &&) = default;
	CNetworkVarEmbeddedCopyableBase &operator=(const CNetworkVarEmbeddedCopyableBase &) = default;
	CNetworkVarEmbeddedCopyableBase &operator=(CNetworkVarEmbeddedCopyableBase &&) = default;

	using Type::Type;
	using Type::operator=;

protected:
#ifndef CLIENT_DLL
	void NetworkStateChanged() override
	{
		Changer::CallOuterNetworkStateChanged_impl( this );
	}

	void NetworkStateChanged( unsigned short offset ) override
	{
		Changer::CallOuterNetworkStateChanged_impl( this, offset );
	}
#endif
};

template< class Type, class Changer >
class CNetworkVarArithmeticBaseImpl : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	using CNetworkVarBaseImpl<Type, Changer>::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

	template< class C >
	CNetworkVarArithmeticBaseImpl& operator+=( const C &val ) 
	{
		this->GetValue_location() += val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	const Type& operator++()
	{
		++this->GetValue_location();
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return this->GetValue_location();
	}
	
	Type operator++( int ) // postfix version..
	{
		Type val = this->GetValue_location();
		this->GetValue_location()++;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return val;
	}

	template< class C >
	Type operator+( const C &val ) const 
	{
		return this->GetValue_location() + val; 
	}

	template< class C >
	CNetworkVarArithmeticBaseImpl& operator-=( const C &val ) 
	{
		this->GetValue_location() -= val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	const Type& operator--()
	{
		--this->GetValue_location();
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return this->GetValue_location();
	}

	Type operator--( int ) // postfix version..
	{
		Type val = this->GetValue_location();
		this->GetValue_location()--;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return val;
	}

	template< class C >
	Type operator-( const C &val ) const
	{ 
		return this->GetValue_location() - val; 
	}
	
	template< class C >
	CNetworkVarArithmeticBaseImpl& operator/=( const C &val ) 
	{
		this->GetValue_location() /= val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	template< class C >
	Type operator/( const C &val ) const
	{
		return this->GetValue_location() / val; 
	}

	template< class C >
	CNetworkVarArithmeticBaseImpl& operator*=( const C &val ) 
	{
		this->GetValue_location() *= val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	template< class C >
	Type operator*( const C &val ) const
	{
		return this->GetValue_location() * val; 
	}
	
	template< class C >
	CNetworkVarArithmeticBaseImpl& operator^=( const C &val ) 
	{
		this->GetValue_location() ^= val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	// For some reason the compiler only generates type conversion warnings for this operator when used like 
	// CNetworkVarBase<unsigned char> = 0x1
	// (it warns about converting from an int to an unsigned char).
	template< class C >
	CNetworkVarArithmeticBaseImpl& operator&=( const C &val ) 
	{	
		this->GetValue_location() &= val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	template< class C >
	Type operator&( const C &val ) const
	{
		return this->GetValue_location() & val; 
	}

	template< class C >
	CNetworkVarArithmeticBaseImpl& operator|=( const C &val ) 
	{
		this->GetValue_location() |= val;
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return *this;
	}

	template< class C >
	Type operator|( const C &val ) const
	{
		return this->GetValue_location() | val; 
	}

	template< class C >
	bool operator>( const C &val ) const
	{ return (this->GetValue_location() > val); }
	template< class C >
	bool operator<( const C &val ) const
	{ return (this->GetValue_location() < val); }
	template< class C >
	bool operator>=( const C &val ) const
	{ return (this->GetValue_location() >= val); }
	template< class C >
	bool operator<=( const C &val ) const
	{ return (this->GetValue_location() <= val); }
};

template< class Type, class Changer >
class CNetworkVarBase : public CNetworkVarArithmeticBaseImpl<Type, Changer>
{
	typedef CNetworkVarArithmeticBaseImpl<Type, Changer> base;

public:
	using CNetworkVarArithmeticBaseImpl<Type, Changer>::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;
};

template< class Type, class Changer >
class CNetworkColorBaseImpl : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	using CNetworkVarBaseImpl< Type, Changer >::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

	inline void Init( byte rVal, byte gVal, byte bVal )
	{
		this->SetColor( rVal, gVal, bVal );
	}

	inline byte GetR() const { return this->Get().r(); }
	inline byte GetG() const { return this->Get().g(); }
	inline byte GetB() const { return this->Get().b(); }
	inline void SetR( byte val ) { this->SetVal( &Type::r, &Type::SetR, val ); }
	inline void SetG( byte val ) { this->SetVal( &Type::g, &Type::SetG, val ); }
	inline void SetB( byte val ) { this->SetVal( &Type::b, &Type::SetB, val ); }

	inline void SetColor( byte rVal, byte gVal, byte bVal )
	{
	#ifndef CLIENT_DLL
		Type &clr = this->GetWithoutModify();
		if ( clr.r() != rVal || clr.g() != gVal || clr.b() != bVal )
		{
			this->CallOuterNetworkStateChanged();
			clr.SetColor( rVal, gVal, bVal );
		}
	#else
		this->GetForModify().SetColor( rVal, gVal, bVal );
	#endif
	}

protected:
	inline void SetVal( unsigned char (Type::*get_func)() const, void(Type::*set_func)(unsigned char), const byte &in )
	{
	#ifndef CLIENT_DLL
		Type &clr = this->GetWithoutModify();
		if ( (clr.*get_func)() != in )
		{
			this->CallOuterNetworkStateChanged();
			(clr.*set_func)( in );
		}
	#else
		(this->GetForModify().*set_func)( in );
	#endif
	}
};

template< class Changer >
class CNetworkColor24Base : public CNetworkColorBaseImpl< color24, Changer >
{
	typedef CNetworkColorBaseImpl< color24, Changer > base;

public:
	using CNetworkColorBaseImpl< color24, Changer >::CNetworkColorBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;
};

template< class Changer >
class CNetworkColor32Base : public CNetworkColorBaseImpl< color32, Changer >
{
	typedef CNetworkColorBaseImpl< color32, Changer > base;

public:
	using CNetworkColorBaseImpl< color32, Changer >::CNetworkColorBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

	using base::SetColor;
	using base::Init;

	inline void Init( byte rVal, byte gVal, byte bVal, byte aVal )
	{
		this->SetColor( rVal, gVal, bVal, aVal );
	}

	inline byte GetA() const { return this->Get().a(); }
	inline byte a() const { return this->Get().a(); }
	inline void SetA( byte val ) { this->SetVal( &color32::a, &color32::SetA, val ); }

	inline void SetColor( byte rVal, byte gVal, byte bVal, byte aVal )
	{
	#ifndef CLIENT_DLL
		color32 &clr = this->GetWithoutModify();
		if ( clr.r() != rVal || clr.g() != gVal || clr.b() != bVal || clr.a() != aVal )
		{
			this->CallOuterNetworkStateChanged();
			clr.SetColor( rVal, gVal, bVal, aVal );
		}
	#else
		this->GetForModify().SetColor( rVal, gVal, bVal, aVal );
	#endif
	}
};

template< class Changer >
class CNetworkColor32EBase : public CNetworkColorBaseImpl< ColorRGBExp32, Changer >
{
	typedef CNetworkColorBaseImpl< ColorRGBExp32, Changer > base;

public:
	using CNetworkColorBaseImpl< ColorRGBExp32, Changer >::CNetworkColorBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

	using base::SetColor;
	using base::Init;

	inline void Init( byte rVal, byte gVal, byte bVal, byte eVal )
	{
		this->SetColor( rVal, gVal, bVal, eVal );
	}

	inline byte GetE() const { return this->Get().e(); }
	inline byte e() const { return this->Get().e(); }
	inline void SetE( byte val ) { this->SetVal( &ColorRGBExp32::e, &ColorRGBExp32::SetE, val ); }

	inline void SetColor( byte rVal, byte gVal, byte bVal, byte eVal )
	{
	#ifndef CLIENT_DLL
		ColorRGBExp32 &clr = this->GetWithoutModify();
		if ( clr.r() != rVal || clr.g() != gVal || clr.b() != bVal || clr.e() != eVal )
		{
			this->CallOuterNetworkStateChanged();
			clr.SetColor( rVal, gVal, bVal, eVal );
		}
	#else
		this->GetForModify().SetColor( rVal, gVal, bVal, eVal );
	#endif
	}
};

// Network vector wrapper.
template< class Type, class Changer >
class CNetworkVectorBaseImpl : public CNetworkVarArithmeticBaseImpl< Type, Changer >
{
	typedef CNetworkVarArithmeticBaseImpl< Type, Changer > base;

public:
	using CNetworkVarArithmeticBaseImpl< Type, Changer >::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

	inline void Init( float ix=0, float iy=0, float iz=0 ) 
	{
	#ifndef CLIENT_DLL
		Type &vec = this->GetWithoutModify();
		if(vec.x != ix || vec.y != iy || vec.z != iz) {
			vec.Init(ix,iy,iz);
			this->CallOuterNetworkStateChanged();
		}
	#else
		this->GetForModify().Init(ix,iy,iz);
	#endif
	}

	inline float GetX() const { return this->Get().x; }
	inline float GetY() const { return this->Get().y; }
	inline float GetZ() const { return this->Get().z; }
	inline float operator[]( int i ) const { return this->Get()[i]; }

	inline void SetX( float val ) { this->DetectChange( this->GetWithoutModify().x, val ); }
	inline void SetY( float val ) { this->DetectChange( this->GetWithoutModify().y, val ); }
	inline void SetZ( float val ) { this->DetectChange( this->GetWithoutModify().z, val ); }
	inline void Set( int i, float val ) { this->DetectChange( this->GetWithoutModify()[i], val ); }

	float LengthSqr() const
	{ return this->Get().LengthSqr(); }

private:
	inline void DetectChange( float &out, float in ) 
	{
	#ifndef CLIENT_DLL
		if ( out != in ) 
		{
			this->CallOuterNetworkStateChanged();
			out = in;
		}
	#else
		out = in;
	#endif
	}
};

template< class Changer >
class CNetworkVectorWorldspaceBase : public CNetworkVectorBaseImpl< Vector, Changer >
{
	typedef CNetworkVectorBaseImpl< Vector, Changer > base;

public:
	using CNetworkVectorBaseImpl< Vector, Changer >::CNetworkVectorBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;
};

template< class Changer >
class CNetworkVectorBase : public CNetworkVectorBaseImpl< Vector, Changer >
{
	typedef CNetworkVectorBaseImpl< Vector, Changer > base;

public:
	using CNetworkVectorBaseImpl< Vector, Changer >::CNetworkVectorBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;
};

template< class Changer >
class CNetworkQAngleBase : public CNetworkVectorBaseImpl< QAngle, Changer >
{
	typedef CNetworkVectorBaseImpl< QAngle, Changer > base;

public:
	using CNetworkVectorBaseImpl< QAngle, Changer >::CNetworkVectorBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;
};

// Network vector wrapper.
template< class Changer >
class CNetworkQuaternionBase : public CNetworkVarArithmeticBaseImpl< Quaternion, Changer >
{
	typedef CNetworkVarArithmeticBaseImpl< Quaternion, Changer > base;

public:
	using CNetworkVarArithmeticBaseImpl< Quaternion, Changer >::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

	inline void Init( float ix=0, float iy=0, float iz=0, float iw = 0 ) 
	{
	#ifndef CLIENT_DLL
		Quaternion &vec = this->GetWithoutModify();
		if(vec.x != ix || vec.y != iy || vec.z != iz || vec.w != iw) {
			vec.Init(ix,iy,iz,iw);
			this->CallOuterNetworkStateChanged();
		}
	#else
		this->GetForModify().Init(ix,iy,iz,iw);
	#endif
	}

	inline float GetX() const { return this->Get().x; }
	inline float GetY() const { return this->Get().y; }
	inline float GetZ() const { return this->Get().z; }
	inline float GetW() const { return this->Get().w; }
	inline float operator[]( int i ) const { return this->Get()[i]; }

	inline void SetX( float val ) { this->DetectChange( this->GetWithoutModify().x, val ); }
	inline void SetY( float val ) { this->DetectChange( this->GetWithoutModify().y, val ); }
	inline void SetZ( float val ) { this->DetectChange( this->GetWithoutModify().z, val ); }
	inline void SetW( float val ) { this->DetectChange( this->GetWithoutModify().w, val ); }
	inline void Set( int i, float val ) { this->DetectChange( this->GetWithoutModify()[i], val ); }

private:
	inline void DetectChange( float &out, float in ) 
	{
	#ifndef CLIENT_DLL
		if ( out != in ) 
		{
			this->CallOuterNetworkStateChanged();
			out = in;
		}
	#else
		out = in;
	#endif
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
	using CNetworkVarBaseImpl< CHandle<Type>, Changer >::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator!=;
	using base::operator==;
	using base::Set;

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
		return ( this->base::Get().Get() == NULL ); 
	}

	explicit operator bool() const 
	{ 
		return ( this->base::Get().Get() != NULL );
	}
	
	operator Type*() const 
	{ 
		return static_cast< Type* >( this->base::Get().Get() );
	}

	explicit operator const Type*() const 
	{ 
		return static_cast< const Type* >( this->base::Get().Get() );
	}

	explicit operator IHandleEntity*() const 
	{ 
		return static_cast< IHandleEntity* >( this->base::Get().Get() );
	}

	explicit operator const IHandleEntity*() const 
	{ 
		return static_cast< const IHandleEntity* >( this->base::Get().Get() );
	}

	void Set( const Type *val )
	{
	#ifndef CLIENT_DLL
		CHandle<Type> &hndl = this->GetWithoutModify();
		if ( hndl.Get() != val )
		{
			this->CallOuterNetworkStateChanged();
			hndl.Set( val );
		}
	#else
		this->GetForModify().Set( val );
	#endif
	}

	void Set( Type *val )
	{
		this->Set( static_cast<const Type *>(val) );
	}

	void Set( const CHandle<Type> &val )
	{
		this->Set( val.Get() );
	}

	void Set( std::nullptr_t )
	{
		this->Term();
	}

	void Term()
	{
	#ifndef CLIENT_DLL
		CHandle<Type> &hndl = this->GetWithoutModify();
		if ( hndl.Get() != NULL )
		{
			this->CallOuterNetworkStateChanged();
			hndl.Term();
		}
	#else
		this->GetForModify().Term();
	#endif
	}
	
	Type* Get() const 
	{ 
		return static_cast< Type* >( this->base::Get().Get() );
	}

	Type* operator->() const 
	{ 
		return static_cast< Type* >( this->base::Get().Get() );
	}

	bool operator==( const Type *val ) const 
	{
		return this->base::Get().Get() == val; 
	}

	bool operator!=( const Type *val ) const 
	{
		return this->base::Get().Get() != val;
	}

	bool operator==( Type *val ) const 
	{
		return this->base::Get().Get() == val; 
	}

	bool operator!=( Type *val ) const 
	{
		return this->base::Get().Get() != val;
	}

	bool operator==( const IHandleEntity *val ) const 
	{
		return this->base::Get().Get() == val; 
	}

	bool operator!=( const IHandleEntity *val ) const 
	{
		return this->base::Get().Get() != val;
	}

	bool operator==( IHandleEntity *val ) const 
	{
		return this->base::Get().Get() == val; 
	}

	bool operator!=( IHandleEntity *val ) const 
	{
		return this->base::Get().Get() != val;
	}

	bool operator==( std::nullptr_t ) const 
	{
		return this->base::Get().Get() == NULL; 
	}

	bool operator!=( std::nullptr_t ) const 
	{
		return this->base::Get().Get() != NULL;
	}
};

template< class Type, class Changer >
class CNetworkHandleBase : public CNetworkHandleBaseImpl<Type, Changer>
{
	typedef CNetworkHandleBaseImpl<Type, Changer> base;

public:
	using CNetworkHandleBaseImpl<Type, Changer>::CNetworkHandleBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;

	bool operator==( const CGameBaseEntity *val ) const 
	{
		return this->base::Get().Get() == val; 
	}

	bool operator!=( const CGameBaseEntity *val ) const 
	{
		return this->base::Get().Get() != val;
	}

	bool operator==( CGameBaseEntity *val ) const 
	{
		return this->base::Get().Get() == val; 
	}

	bool operator!=( CGameBaseEntity *val ) const 
	{
		return this->base::Get().Get() != val;
	}
};

template< class Changer >
class CNetworkHandleBase<CGameBaseEntity, Changer> : public CNetworkHandleBaseImpl<CGameBaseEntity, Changer>
{
	typedef CNetworkHandleBaseImpl<CGameBaseEntity, Changer> base;

public:
	using CNetworkHandleBaseImpl<CGameBaseEntity, Changer>::CNetworkHandleBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
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
#endif

template< class Changer >
class CNetworkStringTBaseImpl : public CNetworkVarBaseImpl< string_t, Changer >
{
	typedef CNetworkVarBaseImpl< string_t, Changer > base;

public:
	using CNetworkVarBaseImpl< string_t, Changer >::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;

	bool operator==( const char *val ) const
	{ return this->Get().operator==(val); }
	bool operator!=( const char *val ) const
	{ return this->Get().operator!=(val); }
};

template< class Changer >
class CNetworkVarBase<string_t, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkVarBase<modelindex_t, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkModelIndexBaseImpl : public CNetworkVarBaseImpl< modelindex_t, Changer >
{
	typedef CNetworkVarBaseImpl< modelindex_t, Changer > base;

public:
	using CNetworkVarBaseImpl< modelindex_t, Changer >::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template< class Changer >
class CNetworkTimeBase : public CNetworkVarArithmeticBaseImpl< float, Changer >
{
	typedef CNetworkVarArithmeticBaseImpl< float, Changer > base;

public:
	using CNetworkVarArithmeticBaseImpl< float, Changer >::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template< class Changer >
class CNetworkDistanceBase : public CNetworkVarArithmeticBaseImpl< float, Changer >
{
	typedef CNetworkVarArithmeticBaseImpl< float, Changer > base;

public:
	using CNetworkVarArithmeticBaseImpl< float, Changer >::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template< class Changer >
class CNetworkScaleBase : public CNetworkVarArithmeticBaseImpl< float, Changer >
{
	typedef CNetworkVarArithmeticBaseImpl< float, Changer > base;

public:
	using CNetworkVarArithmeticBaseImpl< float, Changer >::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template< class Changer >
class CNetworkSequenceBase : public CNetworkVarBaseImpl< sequence_t, Changer >
{
	typedef CNetworkVarBaseImpl< sequence_t, Changer > base;

public:
	using CNetworkVarBaseImpl< sequence_t, Changer >::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template< class Changer >
class CNetworkAnimCycleBase : public CNetworkVarArithmeticBaseImpl< float, Changer >
{
	typedef CNetworkVarArithmeticBaseImpl< float, Changer > base;

public:
	using CNetworkVarArithmeticBaseImpl< float, Changer >::CNetworkVarArithmeticBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;
};

template< class Type, class Changer >
class CNetworkVarBase<CUtlVector<Type>, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Type, class Changer >
class CNetworkUtlVectorBaseImpl : public CNetworkVarBaseImpl< Type, Changer >
{
	typedef CNetworkVarBaseImpl< Type, Changer > base;

public:
	using CNetworkVarBaseImpl< Type, Changer >::CNetworkVarBaseImpl;
	using base::operator=;
	using base::operator==;
	using base::operator!=;
	using base::Set;

	int Count() const
	{
		return this->Get().Count();
	}

	int Size() const
	{
		return this->Get().Size();
	}

	bool IsValidIndex( int index ) const
	{
		return this->Get().IsValidIndex( index );
	}

	void Purge()
	{
	#ifndef CLIENT_DLL
		Type &vec = this->GetWithoutModify();
		if(vec.Count() > 0) {
			vec.Purge();
			this->CallOuterNetworkStateChanged();
		}
	#else
		this->GetForModify().Purge();
	#endif
	}

	void SetCount( int count )
	{
	#ifndef CLIENT_DLL
		Type &vec = this->GetWithoutModify();
		if(vec.Count() != count) {
			vec.SetCount( count );
			this->CallOuterNetworkStateChanged();
		}
	#else
		this->GetForModify().SetCount( count );
	#endif
	}

	void Set( int index, const typename Type::ElemType_t &value )
	{
	#ifndef CLIENT_DLL
		Type &vec = this->GetWithoutModify();
		if(vec.operator[](index) != value) {
			vec.operator[](index) = value;
			this->CallOuterNetworkStateChanged();
		}
	#else
		this->GetForModify().operator[](index) = value;
	#endif
	}

	int AddMultipleToTail( int num )
	{
		return this->GetForModify().AddMultipleToTail( num );
	}

	void RemoveMultiple( int elem, int num )
	{
		this->GetForModify().RemoveMultiple( elem, num );
	}

	bool FindAndRemove( const typename Type::ElemType_t& src )
	{
	#ifndef CLIENT_DLL
		Type &vec = this->GetWithoutModify();
		if(vec.FindAndRemove( src )) {
			this->CallOuterNetworkStateChanged();
			return true;
		}
		return false;
	#else
		return this->GetForModify().FindAndRemove( src );
	#endif
	}

	int AddToTail( const typename Type::ElemType_t& src )
	{
		return this->GetForModify().AddToTail( src );
	}

	int AddToTail( typename Type::ElemType_t&& src )
	{
		return this->GetForModify().AddToTail( Move(src) );
	}

	using base::GetForModify;

	typename Type::ElemType_t& GetForModify( int index )
	{
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return this->GetWithoutModify().operator[](index);
	}

	using base::Get;

	const typename Type::ElemType_t &Get( int index ) const
	{
		return this->base::Get().operator[](index);
	}

	const typename Type::ElemType_t &operator[]( int index ) const
	{
		return this->base::Get().operator[](index);
	}
};

template< class Changer >
class CNetworkVarBase<Color32, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkVarBase<Color24, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkVarBase<FatColor32, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template< class Changer >
class CNetworkVarBase<ColorRGBExp32, Changer> final
{
private:
	CNetworkVarBase() = delete;
	~CNetworkVarBase() = delete;
};

template <int length, class Changer>
class CNetworkStringBase
{
public:
	CNetworkStringBase() { m_Value[0] = '\0'; }
	operator const char*() const { return m_Value; }
	const char* Get() const { return m_Value; }
	char* GetForModify()
	{
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged();
	#endif
		return m_Value;
	}
protected:
#ifndef CLIENT_DLL
	void CallOuterNetworkStateChanged()
	{
		Changer::CallOuterNetworkStateChanged_impl( this );
	}

	void CallOuterNetworkStateChanged( int i )
	{
		Changer::CallOuterNetworkStateChanged_impl( this, i );
	}
#endif
private:
	char m_Value[length];

public:
	static unsigned short GetOffset_memory()
	{
		static unsigned short offset = MyOffsetOf(CNetworkStringBase, m_Value);
		return offset;
	}

	static unsigned short GetOffset_memory( int i )
	{
		return GetOffset_memory() + (sizeof(char) * i);
	}
};

template <typename type, int count, class Changer>
class CNetworkArrayBase
{
public:
	template <typename T> friend int ServerClassInit(T *);

	inline CNetworkArrayBase()
	{
		for ( int i = 0 ; i < count ; ++i ) {
			NetworkVarConstruct( m_Value[i] );
		}
	}

	const type& operator[]( int i ) const
	{
		return Get( i );
	}

	const type& Get( int i ) const
	{
		Assert( i >= 0 && i < count );
		return m_Value[i];
	}

	type& GetForModify( int i )
	{
		Assert( i >= 0 && i < count );
	#ifndef CLIENT_DLL
		this->CallOuterNetworkStateChanged( i );
	#endif
		return m_Value[i];
	}

	void Set( int i, const type &val )
	{
		Assert( i >= 0 && i < count );
	#ifndef CLIENT_DLL
		if( memcmp( &m_Value[i], &val, sizeof(type) ) )
		{
			this->CallOuterNetworkStateChanged( i );
			m_Value[i] = val;
		}
	#else
		m_Value[i] = val;
	#endif
	}

	const type* Base() const { return m_Value; }
	type* Base() { return m_Value; }
	static int Count() { return count; }

protected:
#ifndef CLIENT_DLL
	void CallOuterNetworkStateChanged()
	{
		Changer::CallOuterNetworkStateChanged_impl( this );
	}

	void CallOuterNetworkStateChanged( int i )
	{
		Changer::CallOuterNetworkStateChanged_impl( this, i );
	}
#endif

private:
	type m_Value[count];

public:
	static unsigned short GetOffset_memory()
	{
		static unsigned short offset = MyOffsetOf(CNetworkArrayBase, m_Value);
		return offset;
	}

	static unsigned short GetOffset_memory( int i )
	{
		return GetOffset_memory() + (sizeof(type) * i);
	}
};

template<typename T>
struct NetworkVarType
{
	using type = T;
};

template<typename C>
struct NetworkVarType<CNetworkColor32Base<C>>
{
	using type = color32;
};

template<typename C>
struct NetworkVarType<CNetworkColor32EBase<C>>
{
	using type = ColorRGBExp32;
};

template<typename C>
struct NetworkVarType<CNetworkColor24Base<C>>
{
	using type = color24;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVectorBaseImpl<T, C>>
{
	using type = T;
};

template<typename C>
struct NetworkVarType<CNetworkVectorBase<C>>
{
	using type = Vector;
};

template<typename C>
struct NetworkVarType<CNetworkVectorWorldspaceBase<C>>
{
	using type = Vector;
};

template<typename C>
struct NetworkVarType<CNetworkQAngleBase<C>>
{
	using type = QAngle;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVarArithmeticBaseImpl<T, C>>
{
	using type = T;
};

template<typename C>
struct NetworkVarType<CNetworkQuaternionBase<C>>
{
	using type = Quaternion;
};

template<typename C>
struct NetworkVarType<CNetworkStringTBaseImpl<C>>
{
	using type = string_t;
};

template<typename C>
struct NetworkVarType<CNetworkModelIndexBaseImpl<C>>
{
	using type = modelindex_t;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkUtlVectorBaseImpl<T, C>>
{
	using type = T;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVarBase<T, C>>
{
	using type = T;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVarBaseImpl<T, C>>
{
	using type = T;
};

template<typename C>
struct NetworkVarType<CNetworkTimeBase<C>>
{
	using type = float;
};

template<typename C>
struct NetworkVarType<CNetworkScaleBase<C>>
{
	using type = float;
};

template<typename C>
struct NetworkVarType<CNetworkDistanceBase<C>>
{
	using type = float;
};

template<typename C>
struct NetworkVarType<CNetworkSequenceBase<C>>
{
	using type = sequence_t;
};

template<typename C>
struct NetworkVarType<CNetworkAnimCycleBase<C>>
{
	using type = float;
};

#if defined( CLIENT_DLL ) || defined( GAME_DLL )
template<typename T, typename C>
struct NetworkVarType<CNetworkHandleBaseImpl<T, C>>
{
	using type = CHandle<T>;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkHandleBase<T, C>>
{
	using type = CHandle<T>;
};
#endif

template<int length, typename C>
struct NetworkVarType<CNetworkStringBase<length, C>>
{
	using type = char[length];
};

template<typename T, int length, typename C>
struct NetworkVarType<CNetworkArrayBase<T, length, C>>
{
	using type = T[length];
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVarEmbeddedBase<T, C>>
{
	using type = T;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVarEmbeddedCopyableBase<T, C>>
{
	using type = T;
};

template<typename T, typename C>
struct NetworkVarType<CNetworkVarEmbeddedPtrBase<T, C>>
{
	using type = T *;
};

// Use this macro when you have a base class with a variable, and it doesn't have that variable in a SendTable,
// but a derived class does. Then, the entity is only flagged as changed when the variable is changed in
// an entity that wants to transmit the variable.
#ifndef CLIENT_DLL
#define NETWORK_VAR_DERIVED_OFF( name, accessor ) \
	accessor \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( unsigned short offset ) {}

#define NETWORK_VAR_DERIVED_ON( name, accessor ) \
	accessor \
	virtual void NetworkStateChanged_##name() { CHECK_USENETWORKVARS NetworkStateChanged( NetworkVar_##name##_OuterClass::GetOffset_##name##_memory() ); } \
	virtual void NetworkStateChanged_##name( unsigned short offset ) { CHECK_USENETWORKVARS NetworkStateChanged( offset ); }

#define IMPLEMENT_NETWORK_VAR_FOR_DERIVED( name ) \
	void NetworkStateChanged_##name() override { CHECK_USENETWORKVARS NetworkStateChanged( NetworkVar_##name##_OuterClass::GetOffset_##name##_memory() ); } \
	void NetworkStateChanged_##name( unsigned short offset ) override { CHECK_USENETWORKVARS NetworkStateChanged( offset ); }

#define DISABLE_NETWORK_VAR_FOR_DERIVED( name ) \
	void NetworkStateChanged_##name() override { } \
	void NetworkStateChanged_##name( unsigned short offset ) override {  }
#else
#define IMPLEMENT_NETWORK_VAR_FOR_DERIVED( name )

#define DISABLE_NETWORK_VAR_FOR_DERIVED( name )

#define NETWORK_VAR_DERIVED_OFF( name ) 

#define NETWORK_VAR_DERIVED_ON( name ) 
#endif

// Internal macros used in definitions of network vars.
#ifndef CLIENT_DLL
#define NETWORK_VAR_STATECHANGED( name, stateChangedFn ) \
	static void CallOuterNetworkStateChanged_impl( void *ptr ) \
	{ \
		CHECK_USENETWORKVARS ( ( NetworkVar_##name##_OuterClass * ) ( ((uint8 *)ptr) - NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() ) )->stateChangedFn( NetworkVar_##name##_OuterClass::GetOffset_##name##_memory() ); \
	}

#define NETWORK_VAR_STATECHANGED_ARRAY( name, stateChangedFn ) \
	static void CallOuterNetworkStateChanged_impl( void *ptr, int i ) \
	{ \
		CHECK_USENETWORKVARS ( ( NetworkVar_##name##_OuterClass * ) ( ((uint8 *)ptr) - NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() ) )->stateChangedFn( NetworkVar_##name##_OuterClass::GetOffset_##name##_memory( i ) ); \
	}

#define NETWORK_VAR_STATECHANGED_EMBEDDED( name, stateChangedFn ) \
	static void CallOuterNetworkStateChanged_impl( void *ptr ) \
	{ \
		CHECK_USENETWORKVARS ( ( NetworkVar_##name##_OuterClass * ) ( ((uint8 *)ptr) - NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() ) )->stateChangedFn( NetworkVar_##name##_OuterClass::GetOffset_##name##_memory() ); \
	} \
	static void CallOuterNetworkStateChanged_impl( void *ptr, unsigned short offset ) \
	{ \
		CHECK_USENETWORKVARS ( ( NetworkVar_##name##_OuterClass * ) ( ((uint8 *)ptr) - NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() ) )->stateChangedFn( NetworkVar_##name##_OuterClass::GetOffset_##name##_memory( offset ) ); \
	}
#else
#define NETWORK_VAR_STATECHANGED( name, stateChangedFn )
#define NETWORK_VAR_STATECHANGED_ARRAY( name, stateChangedFn )
#define NETWORK_VAR_STATECHANGED_EMBEDDED( name, stateChangedFn )
#endif

#define NETWORK_VAR_BEGIN_PRE_OFFSETS( name, accessor, base, ... ) \
	accessor \
	class NetworkVar_##name; \
	friend class NetworkVar_##name; \
	typedef ThisClass NetworkVar_##name##_OuterClass; \
	typedef base< __VA_OPT__(__VA_ARGS__,) NetworkVar_##name##_OuterClass::NetworkVar_##name > NetworkVar_##name##_BaseClass; \
	static unsigned short GetOffset_##name##_NetworkVar() \
	{ \
		static unsigned short offset = MyOffsetOf(NetworkVar_##name##_OuterClass, name); \
		return offset; \
	}

#define NETWORK_VAR_BEGIN_POST_OFFSETS( name, accessor, base, ... ) \
	accessor \
	class NetworkVar_##name : public NetworkVar_##name##_OuterClass::NetworkVar_##name##_BaseClass \
	{ \
	public: \
		template <typename T> friend int ServerClassInit(T *); \
		using base< __VA_OPT__(__VA_ARGS__,) NetworkVar_##name##_OuterClass::NetworkVar_##name >::base; \
		using NetworkVar_##name##_BaseClass::operator=; \
		NetworkVar_##name() = default;

#define NETWORK_VAR_END_EMBEDDED( name, accessor ) \
	}; \
	accessor \
	NetworkVar_##name name;
#define NETWORK_VAR_END( name, accessor ) \
	private: \
		typename NetworkVarType< NetworkVar_##name##_OuterClass::NetworkVar_##name##_BaseClass >::type m_Value; \
	public: \
		static unsigned short GetOffset_memory() \
		{ \
			static unsigned short offset = MyOffsetOf(NetworkVar_##name##_OuterClass::NetworkVar_##name, m_Value); \
			return offset; \
		} \
		static typename NetworkVarType< NetworkVar_##name##_OuterClass::NetworkVar_##name##_BaseClass >::type &GetValue_location_impl( void *ptr ) \
		{ \
			return static_cast<NetworkVar_##name##_OuterClass::NetworkVar_##name *>(ptr)->m_Value; \
		} \
		static const typename NetworkVarType< NetworkVar_##name##_OuterClass::NetworkVar_##name##_BaseClass >::type &GetValue_location_impl( const void *ptr ) \
		{ \
			return static_cast<const NetworkVar_##name##_OuterClass::NetworkVar_##name *>(ptr)->m_Value; \
		} \
	}; \
	accessor \
	NetworkVar_##name name;
#define NETWORK_VAR_END_LOCATED( name, accessor, otherClass, otherName ) \
		static typename NetworkVarType< NetworkVar_##name##_BaseClass >::type &GetValue_location_impl( void *ptr ) \
		{ \
			return (( ( NetworkVar_##name##_OuterClass * ) ( ((uint8 *)ptr) - NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() ) )->*( &otherClass::otherName )); \
		} \
		static const typename NetworkVarType< NetworkVar_##name##_BaseClass >::type &GetValue_location_impl( const void *ptr ) \
		{ \
			return (( ( const NetworkVar_##name##_OuterClass * ) ( ((uint8 *)ptr) - NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() ) )->*( &otherClass::otherName )); \
		} \
	}; \
	accessor \
	NetworkVar_##name name;

#define NETWORK_VAR_EMBEDDED( name, accessor1, accessor2, base, stateChangedFn, ... ) \
	NETWORK_VAR_BEGIN_PRE_OFFSETS( name, accessor2, base __VA_OPT__(, __VA_ARGS__) ) \
	static unsigned short GetOffset_##name##_memory() \
	{ \
		return NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar(); \
	} \
	static unsigned short GetOffset_##name##_memory( unsigned short offset ) \
	{ \
		return NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() + offset; \
	} \
	NETWORK_VAR_BEGIN_POST_OFFSETS( name, accessor2, base __VA_OPT__(, __VA_ARGS__) ) \
	NETWORK_VAR_STATECHANGED_EMBEDDED( name, stateChangedFn ) \
	NETWORK_VAR_END_EMBEDDED( name, accessor1 )

#define NETWORK_VAR( name, accessor1, accessor2, base, stateChangedFn, ... ) \
	NETWORK_VAR_BEGIN_PRE_OFFSETS( name, accessor2, base __VA_OPT__(, __VA_ARGS__) ) \
	static unsigned short GetOffset_##name##_memory() \
	{ \
		static unsigned short offset = NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() + NetworkVar_##name##_OuterClass::NetworkVar_##name::GetOffset_memory(); \
		return offset; \
	} \
	NETWORK_VAR_BEGIN_POST_OFFSETS( name, accessor2, base __VA_OPT__(, __VA_ARGS__) ) \
	NETWORK_VAR_STATECHANGED( name, stateChangedFn ) \
	NETWORK_VAR_END( name, accessor1 )

#define NETWORK_VAR_LOCATED( name, accessor1, accessor2, base, stateChangedFn, type, otherClass, otherName ) \
	NETWORK_VAR_BEGIN_PRE_OFFSETS( name, accessor2, base, type ) \
	static unsigned short GetOffset_##name##_memory() \
	{ \
		static unsigned short offset = MyOffsetOf(otherClass, otherName); \
		return offset; \
	} \
	NETWORK_VAR_BEGIN_POST_OFFSETS( name, accessor2, base, type ) \
	NETWORK_VAR_STATECHANGED( name, stateChangedFn ) \
	NETWORK_VAR_END_LOCATED( name, accessor1, otherClass, otherName )

#define NETWORK_VAR_ARRAY( name, accessor1, accessor2, base, stateChangedFn, ... ) \
	NETWORK_VAR_BEGIN_PRE_OFFSETS( name, accessor2, base __VA_OPT__(, __VA_ARGS__) ) \
	static unsigned short GetOffset_##name##_memory() \
	{ \
		static unsigned short offset = NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() + NetworkVar_##name##_OuterClass::NetworkVar_##name##_BaseClass::GetOffset_memory(); \
		return offset; \
	} \
	static unsigned short GetOffset_##name##_memory( int i ) \
	{ \
		return NetworkVar_##name##_OuterClass::GetOffset_##name##_NetworkVar() + NetworkVar_##name##_OuterClass::NetworkVar_##name##_BaseClass::GetOffset_memory( i ); \
	} \
	NETWORK_VAR_BEGIN_POST_OFFSETS( name, accessor2, base __VA_OPT__(, __VA_ARGS__) ) \
	NETWORK_VAR_STATECHANGED( name, stateChangedFn ) \
	NETWORK_VAR_STATECHANGED_ARRAY( name, stateChangedFn ) \
	NETWORK_VAR_END( name, accessor1 )

#define NO_ACCESSOR

#define NETWORK_VAR_GET_ACCESSOR_VAR__VA(...) __VA_ARGS__:
#define NETWORK_VAR_GET_ACCESSOR_VAR_(...) 
#define NETWORK_VAR_GET_ACCESSOR_VAR(...) NETWORK_VAR_GET_ACCESSOR_VAR_##__VA_OPT__(_VA)(__VA_ARGS__)

#define NETWORK_VAR_GET_ACCESSOR_INFO__VA(...) protected:
#define NETWORK_VAR_GET_ACCESSOR_INFO_(...) 
#define NETWORK_VAR_GET_ACCESSOR_INFO(...) NETWORK_VAR_GET_ACCESSOR_INFO_##__VA_OPT__(_VA)(__VA_ARGS__)

#define NETWORK_VAR_GET_ACCESSOR_FUNC__VA(...) private:
#define NETWORK_VAR_GET_ACCESSOR_FUNC_(...) 
#define NETWORK_VAR_GET_ACCESSOR_FUNC(...) NETWORK_VAR_GET_ACCESSOR_FUNC_##__VA_OPT__(_VA)(__VA_ARGS__)

// NOTE: Assignment operator is disabled because it doesn't call copy constructors of scalar types within the aggregate, so they are not marked changed
#define CNetworkVarEmbedded( type, name ) \
	NETWORK_VAR_EMBEDDED( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVarEmbeddedBase, NetworkStateChanged, type )
#define CNetworkVarEmbeddedForDerived( type, name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_EMBEDDED( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarEmbeddedBase, NetworkStateChanged_##name, type )
#define CNetworkVarEmbeddedForDerived_OnByDefault( type, name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_EMBEDDED( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarEmbeddedBase, NetworkStateChanged_##name, type )

#define CNetworkVarEmbeddedCopyable( type, name ) \
	NETWORK_VAR_EMBEDDED( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVarEmbeddedCopyableBase, NetworkStateChanged, type )
#define CNetworkVarEmbeddedCopyableForDerived( type, name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_EMBEDDED( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarEmbeddedCopyableBase, NetworkStateChanged_##name, type )
#define CNetworkVarEmbeddedCopyableForDerived_OnByDefault( type, name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_EMBEDDED( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarEmbeddedCopyableBase, NetworkStateChanged_##name, type )

#define CNetworkVarEmbeddedPtr( type, name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVarEmbeddedPtrBase, NetworkStateChanged, type )
#define CNetworkVarEmbeddedPtrForDerived( type, name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarEmbeddedPtrBase, NetworkStateChanged_##name, type )
#define CNetworkVarEmbeddedPtrForDerived_OnByDefault( type, name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarEmbeddedPtrBase, NetworkStateChanged_##name, type )

#define CNetworkHandle( type, name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkHandleBase, NetworkStateChanged, type )
#define CNetworkHandleForDerived( type, name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkHandleBase, NetworkStateChanged_##name, type )
#define CNetworkHandleForDerived_OnByDefault( type, name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkHandleBase, NetworkStateChanged_##name, type )

#define CNetworkStringT( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkStringTBaseImpl, NetworkStateChanged )
#define CNetworkStringTForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, CNetworkStringTBaseImpl, NetworkStateChanged_##name )
#define CNetworkStringTForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkStringTBaseImpl, NetworkStateChanged_##name )

#define CNetworkModelIndex( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkModelIndexBaseImpl, NetworkStateChanged )
#define CNetworkModelIndexForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkModelIndexBaseImpl, NetworkStateChanged_##name )
#define CNetworkModelIndexForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkModelIndexBaseImpl, NetworkStateChanged_##name )

// Use this macro to define a network variable.
#define CNetworkVar( type, name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVarBase, NetworkStateChanged, type )
#define CNetworkVarLocated( type, name, otherClass, otherName ) \
	NETWORK_VAR_LOCATED( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVarBase, NetworkStateChanged, type, otherClass, otherName )
#define CNetworkVarForDerived( type, name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarBase, NetworkStateChanged_##name, type )
// This virtualizes the change detection on the variable, but it is ON by default.
// Use this when you have a base class in which MOST of its derived classes use this variable
// in their SendTables, but there are a couple that don't (and they
// can use DISABLE_NETWORK_VAR_FOR_DERIVED).
#define CNetworkVarForDerived_OnByDefault( type, name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVarBase, NetworkStateChanged_##name, type )

#define CNetworkTime( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkTimeBase, NetworkStateChanged )
#define CNetworkTimeForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkTimeBase, NetworkStateChanged_##name )
#define CNetworkTimeForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkTimeBase, NetworkStateChanged_##name )

#define CNetworkDistance( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkDistanceBase, NetworkStateChanged )
#define CNetworkDistanceForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkDistanceBase, NetworkStateChanged_##name )
#define CNetworkDistanceForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkDistanceBase, NetworkStateChanged_##name )

#define CNetworkScale( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkScaleBase, NetworkStateChanged )
#define CNetworkScaleForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkScaleBase, NetworkStateChanged_##name )
#define CNetworkScaleForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkScaleBase, NetworkStateChanged_##name )

#define CNetworkSequence( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkSequenceBase, NetworkStateChanged )
#define CNetworkSequenceForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkSequenceBase, NetworkStateChanged_##name )
#define CNetworkSequenceForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkSequenceBase, NetworkStateChanged_##name )

#define CNetworkUtlVector( type, name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkUtlVectorBaseImpl, NetworkStateChanged, CUtlVector<type> )
#define CNetworkUtlVectorForDerived( type, name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkUtlVectorBaseImpl, NetworkStateChanged_##name, CUtlVector<type> )
#define CNetworkUtlVectorForDerived_OnByDefault( type, name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkUtlVectorBaseImpl, NetworkStateChanged_##name, CUtlVector<type> )

// Vectors + some convenient helper functions.
#define CNetworkVector( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVectorBase, NetworkStateChanged )
#define CNetworkVectorForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVectorBase, NetworkStateChanged_##name )
#define CNetworkVectorForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVectorBase, NetworkStateChanged_##name )

#define CNetworkVectorWorldspace( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkVectorWorldspaceBase, NetworkStateChanged )
#define CNetworkVectorWorldspaceForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVectorWorldspaceBase, NetworkStateChanged_##name )
#define CNetworkVectorWorldspaceForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkVectorWorldspaceBase, NetworkStateChanged_##name )

#define CNetworkQAngle( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkQAngleBase, NetworkStateChanged )
#define CNetworkQAngleForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkQAngleBase, NetworkStateChanged_##name )
#define CNetworkQAngleForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkQAngleBase, NetworkStateChanged_##name )

#define CNetworkQuaternion( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkQuaternionBase, NetworkStateChanged )
#define CNetworkQuaternionForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkQuaternionBase, NetworkStateChanged_##name )
#define CNetworkQuaternionForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkQuaternionBase, NetworkStateChanged_##name )

// Helper for color32's. Contains GetR(), SetR(), etc.. functions.
#define CNetworkColor32( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkColor32Base, NetworkStateChanged )
#define CNetworkColor32ForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkColor32Base, NetworkStateChanged_##name )
#define CNetworkColor32ForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( nam, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__)e ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkColor32Base, NetworkStateChanged_##name )

#define CNetworkColor32E( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkColor32EBase, NetworkStateChanged )
#define CNetworkColor32EForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkColor32EBase, NetworkStateChanged_##name )
#define CNetworkColor32EForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkColor32EBase, NetworkStateChanged_##name )

#define CNetworkColor24( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkColor24Base, NetworkStateChanged )
#define CNetworkColor24ForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkColor24Base, NetworkStateChanged_##name )
#define CNetworkColor24ForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkColor24Base, NetworkStateChanged_##name )

#define CNetworkString( name, length ) \
	NETWORK_VAR_ARRAY( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkStringBase, NetworkStateChanged, length )
#define CNetworkStringForDerived( name, length, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_ARRAY( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkStringBase, NetworkStateChanged_##name, length )
#define CNetworkStringForDerived_OnByDefault( name, length, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_ARRAY( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkStringBase, NetworkStateChanged_##name, length )

// Use this to define networked arrays.
// You can access elements for reading with operator[], and you can set elements with the Set() function.
#define CNetworkArray( type, name, count ) \
	NETWORK_VAR_ARRAY( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkArrayBase, NetworkStateChanged, type, count )
#define CNetworkArrayForDerived( type, name, count, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_ARRAY( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkArrayBase, NetworkStateChanged_##name, type, count )
#define CNetworkArrayForDerived_OnByDefault( type, name, count, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR_ARRAY( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkArrayBase, NetworkStateChanged_##name, type, count )

#define CNetworkPredictableId( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkPredictableIdBase, NetworkStateChanged )
#define CNetworkPredictableIdForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkPredictableIdBase, NetworkStateChanged_##name )
#define CNetworkPredictableIdForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkPredictableIdBase, NetworkStateChanged_##name )

#define CNetworkAnimCycle( name ) \
	NETWORK_VAR( name, NO_ACCESSOR, NO_ACCESSOR, CNetworkAnimCycleBase, NetworkStateChanged )
#define CNetworkAnimCycleForDerived( name, ... ) \
	NETWORK_VAR_DERIVED_OFF( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkAnimCycleBase, NetworkStateChanged_##name )
#define CNetworkAnimCycleForDerived_OnByDefault( name, ... ) \
	NETWORK_VAR_DERIVED_ON( name, NETWORK_VAR_GET_ACCESSOR_FUNC(__VA_ARGS__) ) \
	NETWORK_VAR( name, NETWORK_VAR_GET_ACCESSOR_VAR(__VA_ARGS__), NETWORK_VAR_GET_ACCESSOR_INFO(__VA_ARGS__), CNetworkAnimCycleBase, NetworkStateChanged_##name )

#endif // NETWORKVAR_H
