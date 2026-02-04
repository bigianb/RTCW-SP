#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

void MatchTeam( GameEntity *teamLeader, int moverState, int time );

/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/


void Blocked_Door( GameEntity *ent, GameEntity *other ) {
	GameEntity *slave;
	int time;

	// remove anything other than a client
	if ( other ) {
		if ( !other->client ) {

			if ( other->shared.s.eType == ET_MOVER ) {
				if ( strstr( other->classname, "chair" ) ) {
					// break crushed chairs
					G_Damage( other, ent, ent, nullptr, nullptr, 99999, 0, MOD_CRUSH );   // Die!
					return;
				}
			}
			G_FreeEntity( other );
			return;
		}

		if ( ent->damage ) {
			G_Damage( other, ent, ent, nullptr, nullptr, ent->damage, 0, MOD_CRUSH );
		}
	}

	if ( ent->spawnflags & 4 ) {
		return;     // crushers don't reverse
	}

	// reverse direction

	for ( slave = ent ; slave ; slave = slave->teamchain )
	{
		time = level.time - ( slave->shared.s.pos.trDuration - ( level.time - slave->shared.s.pos.trTime ) );

		if ( slave->moverState == MOVER_1TO2 ) {
			SetMoverState( slave, MOVER_2TO1, time );
		} else {
			SetMoverState( slave, MOVER_1TO2, time );
		}
		SV_LinkEntity( &slave->shared );
	}

}

static void Touch_DoorTriggerSpectator( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	int i, axis;
	vec3_t origin, dir, angles;

	axis = ent->count;
	VectorClear( dir );
	if ( fabs( other->shared.s.origin[axis] - ent->shared.r.absmax[axis] ) <
		 fabs( other->shared.s.origin[axis] - ent->shared.r.absmin[axis] ) ) {
		origin[axis] = ent->shared.r.absmin[axis] - 10;
		dir[axis] = -1;
	} else {
		origin[axis] = ent->shared.r.absmax[axis] + 10;
		dir[axis] = 1;
	}
	for ( i = 0; i < 3; i++ ) {
		if ( i == axis ) {
			continue;
		}
		origin[i] = ( ent->shared.r.absmin[i] + ent->shared.r.absmax[i] ) * 0.5;
	}
	vectoangles( dir, angles );
	TeleportPlayer( other, origin, angles );
}

#define DOORPUSHBACK    16

void Blocked_DoorRotate( GameEntity *ent, GameEntity *other ) {

	GameEntity       *slave;
	int time;

	// remove anything other than a client
	if ( other ) {
		if ( !other->client ) {
			G_TempEntity( other->shared.s.origin, EV_ITEM_POP );
			G_FreeEntity( other );
			return;
		}

		if ( other->health <= 0 ) {
			G_Damage( other, ent, ent, nullptr, nullptr, 99999, 0, MOD_CRUSH );
		}

		if ( ent->damage ) {
			G_Damage( other, ent, ent, nullptr, nullptr, ent->damage, 0, MOD_CRUSH );
		}
	}

	// RF, set this timer, so AI know not to try and open the door immediately
	ent->grenadeFired = level.time + 2000;

	for ( slave = ent ; slave ; slave = slave->teamchain )
	{
		// RF, set this timer, so AI know not to try and open the door immediately
		slave->grenadeFired = level.time + 2000;

		// RF, trying to fix "stuck in door" bug
		time = level.time - ( slave->shared.s.apos.trDuration - ( level.time - slave->shared.s.apos.trTime ) );
		//time = level.time - slave->shared.s.apos.trTime;

		if ( slave->moverState == MOVER_1TO2ROTATE ) {
			SetMoverState( slave, MOVER_2TO1ROTATE, time );
		} else
		{
			SetMoverState( slave, MOVER_1TO2ROTATE, time );
		}
		SV_LinkEntity( &slave->shared );
	}


}


