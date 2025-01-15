//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//


#include "dt_send.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "tier0/dbg.h"
#include "dt_utlvector_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_SENDPROP, "SendProp" );

#if !defined(_STATIC_LINKED) || defined(GAME_DLL)


static CNonModifiedPointerProxy *s_pNonModifiedPointerProxyHead = NULL;

const char *s_ElementNames[MAX_ARRAY_ELEMENTS] =
{
	"000", "001", "002", "003", "004", "005", "006", "007", "008", "009", 
	"010", "011", "012", "013", "014", "015", "016", "017", "018", "019",
	"020", "021", "022", "023", "024", "025", "026", "027", "028", "029",
	"030", "031", "032", "033", "034", "035", "036", "037", "038", "039",
	"040", "041", "042", "043", "044", "045", "046", "047", "048", "049",
	"050", "051", "052", "053", "054", "055", "056", "057", "058", "059",
	"060", "061", "062", "063", "064", "065", "066", "067", "068", "069",
	"070", "071", "072", "073", "074", "075", "076", "077", "078", "079",
	"080", "081", "082", "083", "084", "085", "086", "087", "088", "089",
	"090", "091", "092", "093", "094", "095", "096", "097", "098", "099",
	"100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
	"110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
	"120", "121", "122", "123", "124", "125", "126", "127", "128", "129",
	"130", "131", "132", "133", "134", "135", "136", "137", "138", "139",
	"140", "141", "142", "143", "144", "145", "146", "147", "148", "149",
	"150", "151", "152", "153", "154", "155", "156", "157", "158", "159",
	"160", "161", "162", "163", "164", "165", "166", "167", "168", "169",
	"170", "171", "172", "173", "174", "175", "176", "177", "178", "179",
	"180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
	"190", "191", "192", "193", "194", "195", "196", "197", "198", "199",
	"200", "201", "202", "203", "204", "205", "206", "207", "208", "209",
	"210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
	"220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
	"230", "231", "232", "233", "234", "235", "236", "237", "238", "239",
	"240", "241", "242", "243", "244", "245", "246", "247", "248", "249",
	"250", "251", "252", "253", "254", "255", "256", "257", "258", "259",
	"260", "261", "262", "263", "264", "265", "266", "267", "268", "269",
	"270", "271", "272", "273", "274", "275", "276", "277", "278", "279",
	"280", "281", "282", "283", "284", "285", "286", "287", "288", "289",
	"290", "291", "292", "293", "294", "295", "296", "297", "298", "299",
	"300", "301", "302", "303", "304", "305", "306", "307", "308", "309",
	"310", "311", "312", "313", "314", "315", "316", "317", "318", "319",
	"320", "321", "322", "323", "324", "325", "326", "327", "328", "329",
	"330", "331", "332", "333", "334", "335", "336", "337", "338", "339",
	"340", "341", "342", "343", "344", "345", "346", "347", "348", "349",
	"350", "351", "352", "353", "354", "355", "356", "357", "358", "359",
	"360", "361", "362", "363", "364", "365", "366", "367", "368", "369",
	"370", "371", "372", "373", "374", "375", "376", "377", "378", "379",
	"380", "381", "382", "383", "384", "385", "386", "387", "388", "389",
	"390", "391", "392", "393", "394", "395", "396", "397", "398", "399",
	"400", "401", "402", "403", "404", "405", "406", "407", "408", "409",
	"410", "411", "412", "413", "414", "415", "416", "417", "418", "419",
	"420", "421", "422", "423", "424", "425", "426", "427", "428", "429",
	"430", "431", "432", "433", "434", "435", "436", "437", "438", "439",
	"440", "441", "442", "443", "444", "445", "446", "447", "448", "449",
	"450", "451", "452", "453", "454", "455", "456", "457", "458", "459",
	"460", "461", "462", "463", "464", "465", "466", "467", "468", "469",
	"470", "471", "472", "473", "474", "475", "476", "477", "478", "479",
	"480", "481", "482", "483", "484", "485", "486", "487", "488", "489",
	"490", "491", "492", "493", "494", "495", "496", "497", "498", "499",
	"500", "501", "502", "503", "504", "505", "506", "507", "508", "509",
	"510", "511", "512", "513", "514", "515", "516", "517", "518", "519",
	"520", "521", "522", "523", "524", "525", "526", "527", "528", "529",
	"530", "531", "532", "533", "534", "535", "536", "537", "538", "539",
	"540", "541", "542", "543", "544", "545", "546", "547", "548", "549",
	"550", "551", "552", "553", "554", "555", "556", "557", "558", "559",
	"560", "561", "562", "563", "564", "565", "566", "567", "568", "569",
	"570", "571", "572", "573", "574", "575", "576", "577", "578", "579",
	"580", "581", "582", "583", "584", "585", "586", "587", "588", "589",
	"590", "591", "592", "593", "594", "595", "596", "597", "598", "599",
	"600", "601", "602", "603", "604", "605", "606", "607", "608", "609",
	"610", "611", "612", "613", "614", "615", "616", "617", "618", "619",
	"620", "621", "622", "623", "624", "625", "626", "627", "628", "629",
	"630", "631", "632", "633", "634", "635", "636", "637", "638", "639",
	"640", "641", "642", "643", "644", "645", "646", "647", "648", "649",
	"650", "651", "652", "653", "654", "655", "656", "657", "658", "659",
	"660", "661", "662", "663", "664", "665", "666", "667", "668", "669",
	"670", "671", "672", "673", "674", "675", "676", "677", "678", "679",
	"680", "681", "682", "683", "684", "685", "686", "687", "688", "689",
	"690", "691", "692", "693", "694", "695", "696", "697", "698", "699",
	"700", "701", "702", "703", "704", "705", "706", "707", "708", "709",
	"710", "711", "712", "713", "714", "715", "716", "717", "718", "719",
	"720", "721", "722", "723", "724", "725", "726", "727", "728", "729",
	"730", "731", "732", "733", "734", "735", "736", "737", "738", "739",
	"740", "741", "742", "743", "744", "745", "746", "747", "748", "749",
	"750", "751", "752", "753", "754", "755", "756", "757", "758", "759",
	"760", "761", "762", "763", "764", "765", "766", "767", "768", "769",
	"770", "771", "772", "773", "774", "775", "776", "777", "778", "779",
	"780", "781", "782", "783", "784", "785", "786", "787", "788", "789",
	"790", "791", "792", "793", "794", "795", "796", "797", "798", "799",
	"800", "801", "802", "803", "804", "805", "806", "807", "808", "809",
	"810", "811", "812", "813", "814", "815", "816", "817", "818", "819",
	"820", "821", "822", "823", "824", "825", "826", "827", "828", "829",
	"830", "831", "832", "833", "834", "835", "836", "837", "838", "839",
	"840", "841", "842", "843", "844", "845", "846", "847", "848", "849",
	"850", "851", "852", "853", "854", "855", "856", "857", "858", "859",
	"860", "861", "862", "863", "864", "865", "866", "867", "868", "869",
	"870", "871", "872", "873", "874", "875", "876", "877", "878", "879",
	"880", "881", "882", "883", "884", "885", "886", "887", "888", "889",
	"890", "891", "892", "893", "894", "895", "896", "897", "898", "899",
	"900", "901", "902", "903", "904", "905", "906", "907", "908", "909",
	"910", "911", "912", "913", "914", "915", "916", "917", "918", "919",
	"920", "921", "922", "923", "924", "925", "926", "927", "928", "929",
	"930", "931", "932", "933", "934", "935", "936", "937", "938", "939",
	"940", "941", "942", "943", "944", "945", "946", "947", "948", "949",
	"950", "951", "952", "953", "954", "955", "956", "957", "958", "959",
	"960", "961", "962", "963", "964", "965", "966", "967", "968", "969",
	"970", "971", "972", "973", "974", "975", "976", "977", "978", "979",
	"980", "981", "982", "983", "984", "985", "986", "987", "988", "989",
	"990", "991", "992", "993", "994", "995", "996", "997", "998", "999",
	"1000", "1001", "1002", "1003", "1004", "1005", "1006", "1007", "1008", "1009",
	"1010", "1011", "1012", "1013", "1014", "1015", "1016", "1017", "1018", "1019",
	"1020", "1021", "1022", "1023"

};


