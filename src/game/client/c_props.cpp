//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "c_physicsprop.h"
#include "c_physbox.h"
#include "c_props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( DynamicProp, DT_DynamicProp )

BEGIN_NETWORK_TABLE( C_DynamicProp, DT_DynamicProp )
	RecvPropBool(RECVINFO(m_bUseHitboxesForRenderBox)),
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( prop_dynamic, C_DynamicProp );

C_DynamicProp::C_DynamicProp( void )
{
	m_iCachedFrameCount = -1;
}

C_DynamicProp::~C_DynamicProp( void )
{
}

bool C_DynamicProp::TestBoneFollowers( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	// UNDONE: There is no list of the bone followers that is networked to the client
	// so instead we do a search for solid stuff here.  This is not really great - a list would be
	// preferable.
	C_BaseEntity	*pList[128];
	Vector mins, maxs;
	CollisionProp()->WorldSpaceAABB( &mins, &maxs );
	int count = UTIL_EntitiesInBox( pList, ARRAYSIZE(pList), mins, maxs, 0, PARTITION_CLIENT_SOLID_EDICTS );
	for ( int i = 0; i < count; i++ )
	{
		if ( pList[i]->GetOwnerEntity() == this )
		{
			if ( pList[i]->TestCollision(ray, fContentsMask, tr) )
			{
				return true;
			}
		}
	}
	return false;
}

bool C_DynamicProp::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	if ( IsSolidFlagSet(FSOLID_NOT_SOLID) )
	{
		// if this entity is marked non-solid and custom test it must have bone followers
		if ( IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ) && IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ))
		{
			return TestBoneFollowers( ray, fContentsMask, tr );
		}
	}
	return BaseClass::TestCollision( ray, fContentsMask, tr );
}

//-----------------------------------------------------------------------------
// implements these so ragdolls can handle frustum culling & leaf visibility
//-----------------------------------------------------------------------------
void C_DynamicProp::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	if ( m_bUseHitboxesForRenderBox )
	{
		if ( GetModel() )
		{
			studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( GetModel() );
			if ( !pStudioHdr || GetSequence() == -1 )
			{
				theMins = vec3_origin;
				theMaxs = vec3_origin;
				return;
			}

			// Only recompute if it's a new frame
			if ( gpGlobals->framecount != m_iCachedFrameCount )
			{
				ComputeEntitySpaceHitboxSurroundingBox( &m_vecCachedRenderMins, &m_vecCachedRenderMaxs );
				m_iCachedFrameCount = gpGlobals->framecount;
			}

			theMins = m_vecCachedRenderMins;
			theMaxs = m_vecCachedRenderMaxs;
			return;
		}
	}

	BaseClass::GetRenderBounds( theMins, theMaxs );
}

unsigned int C_DynamicProp::ComputeClientSideAnimationFlags()
{
	if ( GetSequence() != -1 )
	{
		CStudioHdr *pStudioHdr = GetModelPtr();
		if ( GetSequenceCycleRate(pStudioHdr, GetSequence()) != 0.0f )
		{
			return BaseClass::ComputeClientSideAnimationFlags();
		}
	}

	// no sequence or no cycle rate, don't do any per-frame calcs
	return 0;
}

// ------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------ //

IMPLEMENT_CLIENTCLASS_DT(C_BasePropDoor, DT_BasePropDoor, CBasePropDoor)
END_RECV_TABLE()

C_BasePropDoor::C_BasePropDoor( void )
{
	m_modelChanged = false;
}

C_BasePropDoor::~C_BasePropDoor( void )
{
}

void C_BasePropDoor::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		BaseClass::PostDataUpdate( updateType );
	}
	else
	{
		const model_t *oldModel = GetModel();
		BaseClass::PostDataUpdate( updateType );
		const model_t *newModel = GetModel();

		if ( oldModel != newModel )
		{
			m_modelChanged = true;
		}
	}
}

extern void VPhysicsShadowDataChanged( bool bCreate, C_BaseEntity *pEntity );

void C_BasePropDoor::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	bool bCreate = (type == DATA_UPDATE_CREATED) ? true : false;
	if ( VPhysicsGetObject() && m_modelChanged )
	{
		VPhysicsDestroyObject();
		m_modelChanged = false;
		bCreate = true;
	}
	VPhysicsShadowDataChanged(bCreate, this);
}

bool C_BasePropDoor::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !VPhysicsGetObject() )
		return false;

	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
		return false;

	physcollision->TraceBox( ray, VPhysicsGetObject()->GetCollide(), GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.DidHit() )
	{
	#if 0
		trace.contents = pStudioHdr->contents();
		// use the default surface properties
		trace.surface.name = "**studio**";
		trace.surface.flags = 0;
	#endif
		trace.surface.surfaceProps = VPhysicsGetObject()->GetMaterialIndex();
		return true;
	}

	return false;
}

//just need to reference by classname in portal
class C_PropDoorRotating : public C_BasePropDoor
{
public:
	DECLARE_CLASS( C_PropDoorRotating, C_BasePropDoor );
	DECLARE_CLIENTCLASS();
};

IMPLEMENT_CLIENTCLASS_DT(C_PropDoorRotating, DT_PropDoorRotating, CPropDoorRotating)
END_RECV_TABLE()
