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

#define     GENERIC_DAMAGE  6

int snd_boardbreak;
int snd_glassbreak;
int snd_metalbreak;
int snd_ceramicbreak;
int snd_chaircreak;
int snd_chairthrow;
int snd_chairhitground;
int chair_metalbreak;

// JOSEPH 1-28-00
void DropToFloorG( gentity_t *ent ) {
	vec3_t dest;
	trace_t tr;

	VectorSet( dest, ent->shared.r.currentOrigin[0], ent->shared.r.currentOrigin[1], ent->shared.r.currentOrigin[2] - 4096 );
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, dest, ent->shared.s.number, MASK_SOLID, qfalse );

	if ( tr.startsolid ) {
		return;
	}

	ent->shared.s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	ent->nextthink = level.time + FRAMETIME;
}

void DropToFloor( gentity_t *ent ) {
	vec3_t dest;
	trace_t tr;

	VectorSet( dest, ent->shared.r.currentOrigin[0], ent->shared.r.currentOrigin[1], ent->shared.r.currentOrigin[2] - 4096 );
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, dest, ent->shared.s.number, MASK_SOLID, qfalse );

	if ( tr.startsolid ) {
		return;
	}

	if ( fabs( ent->shared.r.currentOrigin[2] - tr.endpos[2] ) > 1.0 ) {
		tr.endpos[2] = ( ent->shared.r.currentOrigin[2] - 1.0 );
	}

	ent->shared.s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	ent->think = DropToFloorG;
	ent->nextthink = level.time + FRAMETIME;
}

void moveit( gentity_t *ent, float yaw, float dist ) {
	vec3_t move;
	vec3_t origin;
	trace_t tr;
	vec3_t mins, maxs;

	yaw = yaw * M_PI * 2 / 360;

	move[0] = cos( yaw ) * dist;
	move[1] = sin( yaw ) * dist;
	move[2] = 0;

	VectorAdd( ent->shared.r.currentOrigin, move, origin );

	mins[0] = ent->shared.r.mins[0];
	mins[1] = ent->shared.r.mins[1];
	mins[2] = ent->shared.r.mins[2] + .01;

	maxs[0] = ent->shared.r.maxs[0];
	maxs[1] = ent->shared.r.maxs[1];
	maxs[2] = ent->shared.r.maxs[2] - .01;

	SV_Trace( &tr, ent->shared.r.currentOrigin, mins, maxs, origin, ent->shared.s.number, MASK_SHOT, qfalse );

	if ( ( tr.endpos[0] != origin[0] ) || ( tr.endpos[1] != origin[1] ) ) {
		mins[0] = ent->shared.r.mins[0] - 2.0;
		mins[1] = ent->shared.r.mins[1] - 2.0;
		maxs[0] = ent->shared.r.maxs[0] + 2.0;
		maxs[1] = ent->shared.r.maxs[1] + 2.0;

		SV_Trace( &tr, ent->shared.r.currentOrigin, mins, maxs, origin, ent->shared.s.number, MASK_SHOT, qfalse );
	}

	VectorCopy( tr.endpos, ent->shared.r.currentOrigin );

	VectorCopy( ent->shared.r.currentOrigin, ent->shared.s.pos.trBase );

	SV_LinkEntity( &ent->shared );
}

void touch_props_box_32( gentity_t *self, gentity_t *other, trace_t *trace ) {
	float ratio;
	vec3_t v;

	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 15 ) ) {
		return;
	}

	ratio = 2.5;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

/*QUAKED props_box_32 (1 0 0) (-16 -16 -16) (16 16 16)

*/
void SP_props_box_32( gentity_t *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/boxes/box32.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -16, -16, -16 );
	VectorSet( self->shared.r.maxs, 16, 16, 16 );

	self->touch = touch_props_box_32;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

void touch_props_box_48( gentity_t *self, gentity_t *other, trace_t *trace ) {
	float ratio;
	vec3_t v;

	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 23 ) ) {
		return;
	}

	ratio = 2.0;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

/*QUAKED props_box_48 (1 0 0) (-24 -24 -24) (24 24 24)

*/
void SP_props_box_48( gentity_t *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/boxes/box48.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -24, -24, -24 );
	VectorSet( self->shared.r.maxs, 24, 24, 24 );

	self->touch = touch_props_box_48;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

void touch_props_box_64( gentity_t *self, gentity_t *other, trace_t *trace ) {
	float ratio;
	vec3_t v;

	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 31 ) ) {
		return;
	}

	ratio = 1.5;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

/*QUAKED props_box_64 (1 0 0) (-32 -32 -32) (32 32 32)

*/
void SP_props_box_64( gentity_t *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/boxes/box64.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -32, -32, -32 );
	VectorSet( self->shared.r.maxs, 32, 32, 32 );

	self->touch = touch_props_box_64;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}
// END JOSEPH

// Rafael

void Psmoke_think( gentity_t *ent ) {
	gentity_t *tent;

	ent->count++;

	if ( ent->count == 30 ) {
		ent->think = G_FreeEntity;
	}

	tent = G_TempEntity( ent->shared.s.origin, EV_SMOKE );
	VectorCopy( ent->shared.s.origin, tent->shared.s.origin );
	tent->shared.s.time = 3000;
	tent->shared.s.time2 = 100;
	tent->shared.s.density = 0;
	tent->shared.s.angles2[0] = 4;
	tent->shared.s.angles2[1] = 32;
	tent->shared.s.angles2[2] = 50;

	ent->nextthink = level.time + FRAMETIME;
}

void prop_smoke( gentity_t *ent ) {
	gentity_t *Psmoke;

	Psmoke = G_Spawn();
	VectorCopy( ent->shared.r.currentOrigin, Psmoke->shared.s.origin );
	Psmoke->think = Psmoke_think;
	Psmoke->nextthink = level.time + FRAMETIME;
}

/*QUAKED props_sparks (.8 .46 .16) (-8 -8 -8) (8 8 8) ELECTRIC
the default direction is strait up use info_no_null for alt direction

delay = how long till next spark effect
wait = life of the spark with some random variance default 1.0 sec
health = random number of sparks upto specified amount default 8

start_size default 8 along the x
end_size default 8 along the y
by changing the size will change the spawn origin of the individual spark
ei 16 x 8 or 24 x 32 would cause the sparks to spawn that many units from
the origin

speed controls how quickly the sparks will travel default is 2
*/

void PGUNsparks_use( gentity_t *ent, gentity_t *self, gentity_t *activator ) {
	gentity_t *tent;

	tent = G_TempEntity( ent->shared.r.currentOrigin, EV_GUNSPARKS );
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	VectorCopy( ent->shared.r.currentAngles, tent->shared.s.angles );
	tent->shared.s.density = ent->health;
	tent->shared.s.angles2[2] = ent->speed;

}

void Psparks_think( gentity_t *ent ) {
	gentity_t   *tent;

//(SA) MOVE TO CLIENT!
	return;


	if ( ent->spawnflags & 1 ) {
		tent = G_TempEntity( ent->shared.r.currentOrigin, EV_SPARKS_ELECTRIC );
	} else {
		tent = G_TempEntity( ent->shared.r.currentOrigin, EV_SPARKS );
	}
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	VectorCopy( ent->shared.r.currentAngles, tent->shared.s.angles );
	tent->shared.s.density = ent->health;
	tent->shared.s.frame = ent->wait;
	tent->shared.s.angles2[0] = ent->start_size;
	tent->shared.s.angles2[1] = ent->end_size;
	tent->shared.s.angles2[2] = ent->speed;

	ent->nextthink = level.time + FRAMETIME + ent->delay + ( rand() % 600 );
}

void sparks_angles_think( gentity_t *ent ) {

	gentity_t *target = NULL;
	vec3_t vec;

	if ( ent->target ) {
		target = G_Find( NULL, FOFS( targetname ), ent->target );
	}

	if ( !target ) {
		VectorSet( ent->shared.r.currentAngles, 0, 0, 1 );
	} else
	{
		VectorSubtract( ent->shared.s.origin, target->shared.s.origin, vec );
		VectorNormalize( vec );
		VectorCopy( vec, ent->shared.r.currentAngles );
	}

	SV_LinkEntity( &ent->shared );

	ent->nextthink = level.time + FRAMETIME;
	if ( !Q_stricmp( ent->classname, "props_sparks" ) ) {
		ent->think = Psparks_think;
	} else {
		ent->use = PGUNsparks_use;
	}

}

void SP_props_sparks( gentity_t *ent ) {

	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;

	ent->think = sparks_angles_think;
	ent->nextthink = level.time + FRAMETIME;

	if ( !ent->health ) {
		ent->health = 8;
	}

	if ( !ent->wait ) {
		ent->wait = 1200;
	} else {
		ent->wait *= 1000;
	}

	if ( !ent->start_size ) {
		ent->start_size = 8;
	}

	if ( !ent->end_size ) {
		ent->end_size = 8;
	}

	if ( !ent->speed ) {
		ent->speed = 2;
	}

	SV_LinkEntity( &ent->shared );

}

/*QUAKED props_gunsparks (.8 .46 .16) (-8 -8 -8) (8 8 8)
the default direction is strait up use info_no_null for alt direction

this entity must be used to see the effect

"speed" default is 20
"health" number to spawn default is 4
*/

void SP_props_gunsparks( gentity_t *ent ) {
	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;

	ent->think = sparks_angles_think;
	ent->nextthink = level.time + FRAMETIME;

	if ( !ent->speed ) {
		ent->speed = 20;
	}

	if ( !ent->health ) {
		ent->health = 4;
	}

	SV_LinkEntity( &ent->shared );

}

/*QUAKED props_smokedust (.8 .46 .16) (-8 -8 -8) (8 8 8)
health = how many pieces 16 is default
*/

void smokedust_use( gentity_t *ent, gentity_t *self, gentity_t *activator ) {
	int i;
	gentity_t   *tent;
	vec3_t forward;

	AngleVectors( ent->shared.r.currentAngles, forward, NULL, NULL );

	for ( i = 0; i < ent->health; i++ )
	{
		tent = G_TempEntity( ent->shared.r.currentOrigin, EV_SMOKE );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		VectorCopy( forward, tent->shared.s.origin2 );
		tent->shared.s.time = 1000;
		tent->shared.s.time2 = 750;
		tent->shared.s.density = 3;
	}
}

