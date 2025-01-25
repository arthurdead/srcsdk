#include "cbase.h"
#include "map_entity.h"
#include "gamestringpool.h"
#include "mapentities_shared.h"
#include "filesystem.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlhashtable.h"

#ifdef GAME_DLL
#include "util.h"
#else
#include "iclassmap.h"
#endif

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

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->baseType() == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			void *pEmbeddedObject = GetField<void>(pObject, *pField);
			for ( datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap )
			{
				if ( ParseKeyvalue( pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
					return true;
			}
		}

		if ( (pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName) )
		{
			switch( pField->baseType() )
			{
			case FIELD_FLOAT:
				*GetField<float>(pObject, *pField) = atof( szValue );
				return true;

			case FIELD_UINTEGER:
				*GetField<unsigned int>(pObject, *pField) = atoi( szValue );
				return true;
			case FIELD_INTEGER:
				*GetField<int>(pObject, *pField) = atoi( szValue );
				return true;
			case FIELD_INTEGER64:
				*GetField<int64>(pObject, *pField) = strtoll( szValue, NULL, 10 );
				return true;
			case FIELD_UINTEGER64:
				*GetField<uint64>(pObject, *pField) = strtoull( szValue, NULL, 10 );
				return true;
			case FIELD_USHORT:
				*GetField<unsigned short>(pObject, *pField) = (unsigned short)atoi( szValue );
				return true;
			case FIELD_SHORT:
				*GetField<short>(pObject, *pField) = (short)atoi( szValue );
				return true;
			case FIELD_BOOLEAN:
				*GetField<bool>(pObject, *pField) = (bool)(atoi( szValue ) != 0);
				return true;
			case FIELD_CHARACTER:
				*GetField<char>(pObject, *pField) = (char)atoi( szValue );
				return true;
			case FIELD_UCHARACTER:
				*GetField<unsigned char>(pObject, *pField) = (unsigned char)atoi( szValue );
				return true;
			case FIELD_SCHARACTER:
				*GetField<signed char>(pObject, *pField) = (signed char)atoi( szValue );
				return true;

			case FIELD_MODELINDEX:
			#ifdef GAME_DLL
				*GetField<modelindex_t>(pObject, *pField) = engine->PrecacheModel( szValue );
			#else
				*GetField<modelindex_t>(pObject, *pField) = modelinfo->GetModelIndex( szValue );
			#endif
				return true;

			case FIELD_COLOR32:
				UTIL_StringToColor32( *GetField<color32>(pObject, *pField), szValue );
				return true;
			case FIELD_COLOR32E:
				UTIL_StringToColor32( *GetField<ColorRGBExp32>(pObject, *pField), szValue );
				return true;
			case FIELD_COLOR24:
				UTIL_StringToColor24( *GetField<color24>(pObject, *pField), szValue );
				return true;

			case FIELD_POOLED_STRING:
				*GetField<string_t>(pObject, *pField) = AllocPooledString( szValue );
				return true;

			case FIELD_VECTOR:
				UTIL_StringToVector( *GetField<Vector>(pObject, *pField), szValue );
				return true;
			case FIELD_VMATRIX: {
				VMatrix &mat = *GetField<VMatrix>(pObject, *pField);
				UTIL_StringToFloatArray( mat.Base(), 16, szValue );
			} return true;
			case FIELD_MATRIX3X4: {
				matrix3x4_t &mat = *GetField<matrix3x4_t>(pObject, *pField);
				UTIL_StringToFloatArray( mat.Base(), 12, szValue );
			} return true;
			case FIELD_QANGLE:
				UTIL_StringToQAngle( *GetField<QAngle>(pObject, *pField), szValue );
				return true;

			case FIELD_CUSTOM: {
				FieldInfo_t fieldInfo(
					GetField<void>(pObject, *pField),
					pObject,
					*pField
				);
				pField->pFieldOps->Parse( fieldInfo, szValue );
				return false;
			}

			default:
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

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->baseType() == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			void *pEmbeddedObject = GetField<void>(pObject, *pField);
			for ( datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap )
			{
				if ( ExtractKeyvalue( pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue, iMaxLen ) )
					return true;
			}
		}

		if ( (pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName) )
		{
			switch( pField->baseType() )
			{
			case FIELD_FLOAT:
				Q_snprintf( szValue, iMaxLen, "%f", *GetField<float>(pObject, *pField) );
				return true;

			case FIELD_UINTEGER:
				Q_snprintf( szValue, iMaxLen, "%u", *GetField<unsigned int>(pObject, *pField) );
				return true;
			case FIELD_INTEGER:
				Q_snprintf( szValue, iMaxLen, "%i", *GetField<int>(pObject, *pField) );
				return true;
			case FIELD_INTEGER64:
				Q_snprintf( szValue, iMaxLen, "%lli", *GetField<int64>(pObject, *pField) );
				return true;
			case FIELD_UINTEGER64:
				Q_snprintf( szValue, iMaxLen, "%llu", *GetField<uint64>(pObject, *pField) );
				return true;
			case FIELD_USHORT:
				Q_snprintf( szValue, iMaxLen, "%hu", *GetField<unsigned short>(pObject, *pField) );
				return true;
			case FIELD_SHORT:
				Q_snprintf( szValue, iMaxLen, "%hi", *GetField<short>(pObject, *pField) );
				return true;
			case FIELD_BOOLEAN:
				Q_snprintf( szValue, iMaxLen, "%hhu", *GetField<bool>(pObject, *pField) );
				return true;
			case FIELD_CHARACTER:
				Q_snprintf( szValue, iMaxLen, "%c", *GetField<char>(pObject, *pField) );
				return true;
			case FIELD_UCHARACTER:
				Q_snprintf( szValue, iMaxLen, "%hhu", *GetField<unsigned char>(pObject, *pField) );
				return true;
			case FIELD_SCHARACTER:
				Q_snprintf( szValue, iMaxLen, "%hhi", *GetField<signed char>(pObject, *pField) );
				return true;

			case FIELD_MODELINDEX:
				Q_strncpy( szValue, modelinfo->GetModelName( modelinfo->GetModel( *GetField<modelindex_t>(pObject, *pField) ) ), iMaxLen);
				return true;

			case FIELD_COLOR32: {
				const color32 &clr = *GetField<color32>(pObject, *pField);
				Q_snprintf( szValue, iMaxLen, "%hhu %hhu %hhu %hhu", 
					clr.r(),
					clr.g(),
					clr.b(),
					clr.a() );
			} return true;
			case FIELD_COLOR32E: {
				const ColorRGBExp32 &clr = *GetField<ColorRGBExp32>(pObject, *pField);
				Q_snprintf( szValue, iMaxLen, "%hhu %hhu %hhu %hhi", 
					clr.r(),
					clr.g(),
					clr.b(),
					clr.e() );
			} return true;
			case FIELD_COLOR24: {
				const color24 &clr = *GetField<color24>(pObject, *pField);
				Q_snprintf( szValue, iMaxLen, "%hhu %hhu %hhu", 
					clr.r(),
					clr.g(),
					clr.b() );
			} return true;

			case FIELD_POOLED_STRING:
				Q_strncpy( szValue, STRING( *GetField<string_t>(pObject, *pField) ), iMaxLen );
				return true;

			case FIELD_VECTOR: {
				const Vector &vec = *GetField<Vector>(pObject, *pField);
				Q_snprintf( szValue, iMaxLen, "%f %f %f", 
					vec.x,
					vec.y,
					vec.z );
			} return true;
			case FIELD_VMATRIX:
				Assert(0);
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
				return false;
			case FIELD_MATRIX3X4:
				Assert(0);
				//UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
				return false;
			case FIELD_QANGLE: {
				const QAngle &vec = *GetField<QAngle>(pObject, *pField);
				Q_snprintf( szValue, iMaxLen, "%f %f %f", 
					vec.x,
					vec.y,
					vec.z );
			} return true;

			case FIELD_CUSTOM: {
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
			} return false;

			default:
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
	m_pClassname = classname;
	s_Parsers.Insert( m_pClassname );
}

void CMapKVParser::ParseMapData( CEntityMapData *mapData )
{
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];

	#ifdef _DEBUG
	datamap_t *pDataMap = GetMapDataDesc();
	::ValidateDataDescription(pDataMap, m_pClassname);
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

		if ( *ent_debugkeys.GetString() && !Q_stricmp(ent_debugkeys.GetString(), m_pClassname) )
		{
			// Msg( "-- found entity of type %s\n", m_pClassname );
			printKeyHits = true;
			debugName = m_pClassname;
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
			Msg( "!! (%s) key not handled: \"%s\" \"%s\"\n", m_pClassname, szKeyName, szValue );
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

#ifndef SWDS
static void write_fgd_embedded(datamap_t *pMap, CUtlBuffer &fileBuffer, bool root)
{
	if(!root) {
		if(pMap->baseMap) {
			write_fgd_embedded(pMap->baseMap, fileBuffer, false);
		}
	}

	for(int i = 0; i < pMap->dataNumFields; ++i) {
		const typedescription_t &td = pMap->dataDesc[i];
		if(td == INVALID_TYPEDESC) {
			continue;
		}

		if(td.baseType() == FIELD_EMBEDDED) {
			write_fgd_embedded(td.td, fileBuffer, false);
			continue;
		}

		fileBuffer.PutChar('\t');

		if(td.flags & FTYPEDESC_OUTPUT) {
			fileBuffer.PutString("output ");
		} else if(td.flags & FTYPEDESC_INPUT) {
			fileBuffer.PutString("input ");
		}

		if(td.externalName) {
			fileBuffer.PutString(td.externalName);
		} else if(td.fieldName) {
			fileBuffer.PutString(td.fieldName);
		} else {
			Assert(0);
		}

		if(td.m_nChoicesLen > 0 && (td.flags & (FTYPEDESC_OUTPUT|FTYPEDESC_INPUT)) == FTYPEDESC_NONE) {
			fileBuffer.PutString("(choices)");
		} else {
			switch(td.rawType()) {
			case FIELD_EXACT_CLASSNAME:
			case FIELD_PARTIAL_CLASSNAME:
			case FIELD_EXACT_TARGETNAME:
			case FIELD_PARTIAL_TARGETNAME:
			case FIELD_EXACT_TARGETNAME_OR_CLASSNAME:
			case FIELD_PARTIAL_TARGETNAME_OR_CLASSNAME:
				fileBuffer.PutString("(target_name_or_class)");
				break;
			case FIELD_MATERIALINDEX:
				fileBuffer.PutString("(material)");
				break;
			case FIELD_STUDIO_MODELIDNEX:
			case FIELD_POOLED_MODELNAME:
				fileBuffer.PutString("(studio)");
				break;
			case FIELD_SPRITE_MODELINDEX:
			case FIELD_POOLED_SPRITENAME:
				fileBuffer.PutString("(sprite)");
				break;
			case FIELD_POOLED_SOUNDNAME:
				fileBuffer.PutString("(sound)");
				break;
			case FIELD_POOLED_SCENENAME:
				fileBuffer.PutString("(scene)");
				break;
			default:
				switch(td.baseType()) {
				case FIELD_VOID:
					fileBuffer.PutString("(void)");
					break;
				case FIELD_FLOAT:
					fileBuffer.PutString("(float)");
					break;
				case FIELD_UINTEGER:
				case FIELD_INTEGER:
				case FIELD_INTEGER64:
				case FIELD_UINTEGER64:
				case FIELD_USHORT:
				case FIELD_SHORT:
				case FIELD_CHARACTER:
				case FIELD_UCHARACTER:
				case FIELD_SCHARACTER:
				case FIELD_MODELINDEX:
					fileBuffer.PutString("(integer)");
					break;
				case FIELD_BOOLEAN:
					if((td.flags & (FTYPEDESC_OUTPUT|FTYPEDESC_INPUT)) == FTYPEDESC_NONE) {
						fileBuffer.PutString("(boolean)");
					} else {
						fileBuffer.PutString("(bool)");
					}
					break;
				case FIELD_COLOR32:
				case FIELD_COLOR32E:
				case FIELD_COLOR24:
					fileBuffer.PutString("(color255)");
					break;
				case FIELD_POOLED_STRING:
				case FIELD_CSTRING:
					fileBuffer.PutString("(string)");
					break;
				case FIELD_VECTOR:
				case FIELD_QUATERNION:
				case FIELD_VMATRIX:
				case FIELD_MATRIX3X4:
				case FIELD_QANGLE:
				case FIELD_VECTOR2D:
				case FIELD_VECTOR4D:
					fileBuffer.PutString("(vector)");
					break;
				case FIELD_PREDICTABLEID:
					fileBuffer.PutString("(integer)");
					break;
				case FIELD_ENTITYPTR:
				case FIELD_EHANDLE:
				case FIELD_EDICTPTR:
					fileBuffer.PutString("(target_name_or_class)");
					break;
				default:
					Assert(0);
					break;
				}
			}
		}

		if((td.flags & (FTYPEDESC_OUTPUT|FTYPEDESC_INPUT)) == FTYPEDESC_NONE) {
			fileBuffer.PutString(" : ");
			if(td.m_pGuiName) {
				fileBuffer.PutDelimitedString(GetCStringCharConversion(), td.m_pGuiName);
			} else if(td.externalName) {
				fileBuffer.PutDelimitedString(GetCStringCharConversion(), td.externalName);
			} else if(td.fieldName) {
				fileBuffer.PutDelimitedString(GetCStringCharConversion(), td.fieldName);
			} else {
				Assert(0);
			}
			fileBuffer.PutString(" : ");
			if(td.m_pDefValue[0] != '\0') {
				fileBuffer.PutDelimitedString(GetCStringCharConversion(), td.m_pDefValue);
			}
		}

		fileBuffer.PutString(" : ");
		fileBuffer.PutDelimitedString(GetCStringCharConversion(), td.m_pDescription);

		if(((td.flags & (FTYPEDESC_OUTPUT|FTYPEDESC_INPUT)) == FTYPEDESC_NONE) && (td.m_nChoicesLen > 0)) {
			fileBuffer.PutString(" = [");
			for(int i = 0; i < td.m_nChoicesLen; ++i) {
				fileBuffer.PutString("\t\t");
				fileBuffer.PutString(td.m_pChoices[i].value);
				fileBuffer.PutString(" : ");
				fileBuffer.PutString(td.m_pChoices[i].name);
				fileBuffer.PutChar('\n');
			}
			fileBuffer.PutChar(']');
		}

		fileBuffer.PutChar('\n');
	}
}

static bool has_base(datamap_t *pMap, const char *name)
{
	do {
		if(V_strcmp(pMap->dataClassName, name) == 0) {
			return true;
		}

		pMap = pMap->baseMap;
	} while(pMap);

	return false;
}

static void write_fgd_class(map_datamap_t *pMap, CUtlBuffer &fileBuffer, CUtlHashtable<datamap_t *> &already_parsed)
{
	if(already_parsed.HasElement(pMap))
		return;

	if(pMap->baseMap) {
		write_fgd_class((map_datamap_t *)pMap->baseMap, fileBuffer, already_parsed);
	}

	fileBuffer.PutString("@BaseClass");

	if(pMap->baseMap) {
		fileBuffer.PutString(" base(");
		fileBuffer.PutString(pMap->baseMap->dataClassName);
		fileBuffer.PutString(")");
	}

	fileBuffer.PutString(" = ");
	fileBuffer.PutString(pMap->dataClassName);

	fileBuffer.PutString(" : ");
	fileBuffer.PutDelimitedString(GetCStringCharConversion(), pMap->m_pDescription ? pMap->m_pDescription : "");

	fileBuffer.PutString("\n[\n");

	write_fgd_embedded(pMap, fileBuffer, true);

	fileBuffer.PutString("]\n");

	already_parsed.Insert(pMap);
}

void WriteFGDs()
{
	CUtlBuffer fileBuffer( 4096, 1024*1024, CUtlBuffer::TEXT_BUFFER );

	CUtlHashtable<datamap_t *> already_parsed;

	map_datamap_t *pMap = g_pMapDatamapsHead;
	while(pMap) {
		write_fgd_class(pMap, fileBuffer, already_parsed);
		pMap = pMap->m_pNext;
	}

#ifdef GAME_DLL
	for(int i = 0; i < EntityFactoryDictionary()->GetFactoryCount(); ++i) {
		IEntityFactory *pFactory = EntityFactoryDictionary()->GetFactory(i);
#else
	for(int i = 0; i < GetClassMap().GetFactoryCount(); ++i) {
		IEntityFactory *pFactory = GetClassMap().GetFactory(i);
#endif

		map_datamap_t *pDesc = pFactory->GetMapDataDesc();

		int type = -1;
		map_datamap_t *it = pDesc;
		do {
			type = it->m_nType;
			it = it->m_pNext;
		} while(type == -1 && it);

		switch(type) {
		case MAPENT_SOLIDCLASS:
			fileBuffer.PutString("@SolidClass");
			break;
		case MAPENT_NPCCLASS:
			fileBuffer.PutString("@NPCClass");
			break;
		case MAPENT_MOVECLASS:
			fileBuffer.PutString("@MoveClass");
			break;
		case MAPENT_KEYFRAMECLASS:
			fileBuffer.PutString("@KeyFrameClass");
			break;
		case MAPENT_FILTERCLASS:
			fileBuffer.PutString("@FilterClass");
			break;
		case MAPENT_POINTCLASS:
			fileBuffer.PutString("@PointClass");
			break;
		default:
			fileBuffer.PutString("@PointClass");
			break;
		}

		fileBuffer.PutString(" base(");
		fileBuffer.PutString(pDesc->dataClassName);
		fileBuffer.PutString(")");

		fileBuffer.PutString(" = ");
		fileBuffer.PutString(pFactory->MapClassname());

		fileBuffer.PutString(" : ");
		fileBuffer.PutDelimitedString(GetCStringCharConversion(), pDesc->m_pDescription ? pDesc->m_pDescription : "");

		fileBuffer.PutString("\n[\n]\n");
	}

#ifdef GAME_DLL
	if(!g_pFullFileSystem->WriteFile("resource/server.fgd", "MOD", fileBuffer))
#else
	if(!g_pFullFileSystem->WriteFile("resource/client.fgd", "MOD", fileBuffer))
#endif
	{
		Log_Warning(LOG_MAPPARSE,"unable to write fgd\n");
	}

#ifdef CLIENT_DLL
	if(CommandLine()->HasParm("-nogamedll"))
#endif
	{
		fileBuffer.Clear();

		fileBuffer.Printf("@mapsize(%i, %i)", MIN_COORD_INTEGER, MAX_COORD_INTEGER);

		if(!g_pFullFileSystem->WriteFile("resource/shared.fgd", "MOD", fileBuffer))
		{
			Log_Warning(LOG_MAPPARSE,"unable to write fgd\n");
		}

		fileBuffer.Clear();

		fileBuffer.PutString("@include \"shared.fgd\"\n");
		fileBuffer.PutString("@include \"client.fgd\"\n");
		fileBuffer.PutString("@include \"server.fgd\"\n");

		const char *pGameDir = CommandLine()->ParmValue("-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) );

		char fgdname[MAX_PATH];
		V_strncpy(fgdname, pGameDir, sizeof(fgdname));
		V_AppendSlash(fgdname, sizeof(fgdname));
		V_strncat(fgdname, "gameinfo.txt", sizeof(fgdname));

		KeyValues *gameinfo = new KeyValues("GameInfo");
		gameinfo->LoadFromFile(g_pFullFileSystem, fgdname, NULL);
		const char *fgdvalue = gameinfo->GetString("GameData", "game.fgd");
		V_snprintf(fgdname, sizeof(fgdname), "resource/%s", fgdvalue);
		gameinfo->deleteThis();

		if(!g_pFullFileSystem->WriteFile(fgdname, "MOD", fileBuffer))
		{
			Log_Warning(LOG_MAPPARSE,"unable to write fgd\n");
		}
	}
}

CON_COMMAND_LINKED(write_fgds, "")
{
	WriteFGDs();
}
#endif
