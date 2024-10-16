//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VPHYSICS_SOUND_H
#define VPHYSICS_SOUND_H
#pragma once

#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "vphysics_interface.h"

namespace physicssound
{
	struct impactsound_t
	{
		void			*pGameData;
		int				entityIndex;
		int				soundChannel;
		float			volume;
		float			impactSpeed;
		unsigned short	surfaceProps;
		unsigned short	surfacePropsHit;
		Vector			origin;
	};

	// UNDONE: Use a sorted container and sort by volume/distance?
	struct soundlist_t
	{
		CUtlVector<impactsound_t>	elements;
		impactsound_t	&GetElement(int index) { return elements[index]; }
		impactsound_t	&AddElement() { return elements[elements.AddToTail()]; }
		int Count() { return elements.Count(); }
		void RemoveAll() { elements.RemoveAll(); }
	};

	void PlayImpactSounds( soundlist_t &list );
	void AddImpactSound( soundlist_t &list, void *pGameData, int entityIndex, int soundChannel, IPhysicsObject *pObject, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed );

	struct breaksound_t
	{
		Vector			origin;
		int				surfacePropsBreak;
	};

	void AddBreakSound( CUtlVector<breaksound_t> &list, const Vector &origin, unsigned short surfaceProps );

	void PlayBreakSounds( CUtlVector<breaksound_t> &list );
};


#endif // VPHYSICS_SOUND_H