void SP_SmokeDust( gentity_t *ent ) {

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

void dust_use( gentity_t *ent, gentity_t *self, gentity_t *activator ) {
	gentity_t   *tent;
	vec3_t forward;

	if ( ent->target ) {
		tent = G_TempEntity( ent->shared.r.currentOrigin, EV_DUST );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		VectorCopy( ent->shared.r.currentAngles, tent->shared.s.angles );
		if ( ent->spawnflags & 1 ) {
			tent->shared.s.density = 1;
		}
	} else
	{

		AngleVectors( ent->shared.r.currentAngles, forward, NULL, NULL );

		tent = G_TempEntity( ent->shared.r.currentOrigin, EV_DUST );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		VectorCopy( forward, tent->shared.s.angles );
		if ( ent->spawnflags & 1 ) {
			tent->shared.s.density = 1;
		}
	}
}

void dust_angles_think( gentity_t *ent ) {
	gentity_t *target;
	vec3_t vec;

	target = G_Find( NULL, FOFS( targetname ), ent->target );

	if ( !target ) {
		return;
	}

	VectorSubtract( ent->shared.s.origin, target->shared.s.origin, vec );
	VectorCopy( vec, ent->shared.r.currentAngles );
	SV_LinkEntity( &ent->shared );

}

void SP_Dust( gentity_t *ent ) {
	ent->use = dust_use;
	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;

	if ( ent->target ) {
		ent->think = dust_angles_think;
		ent->nextthink = level.time + FRAMETIME;
	}

	SV_LinkEntity( &ent->shared );
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

extern void G_ExplodeMissile( gentity_t *ent );

void propExplosionLarge( gentity_t *ent ) {
	gentity_t *bolt;

	bolt = G_Spawn();
	bolt->classname = "props_explosion_large";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_ExplodeMissile;
	bolt->shared.s.eType = ET_MISSILE;

	bolt->shared.s.weapon = WP_NONE;

	bolt->shared.s.eFlags = EF_BOUNCE_HALF;
	bolt->shared.r.ownerNum = ent->shared.s.number;
	bolt->parent = ent;
	bolt->damage = ent->health;
	bolt->splashDamage = ent->health;
	bolt->splashRadius = ent->health * 1.5;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;

	VectorCopy( ent->shared.r.currentOrigin, bolt->shared.s.pos.trBase );
	VectorCopy( ent->shared.r.currentOrigin, bolt->shared.r.currentOrigin );
}

void propExplosion( gentity_t *ent ) {
	gentity_t *bolt;

	extern void G_ExplodeMissile( gentity_t *ent );
	bolt = G_Spawn();
	bolt->classname = "props_explosion";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_ExplodeMissile;
	bolt->shared.s.eType = ET_MISSILE;

	bolt->shared.s.weapon = WP_NONE;

	bolt->shared.s.eFlags = EF_BOUNCE_HALF;
	bolt->shared.r.ownerNum = ent->shared.s.number;
	bolt->parent = ent;
	bolt->damage = ent->health;
	bolt->splashDamage = ent->health;
	bolt->splashRadius = ent->health * 1.5;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;

	VectorCopy( ent->shared.r.currentOrigin, bolt->shared.s.pos.trBase );
	VectorCopy( ent->shared.r.currentOrigin, bolt->shared.r.currentOrigin );
}

void InitProp( gentity_t *ent ) {
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	const char        *sound;

	if ( !Q_stricmp( ent->classname, "props_bench" ) ) {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/bench/bench_sm.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_radio" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/mapobjects/electronics/radio1.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_locker_tall" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/storage/lockertall.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_flippy_table" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/table/woodflip.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_crate_32x64" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/crate/crate32x64.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_58x112tablew" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/table/56x112tablew.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_castlebed" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/bed/castlebed.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_radioSEVEN" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/mapobjects/electronics/radios.md3" );
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

	ent->isProp = qtrue;

	ent->moverState = MOVER_POS1;
	ent->shared.s.eType = ET_MOVER;

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );
}

void props_bench_think( gentity_t *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 28 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{
		ent->clipmask = 0;
		ent->shared.r.contents = 0;
		ent->takedamage = qfalse;

		G_UseTargets( ent, NULL );
	}

}

void props_bench_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = props_bench_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED props_bench (.8 .6 .2) ?
requires an origin brush
health = 10 by default
*/
void SP_Props_Bench( gentity_t *ent ) {

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_bench_die;

	SV_LinkEntity( &ent->shared );
}

void props_radio_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {

	propExplosion( ent );

	ent->takedamage = qfalse;

	G_UseTargets( ent, NULL );

	G_FreeEntity( ent );
}

/*QUAKED props_radio (.8 .6 .2) ?
requires an origin brush
health = defaults to 100
*/
void SP_Props_Radio( gentity_t *ent ) {

	// Ridah, had to add this so I could load castle18dk7
	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_radio with NULL model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->takedamage = qtrue;

	ent->die = props_radio_die;

	SV_LinkEntity( &ent->shared );

}


void props_radio_dieSEVEN( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {

	int i;

	propExplosion( ent );

	for ( i = 0; i < 20; i++ )
		Spawn_Shard( ent, inflictor, 1, ent->count );

	Prop_Break_Sound( ent );

	ent->takedamage = qfalse;
	ent->die = NULL;

	SV_LinkEntity( &ent->shared );

	G_UseTargets( ent, NULL );

	G_FreeEntity( ent );
}

/*QUAKED props_radioSEVEN (.8 .6 .2) ?
requires an origin brush
health = defaults to 100


  the models dims are
  x 32
  y 136
  z 32

  if you want more explosions you'll need func explosive

  it will fire all its targets upon death
*/
void SP_Props_RadioSEVEN( gentity_t *ent ) {

	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_radio with NULL model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->takedamage = qtrue;

	ent->die = props_radio_dieSEVEN;

	ent->count = 2; // metal shard and sound

	SV_LinkEntity( &ent->shared );

}


void locker_tall_think( gentity_t *ent ) {
	if ( ent->shared.s.frame == 30 ) {
		G_UseTargets( ent, NULL );

	} else
	{
		ent->shared.s.frame++;
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	}

}

void props_locker_tall_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = locker_tall_think;
	ent->nextthink = level.time + FRAMETIME;

	ent->takedamage = qfalse;

	G_UseTargets( ent, NULL );
}

/*QUAKED props_locker_tall (.8 .6 .2) ?
requires an origin brush
*/
void SP_Props_Locker_Tall( gentity_t *ent ) {

	// Ridah, had to add this so I could load castle18dk7
	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_locker_tall with NULL model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->takedamage = qtrue;

	ent->die = props_locker_tall_die;

	SV_LinkEntity( &ent->shared );

}

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
void Props_Chair_Think( gentity_t *self );
void Props_Chair_Touch( gentity_t *self, gentity_t *other, trace_t *trace );
void Props_Chair_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );

