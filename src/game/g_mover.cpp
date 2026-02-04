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


#include "../idlib/math/Math.h"
#include "g_local.h"
#include "../server/server.h"

const char *hintStrings[] = {
	"",                  // HINT_NONE
	"HINT_NONE",     // actually HINT_FORCENONE, but since this is being specified in the ent, the designer actually means HINT_FORCENONE
	"HINT_PLAYER",
	"HINT_ACTIVATE",
	"HINT_NOACTIVATE",
	"HINT_DOOR",
	"HINT_DOOR_ROTATING",
	"HINT_DOOR_LOCKED",
	"HINT_DOOR_ROTATING_LOCKED",
	"HINT_MG42",
	"HINT_BREAKABLE",
	"HINT_BREAKABLE_BIG",
	"HINT_CHAIR",
	"HINT_ALARM",
	"HINT_HEALTH",
	"HINT_TREASURE",
	"HINT_KNIFE",
	"HINT_LADDER",
	"HINT_BUTTON",
	"HINT_WATER",
	"HINT_CAUTION",
	"HINT_DANGER",
	"HINT_SECRET",
	"HINT_QUESTION",
	"HINT_EXCLAMATION",
	"HINT_CLIPBOARD",
	"HINT_WEAPON",
	"HINT_AMMO",
	"HINT_ARMOR",
	"HINT_POWERUP",
	"HINT_HOLDABLE",
	"HINT_INVENTORY",
	"HINT_SCENARIC",
	"HINT_EXIT",
	"HINT_NOEXIT",
	"HINT_EXIT_FAR",
	"HINT_NOEXIT_FAR",
	"HINT_PLYR_FRIEND",
	"HINT_PLYR_NEUTRAL",
	"HINT_PLYR_ENEMY",
	"HINT_PLYR_UNKNOWN",
	"HINT_BUILD",

	"",                  // HINT_BAD_USER
};

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

void MatchTeam( GameEntity *teamLeader, int moverState, int time );
void Reached_Train( GameEntity *ent );
void Think_BeginMoving( GameEntity *ent );
void Use_Func_Rotate( GameEntity * ent, GameEntity * other, GameEntity * activator );
void Blocked_Door( GameEntity *ent, GameEntity *other );
void Blocked_DoorRotate( GameEntity *ent, GameEntity *other );

typedef struct {
	GameEntity   *ent;
	vec3_t origin;
	vec3_t angles;
	float deltayaw;
} pushed_t;
pushed_t pushed[MAX_GENTITIES], *pushed_p;


GameEntity   *G_TestEntityPosition( GameEntity *ent )
{
	trace_t tr;
	int mask;

	if ( ent->clipmask ) {
		if ( ent->shared.r.contents == CONTENTS_CORPSE ) {
			// corpse aren't important
			return nullptr;
		} else {
			mask = ent->clipmask;
		}
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		SV_TraceCapsule( &tr, ent->client->ps.origin, ent->shared.r.mins, ent->shared.r.maxs, ent->client->ps.origin, ent->shared.s.number, mask );
	} else if ( ent->shared.s.eType == ET_MISSILE ) {
		SV_Trace( &tr, ent->shared.s.pos.trBase, ent->shared.r.mins, ent->shared.r.maxs, ent->shared.s.pos.trBase, ent->shared.r.ownerNum, mask, false );
	} else {
		SV_Trace( &tr, ent->shared.s.pos.trBase, ent->shared.r.mins, ent->shared.r.maxs, ent->shared.s.pos.trBase, ent->shared.s.number, mask, false );
	}

	if ( tr.startsolid ) {
		return &g_entities[ tr.entityNum ];
	}

	return nullptr;
}

void G_TestEntityDropToFloor( GameEntity *ent, float maxdrop ) {
	trace_t tr;
	int mask;
	vec3_t endpos;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		VectorCopy( ent->client->ps.origin, endpos );
	} else {
		VectorCopy( ent->shared.s.pos.trBase, endpos );
	}

	endpos[2] -= maxdrop;
	if ( ent->client ) {
		SV_TraceCapsule( &tr, ent->client->ps.origin, ent->shared.r.mins, ent->shared.r.maxs, endpos, ent->shared.s.number, mask );
	} else {
		SV_Trace( &tr, ent->shared.s.pos.trBase, ent->shared.r.mins, ent->shared.r.maxs, endpos, ent->shared.s.number, mask, false );
	}

	VectorCopy( tr.endpos, ent->shared.s.pos.trBase );
	if ( ent->client ) {
		VectorCopy( tr.endpos, ent->client->ps.origin );
	}
}

void G_TestEntityMoveTowardsPos( GameEntity *ent, vec3_t pos ) {
	trace_t tr;
	int mask;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		SV_TraceCapsule( &tr, ent->client->ps.origin, ent->shared.r.mins, ent->shared.r.maxs, pos, ent->shared.s.number, mask );
	} else {
		SV_Trace( &tr, ent->shared.s.pos.trBase, ent->shared.r.mins, ent->shared.r.maxs, pos, ent->shared.s.number, mask, false );
	}

	VectorCopy( tr.endpos, ent->shared.s.pos.trBase );
	if ( ent->client ) {
		VectorCopy( tr.endpos, ent->client->ps.origin );
	}
}

void G_CreateRotationMatrix( const vec3_t angles, vec3_t matrix[3] ) {
	AngleVectors( angles, matrix[0], matrix[1], matrix[2] );
	VectorInverse( matrix[1] );
}

// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c in Wolf MP source
void G_TransposeMatrix( /*const*/ vec3_t matrix[3], vec3_t transpose[3] )
{
	for (int i = 0; i < 3; i++ ) {
		for (int j = 0; j < 3; j++ ) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
G_RotatePoint
================
*/
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c in Wolf MP source
void G_RotatePoint( vec3_t point, /*const*/ vec3_t matrix[3] ) {
	vec3_t tvec;

	VectorCopy( point, tvec );
	point[0] = DotProduct( matrix[0], tvec );
	point[1] = DotProduct( matrix[1], tvec );
	point[2] = DotProduct( matrix[2], tvec );
}


/*
==================
G_TryPushingEntity

Returns false if the move is blocked
==================
*/
bool    G_TryPushingEntity( GameEntity *check, GameEntity *pusher, vec3_t move, vec3_t amove ) {
	vec3_t org, org2, move2;
	GameEntity   *block;
	vec3_t matrix[3], transpose[3];
	float x, fx, y, fy, z, fz;
#define JITTER_INC  4
#define JITTER_MAX  ( check->shared.r.maxs[0] / 2.0 )

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->shared.s.eFlags & EF_MOVER_STOP ) &&
		 check->shared.s.groundEntityNum != pusher->shared.s.number ) {
		//pusher->shared.s.eFlags |= EF_MOVER_BLOCKED;
		return false;
	}

	// save off the old position
	if ( pushed_p > &pushed[MAX_GENTITIES] ) {
		Com_Error( ERR_DROP, "pushed_p > &pushed[MAX_GENTITIES]" );
        return false; // keep the linter happy, ERR_DROP does not return
	}
	pushed_p->ent = check;
	VectorCopy( check->shared.s.pos.trBase, pushed_p->origin );
	VectorCopy( check->shared.s.apos.trBase, pushed_p->angles );
	if ( check->client ) {
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy( check->client->ps.origin, pushed_p->origin );
	}
	pushed_p++;

	// try moving the contacted entity
	VectorAdd( check->shared.s.pos.trBase, move, check->shared.s.pos.trBase );
	if ( check->client ) {
		// make sure the client's view rotates when on a rotating mover
		// RF, this is done client-side now
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT( amove[YAW] );
		//
		// RF, AI's need their ideal angle adjusted instead
		if ( check->aiCharacter ) {
			AICast_AdjustIdealYawForMover( check->shared.s.number, ANGLE2SHORT( amove[YAW] ) );
		}
	}

	// figure movement due to the pusher's amove
	G_CreateRotationMatrix( amove, transpose );
	G_TransposeMatrix( transpose, matrix );
	VectorSubtract( check->shared.s.pos.trBase, pusher->shared.r.currentOrigin, org );
	if ( check->client ) {
		VectorSubtract( check->client->ps.origin, pusher->shared.r.currentOrigin, org );
	}
	VectorCopy( org, org2 );
	G_RotatePoint( org2, matrix );
	VectorSubtract( org2, org, move2 );
	VectorAdd( check->shared.s.pos.trBase, move2, check->shared.s.pos.trBase );
	if ( check->client ) {
		VectorAdd( check->client->ps.origin, move, check->client->ps.origin );
		VectorAdd( check->client->ps.origin, move2, check->client->ps.origin );
	}

	// may have pushed them off an edge
	if ( check->shared.s.groundEntityNum != pusher->shared.s.number ) {
		check->shared.s.groundEntityNum = -1;
	}

	block = G_TestEntityPosition( check );
	if ( !block ) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, check->shared.r.currentOrigin );
		} else {
			VectorCopy( check->shared.s.pos.trBase, check->shared.r.currentOrigin );
		}
		return true;
	}

	// RF, if still not valid, move them around to see if we can find a good spot
	if ( JITTER_MAX > JITTER_INC ) {
		VectorCopy( check->shared.s.pos.trBase, org );
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, org );
		}
		for ( z = 0; z < JITTER_MAX; z += JITTER_INC )
			for ( fz = -z; fz <= z; fz += 2 * z ) {
				for ( x = JITTER_INC; x < JITTER_MAX; x += JITTER_INC )
					for ( fx = -x; fx <= x; fx += 2 * x ) {
						for ( y = JITTER_INC; y < JITTER_MAX; y += JITTER_INC )
							for ( fy = -y; fy <= y; fy += 2 * y ) {
								VectorSet( move2, fx, fy, fz );
								VectorAdd( org, move2, org2 );
								VectorCopy( org2, check->shared.s.pos.trBase );
								if ( check->client ) {
									VectorCopy( org2, check->client->ps.origin );
								}
								//
								// do the test
								block = G_TestEntityPosition( check );
								if ( !block ) {
									// pushed ok
									if ( check->client ) {
										VectorCopy( check->client->ps.origin, check->shared.r.currentOrigin );
									} else {
										VectorCopy( check->shared.s.pos.trBase, check->shared.r.currentOrigin );
									}
									return true;
								}
							}
					}
				if ( !fz ) {
					break;
				}
			}
		// didnt work, so set the position back
		VectorCopy( org, check->shared.s.pos.trBase );
		if ( check->client ) {
			VectorCopy( org, check->client->ps.origin );
		}
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( ( pushed_p - 1 )->origin, check->shared.s.pos.trBase );
	if ( check->client ) {
		VectorCopy( ( pushed_p - 1 )->origin, check->client->ps.origin );
	}
	VectorCopy( ( pushed_p - 1 )->angles, check->shared.s.apos.trBase );
	block = G_TestEntityPosition( check );
	if ( !block ) {
		check->shared.s.groundEntityNum = -1;
		pushed_p--;
		return true;
	}

	// blocked
	return false;
}


