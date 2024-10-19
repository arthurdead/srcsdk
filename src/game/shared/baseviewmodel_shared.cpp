//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "baseviewmodel_shared.h"
#include "datacache/imdlcache.h"

#if defined( CLIENT_DLL )
#include "iprediction.h"
#include "prediction.h"
#include "c_vguiscreen.h"
#else
#include "vguiscreen.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VIEWMODEL_ANIMATION_PARITY_BITS 3
#define SCREEN_OVERLAY_MATERIAL "vgui/screens/vgui_overlay"

#ifdef CLIENT_DLL
ConVar cl_wpn_sway_interp( "cl_wpn_sway_interp", "0.1", FCVAR_CLIENTDLL );
ConVar cl_wpn_sway_scale( "cl_wpn_sway_scale", "1.0", FCVAR_CLIENTDLL|FCVAR_CHEAT );
#endif

#ifdef CLIENT_DLL
	#define CBaseViewModel C_BaseViewModel
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedBaseViewModel::CBaseViewModel()
#ifdef CLIENT_DLL
  : m_LagAnglesHistory("CSharedBaseViewModel::m_LagAnglesHistory")
#endif
{
#if defined( CLIENT_DLL )
	// NOTE: We do this here because the color is never transmitted for the view model.
	m_nOldAnimationParity = 0;
	AddClientFlags( ENTCLIENTFLAG_ALWAYS_INTERPOLATE );
#endif
	SetRenderColor( 255, 255, 255 );
	SetRenderAlpha( 255 );

	// View model of this weapon
	m_sVMName			= NULL_STRING;		
	// Prefix of the animations that should be used by the player carrying this weapon
	m_sAnimationPrefix	= NULL_STRING;

	m_nViewModelIndex	= 0;

	m_nAnimationParity	= 0;

#ifdef CLIENT_DLL
	m_vLagAngles.Init();
	m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedBaseViewModel::~CBaseViewModel()
{
}

#ifdef CLIENT_DLL
	#undef CBaseViewModel
#endif

void CSharedBaseViewModel::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	DestroyControlPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::Spawn( void )
{
	Precache( );
	SetSize( Vector( -8, -4, -2), Vector(8, 4, 2) );
	SetSolid( SOLID_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::SetControlPanelsActive( bool bState )
{
	// Activate control panel screens
	for ( int i = m_hScreens.Count(); --i >= 0; )
	{
		if (m_hScreens[i].Get())
		{
			m_hScreens[i]->SetActive( bState );
		}
	}
}

//-----------------------------------------------------------------------------
// This is called by the base object when it's time to spawn the control panels
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::SpawnControlPanels()
{
	char buf[64];

	// Destroy existing panels
	DestroyControlPanels();

	CSharedBaseCombatWeapon *weapon = m_hWeapon.Get();

	if ( weapon == NULL )
	{
		return;
	}

	MDLCACHE_CRITICAL_SECTION();

	// FIXME: Deal with dynamically resizing control panels?

	// If we're attached to an entity, spawn control panels on it instead of use
	CSharedBaseAnimating *pEntityToSpawnOn = this;
	const char *pOrgLL = "controlpanel%d_ll";
	const char *pOrgUR = "controlpanel%d_ur";
	const char *pAttachmentNameLL = pOrgLL;
	const char *pAttachmentNameUR = pOrgUR;
	/*
	if ( IsBuiltOnAttachment() )
	{
		pEntityToSpawnOn = dynamic_cast<CBaseAnimating*>((CBaseEntity*)m_hBuiltOnEntity.Get());
		if ( pEntityToSpawnOn )
		{
			char sBuildPointLL[64];
			char sBuildPointUR[64];
			Q_snprintf( sBuildPointLL, sizeof( sBuildPointLL ), "bp%d_controlpanel%%d_ll", m_iBuiltOnPoint );
			Q_snprintf( sBuildPointUR, sizeof( sBuildPointUR ), "bp%d_controlpanel%%d_ur", m_iBuiltOnPoint );
			pAttachmentNameLL = sBuildPointLL;
			pAttachmentNameUR = sBuildPointUR;
		}
		else
		{
			pEntityToSpawnOn = this;
		}
	}
	*/

	Assert( pEntityToSpawnOn );

	// Lookup the attachment point...
	int nPanel;
	for ( nPanel = 0; true; ++nPanel )
	{
		Q_snprintf( buf, sizeof( buf ), pAttachmentNameLL, nPanel );
		int nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
		if (nLLAttachmentIndex <= 0)
		{
			// Try and use my panels then
			pEntityToSpawnOn = this;
			Q_snprintf( buf, sizeof( buf ), pOrgLL, nPanel );
			nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
			if (nLLAttachmentIndex <= 0)
				return;
		}

		Q_snprintf( buf, sizeof( buf ), pAttachmentNameUR, nPanel );
		int nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
		if (nURAttachmentIndex <= 0)
		{
			// Try and use my panels then
			Q_snprintf( buf, sizeof( buf ), pOrgUR, nPanel );
			nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
			if (nURAttachmentIndex <= 0)
				return;
		}

		const char *pScreenName;
		weapon->GetControlPanelInfo( nPanel, pScreenName );
		if (!pScreenName)
			continue;

		const char *pScreenClassname;
		weapon->GetControlPanelClassName( nPanel, pScreenClassname );
		if ( !pScreenClassname )
			continue;

		// Compute the screen size from the attachment points...
		matrix3x4_t	panelToWorld;
		pEntityToSpawnOn->GetAttachment( nLLAttachmentIndex, panelToWorld );

		matrix3x4_t	worldToPanel;
		MatrixInvert( panelToWorld, worldToPanel );

		// Now get the lower right position + transform into panel space
		Vector lr, lrlocal;
		pEntityToSpawnOn->GetAttachment( nURAttachmentIndex, panelToWorld );
		MatrixGetColumn( panelToWorld, 3, lr );
		VectorTransform( lr, worldToPanel, lrlocal );

		float flWidth = lrlocal.x;
		float flHeight = lrlocal.y;

		CSharedVGuiScreen *pScreen = CREATE_PREDICTED_VGUISCREEN( pScreenClassname, pScreenName, pEntityToSpawnOn, this, nLLAttachmentIndex );
		pScreen->ChangeTeam( GetTeamNumber() );
		pScreen->SetActualSize( flWidth, flHeight );
		pScreen->SetActive( false );
		pScreen->MakeVisibleOnlyToTeammates( false );
	
#ifdef INVASION_DLL
		pScreen->SetOverlayMaterial( SCREEN_OVERLAY_MATERIAL );
#endif
		pScreen->SetAttachedToViewModel( true );
		int nScreen = m_hScreens.AddToTail( );
		m_hScreens[nScreen].Set( pScreen );
	}
}

void CSharedBaseViewModel::DestroyControlPanels()
{
	// Kill the control panels
	int i;
	for ( i = m_hScreens.Count(); --i >= 0; )
	{
		DestroyVGuiScreen( m_hScreens[i].Get() );
	}
	m_hScreens.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::SetOwner( CSharedBaseEntity *pEntity )
{
	m_hOwner = pEntity;
#if !defined( CLIENT_DLL )
	// Make sure we're linked into hierarchy
	//SetParent( pEntity );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::SetIndex( int nIndex )
{
	m_nViewModelIndex = nIndex;
	Assert( m_nViewModelIndex < (1 << VIEWMODEL_INDEX_BITS) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSharedBaseViewModel::ViewModelIndex( ) const
{
	return m_nViewModelIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Pass our visibility on to our child screens
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::AddEffects( int nEffects )
{
	if ( nEffects & EF_NODRAW )
	{
		SetControlPanelsActive( false );
	}

	if (GetOwningWeapon() && GetOwningWeapon()->UsesHands())
	{
		// If using hands, apply effect changes to any viewmodel children as well
		// (fixes hand models)
		for (CSharedBaseEntity *pChild = FirstMoveChild(); pChild != NULL; pChild = pChild->NextMovePeer())
		{
			if (FClassnameIs(pChild, gm_isz_class_HandViewmodel))
				pChild->AddEffects( nEffects );
		}
	}

	BaseClass::AddEffects( nEffects );
}

//-----------------------------------------------------------------------------
// Purpose: Pass our visibility on to our child screens
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::RemoveEffects( int nEffects )
{
	if ( nEffects & EF_NODRAW )
	{
		SetControlPanelsActive( true );
	}

	if (GetOwningWeapon() && GetOwningWeapon()->UsesHands())
	{
		// If using hands, apply effect changes to any viewmodel children as well
		// (fixes hand models)
		for (CSharedBaseEntity *pChild = FirstMoveChild(); pChild != NULL; pChild = pChild->NextMovePeer())
		{
			if (FClassnameIs(pChild, gm_isz_class_HandViewmodel))
				pChild->RemoveEffects( nEffects );
		}
	}

	BaseClass::RemoveEffects( nEffects );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *modelname - 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::SetWeaponModel( const char *modelname, CSharedBaseCombatWeapon *weapon )
{
	m_hWeapon = weapon;

#if defined( CLIENT_DLL )
	SetModel( modelname );
#else
	string_t str;
	if ( modelname != NULL )
	{
		str = MAKE_STRING( modelname );
	}
	else
	{
		str = NULL_STRING;
	}

	if ( str != m_sVMName )
	{
		// Msg( "SetWeaponModel %s at %f\n", modelname, gpGlobals->curtime );
		m_sVMName = str;
		SetModel( STRING( m_sVMName ) );

		// Create any vgui control panels associated with the weapon
		SpawnControlPanels();

		bool showControlPanels = weapon && weapon->ShouldShowControlPanels();
		SetControlPanelsActive( showControlPanels );
	}
#endif

	// If our owning weapon doesn't support hands, disable the hands viewmodel(s)
	bool bSupportsHands = weapon != NULL ? weapon->UsesHands() : false;
	for (CSharedBaseEntity *pChild = FirstMoveChild(); pChild != NULL; pChild = pChild->NextMovePeer())
	{
		if (FClassnameIs(pChild, gm_isz_class_HandViewmodel))
		{
			if(bSupportsHands)
				pChild->RemoveEffects( EF_NODRAW );
			else
				pChild->AddEffects( EF_NODRAW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatWeapon
//-----------------------------------------------------------------------------
CSharedBaseCombatWeapon *CSharedBaseViewModel::GetOwningWeapon( void )
{
	return m_hWeapon.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sequence - 
//-----------------------------------------------------------------------------
void CSharedBaseViewModel::SendViewModelMatchingSequence( int sequence )
{
	// since all we do is send a sequence number down to the client, 
	// set this here so other weapons code knows which sequence is playing.
	SetSequence( sequence );

	m_nAnimationParity = ( m_nAnimationParity + 1 ) & ( (1<<VIEWMODEL_ANIMATION_PARITY_BITS) - 1 );

#if defined( CLIENT_DLL )
	m_nOldAnimationParity = m_nAnimationParity;

	// Force frame interpolation to start at exactly frame zero
	SetAnimTime( gpGlobals->curtime );
#else
	CBaseCombatWeapon *weapon = m_hWeapon.Get();
	bool showControlPanels = weapon && weapon->ShouldShowControlPanels();
	SetControlPanelsActive( showControlPanels );
#endif

	// Restart animation at frame 0
	SetCycle( 0 );
	ResetSequenceInfo();
}

#if defined( CLIENT_DLL )
#include "ivieweffects.h"
#endif

void CSharedBaseViewModel::CalcViewModelView( CSharedBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
	// UNDONE: Calc this on the server?  Disabled for now as it seems unnecessary to have this info on the server
#if defined( CLIENT_DLL )
	QAngle vmangoriginal = eyeAngles;
	QAngle vmangles = eyeAngles;
	Vector vmorigin = eyePosition;

	CSharedBaseCombatWeapon *pWeapon = m_hWeapon.Get();
	//Allow weapon lagging
	if ( pWeapon != NULL )
	{
#if defined( CLIENT_DLL )
		if ( !prediction->InPrediction() )
#endif
		{
			// add weapon-specific bob 
			pWeapon->AddViewmodelBob( this, vmorigin, vmangles );
#if defined ( CSTRIKE_DLL )
			CalcViewModelLag( vmorigin, vmangles, vmangoriginal );
#endif
		}
	}
	// Add model-specific bob even if no weapon associated (for head bob for off hand models)
	AddViewModelBob( owner, vmorigin, vmangles );
#if !defined ( CSTRIKE_DLL )
	// This was causing weapon jitter when rotating in updated CS:S; original Source had this in above InPrediction block  07/14/10
	// Add lag
	CalcViewModelLag( vmorigin, vmangles, vmangoriginal );
#endif

#if defined( CLIENT_DLL )
	if ( !prediction->InPrediction() )
	{
		// Let the viewmodel shake at about 10% of the amplitude of the player's view
		GetViewEffects()->ApplyShake( vmorigin, vmangles, 0.1 );	
	}
#endif

	SetLocalOrigin( vmorigin );
	SetLocalAngles( vmangles );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float g_fMaxViewModelLag = 1.5f;

void CSharedBaseViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
	Vector vOriginalOrigin = origin;
	QAngle vOriginalAngles = angles;

	// Calculate our drift
	Vector	forward;
	AngleVectors( angles, &forward, NULL, NULL );

	if ( gpGlobals->frametime != 0.0f )
	{
		Vector vDifference;
		VectorSubtract( forward, m_vecLastFacing, vDifference );

		float flSpeed = 5.0f;

		CSharedBaseCombatWeapon *pWeapon = m_hWeapon.Get();
		if (pWeapon)
		{
			const FileWeaponInfo_t *pInfo = &pWeapon->GetWpnData();
			if (pInfo->m_flSwayScale != 1.0f)
			{
				vDifference *= pInfo->m_flSwayScale;
				pInfo->m_flSwayScale != 0.0f ? flSpeed /= pInfo->m_flSwayScale : flSpeed = 0.0f;
			}
			if (pInfo->m_flSwaySpeedScale != 1.0f)
			{
				flSpeed *= pInfo->m_flSwaySpeedScale;
			}
		}

		// If we start to lag too far behind, we'll increase the "catch up" speed.  Solves the problem with fast cl_yawspeed, m_yaw or joysticks
		//  rotating quickly.  The old code would slam lastfacing with origin causing the viewmodel to pop to a new position
		float flDiff = vDifference.Length();
		if ( (flDiff > g_fMaxViewModelLag) && (g_fMaxViewModelLag > 0.0f) )
		{
			float flScale = flDiff / g_fMaxViewModelLag;
			flSpeed *= flScale;
		}

		// FIXME:  Needs to be predictable?
		VectorMA( m_vecLastFacing, flSpeed * gpGlobals->frametime, vDifference, m_vecLastFacing );
		// Make sure it doesn't grow out of control!!!
		VectorNormalize( m_vecLastFacing );
		VectorMA( origin, 5.0f, vDifference * -1.0f, origin );

		Assert( m_vecLastFacing.IsValid() );
	}

	Vector right, up;
	AngleVectors( original_angles, &forward, &right, &up );

	float pitch = original_angles[ PITCH ];
	if ( pitch > 180.0f )
		pitch -= 360.0f;
	else if ( pitch < -180.0f )
		pitch += 360.0f;

	if ( g_fMaxViewModelLag == 0.0f )
	{
		origin = vOriginalOrigin;
		angles = vOriginalAngles;
	}

	//FIXME: These are the old settings that caused too many exposed polys on some models
	VectorMA( origin, -pitch * 0.035f,	forward,	origin );
	VectorMA( origin, -pitch * 0.03f,		right,	origin );
	VectorMA( origin, -pitch * 0.02f,		up,		origin);

#ifdef CLIENT_DLL
	
#endif
}

//-----------------------------------------------------------------------------
// Stub to keep networking consistent for DEM files
//-----------------------------------------------------------------------------
#if defined( CLIENT_DLL )
  extern void RecvProxy_EffectFlags( const CRecvProxyData *pData, void *pStruct, void *pOut );
  void RecvProxy_SequenceNum( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

//-----------------------------------------------------------------------------
// Purpose: Resets anim cycle when the server changes the weapon on us
//-----------------------------------------------------------------------------
#if defined( CLIENT_DLL )
static void RecvProxy_Weapon( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CSharedBaseViewModel *pViewModel = ((CSharedBaseViewModel*)pStruct);
	CSharedBaseCombatWeapon *pOldWeapon = pViewModel->GetOwningWeapon();

	// Chain through to the default recieve proxy ...
	RecvProxy_IntToEHandle( pData, pStruct, pOut );

	// ... and reset our cycle index if the server is switching weapons on us
	CSharedBaseCombatWeapon *pNewWeapon = pViewModel->GetOwningWeapon();
	if ( pNewWeapon != pOldWeapon )
	{
		// Restart animation at frame 0
		pViewModel->SetCycle( 0 );
		pViewModel->SetAnimTime(gpGlobals->curtime);
	}
}
#endif


LINK_ENTITY_TO_CLASS_ALIASED( viewmodel, BaseViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( BaseViewModel, DT_BaseViewModel )

BEGIN_NETWORK_TABLE_NOBASE(CSharedBaseViewModel, DT_BaseViewModel)
#if !defined( CLIENT_DLL )
	SendPropModelIndex(SENDINFO(m_nModelIndex)),
	SendPropInt		(SENDINFO(m_nBody), 8),
	SendPropInt		(SENDINFO(m_nSkin), 10),
	SendPropInt		(SENDINFO(m_nSequence),	8, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_nViewModelIndex), VIEWMODEL_INDEX_BITS, SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flPlaybackRate),	8,	SPROP_ROUNDUP,	-4.0,	12.0f),
	SendPropInt		(SENDINFO(m_fEffects),		10, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_nAnimationParity), 3, SPROP_UNSIGNED ),
	SendPropEHandle (SENDINFO(m_hWeapon)),
	SendPropEHandle (SENDINFO(m_hOwner)),

	SendPropInt( SENDINFO( m_nNewSequenceParity ), EF_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nResetEventsParity ), EF_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMuzzleFlashParity ), EF_MUZZLEFLASH_BITS, SPROP_UNSIGNED ),

#if !defined( INVASION_DLL ) && !defined( INVASION_CLIENT_DLL )
	SendPropArray	(SendPropFloat(SENDINFO_ARRAY(m_flPoseParameter),	8, 0, 0.0f, 1.0f), m_flPoseParameter),
#endif
#else
	RecvPropInt		(RECVINFO(m_nModelIndex)),
	RecvPropInt		(RECVINFO(m_nSkin)),
	RecvPropInt		(RECVINFO(m_nBody)),
	RecvPropInt		(RECVINFO(m_nSequence), 0, RecvProxy_SequenceNum ),
	RecvPropInt		(RECVINFO(m_nViewModelIndex)),
	RecvPropFloat	(RECVINFO(m_flPlaybackRate)),
	RecvPropInt		(RECVINFO(m_fEffects), 0, RecvProxy_EffectFlags ),
	RecvPropInt		(RECVINFO(m_nAnimationParity)),
	RecvPropEHandle (RECVINFO(m_hWeapon), RecvProxy_Weapon ),
	RecvPropEHandle (RECVINFO(m_hOwner)),

	RecvPropInt( RECVINFO( m_nNewSequenceParity )),
	RecvPropInt( RECVINFO( m_nResetEventsParity )),
	RecvPropInt( RECVINFO( m_nMuzzleFlashParity )),

#if !defined( INVASION_DLL ) && !defined( INVASION_CLIENT_DLL )
	RecvPropArray(RecvPropFloat(RECVINFO(m_flPoseParameter[0]) ), m_flPoseParameter ),
#endif
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL

BEGIN_PREDICTION_DATA( C_BaseViewModel )

	// Networked
	DEFINE_PRED_FIELD( m_nModelIndex, FIELD_SHORT, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nViewModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.125f ),
	DEFINE_PRED_FIELD( m_fEffects, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_OVERRIDE ),
	DEFINE_PRED_FIELD( m_nAnimationParity, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flAnimTime, FIELD_FLOAT, 0 ),

	DEFINE_FIELD( m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flTimeWeaponIdle, FIELD_FLOAT ),
	DEFINE_FIELD( m_Activity, FIELD_INTEGER ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_PRIVATE | FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK ),

END_PREDICTION_DATA()

void RecvProxy_SequenceNum( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_BaseViewModel *model = (C_BaseViewModel *)pStruct;
	if (pData->m_Value.m_Int != model->GetSequence())
	{
		MDLCACHE_CRITICAL_SECTION();

		model->SetSequence(pData->m_Value.m_Int);
		model->SetAnimTime( gpGlobals->curtime );
		model->SetCycle(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSharedBaseViewModel::LookupAttachment( const char *pAttachmentName )
{
	if ( m_hWeapon.Get() && m_hWeapon.Get()->WantsToOverrideViewmodelAttachments() )
		return m_hWeapon.Get()->LookupAttachment( pAttachmentName );

	return BaseClass::LookupAttachment( pAttachmentName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseViewModel::GetAttachment( int number, matrix3x4_t &matrix )
{
	if ( m_hWeapon.Get() && m_hWeapon.Get()->WantsToOverrideViewmodelAttachments() )
		return m_hWeapon.Get()->GetAttachment( number, matrix );

	return BaseClass::GetAttachment( number, matrix );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseViewModel::GetAttachment( int number, Vector &origin )
{
	if ( m_hWeapon.Get() && m_hWeapon.Get()->WantsToOverrideViewmodelAttachments() )
		return m_hWeapon.Get()->GetAttachment( number, origin );

	return BaseClass::GetAttachment( number, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseViewModel::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	if ( m_hWeapon.Get() && m_hWeapon.Get()->WantsToOverrideViewmodelAttachments() )
		return m_hWeapon.Get()->GetAttachment( number, origin, angles );

	return BaseClass::GetAttachment( number, origin, angles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSharedBaseViewModel::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if ( m_hWeapon.Get() && m_hWeapon.Get()->WantsToOverrideViewmodelAttachments() )
		return m_hWeapon.Get()->GetAttachmentVelocity( number, originVel, angleVel );

	return BaseClass::GetAttachmentVelocity( number, originVel, angleVel );
}

#endif

#ifdef CLIENT_DLL
class C_HandViewModel;
typedef C_HandViewModel CSharedHandViewModel;
#else
class CHandViewModel;
typedef CHandViewModel CSharedHandViewModel;
#endif

// ---------------------------------------
// OzxyBox's hand viewmodel code.
// All credit goes to him.
// ---------------------------------------
#ifdef CLIENT_DLL
	#define CHandViewModel C_HandViewModel
#endif

class CHandViewModel : public CSharedBaseViewModel
{
public:
	DECLARE_CLASS( CHandViewModel, CSharedBaseViewModel );

#ifdef CLIENT_DLL
	#undef CHandViewModel
#endif

	DECLARE_NETWORKCLASS();

	CSharedBaseViewModel	*GetVMOwner();

	CSharedBaseCombatWeapon *GetOwningWeapon( void );

private:
	CHandle<CSharedBaseViewModel> m_hVMOwner;
};

LINK_ENTITY_TO_CLASS_ALIASED(hand_viewmodel, HandViewModel);
IMPLEMENT_NETWORKCLASS_ALIASED(HandViewModel, DT_HandViewModel)

BEGIN_NETWORK_TABLE(CSharedHandViewModel, DT_HandViewModel)
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedBaseViewModel *CSharedHandViewModel::GetVMOwner()
{
	if (!m_hVMOwner)
		m_hVMOwner = assert_cast<CSharedBaseViewModel*>(GetMoveParent());
	return m_hVMOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSharedBaseCombatWeapon *CSharedHandViewModel::GetOwningWeapon()
{
	CSharedBaseViewModel *pVM = GetVMOwner();
	if (pVM)
		return pVM->GetOwningWeapon();
	else
		return NULL;
}