#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

// Flame Barrel uses some of the chairs logic.

// in g_props.cpp
void moveit( GameEntity *ent, float yaw, float dist ); 
void propExplosion( GameEntity *ent );

// in chairs.cpp
void Props_Chair_Think( GameEntity *self );

#define FLAMEBARREL_SMOKING    1
#define FLAMEBARREL_NOLID      2
#define FLAMEBARREL_OIL        4
#define FLAMEBARREL_HAS_OILSLICK 8

/*QUAKED props_flamebarrel (.8 .6 .2) (-13 -13 0) (13 13 40) SMOKING NOLID OIL -
angle will determine which way the lid will fly off when it explodes

when selecting the OIL spawnflag you have the option of giving it a target
this will ensure that the oil sprite will show up where you want it
( be sure to put it on the floor )
the default is in the middle of the barrel on the floor
*/
void Props_Barrel_Touch( GameEntity *self, GameEntity *other, trace_t *trace )
{
    // barrels cant move
	return; 
}

void Props_Barrel_Animate( GameEntity *ent )
{
	if ( ent->shared.s.frame == 14 ) {
        ent->think = G_FreeEntity;
        ent->nextthink = level.time + 25000;
        return;
	} 

	ent->nextthink = level.time + ( FRAMETIME / 2 );
	ent->shared.s.frame++;

	if ( !( ent->spawnflags & FLAMEBARREL_SMOKING ) ) {
        // not smoking
		float ratio = 2.5;
        vec3_t v;
		VectorSubtract( ent->shared.r.currentOrigin, ent->enemy->shared.r.currentOrigin, v );
		moveit( ent, vectoyaw( v ), ( ent->delay * ratio * FRAMETIME ) * .001 );
	}
}

void barrel_smoke( GameEntity *ent )
{
	vec3_t point;

	VectorCopy( ent->shared.r.currentOrigin, point );

	GameEntity   *tent = G_TempEntity( point, EV_SMOKE );
	VectorCopy( point, tent->shared.s.origin );
	tent->shared.s.time = 4000;
	tent->shared.s.time2 = 1000;
	tent->shared.s.density = 0;
	tent->shared.s.angles2[0] = 8;
	tent->shared.s.angles2[1] = 64;
	tent->shared.s.angles2[2] = 50;

}

void smoker_think( GameEntity *ent )
{
	ent->count--;

	if ( !ent->count ) {
		G_FreeEntity( ent );
	} else {
		barrel_smoke( ent );
		ent->nextthink = level.time + FRAMETIME;
	}

}

void SP_OilSlick( GameEntity *ent )
{
	GameEntity   *target = nullptr;
	vec3_t point;

	if ( ent->target ) {
		target = G_Find( nullptr, FOFS( targetname ), ent->target );
	}

	if ( target ) {
		VectorCopy( target->shared.s.origin, point );
		point[2] = ent->shared.r.currentOrigin[2]; // just in case
	} else {
		VectorCopy( ent->shared.r.currentOrigin, point );
	}

	GameEntity *tent = G_TempEntity( ent->shared.r.currentOrigin, EV_OILSLICK );
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	tent->shared.s.angles2[0] = 16;
	tent->shared.s.angles2[1] = 48;
	tent->shared.s.angles2[2] = 10000;
	tent->shared.s.density = ent->shared.s.number;
}

void OilParticles_think( GameEntity *ent )
{
	GameEntity   *owner = &g_entities[ent->shared.s.density];

	if ( owner && owner->takedamage && ent->count2 > level.time - 5000 ) {
		ent->nextthink = ( level.time + FRAMETIME / 2 );

		GameEntity * tent = G_TempEntity( ent->shared.r.currentOrigin, EV_OILPARTICLES );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		tent->shared.s.time = ent->count2;
		tent->shared.s.density = ent->shared.s.density;
		VectorCopy( ent->rotate, tent->shared.s.origin2 );
	} else {
		G_FreeEntity( ent );
	}
}

void Delayed_Leak_Think( GameEntity *ent )
{
	vec3_t point;
	VectorCopy( ent->shared.r.currentOrigin, point );

	GameEntity *tent = G_TempEntity( point, EV_OILSLICK );
	VectorCopy( point, tent->shared.s.origin );

	tent->shared.s.angles2[0] = 0;
	tent->shared.s.angles2[1] = 0;
	tent->shared.s.angles2[2] = 2000;
	tent->shared.s.density = ent->count;
}

bool validOilSlickSpawnPoint( vec3_t point, GameEntity *ent ) {
	trace_t tr;
	vec3_t end;
	GameEntity *traceEnt;

	VectorCopy( point, end );
	end[2] -= 9999;

	SV_Trace( &tr, point, nullptr, nullptr, end, ent->shared.s.number, MASK_SHOT, false );

	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt && traceEnt->classname ) {
		if ( !Q_stricmp( traceEnt->classname, "worldspawn" ) ) {
			if ( tr.plane.normal[0] == 0 && tr.plane.normal[1] == 0 && tr.plane.normal[2] == 1 ) {
				return true;
			}
		}
	}

	return false;

}

void SP_OilParticles( GameEntity *ent )
{
    // in client
}


