//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef BUTTONS_H
#define BUTTONS_H
#pragma once

#include "basetoggle.h"
#include "locksounds.h"

enum SFButton_t : uint64
{
	SF_BUTTON_DONTMOVE =				(SF_TOGGLE_LAST_FLAG << 1),
	SF_BUTTON_TOGGLE =				(SF_TOGGLE_LAST_FLAG << 2),		// button stays pushed until reactivated
	SF_BUTTON_TOUCH_ACTIVATES =		(SF_TOGGLE_LAST_FLAG << 3),		// Button fires when touched.
	SF_BUTTON_DAMAGE_ACTIVATES =		(SF_TOGGLE_LAST_FLAG << 4),		// Button fires when damaged.
	SF_BUTTON_USE_ACTIVATES =			(SF_TOGGLE_LAST_FLAG << 5),	// Button fires when used.
	SF_BUTTON_LOCKED =				(SF_TOGGLE_LAST_FLAG << 6),	// Whether the button is initially locked.
	SF_BUTTON_SPARK_IF_OFF =			(SF_TOGGLE_LAST_FLAG << 7),	// button sparks in OFF state
	SF_BUTTON_JIGGLE_ON_USE_LOCKED =	(SF_TOGGLE_LAST_FLAG << 8),	// whether to jiggle if someone uses us when we're locked

	SF_BUTTON_LAST_FLAG = SF_BUTTON_JIGGLE_ON_USE_LOCKED,
};

FLAGENUM_OPERATORS( SFButton_t, uint64 )

class CBaseButton : public CBaseToggle
{
public:

	DECLARE_CLASS( CBaseButton, CBaseToggle );
	DECLARE_SERVERCLASS();

	DECLARE_SPAWNFLAGS( SFButton_t )

	void Spawn( void );
	virtual void Precache( void );
	bool CreateVPhysics();
	void RotSpawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	int DrawDebugTextOverlays();

protected:

	void ButtonActivate( );
	void SparkSoundCache( void );

	void ButtonTouch( ::CBaseEntity *pOther );
	void ButtonSpark ( void );
	void TriggerAndWait( void );
	void ButtonReturn( void );
	void ButtonBackHome( void );
	void ButtonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	bool OnUseLocked( CBaseEntity *pActivator );

	virtual void Lock();
	virtual void Unlock();

	// Input handlers
	void InputLock( inputdata_t &&inputdata );
	void InputUnlock( inputdata_t &&inputdata );
	void InputPress( inputdata_t &&inputdata );
	void InputPressIn( inputdata_t &&inputdata );
	void InputPressOut( inputdata_t &&inputdata );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	
	enum BUTTON_CODE : unsigned char
	{
		BUTTON_NOTHING,
		BUTTON_ACTIVATE,
		BUTTON_RETURN,
		BUTTON_PRESS
	};

	BUTTON_CODE	ButtonResponseToTouch( void );
	void Press( CBaseEntity *pActivator, BUTTON_CODE eCode );
	
	DECLARE_MAPENTITY();

	virtual EntityCaps_t ObjectCaps(void);

	Vector m_vecMoveDir;

	bool	m_fStayPushed;		// button stays pushed in until touched again?
	bool	m_fRotating;		// a rotating button?  default is a sliding button.

	locksound_t m_ls;			// door lock sounds
	
	byte	m_bLockedSound;		// ordinals from entity selection
	byte	m_bLockedSentence;	
	byte	m_bUnlockedSound;	
	byte	m_bUnlockedSentence;
	bool	m_bLocked;
	int		m_sounds;
	float	m_flUseLockedTime;		// Controls how often we fire the OnUseLocked output.

	bool	m_bSolidBsp;

	string_t	m_sNoise;			// The actual WAV file name of the sound.

	COutputEvent m_OnDamaged;
	COutputEvent m_OnPressed;
	COutputEvent m_OnUseLocked;
	COutputEvent m_OnIn;
	COutputEvent m_OnOut;

