#ifndef AI_TASK_ENUM
	#define AI_TASK_ENUM(name, params, ...)
#endif

AI_TASK_ENUM(TASK_INVALID, TaskParamCheck_t(), 0)

// Forces the activity to reset.
AI_TASK_ENUM(TASK_RESET_ACTIVITY, TaskParamCheck_t())

// Waits for the specified number of seconds.
AI_TASK_ENUM(TASK_WAIT, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Make announce attack sound
AI_TASK_ENUM(TASK_ANNOUNCE_ATTACK, TaskParamCheck_t())

// Waits for the specified number of seconds. Will constantly turn to 
// face the enemy while waiting. 
AI_TASK_ENUM(TASK_WAIT_FACE_ENEMY, TaskParamCheck_t())

// Waits up to the specified number of seconds. Will constantly turn to 
// face the enemy while waiting. 
AI_TASK_ENUM(TASK_WAIT_FACE_ENEMY_RANDOM, TaskParamCheck_t())

// Wait until the player enters the same PVS as this character.
AI_TASK_ENUM(TASK_WAIT_PVS, TaskParamCheck_t())

// DON'T use this, it needs to go away. 
AI_TASK_ENUM(TASK_SUGGEST_STATE, TaskParamCheck_t(TASK_DATA_CHECK_NPCSTATE))

// Set m_hTargetEnt to nearest player
AI_TASK_ENUM(TASK_TARGET_PLAYER, TaskParamCheck_t())

// Walk to m_hTargetEnt's location
AI_TASK_ENUM(TASK_SCRIPT_WALK_TO_TARGET, TaskParamCheck_t())

// Run to m_hTargetEnt's location
AI_TASK_ENUM(TASK_SCRIPT_RUN_TO_TARGET, TaskParamCheck_t())

// Move to m_hTargetEnt's location using the activity specified by m_hCine->m_iszCustomMove.
AI_TASK_ENUM(TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET, TaskParamCheck_t())

// Move to within specified range of m_hTargetEnt
AI_TASK_ENUM(TASK_MOVE_TO_TARGET_RANGE, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Move to within specified range of our nav goal
AI_TASK_ENUM(TASK_MOVE_TO_GOAL_RANGE, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Path that moves the character a few steps forward of where it is.
AI_TASK_ENUM(TASK_MOVE_AWAY_PATH, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

AI_TASK_ENUM(TASK_GET_PATH_AWAY_FROM_BEST_SOUND, TaskParamCheck_t())

// Set the implied goal for TASK_GET_PATH_TO_GOAL
AI_TASK_ENUM(TASK_SET_GOAL, TaskParamCheck_t(TASK_DATA_CHECK_GOAL_TYPE))

// Get the path to the goal specified by TASK_SET_GOAL
AI_TASK_ENUM(TASK_GET_PATH_TO_GOAL, TaskParamCheck_t({TASK_DATA_CHECK_PATH_TYPE, TASK_DATA_CHECK_NUM}))

// Path to the enemy's location. Even if the enemy is unseen!
AI_TASK_ENUM(TASK_GET_PATH_TO_ENEMY, TaskParamCheck_t())

// Path to the last place this character saw the enemy
AI_TASK_ENUM(TASK_GET_PATH_TO_ENEMY_LKP, TaskParamCheck_t())

// Path to the enemy's location or path to a LOS with the enemy's last known position, depending on range
AI_TASK_ENUM(TASK_GET_CHASE_PATH_TO_ENEMY, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Path to a LOS with the enemy's last known position
AI_TASK_ENUM(TASK_GET_PATH_TO_ENEMY_LKP_LOS, TaskParamCheck_t())

// Path to the dead enemy's carcass.
AI_TASK_ENUM(TASK_GET_PATH_TO_ENEMY_CORPSE, TaskParamCheck_t())

// Path to the player's origin
AI_TASK_ENUM(TASK_GET_PATH_TO_PLAYER, TaskParamCheck_t())

// Path to node with line of sight to enemy
AI_TASK_ENUM(TASK_GET_PATH_TO_ENEMY_LOS, TaskParamCheck_t())

// Path to node with line of sight to enemy, at least flTaskData units away from m_vSavePosition
AI_TASK_ENUM(TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Path to node with line of sight to enemy, at least flTaskData degrees away from m_vSavePosition from the enemy's POV
AI_TASK_ENUM(TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Path to the within shot range of last place this character saw the enemy
AI_TASK_ENUM(TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Build a path to m_hTargetEnt
AI_TASK_ENUM(TASK_GET_PATH_TO_TARGET, TaskParamCheck_t())

// Allow a little slop, and allow for some Z offset (like the target is a gun on a table).
AI_TASK_ENUM(TASK_GET_PATH_TO_TARGET_WEAPON, TaskParamCheck_t())

// I'm on another NPC's head and I need to get down
AI_TASK_ENUM(TASK_GET_PATH_OFF_OF_NPC, TaskParamCheck_t())

AI_TASK_ENUM(TASK_CREATE_PENDING_WEAPON, TaskParamCheck_t())

// Path to nodes[ m_pHintNode ]
#ifndef AI_USES_NAV_MESH
AI_TASK_ENUM(TASK_GET_PATH_TO_HINTNODE)
#else
AI_TASK_ENUM(TASK_GET_PATH_TO_NAV_AREA, TaskParamCheck_t())
#endif

// Store current position for later reference
AI_TASK_ENUM(TASK_STORE_LASTPOSITION, TaskParamCheck_t())

// Clear stored position
AI_TASK_ENUM(TASK_CLEAR_LASTPOSITION, TaskParamCheck_t())

// Store current position for later reference
AI_TASK_ENUM(TASK_STORE_POSITION_IN_SAVEPOSITION, TaskParamCheck_t())

// Store best sound position for later reference
AI_TASK_ENUM(TASK_STORE_BESTSOUND_IN_SAVEPOSITION, TaskParamCheck_t())

AI_TASK_ENUM(TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION, TaskParamCheck_t())

AI_TASK_ENUM(TASK_REACT_TO_COMBAT_SOUND, TaskParamCheck_t())

// Store current enemy position in saveposition
AI_TASK_ENUM(TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION, TaskParamCheck_t())

// Move to the goal specified by the player in command mode.
AI_TASK_ENUM(TASK_GET_PATH_TO_COMMAND_GOAL, TaskParamCheck_t())

AI_TASK_ENUM(TASK_MARK_COMMAND_GOAL_POS, TaskParamCheck_t())

AI_TASK_ENUM(TASK_CLEAR_COMMAND_GOAL, TaskParamCheck_t())

// Path to last position (Last position must be stored with TASK_STORE_LAST_POSITION)
AI_TASK_ENUM(TASK_GET_PATH_TO_LASTPOSITION, TaskParamCheck_t())

// Path to saved position (Save position must by set in code or by a task)
AI_TASK_ENUM(TASK_GET_PATH_TO_SAVEPOSITION, TaskParamCheck_t())

// Path to location that has line of sight to saved position (Save position must by set in code or by a task)
AI_TASK_ENUM(TASK_GET_PATH_TO_SAVEPOSITION_LOS, TaskParamCheck_t())

// Path to random node
#ifndef AI_USES_NAV_MESH
AI_TASK_ENUM(TASK_GET_PATH_TO_RANDOM_NODE)
#else
AI_TASK_ENUM(TASK_GET_PATH_TO_RANDOM_AREA, TaskParamCheck_t())
#endif

// Path to source of loudest heard sound that I care about
AI_TASK_ENUM(TASK_GET_PATH_TO_BESTSOUND, TaskParamCheck_t())

// Path to source of the strongest scend that I care about
AI_TASK_ENUM(TASK_GET_PATH_TO_BESTSCENT, TaskParamCheck_t())

// Run the current path
AI_TASK_ENUM(TASK_RUN_PATH, TaskParamCheck_t())

// Walk the current path
AI_TASK_ENUM(TASK_WALK_PATH, TaskParamCheck_t())

// Walk the current path for a specified number of seconds
AI_TASK_ENUM(TASK_WALK_PATH_TIMED, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Walk the current path until you are x units from the goal.
AI_TASK_ENUM(TASK_WALK_PATH_WITHIN_DIST, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Walk the current path until for x units
AI_TASK_ENUM(TASK_WALK_PATH_FOR_UNITS, TaskParamCheck_t())

// Rung the current path until you are x units from the goal.
AI_TASK_ENUM(TASK_RUN_PATH_FLEE, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Run the current path for a specified number of seconds
AI_TASK_ENUM(TASK_RUN_PATH_TIMED, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Run the current path until for x units
AI_TASK_ENUM(TASK_RUN_PATH_FOR_UNITS, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Run the current path until you are x units from the goal.
AI_TASK_ENUM(TASK_RUN_PATH_WITHIN_DIST, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Walk the current path sideways (must be supported by animation)
AI_TASK_ENUM(TASK_STRAFE_PATH, TaskParamCheck_t())

// Clear m_flMoveWaitFinished (timer that inhibits movement)
AI_TASK_ENUM(TASK_CLEAR_MOVE_WAIT, TaskParamCheck_t())

// Decide on the appropriate small flinch animation, and play it. 
AI_TASK_ENUM(TASK_SMALL_FLINCH, TaskParamCheck_t())

// Decide on the appropriate big flinch animation, and play it. 
AI_TASK_ENUM(TASK_BIG_FLINCH, TaskParamCheck_t())

// Prevent dodging for a certain amount of time.
AI_TASK_ENUM(TASK_DEFER_DODGE, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Turn to face ideal yaw
AI_TASK_ENUM(TASK_FACE_IDEAL, TaskParamCheck_t())

// Find an interesting direction to face. Don't face into walls, corners if you can help it.
AI_TASK_ENUM(TASK_FACE_REASONABLE, TaskParamCheck_t())

// Turn to face the way I should walk or run
AI_TASK_ENUM(TASK_FACE_PATH, TaskParamCheck_t())

// Turn to face a player
AI_TASK_ENUM(TASK_FACE_PLAYER, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Turn to face the enemy
AI_TASK_ENUM(TASK_FACE_ENEMY, TaskParamCheck_t())

// Turn to face nodes[ m_pHintNode ]
#ifndef AI_USES_NAV_MESH
AI_TASK_ENUM(TASK_FACE_HINTNODE)

// Play activity associate with the current hint
AI_TASK_ENUM(TASK_PLAY_HINT_ACTIVITY)
#else
AI_TASK_ENUM(TASK_FACE_NAV_AREA, TaskParamCheck_t())

// Play activity associate with the current hint
AI_TASK_ENUM(TASK_PLAY_AREA_ACTIVITY, TaskParamCheck_t())
#endif

// Turn to face m_hTargetEnt
AI_TASK_ENUM(TASK_FACE_TARGET, TaskParamCheck_t())

// Turn to face stored last position (last position must be stored first!)
AI_TASK_ENUM(TASK_FACE_LASTPOSITION, TaskParamCheck_t())

// Turn to face stored save position (save position must be stored first!)
AI_TASK_ENUM(TASK_FACE_SAVEPOSITION, TaskParamCheck_t())

// Turn to face directly away from stored save position (save position must be stored first!)
AI_TASK_ENUM(TASK_FACE_AWAY_FROM_SAVEPOSITION, TaskParamCheck_t())

// Set the current facing to be the ideal
AI_TASK_ENUM(TASK_SET_IDEAL_YAW_TO_CURRENT, TaskParamCheck_t())

// Attack the enemy (should be facing the enemy)
AI_TASK_ENUM(TASK_RANGE_ATTACK1, TaskParamCheck_t())

AI_TASK_ENUM(TASK_RANGE_ATTACK2, TaskParamCheck_t())

AI_TASK_ENUM(TASK_MELEE_ATTACK1, TaskParamCheck_t())

AI_TASK_ENUM(TASK_MELEE_ATTACK2, TaskParamCheck_t())

// Reload weapon
AI_TASK_ENUM(TASK_RELOAD, TaskParamCheck_t())

// Execute special attack (user-defined)
AI_TASK_ENUM(TASK_SPECIAL_ATTACK1, TaskParamCheck_t())

AI_TASK_ENUM(TASK_SPECIAL_ATTACK2, TaskParamCheck_t())

#ifndef AI_USES_NAV_MESH
AI_TASK_ENUM(TASK_FIND_HINTNODE)

AI_TASK_ENUM(TASK_FIND_LOCK_HINTNODE)

AI_TASK_ENUM(TASK_CLEAR_HINTNODE)

// Claim m_pHintNode exclusively for this NPC.
AI_TASK_ENUM(TASK_LOCK_HINTNODE)
#else
AI_TASK_ENUM(TASK_FIND_NAV_AREA, TaskParamCheck_t())

AI_TASK_ENUM(TASK_FIND_LOCK_NAV_AREA, TaskParamCheck_t())

AI_TASK_ENUM(TASK_CLEAR_NAV_AREA, TaskParamCheck_t())

// Claim m_pHintNode exclusively for this NPC.
AI_TASK_ENUM(TASK_LOCK_NAV_AREA, TaskParamCheck_t())
#endif

// Emit an angry sound
AI_TASK_ENUM(TASK_SOUND_ANGRY, TaskParamCheck_t())

// Emit a dying sound
AI_TASK_ENUM(TASK_SOUND_DEATH, TaskParamCheck_t())

// Emit an idle sound
AI_TASK_ENUM(TASK_SOUND_IDLE, TaskParamCheck_t())

// Emit a sound because you are pissed off because you just saw someone you don't like
AI_TASK_ENUM(TASK_SOUND_WAKE, TaskParamCheck_t())

// Emit a pain sound
AI_TASK_ENUM(TASK_SOUND_PAIN, TaskParamCheck_t())

// Emit a death sound
AI_TASK_ENUM(TASK_SOUND_DIE, TaskParamCheck_t())

// Speak a sentence
AI_TASK_ENUM(TASK_SPEAK_SENTENCE, TaskParamCheck_t(TASK_DATA_CHECK_INT))

// Wait for the current sentence I'm speaking to finish
AI_TASK_ENUM(TASK_WAIT_FOR_SPEAK_FINISH, TaskParamCheck_t(TASK_DATA_CHECK_BOOL))

// Set current animation activity to the specified activity
AI_TASK_ENUM(TASK_SET_ACTIVITY, TaskParamCheck_t(TASK_DATA_CHECK_ACTIVITY))

// Adjust the framerate to plus/minus N%
AI_TASK_ENUM(TASK_RANDOMIZE_FRAMERATE, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Immediately change to a schedule of the specified type
AI_TASK_ENUM(TASK_SET_SCHEDULE, TaskParamCheck_t(TASK_DATA_CHECK_SCHEDULE_ID))

// Set the specified schedule to execute if the current schedule fails.
AI_TASK_ENUM(TASK_SET_FAIL_SCHEDULE, TaskParamCheck_t(TASK_DATA_CHECK_SCHEDULE_ID))

// How close to route goal do I need to get
AI_TASK_ENUM(TASK_SET_TOLERANCE_DISTANCE, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// How many seconds should I spend search for a route
AI_TASK_ENUM(TASK_SET_ROUTE_SEARCH_TIME, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Return to use of default fail schedule
AI_TASK_ENUM(TASK_CLEAR_FAIL_SCHEDULE, TaskParamCheck_t())

// Play the specified animation sequence before continuing
AI_TASK_ENUM(TASK_PLAY_SEQUENCE, TaskParamCheck_t(TASK_DATA_CHECK_ACTIVITY))

// Play the specified private animation sequence before continuing
AI_TASK_ENUM(TASK_PLAY_PRIVATE_SEQUENCE, TaskParamCheck_t())

// Turn to face the enemy while playing specified animation sequence
AI_TASK_ENUM(TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY, TaskParamCheck_t())

AI_TASK_ENUM(TASK_PLAY_SEQUENCE_FACE_ENEMY, TaskParamCheck_t())

AI_TASK_ENUM(TASK_PLAY_SEQUENCE_FACE_TARGET, TaskParamCheck_t())

// tries lateral cover first, then node cover
AI_TASK_ENUM(TASK_FIND_COVER_FROM_BEST_SOUND, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// tries lateral cover first, then node cover
AI_TASK_ENUM(TASK_FIND_COVER_FROM_ENEMY, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Find a place to hide from the enemy, somewhere on either side of me
AI_TASK_ENUM(TASK_FIND_LATERAL_COVER_FROM_ENEMY, TaskParamCheck_t())

// Find a place further from the saved position
AI_TASK_ENUM(TASK_FIND_BACKAWAY_FROM_SAVEPOSITION, TaskParamCheck_t())

#ifndef AI_USES_NAV_MESH
// Fine a place to hide from the enemy, anywhere. Use the node system.
AI_TASK_ENUM(TASK_FIND_NODE_COVER_FROM_ENEMY)

// Find a place to hide from the enemy that's within the specified distance
AI_TASK_ENUM(TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY)

// data for this one is there MINIMUM aceptable distance to the cover.
AI_TASK_ENUM(TASK_FIND_FAR_NODE_COVER_FROM_ENEMY)
#else
// Fine a place to hide from the enemy, anywhere. Use the node system.
AI_TASK_ENUM(TASK_FIND_AREA_COVER_FROM_ENEMY, TaskParamCheck_t())

// Find a place to hide from the enemy that's within the specified distance
AI_TASK_ENUM(TASK_FIND_NEAR_AREA_COVER_FROM_ENEMY, TaskParamCheck_t())

// data for this one is there MINIMUM aceptable distance to the cover.
AI_TASK_ENUM(TASK_FIND_FAR_AREA_COVER_FROM_ENEMY, TaskParamCheck_t())
#endif

// Find a place to go that can't see to where I am now.
AI_TASK_ENUM(TASK_FIND_COVER_FROM_ORIGIN, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Unhook from the AI system.
AI_TASK_ENUM(TASK_DIE, TaskParamCheck_t())

// Wait until scripted sequence plays
AI_TASK_ENUM(TASK_WAIT_FOR_SCRIPT, TaskParamCheck_t())

// Play scripted sequence animation
AI_TASK_ENUM(TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY, TaskParamCheck_t())

AI_TASK_ENUM(TASK_PLAY_SCRIPT, TaskParamCheck_t())

AI_TASK_ENUM(TASK_PLAY_SCRIPT_POST_IDLE, TaskParamCheck_t())

AI_TASK_ENUM(TASK_ENABLE_SCRIPT, TaskParamCheck_t())

AI_TASK_ENUM(TASK_PLANT_ON_SCRIPT, TaskParamCheck_t())

AI_TASK_ENUM(TASK_FACE_SCRIPT, TaskParamCheck_t())

// Wait for scene to complete
AI_TASK_ENUM(TASK_PLAY_SCENE, TaskParamCheck_t())

// Wait for 0 to specified number of seconds
AI_TASK_ENUM(TASK_WAIT_RANDOM, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Wait forever (until this schedule is interrupted)
AI_TASK_ENUM(TASK_WAIT_INDEFINITE, TaskParamCheck_t())

AI_TASK_ENUM(TASK_STOP_MOVING, TaskParamCheck_t(TASK_DATA_CHECK_BOOL))

// Turn left the specified number of degrees
AI_TASK_ENUM(TASK_TURN_LEFT, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Turn right the specified number of degrees
AI_TASK_ENUM(TASK_TURN_RIGHT, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Remember the specified piece of data
AI_TASK_ENUM(TASK_REMEMBER, TaskParamCheck_t(TASK_DATA_CHECK_MEMORY_ID))

// Forget the specified piece of data
AI_TASK_ENUM(TASK_FORGET, TaskParamCheck_t(TASK_DATA_CHECK_MEMORY_ID))

// Wait until current movement is complete. 
AI_TASK_ENUM(TASK_WAIT_FOR_MOVEMENT, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Wait until a single-step movement is complete.
AI_TASK_ENUM(TASK_WAIT_FOR_MOVEMENT_STEP, TaskParamCheck_t())

// Wait until I can't hear any danger sound.
AI_TASK_ENUM(TASK_WAIT_UNTIL_NO_DANGER_SOUND, TaskParamCheck_t())

// Pick up new weapons:
AI_TASK_ENUM(TASK_WEAPON_FIND, TaskParamCheck_t())

AI_TASK_ENUM(TASK_WEAPON_PICKUP, TaskParamCheck_t())

AI_TASK_ENUM(TASK_WEAPON_RUN_PATH, TaskParamCheck_t())

// run to weapon but break if someone else picks it up
AI_TASK_ENUM(TASK_WEAPON_CREATE, TaskParamCheck_t())

AI_TASK_ENUM(TASK_ITEM_PICKUP, TaskParamCheck_t())

AI_TASK_ENUM(TASK_ITEM_RUN_PATH, TaskParamCheck_t())

// Use small hull for tight navigation
AI_TASK_ENUM(TASK_USE_SMALL_HULL, TaskParamCheck_t())

// wait until you are on ground
AI_TASK_ENUM(TASK_FALL_TO_GROUND, TaskParamCheck_t())

// Wander for a specfied amound of time
AI_TASK_ENUM(TASK_WANDER, TaskParamCheck_t({TASK_DATA_CHECK_NUM, TASK_DATA_CHECK_NUM}))

AI_TASK_ENUM(TASK_FREEZE, TaskParamCheck_t())

// regather conditions at the start of a schedule (all conditions are cleared between schedules)
AI_TASK_ENUM(TASK_GATHER_CONDITIONS, TaskParamCheck_t())

// Require an enemy be seen after the task is run to be considered a candidate enemy
AI_TASK_ENUM(TASK_IGNORE_OLD_ENEMIES, TaskParamCheck_t())

AI_TASK_ENUM(TASK_DEBUG_BREAK, TaskParamCheck_t())

// Add a specified amount of health to this NPC
AI_TASK_ENUM(TASK_ADD_HEALTH, TaskParamCheck_t(TASK_DATA_CHECK_NUM))

// Add a gesture layer and wait until it's finished
AI_TASK_ENUM(TASK_ADD_GESTURE_WAIT, TaskParamCheck_t(TASK_DATA_CHECK_ACTIVITY))

// Add a gesture layer
AI_TASK_ENUM(TASK_ADD_GESTURE, TaskParamCheck_t(TASK_DATA_CHECK_ACTIVITY))

// Get a path to my forced interaction partner
AI_TASK_ENUM(TASK_GET_PATH_TO_INTERACTION_PARTNER, TaskParamCheck_t())

// First task of all schedules for playing back scripted sequences
AI_TASK_ENUM(TASK_PRE_SCRIPT, TaskParamCheck_t())

// Faces the actual interaction angles instead of just facing the enemy
AI_TASK_ENUM(TASK_FACE_INTERACTION_ANGLES, TaskParamCheck_t())

#undef AI_TASK_ENUM