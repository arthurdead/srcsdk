//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMERULES_H
#define GAMERULES_H
#pragma once

#ifdef CLIENT_DLL
	#include "c_baseentity.h"
#else
	#include "baseentity.h"
	#include "recipientfilter.h"
#endif

#include "igamesystem.h"
#include "gamerules_register.h"
#include "baseentity_shared.h"
#include "GameEventListener.h"
#include "dt_shared.h"

//#include "items.h"

DECLARE_LOGGING_CHANNEL( LOG_GAMERULES );

#ifdef CLIENT_DLL
class C_GameRules;
typedef C_GameRules CSharedGameRules;
class C_GameRulesProxy;
typedef C_GameRulesProxy CSharedGameRulesProxy;
#else
class CGameRules;
typedef CGameRules CSharedGameRules;
class CGameRulesProxy;
typedef CGameRulesProxy CSharedGameRulesProxy;
#endif

#ifdef GAME_DLL
class CBaseCombatWeapon;
typedef CBaseCombatWeapon CSharedBaseCombatWeapon;
class CBaseCombatCharacter;
typedef CBaseCombatCharacter CSharedBaseCombatCharacter;
class CBasePlayer;
typedef CBasePlayer CSharedBasePlayer;
class CItem;
#else
class C_BaseCombatWeapon;
typedef C_BaseCombatWeapon CSharedBaseCombatWeapon;
class C_BaseCombatCharacter;
typedef C_BaseCombatCharacter CSharedBaseCombatCharacter;
class C_BasePlayer;
typedef C_BasePlayer CSharedBasePlayer;
#endif

class CAmmoDef;

#ifdef GAME_DLL
class CTacticalMissionManager;
class CBaseExpresserPlayer;
#endif

extern ConVar sk_autoaim_mode;

#ifndef CLIENT_DLL

extern ConVar mp_restartgame;
extern ConVar mp_restartgame_immediate;
extern ConVar mp_waitingforplayers_time;
extern ConVar mp_waitingforplayers_restart;
extern ConVar mp_waitingforplayers_cancel;
extern ConVar mp_clan_readyrestart;
extern ConVar mp_clan_ready_signal;
extern ConVar nextlevel;
extern INetworkStringTable *g_pStringTableServerMapCycle;

#define VOICE_COMMAND_MAX_SUBTITLE_DIST	1900

#endif

extern ConVar mp_show_voice_icons;

#define MAX_SPEAK_CONCEPT_LEN 64
#define MAX_VOICE_COMMAND_SUBTITLE	256

typedef struct
{
#ifndef CLIENT_DLL
	// concept to speak
	int	 m_iConcept;

	// play subtitle?
	bool m_bShowSubtitle;
	bool m_bDistanceBasedSubtitle;

	char m_szGestureActivity[64];

#else
	// localizable subtitle
	char m_szSubtitle[MAX_VOICE_COMMAND_SUBTITLE];

	// localizable string for menu
	char m_szMenuLabel[MAX_VOICE_COMMAND_SUBTITLE];
#endif

} VoiceCommandMenuItem_t;

extern ConVar mp_timelimit;

class CSameTeamGroup
{
public:
	CSameTeamGroup();
	CSameTeamGroup( const CSameTeamGroup &src );

	// Different users will require different logic for whom to add (e.g., all SplitScreen players on same team, or all Steam Friends on same team)
	virtual void Build( CSharedGameRules *pGameRules, CSharedBasePlayer *pl ) = 0;
	virtual void MaybeAddPlayer( CSharedBasePlayer *pl ) = 0;

	CSharedBasePlayer *GetPlayer( int idx );

	int						Count() const;
	int						Score() const;

	static bool Less( const CSameTeamGroup &p1, const CSameTeamGroup &p2 );

protected:

	CUtlVector< CSharedBasePlayer * >				m_Players;
	int										m_nScore;
};

// Autoaiming modes
enum
{
	AUTOAIM_NONE = 0,		// No autoaim at all.
	AUTOAIM_ON,				// Autoaim is on.
	AUTOAIM_EXTRA,		// Autoaim is on, including enhanced features for Console gaming (more assistance, etc)
};

// weapon respawning return codes
enum
{	
	GR_NONE = 0,
	
	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,
	
	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,
	
	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL,
};

#ifdef CLIENT_DLL
	#define CGameRulesProxy C_GameRulesProxy
#endif

