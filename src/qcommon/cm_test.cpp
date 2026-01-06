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

#include "cm_local.h"
#include "clip_model.h"

/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r( const vec3_t p, int num )
{
	const ClipModel& cm = TheClipModel::get();
	while ( num >= 0 )
	{
		cNode_t     *node = cm.nodes + num;
		cplane_t    *plane = node->plane;

		float d;
		if ( plane->type < 3 ) {
			d = p[plane->type] - plane->dist;
		} else {
			d = DotProduct( plane->normal, p ) - plane->dist;
		}
		if ( d < 0 ) {
			num = node->children[1];
		} else {
			num = node->children[0];
		}
	}

	return -1 - num;
}

int CM_PointLeafnum( const vec3_t p )
{
	const ClipModel& cm = TheClipModel::get();
	if ( !cm.numNodes ) {   // map not loaded
		return 0;
	}
	return CM_PointLeafnum_r( p, 0 );
}

/*
======================================================================

LEAF LISTING

======================================================================
*/
void CM_StoreLeafs( leafList_t *ll, int nodenum )
{
	int leafNum = -1 - nodenum;

	const ClipModel& cm = TheClipModel::get();
	// store the lastLeaf even if the list is overflowed
	if ( cm.leaves[ leafNum ].cluster != -1 ) {
		ll->lastLeaf = leafNum;
	}

	if ( ll->count >= ll->maxcount ) {
		ll->overflowed = true;
		return;
	}
	ll->list[ ll->count++ ] = leafNum;
}

void CM_StoreBrushes( leafList_t *ll, int nodenum )
{
	const ClipModel& cm = TheClipModel::get();
	int leafnum = -1 - nodenum;
	cLeaf_t *leaf = &cm.leaves[leafnum];

	for (int k = 0 ; k < leaf->numLeafBrushes ; k++ ) {
		int brushnum = leaf->firstLeafBrush + k;
		if (leaf->fromSubmodel == 0){
			brushnum = cm.leafBrushes[brushnum];
		}
		cBrush_t *b = &cm.brushes[brushnum];
		if ( b->checkcount == cm.checkcount ) {
			continue;   // already checked this brush in another leaf
		}
		b->checkcount = cm.checkcount;
		int i;
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( b->bounds[0][i] >= ll->bounds[1][i] || b->bounds[1][i] <= ll->bounds[0][i] ) {
				break;
			}
		}
		if ( i != 3 ) {
			continue;
		}
		if ( ll->count >= ll->maxcount ) {
			ll->overflowed = true;
			return;
		}
		( (cBrush_t **)ll->list )[ ll->count++ ] = b;
	}
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
void CM_BoxLeafnums_r( leafList_t *ll, int nodenum ) {
	cplane_t    *plane;
	cNode_t     *node;
	int s;
	const ClipModel& cm = TheClipModel::get();
	while ( 1 ) {
		if ( nodenum < 0 ) {
			ll->storeLeafs( ll, nodenum );
			return;
		}

		node = &cm.nodes[nodenum];
		plane = node->plane;
		s = BoxOnPlaneSide( ll->bounds[0], ll->bounds[1], plane );
		if ( s == 1 ) {
			nodenum = node->children[0];
		} else if ( s == 2 ) {
			nodenum = node->children[1];
		} else {
			// go down both
			CM_BoxLeafnums_r( ll, node->children[0] );
			nodenum = node->children[1];
		}
	}
}

