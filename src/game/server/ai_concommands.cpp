//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Console commands for debugging and manipulating NPCs.
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "datacache/imdlcache.h"
#include "ai_addon.h"
#include "collisionproperty.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void			SetDebugBits( CBasePlayer* pPlayer, const char *name, int bit );
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

bool g_bAIDisabledByUser = false;


//------------------------------------------------------------------------------
// Purpose: Disables all NPCs
//------------------------------------------------------------------------------
void EnableAI()
{
	CAI_BaseNPC::m_nDebugBits &= ~bits_debugDisableAI;
	DevMsg("AI Enabled.\n");
	g_bAIDisabledByUser = false;
}

void DisableAI()
{
	CAI_BaseNPC::m_nDebugBits |= bits_debugDisableAI;
	DevMsg("AI Disabled.\n");
	g_bAIDisabledByUser = true;
}

void CC_AI_Disable( void )
{
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		EnableAI();
	}
	else
	{
		DisableAI();
	}

	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand ai_disable("ai_disable", CC_AI_Disable, "Bi-passes all AI logic routines and puts all NPCs into their idle animations.  Can be used to get NPCs out of your way and to test effect of AI logic routines on frame rate", FCVAR_CHEAT);

CON_COMMAND_F( ai_setenabled, "Like ai_disable but you manually specify the state (with a 0 or 1) instead of toggling it.", FCVAR_CHEAT )
{
	if ( args.ArgC() < 2 )
	{
		Warning( "ai_setenabled <0 or 1>\n" );
		return;
	}

	if ( args[1][0] == '0' )
		DisableAI();
	else
		EnableAI();

	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}

//------------------------------------------------------------------------------
// Purpose: NPC step trough AI
//------------------------------------------------------------------------------
void CC_AI_Step( void )
{
	DevMsg("AI Stepping...\n");

	// Start NPC's stepping through tasks
	CAI_BaseNPC::m_nDebugBits |= bits_debugStepAI;
	CAI_BaseNPC::m_nDebugPauseIndex++;
}
static ConCommand ai_step("ai_step", CC_AI_Step, "NPCs will freeze after completing their current task.  To complete the next task, use 'ai_step' again.  To resume processing normally use 'ai_resume'", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Resume normal AI processing after stepping
//------------------------------------------------------------------------------
void CC_AI_Resume( void )
{
	DevMsg("AI Resume...\n");

	// End NPC's stepping through tasks
	CAI_BaseNPC::m_nDebugBits &= ~bits_debugStepAI;
}
static ConCommand ai_resume("ai_resume", CC_AI_Resume, "If NPC is stepping through tasks (see ai_step ) will resume normal processing.", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show route triangulation attempts
//------------------------------------------------------------------------------
void CC_NPC_Bipass( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_TRIANGULATE_BIT);
}
static ConCommand npc_bipass("npc_bipass", CC_NPC_Bipass, "Displays the local movement attempts by the given NPC(s) (triangulation detours).  Failed bypass routes are displayed in red, successful bypasses are shown in green.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at.", FCVAR_CHEAT);
	
//------------------------------------------------------------------------------
// Purpose: Destroy selected NPC
//------------------------------------------------------------------------------
void CC_NPC_Destroy( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_ZAP_BIT);
}
static ConCommand npc_destroy("npc_destroy", CC_NPC_Destroy, "Removes the given NPC(s) from the universe\nArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CC_NPC_Kill( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_KILL_BIT);
}
static ConCommand npc_kill("npc_kill", CC_NPC_Kill, "Kills the given NPC(s)\nArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Show selected NPC's enemies
//------------------------------------------------------------------------------
void CC_NPC_Enemies( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_ENEMIES_BIT);
}
static ConCommand npc_enemies("npc_enemies", CC_NPC_Enemies, "Shows memory of NPC.  Draws an X on top of each memory.\n\tEluded entities drawn in blue (don't know where it went)\n\tUnreachable entities drawn in green (can't get to it)\n\tCurrent enemy drawn in red\n\tCurrent target entity drawn in magenta\n\tAll other entities drawn in pink\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show seletected NPC's current enemy and target entity
//------------------------------------------------------------------------------
void CC_NPC_Focus( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_FOCUS_BIT);
}
static ConCommand npc_focus("npc_focus", CC_NPC_Focus, "Displays red line to NPC's enemy (if has one) and blue line to NPC's target entity (if has one)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at", FCVAR_CHEAT);

