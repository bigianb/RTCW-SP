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

#include "g_local.h"
#include "../server/server.h"

#define MISSILE_PRESTEP_TIME    50


extern void gas_think( GameEntity *gas );
extern void gas_touch( GameEntity *gas, GameEntity *other, trace_t *trace );
extern void SP_target_smoke( GameEntity *ent );



void G_ExplodeMissilePoisonGas( GameEntity *ent );
void M_think( GameEntity *ent );
/*
================
G_BounceMissile

================
*/
bool G_BounceMissile( GameEntity *ent, trace_t *trace ) {
	vec3_t velocity;
	float dot;
	int hitTime;
	int contents;       //----(SA)	added
/*
		// Ridah, if we are a grenade, and we have hit an AI that is waiting to catch us, give them a grenade, and delete ourselves
	if ((ent->splashMethodOfDeath == MOD_GRENADE_SPLASH) && (g_entities[trace->entityNum].flags & FL_AI_GRENADE_KICK) &&
		(trace->endpos[2] > g_entities[trace->entityNum].r.currentOrigin[2])) {
		g_entities[trace->entityNum].grenadeExplodeTime = ent->nextthink;
		g_entities[trace->entityNum].flags &= ~FL_AI_GRENADE_KICK;
		Add_Ammo( &g_entities[trace->entityNum], WP_GRENADE_LAUNCHER, 1, false );	//----(SA)	modified
		G_FreeEntity( ent );
		return false;
	}
*/
	contents = SV_PointContents( ent->shared.s.origin, -1 );

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->shared.s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, ent->shared.s.pos.trDelta );

	// RF, record this for mover pushing
	if ( trace->plane.normal[2] > 0.2 /*&& VectorLength( ent->shared.s.pos.trDelta ) < 40*/ ) {
		ent->shared.s.groundEntityNum = trace->entityNum;
	}

	if ( ent->shared.s.eFlags & EF_BOUNCE_HALF ) {
		if ( contents & MASK_WATER ) {
			// barely bounce underwater
			VectorScale( ent->shared.s.pos.trDelta, 0.04f, ent->shared.s.pos.trDelta );
		} else {
			if ( ent->shared.s.eFlags & EF_BOUNCE ) {     // both flags marked, do a third type of bounce
				VectorScale( ent->shared.s.pos.trDelta, 0.25, ent->shared.s.pos.trDelta );
			} else {
				VectorScale( ent->shared.s.pos.trDelta, 0.65, ent->shared.s.pos.trDelta );
			}
		}

		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->shared.s.pos.trDelta ) < 40 ) {
//----(SA)	make the world the owner of the dynamite, so the player can shoot it after it stops moving
			if ( ent->shared.s.weapon == WP_DYNAMITE ) {
				ent->shared.r.ownerNum = ENTITYNUM_WORLD;

				// make shootable

				ent->health             = 5;
				ent->takedamage         = true;

				// small target cube
				VectorSet( ent->shared.r.mins, -4, -4, 0 );
				VectorCopy( ent->shared.r.mins, ent->shared.r.absmin );
				VectorSet( ent->shared.r.maxs, 4, 4, 8 );
				VectorCopy( ent->shared.r.maxs, ent->shared.r.absmax );

			}
//----(SA)	end
			G_SetOrigin( ent, trace->endpos );
			return false;
		}
	}

	VectorAdd( ent->shared.r.currentOrigin, trace->plane.normal, ent->shared.r.currentOrigin );
	VectorCopy( ent->shared.r.currentOrigin, ent->shared.s.pos.trBase );
	ent->shared.s.pos.trTime = level.time;

	if ( contents & MASK_WATER ) {
		return false;  // no bounce sound

	}
	return true;
}

