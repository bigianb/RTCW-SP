#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

int snd_chaircreak;
int snd_chairthrow;
int snd_chairhitground;
int chair_metalbreak;

// in g_props.cpp
extern int snd_boardbreak;
extern int snd_glassbreak;
extern int snd_metalbreak;
extern int snd_ceramicbreak;

void moveit( GameEntity *ent, float yaw, float dist ); 

/*QUAKED props_chair_chat(.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
model - specify a different model for the chair.  must have same frames as default for this ent

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

/*QUAKED props_chair_chatarm(.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
model - specify a different model for the chair.  must have same frames as default for this ent

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

/*QUAKED props_chair_side (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
model - specify a different model for the chair.  must have same frames as default for this ent

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/


/*QUAKED props_chair_hiback (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
model - specify a different model for the chair.  must have same frames as default for this ent

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

/*QUAKED props_chair (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )
model - specify a different model for the chair.  must have same frames as default for this ent

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/
void Props_Chair_Think( GameEntity *self );
void Props_Chair_Touch( GameEntity *self, GameEntity *other, trace_t *trace );
void Props_Chair_Die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod );

void Just_Got_Thrown( GameEntity *self ) {
	float len;
	vec3_t vec;
	bool prop_hits = false;

	len = 0;

	if ( self->shared.s.groundEntityNum == -1 ) {
		self->nextthink = level.time + FRAMETIME;

		if ( self->enemy ) {
			GameEntity *player;

			player = AICast_FindEntityForName( "player" );

			if ( player && player != self->enemy ) {
				prop_hits = true;
				G_Damage( self->enemy, self, player, nullptr, nullptr, 5, 0, MOD_CRUSH );

				self->die = Props_Chair_Die;

				self->die( self, self, nullptr, 10, 0 );
			}
		}

		return;
	} else
	{
		// RF, alert AI of sound event
//		AICast_AudibleEvent( self->shared.s.number, self->shared.r.currentOrigin, 384 );

		G_AddEvent( self, EV_GENERAL_SOUND, snd_chairhitground );
		VectorSubtract( self->shared.r.currentOrigin, self->shared.s.origin2, vec );
		len = VectorLength( vec );

		{
			trace_t trace;
			vec3_t end;
			GameEntity   *traceEnt;
			GameEntity   *player;
			bool reGrab = true;

			VectorCopy( self->shared.r.currentOrigin, end );
			end[2] += 1;

			SV_Trace( &trace, self->shared.r.currentOrigin, self->shared.r.mins, self->shared.r.maxs, end, self->shared.s.number, MASK_SHOT, false );

			traceEnt = &g_entities[ trace.entityNum ];

			if ( trace.startsolid ) {
				player = AICast_FindEntityForName( "player" );

				// only player can catch
				if ( traceEnt != player ) {
					reGrab = false;
				}

				// no catch when dead
				if ( traceEnt->health <= 0 ) {
					reGrab = false;
				}

				// player can throw, then switch to a two handed weapon before catching.
				// need to catch this (no pun intended)
				if ( player->shared.s.weapon && !( WEAPS_ONE_HANDED & ( 1 << ( player->shared.s.weapon ) ) ) ) {
					reGrab = false;
				}

				if ( reGrab ) {
					// pick the chair back up
					self->active = true;
					self->shared.r.ownerNum = player->shared.s.number;
					player->active = true;
					player->melee = self;
					self->nextthink = level.time + 50;

					self->think = Props_Chair_Think;
					self->touch = nullptr;
					self->die = Props_Chair_Die;
					self->shared.s.eType = ET_MOVER;

					player->client->ps.eFlags |= EF_MELEE_ACTIVE;

					SV_LinkEntity( &self->shared );
					return;
				} else {
					len = 9999;
				}
			}
		}

	}

	self->think = Props_Chair_Think;
	self->touch = Props_Chair_Touch;
	self->die = Props_Chair_Die;
	self->shared.s.eType = ET_MOVER;

	self->nextthink = level.time + FRAMETIME;

	self->shared.r.ownerNum = self->shared.s.number;

	if ( len > 256 ) {
		self->die( self, self, nullptr, 10, 0 );
	}

}


void Props_TurnLightsOff( GameEntity *self ) {
	if ( !Q_stricmp( self->classname, "props_desklamp" ) ) {
		if ( self->target ) {
			G_UseTargets( self, nullptr );
			self->target = nullptr;
		}
	}
}

void Props_Activated( GameEntity *self ) {
	vec3_t angles;
	vec3_t dest;
	vec3_t forward, right;
	vec3_t velocity;
	vec3_t prop_ang;

	GameEntity *prop;

	GameEntity   *owner;

	owner = &g_entities[self->shared.r.ownerNum];

	self->nextthink = level.time + 50;

	if ( !owner->client ) {
		return;
	}

	Props_TurnLightsOff( self );

	if ( !owner->active) {

		owner->melee = nullptr;

		self->physicsObject = true;
		self->physicsBounce = 0.2;

		self->shared.s.groundEntityNum = -1;

		self->shared.s.pos.trType = TR_GRAVITY;
		self->shared.s.pos.trTime = level.time;

		self->active = false;

		G_AddEvent( owner, EV_GENERAL_SOUND, snd_chairthrow );

		AngleVectors( owner->client->ps.viewangles, velocity, nullptr, nullptr );
		VectorScale( velocity, 250, velocity );
		velocity[2] += 100 + crandom() * 25;
		VectorCopy( velocity, self->shared.s.pos.trDelta );

		self->think = nullptr;
		self->nextthink = 0;

		prop = G_Spawn();
		prop->shared.s.modelindex = self->shared.s.modelindex;
		G_SetOrigin( prop, self->shared.r.currentOrigin );

		VectorCopy( owner->client->ps.viewangles, prop_ang );
		prop_ang[0] = 0;

		G_SetAngle( prop, prop_ang );

		prop->clipmask   = CONTENTS_SOLID | CONTENTS_MISSILECLIP;
		prop->shared.r.contents = CONTENTS_SOLID;
		prop->isProp = true;

		VectorSet( prop->shared.r.mins, -12, -12, 0 );
		VectorSet( prop->shared.r.maxs, 12, 12, 48 );

		prop->physicsObject = true;
		prop->physicsBounce = 0.2;

		VectorCopy( owner->client->ps.origin, prop->shared.s.pos.trBase );

		VectorCopy( self->shared.s.pos.trDelta, prop->shared.s.pos.trDelta );

		prop->shared.s.pos.trType = TR_GRAVITY;
		prop->shared.s.pos.trTime = level.time;

		prop->active = false;

		prop->health = self->health;

		prop->duration = self->health;

		prop->count = self->count;

		prop->think = Just_Got_Thrown;
		prop->nextthink = level.time + FRAMETIME;

		prop->takedamage = true;

		prop->wait = self->wait;

		prop->classname = self->classname;

		prop->shared.s.groundEntityNum = -1;

		VectorCopy( self->shared.r.currentOrigin, prop->shared.s.origin2 );

		prop->die = Props_Chair_Die;

		prop->shared.r.ownerNum = owner->shared.s.number;
		prop->shared.s.otherEntityNum = ENTITYNUM_WORLD;

		SV_LinkEntity( &prop->shared );

		G_FreeEntity( self );

		return;
	} else
	{
		if (    !Q_stricmp( self->classname, "props_chair_hiback" ) ||
				!Q_stricmp( self->classname, "props_chair_chat" ) ||
				!Q_stricmp( self->classname, "props_chair_chatarm" ) ||
				!Q_stricmp( self->classname, "props_chair_side" )
				) {
			self->shared.s.frame = 23;
			self->shared.s.density = 1;
		} else if ( !Q_stricmp( self->classname, "props_chair" ) ) {
			self->shared.s.frame = 28;
			self->shared.s.density = 1;
		}
	}

	SV_UnlinkEntity( &self->shared );

	// move the entity in step with the activators movement
	VectorCopy( owner->client->ps.viewangles, angles );
	angles[0] = 0;

	self->shared.s.apos.trBase[YAW] = owner->client->ps.viewangles[YAW];

	AngleVectors( angles, forward, right, nullptr );
	VectorCopy( owner->shared.r.currentOrigin, dest );

	VectorCopy( dest, self->shared.r.currentOrigin );
	VectorCopy( dest, self->shared.s.pos.trBase );

	self->shared.s.eType = ET_PROP;

	self->shared.s.otherEntityNum = owner->shared.s.number + 1;

	SV_LinkEntity( &self->shared );

}


void Prop_Check_Ground( GameEntity *self );


bool Prop_Touch( GameEntity *self, GameEntity *other, vec3_t v ) {

	vec3_t forward;
	vec3_t dest;
	vec3_t angle;
	vec3_t start, end;
	vec3_t mins, maxs;
	trace_t tr;

	if ( !other->client ) {
		return false;
	}

	vectoangles( v, angle );
	angle[0] = 0;
	AngleVectors( angle, forward, nullptr, nullptr );
	VectorClear( dest );
	VectorMA( dest, 128, forward, dest );
	VectorMA( self->shared.r.currentOrigin, 32, forward, end );

	VectorCopy( self->shared.r.currentOrigin, start );
	end[2] += 8;
	start[2] += 8;

	VectorCopy( self->shared.r.mins, mins );
	VectorCopy( self->shared.r.maxs, maxs );

	mins[2] += 1;

	SV_Trace( &tr, start, mins, maxs, end, self->shared.s.number, MASK_SHOT, false );

	if ( tr.fraction != 1 ) {
		return false;
	}

	VectorCopy( dest, self->shared.s.pos.trDelta );
	VectorCopy( self->shared.r.currentOrigin, self->shared.s.pos.trBase );

	self->shared.s.pos.trDuration = level.time + 100;
	self->shared.s.pos.trTime = level.time;
	self->shared.s.pos.trType = TR_LINEAR;

	self->physicsObject = true;

	return true;
}

void Prop_Check_Ground( GameEntity *self ) {
	vec3_t mins, maxs;
	vec3_t start, end;
	trace_t tr;

	VectorCopy( self->shared.r.currentOrigin, start );
	VectorCopy( self->shared.r.currentOrigin, end );

	end[2] -= 4;

	VectorCopy( self->shared.r.mins, mins );
	VectorCopy( self->shared.r.maxs, maxs );

//	SV_Trace( &tr, start, mins, maxs, end, self->shared.s.number, MASK_SHOT );
	SV_Trace( &tr, start, mins, maxs, end, self->shared.s.number, MASK_MISSILESHOT, false );

	if ( tr.fraction == 1 ) {
		self->shared.s.groundEntityNum = -1;
	} else {
		self->shared.s.groundEntityNum = tr.entityNum;
	}

}

void Props_Chair_Touch( GameEntity *self, GameEntity *other, trace_t *trace ) {
	vec3_t v;
	bool has_moved;

	if ( !other->client ) {
		return;
	}

	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 15 ) ) {
		return;
	}

	if ( self->active ) { // someone has activated me
		return;
	}

	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );

	has_moved = Prop_Touch( self, other, v );

	if ( /*!has_moved &&*/ ( other->shared.r.svFlags & SVF_CASTAI ) ) {
		// RF, alert AI of sound event
//		AICast_AudibleEvent( self->shared.s.number, self->shared.r.currentOrigin, 384 );

		// other could play kick animation here
		Props_Chair_Die( self, other, other, 100, 0 );
		return;
	}

	Prop_Check_Ground( self );

	if ( level.time > self->random && has_moved ) {
		// RF, alert AI of sound event
//		AICast_AudibleEvent( self->shared.s.number, self->shared.r.currentOrigin, 384 );

		G_AddEvent( self, EV_GENERAL_SOUND, snd_chaircreak );
		self->random = level.time + 1000 + ( rand() % 200 );
	}

	if ( !Q_stricmp( self->classname, "props_desklamp" ) ) {
		// player may have picked it up before
		if ( self->target ) {
			G_UseTargets( self, nullptr );
			self->target = nullptr;
		}
	}

}

