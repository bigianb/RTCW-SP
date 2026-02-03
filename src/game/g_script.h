#pragma once

class GameEntity;
#include <vector>
#include <string>

//====================================================================
//
// Scripting, these structure are not saved into savegames (parsed each start)
struct g_script_stack_action_t
{
	const char    *actionString;
	bool ( *actionFunc )( GameEntity *ent, const std::vector<std::string>& params );
};

struct g_script_stack_item_t
{
	//
	// set during script parsing
	g_script_stack_action_t     *action;            // points to an action to perform
	std::vector<std::string> params;
};

#define G_MAX_SCRIPT_STACK_ITEMS    64

struct g_script_stack_t
{
	g_script_stack_t() : numItems( 0 )
	{}

	g_script_stack_item_t items[G_MAX_SCRIPT_STACK_ITEMS];
	int numItems;
};
struct g_script_event_t
{
	g_script_event_t()
		: eventNum( 0 )
		, stack()
	{}

	int eventNum;                           // index in scriptEvents[]
	std::vector<std::string> params;        // trigger targetname, etc
	g_script_stack_t stack;
};

struct g_script_event_define_t
{
	const char        *eventStr;
	bool ( *eventMatch )( g_script_event_t *event, const char *eventParm, const char* eventParm2 );
};

//
// Script Flags
#define SCFL_GOING_TO_MARKER    0x1
#define SCFL_ANIMATING          0x2
#define SCFL_WAITING_RESTORE    0x4
//
// Scripting Status (NOTE: this MUST NOT contain any pointer vars)
struct g_script_status_t
{
	int scriptStackHead, scriptStackChangeTime;
	int scriptEventIndex;       // current event containing stack of actions to perform
	// scripting system variables
	int scriptId;                   // incremented each time the script changes
	int scriptFlags;
	std::vector<std::string> animatingParams;
};


void G_Script_ScriptEvent( GameEntity *ent, const char *eventStr, const char *params, const char *params2 );
