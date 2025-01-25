//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "sequence_Transitioner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -----------------------------------------------------------------------------
// CSequenceTransitioner implementation.
// -----------------------------------------------------------------------------

void CSequenceTransitioner::CheckForSequenceChange( 
	const CStudioHdr *hdr,
	sequence_t nCurSequence, 
	bool bForceNewSequence,
	bool bInterpolate )
{
	// sequence may be set before model is initialized
	if ( hdr == NULL)
		return;

	// FIXME?: this should detect that what's been asked to be drawn isn't what was expected
	// due to not only sequence change, by frame index, rate, or whatever.  When that happens, 
	// it should insert the previous rules.

	if (m_animationQueue.Count() == 0)
	{
		m_animationQueue.AddToTail();
#ifdef CLIENT_DLL
		m_animationQueue[0].SetOwner( NULL );
#endif
	}

	CSharedAnimationLayer *currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	if (currentblend->m_flLayerAnimtime && 
		(currentblend->GetSequence() != nCurSequence || bForceNewSequence ))
	{
		if ( (unsigned short)nCurSequence >= hdr->GetNumSeq() )
		{
			// remove all entries
			m_animationQueue.RemoveAll();
		}
		else
		{
			const mstudioseqdesc_t &seqdesc = hdr->pSeqdesc( nCurSequence );
			// sequence changed
			if ((seqdesc.flags & STUDIO_SNAP) || !bInterpolate )
			{
				// remove all entries
				m_animationQueue.RemoveAll();
			}
			else
			{
				const mstudioseqdesc_t &prevseqdesc = hdr->pSeqdesc( currentblend->GetSequence() );
				currentblend->m_flLayerFadeOuttime = MIN( prevseqdesc.fadeouttime, seqdesc.fadeintime );
				/*
				// clip blends to time remaining
				if ( !IsSequenceLooping(hdr, currentblend->GetSequence()) )
				{
					float length = Studio_Duration( hdr, currentblend->GetSequence(), flPoseParameter ) / currentblend->m_flPlaybackRate;
					float timeLeft = (1.0 - currentblend->m_flCycle) * length;
					if (timeLeft < currentblend->m_flLayerFadeOuttime)
						currentblend->m_flLayerFadeOuttime = timeLeft;
				}
				*/
			}
		}
		// push previously set sequence
		m_animationQueue.AddToTail();
		currentblend = &m_animationQueue[m_animationQueue.Count()-1];
#ifdef CLIENT_DLL
		currentblend->SetOwner( NULL );
#endif
	}

	currentblend->SetSequence( INVALID_SEQUENCE );
	currentblend->m_flLayerAnimtime = 0.0;
	currentblend->m_flLayerFadeOuttime = 0.0;
}


void CSequenceTransitioner::UpdateCurrent( 
	const CStudioHdr *hdr,
	sequence_t nCurSequence, 
	float flCurCycle,
	float flCurPlaybackRate,
	float flCurTime )
{
	// sequence may be set before model is initialized
	if ( hdr == NULL)
		return;

	if (m_animationQueue.Count() == 0)
	{
		m_animationQueue.AddToTail();
#ifdef CLIENT_DLL
		m_animationQueue[0].SetOwner( NULL );
#endif
	}

	CSharedAnimationLayer *currentblend = &m_animationQueue[m_animationQueue.Count()-1];

	// keep track of current sequence
	currentblend->SetSequence( nCurSequence );
	currentblend->m_flLayerAnimtime = flCurTime;
	currentblend->SetCycle( flCurCycle );
	currentblend->SetPlaybackRate( flCurPlaybackRate );

	// calc blending weights for previous sequences
	int i;
	for (i = 0; i < m_animationQueue.Count() - 1;)
	{
 		float s = m_animationQueue[i].GetFadeout( flCurTime );

		if (s > 0)
		{
			m_animationQueue[i].SetWeight( s );
			i++;
		}
		else
		{
			m_animationQueue.Remove( i );
		}
	}
}