void Touch_DoorTrigger( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	if ( ent->parent->moverState != MOVER_1TO2 )   {
		Use_BinaryMover( ent->parent, ent, other );
	}
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( GameEntity *ent ) {
	GameEntity       *other;
	vec3_t mins, maxs;
	int i, best;

	// set all of the slaves as shootable
	for ( other = ent ; other ; other = other->teamchain ) {
		other->takedamage = true;
	}

	// find the bounds of everything on the team
	VectorCopy( ent->shared.r.absmin, mins );
	VectorCopy( ent->shared.r.absmax, maxs );

	for ( other = ent->teamchain ; other ; other = other->teamchain ) {
		AddPointToBounds( other->shared.r.absmin, mins, maxs );
		AddPointToBounds( other->shared.r.absmax, mins, maxs );
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn();
	VectorCopy( mins, other->shared.r.mins );
	VectorCopy( maxs, other->shared.r.maxs );
	other->parent = ent;
	other->shared.r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	SV_LinkEntity( &other->shared );

	MatchTeam( ent, ent->moverState, level.time );
}

void Think_MatchTeam( GameEntity *ent ) {
	MatchTeam( ent, ent->moverState, level.time );
}

/*
==============
findNonAIBrushTargeter
	determine if there is an entity pointing at ent that is not a "trigger_aidoor"
	(used now for checking which key to set for a door)
==============
*/
bool findNonAIBrushTargeter( GameEntity *ent ) {
	GameEntity *targeter = nullptr;

	if ( !( ent->targetname ) ) {
		return false;
	}

	while ( ( targeter = G_Find( targeter, FOFS( target ), ent->targetname ) ) != nullptr )
	{
		if ( strcmp( targeter->classname,"trigger_aidoor" ) &&
			 Q_stricmp( targeter->classname, "func_invisible_user" ) ) {
			return true;
		}
	}

	return false;
}



void finishSpawningKeyedMover( GameEntity *ent ) {
	GameEntity       *slave;

	// all ents should be spawned, so it's okay to check for special door triggers now

	if ( ent->key == KEY_UNLOCKED_ENT ) {  // the key was not set in the spawn
		if ( ent->targetname && findNonAIBrushTargeter( ent ) ) {
			ent->key = KEY_LOCKED_TARGET;   // something is targeting this (other than a trigger_aidoor) so leave locked
		} else {
			ent->key = KEY_NONE;
		}
	}


	if ( ent->key ) {
		G_SetAASBlockingEntity( ent, true );
	}

	ent->nextthink = level.time + FRAMETIME;

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		if ( ent->targetname || ent->takedamage ) {  // non touch/shoot doors
			ent->think = Think_MatchTeam;
		}
// (SA) is this safe?  is ent->spawnflags & 8 consistant among all keyed ents?
		else if ( ( ent->spawnflags & 8 ) && ( strcmp( ent->classname, "func_door_rotating" ) ) ) {
			ent->think = Think_SpawnNewDoorTrigger;
		} else {
			ent->think = Think_MatchTeam;
		}

		// (SA) slaves have been marked as FL_TEAMSLAVE now, so they won't
		// finish their think on their own.  So set keys for teamed doors
		for ( slave = ent ; slave ; slave = slave->teamchain )
		{
			if ( slave == ent ) {
				continue;
			}

			slave->key = ent->key;

			if ( slave->key ) {
				G_SetAASBlockingEntity( slave, true );
			}
		}
	}
}

//----(SA) end



/*
==============
Door_reverse_sounds
	The door has been marked as "START_OPEN" which means the open/closed
	positions have been swapped.
	This swaps the sounds around as well
==============
*/
void Door_reverse_sounds( GameEntity *ent ) {
	int stemp;

	stemp = ent->sound1to2;
	ent->sound1to2 = ent->sound2to1;
	ent->sound2to1 = stemp;

	stemp = ent->soundPos1;
	ent->soundPos1 = ent->soundPos2;
	ent->soundPos2 = stemp;

	stemp = ent->sound2to3;
	ent->sound2to3 = ent->sound3to2;
	ent->sound3to2 = stemp;


	stemp = ent->soundSoftopen;
	ent->soundSoftopen = ent->soundSoftclose;
	ent->soundSoftclose = stemp;

	stemp = ent->soundSoftendo;
	ent->soundSoftendo = ent->soundSoftendc;
	ent->soundSoftendc = stemp;

}


/*
==============
DoorSetSounds
	get sound indexes for the various door sounds
	(used by SP_func_door() and SP_func_door_rotating() )
==============
*/
void DoorSetSounds( GameEntity *ent, int doortype, bool isRotating ) {
	ent->sound1to2 = G_SoundIndex( va( "door%i_open", doortype ) );      // opening
	ent->soundPos2 = G_SoundIndex( va( "door%i_endo", doortype ) );      // open
	ent->sound2to1 = G_SoundIndex( va( "door%i_close", doortype ) ); // closing
	ent->soundPos1 = G_SoundIndex( va( "door%i_endc", doortype ) );      // closed
	ent->sound2to3 = G_SoundIndex( va( "door%i_loopo", doortype ) ); // loopopen
	ent->sound3to2 = G_SoundIndex( va( "door%i_loopc", doortype ) ); // loopclosed
	ent->soundPos3 = G_SoundIndex( va( "door%i_locked", doortype ) );    // locked


	ent->soundSoftopen  = G_SoundIndex( va( "door%i_openq", doortype ) );    // opening quietly
	ent->soundSoftendo  = G_SoundIndex( va( "door%i_endoq", doortype ) );    // open quietly
	ent->soundSoftclose = G_SoundIndex( va( "door%i_closeq", doortype ) );   // closing quietly
	ent->soundSoftendc  = G_SoundIndex( va( "door%i_endcq", doortype ) );    // closed quietly

	if ( isRotating ) {
		ent->soundKicked    = G_SoundIndex( va( "door%i_kicked", doortype ) );
		ent->soundKickedEnd = G_SoundIndex( va( "door%i_kickedend", doortype ) );
	}

}

/*
==============
G_TryDoor
	seemed better to have this isolated.  this way i can get func_invisible_user's using the
	regular rules of doors.
==============
*/
void G_TryDoor( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	int soundrange = 0;
	bool walking = false, locked = false;

	walking = (bool)( ent->flags & FL_SOFTACTIVATE );


	if ( ( ent->shared.s.apos.trType == TR_STATIONARY && ent->shared.s.pos.trType == TR_STATIONARY ) ) {
		if ( !ent->active) {
			if ( ent->key >= KEY_LOCKED_ENT ) {    // door force locked
				locked = true;
			} else if ( ent->key == KEY_LOCKED_TARGET ) {
				// door locked because it was targeted, check if the 'other' ent is targeting this door
				if ( Q_stricmp( other->target, ent->targetname ) ) {
					locked = true;
				}
			}

			if ( locked ) {
				if ( !walking && activator ) { // only send audible event if not trying to open slowly
					AICast_AudibleEvent( activator->shared.s.clientNum, ent->shared.s.origin, HEAR_RANGE_DOOR_LOCKED );   // "someone tried locked door near me!"
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
				return;
			}

			if ( activator ) {
				if ( ent->key > KEY_NONE && ent->key < KEY_NUM_KEYS ) { // door requires key
					gitem_t *item = BG_FindItemForKey( (wkey_t)ent->key, 0 );
					if ( !( activator->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
						if ( !walking ) {  // only send audible event if not trying to open slowly
							AICast_AudibleEvent( activator->shared.s.clientNum, ent->shared.s.origin, HEAR_RANGE_DOOR_LOCKED );   // "someone tried locked door near me!"
						}
						// player does not have key
						G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
						return;
					}
				}
			}


			if ( ent->teammaster && ent->team && ent != ent->teammaster ) {
				ent->teammaster->active = true;
				if ( walking ) {
					ent->teammaster->flags |= FL_SOFTACTIVATE;      // no noise generated
				} else {
					if ( activator ) {
						soundrange = HEAR_RANGE_DOOR_OPEN;
					}
				}

				Use_BinaryMover( ent->teammaster, activator, activator );
				G_UseTargets( ent->teammaster, activator );
			} else
			{
				ent->active = true;
				if ( walking ) {
					ent->flags |= FL_SOFTACTIVATE;      // no noise
				} else {
					if ( activator ) {
						soundrange = HEAR_RANGE_DOOR_OPEN;
					}
				}

				Use_BinaryMover( ent, activator, activator );
				G_UseTargets( ent, activator );
			}

			if ( ent->flags & FL_DOORNOISE ) { // this door always plays the 'regular' open sound event
				soundrange = HEAR_RANGE_DOOR_OPEN;
			}
		}
	}
}


/*QUAKED func_door (0 .5 .8) ? START_OPEN TOGGLE CRUSHER TOUCH SHOOT-THRU
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door
SHOOT-THRU	Bullets don't stop when they hit the door.  Set "shoot_thru_scale" with bullet damage scale (see below)

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"angle"	    determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"	    movement speed (100 default)
"closespeed" optional different movement speed for door closing
"wait"      wait before returning (3 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"health"    if set, the door must be shot open
"team"		team name.  other doors with same team name will open/close in syncronicity
"noisescale"multiplier for how far the noise from the door will travel to alert AI
"type"		use sounds based on construction of door:
	 0 - nosound (default)
	 1 - metal
	 2 - stone
	 3 - lab
	 4 - wood
	 5 - iron/jail
	 6 - portcullis
	 7 - wood (quiet)

SOUND NAMING INFO -
inside "sound/movers/doors/door<number>...
	_open.wav		// opening
	_endo.wav		// open
	_close.wav		// closing
	_endc.wav		// closed
	_loopo.wav		// opening loop
	_loopc.wav		// closing loop
	_locked.wav		// locked

	_openq.wav		// opening quietly
	_endoq.wav		// open quietly
	_closeq.wav		// closing quietly
	_endcq.wav		// closed quietly

and for rotating doors:
	_kicked.wav
	_kickedend.wav

*/
void SP_func_door( GameEntity *ent ) {
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;
	int key, doortype;

	G_SpawnInt( "type", "0", &doortype );

	if ( doortype ) { // /*why on earthy did this check for <=8?*/ && doortype <= 8)	// no doortype = silent
		DoorSetSounds( ent, doortype, false );
	}

	ent->blocked = Blocked_Door;

	// default speed of 400
	if ( !ent->speed ) {
		ent->speed = 400;
	}

	// default wait of 2 seconds
	if ( !ent->wait ) {
		ent->wait = 2;
	}
	ent->wait *= 1000;


	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		ent->key = key;

		if ( key == -1 ) {
			ent->key = KEY_LOCKED_ENT;
		} else if ( ent->key > KEY_NUM_KEYS || ent->key < KEY_NONE ) {            // if the key is invalid, set the key in the finishSpawning routine
			Com_Error( ERR_DROP, "invalid key (%d) set for func_door_rotating\n", ent->key );
			ent->key = KEY_UNLOCKED_ENT;
		}
	} else {
		ent->key = KEY_UNLOCKED_ENT;    // otherwise, set the key when this ent finishes spawning
	}


	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( ent->shared.s.origin, ent->pos1 );

	// calculate second position
	SV_SetBrushModel( &ent->shared, ent->model );
	G_SetMovedir( ent->shared.s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->shared.r.maxs, ent->shared.r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	if ( ent->spawnflags & 1 ) {    // START_OPEN - reverse position 1 and 2
		vec3_t temp;
		int tempi;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->shared.s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );

		// swap speeds if door has 'closespeed'
		if ( ent->closespeed ) {
			tempi = ent->speed;
			ent->speed = ent->closespeed;
			ent->closespeed = tempi;
		}

		// swap sounds
		Door_reverse_sounds( ent );
	}

	// TOGGLE
	if ( ent->spawnflags & 2 ) {
		ent->flags |= FL_TOGGLE;
	}

	InitMover( ent );

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = true;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;
}


/*QUAKED func_door_rotating (0 .5 .8) ? - TOGGLE X_AXIS Y_AXIS REVERSE FORCE STAYOPEN
You need to have an origin brush as part of this entity.
The center of that brush will be the point around which it is rotated. It will rotate around the Z axis by default.  You can check either the X_AXIS or Y_AXIS box to change that (only one axis allowed. If both X and Y are checked, the default of Z will be used).
FORCE		door opens even if blocked

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"degrees"   determines how many degrees it will turn (90 default)
"speed"	    movement speed (100 default)
"closespeed" optional different movement speed for door closing
"time"      how many milliseconds it will take to open 1 sec = 1000
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"type"		use sounds based on construction of door:
	 0 - nosound (default)
	 1 - metal
	 2 - stone
	 3 - lab
	 4 - wood
	 5 - iron/jail
	 6 - portcullis
	 7 - wood (quiet)
"team"		team name.  other doors with same team name will open/close in syncronicity
*/




//
//
void SP_func_door_rotating( GameEntity *ent ) {
	int key, doortype;

	G_SpawnInt( "type", "0", &doortype );

	if ( doortype ) {  // /*why on earthy did this check for <=8?*/ && doortype <= 8)	// no doortype = silent
		DoorSetSounds( ent, doortype, true );
		if ( doortype == 5 ) { // iron/jail always makes same noise
			ent->flags |= FL_DOORNOISE;
		}
	}


	// set the duration
	if ( !ent->speed ) {
		ent->speed = 1000;
	}

	// degrees door will open
	if ( !ent->angle ) {
		ent->angle = 90;
	}

	// reverse direction
	if ( ent->spawnflags & 16 ) {
		ent->angle *= -1;
	}

	// TOGGLE
	if ( ent->spawnflags & 2 ) {
		ent->flags |= FL_TOGGLE;
	}

	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		ent->key = key;

		if ( key == -1 ) {
			ent->key = KEY_LOCKED_ENT;
		} else if ( ent->key > KEY_NUM_KEYS || ent->key < KEY_NONE ) {            // if the key is invalid, set the key in the finishSpawning routine
			Com_Error( ERR_DROP, "invalid key (%d) set for func_door_rotating\n", ent->key );
			ent->key = KEY_UNLOCKED_ENT;
		}
	} else {
		ent->key = KEY_UNLOCKED_ENT;    // otherwise, set the key when this ent finishes spawning
	}


	// set the rotation axis
	VectorClear( ent->rotate );
	if      ( ent->spawnflags & 4 ) {
		ent->rotate[2] = 1;
	} else if ( ent->spawnflags & 8 ) {
		ent->rotate[0] = 1;
	} else { ent->rotate[1] = 1;}

	if ( VectorLength( ent->rotate ) > 1 ) { // check that rotation is only set for one axis
		Com_Error( ERR_DROP, "Too many axis marked in func_door_rotating entity.  Only choose one axis of rotation. (defaulting to standard door rotation)" );
		VectorClear( ent->rotate );
		ent->rotate[1] = 1;
	}

	if ( !ent->wait ) {
		ent->wait = 2;
	}
	ent->wait *= 1000;

	//if (!ent->damage) {
	//	ent->damage = 2;
	//}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitMoverRotate( ent );

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = true;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;

	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.pos.trBase, ent->shared.r.currentOrigin );
	VectorCopy( ent->shared.s.apos.trBase, ent->shared.r.currentAngles );

	ent->blocked = Blocked_DoorRotate;

	SV_LinkEntity( &ent->shared );
}

