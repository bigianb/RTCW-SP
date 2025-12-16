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
	//CMod_LoadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES] );
	//CMod_LoadLeafSurfaces( &header.lumps[LUMP_LEAFSURFACES] );
	//CMod_LoadPlanes( &header.lumps[LUMP_PLANES] );
	//CMod_LoadBrushSides( &header.lumps[LUMP_BRUSHSIDES] );
	//CMod_LoadBrushes( &header.lumps[LUMP_BRUSHES] );
	//CMod_LoadSubmodels( &header.lumps[LUMP_MODELS] );
	//CMod_LoadNodes( &header.lumps[LUMP_NODES] );
	//CMod_LoadEntityString( &header.lumps[LUMP_ENTITIES] );
	//CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY] );
	//CMod_LoadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS] );

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

	Com_Memcpy( shaders, in, count * sizeof( *shaders ) );
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
		out->cluster = LittleLong( in->cluster );
		out->area = LittleLong( in->area );
		out->firstLeafBrush = LittleLong( in->firstLeafBrush );
		out->numLeafBrushes = LittleLong( in->numLeafBrushes );
		out->firstLeafSurface = LittleLong( in->firstLeafSurface );
		out->numLeafSurfaces = LittleLong( in->numLeafSurfaces );

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
