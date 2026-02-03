#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

extern void AimAtTarget( GameEntity * self );

static
int sniper_sound;

void Use_Shooter( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	vec3_t dir;
	float deg;
	vec3_t up, right;

	// see if we have a target
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->shared.r.currentOrigin, ent->shared.s.origin, dir );
		if ( ent->shared.s.weapon != WP_SNIPER ) {
			VectorNormalize( dir );
		}
	} else {
		VectorCopy( ent->movedir, dir );
	}

	if ( ent->shared.s.weapon == WP_MORTAR ) {
		AimAtTarget( ent );   // store in ent->shared.s.origin2 the direction/force needed to pass through the target
		VectorCopy( ent->shared.s.origin2, dir );
	}

	if ( ent->shared.s.weapon != WP_SNIPER ) {
		// randomize a bit
		PerpendicularVector( up, dir );
		CrossProduct( up, dir, right );

		deg = crandom() * ent->random;
		VectorMA( dir, deg, up, dir );

		deg = crandom() * ent->random;
		VectorMA( dir, deg, right, dir );

		VectorNormalize( dir );
	}

	switch ( ent->shared.s.weapon ) {
	case WP_GRENADE_LAUNCHER:
		VectorScale( dir, 700, dir );                 //----(SA)	had to add this as fire_grenade now expects a non-normalized direction vector
		fire_grenade( ent, ent->shared.s.origin, dir, WP_GRENADE_LAUNCHER );
		break;
	case WP_PANZERFAUST:
		fire_rocket( ent, ent->shared.s.origin, dir );
		break;

	case WP_MONSTER_ATTACK1:
		fire_zombiespit( ent, ent->shared.s.origin, dir );
		break;

		// Rafael sniper
	case WP_SNIPER:
		fire_lead( ent, ent->shared.s.origin, dir, ent->damage );
		break;
		// done

	case WP_MORTAR:
		AimAtTarget( ent );   // store in ent->shared.s.origin2 the direction/force needed to pass through the target
		VectorScale( dir, VectorLength( ent->shared.s.origin2 ), dir );
		fire_mortar( ent, ent->shared.s.origin, dir );
		break;

	}

	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
}

static void InitShooter_Finish( GameEntity *ent ) {
	ent->enemy = G_PickTarget( ent->target );
	ent->think = 0;
	ent->nextthink = 0;
}

