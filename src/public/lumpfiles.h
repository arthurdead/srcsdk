//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef LUMPFILES_H
#define LUMPFILES_H
#pragma once

#define MAX_LUMPFILES	128

//-----------------------------------------------------------------------------
// Lump files
//-----------------------------------------------------------------------------
void GenerateLumpFileName( const char *bspfilename, char *lumpfilename, int iBufferSize, int iIndex );

#endif // LUMPFILES_H