/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If false is returned, *obstacle will be the blocking entity
============
*/
bool G_MoverPush( GameEntity *pusher, vec3_t move, vec3_t amove, GameEntity **obstacle ) {
	int i, e;
	GameEntity   *check;
	vec3_t mins, maxs;
	pushed_t    *p;
	int entityList[MAX_GENTITIES];
	int moveList[MAX_GENTITIES];
	int listedEntities, moveEntities;
	vec3_t totalMins, totalMaxs;

	*obstacle = nullptr;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->shared.r.currentAngles[0] || pusher->shared.r.currentAngles[1] || pusher->shared.r.currentAngles[2]
		 || amove[0] || amove[1] || amove[2] ) {
		float radius;

		radius = RadiusFromBounds( pusher->shared.r.mins, pusher->shared.r.maxs );
		for ( i = 0; i < 3; i++ ) {
			mins[i] = pusher->shared.r.currentOrigin[i] - radius + move[i];
			maxs[i] = pusher->shared.r.currentOrigin[i] + radius + move[i];
			totalMins[i] = pusher->shared.r.currentOrigin[i] - radius;
			totalMaxs[i] = pusher->shared.r.currentOrigin[i] + radius;
		}
	} else {
		for ( i = 0; i < 3; i++ ) {
			mins[i] = pusher->shared.r.absmin[i] + move[i];
			maxs[i] = pusher->shared.r.absmax[i] + move[i];
		}
		VectorCopy( pusher->shared.r.absmin, totalMins );
		VectorCopy( pusher->shared.r.absmax, totalMaxs );
	}
	for ( i = 0; i < 3; i++ ) {
		if ( move[i] > 0 ) {
			totalMaxs[i] += move[i];
		} else {
			totalMins[i] += move[i];
		}
	}

	// unlink the pusher so we don't get it in the entityList
	SV_UnlinkEntity( &pusher->shared );

	listedEntities = SV_AreaEntities( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to it's final position
	VectorAdd( pusher->shared.r.currentOrigin, move, pusher->shared.r.currentOrigin );
	VectorAdd( pusher->shared.r.currentAngles, amove, pusher->shared.r.currentAngles );
	SV_LinkEntity( &pusher->shared );

	moveEntities = 0;
	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = &g_entities[ entityList[ e ] ];

		if ( check->shared.s.eType == ET_ALARMBOX ) {
			continue;
		}

		if ( check->isProp && check->shared.s.eType == ET_PROP ) {
			continue;
		}

		// only push items and players
		if ( check->shared.s.eType != ET_MISSILE && check->shared.s.eType != ET_ITEM && check->shared.s.eType != ET_PLAYER && !check->physicsObject ) {
			continue;
		}

		if ( check->shared.s.eType == ET_ITEM && check->item->giType == IT_CLIPBOARD && ( check->spawnflags & 1 ) ) {
			continue;
		}

		//if ( check->shared.s.eType == ET_MISSILE && VectorLength( check->shared.s.pos.trDelta ) ) {
		//	continue;	// it's moving
		//}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->shared.s.groundEntityNum != pusher->shared.s.number ) {
			// see if the ent needs to be tested
			if ( check->shared.r.absmin[0] >= maxs[0]
				 || check->shared.r.absmin[1] >= maxs[1]
				 || check->shared.r.absmin[2] >= maxs[2]
				 || check->shared.r.absmax[0] <= mins[0]
				 || check->shared.r.absmax[1] <= mins[1]
				 || check->shared.r.absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if ( G_TestEntityPosition( check ) != pusher ) {
				continue;
			}
		}

		moveList[moveEntities++] = entityList[e];
	}

	// unlink all to be moved entities so they cannot get stuck in each other
	for ( e = 0; e < moveEntities; e++ ) {
		check = &g_entities[ moveList[e] ];

		SV_UnlinkEntity( &check->shared );
	}

	for ( e = 0; e < moveEntities; e++ ) {
		check = &g_entities[ moveList[e] ];

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) ) {
			// link it in now so nothing else tries to clip into us
			SV_LinkEntity( &check->shared );
			continue;
		}

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if ( pusher->shared.s.pos.trType == TR_SINE || pusher->shared.s.apos.trType == TR_SINE ) {
			G_Damage( check, pusher, pusher, nullptr, nullptr, 99999, 0, MOD_CRUSH );
			continue;
		}


		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p = pushed_p - 1 ; p >= pushed ; p-- ) {
			VectorCopy( p->origin, p->ent->shared.s.pos.trBase );
			VectorCopy( p->angles, p->ent->shared.s.apos.trBase );
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				VectorCopy( p->origin, p->ent->client->ps.origin );
			}
		}
		// link all entities at their original position
		for ( e = 0; e < moveEntities; e++ ) {
			check = &g_entities[ moveList[e] ];

			SV_LinkEntity( &check->shared );
		}
		// movement failed
		return false;
	}
	// link all entities at their final position
	for ( e = 0; e < moveEntities; e++ ) {
		check = &g_entities[ moveList[e] ];

		SV_LinkEntity( &check->shared );
	}
	// movement was successfull
	return true;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam( GameEntity *ent ) {
	vec3_t move, amove;
	GameEntity   *part, *obstacle;
	vec3_t origin, angles;

	obstacle = nullptr;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for ( part = ent ; part ; part = part->teamchain ) {
		// get current position
		BG_EvaluateTrajectory( &part->shared.s.pos, level.time, origin );
		BG_EvaluateTrajectory( &part->shared.s.apos, level.time, angles );
		VectorSubtract( origin, part->shared.r.currentOrigin, move );
		VectorSubtract( angles, part->shared.r.currentAngles, amove );

		//if (part->shared.s.eFlags == EF_MOVER_STOP)
		//	part->shared.s.eFlags &= ~EF_MOVER_BLOCKED;

		if ( part->shared.s.eType == ET_BAT && part->model && !G_MoverPush( part, move, amove, &obstacle ) ) {
			break;
		} else if ( !G_MoverPush( part, move, amove, &obstacle ) )    {
			break;  // move was blocked
		}
	}

	if ( part ) {
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain ) {
			part->shared.s.pos.trTime += level.time - level.previousTime;
			part->shared.s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->shared.s.pos, level.time, part->shared.r.currentOrigin );
			BG_EvaluateTrajectory( &part->shared.s.apos, level.time, part->shared.r.currentAngles );
			SV_LinkEntity( &part->shared );
		}

		// if the pusher has a "blocked" function, call it
		if ( ent->blocked ) {
			ent->blocked( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain ) {
		// call the reached function if time is at or past end point

		// opening/closing sliding door (or bats)
		if ( part->shared.s.pos.trType == TR_LINEAR_STOP ) {
			if ( level.time >= part->shared.s.pos.trTime + part->shared.s.pos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
//----(SA)	removed
		// opening or closing rotating door
		else if ( part->shared.s.apos.trType == TR_LINEAR_STOP ) {
			if ( level.time >= part->shared.s.apos.trTime + part->shared.s.apos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover( GameEntity *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		// FIXME
		// hack to fix problem of tram car slaves being linked
		// after being unlinked in G_FindTeams
		if ( ent->shared.r.linked && !Q_stricmp( ent->classname, "func_tramcar" ) ) {
			SV_UnlinkEntity( &ent->shared );
		}
		// Sigh... need to figure out why re links in
		else if ( ent->shared.r.linked && !Q_stricmp( ent->classname, "func_rotating" ) ) {
			SV_UnlinkEntity( &ent->shared );
		}
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->shared.s.pos.trType != TR_STATIONARY || ent->shared.s.apos.trType != TR_STATIONARY ) {
		G_MoverTeam( ent );
	}

	// check think function
	G_RunThink( ent );
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
===============
SetMoverState
===============
*/
void SetMoverState( GameEntity *ent, moverState_t moverState, int time ) {
	vec3_t delta;
	float f;
	bool kicked = false, soft = false;

	kicked = (bool)( ent->flags & FL_KICKACTIVATE );
	soft = (bool)( ent->flags & FL_SOFTACTIVATE );    //----(SA)	added

	if ( ent->flags & FL_DOORNOISE ) { // this door is always 'regular' open
		kicked = false;
		soft = false;
	}

	ent->moverState     = moverState;
	ent->shared.s.pos.trTime   = time;
	ent->shared.s.apos.trTime  = time;
	switch ( moverState ) {
	case MOVER_POS1:
		VectorCopy( ent->pos1, ent->shared.s.pos.trBase );
		ent->shared.s.pos.trType = TR_STATIONARY;
		ent->active = false;
		break;
	case MOVER_POS2:
		VectorCopy( ent->pos2, ent->shared.s.pos.trBase );
		ent->shared.s.pos.trType = TR_STATIONARY;
		break;

		// JOSEPH 1-26-00
	case MOVER_POS3:
		VectorCopy( ent->pos3, ent->shared.s.pos.trBase );
		ent->shared.s.pos.trType = TR_STATIONARY;
		break;

	case MOVER_2TO3:
		VectorCopy( ent->pos2, ent->shared.s.pos.trBase );
		VectorSubtract( ent->pos3, ent->pos2, delta );
		f = 1000.0 / ent->shared.s.pos.trDuration;
		VectorScale( delta, f, ent->shared.s.pos.trDelta );
		ent->shared.s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_3TO2:
		VectorCopy( ent->pos3, ent->shared.s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos3, delta );
		f = 1000.0 / ent->shared.s.pos.trDuration;
		VectorScale( delta, f, ent->shared.s.pos.trDelta );
		ent->shared.s.pos.trType = TR_LINEAR_STOP;
		break;
		// END JOSEPH

	case MOVER_1TO2:        // opening
		VectorCopy( ent->pos1, ent->shared.s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos1, delta );
//----(SA)	numerous changes start here
		ent->shared.s.pos.trDuration = ent->gDuration;
		f = 1000.0 / ent->shared.s.pos.trDuration;
		VectorScale( delta, f, ent->shared.s.pos.trDelta );
		ent->shared.s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:        // closing
		VectorCopy( ent->pos2, ent->shared.s.pos.trBase );
		VectorSubtract( ent->pos1, ent->pos2, delta );
		if ( ent->closespeed ) {                        //----(SA)	handle doors with different close speeds
			ent->shared.s.pos.trDuration = ent->gDurationBack;
			f = 1000.0 / ent->gDurationBack;
		} else {
			ent->shared.s.pos.trDuration = ent->gDuration;
			f = 1000.0 / ent->shared.s.pos.trDuration;
		}
		VectorScale( delta, f, ent->shared.s.pos.trDelta );
		ent->shared.s.pos.trType = TR_LINEAR_STOP;
		break;


	case MOVER_POS1ROTATE:      // at close
		VectorCopy( ent->shared.r.currentAngles, ent->shared.s.apos.trBase );
		ent->shared.s.apos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2ROTATE:      // at open
		VectorCopy( ent->shared.r.currentAngles, ent->shared.s.apos.trBase );
		ent->shared.s.apos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2ROTATE:      // opening
		VectorClear( ent->shared.s.apos.trBase );              // set base to start position {0,0,0}

		if ( kicked ) {
			f = 2000.0 / ent->gDuration;        // double speed when kicked open
			ent->shared.s.apos.trDuration = ent->gDuration / 2.0;
		} else if ( soft ) {
			f = 500.0 / ent->gDuration;         // 1/2 speed when soft opened
			ent->shared.s.apos.trDuration = ent->gDuration * 2;
		} else {
			f = 1000.0 / ent->gDuration;
//				ent->shared.s.apos.trDuration = ent->gDurationBack;	// (SA) durationback?
			ent->shared.s.apos.trDuration = ent->gDuration;
		}
		VectorScale( ent->rotate, f * ent->angle, ent->shared.s.apos.trDelta );
		ent->shared.s.apos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1ROTATE:      // closing
		VectorScale( ent->rotate, ent->angle, ent->shared.s.apos.trBase );     // set base to end position
		// (kicked closes same as normally opened)
		// (soft closes at 1/2 speed)
		f = 1000.0 / ent->gDuration;
		ent->shared.s.apos.trDuration = ent->gDuration;
		if ( soft ) {
			ent->shared.s.apos.trDuration *= 2;
			f *= 0.5f;
		}
		VectorScale( ent->shared.s.apos.trBase, -f, ent->shared.s.apos.trDelta );
		ent->shared.s.apos.trType = TR_LINEAR_STOP;
		ent->active = false;
		break;


	}
	BG_EvaluateTrajectory( &ent->shared.s.pos, level.time, ent->shared.r.currentOrigin );
	if ( !( ent->shared.r.svFlags & SVF_NOCLIENT ) || ( ent->shared.r.contents ) ) {    // RF, added this for bats, but this is safe for all movers, since if they aren't solid, and aren't visible to the client, they don't need to be linked
		SV_LinkEntity( &ent->shared );
		// if this entity is blocking AAS, then update it
		if ( ent->AASblocking && ent->shared.s.pos.trType == TR_STATIONARY ) {
			// reset old blocking areas
			G_SetAASBlockingEntity( ent, false );
			// set new areas
			G_SetAASBlockingEntity( ent, true );
		}
	}
}

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam( GameEntity *teamLeader, int moverState, int time ) {
	GameEntity       *slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {

		// pass along flags for how door was activated
		if ( teamLeader->flags & FL_KICKACTIVATE ) {
			slave->flags |= FL_KICKACTIVATE;
		}
		if ( teamLeader->flags & FL_SOFTACTIVATE ) {
			slave->flags |= FL_SOFTACTIVATE;
		}

		SetMoverState( slave, (moverState_t)moverState, time );
	}
}

/*
MatchTeamReverseAngleOnSlaves

the activator was blocking the door so reverse its direction
*/
void MatchTeamReverseAngleOnSlaves( GameEntity *teamLeader, int moverState, int time ) {
	GameEntity       *slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		// reverse open dir for teamLeader and all slaves
		slave->angle *= -1;

		// pass along flags for how door was activated
		if ( teamLeader->flags & FL_KICKACTIVATE ) {
			slave->flags |= FL_KICKACTIVATE;
		}
		if ( teamLeader->flags & FL_SOFTACTIVATE ) {
			slave->flags |= FL_SOFTACTIVATE;
		}

		SetMoverState( slave, (moverState_t)moverState, time );
	}
}

/*
================
ReturnToPos1
================
*/
void ReturnToPos1( GameEntity *ent ) {
	MatchTeam( ent, MOVER_2TO1, level.time );

	// play starting sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );

	ent->shared.s.loopSound = 0;
	// set looping sound
	ent->shared.s.loopSound = ent->sound3to2;

}

// JOSEPH 1-26-00
/*
================
ReturnToPos2
================
*/
void ReturnToPos2( GameEntity *ent ) {
	MatchTeam( ent, MOVER_3TO2, level.time );

	ent->shared.s.loopSound = 0;
	// looping sound
	ent->shared.s.loopSound = ent->soundLoop;

	// starting sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound3to2 );
}

/*
================
GotoPos3
================
*/
void GotoPos3( GameEntity *ent ) {
	MatchTeam( ent, MOVER_2TO3, level.time );

	ent->shared.s.loopSound = 0;
	// looping sound
	ent->shared.s.loopSound = ent->soundLoop;

	// starting sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to3 );
}
// END JOSEPH

/*
================
ReturnToPos1Rotate
	closing
================
*/
void ReturnToPos1Rotate( GameEntity *ent ) {
	bool inPVS = false;
	GameEntity   *player;

	MatchTeam( ent, MOVER_2TO1ROTATE, level.time );

	player = AICast_FindEntityForName( "player" );

	if ( player ) {
		inPVS = SV_inPVS( player->shared.r.currentOrigin, ent->shared.r.currentOrigin );
	}

	// play starting sound
	if ( inPVS ) {
//		if( (ent->flags & FL_SOFTACTIVATE) &! (ent->flags & FL_DOORNOISE) )
		if ( ( ent->flags & FL_SOFTACTIVATE ) && !( ent->flags & FL_DOORNOISE ) ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftclose );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
	}

	ent->shared.s.loopSound = ent->sound3to2;
}

/*
================
Reached_BinaryMover
================
*/
void Reached_BinaryMover( GameEntity *ent ) {
	bool kicked = false, soft = false;
	// stop the looping sound
	ent->shared.s.loopSound = 0;
//	ent->shared.s.loopSound = ent->soundLoop;

	if ( ent->flags & FL_SOFTACTIVATE ) {
		soft = true;
	}
	if ( ent->flags & FL_KICKACTIVATE ) {
		kicked = true;
	}
	if ( ent->flags & FL_DOORNOISE ) {
		kicked = soft = false;
	}

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		if ( soft ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendo );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}

		G_UseTargets( ent, ent->activator );

		if ( ent->flags & FL_TOGGLE ) {
			ent->active = false;   // enable door activation again
			ent->think = ReturnToPos1;
			ent->nextthink = 0;
			return;
		}

		// JOSEPH 1-27-00
		// return to pos1 after a delay
		if ( ent->wait != -1000 ) {
			ent->think = ReturnToPos1;
			ent->nextthink = level.time + ent->wait;
		}
		// END JOSEPH
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		if ( soft ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendc );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, false );
		}
	} else if ( ent->moverState == MOVER_1TO2ROTATE )   {
		// reached pos2
		SetMoverState( ent, MOVER_POS2ROTATE, level.time );

		// play sound
		if ( kicked ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundKickedEnd );
		} else if ( soft ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendo );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );

		if ( ent->flags & FL_TOGGLE ) {
			ent->active = false;   // enable door activation again
			ent->think = ReturnToPos1Rotate;
			ent->nextthink = 0;
			return;
		}

		if ( ent->wait != -1000 ) {
			// return to pos1 after a delay (if not wait -1)
			ent->think = ReturnToPos1Rotate;
			ent->nextthink = level.time + ent->wait;
		}

	} else if ( ent->moverState == MOVER_2TO1ROTATE )   {
		// reached pos1
		SetMoverState( ent, MOVER_POS1ROTATE, level.time );

		// to stop sound from being requested if not in pvs anoying bug
		{
			bool inPVS = false;
			GameEntity *player;

			player = AICast_FindEntityForName( "player" );

			if ( player ) {
				inPVS = SV_inPVS( player->shared.r.currentOrigin, ent->shared.r.currentOrigin );
			}

			// play sound
			if ( inPVS ) {
				if ( soft ) {
					G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendc );
				} else {
					G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
				}
			}
		}

		// clear the 'soft' flag
		ent->flags &= ~FL_SOFTACTIVATE; //----(SA)	added

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, false );
		}
	} else {
		Com_Error( ERR_DROP, "Reached_BinaryMover: bad moverState" );
        return; // keep the linter happy, ERR_DROP does not return
	}


	ent->flags &= ~FL_KICKACTIVATE; // (SA) it was not opened normally.  Clear this so it thinks it's closed normally

}

