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


#include "tr_local.h"

trGlobals_t tr;

static float s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

// entities that will have procedurally generated surfaces will just
// point at this for their sorting surface
surfaceType_t entitySurface = SF_ENTITY;

// fog stuff
glfog_t glfogsettings[NUM_FOGS];
glfogType_t glfogNum = FOG_NONE;
bool fogIsOn = false;

void R_Fog( glfog_t *curfog ) {
	static glfog_t setfog;

	if ( !r_wolffog->integer ) {
		R_FogOff();
		return;
	}

	if ( !curfog->registered ) { 
		R_FogOff();
		return;
	}

	if ( !curfog->density ) {
		curfog->density = 1;
	}
	if ( !curfog->hint ) {
		curfog->hint = GL_DONT_CARE;
	}
	if ( !curfog->mode ) {
		curfog->mode = GL_LINEAR;
	}

	R_FogOn();

	qglFogi( GL_FOG_MODE, curfog->mode );
	qglFogfv( GL_FOG_COLOR, curfog->color );
	qglFogf( GL_FOG_DENSITY, curfog->density );
	qglHint( GL_FOG_HINT, curfog->hint );
	if ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
		qglFogf( GL_FOG_START, curfog->end );       // snooper starts GL fog out further
	} else {
		qglFogf( GL_FOG_START, curfog->start );
	}

	if ( r_zfar->value ) {             // (SA) allow override for helping level designers test fog distances
		qglFogf( GL_FOG_END, r_zfar->value );
	} else {
		if ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
			qglFogf( GL_FOG_END, curfog->end + 1000 );      // snooper ends GL fog out further.  this works fine with our maps, but could be 'funky' with later maps
		} else {
			qglFogf( GL_FOG_END, curfog->end );
		}
	}


	// NV fog mode
	if ( glConfig.NVFogAvailable ) {
		qglFogi( GL_FOG_DISTANCE_MODE_NV, glConfig.NVFogMode );
	}

	setfog.registered = true;

	qglClearColor( curfog->color[0], curfog->color[1], curfog->color[2], curfog->color[3] );
}

// Ridah, allow disabling fog temporarily
void R_FogOff( void )
{
	if ( !fogIsOn ) {
		return;
	}
	qglDisable( GL_FOG );
	fogIsOn = false;
}

void R_FogOn( void )
{
	if ( fogIsOn ) {
		return;
	}

	if ( backEnd.projection2D ) {  // no fog in 2d
		R_FogOff();
		return;
	}

	if ( !r_wolffog->integer ) {
		return;
	}


	if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) { // don't force world fog on portal sky
		if ( !( glfogsettings[FOG_PORTALVIEW].registered ) ) {
			return;
		}
	} else if ( !glfogNum )     {
		return;
	}

	qglEnable( GL_FOG );
	fogIsOn = true;
}

/*
==============
R_SetFog

  if fogvar == FOG_CMD_SWITCHFOG {
	fogvar is the command
	var1 is the fog to switch to
	var2 is the time to transition
  }
  else {
	fogvar is the fog that's being set
	var1 is the near fog z value
	var2 is the far fog z value
	rgb = color
	density is density, and is used to derive the values of 'mode', 'drawsky', and 'clearscreen'
  }
==============
*/
void R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density ) {
	if ( fogvar != FOG_CMD_SWITCHFOG ) {   // just set the parameters and return

		if ( var1 == 0 && var2 == 0 ) {    // clear this fog
			glfogsettings[fogvar].registered = false;
			return;
		}

		glfogsettings[fogvar].color[0]      = r;
		glfogsettings[fogvar].color[1]      = g;
		glfogsettings[fogvar].color[2]      = b;
		glfogsettings[fogvar].color[3]      = 1;
		glfogsettings[fogvar].start         = var1;
		glfogsettings[fogvar].end           = var2;
		if ( density >= 1 ) {
			glfogsettings[fogvar].mode          = GL_LINEAR;
			glfogsettings[fogvar].drawsky       = false;
			glfogsettings[fogvar].clearscreen   = 1;
			glfogsettings[fogvar].density       = 1.0;
		} else
		{
			glfogsettings[fogvar].mode          = GL_EXP;
			glfogsettings[fogvar].drawsky       = true;
			glfogsettings[fogvar].clearscreen   = 0;
			glfogsettings[fogvar].density       = density;
		}
		glfogsettings[fogvar].hint          = GL_DONT_CARE;
		glfogsettings[fogvar].registered    = true;

		return;
	}

	// FOG_MAP now used to mean 'no fog'
	if ( var1 == FOG_MAP ) {

		// transitioning from...
		if ( glfogsettings[FOG_CURRENT].registered ) {
			memcpy( &glfogsettings[FOG_LAST], &glfogsettings[FOG_CURRENT], sizeof( glfog_t ) );
		}
		memcpy( &glfogsettings[FOG_TARGET], &glfogsettings[glfogNum], sizeof( glfog_t ) );

		// clear, clear, clear
		memset( &glfogsettings[FOG_MAP], 0, sizeof( glfog_t ) );
		memset( &glfogsettings[FOG_TARGET], 0, sizeof( glfog_t ) );
		glfogNum = FOG_NONE;
		return;
	}

	// don't switch to invalid fogs
	if ( glfogsettings[var1].registered != true ) {
		return;
	}


	glfogNum = (glfogType_t)var1;

	// transitioning to new fog, store the current values as the 'from'

	if ( glfogsettings[FOG_CURRENT].registered ) {
		memcpy( &glfogsettings[FOG_LAST], &glfogsettings[FOG_CURRENT], sizeof( glfog_t ) );
	} else {
		// if no current fog fall back to world fog
		// FIXME: handle transition if there is no FOG_MAP fog
//		memcpy(&glfogsettings[FOG_LAST], &glfogsettings[FOG_MAP], sizeof(glfog_t));
		memcpy( &glfogsettings[FOG_LAST], &glfogsettings[glfogNum], sizeof( glfog_t ) );
	}

	memcpy( &glfogsettings[FOG_TARGET], &glfogsettings[glfogNum], sizeof( glfog_t ) );

	if ( !var2 ) { // instant
		glfogsettings[FOG_TARGET].startTime = 0;
		glfogsettings[FOG_TARGET].finishTime = 0;
		glfogsettings[FOG_TARGET].dirty = 1;
		glfogsettings[FOG_CURRENT].dirty = 1;
	} else {
		// setup transition times
		glfogsettings[FOG_TARGET].startTime = tr.refdef.time;
		glfogsettings[FOG_TARGET].finishTime = tr.refdef.time + var2;
	}
}

