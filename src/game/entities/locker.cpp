#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

/*QUAKED props_footlocker (.6 .7 .3) (-12 -21 -12) (12 21 12) ? NO_JUNK
"noise"  the sound entity is to make upon death
the default sounds are:
  "wood"	- "sound/world/boardbreak.wav"
  "glass"	- "sound/world/glassbreak.wav"
  "metal"	- "sound/world/metalbreak.wav"
  "gibs"	- "sound/player/gibsplit1.wav"
  "brick"	- "sound/world/brickfall.wav"
  "stone"	- "sound/world/stonefall.wav"
  "fabric"	- "sound/world/metalbreak.wav"	// (SA) temp

"locknoise" the locked sound to play
"wait"	 denotes how long the wait is going to be before the locked sound is played again default is 1 sec
"health" default is 1

"spawnitem" - will spawn this item upon death use the pickup_name ie. '9mm'

"type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
"mass" - defaults to 75.  This determines how much debris is emitted.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.

"dl_shader" needs to be set the same way as a target_effect

TBD: the spawning of junk still pending and animation when used

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/mapobjects/furniture/footlocker.md3"
*/

#define LOCKER_ANIM_USEEND      5
#define LOCKER_ANIM_DEATHSTART  6
#define LOCKER_ANIM_DEATHEND    11

//////////////////////////////////////////////////
void init_locker( GameEntity *ent );
void props_locker_death( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod );
//////////////////////////////////////////////////
#define MAX_LOCKER_DEBRIS       5

int locker_debris_model[MAX_LOCKER_DEBRIS];
//////////////////////////////////////////////////

void Spawn_Junk( GameEntity *ent ) {
	GameEntity *sfx;
	vec3_t dir, start;

	VectorCopy( ent->shared.r.currentOrigin, start );

	start[0] += crandom() * 32;
	start[1] += crandom() * 32;
	start[2] += 16;

	VectorSubtract( start, ent->shared.r.currentOrigin, dir );
	VectorNormalize( dir );

	sfx = G_Spawn();

	G_SetOrigin( sfx, start );
	G_SetAngle( sfx, ent->shared.r.currentAngles );

	G_AddEvent( sfx, EV_JUNK, DirToByte( dir ) );

	sfx->think = G_FreeEntity;

	sfx->nextthink = level.time + 1000;

	SV_LinkEntity( &sfx->shared );
}

void props_locker_endrattle( GameEntity *ent ) {
	ent->shared.s.frame = 0;   // idle
	ent->think = 0;
	ent->nextthink = 0;
	ent->delay = 0;
}


void props_locker_use( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	if ( !ent->delay ) {
		ent->shared.s.frame = 1;   // rattle when pain starts
	}
	ent->delay = 1;
	ent->think = props_locker_endrattle;
	ent->nextthink = level.time + 1000; // rattle a sec
}

void props_locker_pain( GameEntity *ent, GameEntity *attacker, int damage, vec3_t point ) {
	props_locker_use( ent, attacker, attacker );
}


void init_locker( GameEntity *ent ) {
	ent->isProp = true;
	ent->takedamage = true;
	ent->delay = 0;

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	// TODO: change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
	ent->shared.s.eType = ET_TRAP;

	ent->shared.s.frame = 0;   // closed animation

	ent->count2 = LOCKER_ANIM_DEATHEND;

	ent->die = props_locker_death;
	ent->use = props_locker_use;    // trying it rattles the lock (could also allow 'waking' from trigger)
	ent->pain = props_locker_pain;

	// drop origin down 8 so the designer can put the box entity on the floor rather than /in/ the floor
	// remove if you get a new model from jason w/ the origin moved up 8
	ent->shared.s.origin[2] -= 8;

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !( ent->health ) ) {
		ent->health = 1;
	}

	SV_LinkEntity( &ent->shared );
}

