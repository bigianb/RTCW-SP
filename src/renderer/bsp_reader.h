#pragma once

#include "tr_local.h"

class BSPReader
{
	public:
		world_t* load(const char* name);

    protected:
        void loadShaders(world_t* world);
        void loadLightmaps(world_t* world);
        void loadPlanes(world_t* world);
        void loadFogs(world_t* world);
        void loadSurfaces(world_t* world);
        void loadMarkSurfaces(world_t* world);
        void loadNodesAndLeafs(world_t* world);
        void loadSubmodels(world_t* world);
        void loadVisibility(world_t* world);
        void loadEntities(world_t* world);
        void loadLightGrid(world_t* world);

    private:
        uint8_t *fileBase;
        dheader_t* header;
};
