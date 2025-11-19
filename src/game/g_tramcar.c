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

/*
	SP_Tramcar
*/
#include "g_local.h"
#include "../server/server.h"

// defines
#define TRAMCAR_START_ON        1
#define TRAMCAR_TOGGLE          2
#define TRAMCAR_BLOCK_STOPS     4
#define TRAMCAR_LEADER          8

void props_me109_think( gentity_t *ent );
void ExplodePlaneSndFx( gentity_t *self );

void Think_SetupAirplaneWaypoints( gentity_t *ent );
void truck_cam_think( gentity_t *ent );

// extern calls
extern void Think_SetupTrainTargets( gentity_t *ent );
extern void Reached_BinaryMover( gentity_t *ent );
extern void MatchTeam( gentity_t *teamLeader, int moverState, int time );
extern void SetMoverState( gentity_t *ent, moverState_t moverState, int time );
extern void Blocked_Door( gentity_t *ent, gentity_t *other );
extern void Think_BeginMoving( gentity_t *ent );
extern void propExplosionLarge( gentity_t *ent );
////////////////////////
// truck states
////////////////////////
typedef enum
{
	truck_nosound = 0,
	truck_idle,
	truck_gear1,
	truck_gear2,
	truck_gear3,
	truck_reverse,
	truck_moving,
	truck_breaking,
	truck_bouncy1,
	truck_bouncy2,
	truck_bouncy3
} truck_states1;

/////////////////////////
// truck sounds
/////////////////////////

int truck_idle_snd;
int truck_gear1_snd;
int truck_gear2_snd;
int truck_gear3_snd;
int truck_reverse_snd;
int truck_moving_snd;
int truck_breaking_snd;
int truck_bouncy1_snd;
int truck_bouncy2_snd;
int truck_bouncy3_snd;
int truck_sound;


////////////////////////
// plane states
////////////////////////
typedef enum
{
	plane_nosound = 0,
	plane_idle,
	plane_flyby1,
	plane_flyby2,
	plane_loop,
	plane_choke,
	plane_startup
} truck_states2;

///////////////////////////
// aircraft sounds
///////////////////////////

int fploop_snd;
int fpchoke_snd;
int fpattack_snd;
int fpexpdebris_snd;

int fpflyby1_snd;
int fpflyby2_snd;
int fpidle_snd;
int fpstartup_snd;

///////////////////////////
// airplane parts
///////////////////////////
int fuse_part;
int wing_part;
int tail_part;
int nose_part;
int crash_part;

// functions to be added
void InitTramcar( gentity_t *ent ) {
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	char        *sound;

	// This is here just for show
	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->shared.s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	if ( !Q_stricmp( ent->classname, "props_me109" ) ) {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/mapobjects/vehicles/m109s.md3" );
	}

	if ( !Q_stricmp( ent->classname, "truck_cam" ) ) {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/mapobjects/vehicles/truck_base.md3" );
	}

	// if the "loopsound" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->shared.s.loopSound = G_SoundIndex( sound );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->shared.s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->use = Use_BinaryMover;
//	ent->reached = Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_MOVER;

	VectorCopy( ent->pos1, ent->shared.r.currentOrigin );

	SV_LinkEntity( ent );

	ent->shared.s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->shared.s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( !ent->speed ) {
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->shared.s.pos.trDelta );
	ent->shared.s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->shared.s.pos.trDuration <= 0 ) {
		ent->shared.s.pos.trDuration = 1;
	}
}

void Calc_Roll( gentity_t *ent ) {
	gentity_t *target;
	vec3_t vec;
	vec3_t forward;
	vec3_t right;
	float dot;
	float dot2;
	vec3_t tang;

	target = ent->nextTrain;

	VectorCopy( ent->shared.r.currentAngles, tang );
	tang[ROLL] = 0;

	AngleVectors( tang, forward, right, NULL );
	VectorSubtract( target->nextTrain->nextTrain->shared.s.origin, ent->shared.r.currentOrigin, vec );
	VectorNormalize( vec );

	dot = DotProduct( vec, forward );
	dot2 = DotProduct( vec, right );

	ent->angle = (int) ent->angle;

	if ( dot2 > 0 ) {
		if ( ent->shared.s.apos.trBase[ROLL] < -( ent->angle * 2 ) ) {
			ent->shared.s.apos.trBase[ROLL] += 2;
		} else if ( ent->shared.s.apos.trBase[ROLL] > -( ent->angle * 2 ) ) {
			ent->shared.s.apos.trBase[ROLL] -= 2;
		}

		if ( ent->shared.s.apos.trBase[ROLL] > 90 ) {
			ent->shared.s.apos.trBase[ROLL] = 90;
		}
	} else if ( dot2 < 0 )     {
		if ( ent->shared.s.apos.trBase[ROLL] > -( ent->angle * 2 ) ) {
			ent->shared.s.apos.trBase[ROLL] -= 2;
		} else if ( ent->shared.s.apos.trBase[ROLL] < -( ent->angle * 2 ) ) {
			ent->shared.s.apos.trBase[ROLL] += 2;
		}

		if ( ent->shared.s.apos.trBase[ROLL] < -90 ) {
			ent->shared.s.apos.trBase[ROLL] = -90;
		}
	} else {
		ent->shared.s.apos.trBase[ROLL] = 0;
	}

	SV_LinkEntity( ent );

	ent->nextthink = level.time + 50;
}


