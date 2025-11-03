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

void    trap_Printf( const char *fmt ) {
	Com_Printf( "%s", fmt );
}

void    trap_Error( const char *fmt ) {
	Com_Error( ERR_DROP, "%s", fmt );
}

void    trap_Endgame( void ) {
	Com_Error( ERR_ENDGAME, "endgame" );
}

int     trap_Milliseconds( void ) {
	return Sys_Milliseconds();
}
int     trap_Argc( void ) {
	return Cmd_Argc(  );
}

void    trap_Argv( int n, char *buffer, int bufferLength ) {
	Cmd_ArgvBuffer(n, buffer, bufferLength );
}

int     trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return FS_FOpenFileByMode(qpath, f, mode );
}

void    trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	FS_Read(buffer, len, f );
}


int     trap_FS_Rename( const char *from, const char *to ) {
	FS_Rename(from, to );
	return 0;
}

void    trap_FS_FCloseFile( fileHandle_t f ) {
	FS_FCloseFile( f );
}

void    trap_FS_CopyFile( char *from, char *to ) {  //DAJ
	FS_CopyFileOS(from, to );
}

int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return FS_GetFileList( path, extension, listbuf, bufsize );
}

void    trap_game_SendConsoleCommand( int exec_when, const char *text ) {
	Cbuf_ExecuteText(exec_when, text );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	Cvar_Register(cvar, var_name, value, flags );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	Cvar_Set( var_name, value );
}

int trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return Cvar_VariableIntegerValue( var_name );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	Cvar_VariableStringBuffer( var_name, buffer, bufsize );
}

void trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
						  playerState_t *clients, int sizeofGClient ) {
	SV_LocateGameData(gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient );
}

void trap_DropClient( int clientNum, const char *reason ) {
	SV_GameDropClient( clientNum, reason );
}

extern void SV_GameSendServerCommand( int clientNum, const char *text );
void trap_SendServerCommand( int clientNum, const char *text ) {
	SV_GameSendServerCommand( clientNum, text );
}

void trap_SetConfigstring( int num, const char *string ) {
	SV_SetConfigstring( num, string );
}

void trap_GetConfigstring( int num, char *buffer, int bufferSize ) {
	SV_GetConfigstring(num, buffer, bufferSize );
}

void trap_GetUserinfo( int num, char *buffer, int bufferSize ) {
	SV_GetUserinfo(num, buffer, bufferSize );
}

void trap_SetUserinfo( int num, const char *buffer ) {
	SV_SetUserinfo( num, buffer );
}

extern void SV_GetServerinfo( char *buffer, int bufferSize );
void trap_GetServerinfo( char *buffer, int bufferSize ) {
	SV_GetServerinfo(buffer, bufferSize );
}

void trap_SetBrushModel( gentity_t *ent, const char *name ) {
	SV_SetBrushModel( ent, name );
}

void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	SV_Trace( results, start, mins, maxs, end, passEntityNum, contentmask, qfalse );
}

void trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	SV_Trace( results, start, mins, maxs, end, passEntityNum, contentmask, qtrue );
}

int trap_PointContents( const vec3_t point, int passEntityNum ) {
	return SV_PointContents(point, passEntityNum );
}


qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 ) {
	return SV_inPVS(p1, p2 );
}

qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	return SV_inPVSIgnorePortals(p1, p2 );
}

void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open );
void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	SV_AdjustAreaPortalState(ent, open );
}

void trap_LinkEntity( gentity_t *ent ) {
	SV_LinkEntity( ent );
}

void trap_UnlinkEntity( gentity_t *ent ) {
	SV_UnlinkEntity( ent );
}

int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount ) {
	return SV_AreaEntities(mins, maxs, list, maxcount );
}

extern qboolean    SV_EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, const int capsule );
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return SV_EntityContact(mins, maxs, ent, qfalse );
}

qboolean trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return SV_EntityContact(mins, maxs, ent, qtrue );
}

