#pragma once

#include <cstdint>

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

    dshader_t* shaders;
    int numShaders;

    cLeaf_t* leaves;
    int numLeaves;

    int numClusters;

    cArea_t* areas;
    int numAreas;

    int** areaPortals;
};

class TheClipModel
{
public:
    ClipModel& get() { return clipModel; }
private:
    ClipModel clipModel;
};
