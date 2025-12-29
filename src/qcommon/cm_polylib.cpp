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


// this is only used for visualization tools in cm_ debug functions


#include "cm_local.h"


// counters are only bumped when running single threaded,
// because they are an awefull coherence problem
int c_active_windings;
int c_peak_windings;
int c_winding_allocs;
int c_winding_points;

/*
=============
AllocWinding
=============
*/
winding_t   *AllocWinding( int points )
{
	winding_t   *w = new winding_t();

	c_winding_allocs++;
	c_winding_points += points;
	c_active_windings++;
	if ( c_active_windings > c_peak_windings ) {
		c_peak_windings = c_active_windings;
	}

	w->numpoints = 0;
	w->allocatedPoints = points;
	w->p = new idVec3[points];
	return w;
}

void FreeWinding( winding_t *w ) {
	c_active_windings--;
	delete[] w->p;
	delete w;
}

/*
============
RemoveColinearPoints
============
*/
int c_removed;

void    RemoveColinearPoints( winding_t *w )
{
	idVec3 p[MAX_POINTS_ON_WINDING];

	int nump = 0;
	for (int i = 0; i < w->numpoints ; i++ )
	{
		int j = ( i + 1 ) % w->numpoints;
		int k = ( i + w->numpoints - 1 ) % w->numpoints;
		idVec3 v1 = w->p[j] - w->p[i];
		idVec3 v2 = w->p[i] - w->p[k];

		v1.Normalize();
		v2.Normalize();

		float dot = v1 * v2;
		if ( dot < 0.999 ) {
			p[nump] = w->p[i];
			nump++;
		}
	}

	if ( nump == w->numpoints ) {
		return;
	}

	c_removed += w->numpoints - nump;
	w->numpoints = nump;
	memcpy( w->p, p, nump * sizeof( p[0] ) );
}

/*
============
WindingPlane
============
*/
void WindingPlane( winding_t *w, idVec3& normal, float *dist )
{
	idVec3 v1 = w->p[1] - w->p[0];
	idVec3 v2 = w->p[2] - w->p[0];
	normal = v2.Cross( v1 );
	normal.Normalize();
	*dist = w->p[0] * normal;
}

float WindingArea( winding_t *w )
{
	float total = 0;
	for (int i = 2 ; i < w->numpoints ; i++ ) {
		idVec3 d1 = w->p[i - 1] - w->p[0];
		idVec3 d2 = w->p[i] - w->p[0];
		idVec3 cross = d1.Cross( d2 );
		total += 0.5 * cross.Length();
	}
	return total;
}

void    WindingBounds( winding_t *w, idVec3& mins, idVec3& maxs )
{
	mins.x = mins.y = mins.z = MAX_MAP_BOUNDS;
	maxs.x = maxs.y = maxs.z = -MAX_MAP_BOUNDS;

	for (int i = 0; i < w->numpoints ; i++ ) {
		for (int j = 0; j < 3 ; j++ ) {
			float v = w->p[i][j];
			if ( v < mins[j] ) {
				mins[j] = v;
			}
			if ( v > maxs[j] ) {
				maxs[j] = v;
			}
		}
	}
}

void    WindingCenter( winding_t *w, idVec3& center )
{
	center = id_vec3_origin;
	for (int i = 0 ; i < w->numpoints ; i++ ) {
		center += w->p[i];
	}

	float scale = 1.0 / w->numpoints;
	center *= scale;
}