// This class has the data tables and gets the CGameRules data to the client.
class CGameRulesProxy : public CSharedLogicalEntity
{
public:
	DECLARE_CLASS( CGameRulesProxy, CSharedLogicalEntity );
	CGameRulesProxy();
	~CGameRulesProxy();

#ifdef CLIENT_DLL
	#undef CGameRulesProxy
#endif

	DECLARE_NETWORKCLASS();

	virtual void Spawn( void );

	// CGameRules chains its NetworkStateChanged calls to here, since this
	// is the actual entity that will send the data.
	static void NotifyNetworkStateChanged();

private:
#ifdef CLIENT_DLL
	friend class C_GameRules;
#else
	friend class CGameRules;
#endif
};

#ifdef CLIENT_DLL
typedef C_GameRulesProxy CSharedGameRulesProxy;
#else
typedef CGameRulesProxy CSharedGameRulesProxy;
#endif

extern CSharedGameRulesProxy *g_pGameRulesProxy;

#ifdef CLIENT_DLL
	#define CGameRules C_GameRules
#endif

abstract_class CGameRules : public CMemZeroOnNew, public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	DECLARE_CLASS_GAMEROOT( CGameRules, CAutoGameSystemPerFrame );
	CGameRules(void);
	virtual ~CGameRules( void );
	virtual char const *Name() { return V_STRINGIFY(CGameRules); }

#ifdef CLIENT_DLL
	#undef CGameRules
#endif

	DECLARE_NETWORKCLASS();

	// Stuff shared between client and server.

	virtual	bool	Init();

	virtual void SafeRemoveIfDesired();

	virtual	bool	PostConstructor( const char *classname ) { return true; }
	bool IsNetworked() const { return true; }

	// Damage Queries - these need to be implemented by the various subclasses (single-player, multi-player, etc).
	// The queries represent queries against damage types and properties.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShouldGibCorpse( int iDmgType );		// Damage types that gib the corpse.
	virtual bool	Damage_ShowOnHUD( int iDmgType );			// Damage types that have client HUD art.
	virtual bool	Damage_NoPhysicsForce( int iDmgType );		// Damage types that don't have to supply a physics force & position.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );		// Damage types that don't make the player bleed.
	//Temp: These will go away once DamageTypes become enums.
	virtual int		Damage_GetTimeBased( void );				// Actual bit-fields.
	virtual int		Damage_GetShouldGibCorpse( void );
	virtual int		Damage_GetShowOnHud( void );					
	virtual int		Damage_GetNoPhysicsForce( void );
	virtual int		Damage_GetShouldNotBleed( void );

	void LoadVoiceCommandScript( void );

// Ammo Definitions
	//CAmmoDef* GetAmmoDef();

	virtual bool SwitchToNextBestWeapon( CSharedBaseCombatCharacter *pPlayer, CSharedBaseCombatWeapon *pCurrentWeapon ); // Switch to the next best weapon
	virtual CSharedBaseCombatWeapon *GetNextBestWeapon( CSharedBaseCombatCharacter *pPlayer, CSharedBaseCombatWeapon *pCurrentWeapon ); // I can't use this weapon anymore, get me the next best one.
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

	virtual int DefaultFOV( void ) { return 90; }

	// This function is here for our CNetworkVars.
	inline void NetworkStateChanged()
	{
		// Forward the call to the entity that will send the data.
		CSharedGameRulesProxy::NotifyNetworkStateChanged();
	}

	inline void NetworkStateChanged( void *pVar )
	{
		// Forward the call to the entity that will send the data.
		CSharedGameRulesProxy::NotifyNetworkStateChanged();
	}

	// Get the view vectors for this mod.
	virtual const CViewVectors* GetViewVectors() const;

// Damage rules for ammo types
	virtual float GetAmmoDamage( CSharedBaseEntity *pAttacker, CSharedBaseEntity *pVictim, int nAmmoType );
    virtual float GetDamageMultiplier( void ) { return 1.0f; }    

	virtual const unsigned char *GetEncryptionKey() { return NULL; }

	//Allow thirdperson camera.
	virtual bool AllowThirdPersonCamera( void ) { return false; }

	// IsConnectedUserInfoChangeAllowed allows the clients to change
	// cvars with the FCVAR_NOT_CONNECTED rule if it returns true
	virtual bool IsConnectedUserInfoChangeAllowed( CSharedBasePlayer *pPlayer )
	{ 
		return true; 
	}

	virtual void FireGameEvent( IGameEvent *event );

	// Called when game rules are destroyed by CWorld
	virtual void LevelShutdown( void );