void Props_Chair_Animate( GameEntity *ent ) {

	ent->touch = nullptr;

	if ( !Q_stricmp( ent->classname, "props_chair" ) ) {
		if ( ent->shared.s.frame >= 27 ) {
			ent->shared.s.frame = 27;
			G_UseTargets( ent, nullptr );
			ent->think = G_FreeEntity;
			ent->nextthink = level.time + 2000;
			ent->shared.s.time = level.time;
			ent->shared.s.time2 = level.time + 2000;
			return;
		} else
		{
			ent->nextthink = level.time + ( FRAMETIME / 2 );
		}
	} else if (
		( !Q_stricmp( ent->classname, "props_chair_side" ) ) ||
		( !Q_stricmp( ent->classname, "props_chair_chat" ) ) ||
		( !Q_stricmp( ent->classname, "props_chair_chatarm" ) ) ||
		( !Q_stricmp( ent->classname, "props_chair_hiback" ) )
		) {
		if ( ent->shared.s.frame >= 20 ) {
			ent->shared.s.frame = 20;
			G_UseTargets( ent, nullptr );
			ent->think = G_FreeEntity;
			ent->nextthink = level.time + 2000;
			ent->shared.s.time = level.time;
			ent->shared.s.time2 = level.time + 2000;
			return;
		} else
		{
			ent->nextthink = level.time + ( FRAMETIME / 2 );
		}
	} else if ( !Q_stricmp( ent->classname, "props_desklamp" ) )       {
		if ( ent->shared.s.frame >= 11 ) {
			// player may have picked it up before
			if ( ent->target ) {
				G_UseTargets( ent, nullptr );
			}

			ent->think = G_FreeEntity;
			ent->nextthink = level.time + 2000;
			ent->shared.s.time = level.time;
			ent->shared.s.time2 = level.time + 2000;
			return;
		} else
		{
			ent->nextthink = level.time + ( FRAMETIME / 2 );
		}
	}


	ent->shared.s.frame++;

	if ( ent->enemy ) {
		float ratio;
		vec3_t v;

		ratio = 2.5;
		VectorSubtract( ent->shared.r.currentOrigin, ent->enemy->shared.r.currentOrigin, v );
		moveit( ent, vectoyaw( v ), ( ent->delay * ratio * FRAMETIME ) * .001 );
	}

}