/*
=================
R_CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLocalBox( vec3_t bounds[2] )
{
	vec3_t transformed[8];
	
	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// transform into world space
	for (int i = 0 ; i < 8 ; i++ ) {
		vec3_t v;
		v[0] = bounds[i & 1][0];
		v[1] = bounds[( i >> 1 ) & 1][1];
		v[2] = bounds[( i >> 2 ) & 1][2];

		VectorCopy( tr.orientation.origin, transformed[i] );
		VectorMA( transformed[i], v[0], tr.orientation.axis[0], transformed[i] );
		VectorMA( transformed[i], v[1], tr.orientation.axis[1], transformed[i] );
		VectorMA( transformed[i], v[2], tr.orientation.axis[2], transformed[i] );
	}

	// check against frustum planes
	int anyBack = 0;
	for (int i = 0 ; i < 4 ; i++ ) {
		cplane_t* frust = &tr.viewParms.frustum[i];

		int front = 0;
		int back = 0;
		float dists[8];
		for (int j = 0 ; j < 8 ; j++ ) {
			dists[j] = DotProduct( transformed[j], frust->normal );
			if ( dists[j] > frust->dist ) {
				front = 1;
				if ( back ) {
					break;      // a point is in front
				}
			} else {
				back = 1;
			}
		}
		if ( !front ) {
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if ( !anyBack ) {
		return CULL_IN;     // completely inside frustum
	}

	return CULL_CLIP;       // partially clipped
}

int R_CullLocalPointAndRadius( vec3_t pt, float radius )
{
	vec3_t transformed;
	R_LocalPointToWorld( pt, transformed );

	return R_CullPointAndRadius( transformed, radius );
}

int R_CullPointAndRadius( vec3_t pt, float radius )
{
	bool mightBeClipped = false;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// check against frustum planes
	for (int i = 0 ; i < 4 ; i++ )
	{
		cplane_t* frust = &tr.viewParms.frustum[i];

		float dist = DotProduct( pt, frust->normal ) - frust->dist;
		if ( dist < -radius ) {
			return CULL_OUT;
		} else if ( dist <= radius )   {
			mightBeClipped = true;
		}
	}

	if ( mightBeClipped ) {
		return CULL_CLIP;
	}

	return CULL_IN;     // completely inside frustum
}

void R_LocalNormalToWorld( vec3_t local, vec3_t world ) {
	world[0] = local[0] * tr.orientation.axis[0][0] + local[1] * tr.orientation.axis[1][0] + local[2] * tr.orientation.axis[2][0];
	world[1] = local[0] * tr.orientation.axis[0][1] + local[1] * tr.orientation.axis[1][1] + local[2] * tr.orientation.axis[2][1];
	world[2] = local[0] * tr.orientation.axis[0][2] + local[1] * tr.orientation.axis[1][2] + local[2] * tr.orientation.axis[2][2];
}

void R_LocalPointToWorld( vec3_t local, vec3_t world ) {
	world[0] = local[0] * tr.orientation.axis[0][0] + local[1] * tr.orientation.axis[1][0] + local[2] * tr.orientation.axis[2][0] + tr.orientation.origin[0];
	world[1] = local[0] * tr.orientation.axis[0][1] + local[1] * tr.orientation.axis[1][1] + local[2] * tr.orientation.axis[2][1] + tr.orientation.origin[1];
	world[2] = local[0] * tr.orientation.axis[0][2] + local[1] * tr.orientation.axis[1][2] + local[2] * tr.orientation.axis[2][2] + tr.orientation.origin[2];
}

void R_WorldToLocal( vec3_t world, vec3_t local ) {
	local[0] = DotProduct( world, tr.orientation.axis[0] );
	local[1] = DotProduct( world, tr.orientation.axis[1] );
	local[2] = DotProduct( world, tr.orientation.axis[2] );
}


void R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							 vec4_t eye, vec4_t dst )
{
	for (int i = 0 ; i < 4 ; i++ ) {
		eye[i] =
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];
	}

	for (int i = 0 ; i < 4 ; i++ ) {
		dst[i] =
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window )
{
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) ( window[0] + 0.5 );
	window[1] = (int) ( window[1] + 0.5 );
}

void myGlMultMatrix( const float *a, const float *b, float *out )
{
	for (int i = 0 ; i < 4 ; i++ ) {
		for (int j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms,
						orientationr_t *orientation ) {
	float glMatrix[16];
	vec3_t delta;
	float axisLength;

	if ( ent->e.reType != RT_MODEL ) {
		*orientation = viewParms->world;
		return;
	}

	VectorCopy( ent->e.origin, orientation->origin );

	VectorCopy( ent->e.axis[0], orientation->axis[0] );
	VectorCopy( ent->e.axis[1], orientation->axis[1] );
	VectorCopy( ent->e.axis[2], orientation->axis[2] );

	glMatrix[0] = orientation->axis[0][0];
	glMatrix[4] = orientation->axis[1][0];
	glMatrix[8] = orientation->axis[2][0];
	glMatrix[12] = orientation->origin[0];

	glMatrix[1] = orientation->axis[0][1];
	glMatrix[5] = orientation->axis[1][1];
	glMatrix[9] = orientation->axis[2][1];
	glMatrix[13] = orientation->origin[1];

	glMatrix[2] = orientation->axis[0][2];
	glMatrix[6] = orientation->axis[1][2];
	glMatrix[10] = orientation->axis[2][2];
	glMatrix[14] = orientation->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, orientation->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->orientation.origin, orientation->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->e.nonNormalizedAxes ) {
		axisLength = VectorLength( ent->e.axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0f / axisLength;
		}
	} else {
		axisLength = 1.0f;
	}

	orientation->viewOrigin[0] = DotProduct( delta, orientation->axis[0] ) * axisLength;
	orientation->viewOrigin[1] = DotProduct( delta, orientation->axis[1] ) * axisLength;
	orientation->viewOrigin[2] = DotProduct( delta, orientation->axis[2] ) * axisLength;
}

/*
=================
R_RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/
void R_RotateForViewer( void ) {
	float viewerMatrix[16];
	vec3_t origin;

	memset( &tr.orientation, 0, sizeof( tr.orientation ) );
	tr.orientation.axis[0][0] = 1;
	tr.orientation.axis[1][1] = 1;
	tr.orientation.axis[2][2] = 1;
	VectorCopy( tr.viewParms.orientation.origin, tr.orientation.viewOrigin );

	// transform by the camera placement
	VectorCopy( tr.viewParms.orientation.origin, origin );

	viewerMatrix[0] = tr.viewParms.orientation.axis[0][0];
	viewerMatrix[4] = tr.viewParms.orientation.axis[0][1];
	viewerMatrix[8] = tr.viewParms.orientation.axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + - origin[1] * viewerMatrix[4] + - origin[2] * viewerMatrix[8];

	viewerMatrix[1] = tr.viewParms.orientation.axis[1][0];
	viewerMatrix[5] = tr.viewParms.orientation.axis[1][1];
	viewerMatrix[9] = tr.viewParms.orientation.axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + - origin[1] * viewerMatrix[5] + - origin[2] * viewerMatrix[9];

	viewerMatrix[2] = tr.viewParms.orientation.axis[2][0];
	viewerMatrix[6] = tr.viewParms.orientation.axis[2][1];
	viewerMatrix[10] = tr.viewParms.orientation.axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + - origin[1] * viewerMatrix[6] + - origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( viewerMatrix, s_flipMatrix, tr.orientation.modelMatrix );

	tr.viewParms.world = tr.orientation;

}

void R_SetFrameFog( void )
{
	if ( r_speeds->integer == 5 ) {
		if ( !glfogsettings[FOG_TARGET].registered ) {
			Com_Printf("no fog - calc zFar: %0.1f\n", tr.viewParms.zFar );
			return;
		}
	}

	// still fading
	if ( glfogsettings[FOG_TARGET].finishTime && glfogsettings[FOG_TARGET].finishTime >= tr.refdef.time ) {
		float lerpPos;
		int fadeTime;

		// transitioning from density to distance
		if ( glfogsettings[FOG_LAST].mode == GL_EXP && glfogsettings[FOG_TARGET].mode == GL_LINEAR ) {
			// for now just fast transition to the target when dissimilar fogs are
			memcpy( &glfogsettings[FOG_CURRENT], &glfogsettings[FOG_TARGET], sizeof( glfog_t ) );
			glfogsettings[FOG_TARGET].finishTime = 0;
		}
		// transitioning from distance to density
		else if ( glfogsettings[FOG_LAST].mode == GL_LINEAR && glfogsettings[FOG_TARGET].mode == GL_EXP ) {
			memcpy( &glfogsettings[FOG_CURRENT], &glfogsettings[FOG_TARGET], sizeof( glfog_t ) );
			glfogsettings[FOG_TARGET].finishTime = 0;
		}
		// transitioning like fog modes
		else {

			fadeTime = glfogsettings[FOG_TARGET].finishTime - glfogsettings[FOG_TARGET].startTime;
			if ( fadeTime <= 0 ) {
				fadeTime = 1;   // avoid divide by zero


			}
			lerpPos = (float)( tr.refdef.time - glfogsettings[FOG_TARGET].startTime ) / (float)fadeTime;
			if ( lerpPos > 1 ) {
				lerpPos = 1;
			}

			// lerp near/far
			glfogsettings[FOG_CURRENT].start        = glfogsettings[FOG_LAST].start + ( ( glfogsettings[FOG_TARGET].start - glfogsettings[FOG_LAST].start ) * lerpPos );
			glfogsettings[FOG_CURRENT].end          = glfogsettings[FOG_LAST].end + ( ( glfogsettings[FOG_TARGET].end - glfogsettings[FOG_LAST].end ) * lerpPos );

			// lerp color
			glfogsettings[FOG_CURRENT].color[0]     = glfogsettings[FOG_LAST].color[0] + ( ( glfogsettings[FOG_TARGET].color[0] - glfogsettings[FOG_LAST].color[0] ) * lerpPos );
			glfogsettings[FOG_CURRENT].color[1]     = glfogsettings[FOG_LAST].color[1] + ( ( glfogsettings[FOG_TARGET].color[1] - glfogsettings[FOG_LAST].color[1] ) * lerpPos );
			glfogsettings[FOG_CURRENT].color[2]     = glfogsettings[FOG_LAST].color[2] + ( ( glfogsettings[FOG_TARGET].color[2] - glfogsettings[FOG_LAST].color[2] ) * lerpPos );

			glfogsettings[FOG_CURRENT].density      = glfogsettings[FOG_TARGET].density;
			glfogsettings[FOG_CURRENT].mode         = glfogsettings[FOG_TARGET].mode;
			glfogsettings[FOG_CURRENT].registered   = true;

			// if either fog in the transition clears the screen, clear the background this frame to avoid hall of mirrors
			glfogsettings[FOG_CURRENT].clearscreen  = ( glfogsettings[FOG_TARGET].clearscreen || glfogsettings[FOG_LAST].clearscreen );
		}

		glfogsettings[FOG_CURRENT].dirty = 1;
	} else {
		// potential FIXME: since this is the most common occurance, diff first and only set changes
		memcpy( &glfogsettings[FOG_CURRENT], &glfogsettings[FOG_TARGET], sizeof( glfog_t ) );
		glfogsettings[FOG_CURRENT].dirty = 0;
	}


	// shorten the far clip if the fog opaque distance is closer than the procedural farcip dist

	if ( glfogsettings[FOG_CURRENT].mode == GL_LINEAR ) {
		if ( glfogsettings[FOG_CURRENT].end < tr.viewParms.zFar ) {
			tr.viewParms.zFar = glfogsettings[FOG_CURRENT].end;
		}
		if ( backEnd.refdef.rdflags & RDF_SNOOPERVIEW ) {
			tr.viewParms.zFar += 1000;  // zfar out slightly further for snooper.  this works fine with our maps, but could be 'funky' with later maps

		}
	}


	if ( r_speeds->integer == 5 ) {
		if ( glfogsettings[FOG_CURRENT].mode == GL_LINEAR ) {
			Com_Printf("farclip fog - den: %0.1f  calc zFar: %0.1f  fog zfar: %0.1f\n", glfogsettings[FOG_CURRENT].density, tr.viewParms.zFar, glfogsettings[FOG_CURRENT].end );
		} else {
			Com_Printf("density fog - den: %0.6f  calc zFar: %0.1f  fog zFar: %0.1f\n", glfogsettings[FOG_CURRENT].density, tr.viewParms.zFar, glfogsettings[FOG_CURRENT].end );
		}
	}
}


static void SetFarClip()
{
	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		tr.viewParms.zFar = 2048;
		return;
	}

	//----(SA)	this lets you use r_zfar from the command line to experiment with different
	//			distances, but setting it back to 0 uses the map (or procedurally generated) default
	if ( r_zfar->value ) {

		tr.viewParms.zFar = r_zfar->integer;
		R_SetFrameFog();

		if ( r_speeds->integer == 5 ) {
			Com_Printf("r_zfar value forcing farclip at: %f\n", tr.viewParms.zFar );
		}

		return;
	}

	//
	// set far clipping planes dynamically
	//
	float farthestCornerDistance = 0;
	for (int i = 0; i < 8; i++ )
	{
		vec3_t v;
		vec3_t vecTo;
		float distance;

		if ( i & 1 ) {
			v[0] = tr.viewParms.visBounds[0][0];
		} else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 ) {
			v[1] = tr.viewParms.visBounds[0][1];
		} else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 ) {
			v[2] = tr.viewParms.visBounds[0][2];
		} else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		VectorSubtract( v, tr.viewParms.orientation.origin, vecTo );

		distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if ( distance > farthestCornerDistance ) {
			farthestCornerDistance = distance;
		}
	}

	tr.viewParms.zFar = sqrt( farthestCornerDistance );
	R_SetFrameFog();
}


void R_SetupProjection()
{
	// dynamically compute far clip plane distance
	SetFarClip();

	//
	// set up projection matrix
	//
	float zNear   = r_znear->value;
	float zFar;
	if ( r_zfar->value ) {
		zFar = r_zfar->value;   // (SA) allow override for helping level designers test fog distances
	} else {
		zFar = tr.viewParms.zFar;
	}

	float ymax = zNear * tan( tr.refdef.fov_y * M_PI / 360.0f );
	float ymin = -ymax;

	float xmax = zNear * tan( tr.refdef.fov_x * M_PI / 360.0f );
	float xmin = -xmax;

	float width = xmax - xmin;
	float height = ymax - ymin;
	float depth = zFar - zNear;

	tr.viewParms.projectionMatrix[0] = 2 * zNear / width;
	tr.viewParms.projectionMatrix[4] = 0;
	tr.viewParms.projectionMatrix[8] = ( xmax + xmin ) / width; // normally 0
	tr.viewParms.projectionMatrix[12] = 0;

	tr.viewParms.projectionMatrix[1] = 0;
	tr.viewParms.projectionMatrix[5] = 2 * zNear / height;
	tr.viewParms.projectionMatrix[9] = ( ymax + ymin ) / height;    // normally 0
	tr.viewParms.projectionMatrix[13] = 0;

	tr.viewParms.projectionMatrix[2] = 0;
	tr.viewParms.projectionMatrix[6] = 0;
	tr.viewParms.projectionMatrix[10] = -( zFar + zNear ) / depth;
	tr.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[3] = 0;
	tr.viewParms.projectionMatrix[7] = 0;
	tr.viewParms.projectionMatrix[11] = -1;
	tr.viewParms.projectionMatrix[15] = 0;
}

/*
=================
R_SetupFrustum

Setup that culling frustum planes for the current view
=================
*/
void R_SetupFrustum()
{	
	float ang = tr.viewParms.fovX / 180 * M_PI * 0.5f;
	float xs = sin( ang );
	float xc = cos( ang );

	VectorScale( tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[0].normal );
	VectorMA( tr.viewParms.frustum[0].normal, xc, tr.viewParms.orientation.axis[1], tr.viewParms.frustum[0].normal );

	VectorScale( tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[1].normal );
	VectorMA( tr.viewParms.frustum[1].normal, -xc, tr.viewParms.orientation.axis[1], tr.viewParms.frustum[1].normal );

	ang = tr.viewParms.fovY / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[2].normal );
	VectorMA( tr.viewParms.frustum[2].normal, xc, tr.viewParms.orientation.axis[2], tr.viewParms.frustum[2].normal );

	VectorScale( tr.viewParms.orientation.axis[0], xs, tr.viewParms.frustum[3].normal );
	VectorMA( tr.viewParms.frustum[3].normal, -xc, tr.viewParms.orientation.axis[2], tr.viewParms.frustum[3].normal );

	for (int i = 0 ; i < 4 ; i++ ) {
		tr.viewParms.frustum[i].type = PLANE_NON_AXIAL;
		tr.viewParms.frustum[i].dist = DotProduct( tr.viewParms.orientation.origin, tr.viewParms.frustum[i].normal );
		SetPlaneSignbits( &tr.viewParms.frustum[i] );
	}
}


