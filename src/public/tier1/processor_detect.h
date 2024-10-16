//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: functions to expose CPU capabilities
//
// $NoKeywords: $
//=============================================================================//
#ifndef PROCESSOR_DETECT_H
#define PROCESSOR_DETECT_H

#pragma once

bool CheckMMXTechnology(void);
bool CheckSSETechnology(void);
bool CheckSSE2Technology(void);
bool Check3DNowTechnology(void);

#endif