#define MAXCHOICES  8

void GetNextTrack( gentity_t *ent ) {
	gentity_t   *track = NULL;
	gentity_t   *next;
	gentity_t   *choice[MAXCHOICES];
	int num_choices = 0;
	int rval;

	next = ent->nextTrain;

	if ( !( next->track ) ) {
		Com_Printf( "NULL track name for %s on %s\n", ent->classname, next->targetname );
		return;
	}

	while ( 1 )
	{
		track = G_Find( track, FOFS( targetname ), next->track );

		if ( !track ) {
			break;
		}

		choice[num_choices++] = track;

		if ( num_choices == MAXCHOICES ) {
			break;
		}
	}

	if ( !num_choices ) {
		Com_Printf( "GetNextTrack didnt find a track\n" );
		return;
	}

	rval = rand() % num_choices;

	ent->nextTrain = NULL;
	ent->target = choice[rval]->targetname;

}

void Reached_Tramcar( gentity_t *ent ) {
	gentity_t       *next;
	float speed;
	vec3_t move;
	float length;

	// copy the apropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		return;     // just stop
	}

	// Rafael
	if ( next->wait == -1 && next->count ) {
		// Com_Printf ("stoped wait = -1 count %i\n",next->count);
		return;
	}

	if ( !Q_stricmp( ent->classname, "props_me109" ) ) {
		vec3_t vec, angles;
		float diff;

		if ( next->spawnflags & 8 ) { // laps
			next->count--;

			if ( !next->count ) {
				next->count = next->count2;

				GetNextTrack( ent );
				Think_SetupAirplaneWaypoints( ent );

				next = ent->nextTrain;

				Com_Printf( "changed track to %s\n", next->targetname );
			} else {
				Com_Printf( "%s lap %i\n", next->targetname, next->count );
			}
		} else if ( ( next->spawnflags & 1 ) && !( next->count ) && ent->health > 0 )         { // SCRIPT flag
			GetNextTrack( ent );
			Think_SetupAirplaneWaypoints( ent );
		} else if ( ( next->spawnflags & 2 ) && ( ent->spawnflags & 8 ) && ent->health <= 0 && ent->takedamage )         { // death path
			ent->takedamage = qfalse;

			GetNextTrack( ent );
			Think_SetupAirplaneWaypoints( ent );
		} else if ( ( next->spawnflags & 4 ) )       { // explode the plane
			ExplodePlaneSndFx( ent );

			ent->shared.s.modelindex = crash_part;
			// spawn the wing at the player effect

			ent->nextTrain = NULL;
			G_UseTargets( next, NULL );

			return;
		}

		VectorSubtract( ent->nextTrain->nextTrain->shared.s.origin, ent->shared.r.currentOrigin, vec );
		vectoangles( vec, angles );


		diff = AngleSubtract( ent->shared.r.currentAngles [YAW], angles[YAW] );

		ent->rotate[1] = 1;
		ent->angle = -diff;

		{
			VectorCopy( next->shared.s.origin, ent->pos1 );
			VectorCopy( next->nextTrain->shared.s.origin, ent->pos2 );

			// if the path_corner has a speed, use that
			if ( next->speed ) {
				speed = next->speed;
			} else {
				// otherwise use the train's speed
				speed = ent->speed;
			}
			if ( speed < 1 ) {
				speed = 1;
			}

			// calculate duration
			VectorSubtract( ent->pos2, ent->pos1, move );
			length = VectorLength( move );

			ent->shared.s.apos.trDuration = length * 1000 / speed;

//testing
// ent->gDuration = ent->s.apos.trDuration;
			ent->gDurationBack = ent->gDuration = ent->shared.s.apos.trDuration;
// ent->gDeltaBack = ent->gDelta =

		}

		VectorClear( ent->shared.s.apos.trDelta );

		SetMoverState( ent, MOVER_1TO2ROTATE, level.time );
		VectorCopy( ent->shared.r.currentAngles, ent->shared.s.apos.trBase );

		SV_LinkEntity( &ent->shared );

		ent->think = props_me109_think;
		ent->nextthink = level.time + 50;
	} else if ( !Q_stricmp( ent->classname, "truck_cam" ) )       {
		Com_Printf( "target: %s\n", next->targetname );

		if ( next->spawnflags & 2 ) { // END
			ent->shared.s.loopSound = 0; // stop sound
			ent->nextTrain = NULL;
			return;
		} else
		{
			vec3_t vec, angles;
			float diff;

			if ( next->spawnflags & 4 ) { // reverse
				ent->props_frame_state = truck_reverse;
				VectorSubtract( ent->shared.r.currentOrigin, ent->nextTrain->nextTrain->shared.s.origin, vec );
			} else
			{
				ent->props_frame_state = truck_moving;
				VectorSubtract( ent->nextTrain->nextTrain->shared.s.origin, ent->shared.r.currentOrigin, vec );
			}

			vectoangles( vec, angles );

			diff = AngleSubtract( ent->shared.r.currentAngles [YAW], angles[YAW] );

			ent->rotate[1] = 1;
			ent->angle = -diff;

			if ( angles[YAW] == 0 ) {
				ent->shared.s.apos.trDuration = ent->shared.s.pos.trDuration;
			} else {
				ent->shared.s.apos.trDuration = 1000;
			}

//testing
			ent->gDuration = ent->shared.s.pos.trDuration;

			VectorClear( ent->shared.s.apos.trDelta );

			SetMoverState( ent, MOVER_1TO2ROTATE, level.time );
			VectorCopy( ent->shared.r.currentAngles, ent->shared.s.apos.trBase );

			SV_LinkEntity( &ent->shared );
		}

		if ( next->wait == -1 ) {
			ent->props_frame_state = truck_idle;
		}

		if ( next->count2 == 1 ) {
			ent->props_frame_state = truck_gear1;
		} else if ( next->count2 == 2 ) {
			ent->props_frame_state = truck_gear2;
		} else if ( next->count2 == 3 ) {
			ent->props_frame_state = truck_gear3;
		}

		switch ( ent->props_frame_state )
		{
		case truck_idle: ent->shared.s.loopSound = truck_idle_snd; break;
		case truck_gear1: ent->shared.s.loopSound = truck_gear1_snd; break;
		case truck_gear2: ent->shared.s.loopSound = truck_gear2_snd; break;
		case truck_gear3: ent->shared.s.loopSound = truck_gear3_snd; break;
		case truck_reverse: ent->shared.s.loopSound = truck_reverse_snd; break;
		case truck_moving: ent->shared.s.loopSound = truck_moving_snd; break;
		case truck_breaking: ent->shared.s.loopSound = truck_breaking_snd; break;
		case truck_bouncy1: ent->shared.s.loopSound = truck_bouncy1_snd; break;
		case truck_bouncy2: ent->shared.s.loopSound = truck_bouncy2_snd; break;
		case truck_bouncy3: ent->shared.s.loopSound = truck_bouncy3_snd; break;
		}

//testing
		ent->shared.s.loopSound = truck_sound;
		ent->think = truck_cam_think;
		ent->nextthink = level.time + ( FRAMETIME / 2 );

	} else if ( !Q_stricmp( ent->classname, "camera_cam" ) )       {

	}

	// fire all other targets
	G_UseTargets( next, NULL );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;

	if ( next->wait == -1 ) {
		next->count = 1;
	}

	VectorCopy( next->shared.s.origin, ent->pos1 );
	VectorCopy( next->nextTrain->shared.s.origin, ent->pos2 );

	// if the path_corner has a speed, use that
	if ( next->speed ) {
		speed = next->speed;
	} else {
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if ( speed < 1 ) {
		speed = 1;
	}

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	ent->shared.s.pos.trDuration = length * 1000 / speed;

//testing
// ent->gDuration = ent->s.pos.trDuration;
	ent->gDurationBack = ent->gDuration = ent->shared.s.pos.trDuration;
// ent->gDeltaBack = ent->gDelta = ;

	// looping sound
	if ( next->soundLoop ) {
		ent->shared.s.loopSound = next->soundLoop;
	}

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	// if ( next->wait )
	if ( next->wait && next->wait != -1 ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->shared.s.pos.trType = TR_STATIONARY;
	}
}