winding_t *BaseWindingForPlane( const idVec3& normal, float dist )
{
	// find the major axis

	float max = -MAX_MAP_BOUNDS;
	int x = -1;
	for (int i = 0 ; i < 3; i++ ) {
		float v = fabs( normal[i] );
		if ( v > max ) {
			x = i;
			max = v;
		}
	}
	if ( x == -1 ) {
		Com_Error( ERR_DROP, "BaseWindingForPlane: no axis found" );
	}

	idVec3 vup(0.0);
	switch ( x )
	{
	case 0:
	case 1:
		vup.z = 1;
		break;
	case 2:
		vup[0] = 1;
		break;
	}

	float v = vup * normal;
	vup -= v * normal;
	vup.Normalize();

	idVec3 org = normal * dist;
	idVec3 vright = vup.Cross( normal );

	vup *= MAX_MAP_BOUNDS;
	vright *= MAX_MAP_BOUNDS;

	// project a really big	axis aligned box onto the plane
	winding_t *w = AllocWinding( 4 );

	w->p[0] = org - vright + vup;
	w->p[1] = org + vright + vup;
	w->p[2] = org + vright - vup;
	w->p[3] = org - vright - vup;
	w->numpoints = 4;
	return w;
}

winding_t   *CopyWinding( winding_t *w )
{
	winding_t   *c = AllocWinding( w->numpoints );
	int size = w->numpoints * sizeof( w->p[0] );
	memcpy( c->p, w->p, size );
	c->numpoints = w->numpoints;
	return c;
}

winding_t   *ReverseWinding( winding_t *w )
{
	winding_t* c = AllocWinding( w->numpoints );
	for ( int i = 0 ; i < w->numpoints ; i++ ) {
		c->p[i] = w->p[w->numpoints - 1 - i];
	}
	c->numpoints = w->numpoints;
	return c;
}

void ClipWindingEpsilon( winding_t *in, const idVec3& normal, float dist,
							float epsilon, winding_t **front, winding_t **back )
{
	float dists[MAX_POINTS_ON_WINDING + 4];
	int sides[MAX_POINTS_ON_WINDING + 4];
	int counts[3];
	counts[0] = counts[1] = counts[2] = 0;

	int i;
	// determine sides for each point
	for ( i = 0 ; i < in->numpoints ; i++ ) {
		float dot = in->p[i] * normal;
		dot -= dist;
		dists[i] = dot;
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*front = *back = nullptr;

	if ( !counts[0] ) {
		*back = CopyWinding( in );
		return;
	}
	if ( !counts[1] ) {
		*front = CopyWinding( in );
		return;
	}

	int maxpts = in->numpoints + 4;   // cant use counts[0]+2 because
								  // of fp grouping errors

	winding_t   *f = AllocWinding( maxpts );
	winding_t   *b = AllocWinding( maxpts );
	*front = f;
	*back = b;

	for ( i = 0 ; i < in->numpoints ; i++ ){
		idVec3& p1 = in->p[i];

		if ( sides[i] == SIDE_ON ) {
			f->p[f->numpoints] = p1;
			f->numpoints++;
			b->p[b->numpoints] = p1;
			b->numpoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			f->p[f->numpoints] = p1;
			f->numpoints++;
		}
		if ( sides[i] == SIDE_BACK ) {
			b->p[b->numpoints] = p1;
			b->numpoints++;
		}

		if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] ) {
			continue;
		}

		// generate a split point
		idVec3& p2 = in->p[( i + 1 ) % in->numpoints];

		float dot = dists[i] / ( dists[i] - dists[i + 1] );
		idVec3 mid;
		for (int j = 0 ; j < 3 ; j++ ) {
		   // avoid round off error when possible
			if ( normal[j] == 1 ) {
				mid[j] = dist;
			} else if ( normal[j] == -1 ) {
				mid[j] = -dist;
			} else {
				mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
			}
		}
		f->p[f->numpoints] = mid;
		f->numpoints++;
		b->p[b->numpoints] = mid;
		b->numpoints++;
	}

	if ( f->numpoints > maxpts || b->numpoints > maxpts ) {
		Com_Error( ERR_DROP, "ClipWinding: points exceeded estimate" );
	}
	if ( f->numpoints > MAX_POINTS_ON_WINDING || b->numpoints > MAX_POINTS_ON_WINDING ) {
		Com_Error( ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING" );
	}
}

