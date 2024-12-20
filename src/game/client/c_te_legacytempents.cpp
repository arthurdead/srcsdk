//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "model_types.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "tempentity.h"
#include "dlight.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "clientsideeffects.h"
#include "iefx.h"
#include "engine/IEngineSound.h"
#include "env_wind_shared.h"
#include "clienteffectprecachesystem.h"
#include "fx_sparks.h"
#include "fx.h"
#include "movevars_shared.h"
#include "engine/ivmodelinfo.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "view.h"
#include "tier0/vprof.h"
#include "particles_localspace.h"
#include "physpropclientside.h"
#include "tier0/icommandline.h"
#include "datacache/imdlcache.h"
#include "engine/ivdebugoverlay.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "c_props.h"
#include "c_basedoor.h"

// NOTE: Always include this last!
#include "tier0/memdbgon.h"

extern ConVar muzzleflash_light;

#define TENT_WIND_ACCEL 50

//Precache the effects
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectMuzzleFlash )

	CLIENTEFFECT_MATERIAL( "effects/muzzleflash1" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash2" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash3" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash4" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash1_noz" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash2_noz" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash3_noz" )
	CLIENTEFFECT_MATERIAL( "effects/muzzleflash4_noz" )

CLIENTEFFECT_REGISTER_END()

//Whether or not to eject brass from weapons
ConVar cl_ejectbrass( "cl_ejectbrass", "1" );

ConVar func_break_max_pieces( "func_break_max_pieces", "15", FCVAR_ARCHIVE | FCVAR_REPLICATED );

ConVar cl_fasttempentcollision( "cl_fasttempentcollision", "5" );

// Temp entity interface
static CTempEnts g_TempEnts;
// Expose to rest of the client .dll
ITempEnts *tempents = ( ITempEnts * )&g_TempEnts;




C_LocalTempEntity::C_LocalTempEntity()
{
#ifdef _DEBUG
	tentOffset.Init();
	m_vecTempEntVelocity.Init();
	m_vecTempEntAngVelocity.Init();
	m_vecNormal.Init();
#endif
	m_vecTempEntAcceleration.Init();
	m_pfnDrawHelper = 0;
	m_pszImpactEffect = NULL;
}


#define TE_RIFLE_SHELL 1024
#define TE_PISTOL_SHELL 2048
#define TE_SHOTGUN_SHELL 4096

//-----------------------------------------------------------------------------
// Purpose: Prepare a temp entity for creation
// Input  : time - 
//			*model - 
//-----------------------------------------------------------------------------
void C_LocalTempEntity::Prepare( const model_t *pmodel, float time )
{
	// Use these to set per-frame and termination conditions / actions
	flags = FTENT_NONE;		
	die = time + 0.75;
	SetModelPointer( pmodel );
	SetRenderMode( kRenderNormal );
	SetRenderFX( kRenderFxNone );
	SetBody( 0 );
	SetSkin( 0 ); 
	fadeSpeed = 0.5;
	hitSound = 0;
	clientIndex = -1;
	bounceFactor = 1;
	m_nFlickerFrame = 0;
	m_bParticleCollision = false;
}

//-----------------------------------------------------------------------------
// Sets the velocity
//-----------------------------------------------------------------------------
void C_LocalTempEntity::SetVelocity( const Vector &vecVelocity )
{
	m_vecTempEntVelocity = vecVelocity;
}

