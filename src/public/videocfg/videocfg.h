//===== Copyright © 2005-2008, Valve Corporation, All rights reserved. ======//
//
//
//===========================================================================//

#ifndef VIDEOCFG_H
#define VIDEOCFG_H

#pragma once

#include "tier1/utlvector.h"
#include "shaderapi/IShaderDevice.h"
#include "bittools.h"

class KeyValues;

struct VidMatConfigData_t
{
	char szFileName[MAX_PATH];
	char szPathID[64];
	KeyValues *pConfigKeys;
	int nVendorID;
	int nDeviceID;
	int nDXLevel;
	unsigned int nSystemMemory;
	unsigned int nVideoMemory;
	int nPhysicalScreenWidth;
	int nPhysicalScreenHeight;
	CUtlVector< ShaderDisplayMode_t > displayModes;
	bool bIsVideo;
};

enum CPULevel_t : unsigned char
{
	CPU_LEVEL_UNKNOWN = (unsigned char)-1,

	CPU_LEVEL_LOW = 0,
	CPU_LEVEL_MEDIUM,
	CPU_LEVEL_HIGH,

	CPU_LEVEL_COUNT,

	CPU_LEVEL_BIT_COUNT = MINIMUM_BITS_NEEDED(CPU_LEVEL_COUNT),
};

enum GPULevel_t : unsigned char
{
	GPU_LEVEL_UNKNOWN = (unsigned char)-1,

	GPU_LEVEL_LOW = 0,
	GPU_LEVEL_MEDIUM,
	GPU_LEVEL_HIGH,
	GPU_LEVEL_VERYHIGH,

	GPU_LEVEL_COUNT,

	GPU_LEVEL_BIT_COUNT = MINIMUM_BITS_NEEDED(GPU_LEVEL_COUNT),
};

enum MemLevel_t : unsigned char
{
	MEM_LEVEL_UNKNOWN = (unsigned char)-1,

	MEM_LEVEL_LOW = 0,
	MEM_LEVEL_MEDIUM,
	MEM_LEVEL_HIGH,

	MEM_LEVEL_COUNT,

	MEM_LEVEL_BIT_COUNT = MINIMUM_BITS_NEEDED(MEM_LEVEL_COUNT),
};

enum GPUMemLevel_t : unsigned char
{
	GPU_MEM_LEVEL_UNKNOWN = (unsigned char)-1,

	GPU_MEM_LEVEL_LOW = 0,
	GPU_MEM_LEVEL_MEDIUM,
	GPU_MEM_LEVEL_HIGH,

	GPU_MEM_LEVEL_COUNT,

	GPU_MEM_LEVEL_BIT_COUNT = MINIMUM_BITS_NEEDED(GPU_MEM_LEVEL_COUNT),
};

bool RecommendedConfig( VidMatConfigData_t &configData );
bool ResetVideoConfigToDefaults( KeyValues *pConfigKeys = NULL );
bool UpdateVideoConfigConVars( KeyValues *pConfigKeys = NULL );

bool ReadCurrentVideoConfig( KeyValues *pConfigKeys, bool bDefault = false );
bool UpdateCurrentVideoConfig( int nWidth, int nHeight, int nAspectRatioMode, bool bFullscreen, bool bNoWindowBorder );
void UpdateSystemLevel( CPULevel_t nCPULevel, GPULevel_t nGPULevel, MemLevel_t nMemLevel, GPUMemLevel_t nGPUMemLevel, const char *pModName );

#endif // VIDEOCFG_H