extern void func_explosive_explode( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );

void Tramcar_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	gentity_t       *slave;

	func_explosive_explode( self, self, inflictor, 0, 0 );

	// link all teammembers
	for ( slave = self ; slave ; slave = slave->teamchain )
	{

		if ( slave == self ) {
			continue;
		}

		// slaves need to inherit position
		slave->nextTrain = self->nextTrain;

		slave->shared.s.pos.trType = self->shared.s.pos.trType;
		slave->shared.s.pos.trTime = self->shared.s.pos.trTime;
		slave->shared.s.pos.trDuration = self->shared.s.pos.trDuration;
		VectorCopy( self->shared.s.pos.trBase, slave->shared.s.pos.trBase );
		VectorCopy( self->shared.s.pos.trDelta, slave->shared.s.pos.trDelta );

		slave->shared.s.apos.trType = self->shared.s.apos.trType;
		slave->shared.s.apos.trTime = self->shared.s.apos.trTime;
		slave->shared.s.apos.trDuration = self->shared.s.apos.trDuration;
		VectorCopy( self->shared.s.apos.trBase, slave->shared.s.apos.trBase );
		VectorCopy( self->shared.s.apos.trDelta, slave->shared.s.apos.trDelta );

		slave->think = self->think;
		slave->nextthink = self->nextthink;

		VectorCopy( self->pos1, slave->pos1 );
		VectorCopy( self->pos2, slave->pos2 );

		slave->speed = self->speed;

		slave->flags &= ~FL_TEAMSLAVE;
		// make it visible

		if ( self->use ) {
			slave->use = self->use;
		}

		SV_LinkEntity( slave );
	}

	self->use = NULL;

	self->is_dead = qtrue;

	self->takedamage = qfalse;

	if ( self->nextTrain ) {
		self->nextTrain = 0;
	}

	self->shared.s.loopSound = 0;

	VectorCopy( self->shared.r.currentOrigin, self->shared.s.pos.trBase );
	VectorCopy( self->shared.r.currentAngles, self->shared.s.apos.trBase );

	self->flags |= FL_TEAMSLAVE;
	SV_UnlinkEntity( self );

}

void TramCarUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t       *next;

	if ( level.time >= ent->shared.s.pos.trTime + ent->shared.s.pos.trDuration ) {

		next = ent->nextTrain;

		if ( next->wait == -1 && next->count ) {
			next->count = 0;
			//Com_Printf ("Moving next->count %i\n", next->count);
		}

		Reached_Tramcar( ent );

	}