CNonModifiedPointerProxy::CNonModifiedPointerProxy( SendTableProxyFn fn )
{
	m_pNext = s_pNonModifiedPointerProxyHead;
	s_pNonModifiedPointerProxyHead = this;
	m_Fn = fn;
}


CStandardSendProxiesV1::CStandardSendProxiesV1()
{
	m_Int8 = SendProxy_SChar;
	m_Int16 = SendProxy_Short;
	m_Int32 = SendProxy_Int;

	m_UInt8 = SendProxy_UChar;
	m_UInt16 = SendProxy_UShort;
	m_UInt32 = SendProxy_UInt;
	
	m_Float = SendProxy_Float;
	m_Vector = SendProxy_Vector;

#ifdef DT_INT64_SUPPORTED
	m_Int64 = SendProxy_Int64;
	m_UInt64 = SendProxy_UInt64;
#endif
}

CStandardSendProxies::CStandardSendProxies()
{
	m_DataTableToDataTable = SendProxy_DataTableToDataTable;
	m_SendLocalDataTable = SendProxy_SendLocalDataTable;
	m_ppNonModifiedPointerProxies = &s_pNonModifiedPointerProxyHead;
}

CStandardSendProxiesEx::CStandardSendProxiesEx()
{
#ifndef DT_INT64_SUPPORTED
	m_Int64 = SendProxy_Int64;
	m_UInt64 = SendProxy_UInt64;
#endif
}

CStandardSendProxiesEx g_StandardSendProxies;


// ---------------------------------------------------------------------- //
// Proxies.
// ---------------------------------------------------------------------- //
void SendProxy_FloatAngle( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	float angle;

	angle = *((float*)pData);
	pOut->m_Float = anglemod( angle );

	Assert( IsFinite( pOut->m_Float ) );
}

void SendProxy_Float( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Float = *((float*)pData);
	Assert( IsFinite( pOut->m_Float ) );
}

void SendProxy_QAngles( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	const QAngle &v = *(QAngle*)pData;
	Assert( v.IsValid() );
	pOut->m_Angles = v;
}

void SendProxy_Vector( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	const Vector& v = *(Vector*)pData;
	Assert( v.IsValid() );
	pOut->m_Vector = v;
}

void SendProxy_VectorXY( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	const Vector& v = *(Vector*)pData;
	Assert( IsFinite( v.x ) && IsFinite( v.y ) );
	pOut->m_Vector.x = v.x;
	pOut->m_Vector.y = v.y;
}

void SendProxy_Vector2D( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	const Vector2D& v = *(Vector2D*)pData;
	Assert( v.IsValid() );
	pOut->m_Vector2D = v;
}

