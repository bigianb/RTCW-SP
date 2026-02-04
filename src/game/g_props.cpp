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

void DropToFloorG( GameEntity *ent ) {
	vec3_t dest;
	trace_t tr;

	VectorSet( dest, ent->shared.r.currentOrigin[0], ent->shared.r.currentOrigin[1], ent->shared.r.currentOrigin[2] - 4096 );
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, dest, ent->shared.s.number, MASK_SOLID, false );

	if ( tr.startsolid ) {
		return;
	}

	ent->shared.s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	ent->nextthink = level.time + FRAMETIME;
}

void DropToFloor( GameEntity *ent ) {
	vec3_t dest;
	trace_t tr;

	VectorSet( dest, ent->shared.r.currentOrigin[0], ent->shared.r.currentOrigin[1], ent->shared.r.currentOrigin[2] - 4096 );
	SV_Trace( &tr, ent->shared.r.currentOrigin, ent->shared.r.mins, ent->shared.r.maxs, dest, ent->shared.s.number, MASK_SOLID, false );

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

void moveit( GameEntity *ent, float yaw, float dist ) {
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

	SV_Trace( &tr, ent->shared.r.currentOrigin, mins, maxs, origin, ent->shared.s.number, MASK_SHOT, false );

	if ( ( tr.endpos[0] != origin[0] ) || ( tr.endpos[1] != origin[1] ) ) {
		mins[0] = ent->shared.r.mins[0] - 2.0;
		mins[1] = ent->shared.r.mins[1] - 2.0;
		maxs[0] = ent->shared.r.maxs[0] + 2.0;
		maxs[1] = ent->shared.r.maxs[1] + 2.0;

		SV_Trace( &tr, ent->shared.r.currentOrigin, mins, maxs, origin, ent->shared.s.number, MASK_SHOT, false );
	}

	VectorCopy( tr.endpos, ent->shared.r.currentOrigin );

	VectorCopy( ent->shared.r.currentOrigin, ent->shared.s.pos.trBase );

	SV_LinkEntity( &ent->shared );
}


void Psmoke_think( GameEntity *ent ) {
	GameEntity *tent;

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

void prop_smoke( GameEntity *ent ) {
	GameEntity *Psmoke;

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

void PGUNsparks_use( GameEntity *ent, GameEntity *self, GameEntity *activator ) {
	GameEntity *tent;

	tent = G_TempEntity( ent->shared.r.currentOrigin, EV_GUNSPARKS );
	VectorCopy( ent->shared.r.currentOrigin, tent->shared.s.origin );
	VectorCopy( ent->shared.r.currentAngles, tent->shared.s.angles );
	tent->shared.s.density = ent->health;
	tent->shared.s.angles2[2] = ent->speed;

}

void Psparks_think( GameEntity *ent ) {
	GameEntity   *tent;

//(SA) MOVE TO CLIENT!
	return;
}

void sparks_angles_think( GameEntity *ent ) {

	GameEntity *target = nullptr;
	vec3_t vec;

	if ( ent->target ) {
		target = G_Find( nullptr, FOFS( targetname ), ent->target );
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

void SP_props_sparks( GameEntity *ent ) {

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

void SP_props_gunsparks( GameEntity *ent ) {
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


extern void G_ExplodeMissile( GameEntity *ent );

void propExplosionLarge( GameEntity *ent ) {
	GameEntity *bolt;

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

void propExplosion( GameEntity *ent ) {
	GameEntity *bolt;

	extern void G_ExplodeMissile( GameEntity *ent );
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

void InitProp( GameEntity *ent ) {
	float light;
	vec3_t color;
	bool lightSet, colorSet;
	const char        *sound;

	if ( !Q_stricmp( ent->classname, "props_bench" ) ) {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/bench/bench_sm.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_radio" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/mapobjects/electronics/radio1.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_locker_tall" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/storage/lockertall.md3" );
	} else if ( !Q_stricmp( ent->classname, "props_flippy_table" ) )  {
		ent->shared.s.modelindex2 = G_ModelIndex( "models/furniture/table/woodflip.md3" );
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

	ent->isProp = true;

	ent->moverState = MOVER_POS1;
	ent->shared.s.eType = ET_MOVER;

	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );
}

void props_bench_think( GameEntity *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 28 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{
		ent->clipmask = 0;
		ent->shared.r.contents = 0;
		ent->takedamage = false;

		G_UseTargets( ent, nullptr );
	}

}

void props_bench_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	ent->think = props_bench_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED props_bench (.8 .6 .2) ?
requires an origin brush
health = 10 by default
*/
void SP_Props_Bench( GameEntity *ent ) {

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->takedamage = true;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_bench_die;

	SV_LinkEntity( &ent->shared );
}

void props_radio_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {

	propExplosion( ent );

	ent->takedamage = false;

	G_UseTargets( ent, nullptr );

	G_FreeEntity( ent );
}

/*QUAKED props_radio (.8 .6 .2) ?
requires an origin brush
health = defaults to 100
*/
void SP_Props_Radio( GameEntity *ent ) {

	// Ridah, had to add this so I could load castle18dk7
	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_radio with nullptr model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->takedamage = true;

	ent->die = props_radio_die;

	SV_LinkEntity( &ent->shared );

}


void props_radio_dieSEVEN( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {

	int i;

	propExplosion( ent );

	for ( i = 0; i < 20; i++ )
		Spawn_Shard( ent, inflictor, 1, ent->count );

	Prop_Break_Sound( ent );

	ent->takedamage = false;
	ent->die = nullptr;

	SV_LinkEntity( &ent->shared );

	G_UseTargets( ent, nullptr );

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
void SP_Props_RadioSEVEN( GameEntity *ent ) {

	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_radio with nullptr model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->takedamage = true;

	ent->die = props_radio_dieSEVEN;

	ent->count = 2; // metal shard and sound

	SV_LinkEntity( &ent->shared );

}


void locker_tall_think( GameEntity *ent ) {
	if ( ent->shared.s.frame == 30 ) {
		G_UseTargets( ent, nullptr );

	} else
	{
		ent->shared.s.frame++;
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	}

}

void props_locker_tall_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	ent->think = locker_tall_think;
	ent->nextthink = level.time + FRAMETIME;

	ent->takedamage = false;

	G_UseTargets( ent, nullptr );
}

/*QUAKED props_locker_tall (.8 .6 .2) ?
requires an origin brush
*/
void SP_Props_Locker_Tall( GameEntity *ent ) {

	// Ridah, had to add this so I could load castle18dk7
	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_locker_tall with nullptr model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->takedamage = true;

	ent->die = props_locker_tall_die;

	SV_LinkEntity( &ent->shared );

}


void Spawn_Shard( GameEntity *ent, GameEntity *inflictor, int quantity, int type ) {
	GameEntity *sfx;
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

void Prop_Break_Sound( GameEntity *ent ) {
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


void Use_DamageInflictor( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	GameEntity *daent;

	daent = nullptr;
	while ( ( daent = G_Find( daent, FOFS( targetname ), daent->target ) ) != nullptr )
	{
		if ( daent == ent ) {
			Com_Printf( "Use_DamageInflictor damaging self.\n" );
		} else
		{
			G_Damage( daent, ent, ent, nullptr, nullptr, 9999, 0, MOD_CRUSH );
		}
	}

	G_FreeEntity( ent );
}

/*QUAKED props_damageinflictor (.8 .6 .6) (-8 -8 -8) (8 8 8)
this entity when used will cause 9999 damage to all entities it is targeting
then it will be removed
*/
void SP_Props_DamageInflictor( GameEntity *ent ) {
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

void Use_Props_Shard_Generator( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	int quantity;
	int type;
	GameEntity *inflictor = nullptr;

	type = ent->count;
	quantity = ent->wait;

	inflictor = G_Find( nullptr, FOFS( targetname ), ent->target );

	if ( inflictor ) {
		Spawn_Shard( ent, inflictor, quantity, type );
	}

	G_FreeEntity( ent );
}

void SP_props_shard_generator( GameEntity *ent ) {
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

void props_58x112tablew_think( GameEntity *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 16 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{

		ent->clipmask = 0;
		ent->shared.r.contents = 0;

		G_UseTargets( ent, nullptr );
	}

}

void props_58x112tablew_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	ent->think = props_58x112tablew_think;
	ent->nextthink = level.time + FRAMETIME;
	ent->takedamage = false;
}

void SP_Props_58x112tablew( GameEntity *ent ) {

	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 10;
	}

	ent->takedamage = true;

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

void props_castlebed_touch( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( other->client->ps.pm_flags & PMF_JUMP_HELD
		 && other->shared.s.groundEntityNum == ent->shared.s.number
		 && !other->client->ps.pm_time ) {
		G_Damage( ent, other, other, nullptr, nullptr, 1, 0, MOD_CRUSH );

		// TDB: need sound of bed springs for this
		Com_Printf( "SOUND sqweeky\n" );

		other->client->ps.velocity[2] += 250;

		other->client->ps.pm_time = 250;
		other->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

}

void props_castlebed_animate( GameEntity *ent ) {
	ent->shared.s.frame++;

	if ( ent->shared.s.frame < 8 ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
	} else
	{
		ent->clipmask = 0;
		ent->shared.r.contents = 0;
		G_UseTargets( ent, nullptr );
	}
}

void props_castlebed_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	ent->think = props_castlebed_animate;
	ent->nextthink = level.time + FRAMETIME;
	ent->touch = nullptr;
	ent->takedamage = false;

	ent->count = shard_wood;
	Prop_Break_Sound( ent );
}

void SP_props_castlebed( GameEntity *ent ) {
	SV_SetBrushModel( &ent->shared, ent->model );

	InitProp( ent );

	if ( !ent->health ) {
		ent->health = 20;
	}

	ent->takedamage = true;

	ent->clipmask = CONTENTS_SOLID;

	ent->die = props_castlebed_die;
	ent->touch = props_castlebed_touch;

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

void propsFireColumnUse( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	GameEntity   *tent;
	vec3_t start, dir;

	VectorCopy( ent->shared.s.origin, start );

	AngleVectors( ent->shared.r.currentAngles, dir, nullptr, nullptr );

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

void propsFireColumnInit( GameEntity *ent ) {
	GameEntity   *target;
	vec3_t vec;
	vec3_t angles;

	if ( ent->target ) {
		target = G_Find( nullptr, FOFS( targetname ), ent->target );
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

void SP_propsFireColumn( GameEntity *ent ) {
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

void props_ExploPartUse( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
//	int i, numpieces;
	GameEntity *part;
	vec3_t start, dir;

	VectorCopy( ent->shared.s.origin, start );
	AngleVectors( ent->shared.r.currentAngles, dir, nullptr, nullptr );

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

	G_UseTargets( ent, nullptr );
}

void props_ExploPartInit( GameEntity *ent ) {
	GameEntity *target;
	vec3_t vec, angles;

	if ( ent->target ) {
		target = G_Find( nullptr, FOFS( targetname ), ent->target );
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

void SP_props_ExploPart( GameEntity *ent ) {
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


/*QUAKED props_skyportal (.6 .7 .7) (-8 -8 0) (8 8 16)
"fov" for the skybox default is 90
To have the portal sky fogged, enter any of the following values:
"fogcolor" (r g b) (values 0.0-1.0)
"fognear" distance from entity to start fogging
"fogfar" distance from entity that fog is opaque

*/
void SP_skyportal( GameEntity *ent )
{
	const char    *fov;

	G_SpawnString( "fov", "90", &fov );
	float fov_x = atof( fov );

	vec3_t fogv;
	int isfog = G_SpawnVector( "fogcolor", "0 0 0", fogv );
	int fogn;
	isfog += G_SpawnInt( "fognear", "0", &fogn );
	int fogf; 
	isfog += G_SpawnInt( "fogfar", "300", &fogf );

	SV_SetConfigstring( CS_SKYBOXORG, va( "%.2f %.2f %.2f %.1f %i %.2f %.2f %.2f %i %i", ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], fov_x, (int)isfog, fogv[0], fogv[1], fogv[2], fogn, fogf ) );
}