//	else
//		Com_Printf ("no can do havent reached yet\n");

}


void Blocked_Tramcar( gentity_t *ent, gentity_t *other ) {
	// remove anything other than a client
	if ( !other->client ) {
		
		G_TempEntity( other->shared.s.origin, EV_ITEM_POP );
		G_FreeEntity( other );
		return;
	}

	if ( other->flags & FL_GODMODE ) {
		other->flags &= ~FL_GODMODE;
		other->client->ps.stats[STAT_HEALTH] = other->health = 0;
	}

	G_Damage( other, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );

}

/*QUAKED func_tramcar (0 .5 .8) ? START_ON TOGGLE - LEADER
health value of 999 will designate the piece as non damageable

The leader of the tramcar group must have the leader flag set or
you'll end up with co-planer poly heaven. When the health of the Leader
of the team hits zero it will unlink and the team will become visible

A tramcar is a mover that moves between path_corner target points.
all tramcar parts MUST HAVE AN ORIGIN BRUSH. (this is true for all parts)

The tramcar spawns at the first target it is pointing at.

If you are going to have enemies ride the tramcar it must be placed in its ending
position when you bsp the map you can the start it by targeting the desired path_corner

"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius

"type" type of debris ("glass", "wood", "metal", "gibs") default is "wood"
"mass" defaults to 75.  This determines how much debris is emitted when it explodes.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
void SP_func_tramcar( gentity_t *self ) {

	int mass;
	char    *type;
	char    *s;
	char buffer[MAX_QPATH];

	VectorClear( self->shared.s.angles );

	//if (self->spawnflags & TRAMCAR_BLOCK_STOPS) {
	//	self->damage = 0;
	//	self->s.eFlags |= EF_MOVER_STOP;
	//}
	//else {
	if ( !self->damage ) {
		self->damage = 100;
	}
	//}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		Com_Printf( "func_tramcar without a target at %s\n", vtos( self->shared.r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	if ( self->spawnflags & TRAMCAR_LEADER ) {
		if ( !self->health ) {
			self->health = 50;
		}

		self->takedamage = qtrue;
		self->die = Tramcar_die;

		if ( self->health < 999 ) {
			self->isProp = qtrue;
		}
	}

	SV_SetBrushModel( self, self->model );

	if ( G_SpawnInt( "mass", "75", &mass ) ) {
		self->count = mass;
	} else {
		self->count = 75;
	}

	G_SpawnString( "type", "wood", &type );
	if ( !Q_stricmp( type,"wood" ) ) {
		self->key = 0;
	} else if ( !Q_stricmp( type,"glass" ) )   {
		self->key = 1;
	} else if ( !Q_stricmp( type,"metal" ) )                                                            {
		self->key = 2;
	} else if ( !Q_stricmp( type,"gibs" ) )                                                                                                                     {
		self->key = 3;
	}

	if ( G_SpawnString( "noise", "NOSOUND", &s ) ) {
		if ( Q_stricmp( s, "nosound" ) ) {
			Q_strncpyz( buffer, s, sizeof( buffer ) );
			self->shared.s.dl_intensity = G_SoundIndex( buffer );
		}
	} else {
		switch ( self->key )
		{
		case 0:     // "wood"
			self->shared.s.dl_intensity = G_SoundIndex( "sound/world/boardbreak.wav" );
			break;
		case 1:     // "glass"
			self->shared.s.dl_intensity = G_SoundIndex( "sound/world/glassbreak.wav" );
			break;
		case 2:     // "metal"
			self->shared.s.dl_intensity = G_SoundIndex( "sound/world/metalbreak.wav" );
			break;
		case 3:     // "gibs"
			self->shared.s.dl_intensity = G_SoundIndex( "sound/player/gibsplit1.wav" );
			break;
		}
	}

	self->shared.s.density = self->count;  // pass the "mass" to the client

	InitTramcar( self );

	self->reached = Reached_Tramcar;

	self->nextthink = level.time + ( FRAMETIME / 2 );

	self->think = Think_SetupTrainTargets;

	self->blocked = Blocked_Tramcar;

	if ( self->spawnflags & TRAMCAR_TOGGLE ) {
		self->use = TramCarUse;
	}

}


////////////////////////////
// me109
////////////////////////////

void plane_AIScript_AlertEntity( gentity_t *ent ) {

	// when count reaches 0, the marker is active
	ent->count--;

	if ( ent->count <= 0 ) {
		ent->count = 0;
	}
}

/*QUAKED plane_waypoint (.5 .3 0) (-8 -8 -8) (8 8 8) SCRIPTS DIE EXPLODE LAPS ATTACK
"count" number of times this waypoint needs to be triggered by an AIScript "alertentity" call before the aircraft changes tracks
"track"	tells it what track to branch off to there can be several track with the same track name
the program will pick one randomly there can be a maximum of eight tracks at any branch

the entity will fire its target when reached
*/
void SP_plane_waypoint( gentity_t *self ) {

	if ( !self->targetname ) {
		Com_Printf( "plane_waypoint with no targetname at %s\n", vtos( self->shared.s.origin ) );
		G_FreeEntity( self );
		return;
	}

	if ( self->spawnflags & 1 ) {
		self->AIScript_AlertEntity = plane_AIScript_AlertEntity;
	}

	if ( self->count ) {
		self->count2 = self->count;
	}

	if ( self->wait == -1 ) {
		self->count = 1;
	}
}