void R_MirrorPoint( vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out )
{	
	vec3_t local;
	vec3_t transformed;
	
	VectorSubtract( in, surface->origin, local );
	VectorClear( transformed );
	for (int i = 0 ; i < 3 ; i++ ) {
		float d = DotProduct( local, surface->axis[i] );
		VectorMA( transformed, d, camera->axis[i], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}

void R_MirrorVector( vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out )
{
	VectorClear( out );
	for (int i = 0 ; i < 3 ; i++ ) {
		float d = DotProduct( in, surface->axis[i] );
		VectorMA( out, d, camera->axis[i], out );
	}
}

void R_PlaneForSurface( surfaceType_t *surfType, cplane_t *plane )
{
	if ( !surfType ) {
		memset( plane, 0, sizeof( *plane ) );
		plane->normal[0] = 1;
		return;
	}

	switch ( *surfType ) {
	case SF_FACE:
		*plane = ( (srfSurfaceFace_t *)surfType )->plane;
		break;
	case SF_TRIANGLES:
	{
		srfTriangles_t* tri = (srfTriangles_t *)surfType;
		drawVert_t* v1 = tri->verts + tri->indexes[0];
		drawVert_t* v2 = tri->verts + tri->indexes[1];
		drawVert_t* v3 = tri->verts + tri->indexes[2];
		vec4_t plane4;
		PlaneFromPoints( plane4, v1->xyz, v2->xyz, v3->xyz );
		VectorCopy( plane4, plane->normal );
		plane->dist = plane4[3];
	}
		break;
	case SF_POLY:
	{
		srfPoly_t* poly = (srfPoly_t *)surfType;
		vec4_t plane4;
		PlaneFromPoints( plane4, poly->verts[0].xyz, poly->verts[1].xyz, poly->verts[2].xyz );
		VectorCopy( plane4, plane->normal );
		plane->dist = plane4[3];
	}
		break;
	default:
		memset( plane, 0, sizeof( *plane ) );
		plane->normal[0] = 1;
		break;
	}
}

/*
=================
R_GetPortalOrientation

entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns true if it should be mirrored
=================
*/
bool R_GetPortalOrientations( drawSurf_t *drawSurf, int entityNum,
								  orientation_t *surface, orientation_t *camera,
								  vec3_t pvsOrigin, bool *mirror ) {
	int i;
	cplane_t originalPlane, plane;
	trRefEntity_t   *e;
	float d;
	vec3_t transformed;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != ENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orientation );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orientation.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orientation.origin );
	} else {
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[0] );
	PerpendicularVector( surface->axis[1], surface->axis[0] );
	CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ ) {
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64 ) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( e->e.oldorigin, pvsOrigin );

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] &&
			 e->e.oldorigin[1] == e->e.origin[1] &&
			 e->e.oldorigin[2] == e->e.origin[2] ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
			VectorCopy( surface->axis[1], camera->axis[1] );
			VectorCopy( surface->axis[2], camera->axis[2] );

			*mirror = true;
			return true;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
		VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );

		// now get the camera origin and orientation
		VectorCopy( e->e.oldorigin, camera->origin );
		AxisCopy( e->e.axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
		VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

		// optionally rotate
		if ( e->e.oldframe ) {
			// if a speed is specified
			if ( e->e.frame ) {
				// continuous rotate
				d = ( tr.refdef.time / 1000.0f ) * e->e.frame;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( tr.refdef.time * 0.003f );
				d = e->e.skinNum + d * 4;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			}
		} else if ( e->e.skinNum )   {
			d = e->e.skinNum;
			VectorCopy( camera->axis[1], transformed );
			RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		}
		*mirror = false;
		return true;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//Com_Printf("Portal surface without a portal entity\n" );

	return false;
}

