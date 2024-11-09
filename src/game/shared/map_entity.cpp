#include "cbase.h"
#include "map_entity.h"
#include "gamestringpool.h"
#include "mapentities_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
extern ConVar ent_debugkeys;
#endif

//-----------------------------------------------------------------------------
// Purpose: iterates through a typedescript data block, so it can insert key/value data into the block
// Input  : *pObject - pointer to the struct or class the data is to be insterted into
//			*pFields - description of the data
//			iNumFields - number of fields contained in pFields
//			char *szKeyName - name of the variable to look for
//			char *szValue - value to set the variable to
// Output : Returns true if the variable is found and set, false if the key is not found.
//-----------------------------------------------------------------------------
bool ParseKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue )
{
	int i;
	typedescription_t 	*pField;

	for ( i = 0; i < iNumFields; i++ )
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[ TD_OFFSET_NORMAL ];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for ( datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap )
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if ( ParseKeyvalue( pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
					return true;
			}
		}

		if ( (pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName) )
		{
			switch( pField->fieldType )
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*(string_t *)((char *)pObject + fieldOffset)) = AllocPooledString( szValue );
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float *)((char *)pObject + fieldOffset)) = atof( szValue );
				return true;

			case FIELD_BOOLEAN:
				(*(bool *)((char *)pObject + fieldOffset)) = (bool)(atoi( szValue ) != 0);
				return true;

			case FIELD_CHARACTER:
				(*(char *)((char *)pObject + fieldOffset)) = (char)atoi( szValue );
				return true;

			case FIELD_SHORT:
				(*(short *)((char *)pObject + fieldOffset)) = (short)atoi( szValue );
				return true;

			case FIELD_INTEGER64:
				(*(int64 *)((char *)pObject + fieldOffset)) = strtoull( szValue, NULL, 10 );
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				(*(int *)((char *)pObject + fieldOffset)) = atoi( szValue );
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				UTIL_StringToVector( (float *)((char *)pObject + fieldOffset), szValue );
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
				return true;

			case FIELD_MATRIX3X4_WORLDSPACE:
				UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
				return true;

			case FIELD_COLOR32:
				UTIL_StringToColor32( (color32 *) ((char *)pObject + fieldOffset), szValue );
				return true;

			case FIELD_CUSTOM:
			{
				FieldInfo_t fieldInfo =
				{
					(char *)pObject + fieldOffset,
					pObject,
					pField
				};
				pField->pFieldOps->Parse( fieldInfo, szValue );
				return false;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning( "Bad field in entity!!\n" );
				Assert(0);
				break;
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: iterates through a typedescript data block, so it can insert key/value data into the block
// Input  : *pObject - pointer to the struct or class the data is to be insterted into
//			*pFields - description of the data
//			iNumFields - number of fields contained in pFields
//			char *szKeyName - name of the variable to look for
//			char *szValue - value to set the variable to
// Output : Returns true if the variable is found and set, false if the key is not found.
//-----------------------------------------------------------------------------
bool ExtractKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, char *szValue, int iMaxLen )
{
	int i;
	typedescription_t 	*pField;

	for ( i = 0; i < iNumFields; i++ )
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[ TD_OFFSET_NORMAL ];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for ( datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap )
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if ( ExtractKeyvalue( pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue, iMaxLen ) )
					return true;
			}
		}

		if ( (pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName) )
		{
			switch( pField->fieldType )
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				Q_strncpy( szValue, ((char *)pObject + fieldOffset), iMaxLen );
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				Q_snprintf( szValue, iMaxLen, "%f", (*(float *)((char *)pObject + fieldOffset)) );
				return true;

			case FIELD_BOOLEAN:
				Q_snprintf( szValue, iMaxLen, "%d", (*(bool *)((char *)pObject + fieldOffset)) != 0);
				return true;

			case FIELD_CHARACTER:
				Q_snprintf( szValue, iMaxLen, "%d", (*(char *)((char *)pObject + fieldOffset)) );
				return true;

			case FIELD_SHORT:
				Q_snprintf( szValue, iMaxLen, "%d", (*(short *)((char *)pObject + fieldOffset)) );
				return true;

			case FIELD_INTEGER64:
				Q_snprintf( szValue, iMaxLen, "%lli", (*(int64 *)((char *)pObject + fieldOffset)) );
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				Q_snprintf( szValue, iMaxLen, "%d", (*(int *)((char *)pObject + fieldOffset)) );
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				Q_snprintf( szValue, iMaxLen, "%f %f %f", 
					((float *)((char *)pObject + fieldOffset))[0],
					((float *)((char *)pObject + fieldOffset))[1],
					((float *)((char *)pObject + fieldOffset))[2] );
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				Assert(0);
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
				return false;

			case FIELD_MATRIX3X4_WORLDSPACE:
				Assert(0);
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
				return false;

			case FIELD_COLOR32:
				Q_snprintf( szValue, iMaxLen, "%d %d %d %d", 
					((char *)pObject + fieldOffset)[0],
					((char *)pObject + fieldOffset)[1],
					((char *)pObject + fieldOffset)[2],
					((char *)pObject + fieldOffset)[3] );
				return true;

			case FIELD_CUSTOM:
			{
				Assert(0);
				/*
				SaveRestoreFieldInfo_t fieldInfo =
				{
					(char *)pObject + fieldOffset,
					pObject,
					pField
				};
				pField->pSaveRestoreOps->Parse( fieldInfo, szValue );
				*/
				return false;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning( "Bad field in entity!!\n" );
				Assert(0);
				break;
			}
		}
	}

	return false;
}