ConVar npc_create_equipment("npc_create_equipment", "");

extern int EntityFactory_AutoComplete( const char *cmdname, CUtlVector< CUtlString > &commands, CUtlRBTree< CUtlString > &symbols, char *substring, int checklen = 0 );
extern bool UtlStringLessFunc( const CUtlString &lhs, const CUtlString &rhs );

//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
class CNPCCreateAutoCompletionFunctor : public ICommandCallback, public ICommandCompletionCallback
{
public:
	virtual bool CreateAimed() { return false; }

	virtual void CommandCallback( const CCommand &args )
	{
		MDLCACHE_CRITICAL_SECTION();

		bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );

		// Try to create entity
		CAI_BaseNPC *baseNPC = dynamic_cast< CAI_BaseNPC * >( CreateEntityByName(args[1]) );
		if (baseNPC)
		{
			baseNPC->KeyValue( "additionalequipment", npc_create_equipment.GetString() );

			if ( args.ArgC() == 3 )
			{
				baseNPC->SetName( AllocPooledString( args[2] ) );
			}
			else if ( args.ArgC() > 3 )
			{
				baseNPC->SetName( AllocPooledString( args[2] ) );

				// Pass in any additional parameters.
				for ( int i = 3; i + 1 < args.ArgC(); i += 2 )
				{
					const char *pKeyName = args[i];
					const char *pValue = args[i+1];
					baseNPC->KeyValue( pKeyName, pValue );
				}
			}

			DispatchSpawn(baseNPC);

			// Now attempt to drop into the world
			CBasePlayer* pPlayer = UTIL_GetCommandClient();
			trace_t tr;
			Vector forward;
			QAngle angles;
			pPlayer->EyeVectors( &forward );

			bool bCreateAimed = CreateAimed();
			if (bCreateAimed)
			{
				VectorAngles( forward, angles );
				angles.x = 0;
				angles.z = 0;
			}

			// Pass through the player's vehicle
			CTraceFilterSkipTwoEntities filter( pPlayer, pPlayer->GetVehicleEntity(), COLLISION_GROUP_NONE );
			AI_TraceLine(pPlayer->EyePosition(),
				pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
				&filter, &tr );

			if ( tr.fraction != 1.0)
			{
				if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
				{
					Vector pos = tr.endpos - forward * 36;
					baseNPC->Teleport( &pos, bCreateAimed ? &angles : NULL, NULL );
				}
				else
				{
					// Raise the end position a little up off the floor, place the npc and drop him down
					tr.endpos.z += 12;
					baseNPC->Teleport( &tr.endpos, bCreateAimed ? &angles : NULL, NULL );
					UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
				}

				// Now check that this is a valid location for the new npc to be
				Vector	vUpBit = baseNPC->GetAbsOrigin();
				vUpBit.z += 1;

				AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
					MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );

				// NEW: For vphysics/flying entities, do a second attempt which teleports based on bounding box
				if ( (baseNPC->GetMoveType() == MOVETYPE_VPHYSICS || baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY) && (tr.startsolid || tr.fraction < 1.0) )
				{
					vUpBit.z += baseNPC->BoundingRadius();
					baseNPC->Teleport( &vUpBit, NULL, NULL );
					UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );

					Vector	vUpBit2 = vUpBit;
					vUpBit2.z += 1;
					AI_TraceHull( vUpBit, vUpBit2, baseNPC->CollisionProp()->OBBMins(), baseNPC->CollisionProp()->OBBMaxs(),
						MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
				}

				if ( tr.startsolid || (tr.fraction < 1.0) )
				{
					baseNPC->SUB_Remove();
					DevMsg("Can't create %s.  Bad Position!\n",args[1]);
					NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
				}
			}
			else if (bCreateAimed)
			{
				baseNPC->Teleport( NULL, &angles, NULL );
			}