static bool IsMirror( const drawSurf_t *drawSurf, int entityNum ) {
	int i;
	cplane_t originalPlane, plane;
	trRefEntity_t   *e;
	float d;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->surface, &originalPlane );

	// rotate the plane if necessary
	if ( entityNum != ENTITYNUM_WORLD ) {
		tr.currentEntityNum = entityNum;
		tr.currentEntity = &tr.refdef.entities[entityNum];

		// get the orientation of the entity
		R_RotateForEntity( tr.currentEntity, &tr.viewParms, &tr.orientation );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.orientation.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.orientation.origin );
	} else
	{
		plane = originalPlane;
	}

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( i = 0 ; i < tr.refdef.num_entities ; i++ )
	{
		e = &tr.refdef.entities[i];
		if ( e->e.reType != RT_PORTALSURFACE ) {
			continue;
		}

		d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64 ) {
			continue;
		}

		// if the entity is just a mirror, don't use as a camera point
		if ( e->e.oldorigin[0] == e->e.origin[0] &&
			 e->e.oldorigin[1] == e->e.origin[1] &&
			 e->e.oldorigin[2] == e->e.origin[2] ) {
			return true;
		}

		return false;
	}
	return false;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static bool SurfIsOffscreen( const drawSurf_t *drawSurf, vec4_t clipDest[128] )
{
	float shortest = 100000000;
	int entityNum;
	shader_t *shader;
	int fogNum;
	int dlighted;
	int atiTess;
	
	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	if ( glConfig.smpActive ) {     // FIXME!  we can't do RB_BeginSurface/RB_EndSurface stuff with smp!
		return false;
	}

	R_RotateForViewer();

	R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &atiTess );
	RB_BeginSurface( shader, fogNum );
	rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

	assert( tess.numVertexes < 128 );

	for (int i = 0; i < tess.numVertexes; i++ )
	{
		unsigned int pointFlags = 0;
		vec4_t clip, eye;
		R_TransformModelToClip( tess.xyz[i], tr.orientation.modelMatrix, tr.viewParms.projectionMatrix, eye, clip );

		for (int j = 0; j < 3; j++ )
		{
			if ( clip[j] >= clip[3] ) {
				pointFlags |= ( 1 << ( j * 2 ) );
			} else if ( clip[j] <= -clip[3] )   {
				pointFlags |= ( 1 << ( j * 2 + 1 ) );
			}
		}
		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd ) {
		return true;
	}

	// determine if this surface is backfaced and also determine the distance
	// to the nearest vertex so we can cull based on portal range.  Culling
	// based on vertex distance isn't 100% correct (we should be checking for
	// range to the surface), but it's good enough for the types of portals
	// we have in the game right now.
	int numTriangles = tess.numIndexes / 3;

	for (int i = 0; i < tess.numIndexes; i += 3 )
	{
		vec3_t normal;

		VectorSubtract( tess.xyz[tess.indexes[i]], tr.viewParms.orientation.origin, normal );

		float len = VectorLengthSquared( normal );            // lose the sqrt
		if ( len < shortest ) {
			shortest = len;
		}

		float dot = DotProduct( normal, tess.normal[tess.indexes[i]] );
		if ( dot >= 0 ) {
			numTriangles--;
		}
	}
	if ( !numTriangles ) {
		return true;
	}

	// mirrors can early out at this point, since we don't do a fade over distance
	// with them (although we could)
	if ( IsMirror( drawSurf, entityNum ) ) {
		return false;
	}

	if ( shortest > ( tess.shader->portalRange * tess.shader->portalRange ) ) {
		return true;
	}

	return false;
}

