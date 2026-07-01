/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma once

#include <vector>
#include "../scripting/ScriptParser.h"

// g_local.h -- local definitions for game module
#include <cstdint>  // for uint32_t, etc.
#include "q_shared.h"
#include "bg_public.h"
#include "g_public.h"
#include "g_script.h"
#include "gameEntity.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
//----(SA) Wolfenstein
#define GAMEVERSION "main"
// done.

#define BODY_QUEUE_SIZE     8

#define INFINITE            1000000

#define FRAMETIME           100                 // msec
#define EVENT_VALID_MSEC    1000
#define CARNAGE_REWARD_TIME 3000
#define REWARD_SPRITE_TIME  2000

#define INTERMISSION_DELAY_TIME 1000


// gentity->flags
#define FL_GODMODE              0x00000010
#define FL_NOTARGET             0x00000020
#define FL_DEFENSE_CROUCH       0x00000100  // warzombie defense pose
#define FL_TEAMSLAVE            0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_DROPPED_ITEM         0x00001000
#define FL_NO_BOTS              0x00002000  // spawn point not for bot use
#define FL_NO_HUMANS            0x00004000  // spawn point just for bots
#define FL_AI_GRENADE_KICK      0x00008000  // an AI has already decided to kick this grenade
// Rafael
#define FL_NOFATIGUE            0x00010000  // cheat flag no fatigue

#define FL_TOGGLE               0x00020000  //----(SA)	ent is toggling (doors use this for ex.)
#define FL_KICKACTIVATE         0x00040000  //----(SA)	ent has been activated by a kick (doors use this too for ex.)
#define FL_SOFTACTIVATE         0x00000040  //----(SA)	ent has been activated while 'walking' (doors use this too for ex.)
#define FL_DEFENSE_GUARD        0x00080000  // warzombie defense pose

#define FL_PARACHUTE            0x00100000
#define FL_WARZOMBIECHARGE      0x00200000
#define FL_NO_MONSTERSLICK      0x00400000
#define FL_NO_HEADCHECK         0x00800000

#define FL_NODRAW               0x01000000
#define FL_DOORNOISE            0x02000000  //----(SA)	added




// door AI sound ranges
#define HEAR_RANGE_DOOR_LOCKED      128 // really close since this is a cruel check
#define HEAR_RANGE_DOOR_KICKLOCKED  512
#define HEAR_RANGE_DOOR_OPEN        256
#define HEAR_RANGE_DOOR_KICKOPEN    768


#define SP_PODIUM_MODEL     "models/mapobjects/podium/podium4.md3"

class GameEntity;
class GameClient;


#define CFOFS( x ) ( (intptr_t)&( ( (GameClient *)0 )->x ) )


// Ridah
#include "ai_cast_global.h"
// done.

typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,     // Beginning a team game, spawn at base
	TEAM_ACTIVE     // Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	int location;

	int captures;
	int basedefense;
	int carrierdefense;
	int flagrecovery;
	int fragcarrier;
	int assists;

	float lasthurtcarrier;
	float lastreturnedflag;
	float flagsince;
	float lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define FOLLOW_ACTIVE1  -1
#define FOLLOW_ACTIVE2  -2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {

	int spectatorTime;              // for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient;            // for chasecam and follow mode
	int wins, losses;               // tournament stats
	int playerType;                 // DHM - Nerve :: for GT_WOLF
	int playerWeapon;               // DHM - Nerve :: for GT_WOLF
	int playerPistol;               // DHM - Nerve :: for GT_WOLF
	int playerItem;                 // DHM - Nerve :: for GT_WOLF
	int playerSkin;                 // DHM - Nerve :: for GT_WOLF
} clientSession_t;

//
#define MAX_NETNAME         36
#define MAX_VOTE_COUNT      3

#define PICKUP_ACTIVATE 0   // pickup items only when using "+activate"
#define PICKUP_TOUCH    1   // pickup items when touched
#define PICKUP_FORCE    2   // pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	UserCmd cmd;                  // we would lose angles if not persistant
	UserCmd oldcmd;               // previous command processed by pmove()

	bool initialSpawn;          // the first spawn should be at a cool location
	bool predictItemPickup;     // based on cg_predictItems userinfo
	bool pmoveFixed;            //
	char netname[MAX_NETNAME];

	int autoActivate;               // based on cg_autoactivate userinfo		(uses the PICKUP_ values above)
	int emptySwitch;                // based on cg_emptyswitch userinfo (means "switch my weapon for me when ammo reaches '0' rather than -1)

	int maxHealth;                  // for handicapping
	int enterTime;                  // level.time the client entered the game
	playerTeamState_t teamState;    // status in teamplay games
	int voteCount;                  // to prevent people from constantly calling votes
	int teamVoteCount;              // to prevent people from constantly calling votes
} clientPersistant_t;


// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
class GameClient
{
public:
	// ps MUST be the first element, because the server expects it
	PlayerState ps;               // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	bool readyToExit;           // wishes to leave the intermission

	bool noclip;

	int lastCmdTime;                // level.time of last UserCmd, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	int wbuttons;
	int oldwbuttons;
	int latched_wbuttons;
	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor;               // damage absorbed by armor
	int damage_blood;               // damage taken out of health
	int damage_knockback;           // impact damage
	vec3_t damage_from;             // origin for vector calculation
	bool damage_fromWorld;      // if true, don't use the damage_from vector

	int accurateCount;              // for "impressive" reward sound

	int accuracy_shots;             // total number of shots
	int accuracy_hits;              // total number of hits

	//
	int lastkilled_client;          // last client that this client killed
	int lasthurt_client;            // last client that damaged this client
	int lasthurt_mod;               // type of damage the client did

	// timers
	int respawnTime;                // can respawn when time > this, force after g_forcerespwan
	int inactivityTime;             // kick players when time > this
	bool inactivityWarning;     // true if the five seoond warning has been given
	int rewardTime;                 // clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int airOutTime;

	int lastKillTime;               // for multiple kill rewards

	bool fireHeld;              // used for hook
	GameEntity   *hook;              // grapple hook if out

	int switchTeamTime;             // time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	float currentAimSpreadScale;

	int medicHealAmt;

	// RF, may be shared by multiple clients/characters
	animModelInfo_t *modelInfo;

	// -------------------------------------------------------------------------------------------
	// if working on a post release patch, new variables should ONLY be inserted after this point

	GameEntity   *persistantPowerup;
	int portalID;
	int ammoTimes[WP_NUM_WEAPONS];
	int invulnerabilityTime;

	GameEntity   *cameraPortal;              // grapple hook if out
	vec3_t cameraOrigin;


	int deployQueueNumber;         // JPW NERVE player order in reinforcement FIFO queue
	int sniperRifleFiredTime;         // JPW NERVE last time a sniper rifle was fired (for muzzle flip effects)
	float sniperRifleMuzzleYaw;       // JPW NERVE for time-dependent muzzle flip in multiplayer
	float sniperRifleMuzzlePitch;       // (SA) added

	int saved_persistant[MAX_PERSISTANT];           // DHM - Nerve :: Save ps->persistant here during Limbo
};



//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048

typedef struct {
	GameClient    *clients;       // [maxclients]

	GameEntity    *gentities;
	int gentitySize;
	int num_entities;               // current number, <= MAX_GENTITIES

	int warmupTime;                 // restart match at this time

	fileHandle_t logFile;

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;                           // in msec
	int previousTime;                   // so movers can back up when blocked

	int startTime;                      // level.time the map was started

	bool restarted;                 // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;         // includes connecting clients
	int numPlayingClients;              // connected, non-spectators
	int sortedClients[MAX_CLIENTS];             // sorted by score
	int follow1, follow2;               // clientNums for auto-follow spectators

	int snd_fry;                        // sound index for standing in lava

	int warmupModificationCount;            // for detecting if g_warmup is changed

	// voting state
	char voteString[MAX_STRING_CHARS];
	char voteDisplayString[MAX_STRING_CHARS];
	int voteTime;                       // level.time vote was called
	int voteExecuteTime;                // time the vote is executed
	int voteYes;
	int voteNo;

	// spawn variables
	bool spawning;                  // the G_Spawn*() functions are valid
	int numSpawnVars;
	char        *spawnVars[MAX_SPAWN_VARS][2];  // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int intermissionQueued;             // intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int intermissiontime;               // time the intermission was started
	char        *changemap;
	bool readyToExit;               // at least one client wants to exit
	int exitTime;
	vec3_t intermission_origin;         // also used for spectator spawns
	vec3_t intermission_angle;

	bool locationLinked;            // target_locations get linked
	GameEntity   *locationHead;          // head of the location list
	int bodyQueIndex;                   // dead bodies
	GameEntity   *bodyQue[BODY_QUEUE_SIZE];

	int portalSequence;

	std::vector<ScriptParser::EntityScript> scriptAI;
	int reloadPauseTime;                // don't think AI/client's until this time has elapsed
	int reloadDelayTime;                // don't start loading the savegame until this has expired

	int lastGrenadeKick;

	int loperZapSound;
	int stimSoldierFlySound;
	int bulletRicochetSound;

	int snipersound;

	int numSecrets;
	int numTreasure;
	int numArtifacts;
	int numObjectives;

	int knifeSound[4];


	std::vector<ScriptParser::EntityScript> scriptEntity;

	// player/AI model scripting (server repository)
	animScriptData_t animScriptData;

	// next map to load
	char nextMap[MAX_STRING_CHARS];

	// RF, record last time we loaded, so we can hack around sighting issues on reload
	int lastLoadTime;

} level_locals_t;

