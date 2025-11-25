#pragma once

#include "ai_main.h"

// only C++ functions not used outside of the library.

//return true if the bot is dead
bool BotIsDead( bot_state_t *bs );

//returns true if the bot is in the intermission
bool BotIntermission( bot_state_t *bs );
