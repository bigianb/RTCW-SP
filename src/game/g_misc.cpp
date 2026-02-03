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

#include <algorithm>
#include "../idlib/math/Math.h"
#include "g_local.h"
#include "../server/server.h"


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/


/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp( GameEntity *self ) {
	G_SetOrigin( self, self->shared.s.origin );
}


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( GameEntity *self ) {
	G_FreeEntity( self );
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( GameEntity *self ) {
	G_SetOrigin( self, self->shared.s.origin );
}


/*QUAKED info_notnull_big (1 0 0) (-16 -16 -24) (16 16 32)
info_notnull with a bigger box for ease of positioning
*/
void SP_info_notnull_big( GameEntity *self ) {
	G_SetOrigin( self, self->shared.s.origin );
}



/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point q3map_non-dynamic
Non-displayed light.
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
"q3map_non-dynamic" specifies that this light should not contribute to the world's 'light grid' and therefore will not light dynamic models in the game.(wolf)
*/
void SP_light( GameEntity *self ) {
	G_FreeEntity( self );
}

/*QUAKED lightJunior (0 0.7 0.3) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point
Non-displayed light that only affects dynamic game models, but does not contribute to lightmaps
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
*/
void SP_lightJunior( GameEntity *self ) {
	G_FreeEntity( self );
}



/*
=================================================================================

TELEPORTERS

=================================================================================
*/
void TeleportPlayer( GameEntity *player, vec3_t origin, vec3_t angles ) {
	GameEntity   *tent;

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
	tent->shared.s.clientNum = player->shared.s.clientNum;

	tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
	tent->shared.s.clientNum = player->shared.s.clientNum;

	// unlink to make sure it can't possibly interfere with G_KillBox
	SV_UnlinkEntity( &player->shared );

	VectorCopy( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	G_KillBox( player );

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->shared.s, true );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->shared.r.currentOrigin );


	SV_LinkEntity( &player->shared );
}


/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( GameEntity *ent ) {
}


/*
=================================================================================

	misc_grabber_trap

*/


static int attackDurations[] = {    ( 11 * 1000 ) / 15,
									( 16 * 1000 ) / 15,
									( 16 * 1000 ) / 15 };

static int attackHittimes[] = {     ( 7 * 1000 ) / 15,
									( 6 * 1000 ) / 15,
									( 7 * 1000 ) / 15 };

/*
==============
grabber_think_idle
	think func for the grabber ent to reset to idle if not attacking
==============
*/
void grabber_think_idle( GameEntity *ent ) {
	if ( ent->shared.s.frame > 1 ) {  // non-idle status
		ent->shared.s.frame = rand() % 2;
	}
}

/*
==============
grabber_think_hit
	think func for grabber ent following an attack command
==============
*/
void grabber_think_hit( GameEntity *ent ) {
	G_RadiusDamage( ent->shared.s.pos.trBase, ent, ent->damage, ent->duration, ent, MOD_GRABBER );
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 ); // sound2to1 is the 'pain' sound

	ent->nextthink  = level.time + ( attackDurations[( ent->shared.s.frame ) - 2] - attackHittimes[( ent->shared.s.frame ) - 2] );
	ent->think      = grabber_think_idle;
}

extern void GibEntity( GameEntity * self, int killer ) ;

void grabber_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {

	// FIXME FIXME
	// this is buggy.  the trigger brush entity (ent->enemy) does not free.
	// need to fix.

	GibEntity( ent, 0 );    // use temporarily to show 'death' of entity

	ent->enemy->think = G_FreeEntity;
	ent->enemy->nextthink = level.time + FRAMETIME;

	G_UseTargets( ent, attacker );

	ent->think = G_FreeEntity;
	ent->nextthink = level.time + FRAMETIME;
}




/*
==============
grabber_attack
	direct call to the grabber entity (not a trigger) to call the attack
==============
*/
void grabber_attack( GameEntity *ent ) {
	ent->shared.s.frame    = ( rand() % 3 ) + 2;   // randomly choose an attack sequence

	ent->nextthink  = level.time + attackHittimes[( ent->shared.s.frame ) - 2];
	ent->think      = grabber_think_hit;
}

