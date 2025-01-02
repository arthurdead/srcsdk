//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DEATH_POSE_H
#define DEATH_POSE_H
#pragma once

enum Activity : unsigned short;
enum Hitgroup_t : unsigned int;

#ifdef GAME_DLL
class CBaseAnimating;
class CTakeDamageInfo;
#else
class C_BaseAnimating;
#endif

enum DeathFrame_t : unsigned char
{
	DEATH_FRAME_INVALID = 0,
	DEATH_FRAME_HEAD = 1,
	DEATH_FRAME_STOMACH,
	DEATH_FRAME_LEFTARM,
	DEATH_FRAME_RIGHTARM,
	DEATH_FRAME_LEFTLEG,
	DEATH_FRAME_RIGHTLEG,
	MAX_DEATHPOSE_FRAMES  = DEATH_FRAME_RIGHTLEG
};

#ifdef CLIENT_DLL

void GetRagdollCurSequenceWithDeathPose( C_BaseAnimating *entity, matrix3x4_t *curBones, float flTime, sequence_t activity, DeathFrame_t frame );

#else // !CLIENT_DLL

/// Calculates death pose activity and frame
void SelectDeathPoseActivityAndFrame( CBaseAnimating *entity, const CTakeDamageInfo &info, Hitgroup_t hitgroup, Activity& activity, DeathFrame_t& frame );

#endif // !CLIENT_DLL

#endif // DEATH_POSE_H
