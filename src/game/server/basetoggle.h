//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: For the slow removing of the CBaseToggle entity
//			only old entities that need it for backwards-compatibility should
//			include this file
//=============================================================================//

#ifndef BASETOGGLE_H
#define BASETOGGLE_H
#pragma once

#include "baseentity.h"

enum SFBaseToggle_t : uint64
{
	SF_TOGGLE_ROTATE_ROLL =			(1 << 0),
	SF_TOGGLE_ROTATE_PITCH =		(1 << 1),

	SF_TOGGLE_LAST_FLAG = SF_TOGGLE_ROTATE_PITCH,
};

FLAGENUM_OPERATORS( SFBaseToggle_t, uint64 )

class CBaseToggle : public CBaseEntity
{
public:
	DECLARE_CLASS( CBaseToggle, CBaseEntity );
	DECLARE_SERVERCLASS();

	DECLARE_SPAWNFLAGS( SFBaseToggle_t )

	CBaseToggle();

	virtual bool		KeyValue( const char *szKeyName, const char *szValue );
	virtual bool		KeyValue( const char *szKeyName, Vector vec ) { return BaseClass::KeyValue( szKeyName, vec ); };
	virtual bool		KeyValue( const char *szKeyName, float flValue ) { return BaseClass::KeyValue( szKeyName, flValue ); };

	TOGGLE_STATE		m_toggle_state;
	float				m_flMoveDistance;// how far a door should slide or rotate
	float				m_flWait;
	float				m_flLip;

	Vector				m_vecPosition1;
	Vector				m_vecPosition2;

	QAngle				m_vecMoveAng;
	QAngle				m_vecAngle1;
	QAngle				m_vecAngle2;

	float				m_flHeight;
	EHANDLE				m_hActivator;
	Vector				m_vecFinalDest;
	QAngle				m_vecFinalAngle;

	int					m_movementType;

	virtual float	GetDelay( void ) { return m_flWait; }

	// common member functions
	void LinearMove( const Vector &vecDest, float flSpeed );
	void LinearMoveDone( void );
	void AngularMove( const QAngle &vecDestAngle, float flSpeed );
	void AngularMoveDone( void );
	bool IsLockedByMaster( void );
	virtual void MoveDone( void );

	virtual void GetGroundVelocityToApply( Vector &vecGroundVel );

	float AxisValue( const QAngle &angles );
	void AxisDir( void );
	float AxisDelta( const QAngle &angle1, const QAngle &angle2 );

	string_t m_sMaster;		// If this button has a master switch, this is the targetname.
							// A master switch must be of the multisource type. If all 
							// of the switches in the multisource have been triggered, then
							// the button will be allowed to operate. Otherwise, it will be
							// deactivated.
};



#endif // BASETOGGLE_H
