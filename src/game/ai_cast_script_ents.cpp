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

//===========================================================================
//
// Name:			ai_cast_script_ents.c
// Function:		Wolfenstein AI Character Scripting
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_cast.h"
#include "../server/server.h"

/*QUAKED ai_marker (1 0.5 0) (-18 -18 -24) (18 18 48) NODROP
AI marker

NODROP means dont drop it to the ground

"targetname" : identifier for this marker
*/

/*
============
SP_ai_marker
============
*/
extern vec3_t playerMins, playerMaxs;
void SP_ai_marker( gentity_t *ent ) {
	vec3_t dest;
	trace_t tr;
	vec3_t checkMins, checkMaxs;


//----(SA)	move the bounding box for the check in 1 unit on each side so they can butt up against a wall and not startsolid
	VectorCopy( playerMins, checkMins );
	checkMins[0] += 1;
	checkMins[1] += 1;
	VectorCopy( playerMaxs, checkMaxs );
	checkMaxs[0] -= 1;
	checkMaxs[1] -= 1;
//----(SA)	end

	if ( !( ent->spawnflags & 1 ) ) {
		// drop to floor
		ent->shared.r.currentOrigin[2] += 1.0; // fixes QErad -> engine bug?
		VectorSet( dest, ent->shared.r.currentOrigin[0], ent->shared.r.currentOrigin[1], ent->shared.r.currentOrigin[2] - 4096 );
		SV_Trace( &tr, ent->shared.r.currentOrigin, checkMins, checkMaxs, dest, ent->shared.s.number, MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP, false );

		if ( tr.startsolid ) {
			Com_Printf( "WARNING: ai_marker (%s) in solid at %s\n", ent->targetname, vtos( ent->shared.r.currentOrigin ) );
			return;
		}

		G_SetOrigin( ent, tr.endpos );
	}
}

/*QUAKED ai_effect (0.3 0.8 0.2) (-4 -4 -4) (4 4 4)
AI effect entity

"ainame" is the name of the AI character that uses this entity for effects
*/

/*
============
SP_ai_effect
============
*/
void ai_effect_think( gentity_t *ent ) {
	gentity_t *targ;

	// find the client number that uses this entity
	targ = AICast_FindEntityForName( ent->aiName );
	if ( !targ ) {
		// keep waiting until they enter, if they never do, then we have no purpose, therefore no harm can be done
		ent->think = ai_effect_think;
		ent->nextthink = level.time + 200;
		return;
	}

	// make sure the clients can use this association
	ent->shared.s.otherEntityNum = targ->shared.s.number;

	ent->shared.s.eType = ET_AI_EFFECT;
	G_SetOrigin( ent, ent->shared.s.origin );
	SV_LinkEntity( &ent->shared );
	ent->shared.r.svFlags |= SVF_BROADCAST;    // make sure all clients are aware of this entity
}

void SP_ai_effect( gentity_t *ent ) {
	
	ent->think = ai_effect_think;
	ent->nextthink = level.time + 500;
}

//===========================================================

// the wait time has passed, so set back up for another activation
void AICast_trigger_wait( gentity_t *ent ) {
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void AICast_trigger_trigger( gentity_t *ent, gentity_t *activator ) {
	if ( ent->nextthink ) {
		return;     // can't retrigger until the wait is over
	}

	ent->activator = AICast_FindEntityForName( ent->aiName );
	if ( ent->activator ) { // they might be dead
		// trigger the script event
		AICast_ScriptEvent( AICast_GetCastState( ent->activator->shared.s.number ), "trigger", ent->target );
	}

	if ( ent->wait > 0 ) {
		ent->think = AICast_trigger_wait;
		ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	} else {
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEntity;
	}
}

void AICast_Touch_Trigger( gentity_t *self, gentity_t *other, trace_t *trace ) {
	if ( !other->client || ( other->shared.r.svFlags & SVF_CASTAI ) ) {
		return;
	}
	AICast_trigger_trigger( self, other );
}

/*QUAKED ai_trigger (1 0.5 0) ? Startoff
Triggered only by the player touching it
"wait" : Seconds between triggerings, -1 = one time only (default).
"ainame" : name of AI to target (use "player" for the.. player)
"target" : trigger identifier for that AI script
*/
extern void InitTrigger( gentity_t *self );

void ai_trigger_activate( gentity_t *self ) {
	if ( self->shared.r.linked ) {
		return;
	}

	self->use = NULL;
	self->AIScript_AlertEntity = NULL;

	self->touch = AICast_Touch_Trigger;

	InitTrigger( self );
	SV_LinkEntity( &self->shared );
}

void ai_trigger_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	ai_trigger_activate( self );
}

void SP_ai_trigger( gentity_t *ent ) {
	

	G_SpawnFloat( "wait", "-1", &ent->wait );

	if ( !ent->aiName ) {
		Com_Error( ERR_DROP, "ai_trigger without \"ainame\"\n" );
        return;  // Keep linter happy. ERR_DROP does not return
	}
	if ( !ent->target ) {
		Com_Error( ERR_DROP, "ai_trigger without \"target\"\n" );
        return;  // Keep linter happy. ERR_DROP does not return
	}

	if ( ent->spawnflags & 1 ) { // TriggerSpawn
		ent->AIScript_AlertEntity = ai_trigger_activate;
		ent->use = ai_trigger_use;
		SV_UnlinkEntity( &ent->shared );
	} else {
		ai_trigger_activate( ent );
	}
}