bool IsBinaryMoverBlocked( GameEntity *ent, GameEntity *other, GameEntity *activator ) {

	vec3_t dir, angles;
	vec3_t pos;
	vec3_t vec;
	float dot;
	vec3_t forward;
	bool is_relay = false;

	if ( Q_stricmp( ent->classname, "func_door_rotating" ) == 0 ) {
		if ( ent->spawnflags & 32 ) {
			return false;
		}

		//----(SA)	only check for blockage by players
		if ( !activator ) {
			if ( Q_stricmp( other->classname, "target_relay" ) == 0 ) {
				is_relay = true;
			} else if ( !activator->client )      {
				return false;
			}
		}
		//----(SA)	end

		VectorAdd( ent->shared.r.absmin, ent->shared.r.absmax, pos );
		VectorScale( pos, 0.5, pos );

		VectorSubtract( pos, ent->shared.s.origin, dir );
		vectoangles( dir, angles );

		if ( ent->rotate[YAW] ) {
			angles[YAW] += ent->angle;
		} else if ( ent->rotate[PITCH] ) {
			angles[PITCH] += ent->angle;
		} else if ( ent->rotate[ROLL] ) {
			angles[ROLL] += ent->angle;
		}

		AngleVectors( angles, forward, nullptr, nullptr );
		// VectorSubtract (other->shared.r.currentOrigin, pos, vec);

		if ( is_relay ) {
			VectorSubtract( other->shared.r.currentOrigin, pos, vec );
		} else {
			VectorSubtract( activator->shared.r.currentOrigin, pos, vec );
		}

		VectorNormalize( vec );
		dot = DotProduct( vec, forward );

		if ( dot >= 0 ) {
			return true;
		} else {
			return false;
		}

	}

	return false;

}

