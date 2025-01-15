//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTIONCOPY_H
#define PREDICTIONCOPY_H
#pragma once

#include <memory.h>
#include "datamap.h"
#include "ehandle.h"
#include "tier1/utlstring.h"

#if defined( CLIENT_DLL )
class C_BaseEntity;
typedef CHandle<C_BaseEntity> EHANDLE;

#if defined( _DEBUG )
// #define COPY_CHECK_STRESSTEST
class IGameSystem;
IGameSystem* GetPredictionCopyTester( void );
#endif

#else
class CBaseEntity;
typedef CHandle<CBaseEntity> EHANDLE;
#endif

enum PredCopy_t : unsigned char
{
	PC_EVERYTHING = 0,
	PC_NON_NETWORKED_ONLY,
	PC_NETWORKED_ONLY,
};

#define PC_DATA_PACKED			TD_OFFSET_PACKED
#define PC_DATA_NORMAL			TD_OFFSET_NORMAL

typedef void ( *FN_FIELD_COMPARE )( const char *classname, const char *fieldname, const char *fieldtype,
	bool networked, bool noterrorchecked, bool differs, bool withintolerance, const char *value );

class CPredictionCopy
{
public:
	enum difftype_t
	{
		DIFFERS = 0,
		IDENTICAL,
		WITHINTOLERANCE,
	};

	CPredictionCopy( PredCopy_t type, void *dest, bool dest_packed, void const *src, bool src_packed,
		bool counterrors = false, bool reporterrors = false, bool performcopy = true, 
		bool describefields = false, FN_FIELD_COMPARE func = NULL );

	void	CopyChar( difftype_t dt, char *outvalue, const char *invalue, int count );
	void	CopyUChar( difftype_t dt, unsigned char *outvalue, const unsigned char *invalue, int count );
	void	CopySChar( difftype_t dt, signed char *outvalue, const signed char *invalue, int count );

	void	CopyShort( difftype_t dt, short *outvalue, const short *invalue, int count );
	void	CopyUShort( difftype_t dt, unsigned short *outvalue, const unsigned short *invalue, int count );

	void	CopyInt( difftype_t dt, int *outvalue, const int *invalue, int count );		// Copy an int
	void	CopyUInt( difftype_t dt, unsigned int *outvalue, const unsigned int *invalue, int count );		// Copy an int

	void	CopyInt64( difftype_t dt, int64 *outvalue, const int64 *invalue, int count );		// Copy an int
	void	CopyUInt64( difftype_t dt, uint64 *outvalue, const uint64 *invalue, int count );		// Copy an int

	void	CopyModelindex( difftype_t dt, modelindex_t *outvalue, const modelindex_t *invalue, int count );		// Copy an int

	void	CopyBool( difftype_t dt, bool *outvalue, const bool *invalue, int count );		// Copy a bool

	void	CopyFloat( difftype_t dt, float *outvalue, const float *invalue, int count );	// Copy a float

	void	CopyCString( difftype_t dt, char *outstring, const char *instring );			// Copy a null-terminated string

	void	CopyVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count );	// Copy a vector array

	void	CopyQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count );				// Copy a quaternion array

	void	CopyEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count );

	void	FORCEINLINE CopyData( difftype_t dt, int size, char *outdata, const char *indata )		// Copy a binary data block
	{
		if ( !m_bPerformCopy )
			return;

		if ( dt == IDENTICAL )
			return;

		memcpy( outdata, indata, size );
	}

	int		TransferData( const char *operation, int entindex, datamap_t *dmap );

