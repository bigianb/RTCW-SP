#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity


/*QUAKED props_smokedust (.8 .46 .16) (-8 -8 -8) (8 8 8)
health = how many pieces 16 is default
*/

void smokedust_use( GameEntity *ent, GameEntity *self, GameEntity *activator )
{
	vec3_t forward;
	AngleVectors( ent->shared.r.currentAngles, forward, nullptr, nullptr );

	for (int i = 0; i < ent->health; i++ ) {
		GameEntity* tent = G_TempEntity( ent->shared.r.currentOrigin, EV_SMOKE );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		VectorCopy( forward, tent->shared.s.origin2 );
		tent->shared.s.time = 1000;
		tent->shared.s.time2 = 750;
		tent->shared.s.density = 3;
	}
}

void SP_SmokeDust( GameEntity *ent )
{
	ent->use = smokedust_use;

	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;

	if ( !ent->health ) {
		ent->health = 16;
	}
	SV_LinkEntity( &ent->shared );
}


/*QUAKED props_dust (.7 .3 .16) (-8 -8 -8) (8 8 8) WHITE
you should give this ent a target use a not null
or you could set its angles in the editor
*/

void dust_use( GameEntity *ent, GameEntity *self, GameEntity *activator )
{
	if ( ent->target ) {
		GameEntity* tent = G_TempEntity( ent->shared.r.currentOrigin, EV_DUST );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		VectorCopy( ent->shared.r.currentAngles, tent->shared.s.angles );
		if ( ent->spawnflags & 1 ) {
			tent->shared.s.density = 1;
		}
	} else {
        vec3_t forward;
		AngleVectors( ent->shared.r.currentAngles, forward, nullptr, nullptr );

		GameEntity* tent = G_TempEntity( ent->shared.r.currentOrigin, EV_DUST );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		VectorCopy( forward, tent->shared.s.angles );
		if ( ent->spawnflags & 1 ) {
			tent->shared.s.density = 1;
		}
	}
}

void dust_angles_think( GameEntity *ent )
{
	GameEntity* target = G_Find( nullptr, FOFS( targetname ), ent->target );

	if ( !target ) {
		return;
	}

    vec3_t vec;
	VectorSubtract( ent->shared.s.origin, target->shared.s.origin, vec );
	VectorCopy( vec, ent->shared.r.currentAngles );
	SV_LinkEntity( &ent->shared );

}

void SP_Dust( GameEntity *ent )
{
	ent->use = dust_use;
	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;

	if ( ent->target ) {
		ent->think = dust_angles_think;
		ent->nextthink = level.time + FRAMETIME;
	}

	SV_LinkEntity( &ent->shared );
}