//extern    bool	reloading;				// loading up a savegame

//
// g_spawn.c
//
bool    G_SpawnString( const char *key, const char *defaultString, const char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
bool    G_SpawnFloat( const char *key, const char *defaultString, float *out );
bool    G_SpawnInt( const char *key, const char *defaultString, int *out );
bool    G_SpawnVector( const char *key, const char *defaultString, float *out );
void        G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );
// Ridah
bool G_CallSpawn( GameEntity *ent );
// done.

//
// g_cmds.c
//

void StopFollowing( GameEntity *ent );

void Cmd_FollowCycle_f( GameEntity *ent, int dir );

//
// g_items.c
//
void G_RunItem( GameEntity *ent );
void RespawnItem( GameEntity *ent );

void UseHoldableItem( GameEntity *ent, int item );
void PrecacheItem( gitem_t *it );
GameEntity *Drop_Item( GameEntity *ent, gitem_t *item, float angle, bool novelocity );
GameEntity *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
void SetRespawn( GameEntity *ent, float delay );
void G_SpawnItem( GameEntity *ent, gitem_t *item );
void FinishSpawningItem( GameEntity *ent );
void Think_Weapon( GameEntity *ent );
int ArmorIndex( GameEntity *ent );
void Fill_Clip( PlayerState *ps, int weapon );
void    Add_Ammo( GameEntity *ent, int weapon, int count, bool fillClip );
void Touch_Item( GameEntity *ent, GameEntity *other, trace_t *trace );

// Touch_Item_Auto is bound by the rules of autoactivation (if cg_autoactivate is 0, only touch on "activate")
void Touch_Item_Auto( GameEntity *ent, GameEntity *other, trace_t *trace );

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );
void Prop_Break_Sound( GameEntity *ent );
void Spawn_Shard( GameEntity *ent, GameEntity *inflictor, int quantity, int type );

//
// g_utils.c
//
// Ridah
int G_FindConfigstringIndex( const char *name, int start, int max, bool create );
// done.
int G_ModelIndex( const char *name );
int     G_SoundIndex( const char *name );

void    G_KillBox( GameEntity *ent );
GameEntity *G_Find( GameEntity *from, int fieldofs, const char *match );
GameEntity *G_PickTarget( char *targetname );
void    G_UseTargets( GameEntity *ent, GameEntity *activator );
void    G_SetMovedir( vec3_t angles, vec3_t movedir );

void    G_InitGentity( GameEntity *e );
GameEntity   *G_Spawn( void );
GameEntity *G_TempEntity( vec3_t origin, int event );
void    G_Sound( GameEntity *ent, int soundIndex );
void    G_AnimScriptSound( int soundIndex, vec3_t org, int client );
void    G_FreeEntity( GameEntity *e );
//bool	G_EntitiesFree( void );

void    G_TouchTriggers( GameEntity *ent );
void    G_TouchSolids( GameEntity *ent );

float   *tv( float x, float y, float z );
char    *vtos( const vec3_t v );

void G_AddPredictableEvent( GameEntity *ent, int event, int eventParm );
void G_AddEvent( GameEntity *ent, int event, int eventParm );
void G_SetOrigin( GameEntity *ent, vec3_t origin );
void AddRemap( const char *oldShader, const char *newShader, float timeOffset );
const char *BuildShaderStateConfig();
void G_SetAngle( GameEntity *ent, vec3_t angle );

bool infront( GameEntity *self, GameEntity *other );