#ifdef CLIENT_DLL

	virtual bool IsBonusChallengeTimeBased( void );

	virtual bool AllowMapParticleEffect( const char *pszParticleEffect ) { return true; }

	virtual bool AllowWeatherParticles( void ) { return true; }

	virtual bool AllowMapVisionFilterShaders( void ) { return true; }
	virtual const char* TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName ) { return pchEffectName; }

	virtual void ClientAdjustStartSoundParams( StartSoundParams_t& params ) {}

	virtual bool IsLocalPlayer( int nEntIndex );

	virtual void ModifySentChat( char *pBuf, int iBufSize ) { return; }

	virtual bool ShouldWarnOfAbandonOnQuit() { return false; }

	virtual bool ShouldDrawHeadLabels()
	{
		if ( mp_show_voice_icons.GetBool() == false )
			return false;

		return true;
	}

	virtual void OnFileReceived( const char * fileName, unsigned int transferID ) { return; }

	virtual bool HandleDisconnectAttempt() { return false; }

	const char *GetVoiceCommandSubtitle( int iMenu, int iItem );
	bool GetVoiceMenuLabels( int iMenu, KeyValues *pKV );
	
#else

	virtual void Status( void (*print) (const char *fmt, ...) ) {}

	virtual void GetTaggedConVarList( KeyValues *pCvarTagList );

// CBaseEntity overrides.
public:

// Setup
	virtual void OnBeginChangeLevel( const char *nextMapName, KeyValues *saveData ) {}	///< called just before a trigger_changelevel starts a changelevel

	virtual void Precache( void );

	virtual void RefreshSkillData( bool forceUpdate );// fill skill data struct with proper values
	
	// Called each frame. This just forwards the call to Think().
	virtual void FrameUpdatePostEntityThink();

	virtual void OnServerHibernating() {}

	virtual void Think( void );// GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity );  // Can this item spawn (eg NPCs don't spawn in deathmatch).

	// Called at the end of GameFrame (i.e. after all game logic has run this frame)
	virtual void EndGameFrame( void );

	virtual bool IsSkillLevel( int iLevel ) { return GetSkillLevel() == iLevel; }
	virtual int	GetSkillLevel() { return g_iSkillLevel; }
	virtual void OnSkillLevelChanged( int iNewLevel );
	virtual void SetSkillLevel( int iLevel );

	virtual bool FAllowFlashlight( void );// Are players allowed to switch on their flashlight?
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );// should the player switch to this weapon?

// Functions to verify the single/multiplayer status of a game
	virtual const char *GetGameDescription( void ) { return "Half-Life 2"; }  // this is the game name that gets seen in the server browser
	
// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );// a client just connected to the server (player hasn't spawned yet)
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );// a client just disconnected from the server
	virtual bool ShouldTimeoutClient( int nUserID, float flTimeSinceLastReceived ) { return false; } // return true to disconnect client due to timeout (used to do stricter timeouts when the game is sure the client isn't loading a map)
	
// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );// this client just hit the ground after a fall. How much damage?
	virtual bool  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );// can this player take damage from this attacker?
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }
	virtual float GetAutoAimScale( CBasePlayer *pPlayer ) { return 1.0f; }
	virtual int	GetAutoAimMode()	{ return AUTOAIM_ON; }

	virtual bool ShouldUseRobustRadiusDamage(CBaseEntity *pEntity) { return false; }
	virtual void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
	// Let the game rules specify if fall death should fade screen to black
	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return TRUE; }

	virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );


// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );// called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink( CBasePlayer *pPlayer ); // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer );// is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );// When in the future will this player be able to spawn?
	virtual CBaseEntity *GetPlayerSpawnSpot( CBaseCombatCharacter *pPlayer );
	virtual bool IsSpawnPointValid( CBaseEntity *pSpot, CBaseCombatCharacter *pPlayer );
	virtual void ClientSpawned( edict_t * pPlayer ) { return; }

	virtual bool AllowAutoTargetCrosshair( void );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );  // handles the user commands;  returns TRUE if command handled properly
	virtual VoiceCommandMenuItem_t *VoiceCommand( CBaseExpresserPlayer *pPlayer, int iMenu, int iItem );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );		 // the player has changed cvars
	virtual void ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );// how many points do I award whoever kills this player?
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );// Called each time a player dies
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );// Call this from within a GameRules class to report an obituary.
	virtual const char *GetDamageCustomString( const CTakeDamageInfo &info ) { return NULL; }
	CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor );									// old version of method - kept for backward compat
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );		// new version of method
	virtual bool UseSuicidePenalty() { return true; }		// apply point penalty for suicide?