void SendProxy_Quaternion( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	const Quaternion& q = *(Quaternion*)pData;
	Assert( q.IsValid() );
#if defined DT_QUATERNION_SUPPORTED || defined DT_INT64_SUPPORTED
	pOut->m_Quaternion = q;
#else
	Error( "Quaternion sendprops are not supported" );
#endif
}

void SendProxy_Char( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Char = *((const char*)pData);
}

void SendProxy_SChar( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_SChar = *((const signed char*)pData);
}

void SendProxy_Bool( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Bool = *((const bool*)pData);
}

void SendProxy_UChar( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_UChar = *((const unsigned char*)pData);
}

void SendProxy_Short( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Short = *((short*)pData);
}

void SendProxy_UShort( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_UShort = *((unsigned short*)pData);
}

void SendProxy_Int( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((int*)pData);
}

void SendProxy_UInt( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_Int = *((unsigned int*)pData);
}

void SendProxy_Int64( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
#if defined DT_QUATERNION_SUPPORTED || defined DT_INT64_SUPPORTED
	pOut->m_Int64 = *((int64*)pData);
#else
	*((int64 *)pOut->m_IntPair) = *((int64*)pData);
#endif
}

void SendProxy_UInt64( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
#if defined DT_QUATERNION_SUPPORTED || defined DT_INT64_SUPPORTED
	pOut->m_UInt64 = *((uint64*)pData);
#else
	*((uint64 *)pOut->m_UIntPair) = *((uint64*)pData);
#endif
}

void SendProxy_IntAddOne( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	const int *pInt = (const int *)pVarData;

	pOut->m_Int = (*pInt) + 1;
}

void SendProxy_ShortAddOne( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID)
{
	const short *pInt = (const short *)pVarData;

	pOut->m_Short = (*pInt) + 1;
}

void SendProxy_Color32( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Color32 = *((const color32 *)pData);
}

void SendProxy_Color32E( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Color32E = *((const ColorRGBExp32 *)pData);
}

void SendProxy_Color24( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	pOut->m_Color24 = *((const color24 *)pData);
}

void SendProxy_CString( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	pOut->m_pString = (const char*)pData;
}

void* SendProxy_DataTableToDataTable( const SendPropInfo *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	Assert( pData );
	void *out = (void*)pData;
	Assert( out );
	return out;
}

void* SendProxy_DataTablePtrToDataTable( const SendPropInfo *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	Assert( pData );
	void *out = *((void**)pData);
	Assert( out );
	return out;
}

void SendProxy_Empty( const SendPropInfo *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
}

//-----------------------------------------------------------------------------
// Purpose: If the recipient is the same as objectID, go ahead and iterate down
//  the m_Local stuff, otherwise, act like it wasn't there at all.
// This way, only the local player receives information about him/herself.
// Input  : *pVarData - 
//			*pOut - 
//			objectID - 
//-----------------------------------------------------------------------------

void* SendProxy_SendLocalDataTable( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetOnly( objectID - 1 );
	return ( void * )pVarData;
}


//-----------------------------------------------------------------------------
// Purpose: Filters updates to a variable so that only non-local players see
// the changes.  This is so we can send a low-res origin to non-local players
// while sending a hi-res one to the local player.
// Input  : *pVarData -
//			*pOut -
//			objectID -
//-----------------------------------------------------------------------------

void* SendProxy_SendNonLocalDataTable( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient( objectID - 1 );
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendNonLocalDataTable );


// ---------------------------------------------------------------------- //
// Prop setup functions (for building tables).
// ---------------------------------------------------------------------- //
float AssignRangeMultiplier( int nBits, double range )
{
	unsigned long iHighValue;
	if ( nBits == 32 )
		iHighValue = 0xFFFFFFFE;
	else
		iHighValue = ((1 << (unsigned long)nBits) - 1);

	float fHighLowMul = iHighValue / range;
	if ( CloseEnough( range, 0 ) )
		fHighLowMul = iHighValue;
	
	// If the precision is messing us up, then adjust it so it won't.
	if ( (unsigned long)(fHighLowMul * range) > iHighValue ||
		 (fHighLowMul * range) > (double)iHighValue )
	{
		// Squeeze it down smaller and smaller until it's going to produce an integer
		// in the valid range when given the highest value.
		float multipliers[] = { 0.9999, 0.99, 0.9, 0.8, 0.7 };
		int i;
		for ( i=0; i < ARRAYSIZE( multipliers ); i++ )
		{
			fHighLowMul = (float)( iHighValue / range ) * multipliers[i];
			if ( (unsigned long)(fHighLowMul * range) > iHighValue ||
				(fHighLowMul * range) > (double)iHighValue )
			{
			}
			else
			{
				break;
			}
		}

		if ( i == ARRAYSIZE( multipliers ) )
		{
			// Doh! We seem to be unable to represent this range.
			Assert( false );
			return 0;
		}
	}

	return fHighLowMul;
}

