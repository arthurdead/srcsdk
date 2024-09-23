#include "cbase.h"
#include "enginecallback.h"
#include "igamesystem.h"
#include "filesystem.h"
#include "tier1/utlbuffer.h"
#include "tier1/generichash.h"
#include "tier1/utlhashtable.h"
#ifndef SWDS
#include "game_loopback/igameloopback.h"
#endif

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

class CLightCacheSystem : CAutoGameSystemPerFrame
{
public:
	static inline const unsigned int magic = 'L'<<24 | 'I'<<16 | 'I'<<8 | 'C';
	static inline const unsigned short version = 1;

	virtual void LevelInitPreEntity()
	{
	#ifndef SWDS
		if(!engine->IsDedicatedServer() && g_pGameClientLoopback) {
			return;
		}
	#endif

		char filename[MAX_PATH];
		V_sprintf_safe(filename, "maps/%s.lightcache", STRING(gpGlobals->mapname));

		CUtlBuffer buf;
		if(!filesystem->ReadFile(filename, "MOD", buf))
			return;

		if(buf.GetUnsignedInt() != magic)
			return;

		if(buf.GetUnsignedShort() != version)
			return;

		unsigned int count = buf.GetUnsignedInt();
		lights.Reserve( count );
		for(int i = 0; i < count; ++i) {
			Vector pos;

			pos.x = buf.GetFloat();
			pos.y = buf.GetFloat();
			pos.z = buf.GetFloat();

			float intensity = buf.GetFloat();

			lights.Insert( pos, Lightcache_t( pos, intensity ) );
		}
	}

	virtual void LevelShutdownPostEntity()
	{
		char filename[MAX_PATH];
		V_sprintf_safe(filename, "maps/%s.lightcache", STRING(gpGlobals->mapname));

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

		filesystem->WriteFile(filename, "MOD", buf);

		lights.Purge();
	}

	CUtlHashtable< Vector, Lightcache_t, LightHashFunctor, LightEqualFunctor > lights;
};
static CLightCacheSystem lightcache;

float GetLightIntensity( const Vector &pos )
{
#ifndef SWDS
	if(!engine->IsDedicatedServer() && g_pGameClientLoopback) {
		Vector light( 0, 0, 0 );
		light = g_pGameClientLoopback->GetLightForPoint( pos, true );

		Vector ambientColor;
		g_pGameClientLoopback->GetAmbientLightColor( ambientColor );

		float ambientIntensity = ambientColor.x + ambientColor.y + ambientColor.z;
		float lightIntensity = light.x + light.y + light.z;
		lightIntensity = clamp( lightIntensity, 0.f, 1.f );	// sum can go well over 1.0, but it's the lower region we care about.  if it's bright, we don't need to know *how* bright.

		lightIntensity = MAX( lightIntensity, ambientIntensity );

		bool found = false;

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
			if(lightcache.lights[it].pos.DistTo(pos) <= 100.0f) {
				return lightcache.lights[it].intensity;
			}
		}
	}

	return 1.0f;
}