//-----------------------------------------------------------------------------
// Sets the velocity
//-----------------------------------------------------------------------------
void C_LocalTempEntity::SetAcceleration( const Vector &vecVelocity )
{
	m_vecTempEntAcceleration = vecVelocity;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_LocalTempEntity::DrawStudioModel( int flags, const RenderableInstance_t &instance )
{
	VPROF_BUDGET( "C_LocalTempEntity::DrawStudioModel", VPROF_BUDGETGROUP_MODEL_RENDERING );
	int drawn = 0;

	if ( !GetModel() || modelinfo->GetModelType( GetModel() ) != mod_studio )
		return drawn;
	
	// Make sure m_pstudiohdr is valid for drawing
	MDLCACHE_CRITICAL_SECTION();
	if ( !GetModelPtr() )
		return drawn;

	if ( m_pfnDrawHelper )
	{
		drawn = ( *m_pfnDrawHelper )( this, flags, instance );
	}
	else
	{
		drawn = modelrender->DrawModel( 
			flags, 
			this,
			MODEL_INSTANCE_INVALID,
			entindex(), 
			GetModel(),
			GetAbsOrigin(),
			GetAbsAngles(),
			GetSkin(),
			GetBody(),
			m_nHitboxSet );
	}
	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
//-----------------------------------------------------------------------------
int	C_LocalTempEntity::DrawModel( int flags, const RenderableInstance_t &instance )
{
	int drawn = 0;

	if ( !GetModel() )
	{
		return drawn;
	}

	if ( GetRenderMode() == kRenderNone )
		return drawn;

	if ( this->flags & FTENT_BEOCCLUDED )
	{
		// Check normal
		Vector vecDelta = (GetAbsOrigin() - MainViewOrigin());
		VectorNormalize( vecDelta );
		float flDot = DotProduct( m_vecNormal, vecDelta );
		if ( flDot > 0 )
		{
			float flAlpha = RemapVal( MIN(flDot,0.3), 0, 0.3, 0, 1 );
			flAlpha = MAX( 1.0, tempent_renderamt - (tempent_renderamt * flAlpha) );
			SetRenderAlpha( flAlpha );
		}
	}

	switch ( modelinfo->GetModelType( GetModel() ) )
	{
	case mod_sprite:
		drawn = DrawSprite( 
			this,
			GetModel(), 
			GetAbsOrigin(), 
			GetAbsAngles(), 
			m_flFrame,  // sprite frame to render
			GetBody() > 0 ? cl_entitylist->GetBaseEntity( GetBody() ) : NULL,  // attach to
			GetSkin(),  // attachment point
			GetRenderMode(), // rendermode
			GetRenderFX(), // renderfx
			GetRenderAlpha(), // alpha
			GetRenderColorR(),
			GetRenderColorG(),
			GetRenderColorB(),
			m_flSpriteScale		  // sprite scale
			);
		break;
	case mod_studio:
		drawn = DrawStudioModel( flags, instance );
		break;
	default:
		break;
	}

	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_LocalTempEntity::IsActive( void )
{
	bool active = true;

	float life = die - gpGlobals->curtime;
	
	if ( life < 0 )
	{
		if ( flags & FTENT_FADEOUT )
		{
			int alpha;
			if (GetRenderMode() == kRenderNormal)
			{
				SetRenderMode( kRenderTransTexture );
			}
			
			alpha = tempent_renderamt * ( 1 + life * fadeSpeed );
			
			if ( alpha <= 0 )
			{
				active = false;
				alpha = 0;
			}

			SetRenderAlpha( alpha );
		}
		else 
		{
			active = false;
		}
	}

	// Never die tempents only die when their die is cleared
	if ( flags & FTENT_NEVERDIE )
	{
		active = (die != 0);
	}

	return active;
}

bool C_LocalTempEntity::Frame( float frametime, int framenumber )
{
	float fastFreq = gpGlobals->curtime * 5.5;
	float gravity = -frametime * GetCurrentGravity();
	float gravitySlow = gravity * 0.5;
	float traceFraction = 1;

	Assert( !GetMoveParent() );

	m_vecPrevLocalOrigin = GetLocalOrigin();

	m_vecTempEntVelocity = m_vecTempEntVelocity + ( m_vecTempEntAcceleration * frametime );

	if ( flags & FTENT_PLYRATTACHMENT )
	{
		if ( IClientEntity *pClient = cl_entitylist->GetClientEntity( clientIndex ) )
		{
			SetLocalOrigin( pClient->GetAbsOrigin() + tentOffset );
		}
	}
	else if ( flags & FTENT_SINEWAVE )
	{
		x += m_vecTempEntVelocity[0] * frametime;
		y += m_vecTempEntVelocity[1] * frametime;

		SetLocalOrigin( Vector(
			x + sin( m_vecTempEntVelocity[2] + gpGlobals->curtime /* * anim.prevframe */ ) * (10*m_flSpriteScale),
			y + sin( m_vecTempEntVelocity[2] + fastFreq + 0.7 ) * (8*m_flSpriteScale),
			GetLocalOriginDim( Z_INDEX ) + m_vecTempEntVelocity[2] * frametime ) );
	}
	else if ( flags & FTENT_SPIRAL )
	{
		float s, c;
		SinCos( m_vecTempEntVelocity[2] + fastFreq, &s, &c );

		SetLocalOrigin( GetLocalOrigin() + Vector(
			m_vecTempEntVelocity[0] * frametime + 8 * sin( gpGlobals->curtime * 20 ),
			m_vecTempEntVelocity[1] * frametime + 4 * sin( gpGlobals->curtime * 30 ),
			m_vecTempEntVelocity[2] * frametime ) );
	}
	else
	{
		SetLocalOrigin( GetLocalOrigin() + m_vecTempEntVelocity * frametime );
	}
	
	if ( flags & FTENT_SPRANIMATE )
	{
		m_flFrame += frametime * m_flFrameRate;
		if ( m_flFrame >= m_flFrameMax )
		{
			m_flFrame = m_flFrame - (int)(m_flFrame);

			if ( !(flags & FTENT_SPRANIMATELOOP) )
			{
				// this animating sprite isn't set to loop, so destroy it.
				die = 0.0f;
				return false;
			}
		}
	}
	else if ( flags & FTENT_SPRCYCLE )
	{
		m_flFrame += frametime * 10;
		if ( m_flFrame >= m_flFrameMax )
		{
			m_flFrame = m_flFrame - (int)(m_flFrame);
		}
	}

	if ( flags & FTENT_SMOKEGROWANDFADE )
	{
		m_flSpriteScale += frametime * 0.5f;
		//m_clrRender.a -= frametime * 1500;
	}

	if ( flags & FTENT_ROTATE )
	{
		SetLocalAngles( GetLocalAngles() + m_vecTempEntAngVelocity * frametime );
	}
	else if ( flags & FTENT_ALIGNTOMOTION )
	{
		if ( m_vecTempEntVelocity.Length() > 0.0f )
		{
			QAngle angles;
			VectorAngles( m_vecTempEntVelocity, angles );
			SetAbsAngles( angles );
		}
	}

	if ( flags & (FTENT_COLLIDEALL | FTENT_COLLIDEWORLD | FTENT_COLLIDEPROPS ) )
	{
		Vector	traceNormal;
		traceNormal.Init();
		bool bShouldCollide = true;

		trace_t trace;

		if ( flags & (FTENT_COLLIDEALL | FTENT_COLLIDEPROPS) )
		{
			Vector vPrevOrigin = m_vecPrevLocalOrigin;

			if ( cl_fasttempentcollision.GetInt() > 0 && flags & FTENT_USEFASTCOLLISIONS )
			{
				if ( m_iLastCollisionFrame + cl_fasttempentcollision.GetInt() > gpGlobals->framecount )
				{
					bShouldCollide = false;
				}
				else
				{
					if ( m_vLastCollisionOrigin != vec3_origin )
					{
						vPrevOrigin = m_vLastCollisionOrigin;
					}

					m_iLastCollisionFrame = gpGlobals->framecount;
					bShouldCollide = true; 
				}
			}

			if ( bShouldCollide == true )
			{
				// If the FTENT_COLLISIONGROUP flag is set, use the entity's collision group
				int collisionGroup = COLLISION_GROUP_NONE;
				if ( flags & FTENT_COLLISIONGROUP )
				{
					collisionGroup = GetCollisionGroup();
				}

				UTIL_TraceLine( vPrevOrigin, GetLocalOrigin(), MASK_SOLID, GetOwnerEntity(), collisionGroup, &trace );

				if ( (flags & FTENT_COLLIDEPROPS) && trace.m_pEnt )
				{
					bool bIsDynamicProp = ( NULL != dynamic_cast<C_DynamicProp *>( trace.m_pEnt ) );
					bool bIsDoor = ( NULL != dynamic_cast<C_BaseDoor *>( trace.m_pEnt ) );
					if ( !bIsDynamicProp && !bIsDoor && !trace.m_pEnt->IsWorld() ) // Die on props, doors, and the world.
						return true;
				}

				// Make sure it didn't bump into itself... (?!?)
				if  ( 
					(trace.fraction != 1) && 
						( (trace.DidHitWorld()) || 
						  (trace.m_pEnt != ClientEntityList().GetEnt(clientIndex)) ) 
					)
				{
					traceFraction = trace.fraction;
					VectorCopy( trace.plane.normal, traceNormal );
				}

				m_vLastCollisionOrigin = trace.endpos;
			}
		}
		else if ( flags & FTENT_COLLIDEWORLD )
		{
			CTraceFilterWorldOnly traceFilter;
			UTIL_TraceLine( m_vecPrevLocalOrigin, GetLocalOrigin(), MASK_SOLID, &traceFilter, &trace );
			if ( trace.fraction != 1 )
			{
				traceFraction = trace.fraction;
				VectorCopy( trace.plane.normal, traceNormal );
			}
		}
		
		if ( traceFraction != 1  )	// Decent collision now, and damping works
		{
			float  proj, damp;
			SetLocalOrigin( trace.endpos );
			
			// Damp velocity
			damp = bounceFactor;
			if ( flags & (FTENT_GRAVITY|FTENT_SLOWGRAVITY) )
			{
				damp *= 0.5;
				if ( traceNormal[2] > 0.9 )		// Hit floor?
				{
					if ( m_vecTempEntVelocity[2] <= 0 && m_vecTempEntVelocity[2] >= gravity*3 )
					{
						damp = 0;		// Stop
						flags &= ~(FTENT_ROTATE|FTENT_GRAVITY|FTENT_SLOWGRAVITY|FTENT_COLLIDEWORLD|FTENT_SMOKETRAIL);
						SetLocalAnglesDim( X_INDEX, 0 );
						SetLocalAnglesDim( Z_INDEX, 0 );
					}
				}
			}

			if ( flags & (FTENT_CHANGERENDERONCOLLIDE) )
			{
				OnTranslucencyTypeChanged();
				flags &= ~FTENT_CHANGERENDERONCOLLIDE;
			}	

			if (hitSound)
			{
				tempents->PlaySound(this, damp);
			}

			if ( m_pszImpactEffect )
			{
				CEffectData data;
				//data.m_vOrigin = newOrigin;
				data.m_vOrigin = trace.endpos;
				data.m_vStart = trace.startpos;
				data.m_nSurfaceProp = trace.surface.surfaceProps;
				data.m_nHitBox = trace.hitbox;

				data.m_nDamageType = TEAM_UNASSIGNED;

				IClientNetworkable *pClient = cl_entitylist->GetClientEntity( clientIndex );

				if ( pClient )
				{
					C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer*>(pClient);
					if( pPlayer )
					{
						data.m_nDamageType = pPlayer->GetTeamNumber();
					}
				}

				if ( trace.m_pEnt )
				{
					data.m_hEntity = ClientEntityList().EntIndexToHandle( trace.m_pEnt->entindex() );
				}
				DispatchEffect( m_pszImpactEffect, data );
			}

			// Check for a collision and stop the particle system.
			if ( flags & FTENT_CLIENTSIDEPARTICLES )
			{
				// Stop the emission of particles on collision - removed from the ClientEntityList on removal from the tempent pool.
				ParticleProp()->StopEmission();
				m_bParticleCollision = true;
			}

			if (flags & FTENT_COLLIDEKILL)
			{
				// die on impact
				flags &= ~FTENT_FADEOUT;	
				die = gpGlobals->curtime;			
			}
			else if ( flags & FTENT_ATTACHTOTARGET)
			{
				// If we've hit the world, just stop moving
				if ( trace.DidHitWorld() && !( trace.surface.flags & SURF_SKY ) )
				{
					m_vecTempEntVelocity = vec3_origin;
					m_vecTempEntAcceleration = vec3_origin;

					// Remove movement flags so we don't keep tracing
					flags &= ~(FTENT_COLLIDEALL | FTENT_COLLIDEWORLD);
				}
				else
				{
					// Couldn't attach to this entity. Die.
					flags &= ~FTENT_FADEOUT;
					die = gpGlobals->curtime;
				}
			}
			else
			{
				// Reflect velocity
				if ( damp != 0 )
				{
					proj = ((Vector)m_vecTempEntVelocity).Dot(traceNormal);
					VectorMA( m_vecTempEntVelocity, -proj*2, traceNormal, m_vecTempEntVelocity );
					// Reflect rotation (fake)
					SetLocalAnglesDim( Y_INDEX, -GetLocalAnglesDim( Y_INDEX ) );
				}
				
				if ( damp != 1 )
				{
					VectorScale( m_vecTempEntVelocity, damp, m_vecTempEntVelocity );
					SetLocalAngles( GetLocalAngles() * 0.9 );
				}
			}
		}
	}


	if ( (flags & FTENT_FLICKER) && framenumber == m_nFlickerFrame )
	{
		dlight_t *dl = effects->CL_AllocDlight (LIGHT_INDEX_TE_DYNAMIC);
		VectorCopy (GetLocalOrigin(), dl->origin);
		dl->radius = 60;
		dl->color.SetColor( 255, 120, 0 );
		dl->die = gpGlobals->curtime + 0.01;
	}

	if ( flags & FTENT_SMOKETRAIL )
	{
		 Assert( !"FIXME:  Rework smoketrail to be client side\n" );
	}

	// add gravity if we didn't collide in this frame
	if ( traceFraction == 1 )
	{
		if ( flags & FTENT_GRAVITY )
			m_vecTempEntVelocity[2] += gravity;
		else if ( flags & FTENT_SLOWGRAVITY )
			m_vecTempEntVelocity[2] += gravitySlow;
	}

	if ( flags & FTENT_WINDBLOWN )
	{
		Vector vecWind = GetWindspeedAtLocation( GetAbsOrigin() );

		for ( int i = 0 ; i < 2 ; i++ )
		{
			if ( m_vecTempEntVelocity[i] < vecWind[i] )
			{
				m_vecTempEntVelocity[i] += ( frametime * TENT_WIND_ACCEL );

				// clamp
				if ( m_vecTempEntVelocity[i] > vecWind[i] )
					m_vecTempEntVelocity[i] = vecWind[i];
			}
			else if (m_vecTempEntVelocity[i] > vecWind[i] )
			{
				m_vecTempEntVelocity[i] -= ( frametime * TENT_WIND_ACCEL );

				// clamp.
				if ( m_vecTempEntVelocity[i] < vecWind[i] )
					m_vecTempEntVelocity[i] = vecWind[i];
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Attach a particle effect to a temp entity.
//-----------------------------------------------------------------------------
CNewParticleEffect* C_LocalTempEntity::AddParticleEffect( const char *pszParticleEffect )
{
	// Do we have a valid particle effect.
	if ( !pszParticleEffect || ( pszParticleEffect[0] == '\0' ) )
		return NULL;

	// Check to see that we don't already have a particle effect.
	if ( ( flags & FTENT_CLIENTSIDEPARTICLES ) != 0 )
		return NULL;

	// Add the entity to the ClientEntityList and create the particle system.
	ClientEntityList().AddNonNetworkableEntity( this );
	CNewParticleEffect* pEffect = ParticleProp()->Create( pszParticleEffect, PATTACH_ABSORIGIN_FOLLOW );

	// Set the particle flag on the temp entity and save the name of the particle effect.
	flags |= FTENT_CLIENTSIDEPARTICLES;
	SetParticleEffect( pszParticleEffect );

	return pEffect;
}

//-----------------------------------------------------------------------------
// Purpose: This helper keeps track of batches of "breakmodels" so that they can all share the lighting origin
//  of the first of the group (because the server sends down 15 chunks at a time, and rebuilding 15 light cache
//  entries for a map with a lot of worldlights is really slow).
//-----------------------------------------------------------------------------
class CBreakableHelper
{
public:
	void	Insert( C_LocalTempEntity *entity, bool isSlave );
	void	Remove( C_LocalTempEntity *entity );

	void	Clear();

	const Vector *GetLightingOrigin( C_LocalTempEntity *entity );

private:

	// A context is the first master until the next one, which starts a new context
	struct BreakableList_t
	{
		unsigned int		context;
		C_LocalTempEntity	*entity;
	};

	CUtlLinkedList< BreakableList_t, unsigned short >	m_Breakables;
	unsigned int			m_nCurrentContext;
};

//-----------------------------------------------------------------------------
// Purpose: Adds brekable to list, starts new context if needed
// Input  : *entity - 
//			isSlave - 
//-----------------------------------------------------------------------------
void CBreakableHelper::Insert( C_LocalTempEntity *entity, bool isSlave )
{
	// A master signifies the start of a new run of broken objects
	if ( !isSlave )
	{
		++m_nCurrentContext;
	}
	
	BreakableList_t entry;
	entry.context = m_nCurrentContext;
	entry.entity = entity;

	m_Breakables.AddToTail( entry );
}

//-----------------------------------------------------------------------------
// Purpose: Removes all instances of entity in the list
// Input  : *entity - 
//-----------------------------------------------------------------------------
void CBreakableHelper::Remove( C_LocalTempEntity *entity )
{
	for ( unsigned short i = m_Breakables.Head(); i != m_Breakables.InvalidIndex() ; )
	{
		unsigned short n = m_Breakables.Next( i );

		if ( m_Breakables[ i ].entity == entity )
		{
			m_Breakables.Remove( i );
		}

		i = n;
	}
}

//-----------------------------------------------------------------------------
// Purpose: For a given breakable, find the "first" or head object and use it's current
//  origin as the lighting origin for the entire group of objects
// Input  : *entity - 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector *CBreakableHelper::GetLightingOrigin( C_LocalTempEntity *entity )
{
	unsigned int nCurContext = 0;
	C_LocalTempEntity *head = NULL;
	FOR_EACH_LL( m_Breakables, i )
	{
		BreakableList_t& e = m_Breakables[ i ];

		if ( e.context != nCurContext )
		{
			nCurContext = e.context;
			head = e.entity;
		}

		if ( e.entity == entity )
		{
			Assert( head );
			return head ? &head->GetAbsOrigin() : NULL;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Wipe breakable helper list
// Input  :  - 
//-----------------------------------------------------------------------------
void CBreakableHelper::Clear()
{
	m_Breakables.RemoveAll();
	m_nCurrentContext = 0;
}

static CBreakableHelper g_BreakableHelper;

//-----------------------------------------------------------------------------
// Purpose: See if it's in the breakable helper list and, if so, remove
// Input  :  - 
//-----------------------------------------------------------------------------
void C_LocalTempEntity::OnRemoveTempEntity()
{
	g_BreakableHelper.Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTempEnts::CTempEnts( void ) :
	m_TempEntsPool( ( MAX_TEMP_ENTITIES / 20 ), CUtlMemoryPool::GROW_SLOW )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTempEnts::~CTempEnts( void )
{
	m_TempEntsPool.Clear();
	m_TempEnts.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Create a fizz effect
// Input  : *pent - 
//			modelIndex - 
//			density - 
//-----------------------------------------------------------------------------
void CTempEnts::FizzEffect( C_BaseEntity *pent, modelindex_t modelIndex, int density, int current )
{
	C_LocalTempEntity		*pTemp;
	const model_t	*model;
	int				i, width, depth, count, frameCount;
	float			maxHeight, speed, xspeed, yspeed;
	Vector			origin;
	Vector			mins, maxs;

	if ( !pent->GetModel() || !IsValidModelIndex(modelIndex) ) 
		return;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
		return;

	count = density + 1;
	density = count * 3 + 6;

	modelinfo->GetModelBounds( pent->GetModel(), mins, maxs );

	maxHeight = maxs[2] - mins[2];
	width = maxs[0] - mins[0];
	depth = maxs[1] - mins[1];
	speed = current;

	SinCos( pent->GetLocalAngles()[1]*M_PI/180, &yspeed, &xspeed );
	xspeed *= speed;
	yspeed *= speed;
	frameCount = modelinfo->GetModelFrameCount( model );

	for (i=0 ; i<count ; i++)
	{
		origin[0] = mins[0] + random_valve->RandomInt(0,width-1);
		origin[1] = mins[1] + random_valve->RandomInt(0,depth-1);
		origin[2] = mins[2];
		pTemp = TempEntAlloc( origin, model, "te_legacy_fizz" );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];

		float zspeed = random_valve->RandomInt(80,140);
		pTemp->SetVelocity( Vector(xspeed, yspeed, zspeed) );
		pTemp->die = gpGlobals->curtime + (maxHeight / zspeed) - 0.1;
		pTemp->m_flFrame = random_valve->RandomInt(0,frameCount-1);
		// Set sprite scale
		pTemp->m_flSpriteScale = 1.0 / random_valve->RandomFloat(2,5);
		pTemp->SetRenderMode( kRenderTransAlpha );
		pTemp->SetRenderAlpha( 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create bubbles
// Input  : *mins - 
//			*maxs - 
//			height - 
//			modelIndex - 
//			count - 
//			speed - 
//-----------------------------------------------------------------------------
void CTempEnts::Bubbles( const Vector &mins, const Vector &maxs, float height, modelindex_t modelIndex, int count, float speed )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*model;
	int					i, frameCount;
	float				sine, cosine;
	Vector				origin;

	if ( !IsValidModelIndex(modelIndex) ) 
		return;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
		return;

	frameCount = modelinfo->GetModelFrameCount( model );

	for (i=0 ; i<count ; i++)
	{
		origin[0] = random_valve->RandomInt( mins[0], maxs[0] );
		origin[1] = random_valve->RandomInt( mins[1], maxs[1] );
		origin[2] = random_valve->RandomInt( mins[2], maxs[2] );
		pTemp = TempEntAlloc( origin, model, "te_legacy_bubbles" );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		SinCos( random_valve->RandomInt( -M_PI, M_PI ), &sine, &cosine );
		
		float zspeed = random_valve->RandomInt(80,140);
		pTemp->SetVelocity( Vector(speed * cosine, speed * sine, zspeed) );
		pTemp->die = gpGlobals->curtime + ((height - (origin[2] - mins[2])) / zspeed) - 0.1;
		pTemp->m_flFrame = random_valve->RandomInt( 0, frameCount-1 );
		
		// Set sprite scale
		pTemp->m_flSpriteScale = 1.0 / random_valve->RandomFloat(4,16);
		pTemp->SetRenderMode( kRenderTransAlpha );
		
		pTemp->SetRenderColor( 255, 255, 255 );
		pTemp->SetRenderAlpha( 192 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create bubble trail
// Input  : *start - 
//			*end - 
//			height - 
//			modelIndex - 
//			count - 
//			speed - 
//-----------------------------------------------------------------------------
void CTempEnts::BubbleTrail( const Vector &start, const Vector &end, float flWaterZ, modelindex_t modelIndex, int count, float speed )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*model;
	int					i, frameCount;
	float				dist, angle;
	Vector				origin;

	if ( !IsValidModelIndex(modelIndex) ) 
		return;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
		return;

	frameCount = modelinfo->GetModelFrameCount( model );

	for (i=0 ; i<count ; i++)
	{
		dist = random_valve->RandomFloat( 0, 1.0 );
		VectorLerp( start, end, dist, origin );
		pTemp = TempEntAlloc( origin, model, "te_legacy_bubblestrail" );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_SINEWAVE;

		pTemp->x = origin[0];
		pTemp->y = origin[1];
		angle = random_valve->RandomInt( -M_PI, M_PI );

		float zspeed = random_valve->RandomInt(80,140);
		pTemp->SetVelocity( Vector(speed * cos(angle), speed * sin(angle), zspeed) );
		pTemp->die = gpGlobals->curtime + ((flWaterZ - origin[2]) / zspeed) - 0.1;
		pTemp->m_flFrame = random_valve->RandomInt(0,frameCount-1);
		// Set sprite scale
		pTemp->m_flSpriteScale = 1.0 / random_valve->RandomFloat(4,8);
		pTemp->SetRenderMode( kRenderTransAlpha );
		
		pTemp->SetRenderColor( 255, 255, 255 );
		pTemp->SetRenderAlpha( 192 );
	}
}

#define SHARD_VOLUME 12.0	// on shard ever n^3 units

//-----------------------------------------------------------------------------
// Purpose: Only used by BreakModel temp ents for now.  Allows them to share a single
//  lighting origin amongst a group of objects.  If the master object goes away, the next object
//  in the group becomes the new lighting origin, etc.
// Input  : *entity - 
//			flags - 
// Output : int
//-----------------------------------------------------------------------------
int BreakModelDrawHelper( C_LocalTempEntity *entity, int flags, const RenderableInstance_t &instance )
{
	ModelRenderInfo_t sInfo;
	sInfo.flags = flags;
	sInfo.pRenderable = entity;
	sInfo.instance = MODEL_INSTANCE_INVALID;
	sInfo.entity_index = entity->entindex();
	sInfo.pModel = entity->GetModel();
	sInfo.origin = entity->GetRenderOrigin();
	sInfo.angles = entity->GetRenderAngles();
	sInfo.skin = entity->GetSkin();
	sInfo.body = entity->GetBody();
	sInfo.hitboxset = entity->m_nHitboxSet;

	// This is the main change, look up a lighting origin from the helper singleton
	const Vector *pLightingOrigin = g_BreakableHelper.GetLightingOrigin( entity );
	if ( pLightingOrigin )
	{
		sInfo.pLightingOrigin = pLightingOrigin;
	}

	int drawn = modelrender->DrawModelEx( sInfo );
	return drawn;
}

//-----------------------------------------------------------------------------
// Purpose: Create model shattering shards
// Input  : *pos - 
//			*size - 
//			*dir - 
//			random - 
//			life - 
//			count - 
//			modelIndex - 
//			flags - 
//-----------------------------------------------------------------------------
void CTempEnts::BreakModel( const Vector &pos, const QAngle &angles, const Vector &size, const Vector &dir, 
						   float randRange, float life, int count, modelindex_t modelIndex, char flags)
{
	int					i, frameCount;
	C_LocalTempEntity			*pTemp;
	const model_t		*pModel;

	if (!IsValidModelIndex(modelIndex)) 
		return;

	pModel = modelinfo->GetModel( modelIndex );
	if ( !pModel )
		return;

	// See g_BreakableHelper above for notes...
	bool isSlave = ( flags & BREAK_SLAVE ) ? true : false;

	frameCount = modelinfo->GetModelFrameCount( pModel );

	if (count == 0)
	{
		// assume surface (not volume)
		count = (size[0] * size[1] + size[1] * size[2] + size[2] * size[0])/(3 * SHARD_VOLUME * SHARD_VOLUME);
	}

	if ( count > func_break_max_pieces.GetInt() )
	{
		count = func_break_max_pieces.GetInt();
	}

	matrix3x4_t transform;
	AngleMatrix( angles, pos, transform );
	for ( i = 0; i < count; i++ ) 
	{
		Vector vecLocalSpot, vecSpot;

		// fill up the box with stuff
		vecLocalSpot[0] = random_valve->RandomFloat(-0.5,0.5) * size[0];
		vecLocalSpot[1] = random_valve->RandomFloat(-0.5,0.5) * size[1];
		vecLocalSpot[2] = random_valve->RandomFloat(-0.5,0.5) * size[2];
		VectorTransform( vecLocalSpot, transform, vecSpot );

		pTemp = TempEntAlloc(vecSpot, pModel, "te_legacy_breakableprop");
		
		if (!pTemp)
			return;

		// keep track of break_type, so we know how to play sound on collision
		pTemp->hitSound = flags;
		
		if ( modelinfo->GetModelType( pModel ) == mod_sprite )
		{
			pTemp->m_flFrame = random_valve->RandomInt(0,frameCount-1);
		}
		else if ( modelinfo->GetModelType( pModel ) == mod_studio )
		{
			pTemp->SetBody( random_valve->RandomInt(0,frameCount-1) );
		}

		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_SLOWGRAVITY;

		if ( random_valve->RandomInt(0,255) < 200 ) 
		{
			pTemp->flags |= FTENT_ROTATE;
			pTemp->m_vecTempEntAngVelocity[0] = random_valve->RandomFloat(-256,255);
			pTemp->m_vecTempEntAngVelocity[1] = random_valve->RandomFloat(-256,255);
			pTemp->m_vecTempEntAngVelocity[2] = random_valve->RandomFloat(-256,255);
		}

		if ( (random_valve->RandomInt(0,255) < 100 ) && (flags & BREAK_SMOKE) )
		{
			pTemp->flags |= FTENT_SMOKETRAIL;
		}

		if ((flags & BREAK_GLASS) || (flags & BREAK_TRANS))
		{
			pTemp->SetRenderMode( kRenderTransTexture );
			pTemp->SetRenderAlpha( 128 );
			pTemp->tempent_renderamt = 128;
			pTemp->bounceFactor = 0.3f;
		}
		else
		{
			pTemp->SetRenderMode( kRenderNormal );
			pTemp->tempent_renderamt = 255;		// Set this for fadeout
		}

		pTemp->SetVelocity( Vector( dir[0] + random_valve->RandomFloat(-randRange,randRange),
							dir[1] + random_valve->RandomFloat(-randRange,randRange),
							dir[2] + random_valve->RandomFloat(   0,randRange) ) );

		pTemp->die = gpGlobals->curtime + life + random_valve->RandomFloat(0,1);	// Add an extra 0-1 secs of life

		// We use a special rendering function because these objects will want to share their lighting
		//  origin with the first/master object.  Prevents a huge spike in Light Cache building in maps
		//  with many worldlights.
		pTemp->SetDrawHelper( BreakModelDrawHelper );
		g_BreakableHelper.Insert( pTemp, isSlave );
	}
}

void CTempEnts::PhysicsProp( modelindex_t modelindex, int skin, const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects )
{
	C_PhysPropClientside *pEntity = C_PhysPropClientside::CreateNew();
	
	if ( !pEntity )
		return;

	const model_t *model = modelinfo->GetModel( modelindex );

	if ( !model )
	{
		DevMsg("CTempEnts::PhysicsProp: model index %i not found\n", (int)modelindex );
		return;
	}

	pEntity->SetModelName( MAKE_STRING( modelinfo->GetModelName(model) ) );
	pEntity->SetSkin( skin );
	pEntity->SetAbsOrigin( pos );
	pEntity->SetAbsAngles( angles );
	pEntity->SetPhysicsMode( PHYSICS_CLIENTSIDE );
	pEntity->SetEffects( effects );

	pEntity->SetModelIndex( modelindex );
	pEntity->SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );
	pEntity->SetAbsVelocity( vel );
	pEntity->Spawn();

	if ( !pEntity->Initialize() )
	{
		UTIL_Remove( pEntity );
		return;
	}

	IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();

	if( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &vel, NULL );
	}
	else
	{
		// failed to create a physics object
		UTIL_Remove( pEntity );
		return;
	}

	if ( flags & 2 )
	{
		int numBodygroups = pEntity->GetBodygroupCount( 0 );
		pEntity->SetBodygroup( 0, RandomInt( 0, numBodygroups - 1 ) );
	}

	if ( flags & 1 )
	{
		pEntity->SetHealth( 0 );
		pEntity->Break();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a clientside projectile
// Input  : vecOrigin - 
//			vecVelocity - 
//			modelindex - 
//			lifetime - 
//			*pOwner - 
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::ClientProjectile( const Vector& vecOrigin, const Vector& vecVelocity, const Vector& vecAcceleration, modelindex_t modelIndex, int lifetime, C_BaseEntity *pOwner, const char *pszImpactEffect, const char *pszParticleEffect )
{
	C_LocalTempEntity	*pTemp;
	const model_t		*model;

	if ( !IsValidModelIndex(modelIndex) ) 
		return NULL;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
	{
		Warning("ClientProjectile: No model %d!\n", (int)modelIndex);
		return NULL;
	}

	pTemp = TempEntAlloc( vecOrigin, model, "te_legacy_projectile" );
	if (!pTemp)
		return NULL;

	pTemp->SetVelocity( vecVelocity );
	pTemp->SetAcceleration( vecAcceleration );
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pTemp->SetAbsAngles( angles );
	pTemp->SetAbsOrigin( vecOrigin );
	pTemp->die = gpGlobals->curtime + lifetime;
	pTemp->flags = FTENT_COLLIDEALL | FTENT_ATTACHTOTARGET | FTENT_ALIGNTOMOTION;
	pTemp->clientIndex = ( pOwner != NULL ) ? pOwner->entindex() : 0; 
	pTemp->SetOwnerEntity( pOwner );
	pTemp->SetImpactEffect( pszImpactEffect );
	if ( pszParticleEffect )
	{
		// Add the entity to the ClientEntityList and create the particle system.
		ClientEntityList().AddNonNetworkableEntity( pTemp );
		pTemp->ParticleProp()->Create( pszParticleEffect, PATTACH_ABSORIGIN_FOLLOW );

		// Set the particle flag on the temp entity and save the name of the particle effect.
		pTemp->flags |= FTENT_CLIENTSIDEPARTICLES;
	 	pTemp->SetParticleEffect( pszParticleEffect );
	}
	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Create sprite TE
// Input  : *pos - 
//			*dir - 
//			scale - 
//			modelIndex - 
//			rendermode - 
//			renderfx - 
//			a - 
//			life - 
//			flags - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempSprite( const Vector &pos, const Vector &dir, float scale, modelindex_t modelIndex, int rendermode, int renderfx, float a, float life, int flags, const Vector &normal )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*model;
	int					frameCount;

	if ( !IsValidModelIndex(modelIndex) ) 
		return NULL;

	model = modelinfo->GetModel( modelIndex );
	if ( !model )
	{
		Warning("No model %d!\n", (int)modelIndex);
		return NULL;
	}

	frameCount = modelinfo->GetModelFrameCount( model );

	pTemp = TempEntAlloc( pos, model, "te_legacy_sprite" );
	if (!pTemp)
		return NULL;

	pTemp->m_flFrameMax = frameCount - 1;
	pTemp->m_flFrameRate = 10;
	pTemp->SetRenderMode( (RenderMode_t)rendermode );
	pTemp->SetRenderFX( (RenderFx_t)renderfx );
	pTemp->m_flSpriteScale = scale;
	pTemp->tempent_renderamt = a * 255;
	pTemp->m_vecNormal = normal;
	pTemp->SetRenderColor( 255, 255, 255 );
	pTemp->SetRenderAlpha( a * 255 );

	pTemp->flags |= flags;

	pTemp->SetVelocity( dir );
	pTemp->SetLocalOrigin( pos );
	if ( life )
		pTemp->die = gpGlobals->curtime + life;
	else
		pTemp->die = gpGlobals->curtime + (frameCount * 0.1) + 1;

	pTemp->m_flFrame = 0;
	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Spray sprite
// Input  : *pos - 
//			*dir - 
//			modelIndex - 
//			count - 
//			speed - 
//			iRand - 
//-----------------------------------------------------------------------------
void CTempEnts::Sprite_Spray( const Vector &pos, const Vector &dir, modelindex_t modelIndex, int count, int speed, int iRand )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*pModel;
	float				noise;
	float				znoise;
	int					frameCount;
	int					i;

	noise = (float)iRand / 100;

	// more vertical displacement
	znoise = noise * 1.5;
	
	if ( znoise > 1 )
	{
		znoise = 1;
	}

	pModel = modelinfo->GetModel( modelIndex );
	
	if ( !pModel )
	{
		Warning("No model %d!\n", (int)modelIndex);
		return;
	}

	frameCount = modelinfo->GetModelFrameCount( pModel ) - 1;

	for ( i = 0; i < count; i++ )
	{
		pTemp = TempEntAlloc( pos, pModel, "te_legacy_spritespray" );
		if (!pTemp)
			return;

		pTemp->SetRenderMode( kRenderTransAlpha );
		pTemp->SetRenderColor( 255, 255, 255 );
		pTemp->SetRenderAlpha( 255 );
		pTemp->tempent_renderamt = 255;
		pTemp->SetRenderFX( kRenderFxNoDissipation );
		//pTemp->scale = random_valve->RandomFloat( 0.1, 0.25 );
		pTemp->m_flSpriteScale = 0.5;
		pTemp->flags |= FTENT_FADEOUT | FTENT_SLOWGRAVITY;
		pTemp->fadeSpeed = 2.0;

		// make the spittle fly the direction indicated, but mix in some noise.
		Vector velocity;
		velocity.x = dir[ 0 ] + random_valve->RandomFloat ( -noise, noise );
		velocity.y = dir[ 1 ] + random_valve->RandomFloat ( -noise, noise );
		velocity.z = dir[ 2 ] + random_valve->RandomFloat ( 0, znoise );
		velocity *= random_valve->RandomFloat( (speed * 0.8), (speed * 1.2) );
		pTemp->SetVelocity( velocity );

		pTemp->SetLocalOrigin( pos );

		pTemp->die = gpGlobals->curtime + 0.35;

		pTemp->m_flFrame = random_valve->RandomInt( 0, frameCount );
	}
}

void CTempEnts::Sprite_Trail( const Vector &vecStart, const Vector &vecEnd, modelindex_t modelIndex, int nCount, float flLife, float flSize, float flAmplitude, int nRenderamt, float flSpeed )
{
	C_LocalTempEntity	*pTemp;
	const model_t		*pModel;
	int					flFrameCount;

	pModel = modelinfo->GetModel( modelIndex );
	
	if ( !pModel )
	{
		Warning("No model %d!\n", (int)modelIndex);
		return;
	}

	flFrameCount = modelinfo->GetModelFrameCount( pModel );

	Vector vecDelta;
	VectorSubtract( vecEnd, vecStart, vecDelta );

	Vector vecDir;
	VectorCopy( vecDelta, vecDir );
	VectorNormalize( vecDir );

	flAmplitude /= 256.0;

	for ( int i = 0 ; i < nCount; i++ )
	{
		Vector vecPos;

		// Be careful of divide by 0 when using 'count' here...
		if ( i == 0 )
		{
			VectorMA( vecStart, 0, vecDelta, vecPos );
		}
		else
		{
			VectorMA( vecStart, i / (nCount - 1.0), vecDelta, vecPos );
		}

		pTemp = TempEntAlloc( vecPos, pModel, "te_legacy_spritetrail" );
		if (!pTemp)
			return;

		pTemp->flags |= FTENT_COLLIDEWORLD | FTENT_SPRCYCLE | FTENT_FADEOUT | FTENT_SLOWGRAVITY;

		Vector vecVel = vecDir * flSpeed;
		vecVel.x += random_valve->RandomInt( -127,128 ) * flAmplitude;
		vecVel.y += random_valve->RandomInt( -127,128 ) * flAmplitude;
		vecVel.z += random_valve->RandomInt( -127,128 ) * flAmplitude;
		pTemp->SetVelocity( vecVel );
		pTemp->SetLocalOrigin( vecPos );

		pTemp->m_flSpriteScale		= flSize;
		pTemp->SetRenderMode( kRenderGlow );
		pTemp->SetRenderFX( kRenderFxNoDissipation );
		pTemp->tempent_renderamt	= nRenderamt;
		pTemp->SetRenderColor( 255, 255, 255 );

		pTemp->m_flFrame	= random_valve->RandomInt( 0, flFrameCount - 1 );
		pTemp->m_flFrameMax	= flFrameCount - 1;
		pTemp->die			= gpGlobals->curtime + flLife + random_valve->RandomFloat( 0, 4 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attaches entity to player
// Input  : client - 
//			modelIndex - 
//			zoffset - 
//			life - 
//-----------------------------------------------------------------------------
void CTempEnts::AttachTentToPlayer( int client, modelindex_t modelIndex, float zoffset, float life )
{
	C_LocalTempEntity			*pTemp;
	const model_t		*pModel;
	Vector				position;
	int					frameCount;

	if ( client <= 0 || client > gpGlobals->maxClients )
	{
		Warning("Bad client in AttachTentToPlayer()!\n");
		return;
	}

	IClientEntity *clientClass = cl_entitylist->GetClientEntity( client );
	if ( !clientClass )
	{
		Warning("Couldn't get IClientEntity for %i\n", client );
		return;
	}

	pModel = modelinfo->GetModel( modelIndex );
	
	if ( !pModel )
	{
		Warning("No model %d!\n", (int)modelIndex);
		return;
	}

	VectorCopy( clientClass->GetAbsOrigin(), position );
	position[ 2 ] += zoffset;

	pTemp = TempEntAllocHigh( position, pModel, "te_legacy_attachprop" );
	if (!pTemp)
	{
		Warning("No temp ent.\n");
		return;
	}

	pTemp->SetRenderMode( kRenderNormal );
	pTemp->SetRenderAlpha( 255 );
	pTemp->tempent_renderamt = 255;
	pTemp->SetRenderFX( kRenderFxNoDissipation );
	
	pTemp->clientIndex = client;
	pTemp->tentOffset[ 0 ] = 0;
	pTemp->tentOffset[ 1 ] = 0;
	pTemp->tentOffset[ 2 ] = zoffset;
	pTemp->die = gpGlobals->curtime + life;
	pTemp->flags |= FTENT_PLYRATTACHMENT | FTENT_PERSIST;

	// is the model a sprite?
	if ( modelinfo->GetModelType( pTemp->GetModel() ) == mod_sprite )
	{
		frameCount = modelinfo->GetModelFrameCount( pModel );
		pTemp->m_flFrameMax = frameCount - 1;
		pTemp->flags |= FTENT_SPRANIMATE | FTENT_SPRANIMATELOOP;
		pTemp->m_flFrameRate = 10;
	}
	else
	{
		// no animation support for attached clientside studio models.
		pTemp->m_flFrameMax = 0;
	}

	pTemp->m_flFrame = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Detach entity from player
//-----------------------------------------------------------------------------
void CTempEnts::KillAttachedTents( int client )
{
	if ( client <= 0 || client > gpGlobals->maxClients )
	{
		Warning("Bad client in KillAttachedTents()!\n");
		return;
	}

	FOR_EACH_LL( m_TempEnts, i )
	{
		C_LocalTempEntity *pTemp = m_TempEnts[ i ];

		if ( pTemp->flags & FTENT_PLYRATTACHMENT )
		{
			// this TENT is player attached.
			// if it is attached to this client, set it to die instantly.
			if ( pTemp->clientIndex == client )
			{
				pTemp->die = gpGlobals->curtime;// good enough, it will die on next tent update. 
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: Create ricochet sprite
// Input  : *pos - 
//			*pmodel - 
//			duration - 
//			scale - 
//-----------------------------------------------------------------------------
void CTempEnts::RicochetSprite( const Vector &pos, model_t *pmodel, float duration, float scale )
{
	C_LocalTempEntity	*pTemp;

	pTemp = TempEntAlloc( pos, pmodel, "te_legacy_ricochetsprite" );
	if (!pTemp)
		return;

	pTemp->SetRenderMode( kRenderGlow );
	pTemp->SetRenderFX( kRenderFxNoDissipation );
	pTemp->SetRenderAlpha( 200 );
	pTemp->tempent_renderamt = 200;
	pTemp->m_flSpriteScale = scale;
	pTemp->flags = FTENT_FADEOUT;

	pTemp->SetVelocity( vec3_origin );

	pTemp->SetLocalOrigin( pos );
	
	pTemp->fadeSpeed = 8;
	pTemp->die = gpGlobals->curtime;

	pTemp->m_flFrame = 0;
	pTemp->SetLocalAnglesDim( Z_INDEX, 45 * random_valve->RandomInt( 0, 7 ) );
}

//-----------------------------------------------------------------------------
// Purpose: Create blood sprite
// Input  : *org - 
//			colorindex - 
//			modelIndex - 
//			modelIndex2 - 
//			size - 
//-----------------------------------------------------------------------------
void CTempEnts::BloodSprite( const Vector &org, color32 clr, modelindex_t modelIndex, modelindex_t modelIndex2, float size )
{
	const model_t			*model;

	//Validate the model first
	if ( IsValidModelIndex(modelIndex) && (model = modelinfo->GetModel( modelIndex ) ) != NULL )
	{
		C_LocalTempEntity		*pTemp;
		int						frameCount = modelinfo->GetModelFrameCount( model );

		//Large, single blood sprite is a high-priority tent
		if ( ( pTemp = TempEntAllocHigh( org, model, "te_legacy_bloodsprite" ) ) != NULL )
		{
			pTemp->SetRenderMode( kRenderTransTexture );
			pTemp->SetRenderFX( kRenderFxNone );
			pTemp->m_flSpriteScale	= random_valve->RandomFloat( size / 25, size / 35);
			pTemp->flags			= FTENT_SPRANIMATE;
 
			pTemp->SetRenderColor( clr.r(), clr.g(), clr.b() );
			pTemp->SetRenderAlpha( clr.a() );
			pTemp->tempent_renderamt= pTemp->GetRenderAlpha();

			pTemp->SetVelocity( vec3_origin );

			pTemp->m_flFrameRate	= frameCount * 4; // Finish in 0.250 seconds
			pTemp->die				= gpGlobals->curtime + (frameCount / pTemp->m_flFrameRate); // Play the whole thing Once

			pTemp->m_flFrame		= 0;
			pTemp->m_flFrameMax		= frameCount - 1;
			pTemp->bounceFactor		= 0;
			pTemp->SetLocalAnglesDim( Z_INDEX, random_valve->RandomInt( 0, 360 ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create default sprite TE
// Input  : *pos - 
//			spriteIndex - 
//			framerate - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::DefaultSprite( const Vector &pos, modelindex_t spriteIndex, float framerate )
{
	C_LocalTempEntity		*pTemp;
	int				frameCount;
	const model_t	*pSprite;

	// don't spawn while paused
	if ( gpGlobals->frametime == 0.0 )
		return NULL;

	pSprite = modelinfo->GetModel( spriteIndex );
	if ( !IsValidModelIndex(spriteIndex) || !pSprite || modelinfo->GetModelType( pSprite ) != mod_sprite )
	{
		DevWarning( 1,"No Sprite %d!\n", (int)spriteIndex);
		return NULL;
	}

	frameCount = modelinfo->GetModelFrameCount( pSprite );

	pTemp = TempEntAlloc( pos, pSprite, "te_legacy_sprite" );
	if (!pTemp)
		return NULL;

	pTemp->m_flFrameMax = frameCount - 1;
	pTemp->m_flSpriteScale = 1.0;
	pTemp->flags |= FTENT_SPRANIMATE;
	if ( framerate == 0 )
		framerate = 10;

	pTemp->m_flFrameRate = framerate;
	pTemp->die = gpGlobals->curtime + (float)frameCount / framerate;
	pTemp->m_flFrame = 0;
	pTemp->SetLocalOrigin( pos );

	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Create sprite smoke
// Input  : *pTemp - 
//			scale - 
//-----------------------------------------------------------------------------
void CTempEnts::Sprite_Smoke( C_LocalTempEntity *pTemp, float scale )
{
	if ( !pTemp )
		return;

	pTemp->SetRenderMode( kRenderTransAlpha );
	pTemp->SetRenderFX( kRenderFxNone );
	pTemp->SetVelocity( Vector( 0, 0, 30 ) );
	int iColor = random_valve->RandomInt(20,35);
	pTemp->SetRenderColor( iColor, iColor, iColor );
	pTemp->SetRenderAlpha( 255 );
	pTemp->SetLocalOriginDim( Z_INDEX, pTemp->GetLocalOriginDim( Z_INDEX ) + 20 );
	pTemp->m_flSpriteScale = scale;
	pTemp->flags = FTENT_WINDBLOWN;

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pos1 - 
//			angles - 
//			type - 
//-----------------------------------------------------------------------------
void CTempEnts::EjectBrass( const Vector &pos1, const QAngle &angles, const QAngle &gunAngles, int type, C_BasePlayer *pShooter )
{
	if ( cl_ejectbrass.GetBool() == false )
		return;

	const model_t *pModel = m_pShells[type];
	
	if ( pModel == NULL )
		return;

	C_LocalTempEntity	*pTemp = TempEntAlloc( pos1, pModel, "te_legacy_brasseject" );

	if ( pTemp == NULL )
		return;

	//Keep track of shell type
	if ( type == 2 )
	{
		pTemp->hitSound = BOUNCE_SHOTSHELL;
	}
	else
	{
		pTemp->hitSound = BOUNCE_SHELL;
	}

	pTemp->SetBody( 0 );

	pTemp->flags |= ( FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_GRAVITY | FTENT_ROTATE );

	pTemp->m_vecTempEntAngVelocity[0] = random_valve->RandomFloat(-1024,1024);
	pTemp->m_vecTempEntAngVelocity[1] = random_valve->RandomFloat(-1024,1024);
	pTemp->m_vecTempEntAngVelocity[2] = random_valve->RandomFloat(-1024,1024);

	//Face forward
	pTemp->SetAbsAngles( gunAngles );

	pTemp->SetRenderMode( kRenderNormal );
	pTemp->tempent_renderamt = 255;		// Set this for fadeout

	Vector	dir;

	AngleVectors( angles, &dir );

	dir *= random_valve->RandomFloat( 150.0f, 200.0f );

	pTemp->SetVelocity( Vector(dir[0] + random_valve->RandomFloat(-64,64),
						dir[1] + random_valve->RandomFloat(-64,64),
						dir[2] + random_valve->RandomFloat(  0,64) ) );

	pTemp->die = gpGlobals->curtime + 1.0f + random_valve->RandomFloat( 0.0f, 1.0f );	// Add an extra 0-1 secs of life	
}

//-----------------------------------------------------------------------------
// Purpose: Create some simple physically simulated models
//-----------------------------------------------------------------------------
C_LocalTempEntity * CTempEnts::SpawnTempModel( const model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags )
{
	Assert( pModel );

	// Alloc a new tempent
	C_LocalTempEntity *pTemp = TempEntAlloc( vecOrigin, pModel, "te_legacy_physicsprop" );
	if ( !pTemp )
		return NULL;

	pTemp->SetAbsAngles( vecAngles );
	pTemp->SetBody( 0 );
	pTemp->flags |= iFlags;
	pTemp->m_vecTempEntAngVelocity[0] = random_valve->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[1] = random_valve->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[2] = random_valve->RandomFloat(-255,255);
	pTemp->SetRenderMode( kRenderNormal );
	pTemp->tempent_renderamt = 255;
	pTemp->SetVelocity( vecVelocity );
	pTemp->die = gpGlobals->curtime + flLifeTime;

	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			entityIndex - 
//			attachmentIndex - 
//			firstPerson - 
//-----------------------------------------------------------------------------
void CTempEnts::MuzzleFlash( int type, ClientEntityHandle_t hEntity, int attachmentIndex, bool firstPerson )
{
	switch( type )
	{
	case MUZZLEFLASH_SMG1:
	case MUZZLEFLASH_PISTOL:
		if(firstPerson)
			MuzzleFlash_Pistol_Firstperson( hEntity, attachmentIndex );
		else
			MuzzleFlash_Pistol_Thirdperson( hEntity, attachmentIndex );
		break;
	case MUZZLEFLASH_SHOTGUN:
		if(firstPerson)
			MuzzleFlash_Shotgun_Firstperson( hEntity, attachmentIndex );
		else
			MuzzleFlash_Shotgun_Thirdperson( hEntity, attachmentIndex );
		break;
	case MUZZLEFLASH_357:
		if(firstPerson)
			MuzzleFlash_357_Firstperson( hEntity, attachmentIndex );
		break;
	case MUZZLEFLASH_RPG:
		if(!firstPerson)
			MuzzleFlash_RPG_Thirdperson( hEntity, attachmentIndex );
		break;
	default:
		{
			//NOTENOTE: This means you specified an invalid muzzleflash type, check your spelling?
			Assert( 0 );
		}
		break;
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Shotgun_Firstperson( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Shotgun_Firstperson", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash_Shotgun_Firstperson" );

	pSimple->SetDrawBeforeViewModel( true );

	CacheMuzzleFlashes();

	Vector origin;
	QAngle angles;

	// Get our attachment's transformation matrix
	FX_GetAttachmentTransform( hEntity, attachmentIndex, &origin, &angles );

	pSimple->GetBinding().SetBBox( origin - Vector( 4, 4, 4 ), origin + Vector( 4, 4, 4 ) );

	Vector forward;
	AngleVectors( angles, &forward, NULL, NULL );

	SimpleParticle *pParticle;
	Vector offset;

	float flScale = random_valve->RandomFloat( 1.25f, 1.5f );

	// Flash
	for ( int i = 1; i < 6; i++ )
	{
		offset = origin + (forward * (i*8.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_MuzzleFlash[random_valve->RandomInt(0,3)][0], offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.0001f;

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor.SetColor( 255, 255, 200+random_valve->RandomInt(0,55) );

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 255;

		pParticle->m_uchStartSize	= ( (random_valve->RandomFloat( 6.0f, 8.0f ) * (8-(i))/6) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random_valve->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Shotgun_Thirdperson( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	//Draw the cloud of fire
	FX_MuzzleEffectAttached( 0.75f, hEntity, attachmentIndex );

	// If the material isn't available, let's not do anything else.
	if ( m_Material_MuzzleFlash[0][1] == NULL )
	{
		return;
	}

	QAngle	angles;

	Vector	forward;

	// Setup the origin.
	Vector	origin;
	IClientRenderable *pRenderable = ClientEntityList().GetClientRenderableFromHandle( hEntity );
	if ( !pRenderable )
		return;

	pRenderable->GetAttachment( attachmentIndex, origin, angles );
	AngleVectors( angles, &forward );

	//Embers less often
	if ( random_valve->RandomInt( 0, 2 ) == 0 )
	{
		//Embers
		CSmartPtr<CEmberEffect> pEmbers = CEmberEffect::Create( "muzzle_embers" );
		pEmbers->SetSortOrigin( origin );

		SimpleParticle	*pParticle;

		int	numEmbers = random_valve->RandomInt( 0, 4 );

		for ( int i = 0; i < numEmbers; i++ )
		{
			pParticle = (SimpleParticle *) pEmbers->AddParticle( sizeof( SimpleParticle ), m_Material_MuzzleFlash[0][1], origin );
				
			if ( pParticle == NULL )
				return;

			pParticle->m_flLifetime		= 0.0f;
			pParticle->m_flDieTime		= random_valve->RandomFloat( 0.2f, 0.4f );

			pParticle->m_vecVelocity.Random( -0.05f, 0.05f );
			pParticle->m_vecVelocity += forward;
			VectorNormalize( pParticle->m_vecVelocity );

			pParticle->m_vecVelocity	*= random_valve->RandomFloat( 64.0f, 256.0f );

			pParticle->m_uchColor.SetColor( 255, 128, 64 );

			pParticle->m_uchStartAlpha	= 255;
			pParticle->m_uchEndAlpha	= 0;

			pParticle->m_uchStartSize	= 1;
			pParticle->m_uchEndSize		= 0;

			pParticle->m_flRoll			= 0;
			pParticle->m_flRollDelta	= 0;
		}
	}

	//
	// Trails
	//
	
	CSmartPtr<CTrailParticles> pTrails = CTrailParticles::Create( "MuzzleFlash_Shotgun_Thirdperson" );
	pTrails->SetSortOrigin( origin );

	TrailParticle	*pTrailParticle;

	pTrails->SetFlag( bitsPARTICLE_TRAIL_FADE );
	pTrails->m_ParticleCollision.SetGravity( 0.0f );

	int	numEmbers = random_valve->RandomInt( 4, 8 );

	for ( int i = 0; i < numEmbers; i++ )
	{
		pTrailParticle = (TrailParticle *) pTrails->AddParticle( sizeof( TrailParticle ), m_Material_MuzzleFlash[0][1], origin );
			
		if ( pTrailParticle == NULL )
			return;

		pTrailParticle->m_flLifetime		= 0.0f;
		pTrailParticle->m_flDieTime		= random_valve->RandomFloat( 0.1f, 0.2f );

		float spread = 0.05f;

		pTrailParticle->m_vecVelocity.Random( -spread, spread );
		pTrailParticle->m_vecVelocity += forward;
		
		VectorNormalize( pTrailParticle->m_vecVelocity );
		VectorNormalize( forward );

		float dot = forward.Dot( pTrailParticle->m_vecVelocity );

		dot = (1.0f-fabs(dot)) / spread;
		pTrailParticle->m_vecVelocity *= (random_valve->RandomFloat( 256.0f, 1024.0f ) * (1.0f-dot));

		Color32Init( pTrailParticle->m_color, 255, 242, 191, 255 );

		pTrailParticle->m_flLength	= 0.05f;
		pTrailParticle->m_flWidth	= random_valve->RandomFloat( 0.25f, 0.5f );
	}
}

//==================================================
// Purpose: 
//==================================================
void CTempEnts::MuzzleFlash_357_Firstperson( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_357_Firstperson", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash_357_Firstperson" );

	pSimple->SetDrawBeforeViewModel( true );

	CacheMuzzleFlashes();

	Vector origin;
	QAngle angles;

	// Get our attachment's transformation matrix
	FX_GetAttachmentTransform( hEntity, attachmentIndex, &origin, &angles );

	pSimple->GetBinding().SetBBox( origin - Vector( 4, 4, 4 ), origin + Vector( 4, 4, 4 ) );

	Vector forward;
	AngleVectors( angles, &forward, NULL, NULL );

	SimpleParticle *pParticle;
	Vector			offset;

	// Smoke
	offset = origin + forward * 8.0f;

	pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], offset );
		
	if ( pParticle == NULL )
		return;

	pParticle->m_flLifetime		= 0.0f;
	pParticle->m_flDieTime		= random_valve->RandomFloat( 0.5f, 1.0f );

	pParticle->m_vecVelocity.Init();
	pParticle->m_vecVelocity = forward * random_valve->RandomFloat( 8.0f, 64.0f );
	pParticle->m_vecVelocity[2] += random_valve->RandomFloat( 4.0f, 16.0f );

	unsigned char color = random_valve->RandomInt( 200, 255 );
	pParticle->m_uchColor.SetColor( color, color, color );

	pParticle->m_uchStartAlpha	= random_valve->RandomInt( 64, 128 );
	pParticle->m_uchEndAlpha	= 0;

	pParticle->m_uchStartSize	= random_valve->RandomInt( 2, 4 );
	pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 8.0f;
	pParticle->m_flRoll			= random_valve->RandomInt( 0, 360 );
	pParticle->m_flRollDelta	= random_valve->RandomFloat( -0.5f, 0.5f );

	float flScale = random_valve->RandomFloat( 1.25f, 1.5f );

	// Flash
	for ( int i = 1; i < 6; i++ )
	{
		offset = origin + (forward * (i*8.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_MuzzleFlash[random_valve->RandomInt(0,3)][0], offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.01f;

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor.SetColor( 255, 255, 200+random_valve->RandomInt(0,55) );

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 255;

		pParticle->m_uchStartSize	= ( (random_valve->RandomFloat( 6.0f, 8.0f ) * (8-(i))/6) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random_valve->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Pistol_Firstperson( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	VPROF_BUDGET( "MuzzleFlash_Pistol_Firstperson", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
	CSmartPtr<CSimpleEmitter> pSimple = CSimpleEmitter::Create( "MuzzleFlash_Pistol_Firstperson" );
	pSimple->SetDrawBeforeViewModel( true );

	CacheMuzzleFlashes();

	Vector origin;
	QAngle angles;

	// Get our attachment's transformation matrix
	FX_GetAttachmentTransform( hEntity, attachmentIndex, &origin, &angles );

	pSimple->GetBinding().SetBBox( origin - Vector( 4, 4, 4 ), origin + Vector( 4, 4, 4 ) );

	Vector forward;
	AngleVectors( angles, &forward, NULL, NULL );

	SimpleParticle *pParticle;
	Vector			offset;

	// Smoke
	offset = origin + forward * 8.0f;

	if ( random_valve->RandomInt( 0, 3 ) != 0 )
	{
		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), g_Mat_DustPuff[0], offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= random_valve->RandomFloat( 0.25f, 0.5f );

		pParticle->m_vecVelocity.Init();
		pParticle->m_vecVelocity = forward * random_valve->RandomFloat( 48.0f, 64.0f );
		pParticle->m_vecVelocity[2] += random_valve->RandomFloat( 4.0f, 16.0f );

		unsigned char color = random_valve->RandomInt( 200, 255 );
		pParticle->m_uchColor.SetColor( color, color, color );

		pParticle->m_uchStartAlpha	= random_valve->RandomInt( 64, 128 );
		pParticle->m_uchEndAlpha	= 0;

		pParticle->m_uchStartSize	= random_valve->RandomInt( 2, 4 );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 4.0f;
		pParticle->m_flRoll			= random_valve->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random_valve->RandomFloat( -0.1f, 0.1f );
	}

	float flScale = random_valve->RandomFloat( 1.0f, 1.25f );

	// Flash
	for ( int i = 1; i < 6; i++ )
	{
		offset = origin + (forward * (i*4.0f*flScale));

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), m_Material_MuzzleFlash[random_valve->RandomInt(0,3)][0], offset );
			
		if ( pParticle == NULL )
			return;

		pParticle->m_flLifetime		= 0.0f;
		pParticle->m_flDieTime		= 0.01f;

		pParticle->m_vecVelocity.Init();

		pParticle->m_uchColor.SetColor( 255, 255, 200+random_valve->RandomInt(0,55) );

		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 255;

		pParticle->m_uchStartSize	= ( (random_valve->RandomFloat( 6.0f, 8.0f ) * (8-(i))/6) * flScale );
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize;
		pParticle->m_flRoll			= random_valve->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= 0.0f;
	}
}

//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_Pistol_Thirdperson( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	FX_MuzzleEffectAttached( 0.5f, hEntity, attachmentIndex, NULL, true );
}




//==================================================
// Purpose: 
// Input: 
//==================================================

void CTempEnts::MuzzleFlash_RPG_Thirdperson( ClientEntityHandle_t hEntity, int attachmentIndex )
{
	//Draw the cloud of fire
	FX_MuzzleEffectAttached( 1.5f, hEntity, attachmentIndex );

}

//-----------------------------------------------------------------------------
// Purpose: Create explosion sprite
// Input  : *pTemp - 
//			scale - 
//			flags - 
//-----------------------------------------------------------------------------
void CTempEnts::Sprite_Explode( C_LocalTempEntity *pTemp, float scale, int flags )
{
	if ( !pTemp )
		return;

	if ( flags & TE_EXPLFLAG_NOADDITIVE )
	{
		// solid sprite
		pTemp->SetRenderMode( kRenderNormal );
		pTemp->SetRenderAlpha( 255 ); 
	}
	else if( flags & TE_EXPLFLAG_DRAWALPHA )
	{
		// alpha sprite
		pTemp->SetRenderMode( kRenderTransAlpha ); 
		pTemp->SetRenderAlpha( 180 );
	}
	else
	{
		// additive sprite
		pTemp->SetRenderMode( kRenderTransAdd );
		pTemp->SetRenderAlpha( 180 );
	}

	if ( flags & TE_EXPLFLAG_ROTATE )
	{
		pTemp->SetLocalAnglesDim( Z_INDEX, random_valve->RandomInt( 0, 360 ) );
	}

	pTemp->SetRenderFX( kRenderFxNone );
	pTemp->SetVelocity( Vector( 0, 0, 8 ) );
	pTemp->SetRenderColor( 255, 255, 255 );
	pTemp->SetLocalOriginDim( Z_INDEX, pTemp->GetLocalOriginDim( Z_INDEX ) + 10 );
	pTemp->m_flSpriteScale = scale;
}

//-----------------------------------------------------------------------------
// Purpose: Clear existing temp entities
//-----------------------------------------------------------------------------
void CTempEnts::Clear( void )
{
	FOR_EACH_LL( m_TempEnts, i )
	{
		C_LocalTempEntity *p = m_TempEnts[ i ];

		m_TempEntsPool.Free( p );
	}

	m_TempEnts.RemoveAll();
	g_BreakableHelper.Clear();
}

C_LocalTempEntity *CTempEnts::FindTempEntByID( int nID, int nSubID )
{
	// HACK HACK: We're using skin and hitsounds as a hacky way to store an ID and sub-ID for later identification
	FOR_EACH_LL( m_TempEnts, i )
	{
		C_LocalTempEntity *p = m_TempEnts[ i ];
		if ( p && p->GetSkin() == nID && p->hitSound == nSubID )
		{
			return p;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate temp entity ( normal/low priority )
// Input  : *org - 
//			*model - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempEntAlloc( const Vector& org, const model_t *model, const char *classname )
{
	C_LocalTempEntity		*pTemp;

	if ( !model )
	{
		DevWarning( 1, "Can't create temporary entity with NULL model!\n" );
		return NULL;
	}

	pTemp = TempEntAlloc(classname);

	if ( !pTemp )
	{
		DevWarning( 1, "Overflow %d temporary ents!\n", MAX_TEMP_ENTITIES );
		return NULL;
	}

	m_TempEnts.AddToTail( pTemp );

	pTemp->Prepare( model, gpGlobals->curtime );

	pTemp->priority = TENTPRIORITY_LOW;
	pTemp->SetAbsOrigin( org );

	return pTemp;
}

void					C_LocalTempEntity::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	OnRemoveTempEntity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempEntAlloc( const char *classname )
{
	if ( m_TempEnts.Count() >= MAX_TEMP_ENTITIES )
		return NULL;

	MEM_ALLOC_CREDIT();
	C_LocalTempEntity *pTemp = m_TempEntsPool.AllocZero();
	pTemp->PostConstructor( classname );
	return pTemp;
}

void CTempEnts::TempEntFree( int index )
{
	C_LocalTempEntity *pTemp = m_TempEnts[ index ];
	if ( pTemp )
	{
		// Remove from the active list.
		m_TempEnts.Remove( index );

		// Remove the tempent from the ClientEntityList before removing it from the pool.
		if ( ( pTemp->flags & FTENT_CLIENTSIDEPARTICLES ) )
		{			
			// Stop the particle emission if this hasn't happened already - collision or system timing out on its own.
			if ( !pTemp->m_bParticleCollision )
			{
				pTemp->ParticleProp()->StopEmission();
			}
		}

		UTIL_Remove( pTemp );

		m_TempEntsPool.Free( pTemp );
	}
}


// Free the first low priority tempent it finds.
bool CTempEnts::FreeLowPriorityTempEnt()
{
	int next = 0;
	for( int i = m_TempEnts.Head(); i != m_TempEnts.InvalidIndex(); i = next )
	{
		next = m_TempEnts.Next( i );

		C_LocalTempEntity *pActive = m_TempEnts[ i ];

		if ( pActive->priority == TENTPRIORITY_LOW )
		{
			TempEntFree( i );
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Allocate a temp entity, if there are no slots, kick out a low priority
//  one if possible
// Input  : *org - 
//			*model - 
// Output : C_LocalTempEntity
//-----------------------------------------------------------------------------
C_LocalTempEntity *CTempEnts::TempEntAllocHigh( const Vector& org, const model_t *model, const char *classname )
{
	C_LocalTempEntity		*pTemp;

	if ( !model )
	{
		DevWarning( 1, "temporary ent model invalid\n" );
		return NULL;
	}

	pTemp = TempEntAlloc( classname );
	if ( !pTemp )
	{
		// no temporary ents free, so find the first active low-priority temp ent 
		// and overwrite it.
		FreeLowPriorityTempEnt();

		pTemp = TempEntAlloc( classname );
	}

	
	if ( !pTemp )
	{
		// didn't find anything? The tent list is either full of high-priority tents
		// or all tents in the list are still due to live for > 10 seconds. 
		DevWarning( 1,"Couldn't alloc a high priority TENT (max %i)!\n", MAX_TEMP_ENTITIES );
		return NULL;
	}

	m_TempEnts.AddToTail( pTemp );

	pTemp->Prepare( model, gpGlobals->curtime );

	pTemp->priority = TENTPRIORITY_HIGH;
	pTemp->SetLocalOrigin( org );

	return pTemp;
}

//-----------------------------------------------------------------------------
// Purpose: Play sound when temp ent collides with something
// Input  : *pTemp - 
//			damp - 
//-----------------------------------------------------------------------------
void CTempEnts::PlaySound ( C_LocalTempEntity *pTemp, float damp )
{
	const char	*soundname = NULL;
	float fvol;
	bool isshellcasing = false;
	int zvel;

	switch ( pTemp->hitSound )
	{
	default:
		return;	// null sound

	case BOUNCE_GLASS:
		{
			soundname = "Bounce.Glass";
		}
		break;

	case BOUNCE_METAL:
		{
			soundname = "Bounce.Metal";
		}
		break;

	case BOUNCE_FLESH:
		{
			soundname = "Bounce.Flesh";
		}
		break;

	case BOUNCE_WOOD:
		{
			soundname = "Bounce.Wood";
		}
		break;

	case BOUNCE_SHRAP:
		{
			soundname = "Bounce.Shrapnel";
		}
		break;

	case BOUNCE_SHOTSHELL:
		{
			soundname = "Bounce.ShotgunShell";
			isshellcasing = true; // shell casings have different playback parameters
		}
		break;

	case BOUNCE_SHELL:
		{
			soundname = "Bounce.Shell";
			isshellcasing = true; // shell casings have different playback parameters
		}
		break;

	case BOUNCE_CONCRETE:
		{
			soundname = "Bounce.Concrete";
		}
		break;

	case TE_PISTOL_SHELL:
		{
			soundname = "Bounce.PistolShell";
		}
		break;

	case TE_RIFLE_SHELL:
		{
			soundname = "Bounce.RifleShell";
		}
		break;

	case TE_SHOTGUN_SHELL:
		{
			soundname = "Bounce.ShotgunShell";
		}
		break;
	}

	zvel = abs( pTemp->GetVelocity()[2] );
		
	// only play one out of every n

	if ( isshellcasing )
	{	
		// play first bounce, then 1 out of 3		
		if ( zvel < 200 && random_valve->RandomInt(0,3) )
			return;
	}
	else
	{
		if ( random_valve->RandomInt(0,5) ) 
			return;
	}

	CSoundParameters params;
	if ( !C_BaseEntity::GetParametersForSound( soundname, params, NULL ) )
		return;

	fvol = params.volume;

	if ( damp > 0.0 )
	{
		int pitch;
		
		if ( isshellcasing )
		{
			fvol *= MIN (1.0, ((float)zvel) / 350.0); 
		}
		else
		{
			fvol *= MIN (1.0, ((float)zvel) / 450.0); 
		}
		
		if ( !random_valve->RandomInt(0,3) && !isshellcasing )
		{
			pitch = random_valve->RandomInt( params.pitchlow, params.pitchhigh );
		}
		else
		{
			pitch = params.pitch;
		}

		CLocalPlayerFilter filter;

		EmitSound_t ep;
		ep.m_nChannel = params.channel;
		ep.m_pSoundName =  params.soundname;
		ep.m_flVolume = fvol;
		ep.m_SoundLevel = params.soundlevel;
		ep.m_nPitch = pitch;
		ep.m_pOrigin = &pTemp->GetAbsOrigin();

		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, ep );
	}
}
					
//-----------------------------------------------------------------------------
// Purpose: Add temp entity to visible entities list of it's in PVS
// Input  : *pEntity - 
// Output : int
//-----------------------------------------------------------------------------
bool CTempEnts::IsVisibleTempEntity( C_LocalTempEntity *pEntity )
{
	int i;
	Vector mins, maxs;
	Vector model_mins, model_maxs;

	if ( !pEntity->GetModel() )
		return false;

	modelinfo->GetModelBounds( pEntity->GetModel(), model_mins, model_maxs );

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = pEntity->GetAbsOrigin()[i] + model_mins[i];
		maxs[i] = pEntity->GetAbsOrigin()[i] + model_maxs[i];
	}

	// does the box intersect a visible leaf?
	if ( engine->IsBoxInViewCluster( mins, maxs ) )
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Runs Temp Ent simulation routines
//-----------------------------------------------------------------------------
void CTempEnts::Update(void)
{
	VPROF_("CTempEnts::Update", 1, VPROF_BUDGETGROUP_CLIENT_SIM, false, BUDGETFLAG_CLIENT);
	static int gTempEntFrame = 0;
	float		frametime;

	// Don't simulate while loading
	if ( ( m_TempEnts.Count() == 0 ) || !engine->IsInGame() )		
	{
		return;
	}

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = (gTempEntFrame+1) & 31;

	frametime = gpGlobals->frametime;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if ( frametime != 0 )
	{
		int next = 0;
		for( int i = m_TempEnts.Head(); i != m_TempEnts.InvalidIndex(); i = next )
		{
			next = m_TempEnts.Next( i );

			C_LocalTempEntity *current = m_TempEnts[ i ];

			// Kill it
			if ( !current->IsActive() || !current->Frame( frametime, gTempEntFrame ) )
			{
				TempEntFree( i );
			}
			else
			{
				// Cull to PVS (not frustum cull, just PVS)
				if ( !IsVisibleTempEntity( current ) )
				{
					if ( !( current->flags & FTENT_PERSIST ) ) 
					{
						// If we can't draw it this frame, just dump it.
						current->die = gpGlobals->curtime;
						// Don't fade out, just die
						current->flags &= ~FTENT_FADEOUT;

						TempEntFree( i );
					}
				}
			}
		}
	}
}

// Recache tempents which might have been flushed
void CTempEnts::LevelInit()
{
	m_pSpriteMuzzleFlash = (model_t *)engine->LoadModel( "sprites/muzzleflash4.vmt" );

	m_pShells[SHELL_GENERIC]		= (model_t *)engine->LoadModel( "models/weapons/shell.mdl" );
	m_pShells[SHELL_SHOTGUN]	= (model_t *)engine->LoadModel( "models/weapons/shotgun_shell.mdl" );
	m_pShells[SHELL_RIFLE]	= (model_t *)engine->LoadModel( "models/weapons/rifleshell.mdl" );
	m_pShells[SHELL_9MM]		= (model_t *)engine->LoadModel( "models/shells/shell_9mm.mdl" );
	m_pShells[SHELL_57]		= (model_t *)engine->LoadModel( "models/shells/shell_57.mdl" );
	m_pShells[SHELL_12GAUGE]	= (model_t *)engine->LoadModel( "models/shells/shell_12gauge.mdl" );
	m_pShells[SHELL_556]		= (model_t *)engine->LoadModel( "models/shells/shell_556.mdl" );
	m_pShells[SHELL_762NATO]	= (model_t *)engine->LoadModel( "models/shells/shell_762nato.mdl" );
	m_pShells[SHELL_338MAG]	= (model_t *)engine->LoadModel( "models/shells/shell_338mag.mdl" );
}


//-----------------------------------------------------------------------------
// Purpose: Initialize TE system
//-----------------------------------------------------------------------------
void CTempEnts::Init (void)
{
	m_pSpriteMuzzleFlash = NULL;

	m_pShells[SHELL_GENERIC]		= NULL;
	m_pShells[SHELL_SHOTGUN]	= NULL;
	m_pShells[SHELL_RIFLE]	= NULL;
	m_pShells[SHELL_9MM]		= NULL;
	m_pShells[SHELL_57]		= NULL;
	m_pShells[SHELL_12GAUGE]	= NULL;
	m_pShells[SHELL_556]		= NULL;
	m_pShells[SHELL_762NATO]	= NULL;
	m_pShells[SHELL_338MAG]	= NULL;

	// Clear out lists to start
	Clear();
}


void CTempEnts::LevelShutdown()
{
	// Free all active tempents.
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTempEnts::Shutdown()
{
	LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Cache off all material references
// Input  : *pEmitter - Emitter used for material lookup
//-----------------------------------------------------------------------------
inline void CTempEnts::CacheMuzzleFlashes( void )
{
	int i;
	for ( i = 0; i < 4; i++ )
	{
		if ( m_Material_MuzzleFlash[i][0] == NULL )
		{
			m_Material_MuzzleFlash[i][0] = ParticleMgr()->GetPMaterial( VarArgs( "effects/muzzleflash%d_noz", i+1 ) );
		}
	}

	for ( i = 0; i < 4; i++ )
	{
		if ( m_Material_MuzzleFlash[i][1] == NULL )
		{
			m_Material_MuzzleFlash[i][1] = ParticleMgr()->GetPMaterial( VarArgs( "effects/muzzleflash%d", i+1 ) );
		}
	}
}

void CTempEnts::RocketFlare( const Vector& pos )
{
	C_LocalTempEntity	*pTemp;
	const model_t		*model;
	int					nframeCount;

	model = (model_t *)engine->LoadModel( "sprites/animglow01.vmt" );
	if ( !model )
	{
		return;
	}

	nframeCount = modelinfo->GetModelFrameCount( model );

	pTemp = TempEntAlloc( pos, model, "te_legacy_rocketflare" );
	if ( !pTemp )
		return;

	pTemp->m_flFrameMax = nframeCount - 1;
	pTemp->SetRenderMode( kRenderGlow );
	pTemp->SetRenderFX( kRenderFxNoDissipation );
	pTemp->tempent_renderamt = 255;
	pTemp->m_flFrameRate = 1.0;
	pTemp->m_flFrame = random_valve->RandomInt( 0, nframeCount - 1);
	pTemp->m_flSpriteScale = 1.0;
	pTemp->SetAbsOrigin( pos );
	pTemp->die = gpGlobals->curtime + 0.01;
}

