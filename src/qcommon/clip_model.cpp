#include "clip_model.h"

int ClipModel::loadMap(const char *name)
{
	dheader_t header;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "CM_LoadMap: nullptr name" );
        return -1; // keep the linter happy, ERR_DROP does not return
	}

//	cm_noAreas = Cvar_Get( "cm_noAreas", "0", CVAR_CHEAT );
//	cm_noCurves = Cvar_Get( "cm_noCurves", "0", CVAR_CHEAT );
//	cm_playerCurveClip = Cvar_Get( "cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT );

	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	// free old stuff
	Com_Memset( &cm, 0, sizeof( cm ) );
	CM_ClearLevelPatches();

	//
	// load the file
	//
    int *buf;
	size_t length = FS_ReadFile( name, (void **)&buf );

	if ( !buf ) {
		Com_Error( ERR_DROP, "Couldn't load %s", name );
        return -1; // keep the linter happy, ERR_DROP does not return
	}

	last_checksum = LittleLong( Com_BlockChecksum( buf, length ) );
	*checksum = last_checksum;

	header = *(dheader_t *)buf;
	for (int i = 0 ; i < sizeof( dheader_t ) / 4 ; i++ ) {
		( (int *)&header )[i] = LittleLong( ( (int *)&header )[i] );
	}

	if ( header.version != BSP_VERSION ) {
		Com_Error( ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)"
				   , name, header.version, BSP_VERSION );
        return; // keep the linter happy, ERR_DROP does not return
	}

	cmod_base = (uint8_t *)buf;

	// load into heap
	CMod_LoadShaders( &header.lumps[LUMP_SHADERS] );
	CMod_LoadLeafs( &header.lumps[LUMP_LEAFS] );
	CMod_LoadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES] );
	CMod_LoadLeafSurfaces( &header.lumps[LUMP_LEAFSURFACES] );
	CMod_LoadPlanes( &header.lumps[LUMP_PLANES] );
	CMod_LoadBrushSides( &header.lumps[LUMP_BRUSHSIDES] );
	CMod_LoadBrushes( &header.lumps[LUMP_BRUSHES] );
	CMod_LoadSubmodels( &header.lumps[LUMP_MODELS] );
	CMod_LoadNodes( &header.lumps[LUMP_NODES] );
	CMod_LoadEntityString( &header.lumps[LUMP_ENTITIES] );
	CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY] );
	CMod_LoadPatches( &header.lumps[LUMP_SURFACES], &header.lumps[LUMP_DRAWVERTS] );

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile( buf );

	CM_InitBoxHull();

	CM_FloodAreaConnections();

}

void ClipModel::clearMap() {
    
}