static SendPropType g_dtFieldTypes[FIELD_BASE_TYPECOUNT] = 
{
	DPT_NUMSendPropTypes,
	DPT_Float,
	DPT_VectorXY,
	DPT_Int,
	DPT_Int,
#ifdef DT_INT64_SUPPORTED
	DPT_Int64,
	DPT_Int64,
#else
	DPT_VectorXY,
	DPT_VectorXY,
#endif
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_String,
	DPT_String,
	DPT_Vector,
#ifdef DT_QUATERNION_SUPPORTED
	DPT_Quaternion,
#elif defined DT_INT64_SUPPORTED
	DPT_Int64,
#else
	DPT_NUMSendPropTypes,
#endif
	DPT_NUMSendPropTypes,
	DPT_NUMSendPropTypes,
	DPT_Vector,
	DPT_VectorXY,
#ifdef DT_QUATERNION_SUPPORTED
	DPT_Quaternion,
#elif defined DT_INT64_SUPPORTED
	DPT_Int64,
#else
	DPT_NUMSendPropTypes,
#endif
#ifdef DT_INT64_SUPPORTED
	DPT_Int64,
#else
	DPT_VectorXY,
#endif
	DPT_Int,
	DPT_Int,
	DPT_Int,
	DPT_NUMSendPropTypes,
	DPT_NUMSendPropTypes,
	DPT_NUMSendPropTypes,
	DPT_NUMSendPropTypes,
	DPT_NUMSendPropTypes,
};