int trap_BotAllocateClient( void ) {
	return SV_BotAllocateClient();
}

void trap_BotFreeClient( int clientNum ) {
	SV_BotFreeClient(clientNum );
}

extern void SV_GetUsercmd( int clientNum, usercmd_t *cmd );
void trap_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	SV_GetUsercmd(clientNum, cmd );
}

qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	
	const char  *s = COM_Parse( &sv.entityParsePoint );
	Q_strncpyz( buffer, s, bufferSize );
	if ( !sv.entityParsePoint && !s[0] ) {
		return qfalse;
	} else {
		return qtrue;
	}
}

int trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points ) {
	return BotImport_DebugPolygonCreate(color, numPoints, points );
}

void trap_DebugPolygonDelete( int id ) {
	BotImport_DebugPolygonDelete(id );
}

time_t trap_RealTime( qtime_t *qtime ) {
	return Com_RealTime( qtime );
}

void trap_SnapVector( float *v ) {
	Sys_SnapVector( v );
}


int trap_BotLibShutdown( void ) {
	return SV_BotLibShutdown();
}


extern int Export_BotLibVarSet( char *var_name, char *value );
int trap_BotLibVarSet( char *var_name, char *value ) {
	return Export_BotLibVarSet(var_name, value );
}

extern int Export_BotLibVarGet( char *var_name, char *value, int size );
int trap_BotLibVarGet( char *var_name, char *value, int size ) {
	return Export_BotLibVarGet(var_name, value, size );
}

extern int PC_AddGlobalDefine( char *string );
int trap_BotLibDefine( char *string ) {
	return PC_AddGlobalDefine( string );
}

extern qboolean BotLibSetup( char *str );
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
	
	return Export_BotLibUpdateEntity(ent, bue );
}

int trap_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return SV_BotGetSnapshotEntity(clientNum, sequence );
}

int trap_BotGetServerCommand( int clientNum, char *message, int size ) {
	return SV_BotGetConsoleMessage(clientNum, message, size );
}

void trap_BotUserCommand( int clientNum, usercmd_t *ucmd ) {
	SV_ClientThink( &svs.clients[clientNum], ucmd );
}

void trap_AAS_EntityInfo( int entnum, void /* struct aas_entityinfo_s */ *info ) {
	AAS_EntityInfo(entnum, info );
}

int trap_AAS_Initialized( void ) {
	return AAS_Initialized( );
}

void trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs ) {
	AAS_PresenceTypeBoundingBox(presencetype, mins, maxs );
}

float trap_AAS_Time( void ) {
	return AAS_Time();
}

int trap_AAS_PointAreaNum( vec3_t point ) {
	return AAS_PointAreaNum( point );
}

int trap_AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas ) {
	return AAS_TraceAreas( start, end, areas, points, maxareas );
}

int trap_AAS_PointContents( vec3_t point ) {
	return AAS_PointContents(point );
}

int trap_AAS_NextBSPEntity( int ent ) {
	return AAS_NextBSPEntity(ent);
}

int trap_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size ) {
	return AAS_ValueForBSPEpairKey( ent, key, value, size );
}

int trap_AAS_VectorForBSPEpairKey( int ent, char *key, vec3_t v ) {
	return AAS_VectorForBSPEpairKey( ent, key, v );
}

int trap_AAS_FloatForBSPEpairKey( int ent, char *key, float *value ) {
	return AAS_FloatForBSPEpairKey( ent, key, value );
}

int trap_AAS_IntForBSPEpairKey( int ent, char *key, int *value ) {
	return AAS_IntForBSPEpairKey( ent, key, value );
}

int trap_AAS_AreaReachability( int areanum ) {
	return AAS_AreaReachability(areanum );
}

int trap_AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags ) {
	return AAS_AreaTravelTimeToGoalArea(areanum, origin, goalareanum, travelflags );
}

int trap_AAS_Swimming( vec3_t origin ) {
	return AAS_Swimming( origin );
}

