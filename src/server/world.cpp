#include "world.h"

World::Sector* World::createSector(int depth, const idVec3& min, const idVec3& max) {
    // Implementation for creating a sector in the world
    Sector* newSector = new Sector();
    sectors.push_back(newSector);

    if (depth == areaDepth){
        newSector->axis = -1;   // leaf node
        newSector->children[0] = nullptr;
        newSector->children[1] = nullptr;
        return newSector;
    }

    idVec3 size = max - min;
    if (size.x > size.y) {
        newSector->axis = 0;
    } else {
        newSector->axis = 1;
    }

    newSector->dist = 0.5f * (max[newSector->axis] + min[newSector->axis]);
    idVec3 min1 = min;
    idVec3 max1 = max;
    idVec3 min2 = min;
    idVec3 max2 = max;
    max1[newSector->axis] = newSector->dist;
    min2[newSector->axis] = newSector->dist;

    newSector->children[0] = createSector(depth + 1, min2, max2);
    newSector->children[1] = createSector(depth + 1, min1, max1);

    return newSector;
}