/*
================
G_MissileImpact
	impactDamage is how much damage the impact will do to func_explosives
================
*/
void G_MissileImpact( GameEntity *ent, trace_t *trace, int impactDamage, vec3_t dir ) {  //----(SA)	added 'dir'
	GameEntity       *other;
	bool hitClient = false;
	vec3_t velocity;
	int etype;

	other = &g_entities[trace->entityNum];

	AICast_ProcessBullet( &g_entities[ent->shared.r.ownerNum], g_entities[ent->shared.r.ownerNum].shared.s.pos.trBase, trace->endpos );

	// handle func_explosives
	if ( other->classname && Q_stricmp( other->classname, "func_explosive" ) == 0 ) {
		// the damage is sufficient to "break" the ent (health == 0 is non-breakable)
		if ( other->health && impactDamage >= other->health ) {
			// check for other->takedamage needs to be inside the health check since it is
			// likely that, if successfully destroyed by the missile, in the next runmissile()
			// update takedamage would be set to '0' and the func_explosive would not be
			// removed yet, causing a bounce.
			if ( other->takedamage ) {
				BG_EvaluateTrajectoryDelta( &ent->shared.s.pos, level.time, velocity );
				G_Damage( other, ent, &g_entities[ent->shared.r.ownerNum], velocity, ent->shared.s.origin, impactDamage, 0, ent->methodOfDeath );
			}

			// its possible of the func_explosive not to die from this and it
			// should reflect the missile or explode it not vanish into oblivion
			if ( other->health <= 0 ) {
				if ( other->shared.s.frame != 1 ) { // playing death animation, still 'solid'
					return;
				}
			}
		}
	}

	// check for bounce
	if ( !other->takedamage &&
		 ( ent->shared.s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		if ( G_BounceMissile( ent, trace ) && !trace->startsolid ) {  // no bounce, no bounce sound
			int flags = 0;
			if ( !Q_stricmp( ent->classname, "flamebarrel" ) ) {
				G_AddEvent( ent, EV_FLAMEBARREL_BOUNCE, 0 );
			} else {
				flags = trace->surfaceFlags;
				flags = ( flags >> 12 );    // shift right 12 so I can send in parm	(#define SURF_METAL 0x1000.  metal is lowest flag i need for sound)
				G_AddEvent( ent, EV_GRENADE_BOUNCE, flags );    //----(SA)	send surfaceflags for sound selection
			}
		}
		return;
	}

	if ( other->takedamage && ent->shared.s.density == 1 ) {
		G_ExplodeMissilePoisonGas( ent );
		return;
	}

	// impact damage
	if ( other->takedamage ) { //&& Q_stricmp (ent->classname, "flamebarrel")) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {


			if ( LogAccuracyHit( other, &g_entities[ent->shared.r.ownerNum] ) ) {
				if ( g_entities[ent->shared.r.ownerNum].client ) {
					g_entities[ent->shared.r.ownerNum].client->ps.persistant[PERS_ACCURACY_HITS]++;
				}
				hitClient = true;
			}
			BG_EvaluateTrajectoryDelta( &ent->shared.s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;    // stepped on a grenade
			}
			G_Damage( other, ent, &g_entities[ent->shared.r.ownerNum], velocity,
					  ent->shared.s.origin, ent->damage,
					  0, ent->methodOfDeath );
		} else    // if no damage value, then this is a splash damage grenade only
		{
			G_BounceMissile( ent, trace );
			return;
		}
	}

//----(SA) removed as we have no hook

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->shared.s.otherEntityNum = other->shared.s.number;
	} else {
		// Ridah, try projecting it in the direction it came from, for better decals
//		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
//		vec3_t dir;
//		BG_EvaluateTrajectoryDelta( &ent->shared.s.pos, level.time, dir );
		BG_GetMarkDir( dir, trace->plane.normal, dir );
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );
	}

	ent->freeAfterEvent = true;

	// change over to a normal entity right at the point of impact
	etype = ent->shared.s.eType;
	ent->shared.s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->shared.s.pos.trBase );  // save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if ( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius,
							 other, ent->splashMethodOfDeath ) ) {
			if ( !hitClient && g_entities[ent->shared.r.ownerNum].client ) {
				g_entities[ent->shared.r.ownerNum].client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
		}
	}

	SV_LinkEntity( &ent->shared );


	// weapons with no default smoke business
	if ( etype == ET_MISSILE ) {
		switch ( ent->shared.s.weapon ) {
		case WP_MORTAR:
			return;
		default:
			break;
		}
	}


	if ( strcmp( ent->classname, "zombiespit" ) ) {
		GameEntity *Msmoke;

		Msmoke = G_Spawn();
		VectorCopy( ent->shared.r.currentOrigin, Msmoke->shared.s.origin );
		Msmoke->think = M_think;
		Msmoke->nextthink = level.time + FRAMETIME;
		Msmoke->health = 5;
	}
}

