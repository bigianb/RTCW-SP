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


#include "../game/q_shared.h"
#include "qcommon.h"
#include "cm_polylib.h"
#include "cm_patch.h"




// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON    ( 0.125 )

// cm_test.c

// Used for oriented capsule collision detection
typedef struct
{
	bool use;
	float radius;
	float halfheight;
	vec3_t offset;
} sphere_t;

typedef struct {
	vec3_t start;
	vec3_t end;
	vec3_t size[2];         // size of the box being swept through the model
	vec3_t offsets[8];      // [signbits][x] = either size[0][x] or size[1][x]
	float maxOffset;        // longest corner length from origin
	vec3_t extents;         // greatest of abs(size[0]) and abs(size[1])
	vec3_t bounds[2];       // enclosing box of start and end surrounding by size
	vec3_t modelOrigin;     // origin of the model tracing through
	int contents;           // ored contents of the model tracing through
	bool isPoint;       // optimized case
	trace_t trace;          // returned from trace call
	sphere_t sphere;        // sphere for oriendted capsule collision
} traceWork_t;

typedef struct leafList_s {
	int count;
	int maxcount;
	bool overflowed;
	int     *list;
	vec3_t bounds[2];
	int lastLeaf;           // for overflows where each leaf can't be stored individually
	void ( *storeLeafs )( struct leafList_s *ll, int nodenum );
} leafList_t;

struct cBrush_t;
int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cBrush_t **list, int listsize );

void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

// cm_patch.c

patchCollide_t   *CM_GeneratePatchCollide( int width, int height, idVec3 *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const patchCollide_t *pc );
bool CM_PositionTestInPatchCollide( traceWork_t *tw, const patchCollide_t *pc );
void CM_ClearLevelPatches( void );


