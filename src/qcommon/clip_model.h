#pragma once

#include <cstdint>
#include "../idlib/math/Vector.h"
#include "cm_patch.h"

struct lump_t;
struct dshader_t;

typedef int clipHandle_t;

#define MAX_SUBMODELS           512
#define BOX_MODEL_HANDLE        511
#define CAPSULE_MODEL_HANDLE    510

// Note this is the file format, so don't change it.
struct cLeaf_t
{
	int cluster;
	int area;

	int firstLeafBrush;
	int numLeafBrushes;

	int firstLeafSurface;
	int numLeafSurfaces;
	
	// If this leaf is part of a sub-model then don't index via leaf brushed
	int fromSubmodel;
};

struct cModel_t
{
	idVec3 mins, maxs;
	cLeaf_t leaf;               // submodels don't reference the main tree
};

struct cArea_t
{
	int floodnum;
	int floodvalid;
};

struct cplane_t;

struct cBrushSide_t
{
	cplane_t *plane;
	int surfaceFlags;
	int shaderNum;
};

struct cBrush_t
{
	int shaderNum;    // the shader that determined the contents
	int contents;
	idVec3 bounds[2];
	int numsides;
	cBrushSide_t    *sides;
	int checkcount;  // to avoid repeated testings
};

struct cNode_t
{
	cplane_t *plane;
	int children[2];                // negative numbers are leafs
};

struct cPatch_t
{
    cPatch_t() {
        checkcount = 0;
        surfaceFlags = 0;
        contents = 0;
        pc = nullptr;
    }

	int checkcount;                     // to avoid repeated testings
	int surfaceFlags;
	int contents;
	patchCollide_t   *pc;
};

class ClipModel
{
public:
    ClipModel();
    ~ClipModel();
    void loadMap(const char *name);
    void clearMap();

    int leafArea(int leafnum);
    int leafCluster(int leafnum);
    int inlineModel(int index);     // index 0 = world, 1 + are bmodels

    void modelBounds(int modelIndex, idVec3 &mins, idVec3 &maxs);

    cModel_t* clipHandleToModel(clipHandle_t handle);

    int checkcount;

    /*
        To keep everything totally uniform, bounding boxes are turned into small
        BSP trees instead of being compared directly.
        Capsules are handled differently though.
    */
    clipHandle_t tempBoxModel( const float mins[3], const float maxs[3], int capsule );

    // used by the temp box model
private:
    cModel_t    box_model;
    cplane_t    *box_planes;
    cBrush_t    *box_brush;

private:
    void loadShaders(const lump_t* l, const uint8_t* offsetBase);
    void loadLeaves(const lump_t* l, const uint8_t* offsetBase);
    void loadLeafBrushes(const lump_t* l, const uint8_t* offsetBase);
    void loadLeafSurfaces(const lump_t* l, const uint8_t* offsetBase);
    void loadPlanes(const lump_t* l, const uint8_t* offsetBase);
    void loadBrushSides(const lump_t* l, const uint8_t* offsetBase);
    void loadBrushes(const lump_t* l, const uint8_t* offsetBase);
    void loadSubmodels(const lump_t* l, const uint8_t* offsetBase);
    void loadNodes(const lump_t* l, const uint8_t* offsetBase);
    void loadEntityString(const lump_t* l, const uint8_t* offsetBase);
    void loadVisibility(const lump_t* l, const uint8_t* offsetBase);
    void loadPatches(const lump_t* surfaceLump, const lump_t* drawVertLump, const uint8_t* offsetBase);

    void boundBrush( cBrush_t *b );
    void initBoxHull();
    void floodAreaConnections();
    void floodArea_r( int areaNum, int floodnum );

// TODO: fix up this visibility later.
public:
    int floodvalid;

    dshader_t* shaders;
    int numShaders;

    cLeaf_t* leaves;
    int numLeaves;

    int numClusters;

    cArea_t* areas;
    int numAreas;

    int* areaPortals;

    int* leafBrushes;
    int numLeafBrushes;

    int* leafsurfaces;
    int numLeafSurfaces;

    cplane_t* planes;
    int numPlanes;

    cBrushSide_t* brushsides;
    int numBrushSides;

    cBrush_t* brushes;
    int numBrushes;

    cModel_t* cmodels;
    int numSubModels;

    cNode_t* nodes;
    int numNodes;

    char* entityString;
    int numEntityChars;

    int clusterBytes;
    uint8_t* visibility;
    bool vised;

    int numSurfaces;
    cPatch_t** surfaces;
};

class TheClipModel
{
public:
    static ClipModel& get() { return clipModel; }
private:
    static ClipModel clipModel;
};