void Props_Chair_Think( GameEntity *self ) {
	trace_t tr;

	if ( self->active ) {
		Props_Activated( self );
		return;
	}

	SV_UnlinkEntity( &self->shared );

	BG_EvaluateTrajectory( &self->shared.s.pos, level.time, self->shared.s.pos.trBase );

	if ( level.time > self->shared.s.pos.trDuration ) {
		VectorClear( self->shared.s.pos.trDelta );
		self->shared.s.pos.trDuration = 0;
		self->shared.s.pos.trType = TR_STATIONARY;
	} else
	{
		vec3_t mins, maxs;

		VectorCopy( self->shared.r.mins, mins );
		VectorCopy( self->shared.r.maxs, maxs );

		mins[2] += 1;

		SV_Trace( &tr, self->shared.r.currentOrigin, mins, maxs, self->shared.s.pos.trBase, self->shared.s.number, MASK_SHOT, false );

		if ( tr.fraction == 1 ) {
			VectorCopy( self->shared.s.pos.trBase, self->shared.r.currentOrigin );
		} else
		{
			VectorCopy( self->shared.r.currentOrigin, self->shared.s.pos.trBase );
			VectorClear( self->shared.s.pos.trDelta );
			self->shared.s.pos.trDuration = 0;
			self->shared.s.pos.trType = TR_STATIONARY;
		}

	}

	if ( self->shared.s.groundEntityNum == -1 ) {

		self->physicsObject = true;
		self->physicsBounce = 0.2;

		self->shared.s.pos.trDelta[2] -= 200;

		self->shared.s.pos.trType = TR_GRAVITY;
		self->shared.s.pos.trTime = level.time;

		self->active = false;

		self->think = Just_Got_Thrown;

		if ( self->shared.s.pos.trType != TR_GRAVITY ) {
			self->shared.s.pos.trType = TR_GRAVITY;
			self->shared.s.pos.trTime = level.time;
		}
	}


	Prop_Check_Ground( self );


	self->nextthink = level.time + 50;
	SV_LinkEntity( &self->shared );
}


