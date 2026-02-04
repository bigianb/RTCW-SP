#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

void DropToFloor( GameEntity *ent );
void moveit( GameEntity *ent, float yaw, float dist );
void InitProp( GameEntity *ent );

/*QUAKED props_crate_64 (.8 .6 .2) (-32 -32 0) (32 32 64)
breakable pushable

  health = default = 20
wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

/*QUAKED props_crate_32 (.8 .6 .2) (-16 -16 0) (16 16 32)
breakable pushable

  health = default = 20
wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

/*QUAKED props_crate_32x64 (.8 .6 .2) ?
requires an origin brush

breakable NOT pushable

brushmodel only

  health = default = 20
wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

void touch_crate_64( GameEntity *self, GameEntity *other, trace_t *trace ) {
	float ratio;
	vec3_t v;

	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 31 ) ) {
		return;
	}

	ratio = 1.5;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

void crate_animate( GameEntity *ent ) {
	if ( ent->shared.s.frame == 17 ) {
		G_UseTargets( ent, nullptr );
		ent->think = G_FreeEntity;
		ent->nextthink = level.time + 2000;
		ent->shared.s.time = level.time;
		ent->shared.s.time2 = level.time + 2000;
		return;
	}

	ent->shared.s.frame++;
	ent->nextthink = level.time + ( FRAMETIME / 2 );
}

void crate_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	int quantity;
	int type;

	quantity = ent->wait;
	type = ent->count;

	Spawn_Shard( ent, inflictor, quantity, type );

	ent->takedamage = false;
	ent->think = crate_animate;
	ent->nextthink = level.time + FRAMETIME;
	ent->touch = nullptr;

	SV_UnlinkEntity( &ent->shared );

	ent->clipmask   = 0;
	ent->shared.r.contents = 0;
	ent->shared.s.eType = ET_GENERAL;

	SV_LinkEntity( &ent->shared );

}

void SP_crate_64( GameEntity *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/furniture/crate/crate64.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -32, -32, 0 );
	VectorSet( self->shared.r.maxs, 32, 32, 64 );

	self->shared.s.eType = ET_MOVER;

	self->isProp = true;
	self->nopickup = true;
	G_SetOrigin( self, self->shared.s.origin );
	G_SetAngle( self, self->shared.s.angles );

	self->touch = touch_crate_64;
	self->die = crate_die;

	self->takedamage = true;

	if ( !self->health ) {
		self->health = 20;
	}

	if ( !self->count ) {
		self->count = 1;
	}

	if ( !self->wait ) {
		self->wait = 10;
	}

	self->isProp = true;
	self->nopickup = true;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

void SP_crate_32( GameEntity *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/furniture/crate/crate32.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -16, -16, 0 );
	VectorSet( self->shared.r.maxs, 16, 16, 32 );

	self->shared.s.eType = ET_MOVER;

	self->isProp = true;
	self->nopickup = true;
	G_SetOrigin( self, self->shared.s.origin );
	G_SetAngle( self, self->shared.s.angles );

	self->touch = touch_crate_64;
	self->die = crate_die;

	self->takedamage = true;

	if ( !self->health ) {
		self->health = 20;
	}

	if ( !self->count ) {
		self->count = 1;
	}

	if ( !self->wait ) {
		self->wait = 10;
	}

	self->isProp = true;
	self->nopickup = true;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

//////////////////////////////////////////////

void props_crate32x64_think( GameEntity *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 17 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{
		ent->clipmask = 0;
		ent->shared.r.contents = 0;
		ent->takedamage = false;

		G_UseTargets( ent, nullptr );
	}

}

void props_crate32x64_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	ent->think = props_crate32x64_think;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_Props_Crate32x64( GameEntity *ent )
{
	SV_SetBrushModel( &ent->shared, ent->model );
    ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/crate/crate32x64.md3" );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->takedamage = true;
	ent->clipmask = CONTENTS_SOLID;
	ent->die = props_crate32x64_die;
	SV_LinkEntity( &ent->shared );
}
