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
#include <stddef.h> 
#include "bsp_reader.h"

int c_subdivisions;
int c_gridVerts;

//===============================================================================

static void HSVtoRGB( float h, float s, float v, float rgb[3] )
{
	h *= 5.0f;

	int i = floor( h );
	float f = h - i;

	float p = v * ( 1 - s );
	float q = v * ( 1 - s * f );
	float t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
	case 0:
		rgb[0] = v;
		rgb[1] = t;
		rgb[2] = p;
		break;
	case 1:
		rgb[0] = q;
		rgb[1] = v;
		rgb[2] = p;
		break;
	case 2:
		rgb[0] = p;
		rgb[1] = v;
		rgb[2] = t;
		break;
	case 3:
		rgb[0] = p;
		rgb[1] = q;
		rgb[2] = v;
		break;
	case 4:
		rgb[0] = t;
		rgb[1] = p;
		rgb[2] = v;
		break;
	case 5:
		rgb[0] = v;
		rgb[1] = p;
		rgb[2] = q;
		break;
	}
}

static void R_ColorShiftLightingBytes( uint8_t in[4], uint8_t out[4] )
{
	// shift the color data based on overbright range
	int shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	int r = in[0] << shift;
	int g = in[1] << shift;
	int b = in[2] << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

// TODO: address tr side effects
#define LIGHTMAP_SIZE   128
void BSPReader::loadLightmaps(world_t* world)
{
    uint8_t image[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
	float maxIntensity = 0.0f;
	double sumIntensity = 0.0;

	lump_t *l = &header->lumps[LUMP_LIGHTMAPS];

	int len = l->filelen;
	if ( !len ) {
		return;
	}
	uint8_t* buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_SyncRenderThread();

	// create all the lightmaps
	tr.numLightmaps = len / ( LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3 );
	if ( tr.numLightmaps == 1 ) {
		//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
		//this avoids this, but isn't the correct solution.
		tr.numLightmaps++;
	}

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		return;
	}

	for (int i = 0 ; i < tr.numLightmaps ; i++ ) {
		// expand the 24 bit on-disk to 32 bit
		uint8_t* buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

		if ( r_lightmap->integer == 2 ) { // color code by intensity as development tool	(FIXME: check range)
			for ( int j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
			{
				float r = buf_p[j * 3 + 0];
				float g = buf_p[j * 3 + 1];
				float b = buf_p[j * 3 + 2];
				float intensity;
				float out[3];

				intensity = 0.33f * r + 0.685f * g + 0.063f * b;

				if ( intensity > 255 ) {
					intensity = 1.0f;
				} else {
					intensity /= 255.0f;
				}

				if ( intensity > maxIntensity ) {
					maxIntensity = intensity;
				}

				HSVtoRGB( intensity, 1.00, 0.50, out );

				image[j * 4 + 0] = out[0] * 255;
				image[j * 4 + 1] = out[1] * 255;
				image[j * 4 + 2] = out[2] * 255;
				image[j * 4 + 3] = 255;

				sumIntensity += intensity;
			}
		} else {
			for ( int j = 0 ; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ ) {
				R_ColorShiftLightingBytes( &buf_p[j * 3], &image[j * 4] );
				image[j * 4 + 3] = 255;
			}
		}
		tr.lightmaps[i] = R_CreateImage( va( "*lightmap%d",i ), image,
										 LIGHTMAP_SIZE, LIGHTMAP_SIZE, false, false, GL_CLAMP );
	}

	if ( r_lightmap->integer == 2 ) {
		Com_Printf( "Brightest lightmap value: %d\n", ( int ) ( maxIntensity * 255 ) );
	}
}

void BSPReader::loadVisibility(world_t* world)
{
	lump_t *l = &header->lumps[LUMP_VISIBILITY];
	int len = ( world->numClusters + 63 ) & ~63;
	world->novis = (uint8_t*)Hunk_Alloc( len, h_low );
	memset( world->novis, 0xff, len );

	len = l->filelen;
	if ( !len ) {
		return;
	}
	uint8_t* buf = fileBase + l->fileofs;

	world->numClusters = LittleLong( ( (int *)buf )[0] );
	world->clusterBytes = LittleLong( ( (int *)buf )[1] );

	uint8_t* dest = (uint8_t*)Hunk_Alloc( len - 8, h_low );
	memcpy( dest, buf + 8, len - 8 );
	world->vis = dest;
}

static shader_t *ShaderForShaderNum(dshader_t *dsh, int lightmapNum )
{
	/*
	shaderNum = LittleLong( shaderNum );
	if ( shaderNum < 0 || shaderNum >= s_worldData.numShaders ) {
		Com_Error( ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum );
        return nullptr; // keep the linter happy, ERR_DROP does not return
	}
	dsh = &s_worldData.shaders[ shaderNum ];
	*/

	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		lightmapNum = LIGHTMAP_BY_VERTEX;
	}

	if ( r_fullbright->integer ) {
		lightmapNum = LIGHTMAP_WHITEIMAGE;
	}

	shader_t* shader = R_FindShader( dsh->shader, lightmapNum, true );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

// Ridah, optimizations here
// memory block for use by surfaces
static uint8_t *surfHunkPtr;
static int surfHunkSize;
#define SURF_HUNK_MAXSIZE 0x40000
#define LL( x ) LittleLong( x )

/*
==============
R_InitSurfMemory
==============
*/
void R_InitSurfMemory( void ) {
	// allocate a new chunk
	surfHunkPtr = (uint8_t*)Hunk_Alloc( SURF_HUNK_MAXSIZE, h_low );
	surfHunkSize = 0;
}

/*
==============
R_GetSurfMemory
==============
*/
void *R_GetSurfMemory( int size ) {
	uint8_t *retval;

	// round to cacheline
	size = ( size + 31 ) & ~31;

	surfHunkSize += size;
	if ( surfHunkSize >= SURF_HUNK_MAXSIZE ) {
		// allocate a new chunk
		R_InitSurfMemory();
		surfHunkSize += size;   // since it just got reset
	}
	retval = surfHunkPtr;
	surfHunkPtr += size;

	return (void *)retval;
}

static void ParseFace( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes  ) {
	int i, j;
	srfSurfaceFace_t    *cv;
	int numPoints, numIndexes;
	int lightmapNum;
	int sfaceSize, ofsIndexes;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	dshader_t *dsh = world->shaders + LittleLong( ds->shaderNum );
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numPoints = LittleLong( ds->numVerts );
	if ( numPoints > MAX_FACE_POINTS ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints );
		numPoints = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	numIndexes = LittleLong( ds->numIndexes );

	// create the srfSurfaceFace_t
	sfaceSize = offsetof( srfSurfaceFace_t, points ) + sizeof( *cv->points ) * numPoints;
	ofsIndexes = sfaceSize;
	sfaceSize += sizeof( int ) * numIndexes;

	//cv = Hunk_Alloc( sfaceSize );
	cv = (srfSurfaceFace_t *)R_GetSurfMemory( sfaceSize );

	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;

	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			cv->points[i][j] = LittleFloat( verts[i].xyz[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			cv->points[i][3 + j] = LittleFloat( verts[i].st[j] );
			cv->points[i][5 + j] = LittleFloat( verts[i].lightmap[j] );
		}
		R_ColorShiftLightingBytes( verts[i].color, (uint8_t *)&cv->points[i][7] );
	}

	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		( ( int * )( (uint8_t *)cv + cv->ofsIndices ) )[i] = LittleLong( indexes[ i ] );
	}

	// take the plane information from the lightmap vector
	for ( i = 0 ; i < 3 ; i++ ) {
		cv->plane.normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
	cv->plane.dist = DotProduct( cv->points[0], cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = (surfaceType_t *)cv;
}

static void ParseMesh( dsurface_t *ds, drawVert_t *verts, msurface_t *surf ) {
	srfGridMesh_t   *grid;
	int i, j;
	int width, height, numPoints;
    drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
	int lightmapNum;
	vec3_t bounds[2];
	vec3_t tmpVec;
	static surfaceType_t skipData = SF_SKIP;

	lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW ) {
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;
	for ( i = 0 ; i < numPoints ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			points[i].xyz[j] = LittleFloat( verts[i].xyz[j] );
			points[i].normal[j] = LittleFloat( verts[i].normal[j] );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			points[i].st[j] = LittleFloat( verts[i].st[j] );
			points[i].lightmap[j] = LittleFloat( verts[i].lightmap[j] );
		}
		R_ColorShiftLightingBytes( verts[i].color, points[i].color );
	}

	// pre-tesseleate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = (surfaceType_t *)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = LittleFloat( ds->lightmapVecs[0][i] );
		bounds[1][i] = LittleFloat( ds->lightmapVecs[1][i] );
	}
	VectorAdd( bounds[0], bounds[1], bounds[1] );
	VectorScale( bounds[1], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[0], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );
}

static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfTriangles_t  *tri;
	int i, j;
	int numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );
	numIndexes = LittleLong( ds->numIndexes );

	//tri = Hunk_Alloc( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] )
	//	+ numIndexes * sizeof( tri->indexes[0] ) );
	tri = (srfTriangles_t *)R_GetSurfMemory( sizeof( *tri ) + numVerts * sizeof( tri->verts[0] )
						   + numIndexes * sizeof( tri->indexes[0] ) );

	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = ( drawVert_t * )( tri + 1 );
	tri->indexes = ( int * )( tri->verts + tri->numVerts );

	surf->data = (surfaceType_t *)tri;

	// copy vertexes
	ClearBounds( tri->bounds[0], tri->bounds[1] );
	verts += LittleLong( ds->firstVert );
	for ( i = 0 ; i < numVerts ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			tri->verts[i].xyz[j] = LittleFloat( verts[i].xyz[j] );
			tri->verts[i].normal[j] = LittleFloat( verts[i].normal[j] );
		}
		AddPointToBounds( tri->verts[i].xyz, tri->bounds[0], tri->bounds[1] );
		for ( j = 0 ; j < 2 ; j++ ) {
			tri->verts[i].st[j] = LittleFloat( verts[i].st[j] );
			tri->verts[i].lightmap[j] = LittleFloat( verts[i].lightmap[j] );
		}

		R_ColorShiftLightingBytes( verts[i].color, tri->verts[i].color );
	}

	// copy indexes
	indexes += LittleLong( ds->firstIndex );
	for ( i = 0 ; i < numIndexes ; i++ ) {
		tri->indexes[i] = LittleLong( indexes[i] );
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= numVerts ) {
			Com_Error( ERR_DROP, "Bad index in triangle surface" );
            return; // keep the linter happy, ERR_DROP does not return
		}
	}
}