void Just_Got_Thrown( gentity_t *self ) {
	float len;
	vec3_t vec;
	qboolean prop_hits = qfalse;

	len = 0;

	if ( self->shared.s.groundEntityNum == -1 ) {
		self->nextthink = level.time + FRAMETIME;

		if ( self->enemy ) {
			gentity_t *player;

			player = AICast_FindEntityForName( "player" );

			if ( player && player != self->enemy ) {
				prop_hits = qtrue;
				G_Damage( self->enemy, self, player, NULL, NULL, 5, 0, MOD_CRUSH );

				self->die = Props_Chair_Die;

				self->die( self, self, NULL, 10, 0 );
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
			gentity_t   *traceEnt;
			gentity_t   *player;
			qboolean reGrab = qtrue;

			VectorCopy( self->shared.r.currentOrigin, end );
			end[2] += 1;

			SV_Trace( &trace, self->shared.r.currentOrigin, self->shared.r.mins, self->shared.r.maxs, end, self->shared.s.number, MASK_SHOT, qfalse );

			traceEnt = &g_entities[ trace.entityNum ];

			if ( trace.startsolid ) {
				player = AICast_FindEntityForName( "player" );

				// only player can catch
				if ( traceEnt != player ) {
					reGrab = qfalse;
				}

				// no catch when dead
				if ( traceEnt->health <= 0 ) {
					reGrab = qfalse;
				}

				// player can throw, then switch to a two handed weapon before catching.
				// need to catch this (no pun intended)
				if ( player->shared.s.weapon && !( WEAPS_ONE_HANDED & ( 1 << ( player->shared.s.weapon ) ) ) ) {
					reGrab = qfalse;
				}

				if ( reGrab ) {
					// pick the chair back up
					self->active = qtrue;
					self->shared.r.ownerNum = player->shared.s.number;
					player->active = qtrue;
					player->melee = self;
					self->nextthink = level.time + 50;

					self->think = Props_Chair_Think;
					self->touch = NULL;
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
		self->die( self, self, NULL, 10, 0 );
	}

}

void Props_TurnLightsOff( gentity_t *self ) {
	if ( !Q_stricmp( self->classname, "props_desklamp" ) ) {
		if ( self->target ) {
			G_UseTargets( self, NULL );
			self->target = NULL;
		}
	}
}

void Props_Activated( gentity_t *self ) {
	vec3_t angles;
	vec3_t dest;
	vec3_t forward, right;
	vec3_t velocity;
	vec3_t prop_ang;

	gentity_t *prop;

	gentity_t   *owner;

	owner = &g_entities[self->shared.r.ownerNum];

	self->nextthink = level.time + 50;

	if ( !owner->client ) {
		return;
	}

	Props_TurnLightsOff( self );

	if ( owner->active == qfalse ) {

		owner->melee = NULL;

		self->physicsObject = qtrue;
		self->physicsBounce = 0.2;

		self->shared.s.groundEntityNum = -1;

		self->shared.s.pos.trType = TR_GRAVITY;
		self->shared.s.pos.trTime = level.time;

		self->active = qfalse;

		G_AddEvent( owner, EV_GENERAL_SOUND, snd_chairthrow );

		AngleVectors( owner->client->ps.viewangles, velocity, NULL, NULL );
		VectorScale( velocity, 250, velocity );
		velocity[2] += 100 + crandom() * 25;
		VectorCopy( velocity, self->shared.s.pos.trDelta );

		self->think = NULL;
		self->nextthink = 0;

		prop = G_Spawn();
		prop->shared.s.modelindex = self->shared.s.modelindex;
		G_SetOrigin( prop, self->shared.r.currentOrigin );

		VectorCopy( owner->client->ps.viewangles, prop_ang );
		prop_ang[0] = 0;

		G_SetAngle( prop, prop_ang );

		prop->clipmask   = CONTENTS_SOLID | CONTENTS_MISSILECLIP;
		prop->shared.r.contents = CONTENTS_SOLID;
		prop->isProp = qtrue;

		VectorSet( prop->shared.r.mins, -12, -12, 0 );
		VectorSet( prop->shared.r.maxs, 12, 12, 48 );

		prop->physicsObject = qtrue;
		prop->physicsBounce = 0.2;

		VectorCopy( owner->client->ps.origin, prop->shared.s.pos.trBase );

		VectorCopy( self->shared.s.pos.trDelta, prop->shared.s.pos.trDelta );

		prop->shared.s.pos.trType = TR_GRAVITY;
		prop->shared.s.pos.trTime = level.time;

		prop->active = qfalse;

		prop->health = self->health;

		prop->duration = self->health;

		prop->count = self->count;

		prop->think = Just_Got_Thrown;
		prop->nextthink = level.time + FRAMETIME;

		prop->takedamage = qtrue;

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

	AngleVectors( angles, forward, right, NULL );
	VectorCopy( owner->shared.r.currentOrigin, dest );

	VectorCopy( dest, self->shared.r.currentOrigin );
	VectorCopy( dest, self->shared.s.pos.trBase );

	self->shared.s.eType = ET_PROP;

	self->shared.s.otherEntityNum = owner->shared.s.number + 1;

	SV_LinkEntity( &self->shared );

}

void Prop_Check_Ground( gentity_t *self );

void Props_Chair_Think( gentity_t *self ) {
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

		SV_Trace( &tr, self->shared.r.currentOrigin, mins, maxs, self->shared.s.pos.trBase, self->shared.s.number, MASK_SHOT, qfalse );

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

		self->physicsObject = qtrue;
		self->physicsBounce = 0.2;

		self->shared.s.pos.trDelta[2] -= 200;

		self->shared.s.pos.trType = TR_GRAVITY;
		self->shared.s.pos.trTime = level.time;

		self->active = qfalse;

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

qboolean Prop_Touch( gentity_t *self, gentity_t *other, vec3_t v ) {

	vec3_t forward;
	vec3_t dest;
	vec3_t angle;
	vec3_t start, end;
	vec3_t mins, maxs;
	trace_t tr;

	if ( !other->client ) {
		return qfalse;
	}

	vectoangles( v, angle );
	angle[0] = 0;
	AngleVectors( angle, forward, NULL, NULL );
	VectorClear( dest );
	VectorMA( dest, 128, forward, dest );
	VectorMA( self->shared.r.currentOrigin, 32, forward, end );

	VectorCopy( self->shared.r.currentOrigin, start );
	end[2] += 8;
	start[2] += 8;

	VectorCopy( self->shared.r.mins, mins );
	VectorCopy( self->shared.r.maxs, maxs );

	mins[2] += 1;

	SV_Trace( &tr, start, mins, maxs, end, self->shared.s.number, MASK_SHOT, qfalse );

	if ( tr.fraction != 1 ) {
		return qfalse;
	}

	VectorCopy( dest, self->shared.s.pos.trDelta );
	VectorCopy( self->shared.r.currentOrigin, self->shared.s.pos.trBase );

	self->shared.s.pos.trDuration = level.time + 100;
	self->shared.s.pos.trTime = level.time;
	self->shared.s.pos.trType = TR_LINEAR;

	self->physicsObject = qtrue;

	return qtrue;
}

void Prop_Check_Ground( gentity_t *self ) {
	vec3_t mins, maxs;
	vec3_t start, end;
	trace_t tr;

	VectorCopy( self->shared.r.currentOrigin, start );
	VectorCopy( self->shared.r.currentOrigin, end );

	end[2] -= 4;

	VectorCopy( self->shared.r.mins, mins );
	VectorCopy( self->shared.r.maxs, maxs );

//	SV_Trace( &tr, start, mins, maxs, end, self->shared.s.number, MASK_SHOT );
	SV_Trace( &tr, start, mins, maxs, end, self->shared.s.number, MASK_MISSILESHOT, qfalse );

	if ( tr.fraction == 1 ) {
		self->shared.s.groundEntityNum = -1;
	} else {
		self->shared.s.groundEntityNum = tr.entityNum;
	}

}

void Props_Chair_Touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	vec3_t v;
	qboolean has_moved;

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
			G_UseTargets( self, NULL );
			self->target = NULL;
		}
	}

}

void Props_Chair_Animate( gentity_t *ent ) {

	ent->touch = NULL;

	if ( !Q_stricmp( ent->classname, "props_chair" ) ) {
		if ( ent->shared.s.frame >= 27 ) {
			ent->shared.s.frame = 27;
			G_UseTargets( ent, NULL );
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
			G_UseTargets( ent, NULL );
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
				G_UseTargets( ent, NULL );
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

void Spawn_Shard( gentity_t *ent, gentity_t *inflictor, int quantity, int type ) {
	gentity_t *sfx;
	vec3_t dir, start;

	VectorCopy( ent->shared.r.currentOrigin, start );

	if ( !Q_stricmp( ent->classname, "props_radioSEVEN" ) ) {
		start[0] += crandom() * 32;
		start[1] += crandom() * 32;
		VectorSubtract( inflictor->shared.r.currentOrigin, ent->shared.r.currentOrigin, dir );
		VectorNormalize( dir );
	} else if ( inflictor )     {
		VectorSubtract( inflictor->shared.r.currentOrigin, ent->shared.r.currentOrigin, dir );
		VectorNormalize( dir );
		VectorNegate( dir, dir );
	} else {
		VectorSet( dir, 0,0,1 );
	}

	sfx = G_Spawn();

	sfx->shared.s.density = type;

	if ( type < 4 ) {
		start[2] += 32;
	}

	G_SetOrigin( sfx, start );
	G_SetAngle( sfx, ent->shared.r.currentAngles );

	G_AddEvent( sfx, EV_SHARD, DirToByte( dir ) );

	sfx->think = G_FreeEntity;

	sfx->nextthink = level.time + 1000;

	sfx->shared.s.frame = quantity;

	SV_LinkEntity( &sfx->shared );
}

void Prop_Break_Sound( gentity_t *ent ) {
	switch ( ent->count )
	{
	case shard_wood:
		G_AddEvent( ent, EV_GENERAL_SOUND, snd_boardbreak );
		break;
	case shard_glass:
		G_AddEvent( ent, EV_GENERAL_SOUND, snd_glassbreak );
		break;
	case shard_metal:
		G_AddEvent( ent, EV_GENERAL_SOUND, snd_metalbreak );
		break;
	case shard_ceramic:
		G_AddEvent( ent, EV_GENERAL_SOUND, snd_ceramicbreak );
		break;
	}
}


void Props_Chair_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	int quantity;
	int type;
	int deathSound;

	// if (ent->active)
	{
		gentity_t *player;

		player = AICast_FindEntityForName( "player" );

		if ( player && player->melee == ent ) {
			player->melee = NULL;
			player->active = qfalse;
			player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;

		} else if ( player && player->shared.s.number == ent->shared.r.ownerNum )     {
			player->active = qfalse;
			player->melee = NULL;
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

//	Prop_Break_Sound (ent);
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

void Props_Chair_Skyboxtouch( gentity_t *ent ) {

	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( player && player->melee == ent ) {
		player->melee = NULL;
		player->active = qfalse;
		player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
	} else if ( player && player->shared.s.number == ent->shared.r.ownerNum )     {
		player->active = qfalse;
		player->melee = NULL;
		player->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
	}

	ent->think = G_FreeEntity;

}

void SP_Props_Chair( gentity_t *ent ) {
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

	ent->isProp = qtrue;

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
	ent->takedamage = qtrue;
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
void SP_Props_GenericChair( gentity_t *ent ) {
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

	ent->isProp     = qtrue;

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
	ent->takedamage = qtrue;
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
void SP_Props_ChairChat( gentity_t *ent ) {
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
void SP_Props_ChairChatArm( gentity_t *ent ) {
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
void SP_Props_ChairSide( gentity_t *ent ) {
	if ( !ent->model ) {
		ent->model = "models/furniture/chair/sidechair3.md3";
	}
	SP_Props_GenericChair( ent );

	ent->count = shard_wood; // wood break sound
}

/*
==============
SP_Props_ChairHiback
==============
*/
void SP_Props_ChairHiback( gentity_t *ent ) {
	if ( !ent->model ) {
		ent->model = "models/furniture/chair/hiback5.md3";
	}
	SP_Props_GenericChair( ent );

	ent->count = shard_wood; // wood break sound
}


// end chairs



void Use_DamageInflictor( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t *daent;

	daent = NULL;
	while ( ( daent = G_Find( daent, FOFS( targetname ), daent->target ) ) != NULL )
	{
		if ( daent == ent ) {
			Com_Printf( "Use_DamageInflictor damaging self.\n" );
		} else
		{
			G_Damage( daent, ent, ent, NULL, NULL, 9999, 0, MOD_CRUSH );
		}
	}

	G_FreeEntity( ent );
}

/*QUAKED props_damageinflictor (.8 .6 .6) (-8 -8 -8) (8 8 8)
this entity when used will cause 9999 damage to all entities it is targeting
then it will be removed
*/
void SP_Props_DamageInflictor( gentity_t *ent ) {
	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;

	ent->use = Use_DamageInflictor;
	SV_LinkEntity( &ent->shared );
}

/*QUAKED props_shard_generator (.8 .5 .1) (-4 -4 -4) (4 4 4)

wait = defaults to 5 how many shards to spawn ( try not to exceed 20 )

shard =
shard_glass = 0,
shard_wood = 1,
shard_metal = 2,
shard_ceramic = 3

*/

void Use_Props_Shard_Generator( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int quantity;
	int type;
	gentity_t *inflictor = NULL;

	type = ent->count;
	quantity = ent->wait;

	inflictor = G_Find( NULL, FOFS( targetname ), ent->target );

	if ( inflictor ) {
		Spawn_Shard( ent, inflictor, quantity, type );
	}

	G_FreeEntity( ent );
}

void SP_props_shard_generator( gentity_t *ent ) {
	G_SetOrigin( ent, ent->shared.s.origin );
	ent->shared.s.eType = ET_GENERAL;
	ent->use = Use_Props_Shard_Generator;

	if ( !ent->count ) {
		ent->count = shard_wood;
	}

	if ( !ent->wait ) {
		ent->wait = 5;
	}

	SV_LinkEntity( &ent->shared );
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
void SP_Props_Desklamp( gentity_t *ent ) {
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

	ent->isProp = qtrue;
	ent->nopickup = qtrue;

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
	ent->takedamage = qtrue;
	SV_LinkEntity( &ent->shared );

	snd_boardbreak = G_SoundIndex( "sound/world/boardbreak.wav" );
	snd_glassbreak = G_SoundIndex( "sound/world/glassbreak.wav" );
	snd_metalbreak = G_SoundIndex( "sound/world/metalbreak.wav" );
	snd_ceramicbreak = G_SoundIndex( "sound/world/ceramicbreak.wav" );
	snd_chaircreak = G_SoundIndex( "sound/world/chaircreak.wav" );
}

/*QUAKED props_flamebarrel (.8 .6 .2) (-13 -13 0) (13 13 40) SMOKING NOLID OIL -
angle will determine which way the lid will fly off when it explodes

when selecting the OIL spawnflag you have the option of giving it a target
this will ensure that the oil sprite will show up where you want it
( be sure to put it on the floor )
the default is in the middle of the barrel on the floor
*/
void Props_Barrel_Touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	return; // barrels cant move

	if ( !( self->spawnflags & 4 ) ) {
		Props_Chair_Touch( self, other, trace );
	}
}

void Props_Barrel_Animate( gentity_t *ent ) {
	float ratio;
	vec3_t v;

	if ( ent->shared.s.frame == 14 ) {
		if ( ent->spawnflags & 1 ) {
			//	G_UseTargets (ent, NULL);
			ent->think = G_FreeEntity;
			ent->nextthink = level.time + 25000;
			return;
		} else
		{
			//	G_UseTargets (ent, NULL);
			ent->think = G_FreeEntity;
			ent->nextthink = level.time + 25000;
			//ent->shared.s.time = level.time;
			//ent->shared.s.time2 = level.time + 2000;
			return;
		}
	} else
	{
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	}

	ent->shared.s.frame++;

	if ( !( ent->spawnflags & 1 ) ) {
		ratio = 2.5;
		VectorSubtract( ent->shared.r.currentOrigin, ent->enemy->shared.r.currentOrigin, v );
		moveit( ent, vectoyaw( v ), ( ent->delay * ratio * FRAMETIME ) * .001 );
	}

}

void barrel_smoke( gentity_t *ent ) {
	gentity_t   *tent;
	vec3_t point;

	VectorCopy( ent->shared.r.currentOrigin, point );

	tent = G_TempEntity( point, EV_SMOKE );
	VectorCopy( point, tent->shared.s.origin );
	tent->shared.s.time = 4000;
	tent->shared.s.time2 = 1000;
	tent->shared.s.density = 0;
	tent->shared.s.angles2[0] = 8;
	tent->shared.s.angles2[1] = 64;
	tent->shared.s.angles2[2] = 50;

}

void smoker_think( gentity_t *ent ) {
	ent->count--;

	if ( !ent->count ) {
		G_FreeEntity( ent );
	} else
	{
		barrel_smoke( ent );
		ent->nextthink = level.time + FRAMETIME;
	}

}

void SP_OilSlick( gentity_t *ent ) {
	gentity_t *tent;
	gentity_t   *target = NULL;
	vec3_t point;

	if ( ent->target ) {
		target = G_Find( NULL, FOFS( targetname ), ent->target );
	}

	if ( target ) {
		VectorCopy( target->shared.s.origin, point );
		point[2] = ent->shared.r.currentOrigin[2]; // just in case
	} else {
		VectorCopy( ent->shared.r.currentOrigin, point );
	}

	tent = G_TempEntity( ent->shared.r.currentOrigin, EV_OILSLICK );
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	tent->shared.s.angles2[0] = 16;
	tent->shared.s.angles2[1] = 48;
	tent->shared.s.angles2[2] = 10000;
	tent->shared.s.density = ent->shared.s.number;

}

void OilParticles_think( gentity_t *ent ) {
	gentity_t *tent;
	gentity_t   *owner;

	owner = &g_entities[ent->shared.s.density];

	if ( owner && owner->takedamage && ent->count2 > level.time - 5000 ) {
		ent->nextthink = ( level.time + FRAMETIME / 2 );

		tent = G_TempEntity( ent->shared.r.currentOrigin, EV_OILPARTICLES );
		VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
		tent->shared.s.time = ent->count2;
		tent->shared.s.density = ent->shared.s.density;
		VectorCopy( ent->rotate, tent->shared.s.origin2 );
	} else {
		G_FreeEntity( ent );
	}
}

void Delayed_Leak_Think( gentity_t *ent ) {
	vec3_t point;
	gentity_t *tent;

	VectorCopy( ent->shared.r.currentOrigin, point );

	tent = G_TempEntity( point, EV_OILSLICK );
	VectorCopy( point, tent->shared.s.origin );

	tent->shared.s.angles2[0] = 0;
	tent->shared.s.angles2[1] = 0;
	tent->shared.s.angles2[2] = 2000;
	tent->shared.s.density = ent->count;
}

qboolean validOilSlickSpawnPoint( vec3_t point, gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t *traceEnt;

	VectorCopy( point, end );
	end[2] -= 9999;

	SV_Trace( &tr, point, NULL, NULL, end, ent->shared.s.number, MASK_SHOT, qfalse );

	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt && traceEnt->classname ) {
		if ( !Q_stricmp( traceEnt->classname, "worldspawn" ) ) {
			if ( tr.plane.normal[0] == 0 && tr.plane.normal[1] == 0 && tr.plane.normal[2] == 1 ) {
				return qtrue;
			}
		}
	}

	return qfalse;

}

void SP_OilParticles( gentity_t *ent ) {
	gentity_t *OilLeak;
	vec3_t point;
	vec3_t vec;
	vec3_t forward;

// Note to self quick fix
// need to move this to client
	return;

	OilLeak = G_Spawn();

	VectorCopy( ent->shared.r.currentOrigin, point );

	point[2] = ent->pos3[2];

	VectorSubtract( ent->pos3, point, vec );
	vectoangles( vec, vec );
	AngleVectors( vec, forward, NULL, NULL );
	VectorMA( point, 12, forward, point );

	G_SetOrigin( OilLeak, point );

	G_SetAngle( OilLeak, ent->shared.r.currentAngles );

	VectorCopy( forward, OilLeak->rotate );

	OilLeak->think = OilParticles_think;
	OilLeak->nextthink = level.time + FRAMETIME;

	OilLeak->shared.s.density = ent->shared.s.number;
	OilLeak->count2 = level.time;

	SV_LinkEntity( &OilLeak->shared );

}


void Props_Barrel_Pain( gentity_t *ent, gentity_t *attacker, int damage, vec3_t point ) {

	if ( ent->health <= 0 ) {
		return;
	}

	if ( !( ent->spawnflags & 8 ) ) {
		SP_OilSlick( ent );
		ent->spawnflags |= 8;
	}

	ent->count2++;

	if ( ent->count2 < 6 ) {
		SP_OilParticles( ent );
	}

}

void OilSlick_remove_think( gentity_t *ent ) {
	gentity_t *tent;

	tent = G_TempEntity( ent->shared.r.currentOrigin, EV_OILSLICKREMOVE );
	tent->shared.s.density = ent->shared.s.density;
}

void OilSlick_remove( gentity_t *ent ) {
	gentity_t *remove;

	remove = G_Spawn();
	remove->shared.s.density = ent->shared.s.number;
	remove->think = OilSlick_remove_think;
	remove->nextthink = level.time + 1000;
	VectorCopy( ent->shared.r.currentOrigin, remove->shared.r.currentOrigin );
	SV_LinkEntity( &remove->shared );
}

void Props_Barrel_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	int quantity;
	int type;
	vec3_t dir;
	gentity_t *smoker;

	if ( ent->spawnflags & 1 ) {
		smoker = G_Spawn();
		smoker->nextthink = level.time + FRAMETIME;
		smoker->think = smoker_think;
		smoker->count = 150 + rand() % 100;
		G_SetOrigin( smoker, ent->shared.r.currentOrigin );
		SV_LinkEntity( &smoker->shared );
	}

	G_UseTargets( ent, NULL );

	if ( ent->spawnflags & 4 ) {
		OilSlick_remove( ent );
	}

	ent->health = 100;
	propExplosion( ent );
	ent->health = 0;

	ent->takedamage = qfalse;

	AngleVectors( ent->shared.r.currentAngles, dir, NULL, NULL );
	dir[2] = 1;

	if ( !( ent->spawnflags & 2 ) ) {
		fire_flamebarrel( ent, ent->shared.r.currentOrigin, dir );
	}

	ent->touch = NULL;

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

void Props_OilSlickSlippery( gentity_t *ent ) {
	gentity_t *player;
	vec3_t vec, kvel, dir;
	float len;

	player = AICast_FindEntityForName( "player" );

	if ( player ) {
		VectorSubtract( player->shared.r.currentOrigin, ent->shared.r.currentOrigin, vec );
		len = VectorLength( vec );

		if ( len < 64 && player->shared.s.groundEntityNum != -1 ) {
			len = VectorLength( player->client->ps.velocity );

			if ( len && !( player->client->ps.pm_time ) ) {
				VectorSet( dir, fabs( crandom() ), fabs( crandom() ), 0 );
				VectorScale( dir, 32, kvel );
				VectorAdd( player->client->ps.velocity, kvel, player->client->ps.velocity );

				{
					int t;

					t = 32 * 2;
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

		}
	}
}

void Props_Barrel_Think( gentity_t *self ) {
	self->active = qfalse;
	Props_Chair_Think( self );

	if ( self->spawnflags & 8 ) { // there is an oil slick
		Props_OilSlickSlippery( self );
	}
}

void SP_Props_Flamebarrel( gentity_t *ent ) {
	int mass;

	if ( ent->spawnflags & 4 ) {
		ent->shared.s.modelindex = G_ModelIndex( "models/furniture/barrel/barrel_c.md3" );
	} else if ( ent->spawnflags & 1 ) {
		ent->shared.s.modelindex = G_ModelIndex( "models/furniture/barrel/barrel_d.md3" );
	} else {
		ent->shared.s.modelindex = G_ModelIndex( "models/furniture/barrel/barrel_b.md3" );
	}

	ent->delay = 0; // inherits damage value

	if ( G_SpawnInt( "mass", "5", &mass ) ) {
		ent->wait = mass;
	} else {
		ent->wait = 10;
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType = ET_MOVER;

	ent->isProp = qtrue;
	ent->nopickup = qtrue;

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

	if ( ent->spawnflags & 4 ) {
		ent->pain = Props_Barrel_Pain;
	}

	ent->takedamage = qtrue;
	SV_LinkEntity( &ent->shared );
}

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

void touch_crate_64( gentity_t *self, gentity_t *other, trace_t *trace ) {
	float ratio;
	vec3_t v;

	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 31 ) ) {
		return;
	}

	ratio = 1.5;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

void crate_animate( gentity_t *ent ) {
	if ( ent->shared.s.frame == 17 ) {
		G_UseTargets( ent, NULL );
		ent->think = G_FreeEntity;
		ent->nextthink = level.time + 2000;
		ent->shared.s.time = level.time;
		ent->shared.s.time2 = level.time + 2000;
		return;
	}

	ent->shared.s.frame++;
	ent->nextthink = level.time + ( FRAMETIME / 2 );
}

void crate_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	int quantity;
	int type;

	quantity = ent->wait;
	type = ent->count;

	Spawn_Shard( ent, inflictor, quantity, type );

	ent->takedamage = qfalse;
	ent->think = crate_animate;
	ent->nextthink = level.time + FRAMETIME;
	ent->touch = NULL;

	SV_UnlinkEntity( &ent->shared );

	ent->clipmask   = 0;
	ent->shared.r.contents = 0;
	ent->shared.s.eType = ET_GENERAL;

	SV_LinkEntity( &ent->shared );

}

void SP_crate_64( gentity_t *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/furniture/crate/crate64.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -32, -32, 0 );
	VectorSet( self->shared.r.maxs, 32, 32, 64 );

	self->shared.s.eType = ET_MOVER;

	self->isProp = qtrue;
	self->nopickup = qtrue;
	G_SetOrigin( self, self->shared.s.origin );
	G_SetAngle( self, self->shared.s.angles );

	self->touch = touch_crate_64;
	self->die = crate_die;

	self->takedamage = qtrue;

	if ( !self->health ) {
		self->health = 20;
	}

	if ( !self->count ) {
		self->count = 1;
	}

	if ( !self->wait ) {
		self->wait = 10;
	}

	self->isProp = qtrue;
	self->nopickup = qtrue;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

void SP_crate_32( gentity_t *self ) {
	self->shared.s.modelindex = G_ModelIndex( "models/furniture/crate/crate32.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -16, -16, 0 );
	VectorSet( self->shared.r.maxs, 16, 16, 32 );

	self->shared.s.eType = ET_MOVER;

	self->isProp = qtrue;
	self->nopickup = qtrue;
	G_SetOrigin( self, self->shared.s.origin );
	G_SetAngle( self, self->shared.s.angles );

	self->touch = touch_crate_64;
	self->die = crate_die;

	self->takedamage = qtrue;

	if ( !self->health ) {
		self->health = 20;
	}

	if ( !self->count ) {
		self->count = 1;
	}

	if ( !self->wait ) {
		self->wait = 10;
	}

	self->isProp = qtrue;
	self->nopickup = qtrue;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

//////////////////////////////////////////////

void props_crate32x64_think( gentity_t *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 17 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{
		ent->clipmask = 0;
		ent->shared.r.contents = 0;
		ent->takedamage = qfalse;

		G_UseTargets( ent, NULL );
	}

}

void props_crate32x64_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = props_crate32x64_think;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_Props_Crate32x64( gentity_t *ent ) {

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_crate32x64_die;

	SV_LinkEntity( &ent->shared );
}

/*QUAKED props_flippy_table (.8 .6 .2) ? - - X_AXIS Y_AXIS LEADER
this entity will need a leader and an origin brush
!!!!!!!!!!!!!!
just a reminder to put the origin brush in the proper location for the leader and the
slave so that the table will flip over correctly.
*/

void flippy_table_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	qboolean is_infront;
	gentity_t   *slave;

	// it would be odd to flip a table if your standing on it
	if ( other && other->shared.s.groundEntityNum == ent->shared.s.number ) {
		// Com_Printf ("can't push table over while standing on it\n");
		return;
	}

	ent->use = NULL;

	is_infront = infront( ent, other );

	if ( is_infront ) {
		// need to swap the team leader with the slave
		for ( slave = ent ; slave ; slave = slave->teamchain )
		{
			if ( slave == ent ) {
				continue;
			}

			slave->shared.s.pos.trType = ent->shared.s.pos.trType;
			slave->shared.s.pos.trTime = ent->shared.s.pos.trTime;
			slave->shared.s.pos.trDuration = ent->shared.s.pos.trDuration;
			VectorCopy( ent->shared.s.pos.trBase, slave->shared.s.pos.trBase );
			VectorCopy( ent->shared.s.pos.trDelta, slave->shared.s.pos.trDelta );

			slave->shared.s.apos.trType = ent->shared.s.apos.trType;
			slave->shared.s.apos.trTime = ent->shared.s.apos.trTime;
			slave->shared.s.apos.trDuration = ent->shared.s.apos.trDuration;
			VectorCopy( ent->shared.s.apos.trBase, slave->shared.s.apos.trBase );
			VectorCopy( ent->shared.s.apos.trDelta, slave->shared.s.apos.trDelta );

			slave->think = ent->think;
			slave->nextthink = ent->nextthink;

			VectorCopy( ent->pos1, slave->pos1 );
			VectorCopy( ent->pos2, slave->pos2 );

			slave->speed = ent->speed;

			slave->flags &= ~FL_TEAMSLAVE;
			// make it visible
			SV_LinkEntity( &slave->shared );

			Use_BinaryMover( slave, other, other );
		}

		SV_UnlinkEntity( &ent->shared );
	} else {
		Use_BinaryMover( ent, other, other );
	}

}

void flippy_table_animate( gentity_t *ent ) {
	return;

	if ( ent->shared.s.frame == 9 ) {
		G_UseTargets( ent, NULL );
		ent->think = G_FreeEntity;
		ent->nextthink = level.time + 2000;
	} else
	{
		ent->shared.s.frame++;
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	}
}

void props_flippy_table_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = flippy_table_animate;
	ent->nextthink = level.time + FRAMETIME;

	ent->takedamage = qfalse;

	G_UseTargets( ent, NULL );
}

void props_flippy_blocked( gentity_t *ent, gentity_t *other ) {
	vec3_t velocity;
	vec3_t angles;
	vec3_t kvel;

	// just for now
	float angle = ent->shared.r.currentAngles[YAW];

	if ( other->client ) {
		// shoot the player off of it
		VectorCopy( ent->shared.s.apos.trBase, angles );
		angles[YAW] += angle;
		angles[PITCH] = 0;  // always forward

		AngleVectors( angles, velocity, NULL, NULL );
		VectorScale( velocity, 24, velocity );
		velocity[2] += 100 + crandom() * 50;

		VectorScale( velocity, 32, kvel );
		VectorAdd( other->client->ps.velocity, kvel, other->client->ps.velocity );
	} else if ( other->shared.s.eType == ET_ITEM )     {
		VectorCopy( ent->shared.s.apos.trBase, angles );
		angles[YAW] += angle;
		angles[PITCH] = 0;  // always forward

		AngleVectors( angles, velocity, NULL, NULL );
		VectorScale( velocity, 150, velocity );
		velocity[2] += 300 + crandom() * 50;

		VectorScale( velocity, 8, kvel );
		other->shared.s.pos.trType = TR_GRAVITY;
		other->shared.s.pos.trTime = level.time;
		VectorCopy( kvel, other->shared.s.pos.trDelta );

		other->shared.s.eFlags |= EF_BOUNCE;
	} else
	{
		// just delete it or destroy it
		G_TempEntity( other->shared.s.origin, EV_ITEM_POP );
		G_FreeEntity( other );
		return;
	}
}

void SP_Props_Flipping_Table( gentity_t *ent ) {

	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_Flipping_Table with NULL model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	ent->speed = 500;
	ent->angle = 90;

	// ent->spawnflags |= 8;
	if ( !( ent->spawnflags & 4 ) && !( ent->spawnflags & 8 ) ) {
		Com_Printf( "you forgot to select the X or Y Axis\n" );
	}

	VectorClear( ent->rotate );

	if      ( ent->spawnflags & 4 ) {
		ent->rotate[2] = 1;
	} else if ( ent->spawnflags & 8 ) {
		ent->rotate[0] = 1;
	} else { ent->rotate[1] = 1;}

	ent->spawnflags |= 64; // stay open

	InitMoverRotate( ent );

	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.pos.trBase, ent->shared.r.currentOrigin );
	VectorCopy( ent->shared.s.apos.trBase, ent->shared.r.currentAngles );

	ent->blocked = props_flippy_blocked;

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->wait *= 1000;

	//ent->takedamage = qtrue;

	//ent->die = props_flippy_table_die;

	ent->use = flippy_table_use;

	SV_LinkEntity( &ent->shared );

}


/*QUAKED props_58x112tablew (.8 .6 .2) ?
dimensions are 58 x 112 x 32 (x,y,z)

requires an origin brush

breakable NOT pushable

brushmodel only

  health = default = 10
wait = defaults to 10 how many shards to spawn ( try not to exceed 20 )

shard =
	shard_glass = 0,
	shard_wood = 1,
	shard_metal = 2,
	shard_ceramic = 3

*/

void props_58x112tablew_think( gentity_t *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 16 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{

		ent->clipmask = 0;
		ent->shared.r.contents = 0;

		G_UseTargets( ent, NULL );
	}

}

void props_58x112tablew_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = props_58x112tablew_think;
	ent->nextthink = level.time + FRAMETIME;
	ent->takedamage = qfalse;
}

void SP_Props_58x112tablew( gentity_t *ent ) {

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_58x112tablew_die;

	SV_LinkEntity( &ent->shared );
}

/*QUAKED props_castlebed (.8 .6 .2) ?
dimensions are 112 x 128 x 80 (x,y,z)

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

void props_castlebed_touch( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( other->client->ps.pm_flags & PMF_JUMP_HELD
		 && other->shared.s.groundEntityNum == ent->shared.s.number
		 && !other->client->ps.pm_time ) {
		G_Damage( ent, other, other, NULL, NULL, 1, 0, MOD_CRUSH );

		// TDB: need sound of bed springs for this
		Com_Printf( "SOUND sqweeky\n" );

		other->client->ps.velocity[2] += 250;

		other->client->ps.pm_time = 250;
		other->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

}

void props_castlebed_animate( gentity_t *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 8 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{
		ent->clipmask = 0;
		ent->shared.r.contents = 0;
		G_UseTargets( ent, NULL );
	}
}

void props_castlebed_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = props_castlebed_animate;
	ent->nextthink = level.time + FRAMETIME;
	ent->touch = NULL;
	ent->takedamage = qfalse;

	ent->count = shard_wood;
	Prop_Break_Sound( ent );
}

void SP_props_castlebed( gentity_t *ent ) {
	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 20;
	}

	ent->takedamage = qtrue;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_castlebed_die;
	ent->touch = props_castlebed_touch;

	SV_LinkEntity( &ent->shared );
}

/*QUAKED props_snowGenerator (3 2 7) ? TOGGLE_ON ALWAYS_ON
entity brush need to be targeted to an info notnull this
will determine the direction the snow particles will travel.

speed
gravity
turb

count is the number of snowflurries 3 to 5 would be a good number

duration is how long the effect will last 1 is 1 second
*/

void props_snowGenerator_think( gentity_t *ent ) {
	gentity_t   *tent;
	float high, wide, deep;
	int i;
	vec3_t point;

	if ( !( ent->spawnflags & 1 ) ) {
		return;
	}

	high = ent->shared.r.maxs[2] - ent->shared.r.mins[2];
	wide = ent->shared.r.maxs[1] - ent->shared.r.mins[1];
	deep = ent->shared.r.maxs[0] - ent->shared.r.mins[0];

	for ( i = 0; i < ent->count; i++ )
	{
		VectorCopy( ent->pos1, point );

		// we need to randomize to the extent of the brush
		point[0] += crandom() * ( deep * 0.5 );
		point[1] += crandom() * ( wide * 0.5 );
		point[2] += crandom() * ( high * 0.5 );

		tent = G_TempEntity( point, EV_SNOWFLURRY );
		VectorCopy( point, tent->shared.s.origin );
		VectorCopy( ent->movedir, tent->shared.s.angles );
		tent->shared.s.time = 2000; // life time
		tent->shared.s.time2 = 1000; // alpha fade start
	}

	if ( ent->spawnflags & 2 ) {
		ent->nextthink = level.time + FRAMETIME;
	} else if ( ent->wait < level.time ) {
		ent->nextthink = level.time + FRAMETIME;
	}
}

void props_snowGenerator_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( !( ent->spawnflags & 1 ) ) {
		ent->spawnflags |= 1;
		ent->think = props_snowGenerator_think;
		ent->nextthink = level.time + FRAMETIME;
		ent->wait = level.time + ent->duration;
	} else {
		ent->spawnflags &= ~1;
	}
}

void SP_props_snowGenerator( gentity_t *ent ) {
	vec3_t center;
	gentity_t   *target = NULL;

	SV_SetBrushModel( &ent->shared, ent->model );

	VectorAdd( ent->shared.r.absmin, ent->shared.r.absmax, center );
	VectorScale( center, 0.5, center );

	VectorCopy( center, ent->pos1 );

	if ( !ent->target ) {
		Com_Printf( "snowGenerator at loc %s does not have a target\n", vtos( center ) );
		return;
	} else
	{
		target = G_Find( target, FOFS( targetname ), ent->target );
		if ( !target ) {
			Com_Printf( "error snowGenerator at loc %s does cant find target %s\n", vtos( center ), ent->target );
			return;
		}

		VectorSubtract( target->shared.s.origin, ent->shared.s.origin, ent->movedir );
		VectorNormalize( ent->movedir );
	}

	ent->shared.r.contents = CONTENTS_TRIGGER;
	ent->shared.r.svFlags = SVF_NOCLIENT;

	if ( ent->spawnflags & 1 || ent->spawnflags & 2 ) {
		ent->think = props_snowGenerator_think;
		ent->nextthink = level.time + FRAMETIME;

		if ( ent->spawnflags & 2 ) {
			ent->spawnflags |= 1;
		}
	}

	ent->use = props_snowGenerator_use;

	if ( !( ent->delay ) ) {
		ent->delay = 100;
	} else {
		ent->delay *= 100;
	}

	if ( !( ent->count ) ) {
		ent->count = 32;
	}

	if ( !( ent->duration ) ) {
		ent->duration = 1;
	}

	ent->duration *= 1000;

	SV_LinkEntity( &ent->shared );
}

/////////////////////////////
// FIRES AND EXPLOSION PROPS
/////////////////////////////

/*QUAKED props_FireColumn (.3 .2 .7) (-8 -8 -8) (8 8 8) CORKSCREW SMOKE GRAVITY HALFGRAVITY
this entity will require a target use an infonotnull to specifiy its direction

defaults:
	will leave a flaming trail by default
	will not be affected by gravity

radius = distance flame will corkscrew from origin
speed = default is 900
duration = default is 3 sec

start_size = default is 5
end_size = defaults 7 thru 17
count = defaults 100 thru 500

Pending:
delay before it happens again use trigger_relay for now
assign a model
*/

void propsFireColumnUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t   *tent;
	vec3_t start, dir;

	VectorCopy( ent->shared.s.origin, start );

	AngleVectors( ent->shared.r.currentAngles, dir, NULL, NULL );

	tent = fire_flamebarrel( ent, start, dir );

	if ( !tent ) {
		return;
	}

	if ( ent->spawnflags & 2 ) {
		tent->shared.s.eType = ET_FIRE_COLUMN_SMOKE;
	} else {
		tent->shared.s.eType = ET_FIRE_COLUMN;
	}

	if ( ent->spawnflags & 4 ) {
		tent->shared.s.pos.trType = TR_GRAVITY;
	} else if ( ent->spawnflags & 8 ) {
		tent->shared.s.pos.trType = TR_GRAVITY_LOW;
	} else {
		tent->shared.s.pos.trType = TR_LINEAR;
	}

	if ( ent->spawnflags & 1 ) {
		tent->shared.s.density = ent->radius; // corkscrew effect
	}

	tent->flags |= FL_NODRAW;
	//tent->shared.s.eFlags |= EF_NODRAW;

	// TBD
	// lifetime
	if ( ent->duration ) {
		tent->nextthink = level.time + ent->duration;
	}

	// speed
	if ( ent->speed ) {
		VectorClear( tent->shared.s.pos.trDelta );
		VectorScale( dir, ent->speed + ( crandom() * 100 ), tent->shared.s.pos.trDelta );
		SnapVector( tent->shared.s.pos.trDelta );
		VectorCopy( start, tent->shared.r.currentOrigin );
	}

	if ( ent->start_size ) {
		tent->shared.s.angles[1] = ent->start_size;
	}

	if ( ent->end_size ) {
		tent->shared.s.angles[2] = ent->end_size;
	}

	if ( ent->count ) {
		tent->shared.s.angles[0] = ent->count;
	}

	G_SetAngle( tent, ent->shared.r.currentAngles );
}

void propsFireColumnInit( gentity_t *ent ) {
	gentity_t   *target;
	vec3_t vec;
	vec3_t angles;

	if ( ent->target ) {
		target = G_Find( NULL, FOFS( targetname ), ent->target );
		VectorSubtract( target->shared.s.origin, ent->shared.s.origin, vec );
		vectoangles( vec, angles );
		G_SetAngle( ent, angles );
	} else
	{
		// ok then just up
		VectorSet( vec, 0, 0, 1 );
		vectoangles( vec, angles );
		G_SetAngle( ent, angles );
	}

	if ( ent->duration ) {
		ent->duration = ent->duration * 1000;
	}


}

void SP_propsFireColumn( gentity_t *ent ) {
	G_SetOrigin( ent, ent->shared.s.origin );
	ent->think = propsFireColumnInit;
	ent->nextthink = level.time + FRAMETIME;
	ent->use = propsFireColumnUse;
	SV_LinkEntity( &ent->shared );
}

/*QUAKED props_ExploPart (.3 .5 .7) (-8 -8 -16) (8 8 16)
"model" will load a discreet model
"noise" will load looping sound for the model
"target" point to an infonotnull to specify dir default will be up
"speed"	default to 900

"type"  wood concrete or stone
"count"in the absense of a model count will determine the piece to spawn
for wood:
  it can be one of the following 64 48 32 24 16 8
for concrete:
for stone:
*/
#define EXPLOPARTPIECES 8

void props_ExploPartUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
//	int i, numpieces;
	gentity_t *part;
	vec3_t start, dir;

	VectorCopy( ent->shared.s.origin, start );
	AngleVectors( ent->shared.r.currentAngles, dir, NULL, NULL );

	if ( ent->shared.s.modelindex ) {
		part = fire_flamebarrel( ent, start, dir );
		part->shared.s.modelindex = ent->shared.s.modelindex;
	} else
	{
		Com_Printf( "props_ExploPartUse has not been assigned a model\n" );
		return;
	}

	if ( part ) {
		part->shared.s.pos.trType = TR_GRAVITY;
		part->shared.s.eType = ET_EXPLO_PART;

		G_SetAngle( part, ent->shared.r.currentAngles );

		if ( ent->speed ) {
			VectorClear( part->shared.s.pos.trDelta );
			VectorScale( dir, ent->speed + ( crandom() * 100 ), part->shared.s.pos.trDelta );
			SnapVector( part->shared.s.pos.trDelta );
			VectorCopy( start, part->shared.r.currentOrigin );
		}
	}

	G_UseTargets( ent, NULL );
}

void props_ExploPartInit( gentity_t *ent ) {
	gentity_t *target;
	vec3_t vec, angles;

	if ( ent->target ) {
		target = G_Find( NULL, FOFS( targetname ), ent->target );
		VectorSubtract( target->shared.s.origin, ent->shared.s.origin, vec );
		vectoangles( vec, angles );
		G_SetAngle( ent, angles );
	} else
	{
		// ok then just up
		VectorSet( vec, 0, 0, 1 );
		vectoangles( vec, angles );
		G_SetAngle( ent, angles );
	}
}

void SP_props_ExploPart( gentity_t *ent ) {
	const char *sound;
	const char *type;
//	float	bbox;

	if ( ent->model ) {
		ent->shared.s.modelindex = G_ModelIndex( ent->model );
	}

	G_SpawnString( "type", "wood", &type );

	if ( !Q_stricmp( type,"wood" ) ) {
		if ( ent->count == 64 ) {
			ent->shared.s.modelindex = G_ModelIndex( "models/shards/2x4a.md3" );
		} else if ( ent->count == 48 ) {
			ent->shared.s.modelindex = G_ModelIndex( "models/shards/2x4b.md3" );
		} else if ( ent->count == 32 ) {
			ent->shared.s.modelindex = G_ModelIndex( "models/shards/2x4c.md3" );
		} else if ( ent->count == 24 ) {
			ent->shared.s.modelindex = G_ModelIndex( "models/shards/2x4d.md3" );
		} else if ( ent->count == 16 ) {
			ent->shared.s.modelindex = G_ModelIndex( "models/shards/2x4e.md3" );
		} else if ( ent->count == 8 ) {
			ent->shared.s.modelindex = G_ModelIndex( "models/shards/2x4f.md3" );
		}
	} else if ( !Q_stricmp( type,"concrete" ) )        {
	} else if ( !Q_stricmp( type,"stone" ) )        {
	}

	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->shared.s.loopSound = G_SoundIndex( sound );
	}

	ent->think = props_ExploPartInit;
	ent->nextthink = level.time + FRAMETIME;

	ent->use = props_ExploPartUse;
}

/*QUAKED props_decoration (.6 .7 .7) (-8 -8 0) (8 8 16) STARTINVIS DEBRIS ANIMATE KEEPBLOCK TOUCHACTIVATE LOOPING STARTON
"model2" will specify the model to load

"noise"  the looping sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

you will need to specify the bounding box for the entity
"high"  default is 4
"wide"	default is 4

"frames"	how many frames of animation to play
"loop" when the animation is done start again on this frame
"startonframe" on what frame do you want to start the animation
*/

void props_decoration_animate( gentity_t *ent ) {

	ent->shared.s.frame++;
	ent->shared.s.eType = ET_GENERAL;

	if ( ent->shared.s.frame > ent->count2 ) {
		if ( ent->spawnflags & 32 || ent->spawnflags & 64 ) {
			ent->shared.s.frame = ent->props_frame_state;

			if ( !( ent->spawnflags & 64 ) ) {
				ent->takedamage = qfalse;
			}
		} else
		{
			ent->shared.s.frame = ent->count2;
			ent->takedamage = qfalse;

			return;
		}
	}

	ent->nextthink = level.time + 50;
}

void props_decoration_death( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	if ( !( ent->spawnflags & 8 ) ) {
		ent->clipmask   = 0;
		ent->shared.r.contents = 0;
		ent->shared.s.eType = ET_GENERAL;
		SV_LinkEntity( &ent->shared );
	}

	ent->takedamage = qfalse;

	G_UseTargets( ent, NULL );

	if ( ent->spawnflags & 2 ) {
		Spawn_Shard( ent, inflictor, ent->count, ent->key );
	}

	if ( ent->spawnflags & 4 ) {
		ent->nextthink = level.time + 50;
		ent->think = props_decoration_animate;
		return;
	}

	G_FreeEntity( ent );

}

void Use_props_decoration( gentity_t *ent, gentity_t *self, gentity_t *activator ) {
	if ( ent->spawnflags & 1 ) {
		SV_LinkEntity( &ent->shared );
		ent->spawnflags &= ~1;
	} else if ( ent->spawnflags & 4 )     {
		ent->nextthink = level.time + 50;
		ent->think = props_decoration_animate;
	} else
	{
		SV_UnlinkEntity( &ent->shared );
		ent->spawnflags |= 1;
	}

}

void props_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	if ( self->spawnflags & 16 ) {
		props_decoration_death( self, other, other, 9999, MOD_CRUSH );
	}
}

void SP_props_decoration( gentity_t *ent ) {
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	const char        *sound;
	const char        *type;
	const char        *high;
	const char        *wide;
	const char        *frames;
	float height;
	float width;
	float num_frames;

	const char        *loop;

	const char        *startonframe;

	if ( G_SpawnString( "startonframe", "0", &startonframe ) ) {
		ent->shared.s.frame = atoi( startonframe );
	}

	if ( ent->model2 ) {
		ent->shared.s.modelindex = G_ModelIndex( ent->model2 );
	}

	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->shared.s.loopSound = G_SoundIndex( sound );
	}

	if ( ( ent->spawnflags & 32 ) && G_SpawnString( "loop", "100", &loop ) ) {
		ent->props_frame_state = atoi( loop );
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

	if ( ent->health ) {
		ent->isProp = qtrue;
		ent->takedamage = qtrue;
		ent->die = props_decoration_death;

		G_SpawnString( "type", "wood", &type );
		if ( !Q_stricmp( type,"wood" ) ) {
			ent->key = 1;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			ent->key = 0;
		} else if ( !Q_stricmp( type,"metal" ) )  {
			ent->key = 2;
		} else if ( !Q_stricmp( type,"ceramic" ) ) {
			ent->key = 3;
		} else if ( !Q_stricmp( type, "rubble" ) ) {
			ent->key = 4;
		}

		G_SpawnString( "high", "0", &high );
		height = atof( high );

		if ( !height ) {
			height = 4;
		}

		G_SpawnString( "wide", "0", &wide );
		width = atof( wide );

		if ( !width ) {
			width = 4;
		}

		width /= 2;

		if ( Q_stricmp( ent->classname, "props_decorBRUSH" ) ) {
			VectorSet( ent->shared.r.mins, -width, -width, 0 );
			VectorSet( ent->shared.r.maxs, width, width, height );
		}

		ent->clipmask   = CONTENTS_SOLID;
		ent->shared.r.contents = CONTENTS_SOLID;
		ent->shared.s.eType = ET_MOVER;

		G_SpawnString( "frames", "0", &frames );
		num_frames = atof( frames );

		ent->count2 = num_frames;

		if ( ent->targetname ) {
			ent->use = Use_props_decoration;
		}

		ent->touch = props_touch;

	} else if ( !( ent->health ) && ent->spawnflags & 4 )       {
		G_SpawnString( "frames", "0", &frames );
		num_frames = atof( frames );

		ent->count2 = num_frames;
		ent->use = Use_props_decoration;
	}

	if ( ent->spawnflags & 64 ) {
		ent->nextthink = level.time + 50;
		ent->think = props_decoration_animate;
	}


	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !( ent->spawnflags & 1 ) ) {
		SV_LinkEntity( &ent->shared );
	} else {
		ent->use = Use_props_decoration;
	}

}

/*QUAKED props_decorBRUSH (.6 .7 .7) ? STARTINVIS DEBRIS ANIMATE KEEPBLOCK - LOOPING STARTON
ANIMATE animate on death
STARTON playanimation on death
must have an origin brush

"model2" will specify the model to load

"noise"  the looping sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

"frames"	how many frames of animation to play
"loop" when the animation is done start again on this frame
"startonframe" on what frame do you want to start the animation
*/

void SP_props_decorBRUSH( gentity_t *self ) {

	SV_SetBrushModel( &self->shared, self->model );

	SP_props_decoration( self );

	if ( self->model2 ) {
		self->shared.s.modelindex2 = G_ModelIndex( self->model2 );
	}

}


/*QUAKED props_decoration_scale (.6 .7 .7) (-8 -8 0) (8 8 16) STARTINVIS DEBRIS ANIMATE KEEPBLOCK TOUCHACTIVATE LOOPING STARTON

"modelscale" - Scale multiplier (defaults to 1.0 and scales uniformly)
"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both the "modelscale" is ignored

"model2" will specify the model to load

"noise"  the looping sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

you will need to specify the bounding box for the entity
"high"  default is 4
"wide"	default is 4

"frames"	how many frames of animation to play
"loop" when the animation is done start again on this frame
"startonframe" on what frame do you want to start the animation
*/

void SP_props_decor_Scale( gentity_t *ent ) {

	float scale[3] = {1,1,1};
	vec3_t scalevec;


	SP_props_decoration( ent );

	ent->shared.s.eType        = ET_GAMEMODEL;

	// look for general scaling
	if ( G_SpawnFloat( "modelscale", "1", &scale[0] ) ) {
		scale[2] = scale[1] = scale[0];
	}

	// look for axis specific scaling
	if ( G_SpawnVector( "modelscale_vec", "1 1 1", &scalevec[0] ) ) {
		VectorCopy( scalevec, scale );
	}

	// scale is stored in 'angles2'
	VectorCopy( scale, ent->shared.s.angles2 );

	SV_LinkEntity( &ent->shared );

}

/*QUAKED props_skyportal (.6 .7 .7) (-8 -8 0) (8 8 16)
"fov" for the skybox default is 90
To have the portal sky fogged, enter any of the following values:
"fogcolor" (r g b) (values 0.0-1.0)
"fognear" distance from entity to start fogging
"fogfar" distance from entity that fog is opaque

*/
void SP_skyportal( gentity_t *ent ) {
	const char    *fov;
	vec3_t fogv;    //----(SA)
	int fogn;       //----(SA)
	int fogf;       //----(SA)
	int isfog = 0;      // (SA)

	float fov_x;

	G_SpawnString( "fov", "90", &fov );
	fov_x = atof( fov );

//----(SA)	modified
	isfog += G_SpawnVector( "fogcolor", "0 0 0", fogv );
	isfog += G_SpawnInt( "fognear", "0", &fogn );
	isfog += G_SpawnInt( "fogfar", "300", &fogf );

	SV_SetConfigstring( CS_SKYBOXORG, va( "%.2f %.2f %.2f %.1f %i %.2f %.2f %.2f %i %i", ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], fov_x, (int)isfog, fogv[0], fogv[1], fogv[2], fogn, fogf ) );
//----(SA)	end
}

/*QUAKED props_statue (.6 .3 .2) (-8 -8 0) (8 8 128) HURT DEBRIS ANIMATE KEEPBLOCK
"model2" will specify the model to load

"noise"  the sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

you will need to specify the bounding box for the entity
"high"  default is 4
"wide"	default is 4

"frames"	how many frames of animation to play
"delay"		how long of a delay before damage is inflicted ei. 0.5 sec or 2.7 sec

"damage"	amount of damage to be inflicted
*/

void props_statue_blocked( gentity_t *ent ) {
	trace_t trace;
	vec3_t start, end, mins, maxs;
	vec3_t forward;
	float dist;
	gentity_t   *traceEnt;
	float grav = 128;
	vec3_t kvel;

	if ( !Q_stricmp( ent->classname, "props_statueBRUSH" ) ) {
		return;
	}

	VectorCopy( ent->shared.s.origin, start );
	start[2] += 24;

	VectorSet( mins, ent->shared.r.mins[0], ent->shared.r.mins[1], -23 );
	VectorSet( maxs, ent->shared.r.maxs[0], ent->shared.r.maxs[1], 23 );

	AngleVectors( ent->shared.r.currentAngles, forward, NULL, NULL );

	VectorCopy( start, end );

	dist = ( ( ent->shared.r.maxs[2] + 16 ) / ent->count2 ) * ent->shared.s.frame;

	VectorMA( end, dist, forward, end );

	SV_Trace( &trace, start, mins, maxs, end, ent->shared.s.number, MASK_SHOT, qfalse );

	if ( trace.surfaceFlags & SURF_NOIMPACT ) { // bogus test but just in case
		return;
	}

	traceEnt = &g_entities[ trace.entityNum ];

	if ( traceEnt->takedamage && traceEnt->client ) {
		G_Damage( traceEnt, ent, ent, NULL, trace.endpos, ent->damage, 0, MOD_CRUSH );

		// TBD: push client back a bit
		VectorScale( forward, grav, kvel );
		VectorAdd( traceEnt->client->ps.velocity, kvel, traceEnt->client->ps.velocity );

		if ( !traceEnt->client->ps.pm_time ) {
			int t;

			t = grav * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			traceEnt->client->ps.pm_time = t;
			traceEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}

	} else {
		G_Damage( traceEnt, ent, ent, NULL, trace.endpos, 9999, 0, MOD_CRUSH );
	}

}

void props_statue_animate( gentity_t *ent ) {

	qboolean takeashot = qfalse;

	ent->shared.s.frame++;
	ent->shared.s.eType = ET_GENERAL;

	if ( ent->shared.s.frame > ent->count2 ) {
		ent->shared.s.frame = ent->count2;
		ent->takedamage = qfalse;
	}

	if ( ( ( ent->delay * 1000 ) + ent->timestamp ) > level.time ) {
		ent->count = 0;
	} else if ( ent->count == 5 )     {
		takeashot = qtrue;
		ent->count = 0;
	} else {
		ent->count++;
	}

	if ( takeashot ) {
		props_statue_blocked( ent );
	}

	if ( ent->shared.s.frame < ent->count2 ) {
		ent->nextthink = level.time + 50;
	}
}


void props_statue_death( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {

	ent->timestamp = level.time;

	G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );

	if ( !( ent->spawnflags & 8 ) ) {
		ent->clipmask   = 0;
		ent->shared.r.contents = 0;
		ent->shared.s.eType = ET_GENERAL;
		SV_LinkEntity( &ent->shared );
	}

	ent->takedamage = qfalse;

	G_UseTargets( ent, NULL );

	if ( ent->spawnflags & 2 ) {
		Spawn_Shard( ent, inflictor, ent->count, ent->key );
	}

	if ( ent->spawnflags & 4 ) {
		ent->nextthink = level.time + 50;
		ent->think = props_statue_animate;
		return;
	}

	G_FreeEntity( ent );

}

void props_statue_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	props_statue_death( self, other, other, 9999, MOD_CRUSH );
}

void SP_props_statue( gentity_t *ent ) {
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	const char        *sound;
	const char        *type;
	const char        *high;
	const char        *wide;
	const char        *frames;
	float height;
	float width;
	float num_frames;

	if ( ent->model2 ) {
		ent->shared.s.modelindex = G_ModelIndex( ent->model2 );
	}

	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->noise_index = G_SoundIndex( sound );
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

	ent->isProp = qtrue;
	ent->takedamage = qtrue;
	ent->die = props_statue_death;

	G_SpawnString( "type", "wood", &type );
	if ( !Q_stricmp( type,"wood" ) ) {
		ent->key = 1;
	} else if ( !Q_stricmp( type,"glass" ) )   {
		ent->key = 0;
	} else if ( !Q_stricmp( type,"metal" ) )                                                           {
		ent->key = 2;
	} else if ( !Q_stricmp( type,"ceramic" ) )                                                                                                                   {
		ent->key = 3;
	} else if ( !Q_stricmp( type, "rubble" ) )                                                                                                                                                                             {
		ent->key = 4;
	}

	G_SpawnString( "high", "0", &high );
	height = atof( high );
	if ( !height ) {
		height = 4;
	}

	G_SpawnString( "wide", "0", &wide );
	width = atof( wide );

	if ( !width ) {
		width = 4;
	}

	width /= 2;

	if ( Q_stricmp( ent->classname, "props_statueBRUSH" ) ) {
		VectorSet( ent->shared.r.mins, -width, -width, 0 );
		VectorSet( ent->shared.r.maxs, width, width, height );
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType = ET_MOVER;

	G_SpawnString( "frames", "0", &frames );
	num_frames = atof( frames );

	ent->count2 = num_frames;

	ent->touch = props_statue_touch;


	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !ent->damage ) {
		ent->damage = 1;
	}

	SV_LinkEntity( &ent->shared );
}


/*QUAKED props_statueBRUSH (.6 .3 .2) ? HURT DEBRIS ANIMATE KEEPBLOCK
needs an origin brush

"model2" will specify the model to load

"noise"  the sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

"frames"	how many frames of animation to play
"delay"		how long of a delay before damage is inflicted ei. 0.5 sec or 2.7 sec

THE damage has been disabled at the moment
"damage"	amount of damage to be inflicted

*/

void SP_props_statueBRUSH( gentity_t *self ) {

	SV_SetBrushModel( &self->shared, self->model );

	SP_props_statue( self );

	if ( self->model2 ) {
		self->shared.s.modelindex2 = G_ModelIndex( self->model2 );
	}

	if ( !( self->health ) ) {
		self->health = 6;
	}

}

//////////////////////////////////////////////////
// Lockers
//////////////////////////////////////////////////

#define LOCKER_ANIM_USEEND      5
#define LOCKER_ANIM_DEATHSTART  6
#define LOCKER_ANIM_DEATHEND    11

//////////////////////////////////////////////////
void init_locker( gentity_t *ent );
void props_locker_death( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );
//////////////////////////////////////////////////
#define MAX_LOCKER_DEBRIS       5

int locker_debris_model[MAX_LOCKER_DEBRIS];
//////////////////////////////////////////////////

void Spawn_Junk( gentity_t *ent ) {
	gentity_t *sfx;
	vec3_t dir, start;

	VectorCopy( ent->shared.r.currentOrigin, start );

	start[0] += crandom() * 32;
	start[1] += crandom() * 32;
	start[2] += 16;

	VectorSubtract( start, ent->shared.r.currentOrigin, dir );
	VectorNormalize( dir );

	sfx = G_Spawn();

	G_SetOrigin( sfx, start );
	G_SetAngle( sfx, ent->shared.r.currentAngles );

	G_AddEvent( sfx, EV_JUNK, DirToByte( dir ) );

	sfx->think = G_FreeEntity;

	sfx->nextthink = level.time + 1000;

	SV_LinkEntity( &sfx->shared );
}

/*
==============
props_locker_endrattle
==============
*/
void props_locker_endrattle( gentity_t *ent ) {
	ent->shared.s.frame = 0;   // idle
	ent->think = 0;
	ent->nextthink = 0;
	ent->delay = 0;
}


void props_locker_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( !ent->delay ) {
		ent->shared.s.frame = 1;   // rattle when pain starts
	}
	ent->delay = 1;
	ent->think = props_locker_endrattle;
	ent->nextthink = level.time + 1000; // rattle a sec
}

void props_locker_pain( gentity_t *ent, gentity_t *attacker, int damage, vec3_t point ) {
	props_locker_use( ent, attacker, attacker );
}


void init_locker( gentity_t *ent ) {
	ent->isProp = qtrue;
	ent->takedamage = qtrue;
	ent->delay = 0;

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	// TODO: change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
	ent->shared.s.eType = ET_TRAP;

	ent->shared.s.frame = 0;   // closed animation

	ent->count2 = LOCKER_ANIM_DEATHEND;

	ent->die = props_locker_death;
	ent->use = props_locker_use;    // trying it rattles the lock (could also allow 'waking' from trigger)
	ent->pain = props_locker_pain;

	// drop origin down 8 so the designer can put the box entity on the floor rather than /in/ the floor
	// remove if you get a new model from jason w/ the origin moved up 8
	ent->shared.s.origin[2] -= 8;

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !( ent->health ) ) {
		ent->health = 1;
	}

	SV_LinkEntity( &ent->shared );

}