			baseNPC->Activate();
		}
		CBaseEntity::SetAllowPrecache( allowPrecache );
	}

	virtual int CommandCompletionCallback( const char *partial, CUtlVector< CUtlString > &commands )
	{
		if ( !GameRules() )
		{
			return 0;
		}

		const char *cmdname = CreateAimed() ? "npc_create_aimed" : "npc_create";

		char *substring = (char *)partial;
		if ( Q_strstr( partial, cmdname ) )
		{
			substring = (char *)partial + strlen( cmdname ) + 1;
		}

		int checklen = Q_strlen( substring );

		if (checklen <= 0)
		{
			// Only show classnames prefixed with "npc" unless the user starts typing other characters
			substring = "npc";
			checklen = 3;
		}

		CUtlRBTree< CUtlString > symbols( 0, 0, UtlStringLessFunc );
		return EntityFactory_AutoComplete( cmdname, commands, symbols, substring, checklen );
	}
};

static CNPCCreateAutoCompletionFunctor g_NPCCreateAutoComplete;
static ConCommand npc_create("npc_create", &g_NPCCreateAutoComplete, "Creates an NPC of the given type where the player is looking (if the given NPC can actually stand at that location).\n\tArguments:	{npc_class_name}", FCVAR_CHEAT, &g_NPCCreateAutoComplete);

class CNPCCreateAimedAutoCompletionFunctor : public CNPCCreateAutoCompletionFunctor
{
public:
	virtual bool CreateAimed() { return true; }
};

static CNPCCreateAimedAutoCompletionFunctor g_NPCCreateAimedAutoComplete;

static ConCommand npc_create_aimed("npc_create_aimed", &g_NPCCreateAimedAutoComplete, "Creates an NPC aimed away from the player of the given type where the player is looking (if the given NPC can actually stand at that location).\n\tArguments:	{npc_class_name}", FCVAR_CHEAT, &g_NPCCreateAimedAutoComplete);


//------------------------------------------------------------------------------
// Purpose: Destroy unselected NPCs
//------------------------------------------------------------------------------
void CC_NPC_DestroyUnselected( void )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) && !pNPC->ClassMatches("npc_bullseye"))
		{
			pNPC->m_debugOverlays |= OVERLAY_NPC_ZAP_BIT;
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}
static ConCommand npc_destroy_unselected("npc_destroy_unselected", CC_NPC_DestroyUnselected, "Removes all NPCs from the universe that aren't currently selected", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Freeze or unfreeze the selected NPCs. If no NPCs are selected, the
//			NPC under the crosshair is frozen/unfrozen.
//------------------------------------------------------------------------------
void CC_NPC_Freeze( const CCommand &args )
{
	if (FStrEq(args[1], "")) 
	{
		//	
		// No NPC was specified, try to freeze selected NPCs.
		//
		bool bFound = false;
		CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );
		while (npc)
		{
			if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
			{
				bFound = true;
				npc->ToggleFreeze();
			}
			npc = gEntList.NextEntByClass(npc);
		}

		if (!bFound)
		{
			//	
			// No selected NPCs, look for the NPC under the crosshair.
			//
			CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
			if ( pEntity )
			{
				CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
				if (pNPC)
				{
					pNPC->ToggleFreeze();
				}
			}
		}
	}
	else
	{
		// TODO: look for NPCs by name, classname.
	}
}
static ConCommand npc_freeze("npc_freeze", CC_NPC_Freeze, "Selected NPC(s) will freeze in place (or unfreeze). If there are no selected NPCs, uses the NPC under the crosshair.\n\tArguments:	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Freeze or unfreeze the selected NPCs. If no NPCs are selected, the
//			NPC under the crosshair is frozen/unfrozen.
//------------------------------------------------------------------------------
void CC_NPC_Set_Freeze( const CCommand &args )
{
	//	
	// No NPC was specified, try to freeze selected NPCs.
	//
	bool bFound = false;
	bool bFreeze = ( atoi( args[1] ) != 0 );
	
	CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );
	while (npc)
	{
		if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
		{
			bFound = true;
			if ( bFreeze )
			{
				npc->Freeze();
			}
			else
			{
				npc->Unfreeze();
			}
		}
		npc = gEntList.NextEntByClass(npc);
	}

	if (!bFound)
	{
		//	
		// No selected NPCs, look for the NPC under the crosshair.
		//
		CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		if ( pEntity )
		{
			CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
			if (pNPC)
			{
				if ( bFreeze )
				{
					npc->Freeze();
				}
				else
				{
					npc->Unfreeze();
				}
			}
		}
	}
}
static ConCommand npc_set_freeze("npc_set_freeze", CC_NPC_Set_Freeze, "Selected NPC(s) will freeze in place (or unfreeze). If there are no selected NPCs, uses the NPC under the crosshair.\n\tArguments:	-none-", FCVAR_CHEAT);

