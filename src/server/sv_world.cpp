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

#include "server.h"
#include "../qcommon/clip_model.h"

/*
================
SV_ClipHandleForEntity

Returns a headnode that can be used for testing or clipping to a
given entity.  If the entity is a bsp model, the headnode will
be returned, otherwise a custom box tree will be constructed.
================
*/
clipHandle_t SV_ClipHandleForEntity( const sharedEntity_t *ent )
{
	if ( ent->r.bmodel ) {
		// explicit hulls in the BSP model
		return TheClipModel::get().inlineModel( ent->s.modelindex );
	}
	if ( ent->r.svFlags & SVF_CAPSULE ) {
		// create a temp capsule from bounding box sizes
		return TheClipModel::get().tempBoxModel( ent->r.mins, ent->r.maxs, true );
	}

	// create a temp tree from bounding box sizes
	return TheClipModel::get().tempBoxModel( ent->r.mins, ent->r.maxs, false );
}



/*
===============================================================================

ENTITY CHECKING

To avoid linearly searching through lists of entities during environment testing,
the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
are kept in chains either at the final leafs, or at the first node that splits
them, which prevents having to deal with multiple fragments of a single entity.

===============================================================================
*/

class WorldSector
{
public:
	int axis;           // -1 = leaf node
	float dist;
	WorldSector   *children[2];
	ServerEntity  *entities;
};

#define AREA_DEPTH  4
#define AREA_NODES  64

WorldSector sv_worldSectors[AREA_NODES];
int sv_numworldSectors;