void Props_Chair_Die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	int quantity;
	int type;
	int deathSound;

	// if (ent->active)
	{
		GameEntity *player;

		player = AICast_FindEntityForName( "player" );

		if ( player && player->melee == ent ) {
			player->melee = nullptr;
			player->active = false;
			player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;

		} else if ( player && player->shared.s.number == ent->shared.r.ownerNum )     {
			player->active = false;
			player->melee = nullptr;
			player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
		}
	}

	ent->think = Props_Chair_Animate;
	ent->nextthink = level.time + FRAMETIME;

	ent->health = ent->duration;
	ent->delay = damage;
//	ent->enemy = inflictor;

	quantity = ent->wait;
	type = ent->count;

	Spawn_Shard( ent, inflictor, quantity, type );

	switch ( ent->count ) {
	case shard_wood:
		deathSound = snd_boardbreak;
		break;
	case shard_metal:
		deathSound = chair_metalbreak;
		break;
	default:
		deathSound = 0;
		break;
	}

	if ( deathSound ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, deathSound );
	}


	SV_UnlinkEntity( &ent->shared );

	ent->clipmask   = 0;
	ent->shared.r.contents = 0;
	ent->shared.s.eType = ET_GENERAL;

	SV_LinkEntity( &ent->shared );

}

void Props_Chair_Skyboxtouch( GameEntity *ent ) {

	GameEntity *player;

	player = AICast_FindEntityForName( "player" );

	if ( player && player->melee == ent ) {
		player->melee = nullptr;
		player->active = false;
		player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
	} else if ( player && player->shared.s.number == ent->shared.r.ownerNum )     {
		player->active = false;
		player->melee = nullptr;
		player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
	}

	ent->think = G_FreeEntity;

}