/*
==============
grabber_close
	touch func for attack distance trigger entity
==============
*/
void grabber_close( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	if ( ent->parent->nextthink > level.time ) {
		return;
	}

	grabber_attack( ent->parent );
}



/*
==============
grabber_pain
	pain func for the grabber entity (not triggers)
==============
*/
void grabber_pain( GameEntity *ent, GameEntity *attacker, int damage, vec3_t point ) {
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 ); // sound2to1 is the 'pain' sound
}


/*
==============
grabber_wake
	ent calling this is the bounding box for the grabber, not the grabber ent itself.
	the grabber ent is 'ent->parent'
==============
*/
void grabber_wake( GameEntity *ent ) {
	GameEntity *parent;

	parent = ent->parent;

	// change the 'a' trigger to the 'b' trigger for grabber attacking
	VectorCopy( parent->shared.s.origin, ent->shared.r.mins );
	VectorCopy( parent->shared.s.origin, ent->shared.r.maxs );

	if ( 1 ) {     // temp fast trigger
		VectorAdd( ent->shared.r.mins, tv( -( ent->random ), -( ent->random ), -( ent->random ) ), ent->shared.r.mins );
		VectorAdd( ent->shared.r.maxs, tv( ent->random, ent->random, ent->random ), ent->shared.r.maxs );
	}

	ent->touch = grabber_close;

	// parent entity: show model/play anim/take damage
	{
		parent->clipmask    = CONTENTS_SOLID;
		parent->shared.r.contents  = CONTENTS_SOLID;
		parent->takedamage  = true;
		parent->active      = true;
		parent->die         = grabber_die;
		parent->pain        = grabber_pain;
		SV_LinkEntity( &parent->shared );

		ent->shared.s.frame        = 5;    // starting position

		// go back to an idle if not attacking immediately
		parent->nextthink   = level.time + FRAMETIME;
		parent->think       = grabber_think_idle;
	}

	G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 ); // soundPos1 is the 'wake' sound
}


/*
==============
grabber_use
	use func for the grabber entity
	if not awake, allow waking by trigger
	if awake, allow attacking by trigger
==============
*/
void grabber_use( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	Com_Printf( "grabber_use: %d\n", level.time );

	if ( !ent->active ) {
		grabber_wake( ent );
	} else {
		grabber_attack( ent );
	}
}

/*
==============
grabber_wake_touch
	touch func for the first 'wake' trigger entity
==============
*/
void grabber_wake_touch( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	grabber_wake( ent );
}