/*QUAKED func_secret (0 .5 .8) ? REVERSE x CRUSHER TOUCH
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"angle"	    determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"	    movement speed (100 default)
"wait"      wait before returning (2 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"health"    if set, the door must be shot open
*/
void SP_func_secret( GameEntity *ent ) {
	vec3_t abs_movedir;
	vec3_t angles2;
	float distance;
	vec3_t size;
	float lip;
	int key;

	ent->sound1to2 = ent->sound2to1 = ent->sound2to3 = G_SoundIndex( "sound/movers/doors/dr1_strt.wav" );
	ent->soundPos1 = ent->soundPos3 = G_SoundIndex( "sound/movers/doors/dr1_end.wav" );

	ent->blocked = Blocked_Door;

	// default speed of 100
	if ( !ent->speed ) {
		ent->speed = 100;
	}

	// default wait of 2 seconds
	if ( !ent->wait ) {
		ent->wait = 2;
	}
	ent->wait *= 1000;


	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		ent->key = key;

		if ( key == -1 ) {
			ent->key = KEY_LOCKED_ENT;
		} else if ( ent->key > KEY_NUM_KEYS || ent->key < KEY_NONE ) {            // if the key is invalid, set the key in the finishSpawning routine
			Com_Error( ERR_DROP, "invalid key (%d) set for func_door_rotating\n", ent->key );
			ent->key = KEY_UNLOCKED_ENT;
		}
	} else {
		ent->key = KEY_UNLOCKED_ENT;    // otherwise, set the key when this ent finishes spawning
	}

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( ent->shared.s.origin, ent->pos1 );

	VectorCopy( ent->shared.s.angles, angles2 );

	if ( ent->spawnflags & 1 ) {
		angles2[1] -= 90;
	} else {
		angles2[1] += 90;
	}

	// calculate second position
	SV_SetBrushModel( &ent->shared, ent->model );
	G_SetMovedir( ent->shared.s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->shared.r.maxs, ent->shared.r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	// calculate third position
	G_SetMovedir( angles2, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->shared.r.maxs, ent->shared.r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos2, distance, ent->movedir, ent->pos3 );

	// if "start_open", reverse position 1 and 3
	/*if ( ent->spawnflags & 1 ) {
		vec3_t	temp;

		VectorCopy( ent->pos3, temp );
		VectorCopy( ent->shared.s.origin, ent->pos3 );
		VectorCopy( temp, ent->pos1 );
	}*/

	InitMover( ent );

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = true;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;
}
