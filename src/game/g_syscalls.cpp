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

// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "qcommon.h"
#include "../server/server.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_aas_bsp.h"
#include "../botlib/be_aas_entity.h"
#include "../botlib/be_aas_main.h"
#include "../botlib/be_aas_move.h"
#include "../botlib/be_aas_reach.h"
#include "../botlib/be_aas_route.h"
#include "../botlib/be_aas_routetable.h"
#include "../botlib/be_aas_sample.h"
#include "../game/be_ea.h"
#include "../botlib/botlib.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"

int trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points ) {
	return BotImport_DebugPolygonCreate(color, numPoints, points );
}

void trap_DebugPolygonDelete( int id ) {
	BotImport_DebugPolygonDelete(id );
}

extern bool BotLibSetup( const char *str );
int trap_BotLibStartFrame( float time ) {
	if ( !BotLibSetup( "BotStartFrame" ) ) {
		return BLERR_LIBRARYNOTSETUP;
	}
	return AAS_StartFrame( time );
}

extern int Export_BotLibLoadMap( const char *mapname );
int trap_BotLibLoadMap( const char *mapname ) {
	return Export_BotLibLoadMap(mapname );
}

int Export_BotLibUpdateEntity( int ent, bot_entitystate_t *state );
int trap_BotLibUpdateEntity( int ent, void /* struct bot_updateentity_s */ *bue ) {
	
	return Export_BotLibUpdateEntity(ent, (bot_entitystate_t *)bue );
}

int trap_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return SV_BotGetSnapshotEntity(clientNum, sequence );
}

int trap_BotGetServerCommand( int clientNum, char *message, int size ) {
	return SV_BotGetConsoleMessage(clientNum, message, size );
}

void trap_BotUserCommand( int clientNum, UserCmd *ucmd ) {
	SV_ClientThink( &svs.clients[clientNum], ucmd );
}

int trap_BotLoadCharacter( char *charfile, int skill ) {
	return BotLoadCharacter( charfile, skill );
}

void trap_BotFreeCharacter( int character ) {
	BotFreeCharacter( character );
}

float trap_Characteristic_Float( int character, int index ) {
	return Characteristic_Float( character, index );
}

float trap_Characteristic_BFloat( int character, int index, float min, float max ) {
	return Characteristic_BFloat(character, index,  min, max );
}

int trap_Characteristic_Integer( int character, int index ) {
	return Characteristic_Integer( character, index );
}

int trap_Characteristic_BInteger( int character, int index, int min, int max ) {
	return Characteristic_BInteger( character, index, min, max );
}

void trap_Characteristic_String( int character, int index, char *buf, int size ) {
	Characteristic_String( character, index, buf, size );
}

void trap_BotResetGoalState( int goalstate ) {
	BotResetGoalState(goalstate );
}

void trap_BotResetAvoidGoals( int goalstate ) {
	BotResetAvoidGoals(goalstate );
}

void trap_BotRemoveFromAvoidGoals( int goalstate, int number ) {
	BotRemoveFromAvoidGoals(goalstate, number );
}

void trap_BotPushGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	BotPushGoal(goalstate, (bot_goal_t *)goal );
}

void trap_BotPopGoal( int goalstate ) {
	BotPopGoal(goalstate );
}

void trap_BotEmptyGoalStack( int goalstate ) {
	BotEmptyGoalStack(goalstate );
}

void trap_BotDumpAvoidGoals( int goalstate ) {
	BotDumpAvoidGoals(goalstate );
}

void trap_BotDumpGoalStack( int goalstate ) {
	BotDumpGoalStack(goalstate );
}

void trap_BotGoalName( int number, char *name, int size ) {
	BotGoalName(number, name, size );
}

int trap_BotGetTopGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return BotGetTopGoal(goalstate, (bot_goal_t *)goal );
}

int trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return BotGetSecondGoal(goalstate, (bot_goal_t *)goal );
}