void Props_Barrel_Pain( GameEntity *ent, GameEntity *attacker, int damage, vec3_t point )
{

	if ( ent->health <= 0 ) {
		return;
	}

	if ( !( ent->spawnflags & FLAMEBARREL_HAS_OILSLICK ) ) {
		SP_OilSlick( ent );
		ent->spawnflags |= FLAMEBARREL_HAS_OILSLICK;
	}

	ent->count2++;

	if ( ent->count2 < 6 ) {
		SP_OilParticles( ent );
	}

}

void OilSlick_remove_think( GameEntity *ent )
{
	GameEntity *tent = G_TempEntity( ent->shared.r.currentOrigin, EV_OILSLICKREMOVE );
	tent->shared.s.density = ent->shared.s.density;
}

void OilSlick_remove( GameEntity *ent )
{
	GameEntity *remove = G_Spawn();
	remove->shared.s.density = ent->shared.s.number;
	remove->think = OilSlick_remove_think;
	remove->nextthink = level.time + 1000;
	VectorCopy( ent->shared.r.currentOrigin, remove->shared.r.currentOrigin );
	SV_LinkEntity( &remove->shared );
}

void Props_Barrel_Die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod )
{
	int quantity;
	int type;
	vec3_t dir;
	GameEntity *smoker;

	if ( ent->spawnflags & 1 ) {
		smoker = G_Spawn();
		smoker->nextthink = level.time + FRAMETIME;
		smoker->think = smoker_think;
		smoker->count = 150 + rand() % 100;
		G_SetOrigin( smoker, ent->shared.r.currentOrigin );
		SV_LinkEntity( &smoker->shared );
	}

	G_UseTargets( ent, nullptr );

	if ( ent->spawnflags & 4 ) {
		OilSlick_remove( ent );
	}

	ent->health = 100;
	propExplosion( ent );
	ent->health = 0;

	ent->takedamage = false;

	AngleVectors( ent->shared.r.currentAngles, dir, nullptr, nullptr );
	dir[2] = 1;

	if ( !( ent->spawnflags & 2 ) ) {
		fire_flamebarrel( ent, ent->shared.r.currentOrigin, dir );
	}

	ent->touch = nullptr;

	ent->think = Props_Barrel_Animate;
	ent->nextthink = level.time + FRAMETIME;

	ent->health = ent->duration;
	ent->delay = damage;
	ent->enemy = inflictor;

	quantity = ent->wait;
	type = ent->count;

	if ( inflictor ) {
		Spawn_Shard( ent, inflictor, quantity, type );
	}

	Prop_Break_Sound( ent );

	SV_UnlinkEntity( &ent->shared );

	ent->clipmask   = 0;
	ent->shared.r.contents = 0;
	ent->shared.s.eType = ET_GENERAL;

	SV_LinkEntity( &ent->shared );
}

void Props_OilSlickSlippery( GameEntity *ent )
{
	GameEntity *player = AICast_FindEntityForName( "player" );

	if ( player ) {
        vec3_t vec;
		VectorSubtract( player->shared.r.currentOrigin, ent->shared.r.currentOrigin, vec );
		float len = VectorLength( vec );

		if ( len < 64 && player->shared.s.groundEntityNum != -1 ) {
			len = VectorLength( player->client->ps.velocity );

			if ( len && !( player->client->ps.pm_time ) ) {
                vec3_t kvel, dir;
				VectorSet( dir, fabs( crandom() ), fabs( crandom() ), 0 );
				VectorScale( dir, 32, kvel );
				VectorAdd( player->client->ps.velocity, kvel, player->client->ps.velocity );

                player->client->ps.pm_time = 64;
                player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}

		}
	}
}

void Props_Barrel_Think( GameEntity *self )
{
	self->active = false;
	Props_Chair_Think( self );

	if ( self->spawnflags & FLAMEBARREL_HAS_OILSLICK ) {
        // there is an oil slick
		Props_OilSlickSlippery( self );
	}
}

void SP_Props_Flamebarrel( GameEntity *ent )
{
	if ( ent->spawnflags & FLAMEBARREL_OIL ) {
		ent->shared.s.modelindex = G_ModelIndex( "models/furniture/barrel/barrel_c.md3" );
	} else if ( ent->spawnflags & FLAMEBARREL_SMOKING ) {
		ent->shared.s.modelindex = G_ModelIndex( "models/furniture/barrel/barrel_d.md3" );
	} else {
		ent->shared.s.modelindex = G_ModelIndex( "models/furniture/barrel/barrel_b.md3" );
	}

	ent->delay = 0; // inherits damage value

    int mass;
	if ( G_SpawnInt( "mass", "5", &mass ) ) {
		ent->wait = mass;
	} else {
		ent->wait = 10;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType = ET_MOVER;

	ent->isProp = true;
	ent->nopickup = true;

	VectorSet( ent->shared.r.mins, -13, -13, 0 );
	VectorSet( ent->shared.r.maxs, 13, 13, 36 );

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !ent->health ) {
		ent->health = 20;
	}

	ent->duration = ent->health;

	ent->count = 2; // metal shards

	ent->think = Props_Barrel_Think;
	ent->nextthink = level.time + FRAMETIME;
	ent->touch = Props_Barrel_Touch;
	ent->die = Props_Barrel_Die;

	if ( ent->spawnflags & FLAMEBARREL_OIL ) {
		ent->pain = Props_Barrel_Pain;
	}

	ent->takedamage = true;
	SV_LinkEntity( &ent->shared );
}
