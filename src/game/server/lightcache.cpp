#include "cbase.h"
#include "lightcache.h"
#include "enginecallback.h"
#include "igamesystem.h"
#include "filesystem.h"
#include "tier1/utlbuffer.h"
#include "tier1/generichash.h"
#include "tier1/utlhashtable.h"
#ifndef SWDS
#include "game_loopback/igameclientloopback.h"
#endif
#include "con_nprint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//TODO!!!!! make it not stupid

struct Lightcache_t
{
	Lightcache_t(const Vector &pos_, float intensity_)
		: pos(pos_), intensity(intensity_)
	{
	}

	Vector pos;
	float intensity;
};

struct LightHashFunctor
{
	unsigned int operator()( const Vector &rhs ) const
	{
		unsigned int posi[3]{ rhs.x, rhs.y, rhs.z };
		return Hash12( posi );
	}
};

struct LightEqualFunctor
{
	bool operator()( const Vector &lhs, const Vector &rhs ) const
	{
		return CloseEnough( lhs, rhs );
	}
};

#ifdef _DEBUG
ConVar draw_lightcache("draw_lightcache", "0");
#endif

class CLightCacheSystem : CAutoGameSystemPerFrame
{
public:
	static inline const unsigned int magic = 'L'<<24 | 'I'<<16 | 'I'<<8 | 'C';
	static inline const unsigned short version = 1;

	virtual void FrameUpdatePostEntityThink()
	{
	#ifdef _DEBUG
		if(draw_lightcache.GetBool()) {
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
			if(pPlayer) {
				trace_t tr;
				Vector forward;
				pPlayer->EyeVectors( &forward );
				UTIL_TraceLine(pPlayer->EyePosition(),
					pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_AI_VISION, 
					pPlayer, COLLISION_GROUP_NONE, &tr );

				if ( tr.fraction != 1.0 )
				{
					float lightIntensity = GetLightIntensity( 1, tr.endpos );

					con_nprint_s nxPrn = { 0 };
					nxPrn.time_to_live = -1;
					nxPrn.color[0] = 0.9f, nxPrn.color[1] = 1.0f, nxPrn.color[2] = 0.9f;
					nxPrn.fixed_width_font = true;
					nxPrn.index = 40;

					engine->Con_NXPrintf( &nxPrn, "%f", lightIntensity );
				}
			}
		}
	#endif
	}

	virtual void LevelInitPreEntity()
	{
		if(gpGlobals->mapname == NULL_STRING)
			return;

		char filename[MAX_PATH];
		V_sprintf_safe(filename, "maps" CORRECT_PATH_SEPARATOR_S "%s.lightcache", STRING(gpGlobals->mapname));

		CUtlBuffer buf;
		if(!g_pFullFileSystem->ReadFile(filename, "MOD", buf))
			return;

		if(buf.GetUnsignedInt() != magic)
			return;

		if(buf.GetUnsignedShort() != version)
			return;

		unsigned int count = buf.GetUnsignedInt();
		lights.Reserve( count );
		for(unsigned int i = 0; i < count; ++i) {
			Vector pos;

			pos.x = buf.GetFloat();
			pos.y = buf.GetFloat();
			pos.z = buf.GetFloat();

			float intensity = buf.GetFloat();

			lights.Insert( pos, Lightcache_t( pos, intensity ) );
		}
	}

	void Save()
	{
		if(gpGlobals->mapname == NULL_STRING)
			return;

		char filename[MAX_PATH];
		V_sprintf_safe(filename, "maps" CORRECT_PATH_SEPARATOR_S "%s.lightcache", STRING(gpGlobals->mapname));

		CUtlBuffer buf;
		buf.PutUnsignedInt( magic );
		buf.PutUnsignedShort( version );

		buf.PutUnsignedInt( lights.Count() );
		FOR_EACH_HASHTABLE( lights, it )
		{
			buf.PutFloat( lights[it].pos.x );
			buf.PutFloat( lights[it].pos.y );
			buf.PutFloat( lights[it].pos.z );

			buf.PutFloat( lights[it].intensity );
		}

		g_pFullFileSystem->RemoveFile(filename, "MOD");
		g_pFullFileSystem->WriteFile(filename, "MOD", buf);
	}

	virtual void LevelShutdownPostEntity()
	{
		Save();

		lights.Purge();
	}

	CUtlHashtable< Vector, Lightcache_t, LightHashFunctor, LightEqualFunctor > lights;
};
static CLightCacheSystem lightcache;

void UpdateLightIntensity( int playerIndex, const Vector &pos )
{
#ifndef SWDS
	IGameClientLoopback *cl_loopback = GetGameClientLoopback( playerIndex );
	if(!g_bDedicatedServer && cl_loopback) {
		Vector light( 0, 0, 0 );
		light = cl_loopback->GetLightForPoint( pos, true );

		Vector ambientColor;
		cl_loopback->GetAmbientLightColor( ambientColor );

		float ambientIntensity = ambientColor.x + ambientColor.y + ambientColor.z;
		float lightIntensity = light.x + light.y + light.z;
		lightIntensity = clamp( lightIntensity, 0.f, 1.f );	// sum can go well over 1.0, but it's the lower region we care about.  if it's bright, we don't need to know *how* bright.

		lightIntensity = MAX( lightIntensity, ambientIntensity );

		auto idx = lightcache.lights.Find( pos );
		if(idx != lightcache.lights.InvalidHandle()) {
			lightcache.lights[idx].intensity = lightIntensity;
		} else {
			lightcache.lights.Insert( pos, Lightcache_t( pos, lightIntensity ) );
		}
	}
#endif
}

float GetLightIntensity( int playerIndex, const Vector &pos )
{
#ifndef SWDS
	IGameClientLoopback *cl_loopback = GetGameClientLoopback( playerIndex );
	if(!g_bDedicatedServer && cl_loopback) {
		Vector light( 0, 0, 0 );
		light = cl_loopback->GetLightForPoint( pos, true );

		Vector ambientColor;
		cl_loopback->GetAmbientLightColor( ambientColor );

		float ambientIntensity = ambientColor.x + ambientColor.y + ambientColor.z;
		float lightIntensity = light.x + light.y + light.z;
		lightIntensity = clamp( lightIntensity, 0.f, 1.f );	// sum can go well over 1.0, but it's the lower region we care about.  if it's bright, we don't need to know *how* bright.

		lightIntensity = MAX( lightIntensity, ambientIntensity );

		auto idx = lightcache.lights.Find( pos );
		if(idx != lightcache.lights.InvalidHandle()) {
			lightcache.lights[idx].intensity = lightIntensity;
		} else {
			lightcache.lights.Insert( pos, Lightcache_t( pos, lightIntensity ) );
		}

		return lightIntensity;
	}
	else
#endif
	{
		FOR_EACH_HASHTABLE( lightcache.lights, it )
		{
			if(lightcache.lights[it].pos.DistTo(pos) <= 1.0f) {
				return lightcache.lights[it].intensity;
			}
		}
	}

	return 1.0f;
}

#ifdef _DEBUG
CON_COMMAND(get_lightcache, "")
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if(!pPlayer) {
		return;
	}

	const Vector &pos = pPlayer->EyePosition();

	float lightIntensity = GetLightIntensity( pPlayer->entindex(), pos );
	Msg("%f\n", lightIntensity);
}
#endif

void SaveLightIntensity()
{
	lightcache.Save();
}

CON_COMMAND( save_lightcache, "" )
{
	SaveLightIntensity();
}