int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	return BotChooseLTGItem(goalstate, origin, inventory, travelflags );
}

int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime ) {
	return BotChooseNBGItem(goalstate, origin, inventory, travelflags, (bot_goal_t *)ltg, maxtime );
}

int trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal ) {
	return BotTouchingGoal(origin, (bot_goal_t *)goal );
}

int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal ) {
	return BotItemGoalInVisButNotVisible(viewer, eye, viewangles, (bot_goal_t *)goal );
}

int trap_BotGetLevelItemGoal( int index, const char *classname, void /* struct bot_goal_s */ *goal ) {
	return BotGetLevelItemGoal(index, classname, (bot_goal_t *)goal );
}

int trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal ) {
	return BotGetNextCampSpotGoal(num, (bot_goal_t *)goal );
}

int trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal ) {
	return BotGetMapLocationGoal(name, (bot_goal_t *)goal );
}

float trap_BotAvoidGoalTime( int goalstate, int number ) {
	return BotAvoidGoalTime(goalstate, number );
}

void trap_BotInitLevelItems( void ) {
	BotInitLevelItems();
}

void trap_BotUpdateEntityItems( void ) {
	BotUpdateEntityItems();
}

int trap_BotLoadItemWeights( int goalstate, char *filename ) {
	return BotLoadItemWeights(goalstate, filename );
}

void trap_BotFreeItemWeights( int goalstate ) {
	BotFreeItemWeights(goalstate );
}

void trap_BotMutateGoalFuzzyLogic( int goalstate, float range ) {
	BotMutateGoalFuzzyLogic(goalstate, range );
}

int trap_BotAllocGoalState( int state ) {
	return BotAllocGoalState(state );
}

void trap_BotFreeGoalState( int handle ) {
	BotAllocGoalState(handle );
}

void trap_BotResetMoveState( int movestate ) {
	BotResetMoveState(movestate );
}

void trap_BotMoveToGoal( void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags ) {
	BotMoveToGoal((bot_moveresult_t *)result, movestate, (bot_goal_t *)goal, travelflags );
}

int trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type ) {
	return BotMoveInDirection(movestate, dir, speed, type );
}

void trap_BotResetAvoidReach( int movestate ) {
	BotResetAvoidReach( movestate );
}

void trap_BotResetLastAvoidReach( int movestate ) {
	BotResetAvoidReach(movestate  );
}

int trap_BotReachabilityArea( vec3_t origin, int testground ) {
	return BotReachabilityArea(origin, testground );
}

int trap_BotMovementViewTarget( int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target ) {
	return BotMovementViewTarget(movestate, (bot_goal_t *)goal, travelflags, lookahead, target );
}

int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target ) {
	return BotPredictVisiblePosition(origin, areanum, (bot_goal_t *)goal, travelflags, target );
}

int trap_BotAllocMoveState( void ) {
	return BotAllocMoveState( );
}

void trap_BotFreeMoveState( int handle ) {
	BotFreeMoveState( handle );
}

void trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove ) {
	BotInitMoveState(handle, (bot_initmove_t *)initmove );
}

// Ridah
void trap_BotInitAvoidReach( int handle ) {
	BotInitAvoidReach( handle );
}
// Done.

int trap_BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	return BotChooseBestFightWeapon(weaponstate, inventory );
}

void trap_BotGetWeaponInfo( int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo ) {
	BotGetWeaponInfo(weaponstate, weapon, (weaponinfo_t *)weaponinfo );
}

int trap_BotLoadWeaponWeights( int weaponstate, char *filename ) {
	return BotLoadWeaponWeights(weaponstate, filename );
}

int trap_BotAllocWeaponState( void ) {
	return BotAllocWeaponState();
}

void trap_BotFreeWeaponState( int weaponstate ) {
	BotFreeWeaponState(weaponstate );
}

void trap_BotResetWeaponState( int weaponstate ) {
	BotResetWeaponState(weaponstate );
}