/*QUAKED props_me109 (.7 .3 .1) (-128 -128 0) (128 128 64) START_ON TOGGLE SPINNINGPROP FIXED_DIE
default health = 1000
*/

void ExplodePlaneSndFx( gentity_t *self ) {
	gentity_t   *temp;
	vec3_t dir;
	gentity_t   *part;
	int i;
	vec3_t start;

	temp = G_Spawn();

	if ( !temp ) {
		return;
	}

	G_SetOrigin( temp, self->melee->shared.s.pos.trBase );
	G_AddEvent( temp, EV_GLOBAL_SOUND, fpexpdebris_snd );
	temp->think = G_FreeEntity;
	temp->nextthink = level.time + 10000;
	SV_LinkEntity( temp );

	// added this because plane may be parked on runway
	// we may want to add some exotic deaths to parked aircraft
	if ( self->nextTrain && self->nextTrain->spawnflags & 4 ) { // explode the plane
		// spawn the wing at the player
		gentity_t   *player;
		vec3_t vec, ang;

		player = AICast_FindEntityForName( "player" );

		if ( !player ) {
			return;
		}

		VectorSubtract( player->shared.s.origin, self->shared.r.currentOrigin, vec );
		vectoangles( vec, ang );
		AngleVectors( ang, dir, NULL, NULL );

		dir[2] = 1;

		VectorCopy( self->shared.r.currentOrigin, start );

		part = fire_flamebarrel( temp, start, dir );

		if ( !part ) {
			Com_Printf( "ExplodePlaneSndFx Failed to spawn part\n" );
			return;
		}

		part->shared.s.eType = ET_FP_PARTS;

		part->shared.s.modelindex = wing_part;

		return;
	}

	AngleVectors( self->shared.r.currentAngles, dir, NULL, NULL );

	for ( i = 0; i < 4; i++ )
	{
		VectorCopy( self->shared.r.currentOrigin, start );

		start[0] += crandom() * 64;
		start[1] += crandom() * 64;
		start[2] += crandom() * 32;

		part = fire_flamebarrel( temp, start, dir );

		if ( !part ) {
			continue;
		}

		part->shared.s.eType = ET_FP_PARTS;

		if ( i == 0 ) {
			part->shared.s.modelindex = fuse_part;
		} else if ( i == 1 ) {
			part->shared.s.modelindex = wing_part;
		} else if ( i == 2 ) {
			part->shared.s.modelindex = tail_part;
		} else {
			part->shared.s.modelindex = nose_part;
		}
	}
}

void props_me109_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	Com_Printf( "dead\n" );

	VectorClear( self->rotate );
	VectorSet( self->rotate, 0, 1, 0 ); //sigh

	if ( self->spawnflags & 8 ) { // FIXED_DIE
		return;
	}

	propExplosionLarge( self );
	self->melee->shared.s.loopSound = self->melee->noise_index = 0;
	ExplodePlaneSndFx( self );
	G_FreeEntity( self );
}

void props_me109_pain( gentity_t *self, gentity_t *attacker, int damage, vec3_t point ) {
	vec3_t temp;

	Com_Printf( "pain: health = %i\n", self->health );

	VectorCopy( self->shared.r.currentOrigin, temp );
	VectorCopy( self->pos3, self->shared.r.currentOrigin );
	Spawn_Shard( self, NULL, 6, 999 );
	VectorCopy( temp, self->shared.r.currentOrigin );

	VectorClear( self->rotate );
	VectorSet( self->rotate, 0, 1, 0 ); //sigh
}

void Plane_Fire_Lead( gentity_t *self ) {
	vec3_t dir, right;
	vec3_t pos1, pos2;
	vec3_t position;

	AngleVectors( self->shared.r.currentAngles, dir, right, NULL );
	VectorCopy( self->shared.r.currentOrigin, position );
	VectorMA( position, 64, right, pos1 );
	VectorMA( position, -64, right, pos2 );

	fire_lead( self, pos1, dir, 12 );
	fire_lead( self, pos2, dir, 12 );
}

void Plane_Attack( gentity_t *self, qboolean in_PVS ) {
	if ( self->nextTrain->spawnflags & 16 ) {
		self->count++;

		if ( self->count == 3 ) {
			self->shared.s.density = 8, self->count = 0;

			if ( in_PVS ) {
				G_AddEvent( self, EV_GLOBAL_SOUND, fpattack_snd );
			} else {
				G_AddEvent( self, EV_GENERAL_SOUND, fpattack_snd );
			}

			Plane_Fire_Lead( self );
		} else {
			self->shared.s.density = 7;
		}
	} else if ( self->spawnflags & 4 )     { // spinning prop
		self->shared.s.density = 7;
	} else {
		self->shared.s.density = 0;
	}
}