CON_COMMAND( npc_set_freeze_unselected, "Freeze all NPCs not selected" )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			if ( atoi( args[1] ) != 0 )
			{
				pNPC->Freeze();
			}
			else
			{
				pNPC->Unfreeze();
			}
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}

CON_COMMAND( npc_freeze_unselected, "Freeze all NPCs not selected" )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			pNPC->ToggleFreeze();
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}

//------------------------------------------------------------------------------
CON_COMMAND(npc_thinknow, "Trigger NPC to think")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			pNPC->SetThink( &CAI_BaseNPC::CallNPCThink );
			pNPC->SetNextThink( gpGlobals->curtime );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Tell selected NPC to go to a where player is looking
//------------------------------------------------------------------------------

void CC_NPC_Teleport( void )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	AI_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1.0)
	{
		CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

		while (npc)
		{
			//Only Teleport one NPC if more than one is selected.
			if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
			{
                npc->Teleport( &tr.endpos, NULL, NULL );
				break;
			}

			npc = gEntList.NextEntByClass(npc);
		}
	}
}

static ConCommand npc_teleport("npc_teleport", CC_NPC_Teleport, "Selected NPC will teleport to the location that the player is looking (shown with a purple box)\n\tArguments:	-none-", FCVAR_CHEAT);

static ConVar npc_go_do_run( "npc_go_do_run", "1", 0, "Set whether should run on NPC go" );