int trap_AAS_PredictClientMovement( void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) {
	return AAS_PredictClientMovement(move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, frametime, stopevent, stopareanum, visualize );
}

// Ridah, route-tables
void trap_AAS_RT_ShowRoute( vec3_t srcpos, int srcnum, int destnum ) {
	AAS_RT_ShowRoute(srcpos, srcnum, destnum );
}

qboolean trap_AAS_RT_GetHidePos( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos ) {
	return AAS_RT_GetHidePos(srcpos, srcnum, srcarea, destpos, destnum, destarea, returnPos );
}

extern int AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos );
int trap_AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos ) {
	return AAS_FindAttackSpotWithinRange(srcnum, rangenum, enemynum, rangedist, travelflags, outpos );
}

extern qboolean AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos );
qboolean trap_AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos ) {
	return AAS_GetRouteFirstVisPos(srcpos, destpos, travelflags, retpos );
}

extern void AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking );
void trap_AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking ) {
	AAS_SetAASBlockingEntity( absmin, absmax, blocking );
}
// done.

void trap_EA_Say( int client, char *str ) {
	EA_Say(client, str );
}

void trap_EA_SayTeam( int client, char *str ) {
	EA_SayTeam(client, str );
}

void trap_EA_UseItem( int client, char *it ) {
	EA_UseItem( client, it );
}

void trap_EA_DropItem( int client, char *it ) {
	EA_DropItem( client, it );
}

void trap_EA_UseInv( int client, char *inv ) {
	EA_UseInv( client, inv );
}

void trap_EA_DropInv( int client, char *inv ) {
	EA_DropInv(client, inv );
}

void trap_EA_Gesture( int client ) {
	EA_Gesture(client );
}

void trap_EA_Command( int client, char *command ) {
	EA_Command(client, command );
}

void trap_EA_SelectWeapon( int client, int weapon ) {
	EA_SelectWeapon(client, weapon );
}

void trap_EA_Talk( int client ) {
	EA_Talk(client );
}

void trap_EA_Attack( int client ) {
	EA_Attack(client );
}

void trap_EA_Reload( int client ) {
	EA_Reload( client );
}

void trap_EA_Use( int client ) {
	EA_Use( client );
}

void trap_EA_Respawn( int client ) {
	EA_Respawn(client );
}

void trap_EA_Jump( int client ) {
	EA_Jump( client );
}

void trap_EA_DelayedJump( int client ) {
	EA_DelayedJump( client );
}

void trap_EA_Crouch( int client ) {
	EA_Crouch( client );
}

void trap_EA_MoveUp( int client ) {
	EA_MoveUp( client );
}

void trap_EA_MoveDown( int client ) {
	EA_MoveDown( client );
}

void trap_EA_MoveForward( int client ) {
	EA_MoveForward( client );
}

void trap_EA_MoveBack( int client ) {
	EA_MoveBack( client );
}

void trap_EA_MoveLeft( int client ) {
	EA_MoveLeft( client );
}

void trap_EA_MoveRight( int client ) {
	EA_MoveRight( client );
}

void trap_EA_Move( int client, vec3_t dir, float speed ) {
	EA_Move(client, dir, speed );
}

void trap_EA_View( int client, vec3_t viewangles ) {
	EA_View(client, viewangles );
}

void trap_EA_EndRegular( int client, float thinktime ) {
	EA_EndRegular( client, thinktime );
}

void trap_EA_GetInput( int client, float thinktime, void /* struct bot_input_s */ *input ) {
	EA_GetInput(client, thinktime, input );
}

