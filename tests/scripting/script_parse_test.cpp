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

myScript2 // a comment here
{
    pain
    {
        playsound sound/painsound.wav
    }
    
    trigger cinematic1
	{
		wait 6750  // Another comment
		trigger player cine1_cam1 // comment
    }

    trigger cinematic2
	{
		wait 100
		trigger player cine2_cam1
    }

    spawn
    {
    
    }
}

)";

    ScriptParser parser;
    std::vector<ScriptParser::EntityScript> scripts = parser.parse(scriptText);
    REQUIRE( scripts.size() == 2 );
    REQUIRE( scripts[0].name == "myScript" );
    REQUIRE( scripts[0].events.size() == 2 );
    REQUIRE( scripts[0].events[0].name == "spawn" );
    
    REQUIRE( scripts[0].events[0].actions.size() == 3 );
    REQUIRE( scripts[0].events[0].actions[0].name == "playsound" );
    REQUIRE( scripts[0].events[0].actions[0].parameters.size() == 1 );
    REQUIRE( scripts[0].events[0].actions[0].parameters[0] == "sound/mysound.wav" );
    REQUIRE( scripts[0].events[0].actions[1].name == "wait" );
    REQUIRE( scripts[0].events[0].actions[1].parameters.size() == 1 );
    REQUIRE( scripts[0].events[0].actions[1].parameters[0] == "1000" );
    REQUIRE( scripts[0].events[0].actions[2].name == "trigger" );
    REQUIRE( scripts[0].events[0].actions[2].parameters.size() == 1 );
    REQUIRE( scripts[0].events[0].actions[2].parameters[0] == "target1" );

    REQUIRE( scripts[0].events[1].name == "death" );
    REQUIRE( scripts[0].events[1].actions.size() == 1 );
    REQUIRE( scripts[0].events[1].actions[0].name == "playsound" );
    REQUIRE( scripts[0].events[1].actions[0].parameters.size() == 1 );
    REQUIRE( scripts[0].events[1].actions[0].parameters[0] == "sound/deathsound.wav" );

    REQUIRE( scripts[1].name == "myScript2" );
    REQUIRE( scripts[1].events.size() == 4 );
    REQUIRE( scripts[1].events[1].name == "trigger" );
    REQUIRE( scripts[1].events[1].parameters.size() == 1 );
    REQUIRE( scripts[1].events[1].parameters[0] == "cinematic1" );
    REQUIRE( scripts[1].events[1].actions.size() == 2 );
    REQUIRE( scripts[1].events[3].actions.size() == 0 );
}
