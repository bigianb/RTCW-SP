#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

#define FLAMETHROWER_TRACKING 1
#define FLAMETHROWER_NOSOUND  2

/*QUAKED props_flamethrower (.6 .7 .3) (-8 -8 -8) (8 8 8) TRACKING NOSOUND
the effect occurs when this entity is used
needs to aim at a info_notnull
"duration" how long the effect is going to last for example 1.2 sec 2.7 sec
"random" how long of a random variance so the effect isnt exactly the same each time for example 1.1 sec or 0.2 sec
"size" valid ranges are 1.0 to 0.1

NOSOUND - silent (duh)
*/
void props_flamethrower_think( GameEntity *ent )
{
	vec3_t vec, angles;
	GameEntity   *target = nullptr;

	if ( ent->spawnflags & FLAMETHROWER_TRACKING ) { 
		if ( ent->target ) {
			target = G_Find( nullptr, FOFS( targetname ), ent->target );
		}

		if ( !target ) {
			VectorSet( ent->shared.s.apos.trBase, 0, 0, 1 );
		} else
		{
			VectorSubtract( target->shared.s.origin, ent->shared.s.origin, vec );
			VectorNormalize( vec );
			vectoangles( vec, angles );
			VectorCopy( angles, ent->shared.s.apos.trBase );
		}
	}

	if ( ( ent->timestamp + ent->duration ) > level.time ) {

		ent->shared.s.eFlags |= EF_FIRING;

		ent->nextthink = level.time + 50;

		{
			int rval;
			int rnd;

			if ( ent->random ) {
				rval = ent->random * 1000;
				rnd = rand() % rval;
			} else {
				rnd = 0;
			}

			ent->timestamp = level.time + rnd;
			ent->nextthink = ent->timestamp + 50;
		}
	} else {
		ent->shared.s.eFlags &= ~EF_FIRING;
	}

}

void props_flamethrower_use( GameEntity *ent, GameEntity *other, GameEntity *activator )
{
	int rnd;

	if ( ent->spawnflags & FLAMETHROWER_NOSOUND ) {
		ent->shared.s.eFlags &= ~EF_FIRING;
		ent->spawnflags &= ~FLAMETHROWER_NOSOUND;
		ent->think = nullptr; 
		ent->nextthink = 0;
		return;
	} 
	ent->spawnflags |= FLAMETHROWER_NOSOUND;

	if ( ent->random ) {
		int rval = ent->random * 1000;
		rnd = rand() % rval;
	} else {
		rnd = 0;
	}

	ent->timestamp = level.time + rnd;

	ent->think = props_flamethrower_think;
	ent->nextthink = level.time + 50;

}

void props_flamethrower_init( GameEntity *ent ) {
	GameEntity *target = nullptr;
	vec3_t vec;
	vec3_t angles;

	if ( ent->target ) {
		target = G_Find( nullptr, FOFS( targetname ), ent->target );
	}

	if ( !target ) {
		VectorSet( ent->shared.s.apos.trBase, 0, 0, 1 );
	} else {
		VectorSubtract( target->shared.s.origin, ent->shared.s.origin, vec );
		VectorNormalize( vec );
		vectoangles( vec, angles );

		VectorCopy( angles, ent->shared.s.apos.trBase );
		VectorCopy( angles, ent->shared.s.angles ); // RF, added to fix wierd release build issues
	}

	SV_LinkEntity( &ent->shared );

}

void SP_props_flamethrower( GameEntity *ent ) {
	const char *size;
	float dsize;

	ent->think = props_flamethrower_init;
	ent->nextthink = level.time + 50;
	ent->use = props_flamethrower_use;

	G_SetOrigin( ent, ent->shared.s.origin );

	if ( !( ent->duration ) ) {
		ent->duration = 1000;
	} else {
		ent->duration *= 1000;
	}


	G_SpawnString( "size", "0", &size );
	dsize = atof( size );
	if ( !dsize ) {
		dsize = 1;
	}
	ent->accuracy = dsize;

	if ( ent->spawnflags & FLAMETHROWER_NOSOUND ) { 
		ent->shared.s.density = 1;
	}

	ent->shared.s.eType = ET_FLAMETHROWER_PROP;
	ent->shared.r.svFlags |= SVF_BROADCAST;
}