extern void SendProxy_StringT( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_EHandle( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID);
extern void SendProxy_ModelIndex( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID);
extern void SendProxy_PredictableId( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Time( const SendPropInfo *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

static SendVarProxyFn g_dtFieldProxies[FIELD_BASE_TYPECOUNT] = 
{
	NULL,
	SendProxy_Float,
	NULL,
	SendProxy_UInt,
	SendProxy_Int,
	SendProxy_Int64,
	SendProxy_UInt64,
	SendProxy_UShort,
	SendProxy_Short,
	SendProxy_Bool,
	SendProxy_Char,
	SendProxy_UChar,
	SendProxy_SChar,
	SendProxy_ModelIndex,
	SendProxy_Color32,
	SendProxy_Color32E,
	SendProxy_Color24,
	SendProxy_StringT,
	SendProxy_CString,
	SendProxy_Vector,
	SendProxy_Quaternion,
	NULL,
	NULL,
	SendProxy_QAngles,
	SendProxy_Vector2D,
	NULL,
	SendProxy_PredictableId,
	NULL,
	SendProxy_EHandle,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

void SendPropAuto_impl(SendPropInfoEx &ret, const char *pVarName, fieldtype_t type, int offset, int sizeofVar, int nBits, DTFlags_t flags, SendVarProxyFn varProxy, DTPriority_t priority)
{
	switch(type) {
	case FIELD_TIME:
	case FIELD_DISTANCE:
	case FIELD_SCALE:
		flags |= SPROP_UNSIGNED;
		break;
	default:
		switch(GetBaseFieldType(type)) {
		case FIELD_UINTEGER:
		case FIELD_UINTEGER64:
		case FIELD_USHORT:
		case FIELD_BOOLEAN:
		case FIELD_UCHARACTER:
		case FIELD_COLOR32:
		case FIELD_COLOR32E:
		case FIELD_COLOR24:
		case FIELD_POOLED_STRING:
		case FIELD_CSTRING:
		case FIELD_EHANDLE:
		case FIELD_PREDICTABLEID:
			flags |= SPROP_UNSIGNED;
			break;
		}
		break;
	}

	switch(GetBaseFieldType(type)) {
	case FIELD_INTEGER64:
	case FIELD_UINTEGER64:
	case FIELD_COLOR32:
	case FIELD_COLOR32E:
	case FIELD_POOLED_STRING:
	case FIELD_CSTRING:
		flags |= SPROP_NOSCALE;
		break;
	}

	switch(GetBaseFieldType(type)) {
	case FIELD_FLOAT:
	case FIELD_VECTOR:
	case FIELD_QUATERNION:
	case FIELD_VMATRIX:
	case FIELD_MATRIX3X4:
	case FIELD_VECTOR2D:
	case FIELD_VECTOR4D:
		if((GetFieldTypeFlags(type) & FIELD_TYPE_FLAG_WORLDSPACE) != 0) {
			if((flags & (
				(SPROP_COORD|SPROP_COORD_MP|SPROP_COORD_MP_LOWPRECISION|SPROP_COORD_MP_INTEGRAL)
			#ifdef DT_CELL_COORD_SUPPORTED
				|(SPROP_CELL_COORD|SPROP_CELL_COORD_LOWPRECISION|SPROP_CELL_COORD_INTEGRAL)
			#endif
			)) == SPROP_NONE) {
				flags |= SPROP_COORD_MP;
			}
		}
		break;
	}

	if(flags & SPROP_NOSCALE) {
		flags &= ~(
			(SPROP_NORMAL|SPROP_ROUNDDOWN|SPROP_ROUNDUP)|
			(SPROP_COORD|SPROP_COORD_MP|SPROP_COORD_MP_LOWPRECISION|SPROP_COORD_MP_INTEGRAL)
		#ifdef DT_CELL_COORD_SUPPORTED
			|(SPROP_CELL_COORD|SPROP_CELL_COORD_LOWPRECISION|SPROP_CELL_COORD_INTEGRAL)
		#endif
		);

		nBits = (sizeofVar * 8);
	} else {
		switch(type) {
		case FIELD_TIME:
			nBits = TIME_BITS;
			break;
		case FIELD_DISTANCE:
			nBits = DISTANCE_BITS;
			break;
		case FIELD_SCALE:
			nBits = SCALE_BITS;
			break;
		default:
			switch(GetBaseFieldType(type)) {
			case FIELD_EHANDLE:
				nBits = NUM_NETWORKED_EHANDLE_BITS;
				break;
			case FIELD_PREDICTABLEID:
				nBits = PREDICTABLE_ID_BITS;
				break;
			case FIELD_FLOAT:
			case FIELD_VECTOR:
			case FIELD_QUATERNION:
			case FIELD_VMATRIX:
			case FIELD_MATRIX3X4:
			case FIELD_VECTOR2D:
			case FIELD_VECTOR4D:
				if(flags & SPROP_NORMAL) {
					nBits = NORMAL_FRACTIONAL_BITS;
				} else if(flags & SPROP_COORD_MP_LOWPRECISION) {
					nBits = (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS_MP_LOWPRECISION);
				} else if(flags & (SPROP_COORD_MP|SPROP_COORD_MP_INTEGRAL)) {
					nBits = (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS);
				} else if(flags & SPROP_COORD) {
					nBits = (COORD_INTEGER_BITS + COORD_FRACTIONAL_BITS);
				}
				break;
			case FIELD_BOOLEAN:
			case FIELD_UINTEGER:
			case FIELD_INTEGER:
			case FIELD_INTEGER64:
			case FIELD_UINTEGER64:
			case FIELD_USHORT:
			case FIELD_SHORT:
			case FIELD_CHARACTER:
			case FIELD_UCHARACTER:
			case FIELD_SCHARACTER:
				if(flags & SPROP_NORMAL) {
					nBits = 1;
				} else if(flags & SPROP_COORD_MP_LOWPRECISION) {
					nBits = COORD_INTEGER_BITS_MP;
				} else if(flags & (SPROP_COORD_MP|SPROP_COORD_MP_INTEGRAL)) {
					nBits = COORD_INTEGER_BITS_MP;
				} else if(flags & SPROP_COORD) {
					nBits = COORD_INTEGER_BITS;
				}
				break;
			case FIELD_QANGLE:
				nBits = ANGLE_BITS;
				break;
			default:
				break;
			}
			break;
		}
	}

	if(nBits <= 0) {
		nBits = (sizeofVar * 8);
	}

	if(flags & SPROP_NOSCALE) {
		ret.m_fLowValue = 0.0f;
		ret.m_fHighValue = 0.0f;
		ret.m_fHighLowMul = 0.0f;
	} else if(flags & SPROP_NORMAL) {
		ret.m_fLowValue = -1.0f;
		ret.m_fHighValue = 1.0f;
		ret.m_fHighLowMul = AssignRangeMultiplier( NORMAL_FRACTIONAL_BITS, 1.0f - -1.0f );
	} else if(flags & SPROP_COORD_MP_INTEGRAL) {
		ret.m_fLowValue = INT_MIN;
		ret.m_fHighValue = INT_MAX;
		ret.m_fHighLowMul = AssignRangeMultiplier( (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS), static_cast<float>(INT_MAX) - static_cast<float>(INT_MIN) );
	} else if(flags & SPROP_COORD_MP_LOWPRECISION) {
		ret.m_fLowValue = MIN_COORD_FLOAT;
		ret.m_fHighValue = MAX_COORD_FLOAT;
		ret.m_fHighLowMul = AssignRangeMultiplier( (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS_MP_LOWPRECISION), MAX_COORD_FLOAT - MIN_COORD_FLOAT );
	} else if(flags & SPROP_COORD_MP) {
		ret.m_fLowValue = MIN_COORD_FLOAT;
		ret.m_fHighValue = MAX_COORD_FLOAT;
		ret.m_fHighLowMul = AssignRangeMultiplier( (COORD_INTEGER_BITS_MP + COORD_FRACTIONAL_BITS), MAX_COORD_FLOAT - MIN_COORD_FLOAT );
	} else if(flags & SPROP_COORD) {
		ret.m_fLowValue = MIN_COORD_FLOAT;
		ret.m_fHighValue = MAX_COORD_FLOAT;
		ret.m_fHighLowMul = AssignRangeMultiplier( (COORD_INTEGER_BITS + COORD_FRACTIONAL_BITS), MAX_COORD_FLOAT - MIN_COORD_FLOAT );
	} else if(GetBaseFieldType(type) == FIELD_QANGLE) {
		ret.m_fLowValue = -360.0f;
		ret.m_fHighValue = 360.0f;
		ret.m_fHighLowMul = AssignRangeMultiplier( ANGLE_BITS, 360.0f - -360.0f );
	} else if(type == FIELD_DISTANCE) {
		ret.m_fLowValue = 0.0f;
		ret.m_fHighValue = MAX_TRACE_LENGTH;
		ret.m_fHighLowMul = AssignRangeMultiplier( DISTANCE_BITS, MAX_TRACE_LENGTH - 0.0f );
	} else if(type == FIELD_SCALE) {
		ret.m_fLowValue = 0.0f;
		ret.m_fHighValue = 1.0f;
		ret.m_fHighLowMul = AssignRangeMultiplier( SCALE_BITS, 1.0f - 0.0f );
	}

	if(priority == SENDPROP_CHANGES_OFTEN_PRIORITY) {
		flags |= SPROP_CHANGES_OFTEN;
	}

	if((flags & SPROP_CHANGES_OFTEN) != SPROP_NONE && priority == SENDPROP_DEFAULT_PRIORITY) {
		priority = SENDPROP_CHANGES_OFTEN_PRIORITY;
	}

	ret.m_Type = g_dtFieldTypes[ GetBaseFieldType(type) ];
	Assert( ret.m_Type != DPT_NUMSendPropTypes );

	if(!varProxy) {
		switch(type) {
		case FIELD_TIME:
			varProxy = SendProxy_Time;
			break;
		default:
			varProxy = g_dtFieldProxies[ GetBaseFieldType(type) ];
			break;
		}
	}
	Assert( varProxy != NULL );
	ret.SetProxyFn( varProxy );

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.SetPriority( priority );
}

SendPropInfoEx SendPropFloat(
	const char *pVarName,		
	// Variable name.
	int offset,			// Offset into container structure.
	int sizeofVar,
	int nBits,			// Number of bits to use when encoding.
	DTFlags_t flags,
	float fLowValue,		// For floating point, low and high values.
	float fHighValue,		// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

	SendPropAuto_impl(ret, pVarName, FIELD_FLOAT, offset, sizeofVar, nBits, flags, varProxy, priority);

#ifdef _DEBUG
	if ( varProxy == SendProxy_Float )
	{
		Assert( sizeofVar == 0 || sizeofVar == 4 );
	}
#endif

	if ( nBits <= 0 || nBits == 32 )
	{
		flags |= SPROP_NOSCALE;
		fLowValue = 0.f;
		fHighValue = 0.f;
	}
	else
	{
		if(fHighValue == HIGH_DEFAULT)
			fHighValue = (1 << nBits);

		if (flags & SPROP_ROUNDDOWN)
			fHighValue = fHighValue - ((fHighValue - fLowValue) / (1 << nBits));
		else if (flags & SPROP_ROUNDUP)
			fLowValue = fLowValue + ((fHighValue - fLowValue) / (1 << nBits));
	}

	ret.m_Type = DPT_Float;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL ) )
		ret.m_nBits = 0;

	return ret;
}

SendPropInfoEx SendPropVector(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	DTFlags_t flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

#ifdef _DEBUG
	if(varProxy == SendProxy_Vector )
	{
		Assert(sizeofVar == sizeof(Vector));
	}
#endif

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Vector;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL) )
		ret.m_nBits = 0;

	return ret;
}

