#include "clip_model.h"

#include "./qcommon.h"

void ClipModel::loadMap(const char *name)
{
	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: nullptr name" );
	}

	Com_DPrintf( "ClipModel::loadMap( %s)\n", name );

	clearMap();

	//
	// load the file
	//
    const uint8_t *buf;
	size_t length = FS_ReadFile( name, (void **)&buf );

	if ( !buf ) {
		Com_Error( ERR_DROP, "Couldn't load %s", name );
	}

	const dheader_t& header = *(dheader_t *)buf;

	if ( header.version != BSP_VERSION ) {
		Com_Error( ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)", name, header.version, BSP_VERSION );
	}

	loadShaders( &header.lumps[LUMP_SHADERS], buf);
	loadLeaves( &header.lumps[LUMP_LEAFS], buf );
	loadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES], buf );
	loadLeafSurfaces( &header.lumps[LUMP_LEAFSURFACES], buf );
	loadPlanes( &header.lumps[LUMP_PLANES], buf );
	loadBrushSides( &header.lumps[LUMP_BRUSHSIDES], buf );
	loadBrushes( &header.lumps[LUMP_BRUSHES], buf );
	loadSubmodels( &header.lumps[LUMP_MODELS], buf );
	loadNodes( &header.lumps[LUMP_NODES], buf );
	loadEntityString( &header.lumps[LUMP_ENTITIES], buf );
	loadVisibility( &header.lumps[LUMP_VISIBILITY], buf );
	loadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS], buf );

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile( buf );

	//CM_InitBoxHull();

	//CM_FloodAreaConnections();

}

ClipModel::ClipModel()
{
	shaders = nullptr;
	numShaders = 0;

	leaves = nullptr;
	numLeaves = 0;
	numClusters = 0;

	areas = nullptr;
	numAreas = 0;
	areaPortals = nullptr;

	leafBrushes = nullptr;
	numLeafBrushes = 0;

	leafsurfaces = nullptr;
	numLeafSurfaces = 0;

	planes = nullptr;
	numPlanes = 0;

	brushsides = nullptr;
	numBrushSides = 0;

	brushes = nullptr;
	numBrushes = 0;

	cmodels = nullptr;
	numSubModels = 0;

	nodes = nullptr;
	numNodes = 0;

	entityString = nullptr;
	numEntityChars = 0;

	clusterBytes = 0;
	visibility = nullptr;
	vised = false;
}

ClipModel::~ClipModel()
{
	clearMap();
}

void ClipModel::clearMap() {
    delete[] shaders;
	shaders = nullptr;
	numShaders = 0;

	delete[] leaves;
	leaves = nullptr;
	numLeaves = 0;
	numClusters = 0;

	delete[] areas;
	areas = nullptr;
	delete[] areaPortals;
	areaPortals = nullptr;
	numAreas = 0;

	delete[] leafBrushes;
	leafBrushes = nullptr;
	numLeafBrushes = 0;

	delete[] leafsurfaces;
	leafsurfaces = nullptr;
	numLeafSurfaces = 0;

	delete[] planes;
	planes = nullptr;
	numPlanes = 0;

	delete[] brushsides;
	brushsides = nullptr;
	numBrushSides = 0;

	delete[] brushes;
	brushes = nullptr;
	numBrushes = 0;

	delete [] cmodels;
	cmodels = nullptr;
	numSubModels = 0;

	delete[] nodes;
	nodes = nullptr;
	numNodes = 0;

	delete[] entityString;
	entityString = nullptr;
	numEntityChars = 0;

	delete[] visibility;
	clusterBytes = 0;
	visibility = nullptr;
	vised = false;
}

void ClipModel::loadShaders(const lump_t* l, const uint8_t* offsetBase)
{
	const dshader_t* in = ( const dshader_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "CMod_LoadShaders: funny lump size" );
	}
	int count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no shaders" );
	}
	shaders = new dshader_t[count];
	numShaders = count;
	memcpy( shaders, in, count * sizeof( *shaders ) );
}

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define BOX_BRUSHES     1
#define BOX_LEAF_BRUSHES 1
#define BOX_SIDES       6
#define BOX_LEAFS       2
#define BOX_PLANES      12