void ChopWindingInPlace( winding_t **inout, const idVec3& normal, float dist, float epsilon )
{
	float dists[MAX_POINTS_ON_WINDING + 4] = { 0 };
	int sides[MAX_POINTS_ON_WINDING + 4] = { 0 };
	
	int maxpts;

	winding_t* in = *inout;

	int counts[3];
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	int i;
	for ( i = 0 ; i < in->numpoints ; i++ ) {
		float dot = in->p[i] * normal;
		dot -= dist;
		dists[i] = dot;
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	if ( !counts[0] ) {
		FreeWinding( in );
		*inout = nullptr;
		return;
	}
	if ( !counts[1] ) {
		return;     // inout stays the same

	}
	maxpts = in->numpoints + 4;   // cant use counts[0]+2 because
								  // of fp grouping errors

	winding_t   *f = AllocWinding( maxpts );
	for ( i = 0 ; i < in->numpoints ; i++ )
	{
		idVec3& p1 = in->p[i];

		if ( sides[i] == SIDE_ON ) {
			f->p[f->numpoints] = p1;
			f->numpoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			VectorCopy( p1, f->p[f->numpoints] );
			f->numpoints++;
		}

		if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] ) {
			continue;
		}

		// generate a split point
		idVec3& p2 = in->p[( i + 1 ) % in->numpoints];

		float dot = dists[i] / ( dists[i] - dists[i + 1] );
		idVec3 mid;
		for (int j = 0 ; j < 3 ; j++ ) {
		   // avoid round off error when possible
			if ( normal[j] == 1 ) {
				mid[j] = dist;
			} else if ( normal[j] == -1 ) {
				mid[j] = -dist;
			} else {
				mid[j] = p1[j] + dot * ( p2[j] - p1[j] );
			}
		}

		f->p[f->numpoints] = mid;
		f->numpoints++;
	}

	if ( f->numpoints > maxpts ) {
		Com_Error( ERR_DROP, "ClipWinding: points exceeded estimate" );
	}
	if ( f->numpoints > MAX_POINTS_ON_WINDING ) {
		Com_Error( ERR_DROP, "ClipWinding: MAX_POINTS_ON_WINDING" );
	}

	FreeWinding( in );
	*inout = f;
}


/*
=================
ChopWinding

Returns the fragment of in that is on the front side
of the cliping plane.  The original is freed.
=================
*/
winding_t   *ChopWinding( winding_t *in, const idVec3& normal, float dist )
{
	winding_t   *f, *b;

	ClipWindingEpsilon( in, normal, dist, ON_EPSILON, &f, &b );
	FreeWinding( in );
	if ( b ) {
		FreeWinding( b );
	}
	return f;
}


/*
=================
CheckWinding

=================
*/
void CheckWinding( winding_t *w )
{
	if ( w->numpoints < 3 ) {
		Com_Error( ERR_DROP, "CheckWinding: %i points",w->numpoints );
	}

	float area = WindingArea( w );
	if ( area < 1 ) {
		Com_Error( ERR_DROP, "CheckWinding: %f area", area );
	}

	idVec3 facenormal;
	float facedist;
	WindingPlane( w, facenormal, &facedist );

	for (int i = 0 ; i < w->numpoints ; i++ )
	{
		idVec3& p1 = w->p[i];

		for (int j = 0 ; j < 3 ; j++ ){
			if ( p1[j] > MAX_MAP_BOUNDS || p1[j] < -MAX_MAP_BOUNDS ) {
				Com_Error( ERR_DROP, "CheckFace: BUGUS_RANGE: %f",p1[j] );
			}
		}

		int j = i + 1 == w->numpoints ? 0 : i + 1;

		// check the point is on the face plane
		float d = p1 * facenormal - facedist;
		if ( d < -ON_EPSILON || d > ON_EPSILON ) {
			Com_Error( ERR_DROP, "CheckWinding: point off plane" );
		}

		// check the edge isnt degenerate
		idVec3& p2 = w->p[j];
		idVec3 dir = p2 - p1;

		if ( dir.Length() < ON_EPSILON ) {
			Com_Error( ERR_DROP, "CheckWinding: degenerate edge" );
		}

		idVec3 edgenormal = facenormal.Cross( dir );
		edgenormal.Normalize();
		float edgedist = p1 * edgenormal;
		edgedist += ON_EPSILON;

		// all other points must be on front side
		for ( j = 0 ; j < w->numpoints ; j++ ) {
			if ( j == i ) {
				continue;
			}
			d = w->p[j] * edgenormal;
			if ( d > edgedist ) {
				Com_Error( ERR_DROP, "CheckWinding: non-convex" );
			}
		}
	}
}