SendPropInfoEx SendPropVectorXY(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	DTFlags_t flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

#ifdef _DEBUG
	if(varProxy == SendProxy_VectorXY )
	{
		Assert(sizeofVar == sizeof(Vector));
	}
#endif

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_VectorXY;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL) )
		ret.m_nBits = 0;

	return ret;
}

SendPropInfoEx SendPropQuaternion(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,					// Number of bits to use when encoding.
	DTFlags_t flags,
	float fLowValue,			// For floating point, low and high values.
	float fHighValue,			// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

#ifdef _DEBUG
	if(varProxy == SendProxy_Quaternion )
	{
		Assert(sizeofVar == sizeof(Quaternion));
	}
#endif

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

#ifdef DT_QUATERNION_SUPPORTED
	ret.m_Type = DPT_Quaternion;
#elif defined DT_INT64_SUPPORTED
	ret.m_Type = DPT_Int64;
#else
	Error( "Quaternion sendprops are not supported" );
#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = fLowValue;
	ret.m_fHighValue = fHighValue;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );
	if( ret.GetFlags() & (SPROP_COORD | SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL) )
		ret.m_nBits = 0;

	return ret;
}

SendPropInfoEx SendPropAngle(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

#ifdef _DEBUG
	if(varProxy == SendProxy_FloatAngle )
	{
		Assert(sizeofVar == 4);
	}
#endif

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Float;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 360.0f;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );

	return ret;
}


SendPropInfoEx SendPropQAngles(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

#ifdef _DEBUG
	if(varProxy == SendProxy_QAngles )
	{
		Assert(sizeofVar == sizeof(QAngle));
	}
#endif

	if ( nBits == 32 )
		flags |= SPROP_NOSCALE;

	ret.m_Type = DPT_Vector;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.m_fLowValue = 0.0f;
	ret.m_fHighValue = 360.0f;
	ret.m_fHighLowMul = AssignRangeMultiplier( ret.m_nBits, ret.m_fHighValue - ret.m_fLowValue );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );

	return ret;
}
  
SendPropInfoEx SendPropInt(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int nBits,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

	if ( !varProxy )
	{
		if ( sizeofVar == 1 )
		{
			varProxy = SendProxy_SChar;
		}
		else if ( sizeofVar == 2 )
		{
			varProxy = SendProxy_Short;
		}
		else if ( sizeofVar == 4 )
		{
			varProxy = SendProxy_Int;
		}
		else if ( sizeofVar == 8 )
		{
			varProxy = SendProxy_Int64;
		}
		else
		{
			Assert(!"SendPropInt var has invalid size");
			varProxy = SendProxy_SChar;	// safest one...
		}
	}

	// Figure out # of bits if the want us to.
	if ( nBits <= 0 )
	{
		Assert( sizeofVar == 1 || sizeofVar == 2 || sizeofVar == 4 || sizeofVar == 8 );
		nBits = sizeofVar * 8;
	}

	if( sizeofVar == 8 )
	{
	#ifdef DT_INT64_SUPPORTED
		ret.m_Type = DPT_Int64;
	#else
		ret.m_Type = DPT_VectorXY;

		flags |= SPROP_NOSCALE;
	#endif
	}
	else
	{
		ret.m_Type = DPT_Int;
	}

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_nBits = nBits;
	ret.SetFlags( flags );
	ret.SetPriority( priority );

	// Use UInt proxies if they want unsigned data. This isn't necessary to encode
	// the values correctly, but it lets us check the ranges of the data to make sure
	// they're valid.
	ret.SetProxyFn( varProxy );
	if( ret.GetFlags() & SPROP_UNSIGNED )
	{
		if( varProxy == SendProxy_SChar )
			ret.SetProxyFn( SendProxy_UChar );
		
		else if( varProxy == SendProxy_Short )
			ret.SetProxyFn( SendProxy_UShort );

		else if( varProxy == SendProxy_Int )
			ret.SetProxyFn( SendProxy_UInt );
		else if( varProxy == SendProxy_Int64 )
			ret.SetProxyFn( SendProxy_UInt64 );
	}

	return ret;
}