void props_locker_spawn_item( gentity_t *ent ) {
	gitem_t     *item;
	gentity_t   *drop = NULL;

	item = BG_FindItem( ent->spawnitem );

	if ( !item ) { // empty
		return;
	}

//	drop = Drop_Item (ent, item, 0, qtrue);
	drop = LaunchItem( item, ent->shared.r.currentOrigin, tv( 0, 0, 20 ) );


	if ( !drop ) {
		Com_Printf( "-----> WARNING <-------\n" );
		Com_Printf( "props_locker_spawn_item at %s failed!\n", vtos( ent->shared.r.currentOrigin ) );
	}
}

extern qhandle_t    trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );

void props_locker_mass( gentity_t *ent ) {
	gentity_t   *tent;
	vec3_t start;
	vec3_t dir;

	VectorCopy( ent->shared.r.currentOrigin, start );

	start[0] += crandom() * 32;
	start[1] += crandom() * 32;
	start[2] += 16;

	VectorSubtract( start, ent->shared.r.currentOrigin, dir );
	VectorNormalize( dir );

	tent = G_TempEntity( ent->shared.r.currentOrigin, EV_EFFECT );
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	VectorCopy( dir, tent->shared.s.angles2 );

	tent->shared.s.dl_intensity = 0;

	SV_SetConfigstring( CS_TARGETEFFECT, ent->dl_shader );    //----(SA)	allow shader to be set from entity

	tent->shared.s.frame = ent->key;

	tent->shared.s.eventParm = 8;
	tent->shared.s.density = 100;
}