void props_me109_think( gentity_t *self ) {

	qboolean in_PVS = qfalse;

	{
		gentity_t *player;

		player = AICast_FindEntityForName( "player" );

		if ( player ) {
			in_PVS = SV_inPVS( player->shared.r.currentOrigin, self->shared.s.pos.trBase );

			if ( in_PVS ) {
				self->melee->shared.s.eType = ET_GENERAL;

				{
					float len;
					vec3_t vec;
					vec3_t forward;
					vec3_t dir;
					vec3_t point;

					VectorCopy( player->shared.r.currentOrigin, point );
					VectorSubtract( player->shared.r.currentOrigin, self->shared.r.currentOrigin, vec );
					len = VectorLength( vec );
					vectoangles( vec, dir );
					AngleVectors( dir, forward, NULL, NULL );
					VectorMA( point, len * 0.1, forward, point );

					G_SetOrigin( self->melee, point );
				}
			} else
			{
				self->melee->shared.s.eType = ET_GENERAL;
			}

			SV_LinkEntity( self->melee );
		}
	}

	Plane_Attack( self, in_PVS );

	Calc_Roll( self );

	if ( self->health < 250 ) {
		gentity_t *tent;
		vec3_t point;

		VectorCopy( self->shared.r.currentOrigin, point );
		tent = G_TempEntity( point, EV_SMOKE );
		VectorCopy( point, tent->shared.s.origin );
		tent->shared.s.time = 2000;
		tent->shared.s.time2 = 1000;
		tent->shared.s.density = 4;
		tent->shared.s.angles2[0] = 16;
		tent->shared.s.angles2[1] = 48;
		tent->shared.s.angles2[2] = 10;

		self->props_frame_state = plane_choke;
		self->health--;
	}

	if ( self->health > 0 ) {
		self->nextthink = level.time + 50;

		if ( self->props_frame_state == plane_choke ) {
			self->melee->shared.s.loopSound = self->melee->noise_index = fpchoke_snd;
		} else if ( self->props_frame_state == plane_startup )     {
			self->melee->shared.s.loopSound = self->melee->noise_index = fpstartup_snd;
		} else if ( self->props_frame_state == plane_idle )     {
			self->melee->shared.s.loopSound = self->melee->noise_index = fpidle_snd;
		} else if ( self->props_frame_state == plane_flyby1 )     {
			self->melee->shared.s.loopSound = self->melee->noise_index = fpflyby1_snd;
		} else if ( self->props_frame_state == plane_flyby2 )     {
			self->melee->shared.s.loopSound = self->melee->noise_index = fpflyby2_snd;
		}
	} else
	{
		propExplosionLarge( self );
		self->melee->shared.s.loopSound = self->melee->noise_index = 0;

		ExplodePlaneSndFx( self );
		G_FreeEntity( self->melee );
		G_FreeEntity( self );


	}

}

void Think_SetupAirplaneWaypoints( gentity_t *ent ) {
	gentity_t       *path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( !ent->nextTrain ) {
		Com_Printf( "plane at %s with an unfound target\n",
				  vtos( ent->shared.r.absmin ) );
		return;
	}

	start = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			Com_Printf( "plane at %s without a target\n",
					  vtos( path->shared.s.origin ) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS( targetname ), path->target );
			if ( !next ) {
				Com_Printf( "plane at %s without a target path_corner\n",
						  vtos( path->shared.s.origin ) );
				return;
			}
		} while ( strcmp( next->classname, "plane_waypoint" ) );

		path->nextTrain = next;
	}

	if ( ent->spawnflags & 2 ) { // Toggle
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.s.pos.trBase );
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.r.currentOrigin );
		SV_LinkEntity( &ent->shared );
	} else {
		Reached_Tramcar( ent );
	}
}


void PlaneUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t       *next;

	if ( level.time >= ent->shared.s.pos.trTime + ent->shared.s.pos.trDuration ) {

		next = ent->nextTrain;

		if ( next->wait == -1 && next->count ) {
			next->count = 0;
			//Com_Printf ("Moving next->count %i\n", next->count);
		}

		Reached_Tramcar( ent );

	}
//	else
//		Com_Printf ("no can do havent reached yet\n");

}


void InitPlaneSpeaker( gentity_t *ent ) {
	gentity_t   *snd;

	snd = G_Spawn();

	snd->noise_index = fploop_snd;

	snd->shared.s.eType = ET_SPEAKER;
	snd->shared.s.eventParm = snd->noise_index;
	snd->shared.s.frame = 0;
	snd->shared.s.clientNum = 0;

	snd->shared.s.loopSound = snd->noise_index;

	snd->shared.r.svFlags |= SVF_BROADCAST;

	VectorCopy( ent->shared.s.origin, snd->shared.s.pos.trBase );

	ent->melee = snd;

	SV_LinkEntity( snd );

}

void SP_props_me109( gentity_t *ent ) {

	VectorSet( ent->shared.r.mins, -128, -128, -128 );
	VectorSet( ent->shared.r.maxs, 128, 128, 128 );

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.r.svFlags  = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_MOVER;

	ent->isProp = qtrue;

	ent->shared.s.modelindex = G_ModelIndex( "models/mapobjects/vehicles/m109.md3" );

	if ( !ent->health ) {
		ent->health = 500;
	}

	ent->takedamage = qtrue;

	ent->die = props_me109_die;
	ent->pain = props_me109_pain;

	ent->reached = Reached_Tramcar;

	ent->nextthink = level.time + ( FRAMETIME / 2 );

	ent->think = Think_SetupAirplaneWaypoints;

	ent->use = PlaneUse;

	if ( !( ent->speed ) ) {
		ent->speed = 1000;
	}

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( ent->spawnflags & 4 ) {
		ent->shared.s.density = 7;
	}

	SV_LinkEntity( &ent->shared );

	fploop_snd = G_SoundIndex( "sound/fighterplane/fploop.wav" );
	fpchoke_snd = G_SoundIndex( "sound/fighterplane/fpchoke.wav" );
	fpattack_snd = G_SoundIndex( "sound/weapons/mg42/37mm.wav" );
	fpexpdebris_snd = G_SoundIndex( "sound/fighterplane/fpexpdebris.wav" );


	fpflyby1_snd = G_SoundIndex( "sound/fighterplane/fpflyby1.wav" );
	fpflyby2_snd = G_SoundIndex( "sound/fighterplane/fpflyby2.wav" );
	fpidle_snd = G_SoundIndex( "sound/fighterplane/fpidle.wav" );
	fpstartup_snd = G_SoundIndex( "sound/fighterplane/fpstartup.wav" );


	fuse_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_a.md3" );
	wing_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_b.md3" );
	tail_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_c.md3" );
	nose_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_d.md3" );

	crash_part = G_ModelIndex( "models/mapobjects/vehicles/m109crash.md3" );

	InitPlaneSpeaker( ent );

}