/*
==============
Concussive_think
==============
*/
void Concussive_think( GameEntity *ent ) {
	GameEntity *player;
	vec3_t dir;
	vec3_t kvel;
	float grav = 24;
	vec3_t vec;
	float len;

	if ( level.time > ent->delay ) {
		ent->think = G_FreeEntity;
	}

	ent->nextthink = level.time + FRAMETIME;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	VectorSubtract( player->shared.r.currentOrigin, ent->shared.s.origin, vec );
	len = VectorLength( vec );

//	Com_Printf ("len = %5.3f\n", len);

	if ( len > 512 ) {
		return;
	}

	VectorSet( dir, 0, 0, 1 );
	VectorScale( dir, grav, kvel );
	VectorAdd( player->client->ps.velocity, kvel, player->client->ps.velocity );

	if ( !player->client->ps.pm_time ) {
		int t;

		t = grav * 2;
		if ( t < 50 ) {
			t = 50;
		}
		if ( t > 200 ) {
			t = 200;
		}
		player->client->ps.pm_time = t;
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

}

/*
==============
Concussive_fx
	shake the player
	caused by explosives (grenades/dynamite/etc.)
==============
*/
//void Concussive_fx (GameEntity *ent)
void Concussive_fx( vec3_t origin ) {
//	GameEntity *tent;
//	GameEntity *player;

	GameEntity *concussive;

	// TODO: use new, good shake effect
//	return;



	concussive = G_Spawn();
//	VectorCopy (ent->shared.s.origin, concussive->shared.s.origin);
	VectorCopy( origin, concussive->shared.s.origin );
	concussive->think = Concussive_think;
	concussive->nextthink = level.time + FRAMETIME;
	concussive->delay = level.time + 500;

	return;

// Grenade and bomb flinching event
/*
	player = AICast_FindEntityForName( "player" );

	if (!player)
		return;

	if ( SV_inPVS (player->shared.r.currentOrigin, ent->shared.s.origin) )
	{
		tent = G_TempEntity (ent->shared.s.origin, EV_CONCUSSIVE);
		VectorCopy (ent->shared.s.origin, tent->shared.s.origin);
		tent->shared.s.density = player->shared.s.number;

		// Com_Printf ("sending concussive event\n");
	}
*/

}

/*
==============
M_think
==============
*/
void M_think( GameEntity *ent ) {
	GameEntity *tent;

	ent->count++;

//	if (ent->count == 1)
//		Concussive_fx (ent);	//----(SA)	moved to G_ExplodeMissile()

	if ( ent->count == ent->health ) {
		ent->think = G_FreeEntity;
	}

	tent = G_TempEntity( ent->shared.s.origin, EV_SMOKE );
	VectorCopy( ent->shared.s.origin, tent->shared.s.origin );
	if ( ent->shared.s.density == 1 ) {
		tent->shared.s.origin[2] += 16;
	} else {
		// tent->shared.s.origin[2]+=32;
		// Note to self Maxx said to lower the spawn loc for the smoke 16 units
		tent->shared.s.origin[2] += 16;
	}

	tent->shared.s.time = 3000;
	tent->shared.s.time2 = 100;
	tent->shared.s.density = 0;
	if ( ent->shared.s.density == 1 ) {
		tent->shared.s.angles2[0] = 16;
	} else {
		// Note to self Maxx changed this to 24
		tent->shared.s.angles2[0] = 24;
	}
	tent->shared.s.angles2[1] = 96;
	tent->shared.s.angles2[2] = 50;

	ent->nextthink = level.time + FRAMETIME;

}

/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( GameEntity *ent ) {
	vec3_t dir;
	vec3_t origin;
	bool small = false;
	bool zombiespit = false;
	int etype;

	BG_EvaluateTrajectory( &ent->shared.s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	etype = ent->shared.s.eType;

	ent->shared.s.eType = ET_GENERAL;

	if ( !Q_stricmp( ent->classname, "props_explosion" ) ) {
		G_AddEvent( ent, EV_MISSILE_MISS_SMALL, DirToByte( dir ) );
		small = true;
	}
// JPW NERVE
	else if ( !Q_stricmp( ent->classname, "air strike" ) ) {
		G_AddEvent( ent, EV_MISSILE_MISS_LARGE, DirToByte( dir ) );
		small = false;
	}
// jpw
	else if ( !Q_stricmp( ent->classname, "props_explosion_large" ) ) {
		G_AddEvent( ent, EV_MISSILE_MISS_LARGE, DirToByte( dir ) );
		small = false;
	} else if ( !Q_stricmp( ent->classname, "zombiespit" ) )      {
		G_AddEvent( ent, EV_SPIT_MISS, DirToByte( dir ) );
		zombiespit = true;
	} else if ( !Q_stricmp( ent->classname, "flamebarrel" ) )      {
		ent->freeAfterEvent = true;
		SV_LinkEntity( &ent->shared );
		return;
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );
	}

	ent->freeAfterEvent = true;

	// splash damage
	if ( ent->splashDamage ) {
		if ( G_RadiusDamage( ent->shared.r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath ) ) {    //----(SA)
			if ( g_entities[ent->shared.r.ownerNum].client ) {
				g_entities[ent->shared.r.ownerNum].client->ps.persistant[PERS_ACCURACY_HITS]++;
			}
		}
	}

	SV_LinkEntity( &ent->shared );

	if ( !zombiespit ) {
		GameEntity *Msmoke;

		Msmoke = G_Spawn();
		VectorCopy( ent->shared.r.currentOrigin, Msmoke->shared.s.origin );
		if ( small ) {
			Msmoke->shared.s.density = 1;
		}
		Msmoke->think = M_think;
		Msmoke->nextthink = level.time + FRAMETIME;

		if ( ent->parent && !Q_stricmp( ent->parent->classname, "props_flamebarrel" ) ) {
			Msmoke->health = 10;
		} else {
			Msmoke->health = 5;
		}

		Concussive_fx( Msmoke->shared.s.origin );
	}
}

/*
================
G_MissileDie
================
*/
void G_MissileDie( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	if ( inflictor == self ) {
		return;
	}
	self->takedamage    = false;
	self->think         = G_ExplodeMissile;
	self->nextthink     = level.time + 10;
}

/*
================
G_ExplodeMissilePoisonGas

Explode a missile without an impact
================
*/
void G_ExplodeMissilePoisonGas( GameEntity *ent ) {
	vec3_t dir;
	vec3_t origin;

	BG_EvaluateTrajectory( &ent->shared.s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->freeAfterEvent = true;


	{
		GameEntity *gas;

		gas = G_Spawn();
		gas->think = gas_think;
		gas->nextthink = level.time + FRAMETIME;
		gas->shared.r.contents = CONTENTS_TRIGGER;
		gas->touch = gas_touch;
		gas->health = 100;
		G_SetOrigin( gas, origin );

		SV_LinkEntity( &gas->shared );
	}

}

/*
================
G_RunMissile

================
*/
void G_RunMissile( GameEntity *ent ) {
	vec3_t origin, dir;         // 'dir' is 'deltaMove'
	trace_t tr;
	int impactDamage;

	AICast_CheckDangerousEntity( ent, DANGER_MISSILE, ent->splashRadius, 0.1, 1.0, true );

	// get current position
	BG_EvaluateTrajectory( &ent->shared.s.pos, level.time, origin );

//----(SA)	added direction to pass for mark determination
	VectorSubtract( origin, ent->shared.r.currentOrigin, dir );
	VectorNormalize( dir );
//----(SA)	end

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, origin,
				ent->shared.r.ownerNum, ent->clipmask, false );

	VectorCopy( tr.endpos, ent->shared.r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	SV_LinkEntity( &ent->shared );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if  (   tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if ( ent->parent && ent->parent->client && ent->parent->client->hook == ent ) {
				ent->parent->client->hook = nullptr;
			}
			G_FreeEntity( ent );
			return;
		}

		if ( ent->shared.s.weapon == WP_PANZERFAUST ) {
			impactDamage = 999; // goes through pretty much any func_explosives
		} else {
			impactDamage = 6;   // "grenade"/"dynamite"		// probably adjust this based on velocity //----(SA)	adjusted to not break through so much stuff.  try it to see if this is good enough

		}
		G_MissileImpact( ent, &tr, impactDamage, dir ); //----(SA)	added 'dir'

		if ( ent->shared.s.eType != ET_MISSILE ) {
			return;     // exploded
		}
	} else if ( VectorLength( ent->shared.s.pos.trDelta ) ) {  // free fall/no intersection
		ent->shared.s.groundEntityNum = ENTITYNUM_NONE;
	}

	// check think function after bouncing
	G_RunThink( ent );
}

/*
================
G_PredictBounceMissile

================
*/
void G_PredictBounceMissile( GameEntity *ent, trajectory_t *pos, trace_t *trace, int time ) {
	vec3_t velocity, origin;
	float dot;
	int hitTime;

	BG_EvaluateTrajectory( pos, time, origin );

	// reflect the velocity on the trace plane
	hitTime = time;
	BG_EvaluateTrajectoryDelta( pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, pos->trDelta );

	if ( ent->shared.s.eFlags & EF_BOUNCE_HALF ) {
		if ( ent->shared.s.eFlags & EF_BOUNCE ) {     // both flags marked, do a third type of bounce
			VectorScale( pos->trDelta, 0.25, pos->trDelta );
		} else {
			VectorScale( pos->trDelta, 0.65, pos->trDelta );
		}

		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( pos->trDelta ) < 40 ) {
			VectorCopy( trace->endpos, pos->trBase );
			return;
		}
	}

	VectorAdd( origin, trace->plane.normal, pos->trBase );
	pos->trTime = time;
}