/*QUAKED misc_grabber_trap (1 0 0) (-8 -8 -8) (8 8 8)
fields:
"adist"  - radius of 'wakeup' box.  player passing closer than distance activates grabber (def: 64)
"bdist"  - radius of 'attack' box.  player passing into this gets a swipe.  (def: 32)
"health" - how much damage grabber can take after 'wakeup' (def: 100)
"range"  - when attacking, how far from the origin the grabber can strike (def: 64)
"dmg"    - max damage to give on a successful strike (def: 10)
"wait"   - how long to wait between strikes if the player stays within the 'attack' box (def: see below)

If you do not set a "wait" value, then it will default to the duration of the animations.  (so since the first attack animation is 11 frames long and plays at 15 fps, the default wait after using attack 1 would be 11/15, or 0.73 seconds)

grabber media:
model - "models/misc/grabber/grabber.md3"
wake sound - "models/misc/grabber/grabber_wake.wav"
attack sound - "models/misc/grabber/grabber_attack.wav"
pain sound - "models/misc/grabber/grabber_pain.wav"

The current frames are:
first frame
|   length
	|   looping frames
		|   fps
			|   damage at frame
				|
0   6   6   5   0  (main idle)
5   21  21  7   0  (random idle)
25  11  10  15  7  (attack big swipe)
35  16  0   15  6  (attack small swipe)
50  16  0   15  7  (attack grab)
66  1   1   15  0  (starting position)

*/
void SP_misc_grabber_trap( GameEntity *ent ) {
	int adist, bdist, range;
	GameEntity   *trig;

	// TODO: change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
	ent->shared.s.eType        = ET_TRAP;

	// TODO: make these user assignable?
	ent->shared.s.modelindex   = G_ModelIndex( "models/misc/grabber/grabber.md3" );
	ent->soundPos1      = G_SoundIndex( "models/misc/grabber/grabber_wake.wav" );
	ent->sound1to2      = G_SoundIndex( "models/misc/grabber/grabber_attack.wav" );
	ent->sound2to1      = G_SoundIndex( "models/misc/grabber/grabber_pain.wav" );

	G_SetOrigin( ent, ent->shared.s.origin );
	VectorCopy( ent->shared.s.angles, ent->shared.s.apos.trBase );
	ent->shared.s.apos.trBase[YAW] -= 90;  // adjust for model rotation


	if ( !ent->health ) {
		ent->health = 100;  // default to 100

	}
	if ( !ent->damage ) {
		ent->damage = 10;   // default to 10

	}
	ent->shared.s.frame    = 5;

	ent->use        = grabber_use;  // allow 'waking' from trigger

	VectorSet( ent->shared.r.mins, -12, -12, 0 );   // target area for shooting it after it wakes
	VectorSet( ent->shared.r.maxs, 12, 12, 48 );

	// create the 'a' trigger for waking up the grabber
	trig = ent->enemy = G_Spawn();

	VectorCopy( ent->shared.s.origin, trig->shared.r.mins );
	VectorCopy( ent->shared.s.origin, trig->shared.r.maxs );

	// store attack range in 'duration'
	G_SpawnInt( "range", "64", &range );
	ent->duration = range;

	// store adist/bdist in 'count/random' of the trigger brush ent
	G_SpawnInt( "adist", "64", &adist );
	trig->count = adist;
	G_SpawnInt( "bdist", "32", &bdist );
	trig->random = bdist;

	// just make an even trigger box around the ent (do properly sized/oriented trigger after it's working)
	if ( 1 ) {     // temp fast trigger
		VectorAdd( trig->shared.r.mins, tv( -( trig->count ), -( trig->count ), -( trig->count ) ), trig->shared.r.mins );
		VectorAdd( trig->shared.r.maxs, tv( trig->count, trig->count, trig->count ), trig->shared.r.maxs );
	}

	trig->parent        = ent;
	trig->shared.r.contents    = CONTENTS_TRIGGER;
	trig->shared.r.svFlags     = SVF_NOCLIENT;
	trig->touch         = grabber_wake_touch;
	SV_LinkEntity( &trig->shared );

}

