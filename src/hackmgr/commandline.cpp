#include "hackmgr/hackmgr.h"
#include "tier0/icommandline.h"
#include "createinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

PLATFORM_INTERFACE ICommandLine *CommandLine_Tier0();

DLL_EXPORT ICommandLine *CommandLine()
{ return CommandLine_Tier0(); }

#ifdef __MINGW32__
LIB_EXPORT SYMALIAS("CommandLine") ICommandLine *_imp__CommandLine();
#endif

static bool commandline_initalized = false;

void HackMgr_InitCommandLine()
{
	if(commandline_initalized)
		return;

	CommandLine()->AppendParm("-nop4", "");
	CommandLine()->AppendParm("-enable_keyvalues_cache", "");

	#ifndef SWDS
	if(IsDedicatedServer())
	#endif
	{
		CommandLine()->AppendParm("-dedicated", "");
	}

	#ifndef SWDS
	if(IsDedicatedServer() || CommandLine()->HasParm("-dedicated"))
	#endif
	{
		CommandLine()->AppendParm("-textmode", "");
	}

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
	//CommandLine()->AppendParm("-vguimessages", "");
	#else
	CommandLine()->RemoveParm("-allowdebug");
	CommandLine()->RemoveParm("-dev");
	CommandLine()->AppendParm("-nodev", "");
	CommandLine()->RemoveParm("-internalbuild");
	CommandLine()->RemoveParm("-profile");
	CommandLine()->AppendParm("-conclearlog", "");
	CommandLine()->RemoveParm("-vguimessages");
	#endif

	#ifndef SWDS
	if(!IsDedicatedServer()) {
		if(CommandLine()->HasParm("-textmode") || CommandLine()->HasParm("-dedicated")) {
			CommandLine()->RemoveParm("-edit_linux");
			CommandLine()->RemoveParm("-tools");
			CommandLine()->RemoveParm("-edit");
			CommandLine()->RemoveParm("-foundrymode");
		}
	}
	#endif

	#if !defined SWDS && defined __linux__
	if(!IsDedicatedServer()) {
		if(CommandLine()->HasParm("-edit_linux")) {
			if(!CommandLine()->HasParm("-dedicated")) {
				CommandLine()->AppendParm("-edit", "");
			}
			CommandLine()->RemoveParm("-edit_linux");
		}
	}
	#endif

	#ifndef SWDS
	if(!IsDedicatedServer() && !CommandLine()->HasParm("-dedicated")) {
		if(CommandLine()->HasParm("-edit") && !CommandLine()->HasParm("-foundrymode")) {
			CommandLine()->AppendParm("-foundrymode", "");
		} else if(CommandLine()->HasParm("-foundrymode") && !CommandLine()->HasParm("-edit")) {
			CommandLine()->AppendParm("-edit", "");
		}
	} else 
	#endif
	{
		CommandLine()->RemoveParm("-tools");
		CommandLine()->RemoveParm("-edit");
		CommandLine()->RemoveParm("-edit_linux");
		CommandLine()->RemoveParm("-foundrymode");
	}

	CommandLine()->RemoveParm("+r_hunkalloclightmaps");
	CommandLine()->RemoveParm("+developer");
	CommandLine()->RemoveParm("+sv_cheats");

	commandline_initalized = true;
}

HACKMGR_EXECUTE_ON_LOAD_BEGIN(0)

HackMgr_InitCommandLine();

HACKMGR_EXECUTE_ON_LOAD_END