static void ParseFlare( dsurface_t *ds, drawVert_t *verts, msurface_t *surf, int *indexes ) {
	srfFlare_t      *flare;
	int i;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	flare = (srfFlare_t *)Hunk_Alloc( sizeof( *flare ), h_low );
	flare->surfaceType = SF_FLARE;

	surf->data = (surfaceType_t *)flare;

	for ( i = 0 ; i < 3 ; i++ ) {
		flare->origin[i] = LittleFloat( ds->lightmapOrigin[i] );
		flare->color[i] = LittleFloat( ds->lightmapVecs[0][i] );
		flare->normal[i] = LittleFloat( ds->lightmapVecs[2][i] );
	}
}


/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints( srfGridMesh_t *grid, int offset ) {
	int i, j;

	for ( i = 1; i < grid->width - 1; i++ ) {
		for ( j = i + 1; j < grid->width - 1; j++ ) {
			if ( Q_fabs( grid->verts[i + offset].xyz[0] - grid->verts[j + offset].xyz[0] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[i + offset].xyz[1] - grid->verts[j + offset].xyz[1] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[i + offset].xyz[2] - grid->verts[j + offset].xyz[2] ) > .1 ) {
				continue;
			}
			return true;
		}
	}
	return false;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints( srfGridMesh_t *grid, int offset ) {
	int i, j;

	for ( i = 1; i < grid->height - 1; i++ ) {
		for ( j = i + 1; j < grid->height - 1; j++ ) {
			if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[0] - grid->verts[grid->width * j + offset].xyz[0] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[1] - grid->verts[grid->width * j + offset].xyz[1] ) > .1 ) {
				continue;
			}
			if ( Q_fabs( grid->verts[grid->width * i + offset].xyz[2] - grid->verts[grid->width * j + offset].xyz[2] ) > .1 ) {
				continue;
			}
			return true;
		}
	}
	return false;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 ) {
	int j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) {
			continue;
		}
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) {
			continue;
		}
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) {
			continue;
		}
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) {
			continue;
		}
		//
		touch = false;
		for ( n = 0; n < 2; n++ ) {
			//
			if ( n ) {
				offset1 = ( grid1->height - 1 ) * grid1->width;
			} else { offset1 = 0;}
			if ( R_MergedWidthPoints( grid1, offset1 ) ) {
				continue;
			}
			for ( k = 1; k < grid1->width - 1; k++ ) {
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = ( grid2->height - 1 ) * grid2->width;
					} else { offset2 = 0;}
					if ( R_MergedWidthPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->width - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->widthLodError[k];
						touch = true;
					}
				}
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = grid2->width - 1;
					} else { offset2 = 0;}
					if ( R_MergedHeightPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->height - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->widthLodError[k];
						touch = true;
					}
				}
			}
		}
		for ( n = 0; n < 2; n++ ) {
			//
			if ( n ) {
				offset1 = grid1->width - 1;
			} else { offset1 = 0;}
			if ( R_MergedHeightPoints( grid1, offset1 ) ) {
				continue;
			}
			for ( k = 1; k < grid1->height - 1; k++ ) {
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = ( grid2->height - 1 ) * grid2->width;
					} else { offset2 = 0;}
					if ( R_MergedWidthPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->width - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[l] = grid1->heightLodError[k];
						touch = true;
					}
				}
				for ( m = 0; m < 2; m++ ) {

					if ( m ) {
						offset2 = grid2->width - 1;
					} else { offset2 = 0;}
					if ( R_MergedHeightPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->height - 1; l++ ) {
						//
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[0] - grid2->verts[grid2->width * l + offset2].xyz[0] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[1] - grid2->verts[grid2->width * l + offset2].xyz[1] ) > .1 ) {
							continue;
						}
						if ( Q_fabs( grid1->verts[grid1->width * k + offset1].xyz[2] - grid2->verts[grid2->width * l + offset2].xyz[2] ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[l] = grid1->heightLodError[k];
						touch = true;
					}
				}
			}
		}
		if ( touch ) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void ) {
	int i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID ) {
			continue;
		}
		//
		if ( grid1->lodFixed ) {
			continue;
		}
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1 );
	}
}