void Reached_TrinaryMover( GameEntity *ent ) {

	// stop the looping sound
	ent->shared.s.loopSound = ent->soundLoop;

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// goto pos 3
		ent->think = GotoPos3;
		ent->nextthink = level.time + 1000; //FRAMETIME;

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, false );
		}
	} else if ( ent->moverState == MOVER_2TO3 )   {
		// reached pos3
		SetMoverState( ent, MOVER_POS3, level.time );

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );

		// return to pos2 after a delay
		if ( ent->wait != -1000 ) {
			ent->think = ReturnToPos2;
			ent->nextthink = level.time + ent->wait;
		}

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );
	} else if ( ent->moverState == MOVER_3TO2 )   {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// return to pos1
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + 1000; //FRAMETIME;

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
	} else {
		Com_Error( ERR_DROP, "Reached_BinaryMover: bad moverState" );
        return; // keep the linter happy, ERR_DROP does not return
	}
}

void Use_TrinaryMover( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	int total;
	int partial;
	bool isblocked = false;

	isblocked = IsBinaryMoverBlocked( ent, other, activator );

	if ( isblocked ) {
		MatchTeamReverseAngleOnSlaves( ent, MOVER_1TO2ROTATE, level.time + 50 );

		// starting sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );

		// looping sound
		ent->shared.s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, true );
		}
		return;
	}

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {
		Use_TrinaryMover( ent->teammaster, other, activator );
		return;
	}

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );

		// looping sound
		ent->shared.s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, true );
		}
		return;
	}

	if ( ent->moverState == MOVER_POS2 ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_2TO3, level.time + 50 );

		// starting sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to3 );

		// looping sound
		ent->shared.s.loopSound = ent->soundLoop;

		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS3 ) {
		if ( ent->wait != -1000 ) {
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		total = ent->shared.s.pos.trDuration;
		partial = level.time - ent->shared.s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_1TO2, level.time - ( total - partial ) );

		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		return;
	}

	if ( ent->moverState == MOVER_3TO2 ) {
		total = ent->shared.s.pos.trDuration;
		partial = level.time - ent->shared.s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO3, level.time - ( total - partial ) );

		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to3 );
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		total = ent->shared.s.pos.trDuration;
		partial = level.time - ent->shared.s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO1, level.time - ( total - partial ) );

		if ( ent->flags & FL_SOFTACTIVATE ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftclose );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
		return;
	}

	if ( ent->moverState == MOVER_2TO3 ) {
		total = ent->shared.s.pos.trDuration;
		partial = level.time - ent->shared.s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_3TO2, level.time - ( total - partial ) );

		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound3to2 );
		return;
	}
}

void Use_BinaryMover( GameEntity *ent, GameEntity *other, GameEntity *activator ) {

	bool kicked = false, soft = false;
	bool isblocked = false;
	bool nosound = false;

	if ( ent->flags & FL_SOFTACTIVATE ) {
		soft = true;
	}
	if ( ent->flags & FL_KICKACTIVATE ) {
		kicked = true;
	}
	if ( ent->flags & FL_DOORNOISE ) {
		kicked = soft = false;
	}

	if ( level.time <= 4000 ) { // hack.  don't play door sounds if in the first /four/ seconds of game (FIXME: TODO: THIS IS STILL A HACK)
		nosound = true;
	}

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {

		// pass along flags for how door was activated
		if ( kicked ) {
			ent->teammaster->flags |= FL_KICKACTIVATE;
		}
		if ( soft ) {
			ent->teammaster->flags |= FL_SOFTACTIVATE;
		}

		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	// only check for blocking when opening, otherwise the door has no choice
	if ( ent->moverState == MOVER_POS1 || ent->moverState == MOVER_POS1ROTATE ) {
		isblocked = IsBinaryMoverBlocked( ent, other, activator );
	}


	if ( isblocked ) {
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		// ent->angle *= -1;
		// MatchTeam( ent, MOVER_1TO2ROTATE, level.time + 50 );
		MatchTeamReverseAngleOnSlaves( ent, MOVER_1TO2ROTATE, level.time + 50 );

		// starting sound
		if ( !nosound ) {
			if ( kicked ) {    // kicked
				if ( activator ) {
					AICast_AudibleEvent( activator->shared.s.number, ent->shared.s.origin, HEAR_RANGE_DOOR_KICKOPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundKicked );
			} else if ( soft ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftopen );
			} else {
				if ( activator ) {
					AICast_AudibleEvent( activator->shared.s.number, ent->shared.s.origin, HEAR_RANGE_DOOR_OPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
			}
		}

		ent->shared.s.loopSound = 0;

		// looping sound
		if ( !nosound ) {
			ent->shared.s.loopSound = ent->sound2to3;
		} else if ( !nosound ) {
			ent->shared.s.loopSound = ent->soundLoop;
		}

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, true );
		}
		return;
	}

	ent->activator = activator;

	// Rafael
	if ( ent->nextTrain && ent->nextTrain->wait == -1 && ent->nextTrain->count == 1 ) {
		ent->nextTrain->count = 0;
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// play starting sound
		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		ent->shared.s.loopSound = 0;

		// set looping sound
		if ( !nosound ) {
			ent->shared.s.loopSound = ent->sound2to3;
		}

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, true );
		}
		return;
	}

	if ( ent->moverState == MOVER_POS1ROTATE ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2ROTATE, level.time + 50 );

		// play starting sound
		if ( !nosound ) {
			if ( kicked ) {    // kicked
				if ( activator ) {
					AICast_AudibleEvent( activator->shared.s.number, ent->shared.s.origin, HEAR_RANGE_DOOR_KICKOPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundKicked );
			} else if ( soft ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftopen );
			} else {
				if ( activator ) {
					AICast_AudibleEvent( activator->shared.s.number, ent->shared.s.origin, HEAR_RANGE_DOOR_OPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
			}
		}

		ent->shared.s.loopSound = 0;
		// set looping sound
		if ( !nosound ) {
			ent->shared.s.loopSound = ent->sound2to3;
		}

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			SV_AdjustAreaPortalState( &ent->shared, true );
		}
		return;
	}

	// if all the way up, just delay before coming down
	// JOSEPH 1-27-00
	if ( ent->moverState == MOVER_POS2 ) {
		if ( ent->flags & FL_TOGGLE ) {
			ent->nextthink = level.time + 50;
			return;
		}

		if ( ent->wait != -1000 ) {
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}
	// END JOSEPH

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2ROTATE ) {
		if ( ent->flags & FL_TOGGLE ) {
			ent->nextthink = level.time + 50;   // do it *now* for toggles
		} else {
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		Blocked_Door( ent, nullptr );

		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		Blocked_Door( ent, nullptr );

		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
		return;
	}

	// only partway closed before reversing
	if ( ent->moverState == MOVER_2TO1ROTATE ) {
		Blocked_DoorRotate( ent, nullptr );

		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
		return;
	}

	// only partway open before reversing
	if ( ent->moverState == MOVER_1TO2ROTATE ) {
		Blocked_DoorRotate( ent, nullptr );

		if ( !nosound ) {
			if ( soft ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftclose );
			} else {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
			}
		}
		return;
	}
}