void ClipModel::loadLeaves(const lump_t* l, const uint8_t* offsetBase)
{
	const dleaf_t* in = ( const dleaf_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	int count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no leaves" );
	}

	leaves = new cLeaf_t[BOX_LEAFS + count];
	numLeaves = count;

	for (int i = 0 ; i < count ; i++, in++) {
		cLeaf_t* out = &leaves[i];
		out->fromSubmodel = 0;
		out->cluster = in->cluster;
		out->area = in->area;
		out->firstLeafBrush = in->firstLeafBrush;
		out->numLeafBrushes = in->numLeafBrushes;
		out->firstLeafSurface = in->firstLeafSurface;
		out->numLeafSurfaces = in->numLeafSurfaces;

		if ( out->cluster >= numClusters ) {
			numClusters = out->cluster + 1;
		}
		if ( out->area >= numAreas ) {
			numAreas = out->area + 1;
		}
	}

	areas = new cArea_t[numAreas];
	memset(areas, 0, numAreas * sizeof(areas[0]));
	areaPortals = new int*[numAreas * numAreas];
	memset(areaPortals, 0, numAreas * numAreas * sizeof(areaPortals[0]));
}

void ClipModel::loadLeafBrushes(const lump_t* l, const uint8_t* offsetBase)
{
	int *in = ( int * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( int ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	int count = l->filelen / sizeof( int );

	leafBrushes = new int[BOX_LEAF_BRUSHES + count];
	numLeafBrushes = count;

	memcpy( leafBrushes, in, count * sizeof( int ) );
}

void ClipModel::loadLeafSurfaces(const lump_t* l, const uint8_t* offsetBase)
{
	int* in = ( int * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( int ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	int count = l->filelen / sizeof( int );

	leafsurfaces = new int[count];
	numLeafSurfaces = count;

	memcpy( leafsurfaces, in, count * sizeof( int ) );
}

void ClipModel::loadPlanes(const lump_t* l, const uint8_t* offsetBase)
{
	dplane_t* in = ( dplane_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( dplane_t ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	int count = l->filelen / sizeof( dplane_t );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no planes" );
	}

	planes = new cplane_t[BOX_PLANES + count];
	numPlanes = count;

	cplane_t* out = planes;

	for (int i = 0 ; i < count ; i++, in++, out++ ){
		int bits = 0;
		for (int j = 0 ; j < 3 ; j++ ){
			out->normal[j] = in->normal[j];
			if ( out->normal[j] < 0 ) {
				bits |= 1 << j;
			}
		}

		out->dist = in->dist;
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

void ClipModel::loadBrushSides(const lump_t* l, const uint8_t* offsetBase)
{
	dbrushside_t* in = ( dbrushside_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	int count = l->filelen / sizeof( *in );

	brushsides = new cBrushSide_t[BOX_SIDES + count];
	numBrushSides = count;

	cBrushSide_t* out = brushsides;

	for (int i = 0 ; i < count ; i++, in++, out++ ) {
		int num = in->planeNum;
		out->plane = &planes[num];
		out->shaderNum = in->shaderNum;
		if ( out->shaderNum < 0 || out->shaderNum >= numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum );
		}
		out->surfaceFlags = shaders[out->shaderNum].surfaceFlags;
	}
}

void ClipModel::loadBrushes(const lump_t* l, const uint8_t* offsetBase)
{
	dbrush_t* in = ( dbrush_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	int count = l->filelen / sizeof( *in );

	brushes = new cBrush_t[BOX_BRUSHES + count];
	numBrushes = count;

	cBrush_t* out = brushes;

	for (int i = 0 ; i < count ; i++, out++, in++ ) {
		out->sides = brushsides + in->firstSide;
		out->numsides = in->numSides;

		out->shaderNum = in->shaderNum;
		if ( out->shaderNum < 0 || out->shaderNum >= numShaders ) {
			Com_Error( ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum );
		}
		out->contents = shaders[out->shaderNum].contentFlags;

		boundBrush( out );
	}
}

void ClipModel::boundBrush( cBrush_t *b )
{
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}

void ClipModel::loadSubmodels(const lump_t* l, const uint8_t* offsetBase)
{
	dmodel_t    *in = ( dmodel_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "CMod_LoadSubmodels: funny lump size" );
	}
	int count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map with no models" );
	}
	cmodels = new cModel_t[count];
	numSubModels = count;

	for (int i = 0 ; i < count ; i++, in++ ) {
		cModel_t *out = &cmodels[i];

		for (int j = 0 ; j < 3 ; j++ )
		{   // spread the mins / maxs by a pixel
			out->mins[j] = in->mins[j] - 1;
			out->maxs[j] = in->maxs[j] + 1;
		}

		out->leaf.fromSubmodel = 1;
		
		out->leaf.numLeafBrushes = in->numBrushes;
		out->leaf.firstLeafBrush = in->firstBrush;

		out->leaf.numLeafSurfaces = in->numSurfaces;
		out->leaf.firstLeafSurface = in->firstSurface;
	}
}

void ClipModel::loadNodes(const lump_t* l, const uint8_t* offsetBase)
{
	dnode_t *in = ( dnode_t * )( offsetBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	int count = l->filelen / sizeof( *in );

	if ( count < 1 ) {
		Com_Error( ERR_DROP, "Map has no nodes" );
	}
	nodes = new cNode_t[count];
	numNodes = count;

	cNode_t *out = nodes;

	for (int i = 0 ; i < count ; i++, out++, in++ ) {
		out->plane = planes + in->planeNum;
		for (int j = 0 ; j < 2 ; j++ ) {
			out->children[j] = in->children[j];
		}
	}
}

void ClipModel::loadEntityString(const lump_t* l, const uint8_t* offsetBase)
{
	entityString = new char[l->filelen];
	numEntityChars = l->filelen;
	memcpy( entityString, offsetBase + l->fileofs, l->filelen );
}

#define VIS_HEADER  8
void ClipModel::loadVisibility(const lump_t* l, const uint8_t* offsetBase)
{
	int len = l->filelen;
	if ( !len ) {
		clusterBytes = ( numClusters + 31 ) & ~31;
		visibility = new uint8_t[clusterBytes];
		memset( visibility, 255, clusterBytes );
		return;
	}
	const uint8_t* buf = offsetBase + l->fileofs;

	vised = true;
	visibility = new uint8_t[len];
	numClusters =  ( (const int *)buf )[0];
	clusterBytes = ( (const int *)buf )[1];
	memcpy( visibility, buf + VIS_HEADER, len - VIS_HEADER );
}

#define MAX_PATCH_VERTS     1024
void ClipModel::loadPatches(const lump_t* surfaceLump, const lump_t* drawVertLump, const uint8_t* offsetBase)
{
	cPatch_t    *patch;
	vec3_t points[MAX_PATCH_VERTS];

	dsurface_t* in = ( dsurface_t * )( offsetBase + surfaceLump->fileofs );
	if ( surfaceLump->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	numSurfaces = surfaceLump->filelen / sizeof( *in );
	surfaces = (cPatch_t **)Hunk_Alloc( numSurfaces * sizeof( surfaces[0] ), h_high );

	drawVert_t* dv = ( drawVert_t * )( offsetBase + drawVertLump->fileofs );
	if ( drawVertLump->filelen % sizeof( *dv ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	// scan through all the surfaces, but only load patches,
	// not planar faces
	for (int i = 0 ; i < numSurfaces ; i++, in++ ) {
		if ( in->surfaceType != MST_PATCH ) {
			continue;       // ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cPatch_t* patch = (cPatch_t *)Hunk_Alloc( sizeof( *patch ), h_high );
		surfaces[ i ] = patch;

		// load the full drawverts onto the stack
		int width = in->patchWidth;
		int height = in->patchHeight;
		int c = width * height;
		if ( c > MAX_PATCH_VERTS ) {
			Com_Error( ERR_DROP, "ParseMesh: MAX_PATCH_VERTS" );
		}

		drawVert_t* dv_p = dv + in->firstVert;
		for (int j = 0 ; j < c ; j++, dv_p++ ) {
			points[j][0] = dv_p->xyz[0];
			points[j][1] = dv_p->xyz[1];
			points[j][2] = dv_p->xyz[2];
		}

		int shaderNum = in->shaderNum;
		patch->contents = shaders[shaderNum].contentFlags;
		patch->surfaceFlags = shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide( width, height, points );
	}

}