int R_StitchPatches( int grid1num, int grid2num ) {
	int k, l, m, n, offset1, offset2, row, column;
	srfGridMesh_t *grid1, *grid2;
	float *v1, *v2;

	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	grid2 = (srfGridMesh_t *) s_worldData.surfaces[grid2num].data;
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = ( grid1->height - 1 ) * grid1->width;
		} else { offset1 = 0;}
		if ( R_MergedWidthPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = 0; k < grid1->width - 2; k += 2 ) {

			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k + 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[k + 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
		}
	}
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = grid1->width - 1;
		} else { offset1 = 0;}
		if ( R_MergedHeightPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = 0; k < grid1->height - 2; k += 2 ) {
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k + 2 ) + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[grid1->width * ( k + 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
		}
	}
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = ( grid1->height - 1 ) * grid1->width;
		} else { offset1 = 0;}
		if ( R_MergedWidthPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = grid1->width - 1; k > 1; k -= 2 ) {

			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[k - 2 + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[k - 1 + offset1].xyz, grid1->widthLodError[k + 1] );
					if ( !grid2 ) {
						break;
					}
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
		}
	}
	for ( n = 0; n < 2; n++ ) {
		//
		if ( n ) {
			offset1 = grid1->width - 1;
		} else { offset1 = 0;}
		if ( R_MergedHeightPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( k = grid1->height - 1; k > 1; k -= 2 ) {
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else { offset2 = 0;}
				//if (R_MergedWidthPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->width - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
					v2 = grid2->verts[l + 1 + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[l + offset2].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m ) {
						row = grid2->height - 1;
					} else { row = 0;}
					grid2 = R_GridInsertColumn( grid2, l + 1, row,
												grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
			for ( m = 0; m < 2; m++ ) {

				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				if ( m ) {
					offset2 = grid2->width - 1;
				} else { offset2 = 0;}
				//if (R_MergedHeightPoints(grid2, offset2))
				//	continue;
				for ( l = 0; l < grid2->height - 1; l++ ) {
					//
					v1 = grid1->verts[grid1->width * k + offset1].xyz;
					v2 = grid2->verts[grid2->width * l + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}

					v1 = grid1->verts[grid1->width * ( k - 2 ) + offset1].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[1] - v2[1] ) > .1 ) {
						continue;
					}
					if ( Q_fabs( v1[2] - v2[2] ) > .1 ) {
						continue;
					}
					//
					v1 = grid2->verts[grid2->width * l + offset2].xyz;
					v2 = grid2->verts[grid2->width * ( l + 1 ) + offset2].xyz;
					if ( Q_fabs( v1[0] - v2[0] ) < .01 &&
						 Q_fabs( v1[1] - v2[1] ) < .01 &&
						 Q_fabs( v1[2] - v2[2] ) < .01 ) {
						continue;
					}
					//
					//Com_Printf("found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m ) {
						column = grid2->width - 1;
					} else { column = 0;}
					grid2 = R_GridInsertRow( grid2, l + 1, column,
											 grid1->verts[grid1->width * ( k - 1 ) + offset1].xyz, grid1->heightLodError[k + 1] );
					grid2->lodStitched = false;
					s_worldData.surfaces[grid2num].data = (surfaceType_t *) grid2;
					return true;
				}
			}
		}
	}
	return false;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num ) {
	int j, numstitches;
	srfGridMesh_t *grid1, *grid2;

	numstitches = 0;
	grid1 = (srfGridMesh_t *) s_worldData.surfaces[grid1num].data;
	for ( j = 0; j < s_worldData.numsurfaces; j++ ) {
		//
		grid2 = (srfGridMesh_t *) s_worldData.surfaces[j].data;
		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[0] != grid2->lodOrigin[0] ) {
			continue;
		}
		if ( grid1->lodOrigin[1] != grid2->lodOrigin[1] ) {
			continue;
		}
		if ( grid1->lodOrigin[2] != grid2->lodOrigin[2] ) {
			continue;
		}
		//
		while ( R_StitchPatches( grid1num, j ) )
		{
			numstitches++;
		}
	}
	return numstitches;
}