/////////////////////////
// TRUCK DRIVE
/////////////////////////

/*QUAKED truck_cam (.7 .3 .1) ? START_ON TOGGLE - -
*/
void truck_cam_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( player && player != other ) {
		// Com_Printf ("other: %s\n", other->aiName);
		return;
	}

	if ( !self->nextTrain ) {
		self->touch = NULL;
		return;
	}

	// lock the player to the moving truck
	{
		vec3_t point;

		SV_UnlinkEntity( other );

		// VectorCopy ( self->r.currentOrigin, other->client->ps.origin );
		VectorCopy( self->shared.r.currentOrigin, point );
		point[2] = other->client->ps.origin[2];
		VectorCopy( point, other->client->ps.origin );

		// save results of pmove
		BG_PlayerStateToEntityState( &other->client->ps, &other->shared.s, qtrue );

		// use the precise origin for linking
		VectorCopy( other->client->ps.origin, other->shared.r.currentOrigin );

		other->client->ps.persistant[PERS_HWEAPON_USE] = 1;

		SV_LinkEntity( other );
	}

}

void truck_cam_think( gentity_t *ent ) {
	ent->nextthink = level.time + ( FRAMETIME / 2 );
}

void SP_truck_cam( gentity_t *self ) {
	int mass;

	VectorClear( self->shared.s.angles );

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		Com_Printf( "truck_cam without a target at %s\n", vtos( self->shared.r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	SV_SetBrushModel( self, self->model );

	if ( G_SpawnInt( "mass", "20", &mass ) ) {
		self->count = mass;
	} else {
		self->count = 20;
	}

	InitTramcar( self );

	self->nextthink = level.time + ( FRAMETIME / 2 );

	self->think = Think_SetupTrainTargets;

	self->touch = truck_cam_touch;

	self->shared.s.loopSound = 0;
	self->props_frame_state = 0;

	self->clipmask = CONTENTS_SOLID;

	// G_SetOrigin (self, self->s.origin);
	// G_SetAngle (self, self->s.angles);

	self->reached = Reached_Tramcar;

	self->shared.s.density = 6;

	//start_drive_grind_gears
	truck_sound = G_SoundIndex( "sound/vehicles/start_drive_grind_gears_01_11k.wav" );
	// truck_sound = G_SoundIndex ( "sound/vehicles/tankmove1.wav" );

	truck_idle_snd = G_SoundIndex( "sound/vehicles/truckidle.wav" );
	truck_gear1_snd = G_SoundIndex( "sound/vehicles/truckgear1.wav" );
	truck_gear2_snd = G_SoundIndex( "sound/vehicles/truckgear2.wav" );
	truck_gear3_snd = G_SoundIndex( "sound/vehicles/truckgear3.wav" );
	truck_reverse_snd = G_SoundIndex( "sound/vehicles/truckreverse.wav" );
	truck_moving_snd = G_SoundIndex( "sound/vehicles/truckmoving.wav" );
	truck_breaking_snd = G_SoundIndex( "sound/vehicles/truckbreaking.wav" );
	truck_bouncy1_snd = G_SoundIndex( "sound/vehicles/truckbouncy1.wav" );
	truck_bouncy2_snd = G_SoundIndex( "sound/vehicles/truckbouncy2.wav" );
	truck_bouncy3_snd = G_SoundIndex( "sound/vehicles/truckbouncy3.wav" );
}

/////////////////////////
// camera cam
/////////////////////////

/*QUAKED camera_cam (.5 .7 .3) (-8 -8 -8) (8 8 8) ON TRACKING MOVING -
"track" is the targetname of the entity providing the starting direction use an info_notnull
*/

void delayOnthink( gentity_t *ent ) {
	if ( ent->melee ) {
		ent->melee->use( ent->melee, NULL, NULL );
	}
}

void Init_Camera( gentity_t *ent ) {
	vec3_t move;
	float distance;

	ent->moverState = MOVER_POS1;
	ent->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_MOVER;

	VectorCopy( ent->pos1, ent->shared.r.currentOrigin );

	SV_LinkEntity( &ent->shared );

	ent->shared.s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->shared.s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( !ent->speed ) {
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->shared.s.pos.trDelta );
	ent->shared.s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->shared.s.pos.trDuration <= 0 ) {
		ent->shared.s.pos.trDuration = 1;
	}
}

void camera_cam_think( gentity_t *ent ) {
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	if ( ent->spawnflags & 2 ) { // tracking
		vec3_t point;

		SV_UnlinkEntity( player );

		// VectorCopy ( self->r.currentOrigin, other->client->ps.origin );
		VectorCopy( ent->shared.r.currentOrigin, point );
		point[2] = player->client->ps.origin[2];
		VectorCopy( point, player->client->ps.origin );

		// save results of pmove
		BG_PlayerStateToEntityState( &player->client->ps, &player->shared.s, qtrue );

		// use the precise origin for linking
		VectorCopy( player->client->ps.origin, player->shared.r.currentOrigin );

		// tracking
		{
			gentity_t   *target = NULL;
			vec3_t dang;
			vec3_t vec;

			if ( ent->track ) {
				target = G_Find( NULL, FOFS( targetname ), ent->track );
			}

			if ( target ) {
				VectorSubtract( target->shared.r.currentOrigin, ent->shared.r.currentOrigin, vec );
				vectoangles( vec, dang );
				SetClientViewAngle( player, dang );

				VectorCopy( ent->shared.r.currentOrigin, ent->shared.s.pos.trBase );
				VectorCopy( dang, ent->shared.s.apos.trBase );

				SV_LinkEntity( &ent->shared );
			}
		}

		SV_LinkEntity( player );
	}

	ent->nextthink = level.time + ( FRAMETIME / 2 );
}

void camera_cam_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	if ( !( ent->spawnflags & 1 ) ) {
		ent->think = camera_cam_think;
		ent->nextthink = level.time + ( FRAMETIME / 2 );
		ent->spawnflags |= 1;
		{
			player->client->ps.persistant[PERS_HWEAPON_USE] = 1;
			player->client->ps.viewlocked = 4;
			player->client->ps.viewlocked_entNum = ent->shared.s.number;
		}
	} else
	{
		ent->spawnflags &= ~1;
		ent->think = NULL;
		{
			player->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			player->client->ps.viewlocked = 0;
			player->client->ps.viewlocked_entNum = 0;
		}
	}

}