/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover( GameEntity *ent ) {
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	bool lightSet, colorSet;
	const char        *sound;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->shared.s.modelindex2 = G_ModelIndex( ent->model2 );
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

	// JOSEPH 1-26-00
	if ( !Q_stricmp( ent->classname,"func_secret" ) ) {
		ent->use = Use_TrinaryMover;
		ent->reached = Reached_TrinaryMover;
	} else if ( !Q_stricmp( ent->classname, "func_rotating" ) )       {
		ent->use = Use_Func_Rotate;
		ent->reached = nullptr; // rotating can never reach
	} else
	{
		ent->use = Use_BinaryMover;
		ent->reached = Reached_BinaryMover;
	}
	// END JOSEPH

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

//----(SA)	changes
	// open time based on speed
//	VectorScale( move, ent->speed, ent->shared.s.pos.trDelta );
	VectorScale( move, ent->speed, ent->gDelta );
	ent->shared.s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->shared.s.pos.trDuration <= 0 ) {
		ent->shared.s.pos.trDuration = 1;
	}
	ent->gDurationBack = ent->gDuration = ent->shared.s.pos.trDuration;

	// close time based on speed
	if ( ent->closespeed ) {
		VectorScale( move, ent->closespeed, ent->gDelta );
		ent->gDurationBack = distance * 1000 / ent->closespeed;
		if ( ent->gDurationBack <= 0 ) {
			ent->gDurationBack = 1;
//----(SA) end
		}
	}
}

