#ifndef AI_COND_ENUM
	#define AI_COND_ENUM(name, ...)
#endif

AI_COND_ENUM(COND_NONE)
// A way for a function to return no condition to get

AI_COND_ENUM(COND_IN_PVS)

AI_COND_ENUM(COND_IDLE_INTERRUPT)
// The schedule in question is a low priority idle, and therefore a candidate for translation into something else

AI_COND_ENUM(COND_LOW_PRIMARY_AMMO)

AI_COND_ENUM(COND_NO_PRIMARY_AMMO)

AI_COND_ENUM(COND_NO_SECONDARY_AMMO)

AI_COND_ENUM(COND_NO_WEAPON)

AI_COND_ENUM(COND_SEE_HATE)

AI_COND_ENUM(COND_SEE_FEAR)

AI_COND_ENUM(COND_SEE_DISLIKE)

AI_COND_ENUM(COND_SEE_ENEMY)

AI_COND_ENUM(COND_LOST_ENEMY)

AI_COND_ENUM(COND_ENEMY_WENT_NULL)
// What most people think COND_LOST_ENEMY is: This condition is set in the edge case where you had an enemy last think, but don't have one this think.

AI_COND_ENUM(COND_ENEMY_OCCLUDED)
// Can't see m_hEnemy
AI_COND_ENUM(COND_TARGET_OCCLUDED)
// Can't see m_hTargetEnt

AI_COND_ENUM(COND_HAVE_ENEMY_LOS)

AI_COND_ENUM(COND_HAVE_TARGET_LOS)

AI_COND_ENUM(COND_LIGHT_DAMAGE)

AI_COND_ENUM(COND_HEAVY_DAMAGE)

AI_COND_ENUM(COND_PHYSICS_DAMAGE)

AI_COND_ENUM(COND_REPEATED_DAMAGE)
//  Damaged several times in a row

AI_COND_ENUM(COND_CAN_RANGE_ATTACK1)
// Hitscan weapon only
AI_COND_ENUM(COND_CAN_RANGE_ATTACK2)
// Grenade weapon only

AI_COND_ENUM(COND_CAN_MELEE_ATTACK1)

AI_COND_ENUM(COND_CAN_MELEE_ATTACK2)

AI_COND_ENUM(COND_PROVOKED)

AI_COND_ENUM(COND_NEW_ENEMY)

AI_COND_ENUM(COND_ENEMY_TOO_FAR)
//	Can we get rid of this one!?!?

AI_COND_ENUM(COND_ENEMY_FACING_ME)

AI_COND_ENUM(COND_BEHIND_ENEMY)

AI_COND_ENUM(COND_ENEMY_DEAD)

AI_COND_ENUM(COND_ENEMY_UNREACHABLE)
// Not connected to me via node graph

AI_COND_ENUM(COND_SEE_PLAYER)

AI_COND_ENUM(COND_LOST_PLAYER)

AI_COND_ENUM(COND_SEE_NEMESIS)

AI_COND_ENUM(COND_TASK_FAILED)

AI_COND_ENUM(COND_SCHEDULE_DONE)

AI_COND_ENUM(COND_SMELL)

AI_COND_ENUM(COND_TOO_CLOSE_TO_ATTACK)
// FIXME: most of this next group are meaningless since they're shared between all attack checks!

AI_COND_ENUM(COND_TOO_FAR_TO_ATTACK)

AI_COND_ENUM(COND_NOT_FACING_ATTACK)

AI_COND_ENUM(COND_WEAPON_HAS_LOS)

AI_COND_ENUM(COND_WEAPON_BLOCKED_BY_FRIEND)
// Friend between weapon and target

AI_COND_ENUM(COND_WEAPON_PLAYER_IN_SPREAD)
// Player in shooting direction

AI_COND_ENUM(COND_WEAPON_PLAYER_NEAR_TARGET)
// Player near shooting position

AI_COND_ENUM(COND_WEAPON_SIGHT_OCCLUDED)

AI_COND_ENUM(COND_BETTER_WEAPON_AVAILABLE)

AI_COND_ENUM(COND_HEALTH_ITEM_AVAILABLE)
// There's a healthkit available.

AI_COND_ENUM(COND_GIVE_WAY)
// Another npc requested that I give way

AI_COND_ENUM(COND_WAY_CLEAR)
// I no longer have to give way

AI_COND_ENUM(COND_HEAR_DANGER)

AI_COND_ENUM(COND_HEAR_THUMPER)

AI_COND_ENUM(COND_HEAR_BUGBAIT)

AI_COND_ENUM(COND_HEAR_COMBAT)

AI_COND_ENUM(COND_HEAR_WORLD)

AI_COND_ENUM(COND_HEAR_PLAYER)

AI_COND_ENUM(COND_HEAR_BULLET_IMPACT)

AI_COND_ENUM(COND_HEAR_PHYSICS_DANGER)

AI_COND_ENUM(COND_HEAR_MOVE_AWAY)

AI_COND_ENUM(COND_HEAR_SPOOKY)
// Zombies make this when Alyx is in darkness mode

AI_COND_ENUM(COND_NO_HEAR_DANGER)
// Since we can't use ~CONDITION. Mutually exclusive with COND_HEAR_DANGER

AI_COND_ENUM(COND_FLOATING_OFF_GROUND)

AI_COND_ENUM(COND_MOBBED_BY_ENEMIES)
// Surrounded by a large number of enemies melee attacking me. (Zombies or Antlions, usually).

// Commander stuff
AI_COND_ENUM(COND_RECEIVED_ORDERS)

AI_COND_ENUM(COND_PLAYER_ADDED_TO_SQUAD)

AI_COND_ENUM(COND_PLAYER_REMOVED_FROM_SQUAD)

AI_COND_ENUM(COND_PLAYER_PUSHING)

AI_COND_ENUM(COND_NPC_FREEZE)
// We received an npc_freeze command while we were unfrozen

AI_COND_ENUM(COND_NPC_UNFREEZE)
// We received an npc_freeze command while we were frozen

// This is a talker condition, but done here because we need to handle it in base AI
// due to it's interaction with behaviors.
AI_COND_ENUM(COND_TALKER_RESPOND_TO_QUESTION)

AI_COND_ENUM(COND_NO_CUSTOM_INTERRUPTS)
// Don't call BuildScheduleTestBits for this schedule. Used for schedules that must strictly control their interruptibility.

AI_COND_ENUM(COND_BEHAVIOR_PARAMETERS_CHANGED)

#undef AI_COND_ENUM