/*QUAKED props_footlocker (.6 .7 .3) (-12 -21 -12) (12 21 12) ? NO_JUNK
"noise"  the sound entity is to make upon death
the default sounds are:
  "wood"	- "sound/world/boardbreak.wav"
  "glass"	- "sound/world/glassbreak.wav"
  "metal"	- "sound/world/metalbreak.wav"
  "gibs"	- "sound/player/gibsplit1.wav"
  "brick"	- "sound/world/brickfall.wav"
  "stone"	- "sound/world/stonefall.wav"
  "fabric"	- "sound/world/metalbreak.wav"	// (SA) temp

"locknoise" the locked sound to play
"wait"	 denotes how long the wait is going to be before the locked sound is played again default is 1 sec
"health" default is 1

"spawnitem" - will spawn this item upon death use the pickup_name ie. '9mm'

"type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
"mass" - defaults to 75.  This determines how much debris is emitted.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.

"dl_shader" needs to be set the same way as a target_effect

TBD: the spawning of junk still pending and animation when used

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/mapobjects/furniture/footlocker.md3"
*/

/*
==============
props_locker_death
==============
*/
void props_locker_death( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->takedamage = qfalse;
	ent->shared.s.frame = 2;   // opening animation
	ent->think = 0;
	ent->nextthink = 0;

	SV_UnlinkEntity( &ent->shared );
	ent->shared.r.maxs[2] = 11;    // (SA) make the dead bb half height so the item can look like it's sitting inside
	props_locker_spawn_item( ent );
	SV_LinkEntity( &ent->shared );

}