void use_spotlight( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
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


void spotlight_die( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {

	AICast_AudibleEvent( attacker->shared.s.number, self->shared.r.currentOrigin, 1024 ); // loud audible event

	self->shared.s.time2 = level.time;
	self->shared.s.frame   = 1;    // 1 == dead
	self->takedamage = false;;
}

void spotlight_finish_spawning( GameEntity *ent ) {
	if ( ent->spawnflags & 1 ) {   // START_ON
//		ent->active = 0;
		SV_LinkEntity( &ent->shared );
	}

	ent->use        = use_spotlight;
	ent->die        = spotlight_die;
	if ( !ent->health ) {
		ent->health = 1;
	}
	ent->takedamage = true;
	ent->think      = 0;
	ent->nextthink  = 0;
	ent->shared.s.frame    = 0;

	ent->clipmask       = CONTENTS_SOLID;
	ent->shared.r.contents     = CONTENTS_SOLID;

	VectorSet( ent->shared.r.mins, -10, -10, -10 );
	VectorSet( ent->shared.r.maxs, 10, 10, 10 );
}


//----(SA)	added
/*QUAKED misc_spotlight (1 0 0) (-16 -16 -16) (16 16 16) START_ON BACK_AND_FORTH
"target" - .camera (spline) file for light to track.  do not specify file extension.

BACK_AND_FORTH - when end of target spline is hit, reverse direction rather than looping (looping is default)
( /\ not active yet /\ )
*/
//"model" - 'base' model that moves with the light.  Default: "models/mapobjects/light/searchlight_pivot.md3"
void SP_misc_spotlight( GameEntity *ent ) {

	ent->shared.s.eType        = ET_SPOTLIGHT_EF;

	ent->think = spotlight_finish_spawning;
	ent->nextthink = level.time + 100;

	if ( ent->target ) {
		ent->shared.s.density = G_FindConfigstringIndex( ent->target, CS_SPLINES, MAX_SPLINE_CONFIGSTRINGS, true );
	}
}

//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
"modelscale"	scale multiplier (defaults to 1x)
"modelscale_vec"	scale multiplier (defaults to 1 1 1, scales each axis as requested)

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored
*/
void SP_misc_model( GameEntity *ent ) {
	G_FreeEntity( ent );
}


//----(SA)
/*QUAKED misc_gamemodel (1 0 0) (-16 -16 -16) (16 16 16) ORIENT_LOD
md3 placed in the game at runtime (rather than in the bsp)
"model"			arbitrary .md3 file to display
"modelscale"	scale multiplier (defaults to 1x, and scales uniformly)
"modelscale_vec"	scale multiplier (defaults to 1 1 1, scales each axis as requested)
"trunk"			diameter of solid core (used for trace visibility and collision (not ai pathing))
"trunkheight"	height of trunk
ORIENT_LOD - if flagged, the entity will yaw towards the player when the LOD switches

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored

*/
void SP_misc_gamemodel( GameEntity *ent ) {

	float scale[3] = {1,1,1};
	vec3_t scalevec;
	int trunksize, trunkheight;

	ent->shared.s.eType        = ET_GAMEMODEL;
	ent->shared.s.modelindex   = G_ModelIndex( ent->model );

	// look for general scaling
	if ( G_SpawnFloat( "modelscale", "1", &scale[0] ) ) {
		scale[2] = scale[1] = scale[0];
	}

	// look for axis specific scaling
	if ( G_SpawnVector( "modelscale_vec", "1 1 1", &scalevec[0] ) ) {
		VectorCopy( scalevec, scale );
	}

	G_SpawnInt( "trunk", "0", &trunksize );
	if ( !G_SpawnInt( "trunkhight", "0", &trunkheight ) ) {
		trunkheight = 256;
	}

	if ( trunksize ) {
		float rad;

		ent->clipmask       = CONTENTS_SOLID;
		ent->shared.r.contents     = CONTENTS_SOLID;

		ent->shared.r.svFlags |= SVF_CAPSULE;

		rad = (float)trunksize / 2.0f;
		VectorSet( ent->shared.r.mins, -rad, -rad, 0 );
		VectorSet( ent->shared.r.maxs, rad, rad, trunkheight );
	}

	// scale is stored in 'angles2'
	VectorCopy( scale, ent->shared.s.angles2 );

	G_SetOrigin( ent, ent->shared.s.origin );
	VectorCopy( ent->shared.s.angles, ent->shared.s.apos.trBase );

	if ( ent->spawnflags & 1 ) {
		ent->shared.s.apos.trType = (trType_t)1; // misc_gamemodels (since they have no movement) will use type = 0 for static models, type = 1 for auto-aligning models


	}
	SV_LinkEntity( &ent->shared );

}


void locateMaster( GameEntity *ent ) {
	ent->target_ent = G_Find( nullptr, FOFS( targetname ), ent->target );
	if ( ent->target_ent ) {
		ent->shared.s.otherEntityNum = ent->target_ent->shared.s.number;
	}
}

/*QUAKED misc_vis_dummy (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
For safety, you should have each dummy only point at one entity (however, it's okay to have many dummies pointing at one entity)
*/
void SP_misc_vis_dummy( GameEntity *ent ) {

	if ( !ent->target ) { //----(SA)	added safety check
		Com_Printf( "Couldn't find target for misc_vis_dummy at %s\n", vtos( ent->shared.r.currentOrigin ) );
		G_FreeEntity( ent );
		return;
	}

	ent->shared.r.svFlags |= SVF_VISDUMMY;
	G_SetOrigin( ent, ent->shared.s.origin );
	SV_LinkEntity( &ent->shared );

	ent->think = locateMaster;
	ent->nextthink = level.time + 1000;

}

/*QUAKED misc_vis_dummy_multiple (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
This entity was created to have multiple speakers targeting it
*/
void SP_misc_vis_dummy_multiple( GameEntity *ent ) {
	if ( !ent->targetname ) {
		Com_Printf( "misc_vis_dummy_multiple needs a targetname at %s\n", vtos( ent->shared.r.currentOrigin ) );
		G_FreeEntity( ent );
		return;
	}

	ent->shared.r.svFlags |= SVF_VISDUMMY_MULTIPLE;
	G_SetOrigin( ent, ent->shared.s.origin );
	SV_LinkEntity( &ent->shared );

}


//===========================================================

/*QUAKED misc_light_surface (1 .5 0) (-8 -8 -8) (8 8 8)
The surfaces nearest these entities will be the only surfaces lit by the targeting light
This must be within 64 world units of the surface to be lit!
*/
void SP_misc_light_surface( GameEntity *ent ) {
	G_FreeEntity( ent );
}


/*QUAKED misc_spawner (.3 .7 .8) (-8 -8 -8) (8 8 8)
use the pickup name
  when this entity gets used it will spawn an item
that matches its spawnitem field
e.i.
spawnitem
9mm
*/

void misc_spawner_think( GameEntity *ent ) {

	gitem_t     *item;
	GameEntity   *drop = nullptr;

	item = BG_FindItem( ent->spawnitem );

	drop = Drop_Item( ent, item, 0, false );

	if ( !drop ) {
		Com_Printf( "-----> WARNING <-------\n" );
		Com_Printf( "misc_spawner used at %s failed to drop!\n", vtos( ent->shared.r.currentOrigin ) );
	}

}

void misc_spawner_use( GameEntity *ent, GameEntity *other, GameEntity *activator ) {

	ent->think = misc_spawner_think;
	ent->nextthink = level.time + FRAMETIME;

//	VectorCopy (other->shared.r.currentOrigin, ent->shared.r.currentOrigin);
//	VectorCopy (ent->shared.r.currentOrigin, ent->shared.s.pos.trBase);

//	VectorCopy (other->shared.r.currentAngles, ent->shared.r.currentAngles);

	SV_LinkEntity( &ent->shared );
}

void SP_misc_spawner( GameEntity *ent ) {
	if ( !ent->spawnitem ) {
		Com_Printf( "-----> WARNING <-------\n" );
		Com_Printf( "misc_spawner at loc %s has no spawnitem!\n", vtos( ent->shared.s.origin ) );
		return;
	}

	ent->use = misc_spawner_use;

	SV_LinkEntity( &ent->shared );

}

// (SA) removed dead code 9/7/01

void firetrail_die( GameEntity *ent ) {
	G_FreeEntity( ent );
}

void firetrail_use( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	if ( ent->shared.s.eType == ET_RAMJET ) {
		ent->shared.s.eType = ET_GENERAL;
	} else {
		ent->shared.s.eType = ET_RAMJET;
	}

	SV_LinkEntity( &ent->shared );

}

/*QUAKED misc_tagemitter (.4 .9 .7) (-16 -16 -16) (16 16 16)
This entity must target the script mover it will attach to
'use' to turn on/off
alert entity call to kill it
*/

void tagemitter_die( GameEntity *ent ) {
	G_FreeEntity( ent );
}

void tagemitter_use( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	if ( ent->shared.s.eType == ET_EFFECT3 ) {
		ent->shared.s.eType = ET_GENERAL;
	} else {
		ent->shared.s.eType = ET_EFFECT3;
	}

	SV_LinkEntity( &ent->shared );

}

void misc_tagemitter_finishspawning( GameEntity *ent ) {
	GameEntity *emitter, *parent;

	parent = G_Find( nullptr, FOFS( targetname ), ent->target );
	if ( !parent ) {
		Com_Error( ERR_DROP, "misc_tagemitter: can't find parent script mover with targetname \"%s\"\n", ent->target );
        return; // keep the linter happy, ERR_DROP does not return
	}

	emitter = ent->target_ent;

	emitter->classname = "misc_tagemitter";
	emitter->shared.r.contents = 0;
	emitter->shared.s.eType = ET_GENERAL;
	emitter->tagParent = parent;

	emitter->use = tagemitter_use;
	emitter->AIScript_AlertEntity = tagemitter_die;
	emitter->targetname = ent->targetname;
	G_ProcessTagConnect( emitter, true );
//	SV_LinkEntity( emitter );

	ent->target_ent = nullptr;
}


void SP_misc_tagemitter( GameEntity *ent ) {
	const char *tagName;

	ent->think = misc_tagemitter_finishspawning;    // so it can find it's target
	ent->nextthink = level.time + 100;

	if ( !G_SpawnString( "tag", nullptr, &tagName ) ) {
		Com_Error( ERR_DROP, "misc_tagemitter: no 'tag' specified\n" );
        return; // keep the linter happy, ERR_DROP does not return
	}

	ent->target_ent = G_Spawn();    // spawn the emitter
	ent->target_ent->tagName = (char *)G_Alloc( strlen( tagName ) + 1 );
	Q_strncpyz( (char *)ent->target_ent->tagName, tagName, strlen( tagName ) + 1 );

	ent->tagName = (char *)G_Alloc( strlen( tagName ) + 1 );
	Q_strncpyz( (char *)ent->tagName, tagName, strlen( tagName ) + 1 );

}


/*QUAKED misc_firetrails (.4 .9 .7) (-16 -16 -16) (16 16 16)
This entity must target the plane its going to be attached to

  its use function will turn the fire stream effect on and off

  an alert entity call will kill it
*/

void misc_firetrails_finishspawning( GameEntity *ent ) {
	GameEntity *left, *right, *airplane;

	airplane = G_Find( nullptr, FOFS( targetname ), ent->target );
	if ( !airplane ) {
		Com_Error( ERR_DROP, "can't find airplane with targetname \"%s\" for firetrails", ent->target );
        return; // keep the linter happy, ERR_DROP does not return
	}

	// left fire trail
	left = G_Spawn();
	left->classname = "left_firetrail";
	left->shared.r.contents = 0;
	left->shared.s.eType = ET_RAMJET;
	left->shared.s.modelindex = G_ModelIndex( "models/ammo/rocket/rocket.md3" );
	left->tagParent = airplane;
	left->tagName = "tag_engine1";   // tag to connect to
	left->use = firetrail_use;
	left->AIScript_AlertEntity = firetrail_die;
	left->targetname = ent->targetname;
	G_ProcessTagConnect( left, true );
	SV_LinkEntity( &left->shared );

	// right fire trail
	right = G_Spawn();
	right->classname = "right_firetrail";
	right->shared.r.contents = 0;
	right->shared.s.eType = ET_RAMJET;
	right->shared.s.modelindex = G_ModelIndex( "models/ammo/rocket/rocket.md3" );
	right->tagParent = airplane;
	right->tagName = "tag_engine2";  // tag to connect to
	right->use = firetrail_use;
	right->AIScript_AlertEntity = firetrail_die;
	right->targetname = ent->targetname;
	G_ProcessTagConnect( right, true );
	SV_LinkEntity( &right->shared );

}

void SP_misc_firetrails( GameEntity *ent ) {
	ent->think = misc_firetrails_finishspawning;
	ent->nextthink = level.time + 100;

}