void G_ProcessTagConnect( GameEntity *ent, bool clearAngles );

//
// g_combat.c
//
bool CanDamage( GameEntity *targ, vec3_t origin );
void G_Damage( GameEntity *targ, GameEntity *inflictor, GameEntity *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod );
bool G_RadiusDamage( vec3_t origin, GameEntity *attacker, float damage, float radius, GameEntity *ignore, int mod );
void body_die( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int meansOfDeath );
void TossClientItems( GameEntity *self );

// damage flags
#define DAMAGE_RADIUS               0x00000001  // damage was indirect
#define DAMAGE_NO_ARMOR             0x00000002  // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK         0x00000008  // do not affect velocity, just view angles
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_PROTECTION        0x00000020  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_PASSTHRU             0x00000040  // damage came through an explosive, or other player, or has in some way already given damage to something

//
// g_missile.c
//
void G_RunMissile( GameEntity *ent );
int G_PredictMissile( GameEntity *ent, int duration, vec3_t endPos, bool allowBounce );

// Rafael zombiespit
void G_RunSpit( GameEntity *ent );
void G_RunDebris( GameEntity *ent );

void G_RunCrowbar( GameEntity *ent );

//----(SA) removed unused q3a weapon firing
GameEntity *fire_grenade( GameEntity *self, vec3_t start, vec3_t aimdir, int grenadeWPID );
GameEntity *fire_rocket( GameEntity *self, vec3_t start, vec3_t dir );


// Rafael sniper
void fire_lead( GameEntity *self,  vec3_t start, vec3_t dir, int damage );
bool visible( GameEntity *self, GameEntity *other );

GameEntity *fire_mortar( GameEntity *self, vec3_t start, vec3_t dir );

GameEntity *fire_zombiespit( GameEntity *self, vec3_t start, vec3_t dir );
GameEntity *fire_zombiespirit( GameEntity *self, GameEntity *bolt, vec3_t start, vec3_t dir );
GameEntity *fire_crowbar( GameEntity *self, vec3_t start, vec3_t dir );
GameEntity *fire_flamebarrel( GameEntity *self, vec3_t start, vec3_t dir );
// done

//
// g_mover.c
//
void G_RunMover( GameEntity *ent );
void Use_BinaryMover( GameEntity *ent, GameEntity *other, GameEntity *activator );
void G_Activate( GameEntity *ent, GameEntity *activator );

void G_TryDoor( GameEntity *ent, GameEntity *other, GameEntity *activator ); //----(SA)	added

void InitMoverRotate( GameEntity *ent );

void InitMover( GameEntity *ent );
void SetMoverState( GameEntity *ent, moverState_t moverState, int time );

//
// g_tramcar.c
//
void Reached_Tramcar( GameEntity *ent );


//
// g_misc.c
//
void TeleportPlayer( GameEntity *player, vec3_t origin, vec3_t angles );