/*
==================
CM_BoxLeafnums
==================
*/
int CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *lastLeaf ) {
	leafList_t ll;
	ClipModel& cm = TheClipModel::get();
	cm.checkcount++;

	VectorCopy( mins, ll.bounds[0] );
	VectorCopy( maxs, ll.bounds[1] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = list;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = false;

	CM_BoxLeafnums_r( &ll, 0 );

	*lastLeaf = ll.lastLeaf;
	return ll.count;
}

/*
==================
CM_BoxBrushes
==================
*/
int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cBrush_t **list, int listsize ) {
	leafList_t ll;

	ClipModel& cm = TheClipModel::get();
	cm.checkcount++;

	VectorCopy( mins, ll.bounds[0] );
	VectorCopy( maxs, ll.bounds[1] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = (int *)list;
	ll.storeLeafs = CM_StoreBrushes;
	ll.lastLeaf = 0;
	ll.overflowed = false;

	CM_BoxLeafnums_r( &ll, 0 );

	return ll.count;
}


//====================================================================


/*
==================
CM_PointContents

==================
*/
int CM_PointContents( const vec3_t p, clipHandle_t model ) {
	int leafnum;
	int i, k;
	int brushnum;
	cLeaf_t     *leaf;
	cBrush_t    *b;
	int contents;
	float d;
	cModel_t    *clipm;

	ClipModel& cm = TheClipModel::get();
	if ( !cm.numNodes ) { // map not loaded
		return 0;
	}

	if ( model ) {
		clipm = cm.clipHandleToModel( model );
		leaf = &clipm->leaf;
	} else {
		leafnum = CM_PointLeafnum_r( p, 0 );
		leaf = &cm.leaves[leafnum];
	}

	contents = 0;
	for ( k = 0 ; k < leaf->numLeafBrushes ; k++ ) {
		brushnum = leaf->firstLeafBrush + k;
		if (leaf->fromSubmodel == 0){
			brushnum = cm.leafBrushes[brushnum];
		}
		b = &cm.brushes[brushnum];

		// see if the point is in the brush
		for ( i = 0 ; i < b->numsides ; i++ ) {
			d = DotProduct( p, b->sides[i].plane->normal );
			if ( d > b->sides[i].plane->dist ) {
				break;
			}
		}

		if ( i == b->numsides ) {
			contents |= b->contents;
		}
	}

	return contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	vec3_t p_l;
	vec3_t temp;
	vec3_t forward, right, up;

	// subtract origin offset
	VectorSubtract( p, origin, p_l );

	// rotate start and end into the models frame of reference
	if ( model != BOX_MODEL_HANDLE &&
		 ( angles[0] || angles[1] || angles[2] ) ) {
		AngleVectors( angles, forward, right, up );

		VectorCopy( p_l, temp );
		p_l[0] = DotProduct( temp, forward );
		p_l[1] = -DotProduct( temp, right );
		p_l[2] = DotProduct( temp, up );
	}

	return CM_PointContents( p_l, model );
}



/*
===============================================================================

PVS

===============================================================================
*/

uint8_t *CM_ClusterPVS( int cluster )
{
	const ClipModel& cm = TheClipModel::get();
	if ( cluster < 0 || cluster >= cm.numClusters || !cm.vised ) {
		return cm.visibility;
	}

	return cm.visibility + cluster * cm.clusterBytes;
}



/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void CM_FloodArea_r( int areaNum, int floodnum ) {

	cArea_t *area;


	ClipModel& cm = TheClipModel::get();
	area = &cm.areas[ areaNum ];

	if ( area->floodvalid == cm.floodvalid ) {
		if ( area->floodnum == floodnum ) {
			return;
		}
		Com_Error( ERR_DROP, "FloodArea_r: reflooded" );
        return; // keep the linter happy, ERR_DROP does not return
	}

	area->floodnum = floodnum;
	area->floodvalid = cm.floodvalid;
	int *con = cm.areaPortals + areaNum * cm.numAreas;
	for (int i = 0 ; i < cm.numAreas  ; i++ ) {
		if ( con[i] > 0 ) {
			CM_FloodArea_r( i, floodnum );
		}
	}
}

/*
====================
CM_FloodAreaConnections

====================
*/
void    CM_FloodAreaConnections( void ) {
	int i;
	cArea_t *area;
	int floodnum;

	ClipModel& cm = TheClipModel::get();
	// all current floods are now invalid
	cm.floodvalid++;
	floodnum = 0;

	area = cm.areas;    // Ridah, optimization
	for ( i = 0 ; i < cm.numAreas ; i++, area++ ) {
		if ( area->floodvalid == cm.floodvalid ) {
			continue;       // already flooded into
		}
		floodnum++;
		CM_FloodArea_r( i, floodnum );
	}

}

/*
====================
CM_AdjustAreaPortalState

====================
*/
void    CM_AdjustAreaPortalState( int area1, int area2, bool open ) {
	if ( area1 < 0 || area2 < 0 ) {
		return;
	}

	ClipModel& cm = TheClipModel::get();
	if ( area1 >= cm.numAreas || area2 >= cm.numAreas ) {
		Com_Error( ERR_DROP, "CM_ChangeAreaPortalState: bad area number" );
        return; // keep the linter happy, ERR_DROP does not return
	}

	if ( open ) {
		cm.areaPortals[ area1 * cm.numAreas + area2 ]++;
		cm.areaPortals[ area2 * cm.numAreas + area1 ]++;
	} else if ( cm.areaPortals[ area2 * cm.numAreas + area1 ] ) { // Ridah, fixes loadgame issue
		cm.areaPortals[ area1 * cm.numAreas + area2 ]--;
		cm.areaPortals[ area2 * cm.numAreas + area1 ]--;
		if ( cm.areaPortals[ area2 * cm.numAreas + area1 ] < 0 ) {
			Com_Error( ERR_DROP, "CM_AdjustAreaPortalState: negative reference count" );
            return; // keep the linter happy, ERR_DROP does not return
		}
	}

	CM_FloodAreaConnections();
}

/*
====================
CM_AreasConnected

====================
*/
bool    CM_AreasConnected( int area1, int area2 )
{
	if ( area1 < 0 || area2 < 0 ) {
		return false;
	}

	ClipModel& cm = TheClipModel::get();
	if ( area1 >= cm.numAreas || area2 >= cm.numAreas ) {
		Com_Error( ERR_DROP, "area >= cm.numAreas" );
        return false; // keep the linter happy, ERR_DROP does not return
	}

	if ( cm.areas[area1].floodnum == cm.areas[area2].floodnum ) {
		return true;
	}
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
int CM_WriteAreaBits( uint8_t *buffer, int area ) {
	int i;
	int floodnum;
	int bytes;

	const ClipModel& cm = TheClipModel::get();
	bytes = ( cm.numAreas + 7 ) >> 3;


	if ( area == -1 )
	{   // for debugging, send everything
		memset( buffer, 255, bytes );
	} else
	{
		floodnum = cm.areas[area].floodnum;
		for ( i = 0 ; i < cm.numAreas ; i++ )
		{
			if ( cm.areas[i].floodnum == floodnum || area == -1 ) {
				buffer[i >> 3] |= 1 << ( i & 7 );
			}
		}
	}

	return bytes;
}

