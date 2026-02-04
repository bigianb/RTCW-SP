#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

/*
	toggle hide or show (including collisions) this entity
*/
void Use_Static( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	if ( ent->shared.r.linked ) {
		SV_UnlinkEntity( &ent->shared );
		// DISABLED since func_static will carve up AAS anyway, so blocking makes no sense
		// RF, AAS areas are now free
		//if (ent->model)
		//	G_SetAASBlockingEntity( ent, false );
	} else {
		SV_LinkEntity( &ent->shared );
		// DISABLED since func_static will carve up AAS anyway, so blocking makes no sense
		// RF, AAS areas are now occupied
		//if (ent->model)
		//	G_SetAASBlockingEntity( ent, true );
	}
}

void Static_Pain( GameEntity *ent, GameEntity *attacker, int damage, vec3_t point ) {
	vec3_t temp;

	if ( ent->spawnflags & 4 ) {
		if ( level.time > ent->wait + ent->delay + rand() % 1000 + 500 ) {
			ent->wait = level.time;
		} else {
			return;
		}

		// TBD only venom mg42 rocket and grenade can inflict damage
		if ( attacker && attacker->client
			 && ( attacker->shared.s.weapon == WP_VENOM
				  || attacker->shared.s.weapon == WP_GRENADE_LAUNCHER
				  || attacker->client->ps.persistant[PERS_HWEAPON_USE] ) ) {

			VectorCopy( ent->shared.r.currentOrigin, temp );
			VectorCopy( ent->pos3, ent->shared.r.currentOrigin );
			Spawn_Shard( ent, attacker, 3, ent->count );
			VectorCopy( temp, ent->shared.r.currentOrigin );
		}
		return;
	}

	if ( level.time > ent->wait + ent->delay + rand() % 1000 + 500 ) {
		G_UseTargets( ent, nullptr );
		ent->wait = level.time;
	}

}

/*QUAKED func_static (0 .5 .8) ? start_invis pain painEFX
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
"start_invis" will start the entity as non-existant
If targeted, it will toggle existance when triggered

pain will use its target

When using pain you will need to specify the delay time
value of 1 = 1 sec 2 = 2 sec so on...
default is 1 sec you can use decimals
example :
delay
1.27

painEFX will spawn a shards
example:
shard
4
will spawn rubble

shard default is 4

shard =
shard_glass = 0,
shard_wood = 1,
shard_metal = 2,
shard_ceramic = 3,
shard_pebbles = 4
*/
void SP_func_static( GameEntity *ent ) {
	if ( ent->model2 ) {
		ent->shared.s.modelindex2 = G_ModelIndex( ent->model2 );
	}
	SV_SetBrushModel( &ent->shared, ent->model );
	InitMover( ent );
	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.origin, ent->shared.r.currentOrigin );
	ent->use = Use_Static;

	if ( ent->spawnflags & 1 ) {
		SV_UnlinkEntity( &ent->shared );
	}

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = true;
		}
	}

	if ( ent->spawnflags & 2 || ent->spawnflags & 4 ) {
		ent->pain = Static_Pain;

		if ( !ent->delay ) {
			ent->delay = 1000;
		} else {
			ent->delay *= 1000;
		}

		ent->takedamage = true;

		ent->isProp = true;

		ent->health = 9999;

		if ( !( ent->count ) ) {
			ent->count = 4;
		}
	}
}