//
// g_weapon.c
//
bool LogAccuracyHit( GameEntity *target, GameEntity *attacker );
void CalcMuzzlePoint( GameEntity *ent, int weapon, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void SnapVectorTowards( vec3_t v, vec3_t to );
trace_t *CheckMeleeAttack( GameEntity *ent, float dist, bool isTest );
GameEntity *weapon_grenadelauncher_fire( GameEntity *ent, int grenadeWPID );
// Rafael
GameEntity *weapon_crowbar_throw( GameEntity *ent );

void CalcMuzzlePoints( GameEntity *ent, int weapon );
//----(SA) commented out as we have no hook
//void Weapon_HookFree (GameEntity *ent);
//void Weapon_HookThink (GameEntity *ent);

// Rafael - for activate
void CalcMuzzlePointForActivate( GameEntity *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
// done.

//
// g_client.c
//

void SetClientViewAngle( GameEntity *ent, vec3_t angle );

void respawn( GameEntity *ent );

void InitClientPersistant( GameClient *client );
void InitClientResp( GameClient *client );
void InitBodyQue( void );
void ClientSpawn( GameEntity *ent );
void player_die( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int mod );
void AddScore( GameEntity *ent, int score );

bool G_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo );

//
// g_svcmds.c
//
bool    ConsoleCommand( void );


//
// g_weapon.c
//
void FireWeapon( GameEntity *ent );

//
// p_hud.c
//
void G_SetStats( GameEntity *ent );


//
// g_cmds.c
//

//
// g_pweapon.c
//


//
// g_main.c
//

void G_RunThink( GameEntity *ent );
void  G_LogPrintf( const char *fmt, ... );

void  Com_Printf( const char *fmt, ... );

//----(SA)	added
void G_EndGame( void );
int G_SendMissionStats( void );   // return '0' if objectives not met, '1' if met
void G_ChangeLevel( char *mapName );
//----(SA)	end

//
// g_client.c
//
const char *ClientConnect( int clientNum, bool firstTime, bool isBot );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( GameEntity *ent );
void G_RunClient( GameEntity *ent );

//
// g_mem.c
//
void *G_Alloc( size_t size );


//
// g_session.c
//
void G_ReadSessionData( GameClient *client );
void G_InitSessionData( GameClient *client, char *userinfo );

void G_WriteSessionData( void );

//
// g_bot.c
//



void G_QueueBotBegin( int clientNum );
bool G_BotConnect( int clientNum, bool restart );


// ai_main.c
#define MAX_FILEPATH            144

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupClient( int client, struct bot_settings_s *settings );
int BotAIShutdownClient( int client );
int BotAIStartFrame( int time );
void BotTestAAS( vec3_t origin );


// g_cmd.c
void Cmd_Activate_f( GameEntity *ent );
int Cmd_WolfKick_f( GameEntity *ent );
// Ridah

// g_save.c
bool G_SaveGame( const char *username );
void G_LoadGame( const char *username );
bool G_SavePersistant( char *nextmap );
void G_LoadPersistant( void );

// g_script.c
void G_Script_ScriptParse( GameEntity *ent );
bool G_Script_ScriptRun( GameEntity *ent );

void G_Script_ScriptLoad( void );

float AngleDifference( float ang1, float ang2 );

// g_props.c
void Props_Chair_Skyboxtouch( GameEntity *ent );

extern level_locals_t level;
extern GameEntity g_entities[MAX_GENTITIES];
extern GameEntity       *g_camEnt;

#define FOFS( x ) ( (intptr_t)&( ( (GameEntity *)0 )->x ) )

// Rafael gameskill
extern vmCvar_t g_gameskill;
// done

extern vmCvar_t g_reloading;        //----(SA)	added

extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;               // allow this many total, including spectators
extern vmCvar_t g_maxGameClients;           // allow this many active
extern vmCvar_t g_restarted;

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;

extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_quadfactor;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_debugBullets;     //----(SA)	added
extern vmCvar_t g_debugAudibleEvents;       //----(SA)	added
extern vmCvar_t g_headshotMaxDist;      //----(SA)	added
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_syncronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t g_blood;


extern vmCvar_t g_needpass;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_doWarmup;
extern vmCvar_t g_teamAutoJoin;
extern vmCvar_t g_teamForceBalance;

extern vmCvar_t g_filterBan;
extern vmCvar_t g_rankings;
extern vmCvar_t g_enableBreath;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

//Rafael
extern vmCvar_t g_autoactivate;

extern vmCvar_t g_testPain;

extern vmCvar_t g_missionStats;
extern vmCvar_t ai_scriptName;          // name of AI script file to run (instead of default for that map)
extern vmCvar_t g_scriptName;           // name of script file to run (instead of default for that map)

extern vmCvar_t g_userAim;

extern vmCvar_t g_forceModel;

extern vmCvar_t g_mg42arc;

extern vmCvar_t g_totalPlayTime;
extern vmCvar_t g_attempts;

extern vmCvar_t g_footstepAudibleRange;

extern vmCvar_t g_playerStart;      //----(SA)	added

int     Sys_Milliseconds( void );

void    Cmd_ArgsBuffer( char *buffer, int bufferLength );

void    Cvar_Set( const char *var_name, const char *value );

bool SV_inPVS( const vec3_t p1, const vec3_t p2 );


int     SV_BotLibShutdown( void );
int     Export_BotLibVarSet( const char *var_name, const char *value );


void    BotUserCommand( int client, UserCmd *ucmd );


typedef enum
{
	shard_glass = 0,
	shard_wood,
	shard_metal,
	shard_ceramic,
	shard_rubble
} shards_t;

// sv_game.c
void SV_LocateGameData( SharedEntity *gEnts, int numGEntities, int sizeofGEntity_t, PlayerState *clients, int sizeofGameClient );
void SV_GameDropClient( int clientNum, const char *reason );

