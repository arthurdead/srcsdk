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

bool ExtractBbox( const CStudioHdr *pstudiohdr, sequence_t sequence, Vector& mins, Vector& maxs );

void IndexModelSequences( CStudioHdr *pstudiohdr );
void ResetActivityIndexes( CStudioHdr *pstudiohdr );
void VerifySequenceIndex( CStudioHdr *pstudiohdr );
sequence_t SelectWeightedSequence( const CStudioHdr *pstudiohdr, Activity activity, sequence_t curSequence = INVALID_SEQUENCE );
sequence_t SelectHeaviestSequence( const CStudioHdr *pstudiohdr, Activity activity );
void SetEventIndexForSequence( mstudioseqdesc_t &seqdesc );
void BuildAllAnimationEventIndexes( CStudioHdr *pstudiohdr );
void ResetEventIndexes( CStudioHdr *pstudiohdr );
float GetSequenceLinearMotionAndDuration( const CStudioHdr *pstudiohdr, sequence_t iSequence, const float poseParameter[], Vector *pVec );

void GetEyePosition( const CStudioHdr *pstudiohdr, Vector &vecEyePosition );

Activity LookupActivity( const CStudioHdr *pstudiohdr, const char *label );
sequence_t LookupSequence( const CStudioHdr *pstudiohdr, const char *label );

#define NOMOTION 99999
void GetSequenceLinearMotion( const CStudioHdr *pstudiohdr, sequence_t iSequence, const float poseParameter[], Vector *pVec );

const char *GetSequenceName( const CStudioHdr *pstudiohdr, sequence_t sequence );
const char *GetSequenceActivityName( const CStudioHdr *pstudiohdr, sequence_t iSequence );

SequenceFlags_t GetSequenceFlags( const CStudioHdr *pstudiohdr, sequence_t sequence );
int GetAnimationEvent( const CStudioHdr *pstudiohdr, sequence_t sequence, animevent_t *pNPCEvent, float flStart, float flEnd, int index );
bool HasAnimationEventOfType( const CStudioHdr *pstudiohdr, sequence_t sequence, int type );

sequence_t FindTransitionSequence( const CStudioHdr *pstudiohdr, sequence_t iCurrentSequence, sequence_t iGoalSequence, int *piDir );
bool GotoSequence( const CStudioHdr *pstudiohdr, int iCurrentSequence, float flCurrentCycle, float flCurrentRate, sequence_t iGoalSequence, sequence_t &nNextSequence, float &flNextCycle, int &iNextDir );

void SetBodygroup( const CStudioHdr *pstudiohdr, int& body, int iGroup, int iValue );
int GetBodygroup( const CStudioHdr *pstudiohdr, int body, int iGroup );

const char *GetBodygroupName( const CStudioHdr *pstudiohdr, int iGroup );
int FindBodygroupByName( const CStudioHdr *pstudiohdr, const char *name );
const char *GetBodygroupPartName( const CStudioHdr *pstudiohdr, int iGroup, int iPart );
int GetBodygroupCount( const CStudioHdr *pstudiohdr, int iGroup );
int GetNumBodyGroups( const CStudioHdr *pstudiohdr );

const char *GetBodygroupPartName( const CStudioHdr *pstudiohdr, int iGroup, int iPart );

Activity GetSequenceActivity( const CStudioHdr *pstudiohdr, sequence_t sequence, int *pweight = NULL );

void GetAttachmentLocalSpace( const CStudioHdr *pstudiohdr, int attachIndex, matrix3x4_t &pLocalToWorld );

float SetBlending( const CStudioHdr *pstudiohdr, sequence_t sequence, int *pblendings, int iBlender, float flValue );

int FindHitboxSetByName( const CStudioHdr *pstudiohdr, const char *name );
const char *GetHitboxSetName( const CStudioHdr *pstudiohdr, int setnumber );
int GetHitboxSetCount( const CStudioHdr *pstudiohdr );

#endif	//ANIMATION_H