void SP_Props_Chair( GameEntity *ent ) {
	int mass;

	ent->shared.s.modelindex = G_ModelIndex( "models/furniture/chair/chair_office3.md3" );

	ent->delay = 0; // inherits damage value

	if ( G_SpawnInt( "mass", "5", &mass ) ) {
		ent->wait = mass;
	} else {
		ent->wait = 5;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType = ET_MOVER;

	ent->isProp = true;

	VectorSet( ent->shared.r.mins, -12, -12, 0 );
	VectorSet( ent->shared.r.maxs, 12, 12, 48 );

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->duration = ent->health;

	if ( !ent->count ) {
//		ent->count = 1;
		ent->count = shard_metal; // metal break sound

	}
	ent->think = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch = Props_Chair_Touch;
	ent->die = Props_Chair_Die;
	ent->takedamage = true;
	SV_LinkEntity( &ent->shared );

	snd_boardbreak = G_SoundIndex( "sound/world/boardbreak.wav" );
	snd_chaircreak = G_SoundIndex( "sound/world/chaircreak.wav" );
	chair_metalbreak = G_SoundIndex( "sound/world/metal_chair_break.wav" );

}



//----(SA)	modified

// can be one of two types, but they have the same animations/etc, so re-use what you can
/*
==============
SP_Props_GenericChair
==============
*/
void SP_Props_GenericChair( GameEntity *ent ) {
	int mass;

	ent->delay = 0; // inherits damage value

	if ( ent->model ) {
		ent->shared.s.modelindex = G_ModelIndex( ent->model );
	}

	if ( G_SpawnInt( "mass", "5", &mass ) ) {
		ent->wait = mass;
	} else {
		ent->wait = 5;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType    = ET_MOVER;

	ent->isProp     = true;

	VectorSet( ent->shared.r.mins, -12, -12, 0 );
	VectorSet( ent->shared.r.maxs, 12, 12, 48 );

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->duration = ent->health;

	if ( !ent->count ) {
		ent->count = 1;
	}

	ent->think = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch = Props_Chair_Touch;
	ent->die = Props_Chair_Die;
	ent->takedamage = true;
	SV_LinkEntity( &ent->shared );

	snd_boardbreak = G_SoundIndex( "sound/world/boardbreak.wav" );
	snd_glassbreak = G_SoundIndex( "sound/world/glassbreak.wav" );
	snd_metalbreak = G_SoundIndex( "sound/world/metalbreak.wav" );
	snd_ceramicbreak = G_SoundIndex( "sound/world/ceramicbreak.wav" );
	snd_chaircreak = G_SoundIndex( "sound/world/chaircreak.wav" );
	snd_chairthrow = G_SoundIndex( "sound/props/throw/chairthudgrunt.wav" );
	snd_chairhitground = G_SoundIndex( "sound/props/chair/chairthud.wav" );
}


/*
==============
SP_Props_ChairChat
==============
*/
void SP_Props_ChairChat( GameEntity *ent ) {
	if ( !ent->model ) {
		ent->model = "models/furniture/chair/chair_chat.md3";
	}
	SP_Props_GenericChair( ent );

	ent->count = shard_wood; // wood break sound
}

/*
==============
SP_Props_ChairChatArm
==============
*/
void SP_Props_ChairChatArm( GameEntity *ent ) {
	if ( !ent->model ) {
		ent->model = "models/furniture/chair/chair_chatarm.md3";
	}
	SP_Props_GenericChair( ent );

	ent->count = shard_wood; // wood break sound
}

/*
==============
SP_Props_ChairSide
==============
*/
void SP_Props_ChairSide( GameEntity *ent ) {
	if ( !ent->model ) {
		ent->model = "models/furniture/chair/sidechair3.md3";
	}
	SP_Props_GenericChair( ent );

	ent->count = shard_wood; // wood break sound
}


void SP_Props_ChairHiback( GameEntity *ent ) {
	if ( !ent->model ) {
		ent->model = "models/furniture/chair/hiback5.md3";
	}
	SP_Props_GenericChair( ent );

	ent->count = shard_wood; // wood break sound
}



/*QUAKED props_desklamp (.8 .6 .2) (-16 -16 0) (16 16 32)
point entity
health = default = 10
wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/
void SP_Props_Desklamp( GameEntity *ent ) {
	int mass;

	ent->shared.s.modelindex = G_ModelIndex( "models/furniture/lights/desklamp.md3" );

	ent->delay = 0; // inherits damage value

	if ( G_SpawnInt( "mass", "5", &mass ) ) {
		ent->wait = mass;
	} else {
		ent->wait = 2;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType = ET_MOVER;

	ent->isProp = true;
	ent->nopickup = true;

	VectorSet( ent->shared.r.mins, -6, -6, 0 );
	VectorSet( ent->shared.r.maxs, 6, 6, 14 );

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->duration = ent->health;

	if ( !ent->count ) {
		ent->count = 2;
	}

	ent->think = Props_Chair_Think;
	ent->nextthink = level.time + FRAMETIME;

	ent->touch = Props_Chair_Touch;
	ent->die = Props_Chair_Die;
	ent->takedamage = true;
	SV_LinkEntity( &ent->shared );

	snd_boardbreak = G_SoundIndex( "sound/world/boardbreak.wav" );
	snd_glassbreak = G_SoundIndex( "sound/world/glassbreak.wav" );
	snd_metalbreak = G_SoundIndex( "sound/world/metalbreak.wav" );
	snd_ceramicbreak = G_SoundIndex( "sound/world/ceramicbreak.wav" );
	snd_chaircreak = G_SoundIndex( "sound/world/chaircreak.wav" );
}

