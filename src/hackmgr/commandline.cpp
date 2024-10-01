#include "hackmgr/hackmgr.h"
#include "tier0/icommandline.h"
#include "createinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

PLATFORM_INTERFACE ICommandLine *CommandLine_Tier0();

DLL_EXPORT ICommandLine *CommandLine()
{ return CommandLine_Tier0(); }

HACKMGR_EXECUTE_ON_LOAD_BEGIN(65535)

CommandLine()->AppendParm("-nop4", "");
CommandLine()->AppendParm("-enable_keyvalues_cache", "");

#ifndef SWDS
if(!IsDedicatedServer()) {
	CommandLine()->AppendParm("-gameuidll", "");
	CommandLine()->RemoveParm("-nogamepadui");
	CommandLine()->AppendParm("-gamepadui", "");
}
#endif

#ifdef _DEBUG
CommandLine()->AppendParm("-allowdebug", "");
CommandLine()->RemoveParm("-nodev");
CommandLine()->AppendParm("-dev", "");
CommandLine()->AppendParm("-internalbuild", "");
CommandLine()->AppendParm("-condebug", "");
CommandLine()->AppendParm("-conclearlog", "");
#else
CommandLine()->RemoveParm("-allowdebug");
CommandLine()->RemoveParm("-dev");
CommandLine()->AppendParm("-nodev", "");
CommandLine()->RemoveParm("-internalbuild");
CommandLine()->RemoveParm("-profile");
CommandLine()->AppendParm("-conclearlog", "");
#endif

#ifndef SWDS
if(!IsDedicatedServer()) {
	if(CommandLine()->ParmValue("-tools")) {
		CommandLine()->AppendParm("-edit", "");
	} else if(CommandLine()->ParmValue("-edit")) {
		CommandLine()->AppendParm("-tools", "");
	}
} else 
#endif
{
	CommandLine()->RemoveParm("-tools");
	CommandLine()->RemoveParm("-edit");
}

#ifdef __linux__
CommandLine()->RemoveParm("-edit");
#endif

CommandLine()->RemoveParm("+r_hunkalloclightmaps");
CommandLine()->RemoveParm("+developer");
CommandLine()->RemoveParm("+sv_cheats");

HACKMGR_EXECUTE_ON_LOAD_END