void InitShooter( GameEntity *ent, int weapon ) {
	ent->use = Use_Shooter;
	ent->shared.s.weapon = weapon;

	// Rafael sniper
	if ( weapon != WP_SNIPER ) {
		RegisterItem( BG_FindItemForWeapon( (weapon_t)weapon ) );
	}
	// done

	G_SetMovedir( ent->shared.s.angles, ent->movedir );

	if ( !ent->random ) {
		ent->random = 1.0;
	}

	if ( ent->shared.s.weapon != WP_SNIPER ) {
		ent->random = sin( M_PI * ent->random / 180 );
	}

	// target might be a moving object, so we can't set movedir for it
	if ( ent->target ) {
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	SV_LinkEntity( &ent->shared );
}

/*QUAKED shooter_mortar (1 0 0) (-16 -16 -16) (16 16 16) SMOKE_FX FLASH_FX
Lobs a mortar so that it will pass through the info_notnull targeted by this entity
"random" the number of degrees of deviance from the taget. (1.0 default)
if LAUNCH_FX is checked a smoke effect will play at the origin of this entity.
if FLASH_FX is checked a muzzle flash effect will play at the origin of this entity.
*/
void SP_shooter_mortar( GameEntity *ent ) {
	// (SA) TODO: must have a self->target.  Do a check/print if this is not the case.
	InitShooter( ent, WP_MORTAR );

	if ( ent->spawnflags & 1 ) {   // smoke at source
	}
	if ( ent->spawnflags & 2 ) {   // muzzle flash at source
	}
}

/*QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_rocket( GameEntity *ent ) {
	InitShooter( ent, WP_PANZERFAUST );
}

/*QUAKED shooter_zombiespit (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_zombiespit( GameEntity *ent ) {
	InitShooter( ent, WP_MONSTER_ATTACK1 );
}


void use_shooter_tesla( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	GameEntity   *tent;

	if ( ent->shared.r.linked ) {
		SV_UnlinkEntity( &ent->shared );
	} else
	{
		tent = G_PickTarget( ent->target );
		VectorCopy( tent->shared.s.origin, ent->shared.s.origin2 );

		ent->active = 0;
		SV_LinkEntity( &ent->shared );
	}
}

//----(SA)	added
/*QUAKED shooter_tesla (1 0 0) (-16 -16 -16) (16 16 16) START_ON DLIGHT
START_ON means it starts out active, the default is to start off and fire when triggered
DLIGHT will have a built-in dlight flashing too (use color picker to set color of dlight. (def: blue) )

"sticktime" - how long each bolt should 'stick' to an impact point (def: .5)
"random" - how far away to drift from the target. (def: 0.0)
"width" - width of the bolts (def: 20)
"count" - number of bolts to fire per impact point.  (def: 2)
"dlightsize" - how big to make the attached light.  (def: 500)
*/

void shooter_tesla_finish_spawning( GameEntity *ent ) {
	GameEntity   *tent;  // target ent

	ent->think = 0;
	ent->nextthink = 0;

	// locate the target and set the location
	tent = G_PickTarget( ent->target );
	if ( !tent ) { // if there's a problem with tent
		Com_Printf( "shooter_tesla (%s) at %s has no target.\n", ent->target, vtos( ent->shared.s.origin ) );
		return;
	}

	VectorCopy( tent->shared.s.origin, ent->shared.s.origin2 );

	if ( ent->spawnflags & 1 ) {   // START_ON
		ent->active = 0;
		SV_LinkEntity( &ent->shared );
	}
}

void SP_shooter_tesla( GameEntity *ent ) {

	float tempf;

	//	it's a shooter_ since it will act like any other shooter, except
	//	the tesla is a client-side effect that reports damage back to the
	//	game, so this will create an linked-entity on the client that acts as dictated
	//	and, if necessary, passes back damage info like the weapon.  this will
	//	keep the server->client messages to a minimum (start firing/stop firing)
	//	rather than sending an event for each bolt.
	ent->shared.s.eType        = ET_TESLA_EF;
	ent->use            = use_shooter_tesla;

	// set number of bolts
	if ( ent->count ) {
		ent->shared.s.density = ent->count;
	} else {
		ent->shared.s.density = 2;
	}


	// width
	if ( G_SpawnFloat( "width", "", &tempf ) ) {
		ent->shared.s.frame = (int)tempf;
	} else {
		ent->shared.s.frame = 20;
	}


	// 'sticky' time (stored in .weapon)
	if ( G_SpawnFloat( "sticktime", "", &tempf ) ) {
		ent->shared.s.time2 = (int)( tempf * 1000.0f );
	} else {
		ent->shared.s.time2 = 500; // default to 1/2 sec

	}
	// randomness
	ent->shared.s.angles2[0] = ent->random;


	// DLIGHT
	if ( ent->spawnflags & 2 ) {
		int dlightsize;
		if ( G_SpawnInt( "dlightsize", "", &dlightsize ) ) {
			ent->shared.s.time = dlightsize;
		} else {
			ent->shared.s.time = 500;
		}

		if ( ent->random ) {
			ent->shared.s.time2 = ent->random;
		} else {
			ent->shared.s.time2 = 4; // dlight randomness

		}
		if ( ent->dl_color[0] <= 0 &&    // if it's black or has no color assigned
			 ent->dl_color[1] <= 0 &&
			 ent->dl_color[2] <= 0 ) {
			// default is the same color as the tesla weapon
			ent->dl_color[0] = 0.2f;
			ent->dl_color[1] = 0.6f;
			ent->dl_color[2] = 1.0f;
		}

		ent->dl_color[0] = ent->dl_color[0] * 255;
		ent->dl_color[1] = ent->dl_color[1] * 255;
		ent->dl_color[2] = ent->dl_color[2] * 255;

		ent->shared.s.dl_intensity = (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 );

	} else {
		ent->shared.s.dl_intensity = 0;
	}


	// finish up after everything has spawned in so we know all potential targets are ready
	ent->think = shooter_tesla_finish_spawning;
	ent->nextthink = level.time + 100;
}
//----(SA)	end


/*QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_grenade( GameEntity *ent ) {
	InitShooter( ent, WP_GRENADE_LAUNCHER );
}

// Rafael sniper
/*QUAKED shooter_sniper (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
"damage" the amount of damage sniper will cause when he hits his target default is 10
"radius" is the dist the target would need to travel before sniper lost his beat default 256
"delay"	 is the rate of fire defaults to 1 sec
*/
void SP_shooter_sniper( GameEntity *ent ) {

	const char        *damage;

	if ( G_SpawnString( "damage", "0", &damage ) ) {
		ent->damage = atoi( damage );
	}

	if ( !ent->damage ) {
		ent->damage = 10;
	}
	if ( !ent->radius ) { // radius
		ent->radius = 256;
	}
	if ( !ent->delay ) {
		ent->delay = 1.0; // one sec

	}
	InitShooter( ent, WP_SNIPER );

	ent->delay *= 1000;

	ent->wait = level.time + ent->delay;
}

void brush_activate_sniper( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	GameEntity *sniper;
	float dist;
	vec3_t vec;
	GameEntity *player;

	player = AICast_FindEntityForName( "player" );

	if ( player && player != other ) {
		// Com_Printf ("other: %s\n", other->aiName);
		return;
	}

	if ( other->client ) {
		ent->enemy = other;
	}

	sniper = G_Find( nullptr, FOFS( targetname ), ent->target );

	if ( !sniper ) {
		Com_Printf( "sniper not found: %s\n" );
	} else
	{
		if ( visible( sniper, other ) ) {
			if ( sniper->wait < level.time ) {
				if ( sniper->count == 0 ) {
					sniper->count = 1;
					sniper->wait = level.time + sniper->delay;
					// record enemypos pos
					VectorCopy( ent->enemy->shared.r.currentOrigin, ent->pos1 );
				} else if ( sniper->count == 1 )     {
					VectorSubtract( ent->enemy->shared.r.currentOrigin, ent->pos1, vec );
					dist = VectorLength( vec );
					if ( dist < sniper->radius ) {
						// ok the enemy is still inside the radius take a shot
						sniper->enemy = other;
						sniper->use( sniper, other, other );
						G_UseTargets( ent, other );

						// added sniper shot

						G_AddEvent( player, EV_GENERAL_SOUND, sniper_sound );

					}

					// reset the sniper delay
					sniper->count = 0;
					sniper->wait = level.time + sniper->delay;
				}
			}
		} else
		{
			//sniper->wait = level.time + sniper->delay;
			sniper->count = 0;
		}
	}

}

void sniper_brush_init( GameEntity *ent ) {
	vec3_t center;

	if ( !ent->target ) {
		VectorSubtract( ent->shared.r.maxs, ent->shared.r.mins, center );
		VectorScale( center, 0.5, center );

		Com_Printf( "sniper_brush at %s without a target\n", vtos( center ) );
	}
}

extern void InitTrigger( GameEntity *self );

/*QUAKED sniper_brush (1 0 0) ?
this should be a volume that will encompase the area where the sniper target assigned to the
brush would fire at the player
*/
void SP_sniper_brush( GameEntity *ent ) {
	ent->nextthink = level.time + FRAMETIME;
	ent->think = sniper_brush_init;
	ent->touch = brush_activate_sniper;

	sniper_sound = G_SoundIndex( "sound/weapons/machinegun/machgf1b.wav" );

	InitTrigger( ent );
	SV_LinkEntity( &ent->shared );
}
