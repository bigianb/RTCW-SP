#include "scripting/ScriptParser.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE( "script tests", "parse" ) {

const char* scriptText = R"(

myScript
{
    spawn spawnparam1
    {
        playsound sound/mysound.wav
        wait 1000
        trigger target1
    }
    death
    {
        playsound sound/deathsound.wav
    }
}

myScript2
{
    pain
    {
        playsound sound/painsound.wav
    }
}

)";

    ScriptParser parser;
    std::vector<ScriptParser::EntityScript> scripts = parser.parse(scriptText);
    REQUIRE( scripts.size() == 2 );
    REQUIRE( scripts[0].name == "myScript" );
    REQUIRE( scripts[0].events.size() == 2 );
    REQUIRE( scripts[0].events["spawn"].size() == 3 );
    REQUIRE( scripts[0].events["spawn"][0].name == "playsound" );
    REQUIRE( scripts[0].events["spawn"][0].parameters.size() == 1 );
    REQUIRE( scripts[0].events["spawn"][0].parameters[0] == "sound/mysound.wav" );
    REQUIRE( scripts[0].events["spawn"][1].name == "wait" );
    REQUIRE( scripts[0].events["spawn"][1].parameters.size() == 1 );
    REQUIRE( scripts[0].events["spawn"][1].parameters[0] == "1000" );
    REQUIRE( scripts[0].events["spawn"][2].name == "trigger" );
    REQUIRE( scripts[0].events["spawn"][2].parameters.size() == 1 );
    REQUIRE( scripts[0].events["spawn"][2].parameters[0] == "target1" );
    REQUIRE( scripts[0].events["death"].size() == 1 );
    REQUIRE( scripts[0].events["death"][0].name == "playsound" );
    REQUIRE( scripts[0].events["death"][0].parameters.size() == 1 );
    REQUIRE( scripts[0].events["death"][0].parameters[0] == "sound/deathsound.wav" );

}