/*
============
WindingOnPlaneSide
============
*/
int WindingOnPlaneSide( winding_t *w, const idVec3& normal, float dist )
{
	bool front = false;
	bool back = false;
	for (int i = 0 ; i < w->numpoints ; i++ ) {
		float d = w->p[i] * normal - dist;
		if ( d < -ON_EPSILON ) {
			if ( front ) {
				return SIDE_CROSS;
			}
			back = true;
			continue;
		}
		if ( d > ON_EPSILON ) {
			if ( back ) {
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}

	if ( back ) {
		return SIDE_BACK;
	}
	if ( front ) {
		return SIDE_FRONT;
	}
	return SIDE_ON;
}


/*
=================
AddWindingToConvexHull

Both w and *hull are on the same plane
=================
*/
#define MAX_HULL_POINTS     128
void    AddWindingToConvexHull( winding_t *w, winding_t **hull, idVec3 normal )
{
	idVec3 hullPoints[MAX_HULL_POINTS];
	idVec3 newHullPoints[MAX_HULL_POINTS];
	idVec3 hullDirs[MAX_HULL_POINTS];
	bool hullSide[MAX_HULL_POINTS];

	if ( !*hull ) {
		*hull = CopyWinding( w );
		return;
	}

	int numHullPoints = ( *hull )->numpoints;
	memcpy( hullPoints, ( *hull )->p, numHullPoints * sizeof( idVec3 ) );

	for (int i = 0 ; i < w->numpoints ; i++ ) {
		idVec3& p = w->p[i];

		// calculate hull side vectors
		for (int j = 0 ; j < numHullPoints ; j++ ) {
			int k = ( j + 1 ) % numHullPoints;
			idVec3 dir = hullPoints[k] - hullPoints[j];
			dir.Normalize();
			hullDirs[j] = normal.Cross(dir);
		}

		bool outside = false;
		for (int j = 0 ; j < numHullPoints ; j++ ) {
			idVec3 dir = p - hullPoints[j];
			float d = dir * hullDirs[j];

			if ( d >= ON_EPSILON ) {
				outside = true;
			}
			if ( d >= -ON_EPSILON ) {
				hullSide[j] = true;
			} else {
				hullSide[j] = false;
			}
		}

		// if the point is effectively inside, do nothing
		if ( !outside ) {
			continue;
		}

		// find the back side to front side transition
		int j;
		for (j = 0 ; j < numHullPoints ; j++ ) {
			if ( !hullSide[ j % numHullPoints ] && hullSide[ ( j + 1 ) % numHullPoints ] ) {
				break;
			}
		}
		if ( j == numHullPoints ) {
			continue;
		}

		// insert the point here
		VectorCopy( p, newHullPoints[0] );
		int numNew = 1;

		// copy over all points that aren't double fronts
		j = ( j + 1 ) % numHullPoints;
		for (int k = 0 ; k < numHullPoints ; k++ ) {
			if ( hullSide[ ( j + k ) % numHullPoints ] && hullSide[ ( j + k + 1 ) % numHullPoints ] ) {
				continue;
			}
			idVec3& copy = hullPoints[ ( j + k + 1 ) % numHullPoints ];
			copy = newHullPoints[numNew];
			numNew++;
		}

		numHullPoints = numNew;
		memcpy( hullPoints, newHullPoints, numHullPoints * sizeof( idVec3 ) );
	}

	FreeWinding( *hull );
	w = AllocWinding( numHullPoints );
	w->numpoints = numHullPoints;
	*hull = w;
	memcpy( w->p, hullPoints, numHullPoints * sizeof( idVec3 ) );
}