	int		m_nState;
};

enum SFRotButton_t : uint64
{
	SF_ROTBUTTON_NOTSOLID =			(SF_BUTTON_LAST_FLAG << 1),
	SF_ROTBUTTON_BACKWARDS = (SF_BUTTON_LAST_FLAG << 2),

	SF_ROTBUTTON_LAST_FLAG = SF_ROTBUTTON_BACKWARDS,
};

FLAGENUM_OPERATORS( SFRotButton_t, uint64 )

//
// Rotating button (aka "lever")
//
class CRotButton : public CBaseButton
{
public:
	DECLARE_CLASS( CRotButton, CBaseButton );

	DECLARE_SPAWNFLAGS_OVERLOAD( SFButton_t )
	DECLARE_SPAWNFLAGS( SFRotButton_t )

	void Spawn( void );
	bool CreateVPhysics( void );
};

//-----------------------------------------------------------------------------
// CMomentaryRotButton spawnflags
//-----------------------------------------------------------------------------
enum SFMomentaryRotButton_t : uint64
{
	SF_MOMENTARY_DOOR =			(SF_ROTBUTTON_LAST_FLAG << 1),
	SF_MOMENTARY_NOT_USABLE =		(SF_ROTBUTTON_LAST_FLAG << 2),
	SF_MOMENTARY_AUTO_RETURN =	(SF_ROTBUTTON_LAST_FLAG << 3),
};

FLAGENUM_OPERATORS( SFMomentaryRotButton_t, uint64 )

class CMomentaryRotButton : public CRotButton
{
	DECLARE_CLASS( CMomentaryRotButton, CRotButton );

public:
	DECLARE_SPAWNFLAGS_OVERLOAD( SFButton_t )
	DECLARE_SPAWNFLAGS_OVERLOAD( SFRotButton_t )
	DECLARE_SPAWNFLAGS( SFMomentaryRotButton_t )

	void	Spawn ( void );
	bool	CreateVPhysics( void );
	virtual EntityCaps_t ObjectCaps( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	UseMoveDone( void );
	void	ReturnMoveDone( void );
	void	OutputMovementComplete(void);
	void	SetPositionMoveDone(void);
	void	UpdateSelf( float value, bool bPlaySound );

	void	PlaySound( void );
	void	UpdateTarget( float value, CBaseEntity *pActivator );

	int		DrawDebugTextOverlays(void);

	static CMomentaryRotButton *Instance( edict_t *pent ) { return (CMomentaryRotButton *)GetContainingEntity(pent); }

	float GetPos(const QAngle &vecAngles);

	DECLARE_MAPENTITY();

	virtual void Lock();
	virtual void Unlock();

	// Input handlers
	void InputSetPosition( inputdata_t &&inputdata );
	void InputSetPositionImmediately( inputdata_t &&inputdata );
	void InputDisableUpdateTarget( inputdata_t &&inputdata );
	void InputEnableUpdateTarget( inputdata_t &&inputdata );

	void InputEnable( inputdata_t &&inputdata );
	void InputDisable( inputdata_t &&inputdata );

	virtual void Enable( void );
	virtual void Disable( void );

	bool	m_bDisabled;

	COutputFloat m_Position;
	COutputEvent m_OnUnpressed;
	COutputEvent m_OnFullyOpen;
	COutputEvent m_OnFullyClosed;
	COutputEvent m_OnReachedPosition;

	int			m_lastUsed;
	QAngle		m_start;
	QAngle		m_end;
	float		m_IdealYaw;
	string_t	m_sNoise;

	bool		m_bUpdateTarget;		// Used when jiggling so that we don't jiggle the target (door, etc)

	int			m_direction;
	float		m_returnSpeed;
	float		m_flStartPosition;

protected:

	void UpdateThink( void );
};


#endif // BUTTONS_H