/*
===============
SV_CreateworldSector

Builds a uniformly subdivided tree for the given world size
===============
*/
WorldSector *SV_CreateworldSector( int depth, idVec3 mins, idVec3 maxs )
{
	WorldSector* anode = &sv_worldSectors[sv_numworldSectors];
	sv_numworldSectors++;

	if ( depth == AREA_DEPTH ) {
		anode->axis = -1;
		anode->children[0] = anode->children[1] = nullptr;
		return anode;
	}

	idVec3 size = maxs - mins;
	if ( size[0] > size[1] ) {
		anode->axis = 0;
	} else {
		anode->axis = 1;
	}

	anode->dist = 0.5 * ( maxs[anode->axis] + mins[anode->axis] );
	idVec3 mins1 = mins;
	idVec3 mins2 = mins;
	idVec3 maxs1 = maxs;
	idVec3 maxs2 = maxs;


	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateworldSector( depth + 1, mins2, maxs2 );
	anode->children[1] = SV_CreateworldSector( depth + 1, mins1, maxs1 );

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
// public, called by SV_SpawnServer
void SV_ClearWorld()
{
	memset( sv_worldSectors, 0, sizeof( sv_worldSectors ) );
	sv_numworldSectors = 0;

	// get world map bounds
	clipHandle_t h = TheClipModel::get().inlineModel( 0 );
	idVec3 mins, maxs;
	TheClipModel::get().modelBounds( h, mins, maxs );
	SV_CreateworldSector( 0, mins, maxs );
}


/*
===============
SV_UnlinkEntity

===============
*/
// public, called by lots of things.
void SV_UnlinkEntity( sharedEntity_t *gEnt )
{
	ServerEntity* ent = SV_SvEntityForGentity( gEnt );

	gEnt->r.linked = false;

	WorldSector* ws = ent->worldSector;
	if ( !ws ) {
		return;     // not linked in anywhere
	}
	ent->worldSector = nullptr;

	if ( ws->entities == ent ) {
		ws->entities = ent->nextEntityInWorldSector;
		return;
	}

	for (ServerEntity* scan = ws->entities ; scan ; scan = scan->nextEntityInWorldSector ) {
		if ( scan->nextEntityInWorldSector == ent ) {
			scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
			return;
		}
	}

	Com_Printf( "WARNING: SV_UnlinkEntity: not found in worldSector\n" );
}


/*
===============
SV_LinkEntity

===============
*/
#define MAX_TOTAL_ENT_LEAFS     128
WorldSector *debugNode;
// public, called by lots of things.
void SV_LinkEntity( sharedEntity_t *gEnt )
{
	int leafs[MAX_TOTAL_ENT_LEAFS];
	
	ServerEntity* ent = SV_SvEntityForGentity( gEnt );

	// Ridah, sanity check for possible currentOrigin being reset bug
	if ( !gEnt->r.bmodel && VectorCompare( gEnt->r.currentOrigin, vec3_origin ) ) {
		Com_DPrintf( "WARNING: BBOX entity is being linked at world origin, this is probably a bug\n" );
	}

	if ( ent->worldSector ) {
		SV_UnlinkEntity( gEnt );    // unlink from old position
	}

	// encode the size into the EntityState for client prediction
	if ( gEnt->r.bmodel ) {
		gEnt->s.solid = SOLID_BMODEL;       // a solid_box will never create this value
	} else if ( gEnt->r.contents & ( CONTENTS_SOLID | CONTENTS_BODY ) ) {
		// assume that x/y are equal and symetric
		int i = gEnt->r.maxs[0];
		if ( i < 1 ) {
			i = 1;
		}
		if ( i > 255 ) {
			i = 255;
		}

		// z is not symetric
		int j = ( -gEnt->r.mins[2] );
		if ( j < 1 ) {
			j = 1;
		}
		if ( j > 255 ) {
			j = 255;
		}

		// and z maxs can be negative...
		int k = ( gEnt->r.maxs[2] + 32 );
		if ( k < 1 ) {
			k = 1;
		}
		if ( k > 255 ) {
			k = 255;
		}

		gEnt->s.solid = ( k << 16 ) | ( j << 8 ) | i;
	} else {
		gEnt->s.solid = 0;
	}

	// get the position
	float* origin = gEnt->r.currentOrigin;
	float* angles = gEnt->r.currentAngles;

	// set the abs box
	if ( gEnt->r.bmodel && ( angles[0] || angles[1] || angles[2] ) ) {
		// expand for rotation
		float max = RadiusFromBounds( gEnt->r.mins, gEnt->r.maxs );
		for (int i = 0 ; i < 3 ; i++ ) {
			gEnt->r.absmin[i] = origin[i] - max;
			gEnt->r.absmax[i] = origin[i] + max;
		}
	} else {
		// normal
		VectorAdd( origin, gEnt->r.mins, gEnt->r.absmin );
		VectorAdd( origin, gEnt->r.maxs, gEnt->r.absmax );
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	gEnt->r.absmin[0] -= 1;
	gEnt->r.absmin[1] -= 1;
	gEnt->r.absmin[2] -= 1;
	gEnt->r.absmax[0] += 1;
	gEnt->r.absmax[1] += 1;
	gEnt->r.absmax[2] += 1;

	// link to PVS leafs
	ent->numClusters = 0;
	ent->lastCluster = 0;
	ent->areanum = -1;
	ent->areanum2 = -1;

	ClipModel& clipModel = TheClipModel::get();

	//get all leafs, including solids
	int lastLeaf;
	int num_leafs = CM_BoxLeafnums( gEnt->r.absmin, gEnt->r.absmax,
								leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf );

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if ( !num_leafs ) {
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for (int i = 0 ; i < num_leafs ; i++ ) {
		int area = clipModel.leafArea( leafs[i] );
		if ( area != -1 ) {
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if ( ent->areanum != -1 && ent->areanum != area ) {
				if ( ent->areanum2 != -1 && ent->areanum2 != area && sv.state == SS_LOADING ) {
					Com_DPrintf( "Object %i touching 3 areas at %f %f %f\n",
								 gEnt->s.number,
								 gEnt->r.absmin[0], gEnt->r.absmin[1], gEnt->r.absmin[2] );
				}
				ent->areanum2 = area;
			} else {
				ent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	ent->numClusters = 0;
	int leaf;
	for (int leaf = 0 ; leaf < num_leafs ; leaf++ ) {
		int cluster = clipModel.leafCluster( leafs[leaf] );
		if ( cluster != -1 ) {
			ent->clusternums[ent->numClusters++] = cluster;
			if ( ent->numClusters == MAX_ENT_CLUSTERS ) {
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if ( leaf != num_leafs ) {
		ent->lastCluster = clipModel.leafCluster( lastLeaf );
	}

	gEnt->r.linkcount++;

	// find the first world sector node that the ent's box crosses
	WorldSector* node = sv_worldSectors;
	while ( 1 )
	{
		if ( node->axis == -1 ) {
			break;
		}
		if ( gEnt->r.absmin[node->axis] > node->dist ) {
			node = node->children[0];
		} else if ( gEnt->r.absmax[node->axis] < node->dist ) {
			node = node->children[1];
		} else {
			break;      // crosses the node
		}
	}

	// link it in
	ent->worldSector = node;
	ent->nextEntityInWorldSector = node->entities;
	node->entities = ent;

	gEnt->r.linked = true;
}

/*
============================================================================

AREA QUERY

Fills in a list of all entities who's absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

struct AreaParms
{
	idVec3 mins;
	idVec3 maxs;
	int *list;
	int count, maxcount;
};

// helper function for SV_AreaEntities
void SV_AreaEntities_r( WorldSector *node, AreaParms& ap )
{
	for (ServerEntity * check = node->entities; check; check = check->nextEntityInWorldSector ) {
		sharedEntity_t *gcheck = SV_GEntityForSvEntity( check );

		if ( gcheck->r.absmin[0] > ap.maxs.x
			 || gcheck->r.absmin[1] > ap.maxs.y
			 || gcheck->r.absmin[2] > ap.maxs.z
			 || gcheck->r.absmax[0] < ap.mins.x
			 || gcheck->r.absmax[1] < ap.mins.y
			 || gcheck->r.absmax[2] < ap.mins.z ) {
			continue;
		}

		if ( ap.count == ap.maxcount ) {
			Com_DPrintf( "SV_AreaEntities: MAXCOUNT\n" );
			return;
		}

		ap.list[ap.count++] = (int)(check - sv.svEntities);
	}

	if ( node->axis == -1 ) {
		return;     // terminal node
	}

	// recurse down both sides
	if ( ap.maxs[node->axis] > node->dist ) {
		SV_AreaEntities_r( node->children[0], ap );
	}
	if ( ap.mins[node->axis] < node->dist ) {
		SV_AreaEntities_r( node->children[1], ap );
	}
}

// public
int SV_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount )
{
	AreaParms ap;

	ap.mins = idVec3( mins[0], mins[1], mins[2] );
	ap.maxs = idVec3( maxs[0], maxs[1], maxs[2] );
	ap.list = entityList;
	ap.count = 0;
	ap.maxcount = maxcount;

	SV_AreaEntities_r( sv_worldSectors, ap );

	return ap.count;
}



//===========================================================================


typedef struct {
	vec3_t boxmins, boxmaxs;     // enclose the test object along entire move
	const float *mins;
	const float *maxs;  // size of the moving object
	const float *start;
	vec3_t end;
	trace_t trace;
	int passEntityNum;
	int contentmask;
	int capsule;
} moveclip_t;


/*
====================
SV_ClipToEntity

====================
*/
// public, called eventually by AAS_AreaEntityCollision
void SV_ClipToEntity( trace_t *trace, const vec3_t start,
					 const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int entityNum, int contentmask, int capsule )
{
	sharedEntity_t* touch = SV_GentityNum( entityNum );

	memset( trace, 0, sizeof( trace_t ) );

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if ( !( contentmask & touch->r.contents ) ) {
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle_t clipHandle = SV_ClipHandleForEntity( touch );

	float* origin = touch->r.currentOrigin;
	float* angles = touch->r.currentAngles;

	if ( !touch->r.bmodel ) {
		angles = vec3_origin;   // boxes don't rotate
	}

	CM_TransformedBoxTrace( trace, start, end,
							mins, maxs, clipHandle, contentmask,
							origin, angles, capsule );
	if ( trace->fraction < 1 ) {
		trace->entityNum = touch->s.number;
	}
}


/*
====================
SV_ClipMoveToEntities

====================
*/
// private, called by SV_Trace
void SV_ClipMoveToEntities( moveclip_t *clip )
{
	int touchlist[MAX_GENTITIES];
	int num = SV_AreaEntities( clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES );

	int passOwnerNum = -1;
	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		passOwnerNum = ( SV_GentityNum( clip->passEntityNum ) )->r.ownerNum;
		if ( passOwnerNum == ENTITYNUM_NONE ) {
			passOwnerNum = -1;
		}
	}

	for (int i = 0 ; i < num ; i++ ) {
		if ( clip->trace.allsolid ) {
			return;
		}
		sharedEntity_t* touch = SV_GentityNum( touchlist[i] );

		// see if we should ignore this entity
		if ( clip->passEntityNum != ENTITYNUM_NONE ) {
			if ( touchlist[i] == clip->passEntityNum ) {
				continue;   // don't clip against the pass entity
			}
			if ( touch->r.ownerNum == clip->passEntityNum ) {
				continue;   // don't clip against own missiles
			}
			if ( touch->r.ownerNum == passOwnerNum ) {
				continue;   // don't clip against other missiles from our owner
			}
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if ( !( clip->contentmask & touch->r.contents ) ) {
			continue;
		}

		// RF, special case, ignore chairs if we are carrying them
		if ( touch->s.eType == ET_PROP && touch->s.otherEntityNum == clip->passEntityNum + 1 ) {
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle_t clipHandle = SV_ClipHandleForEntity( touch );

		float* origin = touch->r.currentOrigin;
		float* angles = touch->r.currentAngles;


		if ( !touch->r.bmodel ) {
			angles = vec3_origin;   // boxes don't rotate
		}

		trace_t trace;
		CM_TransformedBoxTrace( &trace, clip->start, clip->end,
								clip->mins, clip->maxs, clipHandle,  clip->contentmask,
								origin, angles, clip->capsule );

		if ( trace.allsolid ) {
			clip->trace.allsolid = true;
			trace.entityNum = touch->s.number;
		} else if ( trace.startsolid ) {
			clip->trace.startsolid = true;
			trace.entityNum = touch->s.number;
		}

		if ( trace.fraction < clip->trace.fraction ) {
			// make sure we keep a startsolid from a previous trace
			bool oldStart = clip->trace.startsolid;

			trace.entityNum = touch->s.number;
			clip->trace = trace;
			if ( oldStart ){
				clip->trace.startsolid = true;
			}
		}
	}
}


// public
void SV_TraceCapsule( trace_t *results, const vec3_t start,
					 const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int passEntityNum, int contentmask )
{
    SV_Trace(results, start, mins, maxs, end, passEntityNum, contentmask, true);
}

/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.
passEntityNum and entities owned by passEntityNum are explicitly not checked.
==================
*/
// public
void SV_Trace( trace_t *results, const vec3_t start,
			  const vec3_t mins, const vec3_t maxs, const vec3_t end,
			  int passEntityNum, int contentmask, int capsule )
{
	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	moveclip_t clip;
	memset( &clip, 0, sizeof( moveclip_t ) );

	// clip to world
	CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, capsule );
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	if ( clip.trace.fraction == 0 ) {
		*results = clip.trace;
		return;     // blocked immediately by the world
	}

	clip.contentmask = contentmask;
	clip.start = start;
	VectorCopy( end, clip.end );
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passEntityNum = passEntityNum;
	clip.capsule = capsule;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for (int i = 0 ; i < 3 ; i++ ) {
		if ( end[i] > start[i] ) {
			clip.boxmins[i] = clip.start[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.end[i] + clip.maxs[i] + 1;
		} else {
			clip.boxmins[i] = clip.end[i] + clip.mins[i] - 1;
			clip.boxmaxs[i] = clip.start[i] + clip.maxs[i] + 1;
		}
	}

	// clip to other solid entities
	SV_ClipMoveToEntities( &clip );

	*results = clip.trace;
}



/*
=============
SV_PointContents
=============
*/
// public
int SV_PointContents( const vec3_t p, int passEntityNum )
{
	// get base contents from world
	int contents = CM_PointContents( p, 0 );

	// or in contents from all the other entities
	int touch[MAX_GENTITIES];
	int num = SV_AreaEntities( p, p, touch, MAX_GENTITIES );

	for (int i = 0 ; i < num ; i++ ) {
		if ( touch[i] == passEntityNum ) {
			continue;
		}
		sharedEntity_t* hit = SV_GentityNum( touch[i] );
		// might intersect, so do an exact clip
		clipHandle_t clipHandle = SV_ClipHandleForEntity( hit );
		float* angles = hit->s.angles;
		if ( !hit->r.bmodel ) {
			angles = vec3_origin;   // boxes don't rotate
		}

		int c2 = CM_TransformedPointContents( p, clipHandle, hit->s.origin, hit->s.angles );
		contents |= c2;
	}

	return contents;
}


