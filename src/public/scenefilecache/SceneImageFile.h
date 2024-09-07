//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A Scene Image file aggregates all the compiled binary VCD files into
// a single file.
//
//=====================================================================================//
#ifndef SCENE_IMAGE_FILE_H
#define SCENE_IMAGE_FILE_H
#pragma once

#include "commonmacros.h"
#include "tier1/checksum_crc.h"
#include "tier0/dbg.h"

#define SCENE_IMAGE_ID			MAKEID( 'V','S','I','F' )
#define SCENE_IMAGE_VERSION		2

// scene summary: cached calcs for commmon startup queries, variable sized
struct SceneImageSummary_t
{
	unsigned int	msecs;
	int				numSounds;
	int				soundStrings[1];	// has numSounds
};

struct SceneImageSummaryEx_t : public SceneImageSummary_t
{
	unsigned int	lastspeech_msecs; ///< milliseconds from beginning of vcd to end of last speak event. 

	// return time in seconds from beginning of scene to end of last Speak event
	inline float GetDurToSpeechEnd( void ) const  { return lastspeech_msecs * 0.001f; }
};

// stored sorted by crc filename for binary search
struct SceneImageEntry_t
{
	CRC32_t	crcFilename;			// expected to be normalized as scenes\???.vcd
	int		nDataOffset;			// offset to dword aligned data from start
	int		nDataLength;
	int		nSceneSummaryOffset;	// offset to summary
};

struct SceneImageHeader_t
{
	int nId;
	int	nVersion;
	int nNumScenes;				// number of scene files
	int	nNumStrings;			// number of unique strings in table
	int nSceneEntryOffset;

	inline const char *String( short iString )
	{
		if ( iString < 0 || iString >= nNumStrings )
		{
			Assert( 0 );
			return NULL;
		}

		// access string table (after header) to access pool
		unsigned int *pTable = (unsigned int *)((byte *)this + sizeof( SceneImageHeader_t ));
		return (char *)this + pTable[iString];
	}
};

#endif // SCENE_IMAGE_FILE_H