/*
================
G_PredictMissile

  selfNum is the character that is checking to see what the missile is going to do

  returns false if the missile won't explode, otherwise it'll return the time is it expected to explode
================
*/
int G_PredictMissile( GameEntity *ent, int duration, vec3_t endPos, bool allowBounce ) {
	vec3_t origin;
	trace_t tr;
	int time;
	trajectory_t pos;
	vec3_t org;
	GameEntity backupEnt;

	pos = ent->shared.s.pos;
	BG_EvaluateTrajectory( &pos, level.time, org );

	backupEnt = *ent;

	for ( time = level.time + FRAMETIME; time < level.time + duration; time += FRAMETIME ) {

		// get current position
		BG_EvaluateTrajectory( &pos, time, origin );

		// trace a line from the previous position to the current position,
		// ignoring interactions with the missile owner
		SV_Trace( &tr, org, ent->shared.r.mins, ent->shared.r.maxs, origin,
					ent->shared.r.ownerNum, ent->clipmask, false );

		VectorCopy( tr.endpos, org );

		if ( tr.startsolid ) {
			*ent = backupEnt;
			return false;
		}

		if ( tr.fraction != 1 ) {
			// never explode or bounce on sky
			if  ( tr.surfaceFlags & SURF_NOIMPACT ) {
				*ent = backupEnt;
				return false;
			}

			if ( allowBounce && ( ent->shared.s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
				G_PredictBounceMissile( ent, &pos, &tr, time - FRAMETIME + (int)( (float)FRAMETIME * tr.fraction ) );
				pos.trTime = time;
				continue;
			}

			// exploded, so drop out of loop
			break;
		}
	}
/*
	if (!allowBounce && tr.fraction < 1 && tr.entityNum > level.maxclients) {
		// go back a bit in time, so we can catch it in the air
		time -= 200;
		if (time < level.time + FRAMETIME)
			time = level.time + FRAMETIME;
		BG_EvaluateTrajectory( &pos, time, org );
	}
*/

	// get current position
	VectorCopy( org, endPos );
	// set the entity data back
	*ent = backupEnt;
	//
	if ( allowBounce && ( ent->shared.s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		return ent->nextthink;
	} else {    // it will probably explode before it times out
		return time;
	}
}

// Rafael zombiespit
/*
================
G_RunSpit
================
*/


void G_RunSpit( GameEntity *ent ) {
	vec3_t origin;
	trace_t tr;
	vec3_t end;
	GameEntity   *smoke;

	// effect when it drips to floor
	if ( rand() % 100 > 60 ) {
		VectorCopy( ent->shared.r.currentOrigin, end );
		end[0] += crandom() * 8;
		end[1] += crandom() * 8;
		end[2] -= 8192;

		SV_Trace( &tr, ent->shared.r.currentOrigin, nullptr, nullptr, end,
					ent->shared.r.ownerNum, MASK_SHOT, false );

		smoke = G_Spawn();
		VectorCopy( tr.endpos, smoke->shared.s.origin );
		smoke->start_size = 4;
		smoke->end_size = 8;
		smoke->spawnflags |= 4;
		smoke->speed = 500; // 5 seconds
		smoke->duration = 100;
		smoke->health = 10 + ( crandom() * 5 );
		SP_target_smoke( smoke );
		smoke->shared.s.density = 5;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->shared.s.pos, level.time, origin );

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, origin,
				ent->shared.r.ownerNum, ent->clipmask, false );

	VectorCopy( tr.endpos, ent->shared.r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	SV_LinkEntity( &ent->shared );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if  (   tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if ( ent->parent && ent->parent->client->hook == ent ) {
				ent->parent->client->hook = nullptr;
			}
			G_FreeEntity( ent );
			return;
		}

		// G_MissileImpact( ent, &tr );
		{
			GameEntity *gas;

			gas = G_Spawn();
			gas->think = gas_think;
			gas->nextthink = level.time + FRAMETIME;
			gas->shared.r.contents = CONTENTS_TRIGGER;
			gas->touch = gas_touch;
			gas->health = 10;
			G_SetOrigin( gas, origin );
			gas->shared.s.density = 5;
			SV_LinkEntity( &gas->shared );

			ent->freeAfterEvent = true;

			// change over to a normal entity right at the point of impact
			ent->shared.s.eType = ET_GENERAL;

		}

		if ( ent->shared.s.eType != ET_MISSILE ) {
			return;     // exploded
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}


void G_RunCrowbar( GameEntity *ent ) {
	vec3_t origin;
	trace_t tr;

	// get current position
	BG_EvaluateTrajectory( &ent->shared.s.pos, level.time, origin );

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, origin,
				ent->shared.r.ownerNum, ent->clipmask, false );

	VectorCopy( tr.endpos, ent->shared.r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	SV_LinkEntity( &ent->shared );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if  (   tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if ( ent->parent && ent->parent->client->hook == ent ) {
				ent->parent->client->hook = nullptr;
			}
			G_FreeEntity( ent );
			return;
		}

		if ( ent->shared.s.eType != ET_MISSILE ) {
			return;     // exploded
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}

//=============================================================================

//----(SA) removed unused quake3 weapons.

int G_GetWeaponDamage( int weapon ); // JPW NERVE

/*
=================
fire_grenade

	NOTE!!!! NOTE!!!!!

	This accepts a /non-normalized/ direction vector to allow specification
	of how hard it's thrown.  Please scale the vector before calling.

=================
*/
GameEntity *fire_grenade( GameEntity *self, vec3_t start, vec3_t dir, int grenadeWPID ) {
	GameEntity   *bolt, *hit; // JPW NERVE
	bool noExplode = false;
	vec3_t mins, maxs;      // JPW NERVE
	static vec3_t range = { 40, 40, 52 };   // JPW NERVE
	int i,num,touch[MAX_GENTITIES];         // JPW NERVE

	bolt = G_Spawn();

	// no self->client for shooter_grenade's
	if ( self->client && self->client->ps.grenadeTimeLeft ) {
		if ( grenadeWPID == WP_DYNAMITE ) {   // remove any fraction of a 5 second 'click'
			self->client->ps.grenadeTimeLeft *= 5;
			self->client->ps.grenadeTimeLeft -= ( self->client->ps.grenadeTimeLeft % 5000 );
			self->client->ps.grenadeTimeLeft += 5000;
//			if(self->client->ps.grenadeTimeLeft < 5000)	// allow dropping of dynamite that won't explode (for shooting)
//				noExplode = true;
		}

		if ( !noExplode ) {
			bolt->nextthink = level.time + self->client->ps.grenadeTimeLeft;
		}
	} else {
		// let non-players throw the default duration
		if ( grenadeWPID == WP_DYNAMITE ) {
			if ( !noExplode ) {
				bolt->nextthink = level.time + 5000;
			}
		} else {
			bolt->nextthink = level.time + 2500;
		}
	}

	// no self->client for shooter_grenade's
	if ( self->client ) {
		self->client->ps.grenadeTimeLeft = 0;       // reset grenade timer

	}
	if ( !noExplode ) {
		bolt->think         = G_ExplodeMissile;
	}

	bolt->shared.s.eType       = ET_MISSILE;
	bolt->shared.r.svFlags     = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
	bolt->shared.s.weapon      = grenadeWPID;
	bolt->shared.r.ownerNum    = self->shared.s.number;
	bolt->parent        = self;

// JPW NERVE -- commented out bolt->damage and bolt->splashdamage, override with G_GetWeaponDamage()
// so it works with different netgame balance.  didn't uncomment bolt->damage on dynamite 'cause its so *special*
	bolt->damage = G_GetWeaponDamage( grenadeWPID ); // overridden for dynamite
	bolt->splashDamage = G_GetWeaponDamage( grenadeWPID );

	if ( self->client && !self->aiCharacter ) {
		bolt->damage *= 2;
		bolt->splashDamage *= 2;
	}
// jpw

	switch ( grenadeWPID ) {
	case WP_GRENADE_LAUNCHER:
		bolt->classname             = "grenade";
//			bolt->damage				= 100;
//			bolt->splashDamage			= 100;
		if ( self->aiCharacter ) {
			bolt->splashRadius          = 150;
		} else {
			bolt->splashRadius          = 150;
		}
		bolt->methodOfDeath         = MOD_GRENADE;
		bolt->splashMethodOfDeath   = MOD_GRENADE_SPLASH;
		bolt->shared.s.eFlags              = EF_BOUNCE_HALF;
		break;
	case WP_GRENADE_PINEAPPLE:
		bolt->classname             = "grenade";
//			bolt->damage				= 80;
//			bolt->splashDamage			= 80;
		bolt->splashRadius          = 300;
		bolt->methodOfDeath         = MOD_GRENADE;
		bolt->splashMethodOfDeath   = MOD_GRENADE_SPLASH;
		bolt->shared.s.eFlags              = EF_BOUNCE_HALF;
		break;
// JPW NERVE
	case WP_GRENADE_SMOKE:
		bolt->classname             = "grenade";
		bolt->shared.s.eFlags              = EF_BOUNCE_HALF;
		break;
// jpw
	case WP_DYNAMITE:
		// oh, this is /so/ cheap...
		// you need to pick up new code ;)
		SV_GameSendServerCommand( self - g_entities, va( "dp %d", ( bolt->nextthink - level.time ) / 1000 ) );

		bolt->classname             = "dynamite";
		bolt->damage                = 0;
//			bolt->splashDamage			= 300;
		bolt->splashRadius          = 400;
		bolt->methodOfDeath         = MOD_DYNAMITE;
		bolt->splashMethodOfDeath   = MOD_DYNAMITE_SPLASH;
		bolt->shared.s.eFlags              = ( EF_BOUNCE | EF_BOUNCE_HALF );   // EF_BOUNCE_HEAVY;

		// dynamite is shootable
		bolt->die                   = G_MissileDie;
		bolt->shared.r.contents            = CONTENTS_CORPSE;      // (player can walk through)

		break;
	}

	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.pos.trType = TR_GRAVITY;
	bolt->shared.s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );
	VectorCopy( dir, bolt->shared.s.pos.trDelta );
	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth

	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}

//=============================================================================



/*
=================
fire_rocket
=================
*/
GameEntity *fire_rocket( GameEntity *self, vec3_t start, vec3_t dir ) {
	GameEntity   *bolt;

	VectorNormalize( dir );

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 20000;   // push it out a little
	bolt->think = G_ExplodeMissile;
	bolt->shared.s.eType = ET_MISSILE;
	bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;

	bolt->shared.s.weapon = WP_PANZERFAUST;

	bolt->shared.r.ownerNum = self->shared.s.number;
	bolt->parent = self;

	if ( self->aiCharacter ) { // ai keep the values they've been using
		bolt->damage = 100;
		bolt->splashDamage = 120;
	} else {
		bolt->damage = G_GetWeaponDamage( WP_PANZERFAUST );
		bolt->splashDamage = G_GetWeaponDamage( WP_PANZERFAUST );
	}

	if ( self->aiCharacter ) {
		bolt->splashRadius = 120;
	} else {
		bolt->splashRadius = G_GetWeaponDamage( WP_PANZERFAUST );
	}

	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.pos.trType = TR_LINEAR;
	bolt->shared.s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );

	// ai gets dynamics they're used to
	if ( self->aiCharacter ) {
		VectorScale( dir, 1000, bolt->shared.s.pos.trDelta );
	} else {
		// (SA) trying a bit more speed in SP for player rockets
		VectorScale( dir, 1300, bolt->shared.s.pos.trDelta );
	}

	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth
	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}

// Rafael zombie spit
/*
=====================
fire_zombiespit
=====================
*/
GameEntity *fire_zombiespit( GameEntity *self, vec3_t start, vec3_t dir ) {
	GameEntity   *bolt;

	VectorNormalize( dir );

	bolt = G_Spawn();
	bolt->classname = "zombiespit";
	bolt->nextthink = level.time + 10000;

	bolt->think = G_ExplodeMissile;

	bolt->shared.s.eType = ET_ZOMBIESPIT;
	bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;

	bolt->shared.s.weapon = WP_PANZERFAUST;

	bolt->shared.r.ownerNum = self->shared.s.number;
	bolt->parent = self;

	bolt->damage = 10;
	bolt->splashDamage = 10;
	bolt->splashRadius = 120;

	bolt->methodOfDeath = MOD_ZOMBIESPIT;
	bolt->splashMethodOfDeath = MOD_ZOMBIESPIT_SPLASH;

	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.loopSound = G_SoundIndex( "sound/Loogie/sizzle.wav" );

	// bolt->shared.s.pos.trType = TR_LINEAR;
	bolt->shared.s.pos.trType = TR_GRAVITY_LOW;
	bolt->shared.s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );
	VectorScale( dir, 600, bolt->shared.s.pos.trDelta );
	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth
	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}

/*
=====================
fire_zombiespirit
=====================
*/
GameEntity *fire_zombiespirit( GameEntity *self, GameEntity *bolt, vec3_t start, vec3_t dir ) {

	VectorNormalize( dir );

	bolt->classname = "zombiespirit";
	bolt->nextthink = level.time + 10000;

	bolt->think = G_ExplodeMissile;

	bolt->shared.s.eType = ET_ZOMBIESPIRIT;
	bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;

	bolt->shared.s.weapon = WP_PANZERFAUST;

	bolt->shared.r.ownerNum = self->shared.s.number;
	bolt->parent = self;
	bolt->damage = 10;
	bolt->splashDamage = 10;
	bolt->splashRadius = 120;

	bolt->methodOfDeath = MOD_ZOMBIESPIRIT;
	bolt->splashMethodOfDeath = MOD_ZOMBIESPIRIT_SPLASH;

	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.loopSound = G_SoundIndex( "sound/Zombie/attack/spirit_loop.wav" );

	bolt->shared.s.pos.trType = TR_INTERPOLATE;        // we'll move it manually, since it needs to track it's enemy
	bolt->shared.s.pos.trTime = level.time;            // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );
	VectorScale( dir, 800, bolt->shared.s.pos.trDelta );
	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth
	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}

// the crowbar for the mechanic
GameEntity *fire_crowbar( GameEntity *self, vec3_t start, vec3_t dir ) {
	GameEntity   *bolt;

	VectorNormalize( dir );

	bolt = G_Spawn();
	bolt->classname = "crowbar";
	bolt->nextthink = level.time + 50000;
	bolt->think = G_ExplodeMissile;
	bolt->shared.s.eType = ET_CROWBAR;


	bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;
	// bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;

	bolt->shared.s.weapon = WP_PANZERFAUST;
	bolt->shared.r.ownerNum = self->shared.s.number;
	bolt->parent = self;
	bolt->damage = 10;
	bolt->splashDamage = 0;
	bolt->splashRadius = 0;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
//	bolt->clipmask = MASK_SHOT;
	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.pos.trType = TR_GRAVITY;
	bolt->shared.s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );
	VectorScale( dir, 800, bolt->shared.s.pos.trDelta );
	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth
	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}

