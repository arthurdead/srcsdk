//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef VSMAYADMX_H
#define VSMAYADMX_H

#pragma once

// Maya includes

#include <maya/MDagPath.h>

// Valve includes

#include "movieobjects/dmemesh.h"

class VsMayaDmx
{
	static CDmeMesh *MayaMeshToDmeMesh(
		const MDagPath &i_mDagPath,
		DmFileId_t fileId );

};

#endif // VSMAYADMX_H