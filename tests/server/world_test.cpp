#include "server/world.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE( "world tests", "world" ) {
    World world( 4, 64 );
    idVec3 min( -100.0f, -100.0f, -100.0f );
    idVec3 max(  100.0f,  100.0f,  100.0f );

    world.reset(min, max );

    REQUIRE( world.debug_getSectorCount() == 31 ); // Expected number of sectors for depth 4

}