// Rafael flamebarrel
/*
======================
fire_flamebarrel
======================
*/

GameEntity *fire_flamebarrel( GameEntity *self, vec3_t start, vec3_t dir ) {
	GameEntity   *bolt;

	VectorNormalize( dir );

	bolt = G_Spawn();
	bolt->classname = "flamebarrel";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->shared.s.eType = ET_FLAMEBARREL;
	bolt->shared.s.eFlags = EF_BOUNCE_HALF;
	bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->shared.s.weapon = WP_PANZERFAUST;
	bolt->shared.r.ownerNum = self->shared.s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 20;
	bolt->splashRadius = 60;

	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;

	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.pos.trType = TR_GRAVITY;
	bolt->shared.s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );
	VectorScale( dir, 900 + ( crandom() * 100 ), bolt->shared.s.pos.trDelta );
	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth
	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}


// Rafael sniper
/*
=================
fire_lead
=================
*/

void fire_lead( GameEntity *self, vec3_t start, vec3_t dir, int damage ) {

	trace_t tr;
	vec3_t end;
	GameEntity       *tent;
	GameEntity       *traceEnt;
	vec3_t forward, right, up;
	vec3_t angles;
	float r, u;
	bool anti_tank_enable = false;

	r = crandom() * self->random;
	u = crandom() * self->random;

	vectoangles( dir, angles );
	AngleVectors( angles, forward, right, up );

	VectorMA( start, 8192, forward, end );
	VectorMA( end, r, right, end );
	VectorMA( end, u, up, end );

	SV_Trace( &tr, start, nullptr, nullptr, end, self->shared.s.number, MASK_SHOT, false);
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, start );

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->shared.s.eventParm = traceEnt->shared.s.number;
	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );

		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->shared.s.eventParm = DirToByte( reflect );
		// done.
	}
	tent->shared.s.otherEntityNum = self->shared.s.number;

	if ( traceEnt->takedamage ) {

		if ( self->shared.s.weapon == WP_SNIPER
			 && traceEnt->shared.s.eType == ET_MOVER
			 && traceEnt->aiName[0] ) {
			anti_tank_enable = true;
		}

		if ( anti_tank_enable ) {
			self->shared.s.weapon = WP_PANZERFAUST;
		}

		G_Damage( traceEnt, self, self, forward, tr.endpos,
				  damage, 0, MOD_MACHINEGUN );

		if ( anti_tank_enable ) {
			self->shared.s.weapon = WP_SNIPER;
		}

	}

}


