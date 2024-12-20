//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "variant_t.h"
#include "entitylist_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

variant_t::~variant_t()
{
	dealloc();
}

void variant_t::dealloc()
{
	if(baseFieldType() == FIELD_CSTRING) {
		if(szVal) {
			free(szVal);
		}
	}
}

void variant_t::assign_plain(const variant_t &other)
{
	switch(other.baseFieldType()) {
	case FIELD_VOID:
		break;
	case FIELD_FLOAT:
		flVal = other.flVal;
		break;
	case FIELD_INTERVAL:
		interVal = other.interVal;
		break;
	case FIELD_UINTEGER:
		uVal = other.uVal;
		break;
	case FIELD_INTEGER:
		iVal = other.iVal;
		break;
	case FIELD_INTEGER64:
		iVal64 = other.iVal64;
		break;
	case FIELD_UINTEGER64:
		uVal64 = other.uVal64;
		break;
	case FIELD_USHORT:
		uhVal = other.uhVal;
		break;
	case FIELD_SHORT:
		hVal = other.hVal;
		break;
	case FIELD_BOOLEAN:
		bVal = other.bVal;
		break;
	case FIELD_CHARACTER:
		cVal = other.cVal;
		break;
	case FIELD_UCHARACTER:
		ucVal = other.ucVal;
		break;
	case FIELD_SCHARACTER:
		scVal = other.scVal;
		break;
	case FIELD_MODELINDEX:
		mdlVal = other.mdlVal;
		break;
	case FIELD_COLOR32:
		rgbaVal = other.rgbaVal;
		break;
	case FIELD_COLOR32E:
		rgbeVal = other.rgbeVal;
		break;
	case FIELD_COLOR24:
		rgbVal = other.rgbVal;
		break;
	case FIELD_POOLED_STRING:
		iszVal = other.iszVal;
		break;
	case FIELD_CSTRING:
		break;
	case FIELD_VECTOR:
		vecVal = other.vecVal;
		break;
	case FIELD_QUATERNION:
		quatVal = other.quatVal;
		break;
	case FIELD_VMATRIX:
		vmatVal = other.vmatVal;
		break;
	case FIELD_MATRIX3X4:
		matVal = other.matVal;
		break;
	case FIELD_QANGLE:
		angVal = other.angVal;
		break;
	case FIELD_VECTOR2D:
		vec2dVal = other.vec2dVal;
		break;
	case FIELD_VECTOR4D:
		vec4dval = other.vec4dval;
		break;
	case FIELD_ENTITYPTR:
		epVal = other.epVal;
		break;
	case FIELD_EHANDLE:
		ehVal = other.ehVal;
		break;
	case FIELD_EDICTPTR:
		edpVal = other.edpVal;
		break;
	case FIELD_FUNCTION:
		fpVal = other.fpVal;
		break;
	case FIELD_INPUT:
		break;
	case FIELD_CUSTOM:
		break;
	case FIELD_EMBEDDED:
		break;
	default:
		break;
	}
}

variant_t::variant_t(const variant_t &other)
	: fieldType_(other.fieldType_)
{
	if(other.baseFieldType() == FIELD_CSTRING) {
		if(other.szVal) {
			int len1 = V_strlen(other.szVal);
			szVal = (char *)malloc(sizeof(char) * len1);
			V_strncpy(szVal, other.szVal, len1);
			szVal[len1] = '\0';
		} else {
			szVal = NULL;
		}
	} else {
		assign_plain(other);
	}
}

variant_t::variant_t(variant_t &&other)
	: fieldType_(other.fieldType_)
{
	if(other.baseFieldType() == FIELD_CSTRING) {
		szVal = other.szVal;
		other.szVal = NULL;
	} else {
		assign_plain(other);
	}
}