// Weapon Damage
	// Determines how much damage Player's attacks inflict, based on skill level.
	virtual float AdjustPlayerDamageInflicted( float damage ) { return damage; }
	virtual void  AdjustPlayerDamageTaken( CTakeDamageInfo *pInfo ) {}; // Base class does nothing.

// Weapon retrieval
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );// The player is touching an CBaseCombatWeapon, do I give it to him?

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );// should this weapon respawn?
	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );// when may this weapon respawn?
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon ); // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon );// where in the world should this weapon respawn?

// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );// is this player allowed to take this item?
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );// call each time a player picks up an item (battery, healthkit)

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );// Should this item respawn?
	virtual float FlItemRespawnTime( CItem *pItem );// when may this item respawn?
	virtual Vector VecItemRespawnSpot( CItem *pItem );// where in the world should this item respawn?
	virtual QAngle VecItemRespawnAngles( CItem *pItem );// what angles should this item use when respawing?

// Ammo retrieval
	virtual bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex ); // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, const char *szName );
	virtual void PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount );// called each time a player picks up some ammo in the world
	virtual float GetAmmoQuantityScale( int iAmmoIndex ) { return 1.0f; }

// AI Definitions
	virtual void			InitDefaultAIRelationships( void );
	virtual const char*		AIClassText(Class_T classType);
	virtual int				NumEntityClasses() const	{ return NUM_SHARED_ENTITY_CLASSES; }
	virtual const char*		AIFactionText(Faction_T classType);
	virtual int				NumFactions() const	{ return NUM_SHARED_FACTIONS; }
	virtual int				NumTeams() const	{ return NUM_SHARED_TEAMS; }

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );// how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime( void );// how long until a depleted HealthCharger recharges itself?

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );// what do I do with a player's weapons when he's killed?

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );// Do I drop ammo when the player dies? How much?

// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) { return GetIndexedTeamName(pEntity->GetTeamNumber()); } // what team is this entity on?
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );// What is the player's relationship with this entity?
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );
	virtual void CheckChatText( CBasePlayer *pPlayer, char *pText ) { return; }

	virtual Team_t GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( Team_t teamIndex );
	virtual bool IsValidTeam( const char *pTeamName ) { return GetTeamIndex(pTeamName) != TEAM_INVALID; }
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib );
	virtual void UpdateClientData( CBasePlayer *pPlayer ) { };

// Sounds
	virtual bool PlayTextureSounds( void ) { return TRUE; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl );
	virtual bool AllowSoundscapes( void ) { return TRUE; }