void camera_cam_firstthink( gentity_t *ent ) {
	gentity_t   *target = NULL;
	vec3_t dang;
	vec3_t vec;

	if ( ent->track ) {
		target = G_Find( NULL, FOFS( targetname ), ent->track );
	}

	if ( target ) {
		VectorSubtract( target->shared.s.origin, ent->shared.r.currentOrigin, vec );
		vectoangles( vec, dang );
		G_SetAngle( ent, dang );
	}

	if ( ent->target ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
		ent->think = Think_SetupTrainTargets;
	}
}

void SP_camera_cam( gentity_t *ent ) {
	Init_Camera( ent );

	ent->shared.r.svFlags  = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_MOVER;

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	ent->reached = Reached_Tramcar;

	ent->nextthink = level.time + ( FRAMETIME / 2 );

	ent->think = camera_cam_firstthink;

	ent->use = camera_cam_use;

	if ( ent->spawnflags & 1 ) { // On
		gentity_t *delayOn;

		delayOn = G_Spawn();
		delayOn->think = delayOnthink;
		delayOn->nextthink = level.time + 1000;
		delayOn->melee = ent;
		SV_LinkEntity( delayOn );
	}

}


/*QUAKED screen_fade (.3 .7 .9) (-8 -8 -8) (8 8 8)
"wait" duration of fade out
"delay" duration of fade in

  1 = 1 sec
  .5 = .5 sec

defaults are .5 sec
*/
void screen_fade_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 1 ) {
		// fade out
		SV_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time + 100, (int) ent->wait ) );
		ent->spawnflags &= ~1;
	} else
	{
		// fade in
		SV_SetConfigstring( CS_SCREENFADE, va( "0 %i %i", level.time + 100, (int) ent->delay ) );
		ent->spawnflags |= 1;
	}

}

void SP_screen_fade( gentity_t *ent ) {
	ent->use = screen_fade_use;

	if ( !ent->wait ) {
		ent->wait = 500;
	}
	if ( !ent->delay ) {
		ent->delay = 500;
	}

}

/*QUAKED camera_reset_player (.5 .7 .3) ?
touched will record the players position and fire off its targets and or cameras

used will reset the player back to his last position
*/

void mark_players_pos( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	gentity_t   *player;

	player = AICast_FindEntityForName( "player" );

	if ( player == other ) {
		VectorCopy( player->shared.r.currentOrigin, ent->shared.s.origin2 );
		VectorCopy( player->shared.r.currentAngles, ent->shared.s.angles2 );

		G_UseTargets( ent, NULL );
	}

}

void reset_players_pos( gentity_t *ent, gentity_t *other, gentity_t *activator ) {

	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	SV_UnlinkEntity( player );

	VectorCopy( ent->shared.s.origin2, player->client->ps.origin );

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->shared.s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->shared.r.currentOrigin );

	SetClientViewAngle( player, ent->shared.s.angles2 );

	player->client->ps.persistant[PERS_HWEAPON_USE] = 0;
	player->client->ps.viewlocked = 0;
	player->client->ps.viewlocked_entNum = 0;

	SV_LinkEntity( player );

}

extern void InitTrigger( gentity_t *self );

void SP_camera_reset_player( gentity_t *ent ) {
	InitTrigger( ent );

	ent->shared.r.contents = CONTENTS_TRIGGER;

	ent->touch = mark_players_pos;
	ent->use = reset_players_pos;

	SV_LinkEntity( &ent->shared );
}
