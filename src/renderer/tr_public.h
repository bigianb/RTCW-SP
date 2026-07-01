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

#pragma once

#include "../cgame/tr_types.h"

int R_MarkFragments( int orientation, const vec3_t *points, const vec3_t projection,
					 int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

void        RE_LoadWorldMap( const char *mapname );
qhandle_t   RE_RegisterModel( const char *name );

bool    RE_GetSkinModel( qhandle_t skinid, const char *type, char *name );
qhandle_t   RE_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );
qhandle_t   RE_RegisterSkin( const char *name );
qhandle_t   RE_RegisterShader( const char *name );
void RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );
void RE_ClearScene();
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void RE_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void RB_ZombieFXAddNewHit( int entityNum, const vec3_t hitPos, const vec3_t hitDir );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, unsigned int overdraw );
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags );

qhandle_t  RE_RegisterShaderNoMip( const char *name );

void R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density );
void RE_RenderScene( const refdef_t *fd );

void RE_StretchPic( float x, float y, float w, float h,
					float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_StretchPicGradient( float x, float y, float w, float h,
							float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );
void RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const uint8_t *data, int client, bool dirty );

int  R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );
void R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

void RE_SetColor( const float *rgba );
void RE_BeginRegistration( glconfig_t *glconfig );
void RE_EndRegistration();

void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );

void RE_UploadCinematic( int w, int h, int cols, int rows, const uint8_t *data, int client, bool dirty );

void RE_Shutdown( bool destroyWindow );
