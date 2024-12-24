//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef ANIMATION_H
#define ANIMATION_H

#pragma once

#include "mathlib/vector.h"
#include "mathlib/mathlib.h"
#include "studio.h"
#include "ai_activity.h"

DECLARE_LOGGING_CHANNEL( LOG_MODEL );

struct animevent_t;
struct studiohdr_t;
class CStudioHdr;
struct mstudioseqdesc_t;

bool ExtractBbox( CStudioHdr *pstudiohdr, sequence_t sequence, Vector& mins, Vector& maxs );

void IndexModelSequences( CStudioHdr *pstudiohdr );
void ResetActivityIndexes( CStudioHdr *pstudiohdr );
void VerifySequenceIndex( CStudioHdr *pstudiohdr );
sequence_t SelectWeightedSequence( CStudioHdr *pstudiohdr, Activity activity, sequence_t curSequence = INVALID_SEQUENCE );
sequence_t SelectHeaviestSequence( CStudioHdr *pstudiohdr, Activity activity );
void SetEventIndexForSequence( mstudioseqdesc_t &seqdesc );
void BuildAllAnimationEventIndexes( CStudioHdr *pstudiohdr );
void ResetEventIndexes( CStudioHdr *pstudiohdr );
float GetSequenceLinearMotionAndDuration( CStudioHdr *pstudiohdr, sequence_t iSequence, const float poseParameter[], Vector *pVec );

void GetEyePosition( CStudioHdr *pstudiohdr, Vector &vecEyePosition );

Activity LookupActivity( CStudioHdr *pstudiohdr, const char *label );
sequence_t LookupSequence( CStudioHdr *pstudiohdr, const char *label );

#define NOMOTION 99999
void GetSequenceLinearMotion( CStudioHdr *pstudiohdr, sequence_t iSequence, const float poseParameter[], Vector *pVec );

const char *GetSequenceName( CStudioHdr *pstudiohdr, sequence_t sequence );
const char *GetSequenceActivityName( CStudioHdr *pstudiohdr, sequence_t iSequence );

SequenceFlags_t GetSequenceFlags( CStudioHdr *pstudiohdr, sequence_t sequence );
int GetAnimationEvent( CStudioHdr *pstudiohdr, sequence_t sequence, animevent_t *pNPCEvent, float flStart, float flEnd, int index );
bool HasAnimationEventOfType( CStudioHdr *pstudiohdr, sequence_t sequence, int type );

sequence_t FindTransitionSequence( CStudioHdr *pstudiohdr, sequence_t iCurrentSequence, sequence_t iGoalSequence, int *piDir );
bool GotoSequence( CStudioHdr *pstudiohdr, int iCurrentSequence, float flCurrentCycle, float flCurrentRate, sequence_t iGoalSequence, sequence_t &nNextSequence, float &flNextCycle, int &iNextDir );

void SetBodygroup( CStudioHdr *pstudiohdr, int& body, int iGroup, int iValue );
int GetBodygroup( CStudioHdr *pstudiohdr, int body, int iGroup );

const char *GetBodygroupName( CStudioHdr *pstudiohdr, int iGroup );
int FindBodygroupByName( CStudioHdr *pstudiohdr, const char *name );
const char *GetBodygroupPartName( CStudioHdr *pstudiohdr, int iGroup, int iPart );
int GetBodygroupCount( CStudioHdr *pstudiohdr, int iGroup );
int GetNumBodyGroups( CStudioHdr *pstudiohdr );

const char *GetBodygroupPartName( CStudioHdr *pstudiohdr, int iGroup, int iPart );

Activity GetSequenceActivity( CStudioHdr *pstudiohdr, sequence_t sequence, int *pweight = NULL );

void GetAttachmentLocalSpace( CStudioHdr *pstudiohdr, int attachIndex, matrix3x4_t &pLocalToWorld );

float SetBlending( CStudioHdr *pstudiohdr, sequence_t sequence, int *pblendings, int iBlender, float flValue );

int FindHitboxSetByName( CStudioHdr *pstudiohdr, const char *name );
const char *GetHitboxSetName( CStudioHdr *pstudiohdr, int setnumber );
int GetHitboxSetCount( CStudioHdr *pstudiohdr );

#endif	//ANIMATION_H