private:
	void	TransferData_R( int chaincount, datamap_t *dmap );

	void	DetermineWatchField( const char *operation, int entindex,  datamap_t *dmap );
	void	DumpWatchField( typedescription_t *field );
	void	WatchMsg( PRINTF_FORMAT_STRING const char *fmt, ... );

	difftype_t	CompareChar( char *outvalue, const char *invalue, int count );
	difftype_t	CompareUChar( unsigned char *outvalue, const unsigned char *invalue, int count );
	difftype_t	CompareSChar( signed char *outvalue, const signed char *invalue, int count );

	difftype_t	CompareShort( short *outvalue, const short *invalue, int count );
	difftype_t	CompareUShort( unsigned short *outvalue, const unsigned short *invalue, int count );

	difftype_t	CompareInt( int *outvalue, const int *invalue, int count );		// Compare an int
	difftype_t	CompareUInt( unsigned int *outvalue, const unsigned int *invalue, int count );		// Compare an int

	difftype_t	CompareInt64( int64 *outvalue, const int64 *invalue, int count );		// Compare an int
	difftype_t	CompareUInt64( uint64 *outvalue, const uint64 *invalue, int count );		// Compare an int

	difftype_t	CompareModelindex( modelindex_t *outvalue, const modelindex_t *invalue, int count );		// Compare an int

	difftype_t	CompareBool( bool *outvalue, const bool *invalue, int count );		// Compare a bool

	difftype_t	CompareFloat( float *outvalue, const float *invalue, int count );	// Compare a float

	difftype_t	CompareData( int size, char *outdata, const char *indata );		// Compare a binary data block

	difftype_t	CompareCString( char *outstring, const char *instring );			// Compare a null-terminated string

	difftype_t	CompareVector( Vector* outValue, const Vector *inValue, int count );	// Compare a vector array

	difftype_t	CompareQuaternion( Quaternion* outValue, const Quaternion *inValue, int count );	// Compare a Quaternion array

	difftype_t	CompareEHandle( EHANDLE *outvalue, EHANDLE const *invalue, int count );

	void	DescribeChar( difftype_t dt, char *outvalue, const char *invalue, int count );
	void	DescribeUChar( difftype_t dt, unsigned char *outvalue, const unsigned char *invalue, int count );
	void	DescribeSChar( difftype_t dt, signed char *outvalue, const signed char *invalue, int count );

	void	DescribeShort( difftype_t dt, short *outvalue, const short *invalue, int count );
	void	DescribeUShort( difftype_t dt, unsigned short *outvalue, const unsigned short *invalue, int count );

	void	DescribeInt( difftype_t dt, int *outvalue, const int *invalue, int count );		// Compare an int
	void	DescribeUInt( difftype_t dt, unsigned int *outvalue, const unsigned int *invalue, int count );		// Compare an int

	void	DescribeModelindex( difftype_t dt, modelindex_t *outvalue, const modelindex_t *invalue, int count );		// Compare an int

	void	DescribeInt64( difftype_t dt, int64 *outvalue, const int64 *invalue, int count );		// Compare an int
	void	DescribeUInt64( difftype_t dt, uint64 *outvalue, const uint64 *invalue, int count );		// Compare an int

	void	DescribeBool( difftype_t dt, bool *outvalue, const bool *invalue, int count );		// Compare a bool

	void	DescribeFloat( difftype_t dt, float *outvalue, const float *invalue, int count );	// Compare a float

	void	DescribeData( difftype_t dt, int size, char *outdata, const char *indata );		// Compare a binary data block

	void	DescribeCString( difftype_t dt, char *outstring, const char *instring );			// Compare a null-terminated string

	void	DescribeVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count );	// Compare a vector array

	void	DescribeQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count );	// Compare a Quaternion array

	void	DescribeEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count );

	void	WatchChar( difftype_t dt, char *outvalue, const char *invalue, int count );
	void	WatchUChar( difftype_t dt, unsigned char *outvalue, const unsigned char *invalue, int count );
	void	WatchSChar( difftype_t dt, signed char *outvalue, const signed char *invalue, int count );

	void	WatchShort( difftype_t dt, short *outvalue, const short *invalue, int count );
	void	WatchUShort( difftype_t dt, unsigned short *outvalue, const unsigned short *invalue, int count );

	void	WatchInt( difftype_t dt, int *outvalue, const int *invalue, int count );		// Compare an int
	void	WatchUInt( difftype_t dt, unsigned int *outvalue, const unsigned int *invalue, int count );		// Compare an int

	void	WatchModelindex( difftype_t dt, modelindex_t *outvalue, const modelindex_t *invalue, int count );		// Compare an int

	void	WatchInt64( difftype_t dt, int64 *outvalue, const int64 *invalue, int count );		// Compare an int
	void	WatchUInt64( difftype_t dt, uint64 *outvalue, const uint64 *invalue, int count );		// Compare an int

	void	WatchBool( difftype_t dt, bool *outvalue, const bool *invalue, int count );		// Compare a bool

	void	WatchFloat( difftype_t dt, float *outvalue, const float *invalue, int count );	// Compare a float

	void	WatchData( difftype_t dt, int size, char *outdata, const char *indata );		// Compare a binary data block

	void	WatchCString( difftype_t dt, char *outstring, const char *instring );			// Compare a null-terminated string

	void	WatchVector( difftype_t dt, Vector* outValue, const Vector *inValue, int count );	// Compare a vector array

	void	WatchQuaternion( difftype_t dt, Quaternion* outValue, const Quaternion *inValue, int count );	// Compare a Quaternion array

	void	WatchEHandle( difftype_t dt, EHANDLE *outvalue, EHANDLE const *invalue, int count );

	// Report function
	void	ReportFieldsDiffer( PRINTF_FORMAT_STRING const char *fmt, ... );
	void	DescribeFields( difftype_t dt, PRINTF_FORMAT_STRING const char *fmt, ... );
	
	bool	CanCheck( void );

	void	CopyFields( int chaincount, datamap_t *pMap, typedescription_t *pFields, int fieldCount );

private:

	PredCopy_t				m_nType;
	void			*m_pDest;
	void const		*m_pSrc;
	TdOffsetType_t				m_nDestOffsetIndex;
	TdOffsetType_t				m_nSrcOffsetIndex;


	bool			m_bErrorCheck;
	bool			m_bReportErrors;
	bool			m_bDescribeFields;
	typedescription_t *m_pCurrentField;
	char const		*m_pCurrentClassName;
	datamap_t		*m_pCurrentMap;
	bool			m_bShouldReport;
	bool			m_bShouldDescribe;
	int				m_nErrorCount;
	bool			m_bPerformCopy;

	FN_FIELD_COMPARE	m_FieldCompareFunc;

	typedescription_t	 *m_pWatchField;
	char const			*m_pOperation;
};