/*
========================
R_MirrorViewBySurface

Returns true if another view has been rendered
========================
*/
bool R_MirrorViewBySurface( drawSurf_t *drawSurf, int entityNum ) {
	vec4_t clipDest[128];
	viewParms_t newParms;
	viewParms_t oldParms;
	orientation_t surface, camera;

	// don't recursively mirror
	if ( tr.viewParms.isPortal ) {
		Com_Printf(S_COLOR_RED  "WARNING: recursive mirror/portal found\n" );
		return false;
	}

	if ( r_noportals->integer || r_fastsky->integer ) {
		return false;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf, clipDest ) ) {
		return false;
	}

	// save old viewParms so we can return to it after the mirror view
	oldParms = tr.viewParms;

	newParms = tr.viewParms;
	newParms.isPortal = true;
	if ( !R_GetPortalOrientations( drawSurf, entityNum, &surface, &camera,
								   newParms.pvsOrigin, &newParms.isMirror ) ) {
		return false;      // bad portal, no portalentity
	}

	R_MirrorPoint( oldParms.orientation.origin, &surface, &camera, newParms.orientation.origin );

	VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );

	R_MirrorVector( oldParms.orientation.axis[0], &surface, &camera, newParms.orientation.axis[0] );
	R_MirrorVector( oldParms.orientation.axis[1], &surface, &camera, newParms.orientation.axis[1] );
	R_MirrorVector( oldParms.orientation.axis[2], &surface, &camera, newParms.orientation.axis[2] );

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView( &newParms );

	tr.viewParms = oldParms;

	return true;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_SpriteFogNum( trRefEntity_t *ent ) 
{
	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for (int i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog_t *fog = &tr.world->fogs[i];
		int j;
		for ( j = 0 ; j < 3 ; j++ ) {
			if ( ent->e.origin[j] - ent->e.radius >= fog->bounds[1][j] ) {
				break;
			}
			if ( ent->e.origin[j] + ent->e.radius <= fog->bounds[0][j] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return 0;
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

static inline void R_Radix( int key, int size, drawSurf_t *source, drawSurf_t *dest )
{
  int           count[ 256 ] = { 0 };
  int           index[ 256 ];

  unsigned char* sortKey = ( (unsigned char *)&source[ 0 ].sort ) + key;
  unsigned char* end = sortKey + ( size * sizeof( drawSurf_t ) );
  for( ; sortKey < end; sortKey += sizeof( drawSurf_t ) ){
    ++count[ *sortKey ];
  }
  index[ 0 ] = 0;

  for(int i = 1; i < 256; ++i ){
    index[ i ] = index[ i - 1 ] + count[ i - 1 ];
  }
  sortKey = ( (unsigned char *)&source[ 0 ].sort ) + key;
  for(int i = 0; i < size; ++i, sortKey += sizeof( drawSurf_t ) ){
    dest[ index[ *sortKey ]++ ] = source[ i ];
  }
}

/*
===============
R_RadixSort

Radix sort with 4 uint8_t size buckets
===============
*/
static void R_RadixSort( drawSurf_t *source, int size )
{
  static drawSurf_t scratch[ MAX_DRAWSURFS ];

  R_Radix( 0, size, source, scratch );
  R_Radix( 1, size, scratch, source );
  R_Radix( 2, size, source, scratch );
  R_Radix( 3, size, scratch, source );
}

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader,
					int fogIndex, int dlightMap, int atiTess )
{
	// instead of checking for overflow, we just mask the index
	// so it wraps around
	int index = tr.refdef.numDrawSurfs & DRAWSURF_MASK;
	// the sort data is packed into a single 32 bit value so it can be
	// compared quickly during the qsorting process
	tr.refdef.drawSurfs[index].sort = ( shader->sortedIndex << QSORT_SHADERNUM_SHIFT )
									  | ( atiTess << QSORT_ATI_TESS_SHIFT )
									  | tr.shiftedEntityNum | ( fogIndex << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
	tr.refdef.drawSurfs[index].surface = surface;
	tr.refdef.numDrawSurfs++;
}


void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader,
					  int *fogNum, int *dlightMap, int *atiTess )
{
	*fogNum = ( sort >> QSORT_FOGNUM_SHIFT ) & 31;
	*shader = tr.sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & ( MAX_SHADERS - 1 ) ];
	*entityNum = ( sort >> QSORT_ENTITYNUM_SHIFT ) & ( MAX_GENTITIES - 1 );
	*dlightMap = sort & 3;
	*atiTess = ( sort >> QSORT_ATI_TESS_SHIFT ) & 1;
}

void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	// it is possible for some views to not have any surfaces
	if ( numDrawSurfs < 1 ) {
		// we still need to add it for hyperspace cases
		R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
		return;
	}

	// if we overflowed MAX_DRAWSURFS, the drawsurfs
	// wrapped around in the buffer and we will be missing
	// the first surfaces, not the last ones
	if ( numDrawSurfs > MAX_DRAWSURFS ) {
		numDrawSurfs = MAX_DRAWSURFS;
	}

	// sort the drawsurfs by sort type, then orientation, then shader
	R_RadixSort( drawSurfs, numDrawSurfs );

	// check for any pass through drawing, which
	// may cause another view to be rendered first
	for (int i = 0 ; i < numDrawSurfs ; i++ ) {
		shader_t *shader;
		int fogNum;
		int entityNum;
		int dlighted;
		int atiTess;
		R_DecomposeSort( ( drawSurfs + i )->sort, &entityNum, &shader, &fogNum, &dlighted, &atiTess );

		if ( shader->sort > SS_PORTAL ) {
			break;
		}

		// no shader should ever have this sort type
		if ( shader->sort == SS_BAD ) {
			Com_Error( ERR_DROP, "Shader '%s'with sort == SS_BAD", shader->name );
		}

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( ( drawSurfs + i ), entityNum ) ) {
			// this is a debug option to see exactly what is being mirrored
			if ( r_portalOnly->integer ) {
				return;
			}
			break;      // only one mirror view at a time
		}
	}

	R_AddDrawSurfCmd( drawSurfs, numDrawSurfs );
}