// NPCs
	virtual bool FAllowNPCs( void );//are NPCs allowed

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }
				    
	// trace line rules
	virtual float WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd, unsigned int mask, trace_t *ptr );

	// Setup g_pPlayerResource (some mods use a different entity type here).
	virtual void CreateStandardEntities();
	virtual CGameRulesProxy *AllocateProxy();

	// Team name, etc shown in chat and dedicated server console
	virtual const char *GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );

	// Location name shown in chat
	virtual const char *GetChatLocation( bool bTeamOnly, CBasePlayer *pPlayer ) { return NULL; }

	// VGUI format string for chat, if desired
	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

	// Whether props that are on fire should get a DLIGHT.
	virtual bool ShouldBurningPropsEmitLight() { return false; }

	virtual bool CanEntityBeUsePushed( CBaseEntity *pEnt ) { return true; }

	virtual void CreateCustomNetworkStringTables( void ) { }

	// Game Achievements (server version)
	virtual void MarkAchievement ( IRecipientFilter& filter, char const *pchAchievementName );

	virtual void ResetMapCycleTimeStamp( void ){ m_nMapCycleTimeStamp = 0; }

	virtual void OnNavMeshLoad( void ) { return; }

	virtual void UpdateGameplayStatsFromSteam( void ) { return; }

	// Assume the game doesn't care
	virtual int GetMaxHumanPlayers() const { return -1; }

	// game-specific factories
	virtual CTacticalMissionManager *TacticalMissionManagerFactory( void );

	virtual void ProcessVerboseLogOutput( void ){}

	bool IsLoadingBugBaitReport( void );

	virtual void HandleTimeLimitChange( void ){ return; }

	void IncrementMapCycleIndex();

	void HaveAllPlayersSpeakConceptIfAllowed( int iConcept, int iTeam = TEAM_UNASSIGNED, const char *modifiers = NULL );
	void RandomPlayersSpeakConceptIfAllowed( int iConcept, int iNumRandomPlayer = 1, int iTeam = TEAM_UNASSIGNED, const char *modifiers = NULL );

	void SkipNextMapInCycle();

	virtual float GetLastMajorEventTime( void ){ return -1.0f; }

	virtual bool IsManualMapChangeOkay( const char **pszReason );

	virtual void InitCustomResponseRulesDicts()	{}
	virtual void ShutdownCustomResponseRulesDicts() {}

	virtual void GetNextLevelName( char *szNextMap, int bufsize, bool bRandom = false );

	static void DetermineMapCycleFilename( char *pszResult, int nSizeResult, bool bForceSpew );
	virtual void LoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList );
	static void FreeMapCycleFileVector ( CUtlVector<char *> &mapList );

	// LoadMapCycleFileIntoVector without the fixups inherited versions of gamerules may provide
	static void RawLoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList );

	bool IsMapInMapCycle( const char *pszName );

	virtual void ChangeLevel( void );

	virtual void GoToIntermission( void );
	virtual void LoadMapCycleFile( void );
	void ChangeLevelToMap( const char *pszMap );

	bool IsIntermission() { return m_flIntermissionEndTime > gpGlobals->curtime; }

	virtual bool InRoundRestart( void ) { return false; }

	virtual edict_t *DoFindClientInPVS( edict_t *pEdict, unsigned char *pvs, unsigned pvssize );

	float GetMapRemainingTime() const;

	void	AddLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity );

	virtual bool IsOfficialMap() const;

	ThreeState_t	GlobalFriendlyFire();
	void			SetGlobalFriendlyFire(ThreeState_t val);

#endif

	virtual bool IsTeamplay( void ); // is this deathmatch game being played with team rules?
	virtual bool IsDeathmatch( void );//is this a deathmatch game?
	virtual bool IsCoOp( void );// is this a coop game?

	virtual const char *GetGameTypeName( void ){ return NULL; }
	virtual int GetGameType( void ){ return 0; }

	virtual int GetNumHolidays() const { return NUM_SHARED_HOLIDAYS; }
	virtual bool IsHolidayActive( EHolidayFlag eHoliday ) const { return false; }
	virtual bool GetHolidayString( EHolidayFlag eHoliday, char *str, int maxlen ) const { return false; }

	float GetGravityMultiplier(  void ){ return m_flGravityMultiplier; }
	void			SetGravityMultiplier( float flValue ){ m_flGravityMultiplier.Set( flValue ); }

private:
	CUtlVector< CUtlVector< VoiceCommandMenuItem_t > > m_VoiceCommandMenus;

#ifdef GAME_DLL
	void CheckRestartGame();
	void CleanUpMap();
	void ManageObjectRelocation( void );
	bool CheckGameOver();
	void RestartGame();

	float m_flIntermissionEndTime;
	static int m_nMapCycleTimeStamp;
	static int m_nMapCycleindex;
	static CUtlStringList m_MapList;

	float m_flTimeLastMapChangeOrPlayerWasConnected;

	CNetworkVar(float, m_flGameStartTime);
	CUtlVector<EHANDLE> m_hRespawnableItemsAndWeapons;
	float m_tmNextPeriodicThink;
	float m_flRestartGameTime;
	bool m_bCompleteReset;
	bool m_bChangelevelDone;
#endif

	CNetworkVar( float, m_flGravityMultiplier );

#ifndef CLIENT_DLL
private:
	float m_flNextVerboseLogOutput;
#endif // CLIENT_DLL
};


#ifndef CLIENT_DLL
	void InstallGameRules();
	
	// Create user messages for game here, calls into static player class creation functions
	void RegisterUserMessages( void );
#endif


extern ConVar g_Language;


//-----------------------------------------------------------------------------
// Gets us at the game rules
//-----------------------------------------------------------------------------

CSharedGameRules* GameRules();

#endif // GAMERULES_H