void R_StitchAllPatches( void ) {
	int i, stitched, numstitches;
	srfGridMesh_t *grid1;

	numstitches = 0;
	do
	{
		stitched = false;
		for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
			//
			grid1 = (srfGridMesh_t *) s_worldData.surfaces[i].data;
			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID ) {
				continue;
			}
			//
			if ( grid1->lodStitched ) {
				continue;
			}
			//
			grid1->lodStitched = true;
			stitched = true;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while ( stitched );
	Com_Printf("stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk( void ) {
	int i, size;
	srfGridMesh_t *grid, *hunkgrid;

	for ( i = 0; i < s_worldData.numsurfaces; i++ ) {
		//
		grid = (srfGridMesh_t *) s_worldData.surfaces[i].data;
		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID ) {
			continue;
		}
		//
		size = ( grid->width * grid->height - 1 ) * sizeof( drawVert_t ) + sizeof( *grid );
		hunkgrid = (srfGridMesh_t *)Hunk_Alloc( size, h_low );
		Com_Memcpy( hunkgrid, grid, size );

		hunkgrid->widthLodError = (float *)Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = (float *)Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( grid->heightLodError, grid->heightLodError, grid->height * 4 );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[i].data = (surfaceType_t *) hunkgrid;
	}
}

void BSPReader::loadSurfaces( world_t* world )
{
	lump_t *surfs = &header->lumps[LUMP_SURFACES];
	lump_t *verts = &header->lumps[LUMP_DRAWVERTS];
	lump_t *indexLump = &header->lumps[LUMP_DRAWINDEXES];

	int numFaces = 0;
	int numMeshes = 0;
	int numTriSurfs = 0;
	int numFlares = 0;

	dsurface_t* in = ( dsurface_t * )( fileBase + surfs->fileofs );
	if ( surfs->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	int count = surfs->filelen / sizeof( *in );

	drawVert_t* dv = ( drawVert_t * )( fileBase + verts->fileofs );
	if ( verts->filelen % sizeof( *dv ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}

	int* indexes = ( int * )( fileBase + indexLump->fileofs );
	if ( indexLump->filelen % sizeof( *indexes ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}

	msurface_t* out = (msurface_t *)Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	// Ridah, init the surface memory. This is optimization, so we don't have to
	// look for memory for each surface, we allocate a big block and just chew it up
	// as we go
	R_InitSurfMemory();

	for (int i = 0 ; i < count ; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case MST_PATCH:
			ParseMesh( in, dv, out );
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
			ParseTriSurf( in, dv, out, indexes );
			numTriSurfs++;
			break;
		case MST_PLANAR:
			ParseFace( in, dv, out, indexes );
			numFaces++;
			break;
		case MST_FLARE:
			ParseFlare( in, dv, out, indexes );
			numFlares++;
			break;
		default:
			Com_Error( ERR_DROP, "Bad surfaceType" );
		}
	}

	R_StitchAllPatches();
	R_FixSharedVertexLodError();
	R_MovePatchSurfacesToHunk();

	Com_Printf( "...loaded %d faces, %i meshes, %i trisurfs, %i flares\n",
			   numFaces, numMeshes, numTriSurfs, numFlares );
}

void BSPReader::loadSubmodels(world_t* world)
{
	lump_t *l = &header->lumps[LUMP_MODELS];

	dmodel_t* in = ( dmodel_t * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", world->name );
	}
	int count = l->filelen / sizeof( *in );

	world->bmodels = (bmodel_t *)Hunk_Alloc( count * sizeof( bmodel_t ), h_low );

	bmodel_t* out = world->bmodels;
	for ( int i = 0 ; i < count ; i++, in++, out++ ) {
		model_t *model = R_AllocModel();

		assert( model != nullptr );            // this should never happen

		model->type = MOD_BRUSH;
		model->bmodel = out;
		snprintf( model->name, sizeof( model->name ), "*%d", i );

		for ( int j = 0 ; j < 3 ; j++ ) {
			out->bounds[0][j] = LittleFloat( in->mins[j] );
			out->bounds[1][j] = LittleFloat( in->maxs[j] );
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}

static void R_SetParent( mnode_t *node, mnode_t *parent ) {
	node->parent = parent;
	if ( node->contents != -1 ) {
		return;
	}
	R_SetParent( node->children[0], node );
	R_SetParent( node->children[1], node );
}

void BSPReader::loadNodesAndLeafs(world_t* world)
{
	lump_t *nodeLump = &header->lumps[LUMP_NODES];
	lump_t *leafLump = &header->lumps[LUMP_LEAFS];

	dnode_t* in = ( dnode_t * )( fileBase + nodeLump->fileofs );
	if ( nodeLump->filelen % sizeof( dnode_t ) ||
		 leafLump->filelen % sizeof( dleaf_t ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", world->name );
	}
	int numNodes = nodeLump->filelen / sizeof( dnode_t );
	int numLeafs = leafLump->filelen / sizeof( dleaf_t );

	mnode_t* out = (mnode_t *)Hunk_Alloc( ( numNodes + numLeafs ) * sizeof( *out ), h_low );

	world->nodes = out;
	world->numnodes = numNodes + numLeafs;
	world->numDecisionNodes = numNodes;

	// load nodes
	for (int i = 0 ; i < numNodes; i++, in++, out++ )
	{
		for (int j = 0 ; j < 3 ; j++ )
		{
			out->mins[j] = LittleLong( in->mins[j] );
			out->maxs[j] = LittleLong( in->maxs[j] );
		}

		int p = LittleLong( in->planeNum );
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE;  // differentiate from leafs

		for ( int j = 0 ; j < 2 ; j++ )
		{
			int p = LittleLong( in->children[j] );
			if ( p >= 0 ) {
				out->children[j] = s_worldData.nodes + p;
			} else {
				out->children[j] = s_worldData.nodes + numNodes + ( -1 - p );
			}
		}
	}

	// load leafs
	dleaf_t* inLeaf = ( dleaf_t * )( fileBase + leafLump->fileofs );
	for ( int i = 0 ; i < numLeafs ; i++, inLeaf++, out++ )
	{
		for ( int j = 0 ; j < 3 ; j++ )
		{
			out->mins[j] = LittleLong( inLeaf->mins[j] );
			out->maxs[j] = LittleLong( inLeaf->maxs[j] );
		}

		out->cluster = LittleLong( inLeaf->cluster );
		out->area = LittleLong( inLeaf->area );

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = s_worldData.marksurfaces +
								LittleLong( inLeaf->firstLeafSurface );
		out->nummarksurfaces = LittleLong( inLeaf->numLeafSurfaces );
	}

	// chain decendants
	R_SetParent( s_worldData.nodes, nullptr );
}

//=============================================================================


void BSPReader::loadShaders(world_t* world)
{
	lump_t *l = &header->lumps[LUMP_SHADERS];

	dshader_t* in = ( dshader_t * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	int count = l->filelen / sizeof( *in );
	dshader_t* out = (dshader_t *)Hunk_Alloc( count * sizeof( *out ), h_low );

	world->shaders = out;
	world->numShaders = count;

	memcpy( out, in, count * sizeof( *out ) );

	for (int i = 0 ; i < count ; i++ ) {
		out[i].surfaceFlags = LittleLong( out[i].surfaceFlags );
		out[i].contentFlags = LittleLong( out[i].contentFlags );
	}
}

void BSPReader::loadMarkSurfaces( world_t* world )
{
	lump_t *l = &header->lumps[LUMP_LEAFSURFACES];

	int *in = ( int * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
        return; // keep the linter happy, ERR_DROP does not return
	}
	int count = l->filelen / sizeof( *in );
	msurface_t ** out = (msurface_t **)Hunk_Alloc( count * sizeof( *out ), h_low );

	world->marksurfaces = out;
	world->nummarksurfaces = count;

	for ( int i = 0 ; i < count ; i++ )
	{
		int j = LittleLong( in[i] );
		out[i] = s_worldData.surfaces + j;
	}
}

void BSPReader::loadPlanes(world_t* world)
{
	lump_t* l = &header->lumps[LUMP_PLANES];

	dplane_t* in = ( dplane_t * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s", world->name );
	}
	int count = l->filelen / sizeof( *in );
	cplane_t* out = (cplane_t *)Hunk_Alloc( count * 2 * sizeof( *out ), h_low );

	world->planes = out;
	world->numplanes = count;

	for (int i = 0 ; i < count ; i++, in++, out++ ) {
		int bits = 0;
		for ( int j = 0 ; j < 3 ; j++ ) {
			out->normal[j] = LittleFloat( in->normal[j] );
			if ( out->normal[j] < 0 ) {
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

void BSPReader::loadFogs(world_t* world)
{
	lump_t *l = &header->lumps[LUMP_FOGS];
	lump_t *brushesLump = &header->lumps[LUMP_BRUSHES];
	lump_t *sidesLump = &header->lumps[LUMP_BRUSHSIDES];

	dfog_t* fogs = ( dfog_t * )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *fogs ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	int count = l->filelen / sizeof( *fogs );

	// create fog strucutres for them
	world->numfogs = count + 1;
	world->fogs = (fog_t *)Hunk_Alloc( world->numfogs * sizeof( fog_t ), h_low );
	fog_t* out = world->fogs + 1;

	if ( !count ) {
		return;
	}

	dbrush_t* brushes = ( dbrush_t * )( fileBase + brushesLump->fileofs );
	if ( brushesLump->filelen % sizeof( *brushes ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
        return; // keep the linter happy, ERR_DROP does not return
	}
	int brushesCount = brushesLump->filelen / sizeof( *brushes );

	dbrushside_t* sides = ( dbrushside_t * )( fileBase + sidesLump->fileofs );
	if ( sidesLump->filelen % sizeof( *sides ) ) {
		Com_Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
        return; // keep the linter happy, ERR_DROP does not return
	}
	int sidesCount = sidesLump->filelen / sizeof( *sides );

	for ( int i = 0 ; i < count ; i++, fogs++ ) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		if ( (unsigned)out->originalBrushNumber >= brushesCount ) {
			Com_Error( ERR_DROP, "fog brushNumber out of range" );
		}
		dbrush_t *brush = brushes + out->originalBrushNumber;

		int firstSide = LittleLong( brush->firstSide );

		if ( (unsigned)firstSide > sidesCount - 6 ) {
			Com_Error( ERR_DROP, "fog brush sideNumber out of range" );
		}

		// brushes are always sorted with the axial sides first
		int sideNum = firstSide + 0;
		int planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][0] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 1;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][0] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 2;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][1] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 3;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][1] = s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 4;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[0][2] = -s_worldData.planes[ planeNum ].dist;

		sideNum = firstSide + 5;
		planeNum = LittleLong( sides[ sideNum ].planeNum );
		out->bounds[1][2] = s_worldData.planes[ planeNum ].dist;

		// get information from the shader for fog parameters
		shader_t *shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, true );

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4( shader->fogParms.color[0] * tr.identityLight,
									 shader->fogParms.color[1] * tr.identityLight,
									 shader->fogParms.color[2] * tr.identityLight, 1.0 );

		int d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum == -1 ) {
			out->hasSurface = false;
		} else {
			out->hasSurface = true;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[3] = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

}

void R_FindLightGridBounds( vec3_t mins, vec3_t maxs ) {
	world_t *w;
	msurface_t  *surf;
	srfSurfaceFace_t *surfFace;
	struct shader_s     *shd;

	bool foundGridBrushes = false;
	int i,j;

	w = &s_worldData;

//----(SA)	temp - disable this whole thing for now
	VectorCopy( w->bmodels[0].bounds[0], mins );
	VectorCopy( w->bmodels[0].bounds[1], maxs );
	return;
}

void BSPReader::loadLightGrid( world_t* world ) 
{
	lump_t *l = &header->lumps[LUMP_LIGHTGRID];

	world->lightGridInverseSize[0] = 1.0 / world->lightGridSize[0];
	world->lightGridInverseSize[1] = 1.0 / world->lightGridSize[1];
	world->lightGridInverseSize[2] = 1.0 / world->lightGridSize[2];

	// TODO: this makes little sense.
	vec3_t wMins;
	vec3_t wMaxs;
//----(SA)	modified
	R_FindLightGridBounds( wMins, wMaxs );
//	wMins = w->bmodels[0].bounds[0];
//	wMaxs = w->bmodels[0].bounds[1];
//----(SA)	end

	vec3_t maxs;
	for (int i = 0 ; i < 3 ; i++ ) {
		world->lightGridOrigin[i] = world->lightGridSize[i] * ceil( wMins[i] / world->lightGridSize[i] );
		maxs[i] = world->lightGridSize[i] * floor( wMaxs[i] / world->lightGridSize[i] );
		world->lightGridBounds[i] = ( maxs[i] - world->lightGridOrigin[i] ) / world->lightGridSize[i] + 1;
	}

	int numGridPoints = world->lightGridBounds[0] * world->lightGridBounds[1] * world->lightGridBounds[2];

	if ( l->filelen != numGridPoints * 8 ) {
		Com_Printf(S_COLOR_YELLOW "WARNING: light grid mismatch\n" );
		world->lightGridData = nullptr;
		return;
	}

	world->lightGridData = (uint8_t *)Hunk_Alloc( l->filelen, h_low );
	memcpy( world->lightGridData, ( void * )( fileBase + l->fileofs ), l->filelen );

	// deal with overbright bits
	for (int i = 0 ; i < numGridPoints ; i++ ) {
		R_ColorShiftLightingBytes( &world->lightGridData[i * 8], &world->lightGridData[i * 8] );
		R_ColorShiftLightingBytes( &world->lightGridData[i * 8 + 3], &world->lightGridData[i * 8 + 3] );
	}
}

void BSPReader::loadEntities( world_t* world )
{
	char keyname[MAX_TOKEN_CHARS];
	char value[MAX_TOKEN_CHARS];

	lump_t *l = &header->lumps[LUMP_ENTITIES];

	world->lightGridSize[0] = 64;
	world->lightGridSize[1] = 64;
	world->lightGridSize[2] = 128;

	const char* p = ( const char * )( fileBase + l->fileofs );

	// store for reference by the cgame
	world->entityString = (char *)Hunk_Alloc( l->filelen + 1, h_low );
	strcpy( world->entityString, p );
	world->entityParsePoint = world->entityString;

	char *token = COM_ParseExt( &p, true );
	if ( !*token || *token != '{' ) {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {
		// parse key
		token = COM_ParseExt( &p, true );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt( &p, true );

		if ( !*token || *token == '}' ) {
			break;
		}
		Q_strncpyz( value, token, sizeof( value ) );

		// check for remapping of shaders for vertex lighting
		const char* s = "vertexremapshader";
		if ( !Q_strncmp( keyname, s, strlen( s ) ) ) {
			char* sv = strchr( value, ';' );
			if ( !sv ) {
				Com_Printf(S_COLOR_YELLOW  "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*sv++ = 0;
			if ( r_vertexLight->integer ) {
				R_RemapShader( value, sv, "0" );
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if ( !Q_strncmp( keyname, s, strlen( s ) ) ) {
			char* sv = strchr( value, ';' );
			if ( !sv ) {
				Com_Printf(S_COLOR_YELLOW  "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*sv++ = 0;
			R_RemapShader( value, sv, "0" );
			continue;
		}
		// check for a different grid size
		if ( !Q_stricmp( keyname, "gridsize" ) ) {
			sscanf( value, "%f %f %f", &world->lightGridSize[0], &world->lightGridSize[1], &world->lightGridSize[2] );
			continue;
		}
	}
}


bool R_GetEntityToken( char *buffer, int size ) {
	const char  *s;

	s = COM_Parse( (const char**)&s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return false;
	} else {
		return true;
	}
}

/*
Called directly from cgame
*/
void RE_LoadWorldMap( const char *name ) {
	skyboxportal = 0;

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45;
	tr.sunDirection[1] = 0.3;
	tr.sunDirection[2] = 0.9;

	tr.sunShader = 0;   // clear sunshader so it's not there if the level doesn't specify it

	// invalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//			now that I can see how it's been used.  (functionality can narrow since
	//			it's not used as much as it's designed for.)
	R_SetFog( FOG_SKY,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_PORTALVIEW,0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_HUD,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_MAP,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_CURRENT,   0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_TARGET,    0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_WATER,     0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_SERVER,    0, 0, 0, 0, 0, 0 );

	VectorNormalize( tr.sunDirection );

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	delete(tr.world);
	tr.world = nullptr;

	BSPReader bspReader;
	tr.world = bspReader.load( name );

	if ( tr.sunShaderName ) {
		tr.sunShader = R_FindShader( tr.sunShaderName, LIGHTMAP_NONE, true );
	}
}

void updateScreen()
{
	updateScreen();
}

world_t* BSPReader::load(const char* name)
{
	FS_ReadFile( name, (void **)&fileBase );
	if ( !fileBase ) {
		Com_Error( ERR_DROP, "RE_LoadWorldMap: %s not found", name );
	}

	world_t* world = new world_t();

	Q_strncpyz( world->name, name, sizeof( world->name ) );
	Q_strncpyz( world->baseName, COM_SkipPath( world->name ), sizeof( world->name ) );
	COM_StripExtension( world->baseName, world->baseName );

	c_gridVerts = 0;

	header = (dheader_t *)fileBase;

	int version = LittleLong( header->version );
	if ( version != BSP_VERSION ) {
		Com_Error( ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)", name, version, BSP_VERSION );
	}

	// swap all the lumps
	for (int i = 0 ; i < sizeof( dheader_t ) / 4 ; i++ ) {
		( (int *)header )[i] = LittleLong( ( (int *)header )[i] );
	}

	// load into heap
	updateScreen();
	loadShaders(world);
	updateScreen();

	loadLightmaps(world);
	updateScreen();

	loadPlanes(world);
	updateScreen();

	loadFogs(world);
	updateScreen();

	loadSurfaces(world);
	updateScreen();

	loadMarkSurfaces(world);
	updateScreen();

	loadNodesAndLeafs(world);
	updateScreen();

	loadSubmodels(world);
	updateScreen();

	loadVisibility(world);
	updateScreen();

	loadEntities(world);
	updateScreen();

	loadLightGrid(world);
	updateScreen();

	
	fileBase = nullptr;
	header = nullptr;
	FS_FreeFile( fileBase );

	return world;
}