variant_t &variant_t::operator=(const variant_t &other)
{
	if(other.baseFieldType() != FIELD_CSTRING && baseFieldType() == FIELD_CSTRING) {
		if(szVal) {
			free(szVal);
		}
	}

	if(other.baseFieldType() == FIELD_CSTRING) {
		if(other.szVal) {
			int len1 = V_strlen(other.szVal);
			if(szVal) {
				int len2 = V_strlen(szVal);
				if(len2 < len1) {
					szVal = (char *)realloc(szVal, sizeof(char) * len1);
				}
			} else {
				szVal = (char *)malloc(sizeof(char) * len1);
			}

			V_strncpy(szVal, other.szVal, len1);
			szVal[len1] = '\0';
		} else {
			if(szVal) {
				free(szVal);
			}
			szVal = NULL;
		}
	} else {
		assign_plain(other);
	}

	fieldType_ = other.fieldType_;
	return *this;
}

variant_t &variant_t::operator=(variant_t &&other)
{
	if(other.baseFieldType() != FIELD_CSTRING && baseFieldType() == FIELD_CSTRING) {
		if(szVal) {
			free(szVal);
		}
	}

	if(other.baseFieldType() == FIELD_CSTRING) {
		szVal = other.szVal;
		other.szVal = NULL;
	} else {
		assign_plain(other);
	}

	fieldType_ = other.fieldType_;
	return *this;
}

void variant_t::SetEntityH( CSharedBaseEntity *val ) 
{ 
	dealloc(); 
	ehVal = val;
	fieldType_ = FIELD_EHANDLE; 
}

const char *variant_t::String( void ) const
{
	static char strbuffer[512];
	switch(baseFieldType()) {
	case FIELD_VOID:
		V_snprintf(strbuffer, sizeof(strbuffer), "");
		break;
	case FIELD_FLOAT:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f", flVal);
		break;
	case FIELD_INTERVAL:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f -> %f", interVal.start, interVal.range);
		break;
	case FIELD_UINTEGER:
		V_snprintf(strbuffer, sizeof(strbuffer), "%u", uVal);
		break;
	case FIELD_INTEGER:
		V_snprintf(strbuffer, sizeof(strbuffer), "%i", iVal);
		break;
	case FIELD_INTEGER64:
		V_snprintf(strbuffer, sizeof(strbuffer), "%lli", iVal64);
		break;
	case FIELD_UINTEGER64:
		V_snprintf(strbuffer, sizeof(strbuffer), "%llu", uVal64);
		break;
	case FIELD_USHORT:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hu", uhVal);
		break;
	case FIELD_SHORT:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hi", hVal);
		break;
	case FIELD_BOOLEAN:
		V_snprintf(strbuffer, sizeof(strbuffer), "%s", bVal ? "true" : "false");
		break;
	case FIELD_CHARACTER:
		V_snprintf(strbuffer, sizeof(strbuffer), "%c", cVal);
		break;
	case FIELD_UCHARACTER:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hhu", ucVal);
		break;
	case FIELD_SCHARACTER:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hhi", scVal);
		break;
	case FIELD_MODELINDEX:
		V_snprintf(strbuffer, sizeof(strbuffer), "%s", IsValidModelIndex(mdlVal) ? modelinfo->GetModelName( modelinfo->GetModel( mdlVal ) ) : "models/error.mdl" );
		break;
	case FIELD_COLOR32:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hhu %hhu %hhu %hhu", rgbaVal.r(), rgbaVal.g(), rgbaVal.b(), rgbaVal.a());
		break;
	case FIELD_COLOR32E:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hhu %hhu %hhu %hhi", rgbeVal.r(), rgbeVal.g(), rgbeVal.b(), rgbeVal.e());
		break;
	case FIELD_COLOR24:
		V_snprintf(strbuffer, sizeof(strbuffer), "%hhu %hhu %hhu", rgbVal.r(), rgbVal.g(), rgbVal.b());
		break;
	case FIELD_POOLED_STRING:
		V_snprintf(strbuffer, sizeof(strbuffer), "%s", iszVal != NULL_STRING ? STRING( iszVal ) : "");
		break;
	case FIELD_CSTRING:
		V_snprintf(strbuffer, sizeof(strbuffer), "%s", szVal ? szVal : "");
		break;
	case FIELD_VECTOR:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f %f %f", vecVal.x, vecVal.y, vecVal.z);
		break;
	case FIELD_QUATERNION:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f %f %f %f", quatVal.x, quatVal.y, quatVal.z, quatVal.w);
		break;
	case FIELD_VMATRIX:
		Assert(0);
		break;
	case FIELD_MATRIX3X4:
		Assert(0);
		break;
	case FIELD_QANGLE:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f %f %f", angVal.x, angVal.y, angVal.z);
		break;
	case FIELD_VECTOR2D:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f %f", vec2dVal.x, vec2dVal.y);
		break;
	case FIELD_VECTOR4D:
		V_snprintf(strbuffer, sizeof(strbuffer), "%f %f %f %f", vec4dval.x, vec4dval.y, vec4dval.z, vec4dval.w);
		break;
	case FIELD_ENTITYPTR: {
		if(epVal) {
			string_t name = epVal->GetEntityNameAsTStr();
			if(name == NULL_STRING) {
				name = epVal->GetClassnameStr();
			}
			V_snprintf(strbuffer, sizeof(strbuffer), "%s", STRING(name));
		} else {
			V_snprintf(strbuffer, sizeof(strbuffer), "");
		}
	} break;
	case FIELD_EHANDLE: {
		CSharedBaseEntity *ent = ehVal.Get();
		if(ent) {
			string_t name = ent->GetEntityNameAsTStr();
			if(name == NULL_STRING) {
				name = ent->GetClassnameStr();
			}
			V_snprintf(strbuffer, sizeof(strbuffer), "%s", STRING(name));
		} else {
			V_snprintf(strbuffer, sizeof(strbuffer), "");
		}
	} break;
	case FIELD_EDICTPTR: {
		CSharedBaseEntity *ent = GetContainingEntity( edpVal );
		if(ent) {
			string_t name = ent->GetEntityNameAsTStr();
			if(name == NULL_STRING) {
				name = ent->GetClassnameStr();
			}
			V_snprintf(strbuffer, sizeof(strbuffer), "%s", STRING(name));
		} else {
			V_snprintf(strbuffer, sizeof(strbuffer), "");
		}
	} break;
	case FIELD_FUNCTION:
		Assert(0);
		break;
	case FIELD_INPUT:
		Assert(0);
		break;
	case FIELD_VARIANT:
		Assert(0);
		break;
	case FIELD_CUSTOM:
		Assert(0);
		break;
	case FIELD_EMBEDDED:
		Assert(0);
		break;
	}
	return strbuffer;
}