typedef void (*FN_FIELD_DESCRIPTION)( const char *classname, const char *fieldname, const char *fieldtype,
	bool networked, const char *value );

//-----------------------------------------------------------------------------
// Purpose: Simply dumps all data fields in object
//-----------------------------------------------------------------------------
class CPredictionDescribeData
{
public:
	CPredictionDescribeData( void const *src, bool src_packed, FN_FIELD_DESCRIPTION func = 0 );

	void	DescribeChar( const char *invalue, int count );
	void	DescribeUChar( const unsigned char *invalue, int count );
	void	DescribeSChar( const signed char *invalue, int count );

	void	DescribeShort( const short *invalue, int count );
	void	DescribeUShort( const unsigned short *invalue, int count );

	void	DescribeInt( const int *invalue, int count );		
	void	DescribeUInt( const unsigned int *invalue, int count );		

	void	DescribeInt64( const int64 *invalue, int count );		
	void	DescribeUInt64( const uint64 *invalue, int count );		

	void	DescribeModelindex( const modelindex_t *invalue, int count );

	void	DescribeBool( const bool *invalue, int count );	

	void	DescribeFloat( const float *invalue, int count );	

	void	DescribeData( int size, const char *indata );		

	void	DescribeCString( const char *instring );			

	void	DescribeVector( const Vector *inValue, int count );

	void	DescribeQuaternion( const Quaternion *inValue, int count );

	void	DescribeEHandle( EHANDLE const *invalue, int count );

	void	DumpDescription( datamap_t *pMap );

private:
	void	DescribeFields_R( int chain_count, datamap_t *pMap, typedescription_t *pFields, int fieldCount );

	void const		*m_pSrc;
	TdOffsetType_t				m_nSrcOffsetIndex;

	void			Describe( PRINTF_FORMAT_STRING const char *fmt, ... );

	typedescription_t *m_pCurrentField;
	char const		*m_pCurrentClassName;
	datamap_t		*m_pCurrentMap;

	bool			m_bShouldReport;

	FN_FIELD_DESCRIPTION	m_FieldDescFunc;
};

#if defined( CLIENT_DLL )
class CValueChangeTracker
{
public:
	CValueChangeTracker();

	void Reset();

	void StartTrack( char const *pchContext );
	void EndTrack();

	bool IsActive() const;

	void SetupTracking( C_BaseEntity *ent, char const *pchFieldName );
	void ClearTracking();

	void Spew();

	C_BaseEntity *GetEntity();

private:

	enum
	{
		eChangeTrackerBufSize = 128,
	};

	// Returns field size
	void				GetValue( char *buf, size_t bufsize );

	bool				m_bActive : 1;
	bool				m_bTracking : 1;
	EHANDLE				m_hEntityToTrack;
	CUtlVector< typedescription_t * > m_FieldStack;
	CUtlString			m_strFieldName;
	CUtlString			m_strContext;
	// First 128 bytes of data is all we will consider
	char				m_OrigValueBuf[ eChangeTrackerBufSize ];
	CUtlVector< CUtlString >	 m_History;
};

extern CValueChangeTracker *g_pChangeTracker;

class CValueChangeTrackerScope
{
public:
	CValueChangeTrackerScope( char const *pchContext )
	{
		m_bCallEndTrack = true;
		g_pChangeTracker->StartTrack( pchContext );
	}

	// Only calls Start/End if passed in entity matches entity to track
	CValueChangeTrackerScope( C_BaseEntity *pEntity, char const *pchContext )
	{
		m_bCallEndTrack = g_pChangeTracker->GetEntity() == pEntity;
		if ( m_bCallEndTrack )
		{
			g_pChangeTracker->StartTrack( pchContext );
		}
	}

	~CValueChangeTrackerScope()
	{
		if ( m_bCallEndTrack )
		{
			g_pChangeTracker->EndTrack();
		}
	}
private:

	bool		m_bCallEndTrack;
};

#if defined( _DEBUG )
#define PREDICTION_TRACKVALUECHANGESCOPE( context )		CValueChangeTrackerScope scope( context );
#define PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( entity, context ) CValueChangeTrackerScope scope( entity, context );
#define PREDICTION_STARTTRACKVALUE( context )			g_pChangeTracker->StartTrack( context );
#define PREDICTION_ENDTRACKVALUE()						g_pChangeTracker->EndTrack();
#define PREDICTION_SPEWVALUECHANGES()					g_pChangeTracker->Spew();
#else
#define PREDICTION_TRACKVALUECHANGESCOPE( context )
#define PREDICTION_TRACKVALUECHANGESCOPE_ENTITY( entity, context )
#define PREDICTION_STARTTRACKVALUE( context )
#define PREDICTION_ENDTRACKVALUE()	
#define PREDICTION_SPEWVALUECHANGES() 
#endif

#endif // !CLIENT_DLL
#endif // PREDICTIONCOPY_H
