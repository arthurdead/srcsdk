#include "hackmgr/hackmgr.h"
#include "tier0/icommandline.h"
#include "createinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HACKMGR_EXECUTE_ON_LOAD_BEGIN(65535)

CommandLine()->AppendParm("-nop4", "");

#ifndef SWDS
if(!IsDedicatedServer()) {
	CommandLine()->RemoveParm("-nogamepadui");
	CommandLine()->AppendParm("-gamepadui", "");
}
#endif

#ifdef _DEBUG
CommandLine()->AppendParm("-allowdebug", "");
CommandLine()->AppendParm("-dev", "");
CommandLine()->AppendParm("-internalbuild", "");
CommandLine()->AppendParm("-console", "");
CommandLine()->AppendParm("-condebug", "");
CommandLine()->AppendParm("-conclearlog", "");
#else
CommandLine()->RemoveParm("-allowdebug");
CommandLine()->RemoveParm("-dev");
CommandLine()->RemoveParm("-internalbuild");
CommandLine()->RemoveParm("-profile");
CommandLine()->AppendParm("-conclearlog", "");
#endif

#ifdef SWDS
CommandLine()->RemoveParm("-tools");
CommandLine()->RemoveParm("-edit");
#else
if(IsDedicatedServer()) {
	CommandLine()->RemoveParm("-tools");
	CommandLine()->RemoveParm("-edit");
} else {
#ifdef _DEBUG
	CommandLine()->AppendParm("-tools", "");
	CommandLine()->AppendParm("-edit", "");
#else
	if(CommandLine()->ParmValue("-tools")) {
		CommandLine()->AppendParm("-edit", "");
	} else if(CommandLine()->ParmValue("-edit")) {
		CommandLine()->AppendParm("-tools", "");
	}
#endif
}
#endif

#ifdef __linux__
CommandLine()->RemoveParm("-edit");
#endif

CommandLine()->RemoveParm("+r_hunkalloclightmaps");
CommandLine()->RemoveParm("+developer");
CommandLine()->RemoveParm("+sv_cheats");

HACKMGR_EXECUTE_ON_LOAD_END