// Rafael sniper
// visible

/*
==============
visible
==============
*/
bool visible( GameEntity *self, GameEntity *other ) {
//	vec3_t		spot1;
//	vec3_t		spot2;
	trace_t tr;
	GameEntity   *traceEnt;

	SV_Trace( &tr, self->shared.r.currentOrigin, nullptr, nullptr, other->shared.r.currentOrigin, self->shared.s.number, MASK_SHOT, false );

	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt == other ) {
		return true;
	}

	return false;

}



/*
==============
fire_mortar
	dir is a non-normalized direction/power vector
==============
*/
GameEntity *fire_mortar( GameEntity *self, vec3_t start, vec3_t dir ) {
	GameEntity   *bolt;

//	VectorNormalize (dir);

	if ( self->spawnflags ) {
		GameEntity   *tent;
		tent = G_TempEntity( self->shared.s.pos.trBase, EV_MORTAREFX );
		tent->shared.s.density = self->spawnflags; // send smoke and muzzle flash flags
		VectorCopy( self->shared.s.pos.trBase, tent->shared.s.origin );
		VectorCopy( self->shared.s.apos.trBase, tent->shared.s.angles );
	}

	bolt = G_Spawn();
	bolt->classname = "mortar";
	bolt->nextthink = level.time + 20000;   // push it out a little
	bolt->think = G_ExplodeMissile;
	bolt->shared.s.eType = ET_MISSILE;

	bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;   // broadcast sound.  not multiplayer friendly, but for mortars it should be okay
	// bolt->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;

	bolt->shared.s.weapon = WP_MORTAR;
	bolt->shared.r.ownerNum = self->shared.s.number;
	bolt->parent = self;
	bolt->damage = G_GetWeaponDamage( WP_MORTAR ); // JPW NERVE
	bolt->splashDamage = G_GetWeaponDamage( WP_MORTAR ); // JPW NERVE
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_MORTAR;
	bolt->splashMethodOfDeath = MOD_MORTAR_SPLASH;
	bolt->clipmask = MASK_MISSILESHOT;

	bolt->shared.s.pos.trType = TR_GRAVITY;
	bolt->shared.s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;     // move a bit on the very first frame
	VectorCopy( start, bolt->shared.s.pos.trBase );
//	VectorScale( dir, 900, bolt->shared.s.pos.trDelta );
	VectorCopy( dir, bolt->shared.s.pos.trDelta );
	SnapVector( bolt->shared.s.pos.trDelta );          // save net bandwidth
	VectorCopy( start, bolt->shared.r.currentOrigin );

	return bolt;
}