void R_AddEntitySurfaces()
{
	if ( !r_drawentities->integer ) {
		return;
	}

	for ( tr.currentEntityNum = 0;
		  tr.currentEntityNum < tr.refdef.num_entities;
		  tr.currentEntityNum++ ) {
		trRefEntity_t* ent = tr.currentEntity = &tr.refdef.entities[tr.currentEntityNum];

		ent->needDlights = 0;

		// preshift the value we are going to OR into the drawsurf sort
		tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

		//
		// the weapon model must be handled special --
		// we don't want the hacked weapon position showing in
		// mirrors, because the true body position will already be drawn
		//
		if ( ( ent->e.renderfx & RF_FIRST_PERSON ) && tr.viewParms.isPortal ) {
			continue;
		}

		// simple generated models, like sprites and beams, are not culled
		switch ( ent->e.reType ) {
		case RT_PORTALSURFACE:
			break;      // don't draw anything
		case RT_SPRITE:
		case RT_SPLASH:
		case RT_BEAM:
		case RT_LIGHTNING:
		case RT_RAIL_CORE:
		case RT_RAIL_CORE_TAPER:
		case RT_RAIL_RINGS:
		{
			// self blood sprites, talk balloons, etc should not be drawn in the primary
			// view.  We can't just do this check for all entities, because md3
			// entities may still want to cast shadows from them
			if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
				continue;
			}
			shader_t* shader = R_GetShaderByHandle( ent->e.customShader );
			R_AddDrawSurf( &entitySurface, shader, R_SpriteFogNum( ent ), 0, ATI_TESS_NONE );
		}
			break;

		case RT_MODEL:
			// we must set up parts of tr.orientation for model culling
			R_RotateForEntity( ent, &tr.viewParms, &tr.orientation );

			tr.currentModel = R_GetModelByHandle( ent->e.hModel );
			if ( !tr.currentModel ) {
				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0, ATI_TESS_NONE );
			} else {
				switch ( tr.currentModel->type ) {
				case MOD_MESH:
					R_AddMD3Surfaces( ent );
					break;
					// Ridah
				case MOD_MDC:
					R_AddMDCSurfaces( ent );
					break;
					// done.
				case MOD_MDS:
					R_AddAnimSurfaces( ent );
					break;
				case MOD_BRUSH:
					R_AddBrushModelSurfaces( ent );
					break;
				case MOD_BAD:       // null model axis
					if ( ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal ) {
						break;
					}
					//shader = R_GetShaderByHandle( ent->e.customShader );
					R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0, ATI_TESS_NONE );
					break;
				default:
					Com_Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
					break;
				}
			}
			break;
		default:
			Com_Error( ERR_DROP, "R_AddEntitySurfaces: Bad reType" );
		}
	}

}


