#include "clip_model.h"

#include "./qcommon.h"

ClipModel TheClipModel::clipModel;

int ClipModel::leafArea(int leafnum)
{
	if ( leafnum < 0 || leafnum >= numLeaves ) {
		Com_Error( ERR_DROP, "CM_LeafArea: bad number" );
	}
	return leaves[leafnum].area;
}

int ClipModel::leafCluster(int leafnum)
{
	if ( leafnum < 0 || leafnum >= numLeaves ) {
		Com_Error( ERR_DROP, "CM_LeafCluster: bad number" );
	}
	return leaves[leafnum].cluster;
}

int ClipModel::inlineModel(int index)
{
	if ( index < 0 || index >= numSubModels ) {
		Com_Error( ERR_DROP, "CM_InlineModel: bad number" );
	}
	return index;
}

void ClipModel::modelBounds(int modelIndex, idVec3 &mins, idVec3 &maxs)
{
	cModel_t *cmod = clipHandleToModel(modelIndex);
	mins = cmod->mins;
	maxs = cmod->maxs;
}

cModel_t *ClipModel::clipHandleToModel(clipHandle_t handle)
{
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < numSubModels ) {
		return &cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE ) {
		return &box_model;
	}
	if ( handle < MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i",
				   numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );
	return nullptr;
}

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

	initBoxHull();

	floodAreaConnections();
}

ClipModel::ClipModel()
{
	checkcount = 0;
	floodvalid = 0;

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

	numSurfaces = 0;
	surfaces = nullptr;
}

ClipModel::~ClipModel()
{
	clearMap();
}

void ClipModel::clearMap()
{
	checkcount = 0;
	floodvalid = 0;

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

	for ( int i = 0; i <  numSurfaces; i++ ) {
		delete surfaces[i];
	}
	numSurfaces = 0;
	delete[] surfaces;
	surfaces = nullptr;
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
	areaPortals = new int[numAreas * numAreas];
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
	idVec3 points[MAX_PATCH_VERTS];

	dsurface_t* in = ( dsurface_t * )( offsetBase + surfaceLump->fileofs );
	if ( surfaceLump->filelen % sizeof( *in ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}
	numSurfaces = surfaceLump->filelen / sizeof( *in );
	surfaces = new cPatch_t*[numSurfaces];
	drawVert_t* dv = ( drawVert_t * )( offsetBase + drawVertLump->fileofs );
	if ( drawVertLump->filelen % sizeof( *dv ) ) {
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	// scan through all the surfaces, but only load patches, not planar faces
	for (int i = 0 ; i < numSurfaces ; i++, in++ ) {
		if ( in->surfaceType != MST_PATCH ) {
			surfaces[ i ] = nullptr;
			continue;       // ignore other surfaces
		}
		// FIXME: check for non-colliding patches

		cPatch_t* patch = new cPatch_t;
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

void ClipModel::initBoxHull()
{
	box_model.leaf.numLeafBrushes = 1;
	box_model.leaf.firstLeafBrush = numLeafBrushes;
	leafBrushes[numLeafBrushes] = numBrushes;
	numLeafBrushes += BOX_LEAF_BRUSHES;

	// create the planes for the axial box
	box_planes = &planes[numPlanes];
	for ( int i = 0 ; i < 6 ; i++ ) {
		int side = i & 1;

		cplane_t* p = &box_planes[i * 2];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[i >> 1] = 1;

		p = &box_planes[i * 2 + 1];
		p->type = 3 + ( i >> 1 );
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[i >> 1] = -1;

		SetPlaneSignbits( p );
	}

	numPlanes += BOX_PLANES;

	// create the single brush that is the box
	box_brush = &brushes[numBrushes];
	box_brush->sides = &brushsides[numBrushSides];
	box_brush->numsides = BOX_SIDES;
	box_brush->shaderNum = 0; // default shader
	box_brush->contents = CONTENTS_BODY;

	for ( int i = 0 ; i < BOX_SIDES ; i++ ) {
		brushsides[numBrushSides + i].plane = &box_planes[i];
		brushsides[numBrushSides + i].shaderNum = 0; // default shader
		brushsides[numBrushSides + i].surfaceFlags = 0;
	}

	numBrushSides += BOX_SIDES;
	numBrushes += BOX_BRUSHES;
}

clipHandle_t ClipModel::tempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule )
{
	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	VectorCopy( mins, box_brush->bounds[0] );
	VectorCopy( maxs, box_brush->bounds[1] );

	if ( capsule ) {
		return CAPSULE_MODEL_HANDLE;
	}

	return BOX_MODEL_HANDLE;
}
void ClipModel::floodArea_r( int areaNum, int floodnum )
{
	cArea_t * area = &areas[ areaNum ];

	if ( area->floodvalid == floodvalid ) {
		if ( area->floodnum == floodnum ) {
			return;
		}
		Com_Error( ERR_DROP, "FloodArea_r: reflooded" );
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	int *con = areaPortals + areaNum * numAreas;
	for (int i = 0; i < numAreas; i++ ) {
		if ( con[i] > 0 ) {
			floodArea_r( i, floodnum );
		}
	}
}

void ClipModel::floodAreaConnections()
{
	// all current floods are now invalid
	floodvalid++;
	int floodnum = 0;

	cArea_t *area = areas;    // Ridah, optimization
	for (int i = 0 ; i < numAreas ; i++, area++ ) {
		if ( area->floodvalid == floodvalid ) {
			continue;       // already flooded into
		}
		floodnum++;
		floodArea_r( i, floodnum );
	}

}