const char *variant_t::GetDebug() const
{
	return UTIL_VarArgs("%s (%s)", String(), GetFieldName(rawFieldType(), true));
}

void variant_t::Parse(const char *value)
{
	dealloc();


}

class CVariantFieldOps : public ICustomFieldOps
{
	virtual bool IsEmpty( const FieldInfo_t &fieldInfo )
	{
		// check all the elements of the array (usually only 1)
		variant_t *var = fieldInfo.GetField<variant_t>();
		for ( int i = 0; i < fieldInfo.pTypeDesc->fieldSize; i++, var++ )
		{
			if ( var->baseFieldType() != FIELD_VOID )
				return false;
		}

		// variant has no data
		return true;
	}

	virtual void MakeEmpty( const FieldInfo_t &fieldInfo )
	{
		variant_t *var = fieldInfo.GetField<variant_t>();
		for ( int i = 0; i < fieldInfo.pTypeDesc->fieldSize; i++, var++ )
		{
			var->SetVoid();
		}
	}

	// Parses a keyvalue string into a variant_t.
	// We could just turn it into a string since variant_t can convert it later, but this keyvalue is probably a variant_t for a reason,
	// meaning it might use strings and numbers completely differently without converting them.
	// As a result, we try to read it to figure out what type it is.
	virtual bool Parse( const FieldInfo_t &fieldInfo, char const* szValue )
	{
		variant_t *var = fieldInfo.GetField<variant_t>();
		for ( int i = 0; i < fieldInfo.pTypeDesc->fieldSize; i++, var++ )
		{
			var->Parse(szValue);
		}

		return true;
	}
};

CVariantFieldOps g_VariantFieldOps;
ICustomFieldOps *variantFuncs = &g_VariantFieldOps;
