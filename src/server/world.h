#pragma once

#include <list>
#include "../idlib/math/Vector.h"

class ServerEntity;

class World
{
public:
    World(int areaDepth, int areaNodes){
        this->areaDepth = areaDepth;
        this->areaNodes = areaNodes;
    }

    void reset(){
        for (Sector* sector : sectors) {
            delete sector;
        }
        sectors.clear();
    }

    

private:
    int areaDepth;
    int areaNodes;

    class Sector
    {
    public:
        int axis;           // -1 = leaf node
        float dist;
        Sector   *children[2];
        ServerEntity  *entities;
    };

    Sector* createSector(int depth, const idVec3& min, const idVec3& max);

    std::list<Sector*> sectors;

};

/**
 * @brief Singleton class representing the game world.
 * This class provides a global point of access to world-related data and functionality.
 * It ensures that only one instance of the world exists throughout the application.
 * 
 */
class TheWorld
{
public:
    static World& getInstance() {
        static World instance(4, 64);
        return instance;
    }

    static void clear() {
        getInstance().reset();
    }
};