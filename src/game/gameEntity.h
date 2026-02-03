#pragma once

#include <vector>
#include <cstdint>  // for uint32_t, etc.
#include "g_public.h"
#include "g_script.h"
#include "q_shared.h"

#define G_MAX_SCRIPT_ACCUM_BUFFERS  8

class GameClient;
struct gitem_t;

// movers are things like doors, plats, buttons, etc
enum moverState_t
{
	MOVER_POS1,
	MOVER_POS2,
	MOVER_POS3,
	MOVER_1TO2,
	MOVER_2TO1,
	MOVER_2TO3,
	MOVER_3TO2,

	MOVER_POS1ROTATE,
	MOVER_POS2ROTATE,
	MOVER_1TO2ROTATE,
	MOVER_2TO1ROTATE
};

class GameEntity
{
public:
	SharedEntity shared;

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	GameClient    *client;            // nullptr if not a client

	bool inuse;

	const char        *classname;         // set in QuakeEd
	int spawnflags;                 // set in QuakeEd

	bool neverFree;             // if true, FreeEntity will only unlink
									// bodyque uses this

	int flags;                      // FL_* variables

	const char        *model;
	char        *model2;
	int freetime;                   // level.time when the object was freed

	int eventTime;                  // events will be cleared EVENT_VALID_MSEC after set
	bool freeAfterEvent;
	bool unlinkAfterEvent;

	bool physicsObject;         // if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects,
	float physicsBounce;            // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;                   // brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;

	int sound2to3;
	int sound3to2;
	int soundPos3;

	int soundKicked;
	int soundKickedEnd;

	int soundSoftopen;
	int soundSoftendo;
	int soundSoftclose;
	int soundSoftendc;

	GameEntity   *parent;
	GameEntity   *nextTrain;
	GameEntity   *prevTrain;
	vec3_t pos1, pos2, pos3;

	char        *message;

	int timestamp;              // body queue sinking, etc

	float angle;                // set in editor, -1 = up, -2 = down
	char        *target;
	char        *targetdeath;   // fire this on death exclusively //----(SA)	added
	char        *targetname;
	char        *team;
	char        *targetShaderName;
	char        *targetShaderNewName;
	GameEntity   *target_ent;

	float speed;
	float closespeed;           // for movers that close at a different speed than they open
	vec3_t movedir;

	int gDuration;
	int gDurationBack;
	vec3_t gDelta;
	vec3_t gDeltaBack;

	int nextthink;
	void ( *think )( GameEntity *self );
	void ( *reached )( GameEntity *self );       // movers call this when hitting endpoint
	void ( *blocked )( GameEntity *self, GameEntity *other );
	void ( *touch )( GameEntity *self, GameEntity *other, trace_t *trace );
	void ( *use )( GameEntity *self, GameEntity *other, GameEntity *activator );
	void ( *pain )( GameEntity *self, GameEntity *attacker, int damage, vec3_t point );
	void ( *die )( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int mod );

	int pain_debounce_time;
	int fly_sound_debounce_time;            // wind tunnel
	int last_move_time;

	int health;

	bool takedamage;

	int damage;
	int splashDamage;           // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;

	GameEntity   *chain;
	GameEntity   *enemy;
	GameEntity   *activator;
	GameEntity   *teamchain;     // next entity in team
	GameEntity   *teammaster;    // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	// Rafael - sniper variable
	// sniper uses delay, random, radius
	int radius;
	float delay;

	// JOSEPH 10-11-99
	int TargetFlag;
	float duration;
	vec3_t rotate;
	vec3_t TargetAngles;
	// END JOSEPH

	gitem_t     *item;          // for bonus items

	// Ridah, AI fields
	char        *aiAttributes;
	const char        *aiName;
	int aiTeam;
	void ( *AIScript_AlertEntity )( GameEntity *ent );
	bool aiInactive;
	int aiCharacter;            // the index of the type of character we are (from aicast_soldier.c)
	// done.

	const char        *aiSkin;
	const char        *aihSkin;

	vec3_t dl_color;
	const char        *dl_stylestring;
	char        *dl_shader;
	int dl_atten;


	int key;                    // used by:  target_speaker->nopvs,

	int active;
	bool botDelayBegin;

	// Rafael - mg42
	float harc;
	float varc;

	//----(SA)	added
	float activateArc;              // right now just for mg42, but available for setting what angle this ent can be touched/killed from

	int props_frame_state;

	// Ridah
	int missionLevel;                   // highest mission level completed (for previous level de-briefings)
	int missionObjectives;              // which objectives for the current level have been met
										// gets reset each new level

	int numSecretsFound;                //----(SA)	added to get into savegame
	int numTreasureFound;               //----(SA)	added to get into savegame

	// done.

	// Rafael
	bool is_dead;
	// done

	int start_size;
	int end_size;

	// Rafael props

	bool isProp;

	int mg42BaseEnt;

	GameEntity   *melee;

	char        *spawnitem;

	bool nopickup;

	int flameQuota, flameQuotaTime, flameBurnEnt;

	int count2;

	int grenadeExplodeTime;         // we've caught a grenade, which was due to explode at this time
	int grenadeFired;               // the grenade entity we last fired

	int mg42ClampTime;              // time to wait before an AI decides to ditch the mg42

	char        *track;

	// entity scripting system
	const char                *scriptName;

	int numScriptEvents;
	std::vector<g_script_event_t> scriptEvents;  // contains a list of actions to perform for each event type
	g_script_status_t scriptStatus;     // current status of scripting
	g_script_status_t scriptStatusBackup;
	// the accumulation buffer
	int scriptAccumBuffer[G_MAX_SCRIPT_ACCUM_BUFFERS];

	bool AASblocking;
	float accuracy;

	const char        *tagName;       // name of the tag we are attached to
	GameEntity   *tagParent;

	float headshotDamageScale;

	g_script_status_t scriptStatusCurrent;      // had to go down here to keep savegames compatible

	int emitID;
	int emitNum;
	int emitPressure;  
	int emitTime;     
};
