#pragma once

#include <list>

class ServerEntity;

class World
{
public:
    World(int areaDepth, int areaNodes){

    }

    void reset(){
        sectors.clear();
    }

    void createSector(int depth, float min[3], float max[3]);

private:
    class Sector
    {
    public:
        int axis;           // -1 = leaf node
        float dist;
        Sector   *children[2];
        ServerEntity  *entities;
    };

    std::list<Sector> sectors;

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