/*
================
InitMoverRotate

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMoverRotate( GameEntity *ent ) {
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	bool lightSet, colorSet;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->shared.s.modelindex2 = G_ModelIndex( ent->model2 );
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

	if ( !( ent->spawnflags & 64 ) ) { // STAYOPEN
		ent->reached = Reached_BinaryMover;
	}

	ent->moverState = MOVER_POS1ROTATE;
	ent->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_MOVER;
	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
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

	ent->shared.s.apos.trDuration = ent->speed;
	if ( ent->shared.s.apos.trDuration <= 0 ) {
		ent->shared.s.apos.trDuration = 1;
	}

	ent->gDuration = ent->gDurationBack = ent->shared.s.apos.trDuration;   // (SA) store 'real' durations so doors can be opened/closed at different speeds
}



/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
void Touch_PlatCenterTrigger( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger( GameEntity *ent ) {
	GameEntity   *trigger;
	vec3_t tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->shared.r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[0] = ent->pos1[0] + ent->shared.r.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->shared.r.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->shared.r.mins[2];

	tmax[0] = ent->pos1[0] + ent->shared.r.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->shared.r.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->shared.r.maxs[2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ent->pos1[0] + ( ent->shared.r.mins[0] + ent->shared.r.maxs[0] ) * 0.5;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ent->pos1[1] + ( ent->shared.r.mins[1] + ent->shared.r.maxs[1] ) * 0.5;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy( tmin, trigger->shared.r.mins );
	VectorCopy( tmax, trigger->shared.r.maxs );

	SV_LinkEntity( &trigger->shared );
}


/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat( GameEntity *ent ) {
	float lip, height;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex( "sound/movers/plats/pt1_strt.wav" );
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex( "sound/movers/plats/pt1_end.wav" );

	VectorClear( ent->shared.s.angles );

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	SV_SetBrushModel( &ent->shared, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = ( ent->shared.r.maxs[2] - ent->shared.r.mins[2] ) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->shared.s.origin, ent->pos2 );
	VectorCopy( ent->pos2, ent->pos1 );
	ent->pos1[2] -= height;

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;  // so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger( ent );
	}
}


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button( GameEntity *ent, GameEntity *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ? x x x TOUCH x x STAYOPEN
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button( GameEntity *ent ) {
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;

	ent->sound1to2 = G_SoundIndex( "sound/movers/switches/butn2.wav" );

	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( ent->shared.s.origin, ent->pos1 );

	// calculate second position
	SV_SetBrushModel( &ent->shared, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( ent->shared.s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->shared.r.maxs, ent->shared.r.mins, size );
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	if ( ent->health ) {
		// shootable button
		ent->takedamage = true;
	} else if ( ent->spawnflags & 8 ) {
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover( ent );
}



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON      1
#define TRAIN_TOGGLE        2
#define TRAIN_BLOCK_STOPS   4

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( GameEntity *ent ) {
	ent->shared.s.pos.trTime = level.time;
	ent->shared.s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/
void Reached_Train( GameEntity *ent ) {
	GameEntity       *next;
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
		return;
	}

	// fire all other targets
	G_UseTargets( next, nullptr );

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

	if ( !strcmp( ent->classname, "func_bats" ) && next->radius ) {
		ent->radius = next->radius;
	}

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	ent->shared.s.pos.trDuration = length * 1000 / speed;
	ent->gDuration = ent->shared.s.pos.trDuration;

	// looping sound
	ent->shared.s.loopSound = next->soundLoop;

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->shared.s.pos.trType = TR_STATIONARY;
	}
}


/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( GameEntity *ent ) {
	GameEntity       *path, *next, *start;

	ent->nextTrain = G_Find( nullptr, FOFS( targetname ), ent->target );
	if ( !ent->nextTrain ) {
		Com_Printf( "func_train at %s with an unfound target\n",
				  vtos( ent->shared.r.absmin ) );
		return;
	}

	start = nullptr;

	if ( ent->shared.s.eType == ET_BAT ) { // NOTE: this is odd.  it will never get hit.  remove?  what did it do?
		for ( path = ent->nextTrain ; path != start ; path = next ) {

			if ( !start ) {
				start = path;
			}

			if ( !path->target ) {
				Com_Printf( "Train corner at %s without a target\n",
						  vtos( path->shared.s.origin ) );
				return;
			}

			// find a path_corner among the targets
			// there may also be other targets that get fired when the corner
			// is reached
			next = nullptr;
			do {
				next = G_Find( next, FOFS( targetname ), path->target );
				if ( !next ) {
					Com_Printf( "Train corner at %s without a target path_corner\n",
							  vtos( path->shared.s.origin ) );
					return;
				}
			} while ( strcmp( next->classname, "path_corner" ) );

			path->nextTrain = next;
		}
	} else
	{
		for ( path = ent->nextTrain ; !path->nextTrain ; path = next ) {

			if ( !start ) {
				start = path;
			}

			if ( !path->target ) {
				Com_Printf( "Train corner at %s without a target\n",
						  vtos( path->shared.s.origin ) );
				return;
			}

			// find a path_corner among the targets
			// there may also be other targets that get fired when the corner
			// is reached
			next = nullptr;
			do {
				next = G_Find( next, FOFS( targetname ), path->target );
				if ( !next ) {
					Com_Printf( "Train corner at %s without a target path_corner\n",
							  vtos( path->shared.s.origin ) );
					return;
				}
			} while ( strcmp( next->classname, "path_corner" ) );

			path->nextTrain = next;
		}
	}

	if ( !Q_stricmp( ent->classname, "func_train" ) && ent->spawnflags & 2 ) { // TOGGLE
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.s.pos.trBase );
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.r.currentOrigin );
		SV_LinkEntity( &ent->shared );
	} else if ( !Q_stricmp( ent->classname, "func_train_particles" ) && ent->spawnflags & 2 )       { // TOGGLE
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.s.pos.trBase );
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.r.currentOrigin );
		SV_LinkEntity( &ent->shared );
	} else if ( !Q_stricmp( ent->classname, "func_tramcar" ) && ent->spawnflags & 2 )       { // TOGGLE
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.s.pos.trBase );
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.r.currentOrigin );
		SV_LinkEntity( &ent->shared );
	} else if ( !Q_stricmp( ent->classname, "func_bat" ) )       {
		//VectorCopy (ent->nextTrain->shared.s.origin, ent->shared.s.pos.trBase);
		//VectorCopy (ent->nextTrain->shared.s.origin, ent->shared.r.currentOrigin);
		//SV_LinkEntity (ent);
		if ( ent->spawnflags & 1 ) {  // start on
			ent->use( ent, ent, ent );
		}
	} else if ( !Q_stricmp( ent->classname, "truck_cam" ) && ent->spawnflags & 2 )     { // TOGGLE
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.s.pos.trBase );
		VectorCopy( ent->nextTrain->shared.s.origin, ent->shared.r.currentOrigin );
		SV_LinkEntity( &ent->shared );
	} else
	{
		if ( !Q_stricmp( ent->classname, "func_tramcar" ) ) {
			Reached_Tramcar( ent );
		} else if ( !Q_stricmp( ent->classname, "truck_cam" ) ) {
			Reached_Tramcar( ent );
		} else if ( !Q_stricmp( ent->classname, "camera_cam" ) ) {
			Reached_Tramcar( ent );
		} else {
			Reached_Train( ent );
		}
	}
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) STOP END REVERSE
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner

"count2" used only in conjunction with the truck_cam to control playing of gear changes
*/
void SP_path_corner( GameEntity *self ) {
	if ( !self->targetname ) {
		Com_Printf( "path_corner with no targetname at %s\n", vtos( self->shared.s.origin ) );
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in

	if ( self->wait == -1 ) {
		self->count = 1;
	}
}



/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_train( GameEntity *self ) {
	VectorClear( self->shared.s.angles );

	if ( self->spawnflags & TRAIN_BLOCK_STOPS ) {
		self->damage = 0;
		self->shared.s.eFlags |= EF_MOVER_STOP;
	} else {
		if ( !self->damage ) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		Com_Printf( "func_train without a target at %s\n", vtos( self->shared.r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	SV_SetBrushModel( &self->shared, self->model );
	InitMover( self );

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;

	self->blocked = Blocked_Door;

}

// Rafael - bats
/*QUAKED func_train_particles ( 0.3 0.1 0.8) ? START_ON TOGGLE
health = default 16 bats
*/
void Func_train_particles_reached( GameEntity *self ) {
	GameEntity *tent;
	vec3_t vec, ang;
	vec3_t forward;

	Reached_Train( self );

	if ( self->nextTrain->wait == -1 && self->nextTrain->count ) {
		return;
	}

	if ( !self->count ) {
		tent = G_TempEntity( self->shared.r.currentOrigin, EV_BATS );
		tent->shared.s.time = self->speed;
		tent->shared.s.density = self->health;
		VectorCopy( self->shared.r.currentOrigin, tent->shared.s.origin );
		VectorSubtract( self->nextTrain->shared.s.origin, self->shared.r.currentOrigin, vec );
		vectoangles( vec, ang );
		AngleVectors( ang, forward, nullptr, nullptr );
		VectorCopy( forward, tent->shared.s.angles );
		self->count = 1;
	} else
	{
		tent = G_TempEntity( self->shared.r.currentOrigin, EV_BATS_UPDATEPOSITION );
		tent->shared.s.time = self->speed;
		VectorCopy( self->shared.r.currentOrigin, tent->shared.s.origin );
		VectorSubtract( self->nextTrain->shared.s.origin, self->shared.r.currentOrigin, vec );
		vectoangles( vec, ang );
		AngleVectors( ang, forward, nullptr, nullptr );
		VectorCopy( forward, tent->shared.s.angles );
	}

	tent->shared.s.frame = self->shared.s.number;
	SV_LinkEntity( &self->shared );

}

void SP_func_train_particles( GameEntity *self ) {
	SP_func_train( self );
	self->reached = Func_train_particles_reached;
	self->blocked = nullptr;

	self->damage = 0;

	if ( !self->health ) {
		self->health = 16;
	}

	if ( !self->speed ) {
		self->speed = 50;
	}
}


/*QUAKED func_bats ( 0.3 0.1 0.8) (-32 -32 -32) (32 32 32) START_ON TOGGLE END_OUTER
count = default 10 bats
radius = maximum distance from center of entity to place each bat (default=32)
speed = speed to travel to next waypoint (default=300)
wait = (used for end map) wait seconds in between spawning
target = (used for end map) distance check from this entity to enable spawning if player is more than
	"radius" distance from the target
delay = (end map) wait in seconds this long after player steps outside, before spawning spirits
*/
void FuncBatsReached( GameEntity *self ) {
	if ( self->active == 2 ) {
		self->nextthink = -1;
		self->think = nullptr;
		return;
	}

	Reached_Train( self );

	if ( !self->nextTrain || !self->nextTrain->target ) {
		self->active = 2;   // remove the bats at next point
		return;
	}

}

// each bat calls this every server frame, so it moves towards it's ideal position
void BatMoveThink( GameEntity *bat ) {
	GameEntity *owner;
	vec3_t goalpos, vec;
	float speed, dist;
	int i;


	owner = &g_entities[bat->shared.r.ownerNum];
	if ( owner->active && owner->inuse ) { // move towards the owner
		BG_EvaluateTrajectory( &owner->shared.s.pos, level.time, goalpos );

		// randomize ther movedir as we go
		for ( i = 0; i < 3; i++ )
			bat->movedir[i] += crandom() * (float)owner->radius * 0.1;
		if ( VectorLength( bat->movedir ) > (float)owner->radius ) {
			VectorNormalize( bat->movedir );
			VectorScale( bat->movedir, (float)owner->radius, bat->movedir );
		}
		VectorAdd( goalpos, bat->movedir, goalpos );

		VectorSubtract( goalpos, bat->shared.s.pos.trBase, vec );
		dist = VectorLength( vec );
		speed = dist / 64;
		VectorMA( bat->shared.s.pos.trBase, 0.05 * speed, vec, bat->shared.s.pos.trBase );
		bat->shared.s.pos.trTime = level.time;
		VectorCopy( bat->shared.s.pos.trBase, bat->shared.r.currentOrigin );
		if ( dist * speed > 20 ) {
			vectoangles( vec, bat->shared.s.angles );
		}
		SV_LinkEntity( &bat->shared );

	} else if ( owner->active == 2 || !owner->inuse ) {
		// owner has finished
		G_FreeEntity( bat );
		return;
	}
	bat->nextthink = level.time + 50;
}

void BatDie( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int meansOfDeath ) {
	G_AddEvent( self, EV_BATS_DEATH, 0 );
	self->think = G_FreeEntity;
	self->nextthink = level.time + 100;
}

void FuncBatsActivate( GameEntity *self, GameEntity * other, GameEntity * activator ) {
	int i;
	GameEntity *bat;
	vec3_t vec;

	if ( !self->active ) {
		self->active = true;

		// spawn "count" bats
		for ( i = 0; i < self->count; i++ ) {
			bat = G_Spawn();
			bat->classname = "func_bat";
			bat->shared.s.eType = ET_BAT;

			VectorSet( vec, crandom(), crandom(), crandom() );
			VectorNormalize( vec );
			VectorScale( vec, random() * (float)self->radius, bat->movedir );
			VectorAdd( self->shared.s.pos.trBase, bat->movedir, bat->shared.s.pos.trBase );
			bat->shared.s.pos.trTime = level.time;
			VectorClear( bat->shared.s.pos.trDelta );
			VectorCopy( bat->shared.s.pos.trBase, bat->shared.r.currentOrigin );

			bat->shared.r.ownerNum = self->shared.s.number;
			bat->shared.r.contents = 0; //CONTENTS_CORPSE;
			bat->takedamage = false;
			bat->health = 1;
			bat->pain = nullptr;
			bat->die = nullptr; //BatDie;
			//VectorSet( bat->shared.r.mins, -18, -18, -18 );
			//VectorSet( bat->shared.r.maxs,  18,  18,  18 );

			bat->speed = self->speed;
			bat->radius = self->radius;

			bat->think = BatMoveThink;
			bat->nextthink = level.time + 50;

			SV_LinkEntity( &bat->shared );
		}

		InitMover( self );  // start moving
		FuncBatsReached( self );
		self->reached = FuncBatsReached;
		self->blocked = nullptr;

		// disable this to debug path
		self->shared.r.svFlags |= SVF_NOCLIENT;
		self->shared.r.contents = 0;

		self->use = FuncBatsActivate;   // make sure this stays the same

	} else {    // second use kills bats
		self->active = 2;
	}
}

void FuncEndSpiritsThink( GameEntity *self ) {
	vec3_t enemyPos;
	GameEntity *cEnt, *heinrich;
	//
	self->nextthink = level.time + (int)( ( 1.5 + 2.0 * random() ) * ( self->wait * 1000 ) );
	//
	if ( !self->active ) {
		return; // we are not allowed to release spirits yet
	}
	//
	// if heinrich isn't active yet, dont spawn
	if ( ( heinrich = AICast_FindEntityForName( "heinrich" ) ) ) {
		if ( heinrich->aiInactive ) {
			return;
		}
		if ( heinrich->health <= 0 ) {
			return;
		}
		if ( heinrich->shared.s.aiState < AISTATE_COMBAT ) {
			return;
		}
		if ( !g_entities[0].client || g_entities[0].client->cameraPortal ) {
			return;
		}
	} else {    // no heinrich?
		return;
	}
    
	//
	// if the player is close enough, and outside arena, spawn a spirit
	VectorCopy( g_entities[0].shared.s.pos.trBase, enemyPos );
	cEnt = G_Find( nullptr, FOFS( targetname ), self->target );
	if ( !cEnt ) {
		Com_Error( ERR_DROP, "couldnt find center marker for spirit spawner" );
        return; // keep the linter happy, ERR_DROP does not return
	}
	if ( VectorDistance( enemyPos, cEnt->shared.s.origin ) > self->radius ) {
		// also make sure the player is between us and the center entity
		if ( VectorDistance( self->shared.s.origin, enemyPos ) < VectorDistance( self->shared.s.origin, cEnt->shared.s.origin ) ) {
			// if we are not "spawning" then set the delay
			if ( !self->botDelayBegin ) {
				self->botDelayBegin = true;
				// set the delay before we start spawning them
				self->nextthink = level.time + (int)( self->delay * 1000.0 );
			} else {
				G_AddEvent( self, EV_SPAWN_SPIRIT, 0 );
			}
		} else {
			self->botDelayBegin = false;
		}
	} else {
		self->botDelayBegin = false;
	}
}

void SP_func_bats( GameEntity *self ) {
	if ( !self->count ) {
		self->count = 10;
	}

	if ( !self->radius ) {
		self->radius = 32;
	}

	if ( !self->speed ) {
		self->speed = 300;
	}

	// setup train waypoints
	self->active = false;
	self->use = FuncBatsActivate;

	self->damage = 0;

	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;

	// disable this to debug path
	self->shared.r.svFlags |= SVF_NOCLIENT;
	self->shared.r.contents = 0;

	if ( self->spawnflags & 4 ) { // END spirit spawners
		self->shared.r.svFlags &= ~SVF_NOCLIENT;
		self->shared.r.svFlags |= SVF_BROADCAST;
		self->shared.s.eFlags |= EF_NODRAW;
		self->shared.s.eType = ET_SPIRIT_SPAWNER;
		self->shared.s.otherEntityNum2 = 0;    // HACK: point to the player
		self->shared.s.time = (int)( self->delay * 1000 );
		self->use = nullptr;
		self->botDelayBegin = false;
		//
		self->think = FuncEndSpiritsThink;
		self->nextthink = level.time + ( self->wait * 1000 );
		//
		self->shared.r.contents = 0;
		SV_LinkEntity( &self->shared );
	}
}

// JOSEPH 9-27-99
/*
===============
Think_BeginMoving_rotating

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving_rotating( GameEntity *ent ) {
	ent->shared.s.pos.trTime = level.time;
	ent->shared.s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train_rotating
===============
*/
void Reached_Train_rotating( GameEntity *ent ) {
	GameEntity       *next;
	float speed;
	vec3_t move;
	float length;
	float frames;

	// copy the apropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		return;     // just stop
	}

	// fire all other targets
	G_UseTargets( next, nullptr );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
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

	ent->rotate[0] = next->rotate[2];
	ent->rotate[1] = next->rotate[0];
	ent->rotate[2] = next->rotate[1];

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	if ( next->duration ) {
		ent->shared.s.pos.trDuration = ( next->duration * 1000 );
	} else {
		ent->shared.s.pos.trDuration = length * 1000 / speed;
	}

	// Rotate the train
	frames = floor( ent->shared.s.pos.trDuration / 100 );

	if ( !frames ) {
		frames = 0.001;
	}

	ent->shared.s.apos.trType = TR_LINEAR;

	if ( ent->TargetFlag ) {
		VectorCopy( ent->TargetAngles, ent->shared.r.currentAngles );
		VectorCopy( ent->shared.r.currentAngles, ent->shared.s.angles );
		VectorCopy( ent->shared.s.angles, ent->shared.s.apos.trBase );
		ent->TargetFlag = 0;
	}

	//Com_Printf( "Train angles %s\n",
	//			vtos(ent->shared.s.angles) );

	//Com_Printf( "Add  X  Y  X %s\n",
	//			vtos(ent->rotate) );

	// X
	if ( ent->rotate[2] ) {
		ent->shared.s.apos.trDelta[2] = ( ent->rotate[2] / frames ) * 10;
	} else {
		ent->shared.s.apos.trDelta[2] = 0;
	}
	// Y
	if ( ent->rotate[0] ) {
		ent->shared.s.apos.trDelta[0] = ( ent->rotate[0] / frames ) * 10;
	} else {
		ent->shared.s.apos.trDelta[0] = 0;
	}
	// Z
	if ( ent->rotate[1] ) {
		ent->shared.s.apos.trDelta[1] = ( ent->rotate[1] / frames ) * 10;
	} else {
		ent->shared.s.apos.trDelta[1] = 0;
	}

	// looping sound
	ent->shared.s.loopSound = next->soundLoop;

	ent->TargetFlag = 1;
	ent->TargetAngles[0] = ent->shared.r.currentAngles[0] + ent->rotate[0];
	//ent->TargetAngles[0] = AngleNormalize360 (ent->TargetAngles[0]);
	ent->TargetAngles[1] = ent->shared.r.currentAngles[1] + ent->rotate[1];
	//ent->TargetAngles[1] = AngleNormalize360 (ent->TargetAngles[1]);
	ent->TargetAngles[2] = ent->shared.r.currentAngles[2] + ent->rotate[2];
	//ent->TargetAngles[2] = AngleNormalize360 (ent->TargetAngles[2]);

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving_rotating;
		ent->shared.s.pos.trType = TR_STATIONARY;
	}
}

/*
===============
Think_SetupTrainTargets_rotating

Link all the corners together
===============
*/
void Think_SetupTrainTargets_rotating( GameEntity *ent ) {
	GameEntity       *path, *next, *start;


	ent->nextTrain = G_Find( nullptr, FOFS( targetname ), ent->target );
	if ( !ent->nextTrain ) {
		Com_Printf( "func_train at %s with an unfound target\n",
				  vtos( ent->shared.r.absmin ) );
		return;
	}

	VectorCopy( ent->shared.s.angles, ent->shared.s.apos.trBase );
	VectorCopy( ent->shared.s.angles, ent->TargetAngles );
	ent->TargetFlag = 1;

	start = nullptr;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			Com_Printf( "Train corner at %s without a target\n",
					  vtos( path->shared.s.origin ) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = nullptr;
		do {
			next = G_Find( next, FOFS( targetname ), path->target );
			if ( !next ) {
				Com_Printf( "Train corner at %s without a target path_corner\n",
						  vtos( path->shared.s.origin ) );
				return;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		path->nextTrain = next;
	}

	// start the train moving from the first corner
	Reached_Train_rotating( ent );
}

/*QUAKED func_train_rotating (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
This train can also rotate along the X Y Z
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.

"model2"	.md3 model to also draw
"dmg"		default	2
"speed"		default 100
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius

On the path corner:
speed    departure speed from that corner
rotate   angle change for X Y Z to next corner
duration duration for angle change (overrides speed)
*/

void SP_func_train_rotating( GameEntity *self ) {
	VectorClear( self->shared.s.angles );

	if ( self->spawnflags & TRAIN_BLOCK_STOPS ) {
		self->damage = 0;
	} else {
		if ( !self->damage ) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		Com_Printf( "func_train without a target at %s\n", vtos( self->shared.r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	SV_SetBrushModel( &self->shared, self->model );
	InitMover( self );

	self->reached = Reached_Train_rotating;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets_rotating;
}


/*QUAKED func_leaky (0 .5 .8) ?
"leaktype" - leaks particles of this type ("type" is equiv in this ent)
"leakpressure" - force the particles come out with.  '0' would cause them to fall straight down
"leaktime" - how long it leaks before it quits
"leakcount" - how many holes in the entity before it no longer leaks.

1:oil
2:water
3:steam
4:wine
5:smoke
6:electrical

*/
void SP_func_leaky( GameEntity *ent ) {

	if ( ent->model2 ) {
		ent->shared.s.modelindex2 = G_ModelIndex( ent->model2 );
	}
	SV_SetBrushModel( &ent->shared, ent->model );
	ent->shared.s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.origin, ent->shared.r.currentOrigin );

	// (SA) this is not ideal, but gets us finished.
	G_SpawnInt( "type", "0", &ent->emitID );
	if ( !ent->emitID ) {
		G_SpawnInt( "leaktype", "0", &ent->emitID );
	}

	G_SpawnInt( "leakpressure", "30", &ent->emitPressure );

// hacks
	if ( ent->emitID == 2 ) {  // no water
		ent->emitID = 3;    // make it steam

	}
	if ( ent->emitID == 3 ) {  // steam
		ent->emitPressure = 100;
	}

// end hacks

	G_SpawnInt( "leaktime", "10", &ent->emitTime );
	ent->emitTime *= 1000;  // make ms
	G_SpawnInt( "leakcount", "10", &ent->emitNum );
	ent->shared.s.eType = ET_LEAKY;
	SV_LinkEntity( &ent->shared );
}




/*
===============================================================================

ROTATING

===============================================================================
*/


/*QUAKED func_rotating (0 .5 .8) ? START_ON STARTINVIS X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.
The center of that brush will be the point around which it is rotated. It will rotate around the Z axis by default.  You can check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/

void Use_Func_Rotate( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	if ( ent->spawnflags & 4 ) {
		ent->shared.s.apos.trDelta[2] = ent->speed;
	} else if ( ent->spawnflags & 8 )   {
		ent->shared.s.apos.trDelta[0] = ent->speed;
	} else {
		ent->shared.s.apos.trDelta[1] = ent->speed;
	}

	if ( ent->spawnflags & 2 ) {
		ent->flags &= ~FL_TEAMSLAVE;
	}

	SV_LinkEntity( &ent->shared );
}

void SP_func_rotating( GameEntity *ent ) {
	if ( !ent->speed ) {
		ent->speed = 100;
	}

	// set the axis of rotation
	ent->shared.s.apos.trType = TR_LINEAR;

	if ( ent->spawnflags & 1 ) {
		if ( ent->spawnflags & 4 ) {
			ent->shared.s.apos.trDelta[2] = ent->speed;
		} else if ( ent->spawnflags & 8 ) {
			ent->shared.s.apos.trDelta[0] = ent->speed;
		} else {
			ent->shared.s.apos.trDelta[1] = ent->speed;
		}
	}

	if ( !ent->damage ) {
		ent->damage = 2;
	}

	SV_SetBrushModel( &ent->shared, ent->model );
	InitMover( ent );

	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.pos.trBase, ent->shared.r.currentOrigin );
	VectorCopy( ent->shared.s.apos.trBase, ent->shared.r.currentAngles );

	if ( ent->spawnflags & 2 ) {
		ent->flags |= FL_TEAMSLAVE;
		SV_UnlinkEntity( &ent->shared );
	} else {
		SV_LinkEntity( &ent->shared );
	}

}


/*
===============================================================================

BOBBING

===============================================================================
*/


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_bobbing( GameEntity *ent ) {
	float height;
	float phase;

	G_SpawnFloat( "speed", "4", &ent->speed );
	G_SpawnFloat( "height", "32", &height );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	SV_SetBrushModel( &ent->shared, ent->model );
	InitMover( ent );

	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.origin, ent->shared.r.currentOrigin );

	ent->shared.s.pos.trDuration = ent->speed * 1000;
	ent->shared.s.pos.trTime = ent->shared.s.pos.trDuration * phase;
	ent->shared.s.pos.trType = TR_SINE;

	// set the axis of bobbing
	if ( ent->spawnflags & 1 ) {
		ent->shared.s.pos.trDelta[0] = height;
	} else if ( ent->spawnflags & 2 ) {
		ent->shared.s.pos.trDelta[1] = height;
	} else {
		ent->shared.s.pos.trDelta[2] = height;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum( GameEntity *ent ) {
	float freq;
	float length;
	float phase;
	float speed;

	G_SpawnFloat( "speed", "30", &speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	SV_SetBrushModel( &ent->shared, ent->model );

	// find pendulum length
	length = fabs( ent->shared.r.mins[2] );
	if ( length < 8 ) {
		length = 8;
	}

	freq = 1 / ( M_PI * 2 ) * sqrt( g_gravity.value / ( 3 * length ) );

	ent->shared.s.pos.trDuration = ( 1000 / freq );

	InitMover( ent );

	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.origin, ent->shared.r.currentOrigin );

	VectorCopy( ent->shared.s.angles, ent->shared.s.apos.trBase );

	ent->shared.s.apos.trDuration = 1000 / freq;
	ent->shared.s.apos.trTime = ent->shared.s.apos.trDuration * phase;
	ent->shared.s.apos.trType = TR_SINE;
	ent->shared.s.apos.trDelta[2] = speed;
}


/*QUAKED func_invisible_user (.3 .5 .8) ? STARTOFF HAS_USER NO_OFF_NOISE NOT_KICKABLE
when activated will use its target
"delay" - time (in seconds) before it can be used again
"offnoise" - specifies an alternate sound
"cursorhint" - overrides the auto-location of targeted entity (list below)
Normally when a player 'activates' this entity, if the entity has been turned 'off' (by a scripted command) you will hear a sound to indicate that you cannot activate the user.
The sound defaults to "sound/movers/invis_user_off.wav"

NO_OFF_NOISE - no sound will play if the invis_user is used when 'off'
NOT_KICKABLE - kicking doesn't fire, only player activating

"cursorhint" cursor types: (probably more, ask sherman if you think the list is out of date)
they /don't/ need to be all uppercase
	HINT_NONE
	HINT_PLAYER
	HINT_ACTIVATE
	HINT_DOOR
	HINT_DOOR_ROTATING
	HINT_DOOR_LOCKED
	HINT_DOOR_ROTATING_LOCKED
	HINT_MG42
	HINT_BREAKABLE
	HINT_BREAKABLE_BIG
	HINT_CHAIR
	HINT_ALARM
	HINT_HEALTH
	HINT_TREASURE
	HINT_KNIFE
	HINT_LADDER
	HINT_BUTTON
	HINT_WATER
	HINT_CAUTION
	HINT_DANGER
	HINT_SECRET
	HINT_QUESTION
	HINT_EXCLAMATION
	HINT_CLIPBOARD
	HINT_WEAPON
	HINT_AMMO
	HINT_ARMOR
	HINT_POWERUP
	HINT_HOLDABLE
	HINT_INVENTORY
	HINT_SCENARIC
	HINT_EXIT
	HINT_NOEXIT
	HINT_EXIT_FAR
	HINT_NOEXIT_FAR
	HINT_PLYR_FRIEND
	HINT_PLYR_NEUTRAL
	HINT_PLYR_ENEMY
	HINT_PLYR_UNKNOWN
*/

void use_invisible_user( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	GameEntity *player;

	if ( ent->wait < level.time ) {
		ent->wait = level.time + ent->delay;
	} else {
		return;
	}

	if ( !( other->client ) ) {
		if ( ent->spawnflags & 1 ) {
			ent->spawnflags &= ~1;
		} else
		{
			ent->spawnflags |= 1;
		}

		if ( ent->spawnflags & 2 && !( ent->spawnflags & 1 ) ) {
			if ( ent->aiName ) {
				player = AICast_FindEntityForName( "player" );
				if ( player ) {
					AICast_ScriptEvent( AICast_GetCastState( player->shared.s.number ), "trigger", ent->target );
				}
			}

			G_UseTargets( ent, other );

			// Com_Printf ("ent%s used by %s\n", ent->classname, other->classname);
		}

		return;
	}

	if ( other->client && ent->spawnflags & 1 ) {
		//----(SA)	play 'off' sound
		//----(SA)	I think this is where this goes.  Raf, let me know if it's wrong.  I need someone to tell me what a test map is for this (I'll ask Dan tomorrow)
		// not usable by player.  turned off.
		G_Sound( ent, ent->soundPos1 );
		return;
	}

	if ( ent->aiName ) {
		player = AICast_FindEntityForName( "player" );
		if ( player ) {
			AICast_ScriptEvent( AICast_GetCastState( player->shared.s.number ), "trigger", ent->target );
		}
	}

	G_UseTargets( ent, other ); //----(SA)	how about this so the triggered targets have an 'activator' as well as an 'other'?
								//----(SA)	Please let me know if you forsee any problems with this.
}


void func_invisible_user( GameEntity *ent ) {
	int i;
	const char    *sound;
	const char    *cursorhint;

	VectorCopy( ent->shared.s.origin, ent->pos1 );
	SV_SetBrushModel( &ent->shared, ent->model );

	// InitMover (ent);
	VectorCopy( ent->pos1, ent->shared.r.currentOrigin );
	SV_LinkEntity( &ent->shared );

	ent->shared.s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->shared.s.pos.trBase );

	ent->shared.r.contents = CONTENTS_TRIGGER;

	ent->shared.r.svFlags = SVF_NOCLIENT;

	ent->delay *= 1000; // convert to ms

	ent->use = use_invisible_user;

	if ( G_SpawnString( "cursorhint", "0", &cursorhint ) ) {
		for ( i = 1; i < HINT_NUM_HINTS; i++ ) {
			if ( !Q_strcasecmp( cursorhint, hintStrings[i] ) ) {
				ent->shared.s.dmgFlags = i;
				break;
			}
		}
	}

	if ( !( ent->spawnflags & 4 ) ) {    // !NO_OFF_NOISE
		if ( G_SpawnString( "offnoise", "0", &sound ) ) {
			ent->soundPos1 = G_SoundIndex( sound );
		} else {
			ent->soundPos1 = G_SoundIndex( "sound/movers/invis_user_off.wav" );
		}
	}
}

/*
==========
G_Activate

  Generic activation routine for doors
==========
*/
void G_Activate( GameEntity *ent, GameEntity *activator ) {
	if ( ( ent->shared.s.apos.trType == TR_STATIONARY && ent->shared.s.pos.trType == TR_STATIONARY )
		 && !ent->active ) {
		// trigger the ent if possible, if not, then we'll just wait at the marker until it opens, which could be never(!?)
		if ( ent->key >= KEY_LOCKED_TARGET ) { // ent force locked
			return;
		}

		if ( ent->key > KEY_NONE && ent->key < KEY_NUM_KEYS ) { // ent requires key
			gitem_t *item = BG_FindItemForKey( (wkey_t)ent->key, 0 );
			if ( !( activator->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
				return;
			}
		}

		if ( !Q_stricmp( ent->classname, "script_mover" ) ) { // RF, dont activate script_mover's
			if ( activator->aiName ) {
				G_Script_ScriptEvent( ent, "activate", activator->aiName, "" );
			}
			return;
		}

		// hack fix for bigdoor1 on tram1_21

		if ( !( ent->teammaster ) ) {
			ent->active = true;
			Use_BinaryMover( ent, activator, activator );
			G_UseTargets( ent->teammaster, activator );
			return;
		}

		if ( ent->team && ent != ent->teammaster ) {
			ent->teammaster->active = true;
			Use_BinaryMover( ent->teammaster, activator, activator );
			G_UseTargets( ent->teammaster, activator );
		} else
		{
			ent->active = true;
			Use_BinaryMover( ent, activator, activator );
			G_UseTargets( ent->teammaster, activator );
		}
	}
}