SendPropInfoEx SendPropBool(
	const char *pVarName,
	int offset,
	int sizeofVar,
	DTPriority_t priority )
{
	Assert( sizeofVar == sizeof( bool ) );
	return SendPropInt( pVarName, offset, sizeofVar, 1, SPROP_UNSIGNED, SendProxy_Bool, priority );
}

SendPropInfoEx SendPropIntWithMinusOneFlag( const char *pVarName, int offset, int sizeofVar, int nBits, SendVarProxyFn proxyFn )
{
	return SendPropInt( pVarName, offset, sizeofVar, nBits, SPROP_UNSIGNED, proxyFn );
}

SendPropInfoEx SendPropString(
	const char *pVarName,
	int offset,
	int bufferLen,
	DTFlags_t flags,
	SendVarProxyFn varProxy,
	DTPriority_t priority)
{
	SendPropInfoEx ret;

	Assert( bufferLen <= DT_MAX_STRING_BUFFERSIZE ); // You can only have strings with 8-bits worth of length.
	
	ret.m_Type = DPT_String;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetFlags( flags );
	ret.SetProxyFn( varProxy );
	ret.SetPriority( priority );

	return ret;
}

SendPropInfoEx SendPropArray3(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	SendPropInfoEx pArrayProp,
	SendTableProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

	Assert( elements <= MAX_ARRAY_ELEMENTS );

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetDataTableProxyFn( varProxy );
	ret.SetPriority( priority );

	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}

	SendPropInfoEx *pArrayPropAllocated = new SendPropInfoEx;
	*pArrayPropAllocated = pArrayProp;
	ret.SetArrayProp( pArrayPropAllocated );
	ret.m_Flags |= SPROP_ALLOCATED_ARRAYPROP;

	SendPropInfo *pProps = new SendPropInfo[elements]; // TODO free that again
	
	for ( int i = 0; i < elements; i++ )
	{
		pProps[i] = pArrayProp;	// copy array element property setting
		pProps[i].SetOffset( i*sizeofVar ); // adjust offset
		pProps[i].m_pVarName = s_ElementNames[i];	// give unique name
		pProps[i].m_pParentArrayPropName = pVarName; // For debugging...
	}

	SendTableInfo *pTable = new SendTableInfo( pProps, elements, pVarName ); // TODO free that again
	ret.SetDataTable( pTable );
	ret.m_Flags |= SPROP_ALLOCATED_SENDTABLE;

	return ret;
}

SendPropInfoEx SendPropDataTable(
	const char *pVarName,
	int offset,
	SendTableInfo *pTable,
	SendTableProxyFn varProxy,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.SetDataTable( pTable );
	ret.SetDataTableProxyFn( varProxy );
	ret.SetPriority( priority );
	
	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}
	
	if ( varProxy == SendProxy_DataTableToDataTable && offset == 0 )
	{
		ret.SetFlags( SPROP_COLLAPSIBLE );
	}

	return ret;
}


SendPropInfoEx InternalSendPropArray(
	const int elementCount,
	const int elementStride,
	const char *pName,
	ArrayLengthSendProxyFn arrayLengthFn,
	DTPriority_t priority
	)
{
	SendPropInfoEx ret;

	ret.m_Type = DPT_Array;
	ret.m_nElements = elementCount;
	ret.m_ElementStride = elementStride;
	ret.m_pVarName = pName;
	ret.SetPriority( priority );
	ret.SetProxyFn( SendProxy_Empty );
	ret.m_pArrayProp = NULL;	// This gets set in SendTable_InitTable. It always points at the property that precedes
								// this one in the datatable's list.
	ret.SetArrayLengthProxy( arrayLengthFn );
		
	return ret;
}


SendPropInfoEx SendPropExclude(
	const char *pDataTableName,	// Data table name (given to BEGIN_SEND_TABLE and BEGIN_RECV_TABLE).
	const char *pPropName		// Name of the property to exclude.
	)
{
	SendPropInfoEx ret;

	ret.SetFlags( SPROP_EXCLUDE );
	ret.m_pExcludeDTName = pDataTableName;
	ret.m_pVarName = pPropName;

	return ret;
}



// ---------------------------------------------------------------------- //
// SendProp
// ---------------------------------------------------------------------- //
SendPropInfo::SendPropInfo()
{
	m_pVarName = NULL;
	m_Offset = 0;
	m_pDataTable = NULL;
	m_ProxyFn = NULL;
	m_pExcludeDTName = NULL;
	m_pParentArrayPropName = NULL;

	m_Type = DPT_Int;
	m_Flags = SPROP_NONE;
	m_nBits = 0;

	m_fLowValue = 0.0f;
	m_fHighValue = 0.0f;
	m_fHighLowMul = 0.0f;
	m_pArrayProp = 0;
	m_ArrayLengthProxy = 0;
	m_nElements = 1;
	m_ElementStride = -1;

#ifdef DT_PRIORITY_SUPPORTED
	m_priority = (byte)SENDPROP_DEFAULT_PRIORITY;
#else
	m_pExtraData = new CSendPropExtra_Base;
	m_pExtraData->m_priority = (byte)SENDPROP_DEFAULT_PRIORITY;
	m_Flags |= SPROP_ALLOCATED_EXTRADATA;
#endif
}