void SP_props_footlocker( gentity_t *self ) {
	const char    *type;
	const char    *sound;
	const char    *locked;
	int mass;

//	SV_SetBrushModel (self, self->model);

	// (SA) if angle is xx or yy, rotate the bounding box 90 deg to match
	// NOTE:	Non axis-aligned orientation not allowed.  It will work, but
	//			the bounding box will not exactly match the model.
	if ( self->shared.s.angles[1] == 90 || self->shared.s.angles[1] == 270 ) {
		VectorSet( self->shared.r.mins, -21, -12, 0 );
		VectorSet( self->shared.r.maxs, 21, 12, 24 );
	} else {
		VectorSet( self->shared.r.mins, -12, -21, 0 );
		VectorSet( self->shared.r.maxs, 12, 21, 24 );
	}

	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/furniture/footlocker.md3" );

	if ( G_SpawnString( "noise", "NOSOUND", &sound ) ) {
		self->noise_index = G_SoundIndex( sound );
	}

	if ( G_SpawnString( "locknoise", "NOSOUND", &locked ) ) {
		self->soundPos1 = G_SoundIndex( locked );
	}

	if ( !( self->wait ) ) {
		self->wait = 1000;
	} else {
		self->wait *= 1000;
	}

	if ( G_SpawnInt( "mass", "75", &mass ) ) {
		self->count = mass;
	} else {
		self->count = 75;
	}

	if ( G_SpawnString( "type", "wood", &type ) ) {
		if ( !Q_stricmp( type,"wood" ) ) {
			self->key = 0;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			self->key = 1;
		} else if ( !Q_stricmp( type,"metal" ) )  {
			self->key = 2;
		} else if ( !Q_stricmp( type,"gibs" ) )   {
			self->key = 3;
		} else if ( !Q_stricmp( type,"brick" ) ) {
			self->key = 4;
		} else if ( !Q_stricmp( type,"rock" ) )  {
			self->key = 5;
		} else if ( !Q_stricmp( type,"fabric" ) )                                                                                                                                                                                                                                                                                         {
			self->key = 6;
		}
	} else {
		self->key = 0;
	}

	self->delay = level.time + self->wait;

	init_locker( self );

}

