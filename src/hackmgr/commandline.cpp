#include "hackmgr/hackmgr.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_EXECUTE_ON_LOAD_BEGIN(0)

CommandLine()->AppendParm("-nop4", "");

CommandLine()->RemoveParm("+r_hunkalloclightmaps");
CommandLine()->AppendParm("+r_hunkalloclightmaps", "0");

HACKMGR_EXECUTE_ON_LOAD_END