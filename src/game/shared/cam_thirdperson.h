//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef CAM_THIRDPERSON_H
#define CAM_THIRDPERSON_H

#pragma once

#include "iinput.h"

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
#else
	#include "baseplayer.h"
#endif

#define DIST_FORWARD 0
#define DIST_RIGHT 1
#define DIST_UP 2

//-------------------------------------------------- Constants

#define CAM_MIN_DIST 30.0
#define CAM_ANGLE_MOVE .5
#define MAX_ANGLE_DIFF 10.0
#define PITCH_MAX 90.0
#define PITCH_MIN 0
#define YAW_MAX  135.0
#define YAW_MIN	 -135.0
#define	DIST	 2
#define CAM_HULL_OFFSET		14.0    // the size of the bounding hull used for collision checking

#define CAMERA_UP_OFFSET	25.0f
#define CAMERA_OFFSET_LERP_TIME 0.5f
#define CAMERA_UP_OFFSET_LERP_TIME 0.25f

enum
{
	FORCED_CAM_DISABLED = -1,
	FORCED_CAM_FIRSTPERSON = CAM_FIRSTPERSON,
	FORCED_CAM_THIRDPERSON = CAM_THIRDPERSON,
	FORCED_CAM_THIRDPERSONSHOULDER = CAM_THIRDPERSONSHOULDER,
	FORCED_CAM_MAYAMODE = CAM_MAYAMODE,
	FORCED_CAM_ORTHOGRAPHIC = CAM_ORTHOGRAPHIC
};

class CThirdPersonManager
{
public:
	CThirdPersonManager();

	void	SetCameraOffsetAngles( const Vector& vecOffset ) { m_vecCameraOffset = vecOffset; }
	const Vector&	GetCameraOffsetAngles( void ) const { return m_vecCameraOffset; }
	
	void	SetDesiredCameraOffset( const Vector& vecOffset ) { m_vecDesiredCameraOffset = vecOffset; }
	const Vector&	GetDesiredCameraOffset( void ) const { return m_vecDesiredCameraOffset; }

	Vector	GetFinalCameraOffset( void );

	void	SetCameraOrigin( const Vector& vecOffset ) { m_vecCameraOrigin = vecOffset; }
	const Vector&	GetCameraOrigin( void ) const { return m_vecCameraOrigin; }

	void	Update( void );

	void	PositionCamera( CSharedBasePlayer *pPlayer, const QAngle& angles );

	void	UseCameraOffsets( bool bUse ) { m_bUseCameraOffsets = bUse; }
	bool	UsingCameraOffsets( void ) { return m_bUseCameraOffsets; }

	const QAngle&	GetCameraViewAngles( void ) const { return m_ViewAngles; }

	Vector	GetDistanceFraction( void );

	void	Init( void );

	void	SetForcedCamera( int iForced ) { m_iForced = iForced; }
	int	GetForcedCamera() const { return m_iForced; }

private:

	// What is the current camera offset from the view origin?
	Vector		m_vecCameraOffset;
	// Distances from the center
	Vector		m_vecDesiredCameraOffset;

	Vector m_vecCameraOrigin;

	bool	m_bUseCameraOffsets;

	QAngle	m_ViewAngles;

	float	m_flFraction;
	float	m_flUpFraction;

	float	m_flTargetFraction;
	float	m_flTargetUpFraction;

	int	m_iForced;

	float	m_flUpOffset;

	float	m_flLerpTime;
	float	m_flUpLerpTime;
};

extern CThirdPersonManager g_ThirdPersonManager;

#endif // CAM_THIRDPERSON_H
