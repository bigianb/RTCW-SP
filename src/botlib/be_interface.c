/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_interface.c
 *
 * desc:		bot library interface
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas_entity.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "be_interface.h"

#include "../game/be_ea.h"
#include "be_ai_weight.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_gen.h"

//library globals in a structure
botlib_globals_t botlibglobals;

botlib_export_t be_botlib_export;
botlib_import_t botimport;
//
int bot_developer;
//qtrue if the library is setup
int botlibsetup = qfalse;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
// Ridah, faster Win32 code
#ifdef _WIN32
#undef MAX_PATH     // this is an ugly hack, to temporarily ignore the current definition, since it's also defined in windows.h
#include <windows.h>
#undef MAX_PATH
#define MAX_PATH    MAX_QPATH
#endif

int Sys_MilliSeconds( void ) {
// Ridah, faster Win32 code
#ifdef _WIN32
	int sys_curtime;
	static qboolean initialized = qfalse;
	static int sys_timeBase;

	if ( !initialized ) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
#else
	return clock() * 1000 / CLOCKS_PER_SEC;
#endif
} //end of the function Sys_MilliSeconds
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidClientNumber( int num, char *str ) {
	if ( num < 0 || num > botlibglobals.maxclients ) {
		//weird: the disabled stuff results in a crash
		BotImport_Print( PRT_ERROR, "%s: invalid client number %d, [0, %d]\n",
						 str, num, botlibglobals.maxclients );
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidEntityNumber( int num, char *str ) {
	if ( num < 0 || num > botlibglobals.maxentities ) {
		BotImport_Print( PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n",
						 str, num, botlibglobals.maxentities );
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BotLibSetup( char *str ) {
//	return qtrue;

	if ( !botlibglobals.botlibsetup ) {
		BotImport_Print( PRT_ERROR, "%s: bot library used before being setup\n", str );
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotLibSetup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibSetup( void ) {
	int errnum;

	bot_developer = LibVarGetValue( "bot_developer" );

	Log_Open( "botlib.log" );
	//
	BotImport_Print( PRT_MESSAGE, "------- BotLib Initialization -------\n" );
	//
	botlibglobals.maxclients = (int) LibVarValue( "maxclients", "128" );
	botlibglobals.maxentities = (int) LibVarValue( "maxentities", "2048" );

	errnum = AAS_Setup();           //be_aas_main.c
	if ( errnum != BLERR_NOERROR ) {
		return errnum;
	}
	errnum = EA_Setup();            //be_ea.c
	if ( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	errnum = BotSetupMoveAI();      //be_ai_move.c
	if ( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	botlibsetup = qtrue;
	botlibglobals.botlibsetup = qtrue;

	return BLERR_NOERROR;
} //end of the function Export_BotLibSetup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibShutdown( void ) {
	static int recursive = 0;

	if ( !BotLibSetup( "BotLibShutdown" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}
	//
	if ( recursive ) {
		return BLERR_NOERROR;
	}
	recursive = 1;
	// shutdown all AI subsystems
	BotShutdownChatAI();        //be_ai_chat.c
	BotShutdownMoveAI();        //be_ai_move.c
	BotShutdownGoalAI();        //be_ai_goal.c
	BotShutdownWeaponAI();      //be_ai_weap.c
	BotShutdownWeights();       //be_ai_weight.c
	BotShutdownCharacters();    //be_ai_char.c
	// shutdown AAS
	AAS_Shutdown();
	// shutdown bot elemantary actions
	EA_Shutdown();
	// free all libvars
	LibVarDeAllocAll();
	// remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();
	// shut down library log file
	Log_Shutdown();
	//
	botlibsetup = qfalse;
	botlibglobals.botlibsetup = qfalse;
	recursive = 0;
	// print any files still open
	PC_CheckOpenSourceHandles();
	//
#ifdef _DEBUG
	Log_AlwaysOpen( "memory.log" );
	PrintMemoryLabels();
	Log_Shutdown();
#endif
	return BLERR_NOERROR;
} //end of the function Export_BotLibShutdown
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarSet( char *var_name, char *value ) {
	LibVarSet( var_name, value );
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarSet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarGet( char *var_name, char *value, int size ) {
	char *varvalue;

	varvalue = LibVarGetString( var_name );
	strncpy( value, varvalue, size - 1 );
	value[size - 1] = '\0';
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarGet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibStartFrame( float time ) {
	if ( !BotLibSetup( "BotStartFrame" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}
	return AAS_StartFrame( time );
} //end of the function Export_BotLibStartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibLoadMap( const char *mapname ) {
#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif
	int errnum;

	if ( !BotLibSetup( "BotLoadMap" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}
	//
	BotImport_Print( PRT_MESSAGE, "------------ Map Loading ------------\n" );
	//startup AAS for the current map, model and sound index
	errnum = AAS_LoadMap( mapname );
	if ( errnum != BLERR_NOERROR ) {
		return errnum;
	}
	//initialize the items in the level
	BotInitLevelItems();        //be_ai_goal.h
	BotSetBrushModelTypes();    //be_ai_move.h
	//
	BotImport_Print( PRT_MESSAGE, "-------------------------------------\n" );
#ifdef DEBUG
	BotImport_Print( PRT_MESSAGE, "map loaded in %d msec\n", Sys_MilliSeconds() - starttime );
#endif
	//
	return BLERR_NOERROR;
} //end of the function Export_BotLibLoadMap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibUpdateEntity( int ent, bot_entitystate_t *state ) {
	if ( !BotLibSetup( "BotUpdateEntity" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}
	if ( !ValidEntityNumber( ent, "BotUpdateEntity" ) ) {
		return BLERR_INVALIDENTITYNUMBER;
	}

	return AAS_UpdateEntity( ent, state );
} //end of the function Export_BotLibUpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_TestMovementPrediction( int entnum, vec3_t origin, vec3_t dir );
void ElevatorBottomCenter( aas_reachability_t *reach, vec3_t bottomcenter );
int BotGetReachabilityToGoal( vec3_t origin, int areanum, int entnum,
							  int lastgoalareanum, int lastareanum,
							  int *avoidreach, float *avoidreachtimes, int *avoidreachtries,
							  bot_goal_t *goal, int travelflags, int movetravelflags );

int AAS_PointLight( vec3_t origin, int *red, int *green, int *blue );

int AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas );

int AAS_Reachability_WeaponJump( int area1num, int area2num );

int BotFuzzyPointReachabilityArea( vec3_t origin );

float BotGapDistance( vec3_t origin, vec3_t hordir, int entnum );

int AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags );

int AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos );

qboolean AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos );

void AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking );

int BotExportTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 ) {

	return 0;
}

/*
============
Init_EA_Export
============
*/
static void Init_EA_Export( ea_export_t *ea ) {
	
}

/*
============
Init_AI_Export
============
*/
static void Init_AI_Export( ai_export_t *ai ) {
	//-----------------------------------
	// be_ai_char.h
	//-----------------------------------
	ai->BotLoadCharacter = BotLoadCharacter;
	ai->BotFreeCharacter = BotFreeCharacter;
	ai->Characteristic_Float = Characteristic_Float;
	ai->Characteristic_BFloat = Characteristic_BFloat;
	ai->Characteristic_Integer = Characteristic_Integer;
	ai->Characteristic_BInteger = Characteristic_BInteger;
	ai->Characteristic_String = Characteristic_String;
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	ai->BotAllocChatState = BotAllocChatState;
	ai->BotFreeChatState = BotFreeChatState;
	ai->BotQueueConsoleMessage = BotQueueConsoleMessage;
	ai->BotRemoveConsoleMessage = BotRemoveConsoleMessage;
	ai->BotNextConsoleMessage = BotNextConsoleMessage;
	ai->BotNumConsoleMessages = BotNumConsoleMessages;
	ai->BotInitialChat = BotInitialChat;
	ai->BotNumInitialChats = BotNumInitialChats;
	ai->BotReplyChat = BotReplyChat;
	ai->BotChatLength = BotChatLength;
	ai->BotEnterChat = BotEnterChat;
	ai->BotGetChatMessage = BotGetChatMessage;
	ai->StringContains = StringContains;
	ai->BotFindMatch = BotFindMatch;
	ai->BotMatchVariable = BotMatchVariable;
	ai->UnifyWhiteSpaces = UnifyWhiteSpaces;
	ai->BotReplaceSynonyms = BotReplaceSynonyms;
	ai->BotLoadChatFile = BotLoadChatFile;
	ai->BotSetChatGender = BotSetChatGender;
	ai->BotSetChatName = BotSetChatName;
	//-----------------------------------
	// be_ai_goal.h
	//-----------------------------------
	ai->BotResetGoalState = BotResetGoalState;
	ai->BotResetAvoidGoals = BotResetAvoidGoals;
	ai->BotRemoveFromAvoidGoals = BotRemoveFromAvoidGoals;
	ai->BotPushGoal = BotPushGoal;
	ai->BotPopGoal = BotPopGoal;
	ai->BotEmptyGoalStack = BotEmptyGoalStack;
	ai->BotDumpAvoidGoals = BotDumpAvoidGoals;
	ai->BotDumpGoalStack = BotDumpGoalStack;
	ai->BotGoalName = BotGoalName;
	ai->BotGetTopGoal = BotGetTopGoal;
	ai->BotGetSecondGoal = BotGetSecondGoal;
	ai->BotChooseLTGItem = BotChooseLTGItem;
	ai->BotChooseNBGItem = BotChooseNBGItem;
	ai->BotTouchingGoal = BotTouchingGoal;
	ai->BotItemGoalInVisButNotVisible = BotItemGoalInVisButNotVisible;
	ai->BotGetLevelItemGoal = BotGetLevelItemGoal;
	ai->BotGetNextCampSpotGoal = BotGetNextCampSpotGoal;
	ai->BotGetMapLocationGoal = BotGetMapLocationGoal;
	ai->BotAvoidGoalTime = BotAvoidGoalTime;
	ai->BotInitLevelItems = BotInitLevelItems;
	ai->BotUpdateEntityItems = BotUpdateEntityItems;
	ai->BotLoadItemWeights = BotLoadItemWeights;
	ai->BotFreeItemWeights = BotFreeItemWeights;
	ai->BotInterbreedGoalFuzzyLogic = BotInterbreedGoalFuzzyLogic;
	ai->BotSaveGoalFuzzyLogic = BotSaveGoalFuzzyLogic;
	ai->BotMutateGoalFuzzyLogic = BotMutateGoalFuzzyLogic;
	ai->BotAllocGoalState = BotAllocGoalState;
	ai->BotFreeGoalState = BotFreeGoalState;
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotResetMoveState = BotResetMoveState;
	ai->BotMoveToGoal = BotMoveToGoal;
	ai->BotMoveInDirection = BotMoveInDirection;
	ai->BotResetAvoidReach = BotResetAvoidReach;
	ai->BotResetLastAvoidReach = BotResetLastAvoidReach;
	ai->BotReachabilityArea = BotReachabilityArea;
	ai->BotMovementViewTarget = BotMovementViewTarget;
	ai->BotPredictVisiblePosition = BotPredictVisiblePosition;
	ai->BotAllocMoveState = BotAllocMoveState;
	ai->BotFreeMoveState = BotFreeMoveState;
	ai->BotInitMoveState = BotInitMoveState;
	// Ridah
	ai->BotInitAvoidReach = BotInitAvoidReach;
	// done.
	//-----------------------------------
	// be_ai_weap.h
	//-----------------------------------
	ai->BotChooseBestFightWeapon = BotChooseBestFightWeapon;
	ai->BotGetWeaponInfo = BotGetWeaponInfo;
	ai->BotLoadWeaponWeights = BotLoadWeaponWeights;
	ai->BotAllocWeaponState = BotAllocWeaponState;
	ai->BotFreeWeaponState = BotFreeWeaponState;
	ai->BotResetWeaponState = BotResetWeaponState;
	//-----------------------------------
	// be_ai_gen.h
	//-----------------------------------
	ai->GeneticParentsAndChildSelection = GeneticParentsAndChildSelection;
}