SendPropInfo::SendPropInfo(SendPropInfo &&other)
{
	m_pMatchingRecvProp = other.m_pMatchingRecvProp;
	m_Type = other.m_Type;
	m_nBits = other.m_nBits;
	m_fLowValue = other.m_fLowValue;
	m_fHighValue = other.m_fHighValue;
	m_pArrayProp = other.m_pArrayProp;
	m_ArrayLengthProxy = other.m_ArrayLengthProxy;
	m_nElements = other.m_nElements;
	m_ElementStride = other.m_ElementStride;
	m_pExcludeDTName = other.m_pExcludeDTName;
	m_pParentArrayPropName = other.m_pParentArrayPropName;
	m_pVarName = other.m_pVarName;
	m_fHighLowMul = other.m_fHighLowMul;
#ifdef DT_PRIORITY_SUPPORTED
	m_priority = other.m_priority;
#endif
	m_Flags = other.m_Flags;
	m_ProxyFn = other.m_ProxyFn;
	m_DataTableProxyFn = other.m_DataTableProxyFn;
	m_pDataTable = other.m_pDataTable;
	m_Offset = other.m_Offset;
	m_pExtraData = other.m_pExtraData;

	other.m_pExtraData = NULL;
	other.m_Flags &= ~SPROP_ALLOCATED_EXTRADATA;
	other.m_pArrayProp = NULL;
	other.m_Flags &= ~SPROP_ALLOCATED_ARRAYPROP;
	other.m_pDataTable = NULL;
	other.m_Flags &= ~SPROP_ALLOCATED_SENDTABLE;
}

SendPropInfo &SendPropInfo::operator=(SendPropInfo &&other)
{
	if((m_Flags & SPROP_ALLOCATED_EXTRADATA) != SPROP_NONE) {
		if(m_pExtraData) {
			delete m_pExtraData;
		}
	}

	if((m_Flags & SPROP_ALLOCATED_ARRAYPROP) != SPROP_NONE) {
		if(m_pArrayProp) {
			delete m_pArrayProp;
		}
	}

	if((m_Flags & SPROP_ALLOCATED_SENDTABLE) != SPROP_NONE) {
		if(m_pDataTable) {
			delete[] m_pDataTable->m_pProps;
			delete m_pDataTable;
		}
	}

	m_pMatchingRecvProp = other.m_pMatchingRecvProp;
	m_Type = other.m_Type;
	m_nBits = other.m_nBits;
	m_fLowValue = other.m_fLowValue;
	m_fHighValue = other.m_fHighValue;
	m_pArrayProp = other.m_pArrayProp;
	m_ArrayLengthProxy = other.m_ArrayLengthProxy;
	m_nElements = other.m_nElements;
	m_ElementStride = other.m_ElementStride;
	m_pExcludeDTName = other.m_pExcludeDTName;
	m_pParentArrayPropName = other.m_pParentArrayPropName;
	m_pVarName = other.m_pVarName;
	m_fHighLowMul = other.m_fHighLowMul;
#ifdef DT_PRIORITY_SUPPORTED
	m_priority = other.m_priority;
#endif
	m_Flags = other.m_Flags;
	m_ProxyFn = other.m_ProxyFn;
	m_DataTableProxyFn = other.m_DataTableProxyFn;
	m_pDataTable = other.m_pDataTable;
	m_Offset = other.m_Offset;
	m_pExtraData = other.m_pExtraData;

	other.m_pExtraData = NULL;
	other.m_Flags &= ~SPROP_ALLOCATED_EXTRADATA;
	other.m_pArrayProp = NULL;
	other.m_Flags &= ~SPROP_ALLOCATED_ARRAYPROP;
	other.m_pDataTable = NULL;
	other.m_Flags &= ~SPROP_ALLOCATED_SENDTABLE;

	return *this;
}

SendPropInfo::~SendPropInfo()
{
	if((m_Flags & SPROP_ALLOCATED_EXTRADATA) != SPROP_NONE) {
		if(m_pExtraData) {
			delete m_pExtraData;
		}
	}

	if((m_Flags & SPROP_ALLOCATED_ARRAYPROP) != SPROP_NONE) {
		if(m_pArrayProp) {
			delete m_pArrayProp;
		}
	}

	if((m_Flags & SPROP_ALLOCATED_SENDTABLE) != SPROP_NONE) {
		if(m_pDataTable) {
			delete[] m_pDataTable->m_pProps;
			delete m_pDataTable;
		}
	}
}


int SendPropInfo::GetNumArrayLengthBits() const
{
	Assert( GetType() == DPT_Array );

	return Q_log2( GetNumElements() ) + 1;
}


// ---------------------------------------------------------------------- //
// SendTable
// ---------------------------------------------------------------------- //
SendTableInfo::SendTableInfo()
{
	Construct( NULL, 0, NULL );
}


SendTableInfo::SendTableInfo(SendPropInfo *pProps, int nProps, const char *pNetTableName)
{
	Construct( pProps, nProps, pNetTableName );
}


SendTableInfo::~SendTableInfo()
{
//	Assert( !m_pPrecalc );
}


void SendTableInfo::Construct( SendPropInfo *pProps, int nProps, const char *pNetTableName )
{
	m_pProps = pProps;
	m_nProps = nProps;
	m_pNetTableName = pNetTableName;
	m_pPrecalc = 0;
	m_bInitialized = false;
	m_bHasBeenWritten = false;
	m_bHasPropsEncodedAgainstCurrentTickCount = false;
}

#endif
