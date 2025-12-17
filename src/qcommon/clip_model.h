#pragma once

#include <cstdint>
#include "../idlib/math/Vector.h"

struct lump_t;
struct dshader_t;

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

class ClipModel
{
public:
    ClipModel();
    ~ClipModel();
    void loadMap(const char *name);
    void clearMap();

private:
    void loadShaders(const lump_t* l, const uint8_t* offsetBase);
    void loadLeaves(const lump_t* l, const uint8_t* offsetBase);
    void loadLeafBrushes(const lump_t* l, const uint8_t* offsetBase);
    void loadLeafSurfaces(const lump_t* l, const uint8_t* offsetBase);
    void loadPlanes(const lump_t* l, const uint8_t* offsetBase);
    void loadBrushSides(const lump_t* l, const uint8_t* offsetBase);
    void loadBrushes(const lump_t* l, const uint8_t* offsetBase);

    void boundBrush( cBrush_t *b );

    dshader_t* shaders;
    int numShaders;

    cLeaf_t* leaves;
    int numLeaves;

    int numClusters;

    cArea_t* areas;
    int numAreas;

    int** areaPortals;

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
};

class TheClipModel
{
public:
    ClipModel& get() { return clipModel; }
private:
    ClipModel clipModel;
};