void CC_NPC_Go( void )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	AI_TraceLine(pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0)
	{
		CAI_BaseNPC::ForceSelectedGo(pPlayer, tr.endpos, forward, npc_go_do_run.GetBool());
	}
}
static ConCommand npc_go("npc_go", CC_NPC_Go, "Selected NPC(s) will go to the location that the player is looking (shown with a purple box)\n\tArguments:	-none-", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose: Tell seclected NPC to go to a random node location
//------------------------------------------------------------------------------
void CC_NPC_GoRandom( void )
{
	CAI_BaseNPC::ForceSelectedGoRandom();
}
static ConCommand npc_go_random("npc_go_random", CC_NPC_GoRandom, "Sends all selected NPC(s) to a random node.\n\tArguments:   	-none-", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: ?Does this work?
//------------------------------------------------------------------------------
void CC_NPC_Reset( void )
{
	CAI_BaseNPC::ClearAllSchedules();
	g_AI_SchedulesManager.LoadAllSchedules();
}
static ConCommand npc_reset("npc_reset", CC_NPC_Reset, "Reloads schedules for all NPC's from their script files\n\tArguments:	-none-", FCVAR_CHEAT);

extern bool UtlStringLessFunc( const CUtlString &lhs, const CUtlString &rhs );

//------------------------------------------------------------------------------
// Purpose : Auto-completes with entities in the entity list, but only uses NPC-derived entities.
// Input   : cmdname - The name of the command.
//			 &commands - Where the complete autocompletes should be sent to.
//			 substring - The current search query. (only pool entities that start with this)
//			 checklen - The number of characters to check.
// Output  : A pointer to a cUtlRBTRee containing all of the entities.
//------------------------------------------------------------------------------
static int AutoCompleteNPCs(const char *cmdname, CUtlVector< CUtlString > &commands, CUtlRBTree< CUtlString > &symbols, char *substring, int checklen = 0)
{
	CBaseEntity *pos = NULL;
	while ((pos = gEntList.NextEnt(pos)) != NULL)
	{
		if (!pos->IsNPC())
			continue;

		const char *name = pos->GetClassname();
		if (pos->GetEntityName() == NULL_STRING || Q_strnicmp(STRING(pos->GetEntityName()), substring, checklen))
		{
			if (Q_strnicmp(pos->GetClassname(), substring, checklen))
				continue;
		}
		else
			name = STRING(pos->GetEntityName());

		CUtlString sym = name;
		int idx = symbols.Find(sym);
		if (idx == symbols.InvalidIndex())
		{
			symbols.Insert(sym);
		}

		// Too many
		if (symbols.Count() >= COMMAND_COMPLETION_MAXITEMS)
			break;
	}

	// Now fill in the results
	for (int i = symbols.FirstInorder(); i != symbols.InvalidIndex(); i = symbols.NextInorder(i))
	{
		const char *name = symbols[i].String();

		char buf[512];
		Q_strncpy(buf, name, sizeof(buf));
		Q_strlower(buf);

		CUtlString command;
		command = CFmtStr("%s %s", cmdname, buf);
		commands.AddToTail(command);
	}

	return symbols.Count();
}

//------------------------------------------------------------------------------
// There's a big set of NPC debug commands that do similar operations and
// can fall under this base class for auto-completion, etc.
//------------------------------------------------------------------------------
class CNPCDebugAutoCompletionFunctor : public ICommandCallback, public ICommandCompletionCallback
{
public:
	virtual const char *CommandName() { return NULL; }
	virtual void CommandCallback( const CCommand &args )
	{
		SetDebugBits( UTIL_GetCommandClient(), args[1], OVERLAY_NPC_NEAREST_BIT );
	}

	virtual int CommandCompletionCallback( const char *partial, CUtlVector< CUtlString > &commands )
	{
		if ( !GameRules() )
		{
			return 0;
		}

		const char *cmdname = CommandName();

		char *substring = (char *)partial;
		if ( Q_strstr( partial, cmdname ) )
		{
			substring = (char *)partial + strlen( cmdname ) + 1;
		}

		int checklen = Q_strlen( substring );

		if (checklen == 0 || atoi(substring) != 0)
		{
			// Must be the picker or an entity index
			return 0;
		}

		CUtlRBTree< CUtlString > symbols( 0, 0, UtlStringLessFunc );
		return AutoCompleteNPCs(cmdname, commands, symbols, substring, checklen);
	}
};

#define NPCDebugCommand(name, functor, bit, help) class CNPC##functor##AutoCompletionFunctor : public CNPCDebugAutoCompletionFunctor \
{ \
public: \
	virtual const char *CommandName() { return #name; } \
	virtual void CommandCallback( const CCommand &args ) \
	{ \
		SetDebugBits( UTIL_GetCommandClient(), args[1], bit ); \
	} \
}; \
static CNPC##functor##AutoCompletionFunctor g_NPC##functor##AutoCompletionFunctor; \
static ConCommand name(#name, &g_NPC##functor##AutoCompletionFunctor, help, FCVAR_CHEAT, &g_NPC##functor##AutoCompletionFunctor);

NPCDebugCommand( npc_nearest, Nearest, OVERLAY_NPC_NEAREST_BIT, "Draw's a while box around the NPC(s) nearest node\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at  " );
NPCDebugCommand( npc_route, Route, OVERLAY_NPC_ROUTE_BIT, "Displays the current route of the given NPC as a line on the screen.  Waypoints along the route are drawn as small cyan rectangles.  Line is color coded in the following manner:\n\tBlue	- path to a node\n\tCyan	- detour around an object (triangulation)\n\tRed	- jump\n\tMaroon - path to final target position\n\tArguments:   	{npc_name} / {npc_class_name} / no argument picks what player is looking at " );
NPCDebugCommand( npc_select, Select, OVERLAY_NPC_SELECTED_BIT, "Select or deselects the given NPC(s) for later manipulation.  Selected NPC's are shown surrounded by a red translucent box\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at " );
NPCDebugCommand( npc_combat, Combat, OVERLAY_NPC_SQUAD_BIT, "Displays text debugging information about the squad and enemy of the selected NPC  (See Overlay Text)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at" );
NPCDebugCommand( npc_tasks, Tasks, OVERLAY_NPC_TASK_BIT, "Displays detailed text debugging information about the all the tasks of the selected NPC current schedule (See Overlay Text)\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at " );
NPCDebugCommand( npc_task_text, TaskText, OVERLAY_TASK_TEXT_BIT, "Outputs text debugging information to the console about the all the tasks + break conditions of the selected NPC current schedule\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at " );
NPCDebugCommand( npc_conditions, Conditions, OVERLAY_NPC_CONDITIONS_BIT, "Displays all the current AI conditions that an NPC has in the overlay text.\n\tArguments:   	{npc_name} / {npc class_name} / no argument picks what player is looking at" );
NPCDebugCommand( npc_viewcone, Viewcone, OVERLAY_NPC_VIEWCONE_BIT, "Displays the viewcone of the NPC (where they are currently looking and what the extents of there vision is)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at" );
NPCDebugCommand( npc_relationships, Relationships, OVERLAY_NPC_RELATION_BIT, "Displays the relationships between this NPC and all others.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at" );
NPCDebugCommand( npc_steering, Steering, OVERLAY_NPC_STEERING_REGULATIONS, "Displays the steering obstructions of the NPC( used to perform local avoidance )\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at" );

// For backwards compatibility
void CC_NPC_Squads( const CCommand &args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1],OVERLAY_NPC_SQUAD_BIT);
}
static ConCommand npc_squads("npc_squads", CC_NPC_Squads, "Obsolete.  Replaced by npc_combat", FCVAR_CHEAT);


void CC_NPC_ViewSteeringRegulationsAll( void )
{
	CAI_BaseNPC *pNPC = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (pNPC)
	{
		if (!(pNPC->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			pNPC->m_debugOverlays |= OVERLAY_NPC_STEERING_REGULATIONS;
		}
		else
		{
			pNPC->m_debugOverlays &= ~OVERLAY_NPC_STEERING_REGULATIONS;
		}
		pNPC = gEntList.NextEntByClass(pNPC);
	}
}
static ConCommand npc_steering_all("npc_steering_all", CC_NPC_ViewSteeringRegulationsAll, "Displays the steering obstructions of all NPCs (used to perform local avoidance)\n", FCVAR_CHEAT);

//------------------------------------------------------------------------------

CON_COMMAND( npc_heal, "Heals the target back to full health" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC)
		{
			pNPC->SetHealth( pNPC->GetMaxHealth() );
		}
	}
}

CON_COMMAND( npc_ammo_deplete, "Subtracts half of the target's ammo" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if (pNPC && pNPC->GetActiveWeapon())
		{
			pNPC->GetActiveWeapon()->m_iClip1 *= 0.5;
		}
	}
}

CON_COMMAND( ai_test_los, "Test AI LOS from the player's POV" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	trace_t tr;
	// Use the custom LOS trace filter
	CTraceFilterLOS traceFilter( UTIL_GetCommandClient(), COLLISION_GROUP_NONE );
	UTIL_TraceLine( UTIL_GetCommandClient()->EyePosition(), UTIL_GetCommandClient()->EyePosition() + UTIL_GetCommandClient()->EyeDirection3D() * MAX_COORD_RANGE, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr );
	NDebugOverlay::Line( UTIL_GetCommandClient()->EyePosition(), tr.endpos, 127, 127, 127, true, 5 );
	NDebugOverlay::Cross3D( tr.endpos, 24, 255, 255, 255, true, 5 );
}

#ifdef VPROF_ENABLED

CON_COMMAND(ainet_generate_report, "Generate a report to the console.")
{
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "AINet" );
}

CON_COMMAND(ainet_generate_report_only, "Generate a report to the console.")
{
	g_VProfCurrentProfile.OutputReport( VPRT_FULL, "AINet", g_VProfCurrentProfile.BudgetGroupNameToBudgetGroupID( "AINet" ) );
}

#endif