#ifdef _DEBUG
extern void ValidateDataDescription(datamap_t *pDataMap, const char *classname);
#endif

BEGIN_MAPKVPARSER_NO_BASE(CMapKVParser)
END_MAPKVPARSER()

CUtlDict<CMapKVParser *, unsigned short> CMapKVParser::s_Parsers;

CMapKVParser::CMapKVParser( const char *classname )
{
	m_iClassname = AllocPooledString( classname );

	s_Parsers.Insert( STRING(m_iClassname) );
}

void CMapKVParser::ParseMapData( CEntityMapData *mapData )
{
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];

	#ifdef _DEBUG
	datamap_t *pDataMap = GetMapDataDesc();
	::ValidateDataDescription(pDataMap, STRING(m_iClassname));
	#endif // _DEBUG

	// loop through all keys in the data block and pass the info back into the object
	if ( mapData->GetFirstKey(keyName, value) )
	{
		do 
		{
			KeyValue( keyName, value );
		} 
		while ( mapData->GetNextKey(keyName, value) );
	}

	OnParseMapDataFinished();
}

bool CMapKVParser::KeyValue( const char *szKeyName, const char *szValue ) 
{
	//!! temp hack, until worldcraft is fixed
	// strip the # tokens from (duplicate) key names
	char *s = (char *)strchr( szKeyName, '#' );
	if ( s )
	{
		*s = '\0';
	}

	// loop through the data description, and try and place the keys in
#ifdef GAME_DLL
	if ( !*ent_debugkeys.GetString() )
#endif
	{
		for ( datamap_t *dmap = GetMapDataDesc(); dmap != NULL; dmap = dmap->baseMap )
		{
			if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
				return true;
		}
	}
#ifdef GAME_DLL
	else
	{
		// debug version - can be used to see what keys have been parsed in
		bool printKeyHits = false;
		const char *debugName = "";

		if ( *ent_debugkeys.GetString() && !Q_stricmp(ent_debugkeys.GetString(), STRING(m_iClassname)) )
		{
			// Msg( "-- found entity of type %s\n", STRING(m_iClassname) );
			printKeyHits = true;
			debugName = STRING(m_iClassname);
		}

		// loop through the data description, and try and place the keys in
		for ( datamap_t *dmap = GetMapDataDesc(); dmap != NULL; dmap = dmap->baseMap )
		{
			if ( !printKeyHits && *ent_debugkeys.GetString() && !Q_stricmp(dmap->dataClassName, ent_debugkeys.GetString()) )
			{
				// Msg( "-- found class of type %s\n", dmap->dataClassName );
				printKeyHits = true;
				debugName = dmap->dataClassName;
			}

			if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
			{
				if ( printKeyHits )
					Msg( "(%s) key: %-16s value: %s\n", debugName, szKeyName, szValue );
				
				return true;
			}
		}

		if ( printKeyHits )
			Msg( "!! (%s) key not handled: \"%s\" \"%s\"\n", STRING(m_iClassname), szKeyName, szValue );
	}
#endif

	// key hasn't been handled
	return false;
}

void CMapKVParser::OnParseMapDataFinished()
{
}

CMapKVParser *CMapKVParser::Find( const char *classname )
{
	int idx = s_Parsers.Find( classname );
	if(idx == s_Parsers.InvalidIndex()) {
		return NULL;
	}

	return s_Parsers[idx];
}