void trap_EA_ResetInput( int client, void *init ) {
	EA_ResetInput( client, init );
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

int trap_BotAllocChatState( void ) {
	return BotAllocChatState( );
}

void trap_BotFreeChatState( int handle ) {
	BotFreeChatState( handle );
}

void trap_BotQueueConsoleMessage( int chatstate, int type, char *message ) {
	BotQueueConsoleMessage( chatstate, type, message );
}

void trap_BotRemoveConsoleMessage( int chatstate, int handle ) {
	BotRemoveConsoleMessage(chatstate, handle );
}

int trap_BotNextConsoleMessage( int chatstate, void /* struct bot_consolemessage_s */ *cm ) {
	return BotNextConsoleMessage(chatstate, cm );
}

int trap_BotNumConsoleMessages( int chatstate ) {
	return BotNumConsoleMessages(chatstate );
}

void trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	BotInitialChat(chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

int trap_BotNumInitialChats( int chatstate, char *type ) {
	return BotNumInitialChats(chatstate, type );
}

int trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	return BotReplyChat(chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

int trap_BotChatLength( int chatstate ) {
	return BotChatLength(chatstate );
}

void trap_BotEnterChat( int chatstate, int client, int sendto ) {
	BotEnterChat(chatstate, client, sendto );
}

void trap_BotGetChatMessage( int chatstate, char *buf, int size ) {
	BotGetChatMessage(chatstate, buf, size );
}

int trap_StringContains( char *str1, char *str2, int casesensitive ) {
	return StringContains(str1, str2, casesensitive );
}

int trap_BotFindMatch( char *str, void /* struct bot_match_s */ *match, unsigned long int context ) {
	return BotFindMatch(str, match, context );
}

void trap_BotMatchVariable( void /* struct bot_match_s */ *match, int variable, char *buf, int size ) {
	BotMatchVariable(match, variable, buf, size );
}

void trap_UnifyWhiteSpaces( char *string ) {
	UnifyWhiteSpaces(string );
}

void trap_BotReplaceSynonyms( char *string, unsigned long int context ) {
	BotReplaceSynonyms(string, context );
}

int trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname ) {
	return BotLoadChatFile(chatstate, chatfile, chatname );
}

void trap_BotSetChatGender( int chatstate, int gender ) {
	BotSetChatGender(chatstate, gender );
}

void trap_BotSetChatName( int chatstate, char *name ) {
	BotSetChatName(chatstate, name );
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
	BotPushGoal(goalstate, goal );
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
	return BotGetTopGoal(goalstate, goal );
}

int trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return BotGetSecondGoal(goalstate, goal );
}

int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	return BotChooseLTGItem(goalstate, origin, inventory, travelflags );
}

int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime ) {
	return BotChooseNBGItem(goalstate, origin, inventory, travelflags, ltg, maxtime );
}

int trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal ) {
	return BotTouchingGoal(origin, goal );
}

int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal ) {
	return BotItemGoalInVisButNotVisible(viewer, eye, viewangles, goal );
}

int trap_BotGetLevelItemGoal( int index, char *classname, void /* struct bot_goal_s */ *goal ) {
	return BotGetLevelItemGoal(index, classname, goal );
}

int trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal ) {
	return BotGetNextCampSpotGoal(num, goal );
}

int trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal ) {
	return BotGetMapLocationGoal(name, goal );
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

void trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	BotInterbreedGoalFuzzyLogic(parent1, parent2, child );
}

void trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename ) {
	BotSaveGoalFuzzyLogic(goalstate, filename );
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
	BotMoveToGoal(result, movestate, goal, travelflags );
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
	return BotMovementViewTarget(movestate, goal, travelflags, lookahead, target );
}

int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target ) {
	return BotPredictVisiblePosition(origin, areanum, goal, travelflags, target );
}

int trap_BotAllocMoveState( void ) {
	return BotAllocMoveState( );
}

void trap_BotFreeMoveState( int handle ) {
	BotFreeMoveState( handle );
}

void trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove ) {
	BotInitMoveState(handle, initmove );
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
	BotGetWeaponInfo(weaponstate, weapon, weaponinfo );
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

int trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child ) {
	return GeneticParentsAndChildSelection(numranks, ranks, parent1, parent2, child );
}