void props_locker_spawn_item( GameEntity *ent )
{
	gitem_t     *item = BG_FindItem( ent->spawnitem );

	if ( !item ) {
		// empty
		return;
	}

	GameEntity* drop = LaunchItem( item, ent->shared.r.currentOrigin, tv( 0, 0, 20 ) );

	if ( !drop ) {
		Com_Printf( "-----> WARNING <-------\n" );
		Com_Printf( "props_locker_spawn_item at %s failed!\n", vtos( ent->shared.r.currentOrigin ) );
	}
}

void props_locker_mass( GameEntity *ent ) {
	GameEntity   *tent;
	vec3_t start;
	vec3_t dir;

	VectorCopy( ent->shared.r.currentOrigin, start );

	start[0] += crandom() * 32;
	start[1] += crandom() * 32;
	start[2] += 16;

	VectorSubtract( start, ent->shared.r.currentOrigin, dir );
	VectorNormalize( dir );

	tent = G_TempEntity( ent->shared.r.currentOrigin, EV_EFFECT );
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	VectorCopy( dir, tent->shared.s.angles2 );

	tent->shared.s.dl_intensity = 0;

	SV_SetConfigstring( CS_TARGETEFFECT, ent->dl_shader );    //----(SA)	allow shader to be set from entity

	tent->shared.s.frame = ent->key;

	tent->shared.s.eventParm = 8;
	tent->shared.s.density = 100;
}




void props_locker_death( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	ent->takedamage = false;
	ent->shared.s.frame = 2;   // opening animation
	ent->think = 0;
	ent->nextthink = 0;

	SV_UnlinkEntity( &ent->shared );
	ent->shared.r.maxs[2] = 11;    // (SA) make the dead bb half height so the item can look like it's sitting inside
	props_locker_spawn_item( ent );
	SV_LinkEntity( &ent->shared );
}


void SP_props_footlocker( GameEntity *self )
{
	const char    *type;
	const char    *sound;
	const char    *locked;
	int mass;

	// (SA) if angle is xx or yy, rotate the bounding box 90 deg to match
	// NOTE:	Non axis-aligned orientation not allowed.  It will work, but
	//			the bounding box will not exactly match the model.
	if ( self->shared.s.angles[1] == 90 || self->shared.s.angles[1] == 270 ) {
		VectorSet( self->shared.r.mins, -21, -12, 0 );
		VectorSet( self->shared.r.maxs, 21, 12, 24 );
	} else {
		VectorSet( self->shared.r.mins, -12, -21, 0 );
		VectorSet( self->shared.r.maxs, 12, 21, 24 );
	}

	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/furniture/footlocker.md3" );

	if ( G_SpawnString( "noise", "NOSOUND", &sound ) ) {
		self->noise_index = G_SoundIndex( sound );
	}

	if ( G_SpawnString( "locknoise", "NOSOUND", &locked ) ) {
		self->soundPos1 = G_SoundIndex( locked );
	}

	if ( !( self->wait ) ) {
		self->wait = 1000;
	} else {
		self->wait *= 1000;
	}

	if ( G_SpawnInt( "mass", "75", &mass ) ) {
		self->count = mass;
	} else {
		self->count = 75;
	}

	if ( G_SpawnString( "type", "wood", &type ) ) {
		if ( !Q_stricmp( type,"wood" ) ) {
			self->key = 0;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			self->key = 1;
		} else if ( !Q_stricmp( type,"metal" ) )  {
			self->key = 2;
		} else if ( !Q_stricmp( type,"gibs" ) )   {
			self->key = 3;
		} else if ( !Q_stricmp( type,"brick" ) ) {
			self->key = 4;
		} else if ( !Q_stricmp( type,"rock" ) )  {
			self->key = 5;
		} else if ( !Q_stricmp( type,"fabric" ) )                                                                                                                                                                                                                                                                                         {
			self->key = 6;
		}
	} else {
		self->key = 0;
	}

	self->delay = level.time + self->wait;

	init_locker( self );
}