void R_GenerateDrawSurfs()
{
	R_AddWorldSurfaces();

	R_AddPolygonSurfaces();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	R_SetupProjection();

	R_AddEntitySurfaces();
}

void R_DebugPolygon( int color, int numPoints, float *points )
{
	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( color & 1, ( color >> 1 ) & 1, ( color >> 2 ) & 1 );
	qglBegin( GL_POLYGON );
	for (int i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for (int i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
}

/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView( viewParms_t *parms )
{
	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	// Ridah, purge media that were left over from the last level
	if ( r_cache->integer ) {
		extern void R_PurgeBackupImages( int purgeCount );
		static int lastTime;

		if ( ( lastTime > tr.refdef.time ) || ( lastTime < ( tr.refdef.time - 200 ) ) ) {
			R_FreeImageBuffer();    // clear all image buffers
			R_PurgeShaders( 10 );
			R_PurgeBackupImages( 1 );
			R_PurgeModels( 1 );
			lastTime = tr.refdef.time;
		}
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	int firstDrawSurf = tr.refdef.numDrawSurfs;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer();

	R_SetupFrustum();

	R_GenerateDrawSurfs();

	R_SortDrawSurfs( tr.refdef.drawSurfs + firstDrawSurf, tr.refdef.numDrawSurfs - firstDrawSurf );
}
