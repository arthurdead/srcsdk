//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEVIEWMODEL_SHARED_H
#define BASEVIEWMODEL_SHARED_H
#pragma once

#include "predictable_entity.h"
#include "utlvector.h"
#include "baseplayer_shared.h"
#include "shared_classnames.h"

#ifdef GAME_DLL
class CBaseCombatWeapon;
typedef CBaseCombatWeapon CSharedBaseCombatWeapon;
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
#else
class C_BaseCombatWeapon;
typedef C_BaseCombatWeapon CSharedBaseCombatWeapon;
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
#endif

#ifdef GAME_DLL
class CVGuiScreen;
typedef CVGuiScreen CSharedVGuiScreen;
#else
class C_VGuiScreen;
typedef C_VGuiScreen CSharedVGuiScreen;
#endif

#define VIEWMODEL_INDEX_BITS 1

#ifdef CLIENT_DLL
	#define CBaseViewModel C_BaseViewModel
#endif

class CBaseViewModel : public CSharedBaseAnimating
{
public:
	DECLARE_CLASS( CBaseViewModel, CSharedBaseAnimating );
	CBaseViewModel( void );
	~CBaseViewModel( void );
private:
	CBaseViewModel( const CBaseViewModel & ); // not defined, not accessible

#ifdef CLIENT_DLL
	#undef CBaseViewModel
#endif

public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	bool IsViewable(void) { return false; }

	virtual void					UpdateOnRemove( void );

	// Weapon client handling
	virtual void			SendViewModelMatchingSequence( int sequence );
	void			SendViewModelMatchingActivity( Activity activity )
	{ SendViewModelMatchingSequence( SelectWeightedSequence(activity) ); }
	virtual void			SetWeaponModel( const char *pszModelname, CSharedBaseCombatWeapon *weapon );

	virtual void			CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles );
	virtual void			CalcViewModelView( CSharedBasePlayer *owner, const Vector& eyePosition, 
								const QAngle& eyeAngles );
	virtual void			AddViewModelBob( CSharedBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles ) {};

	// Initializes the viewmodel for use							
	void					SetOwner( CSharedBaseEntity *pEntity );
	void					SetIndex( int nIndex );
	// Returns which viewmodel it is
	int						ViewModelIndex( ) const;

	virtual void			Precache( void );

	virtual void			Spawn( void );

	virtual CSharedBaseEntity *GetOwner( void ) { return m_hOwner.Get(); };

	virtual void			AddEffects( int nEffects );
	virtual void			RemoveEffects( int nEffects );

	void					SpawnControlPanels();
	void					DestroyControlPanels();
	void					SetControlPanelsActive( bool bState );
	void					ShowControlPanells( bool show );

	virtual CSharedBaseCombatWeapon *GetOwningWeapon( void );

	virtual bool			IsViewModel() const { return true; }
	virtual bool			IsViewModelOrAttachment() const { return true; }
	
	virtual CSharedBaseEntity	*GetOwnerViaInterface( void ) { return GetOwner(); }

	virtual bool			IsSelfAnimating()
	{
		return true;
	}

	Vector					m_vecLastFacing;

	// All predicted weapons need to implement and return true
	virtual bool			IsPredicted( void ) const
	{ 
		return true;
	}

#if !defined( CLIENT_DLL )
	virtual int				UpdateTransmitState( void );
	virtual int				ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void			SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
#else

#if defined( CLIENT_DLL )
	virtual bool ShouldPredict( void )
	{
		if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
			return true;

		return BaseClass::ShouldPredict();
	}
#endif


	virtual void			FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	virtual void			OnDataChanged( DataUpdateType_t updateType );
	virtual void			PostDataUpdate( DataUpdateType_t updateType );

	virtual C_BasePlayer	*GetPredictionOwner( void );

	virtual bool			Interpolate( float currentTime );

	bool					ShouldFlipViewModel();
	void					UpdateAnimationParity( void );

	virtual void			ApplyBoneMatrixTransform( matrix3x4_t& transform );

	virtual bool			ShouldDraw();
	virtual int				DrawModel( int flags, const RenderableInstance_t &instance );
	virtual int				InternalDrawModel( int flags, const RenderableInstance_t &instance );
	int						DrawOverriddenViewmodel( int flags, const RenderableInstance_t &instance );
	virtual uint8			OverrideRenderAlpha( uint8 nAlpha );
	RenderableTranslucencyType_t ComputeTranslucencyType( void );
	virtual int 					GetRenderFlags( void );
	
	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType() { return SHADOWS_NONE; }

	// Should this object receive shadows?
	virtual bool			ShouldReceiveProjectedTextures( int flags );

	// Add entity to visible view models list?
	virtual bool			Simulate( void );

	virtual void			GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS]);

	// See C_StudioModel's definition of this.
	virtual void			UncorrectViewModelAttachment( Vector &vOrigin );

	// (inherited from C_BaseAnimating)
	virtual void			FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld );

	CSharedBaseCombatWeapon		*GetWeapon() const { return m_hWeapon.Get(); }

#ifdef CLIENT_DLL
	virtual bool			ShouldResetSequenceOnNewModel( void ) { return false; }

	// Attachments
	virtual int				LookupAttachment( const char *pAttachmentName );
	virtual bool			GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool			GetAttachment( int number, Vector &origin );
	virtual	bool			GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool			GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );
#endif

#endif

private:
	CNetworkVar( int, m_nViewModelIndex );		// Which viewmodel is it?
	CNetworkHandle( CSharedBaseEntity, m_hOwner );				// Player or AI carrying this weapon

	// soonest time Update will call WeaponIdle
	float					m_flTimeWeaponIdle;							

	Activity				m_Activity;

	// Used to force restart on client, only needs a few bits
	CNetworkVar( int, m_nAnimationParity );

	// Weapon art
	string_t				m_sVMName;			// View model of this weapon
	string_t				m_sAnimationPrefix;		// Prefix of the animations that should be used by the player carrying this weapon

#if defined( CLIENT_DLL )
	int						m_nOldAnimationParity;
#endif

#if defined( CLIENT_DLL )

	// This is used to lag the angles.
	CInterpolatedVar<QAngle> m_LagAnglesHistory;
	QAngle m_vLagAngles;

#endif

	CNetworkHandle( CSharedBaseCombatWeapon, m_hWeapon );

	// Control panel
	typedef CHandle<CSharedVGuiScreen>	ScreenHandle_t;
	CUtlVector<ScreenHandle_t>	m_hScreens;
};

#ifdef GAME_DLL
typedef CBaseViewModel CSharedBaseViewModel;
#else
typedef C_BaseViewModel CSharedBaseViewModel;
#endif

inline CSharedBaseViewModel *ToBaseViewModel( CSharedBaseAnimating *pAnim )
{
	if ( pAnim && pAnim->IsViewModel() )
		return assert_cast<CSharedBaseViewModel *>(pAnim);
	return NULL;
}

inline CSharedBaseViewModel *ToBaseViewModel( CSharedBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;
	return ToBaseViewModel(pEntity->GetBaseAnimating());
}

#endif // BASEVIEWMODEL_SHARED_H
