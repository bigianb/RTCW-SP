#include "scripting/ScriptParser.h"

#include <catch2/catch_test_macros.hpp>


TEST_CASE( "script tests", "parse" ) {

const char* scriptText = R"(
{
    myScript
    {
        spawn
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
}
)";

    ScriptParser parser;
    parser.parse(scriptText);

}