/*QUAKED props_flamethrower (.6 .7 .3) (-8 -8 -8) (8 8 8) TRACKING NOSOUND
the effect occurs when this entity is used
needs to aim at a info_notnull
"duration" how long the effect is going to last for example 1.2 sec 2.7 sec
"random" how long of a random variance so the effect isnt exactly the same each time for example 1.1 sec or 0.2 sec
"size" valid ranges are 1.0 to 0.1

NOSOUND - silent (duh)
*/
void props_flamethrower_think( gentity_t *ent ) {
	vec3_t vec, angles;
	gentity_t   *target = NULL;

	if ( ent->spawnflags & 1 ) { // tracking
		if ( ent->target ) {
			target = G_Find( NULL, FOFS( targetname ), ent->target );
		}

		if ( !target ) {
//			VectorSet (ent->shared.r.currentAngles, 0, 0, 1);	// (SA) wasn't working
			VectorSet( ent->shared.s.apos.trBase, 0, 0, 1 );
		} else
		{
			VectorSubtract( target->shared.s.origin, ent->shared.s.origin, vec );
			VectorNormalize( vec );
			vectoangles( vec, angles );
//			VectorCopy (angles, ent->shared.r.currentAngles);	// (SA) wasn't working
			VectorCopy( angles, ent->shared.s.apos.trBase );
		}
	}

	if ( ( ent->timestamp + ent->duration ) > level.time ) {
		//G_AddEvent (ent, EV_FLAMETHROWER_EFFECT, 0);
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

void props_flamethrower_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int rval;
	int rnd;

	if ( ent->spawnflags & 2 ) {
		ent->shared.s.eFlags &= ~EF_FIRING;
		ent->spawnflags &= ~2;
		ent->think = NULL;      // (SA) wasn't working
		ent->nextthink = 0;
		return;
	} else
	{
		ent->spawnflags |= 2;
	}

	if ( ent->random ) {
		rval = ent->random * 1000;
		rnd = rand() % rval;
	} else {
		rnd = 0;
	}

	ent->timestamp = level.time + rnd;

	ent->think = props_flamethrower_think;
	ent->nextthink = level.time + 50;

}

void props_flamethrower_init( gentity_t *ent ) {
	gentity_t *target = NULL;
	vec3_t vec;
	vec3_t angles;

	if ( ent->target ) {
		target = G_Find( NULL, FOFS( targetname ), ent->target );
	}

	if ( !target ) {
//		VectorSet (ent->shared.r.currentAngles, 0, 0, 1);	//----(SA)
		VectorSet( ent->shared.s.apos.trBase, 0, 0, 1 );
	} else
	{
		VectorSubtract( target->shared.s.origin, ent->shared.s.origin, vec );
		VectorNormalize( vec );
		vectoangles( vec, angles );
//		VectorCopy (angles, ent->shared.r.currentAngles);	//----(SA)
		VectorCopy( angles, ent->shared.s.apos.trBase );
		VectorCopy( angles, ent->shared.s.angles ); // RF, added to fix wierd release build issues
	}

	SV_LinkEntity( &ent->shared );

}

void SP_props_flamethrower( gentity_t *ent ) {
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

	if ( ent->spawnflags & 2 ) { // SILENT
		ent->shared.s.density = 1;
	}

	ent->shared.s.eType = ET_FLAMETHROWER_PROP;
	ent->shared.r.svFlags |= SVF_BROADCAST;
}
