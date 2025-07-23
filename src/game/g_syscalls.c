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
#include "../botlib/be_aas_main.h"
#include "../botlib/be_aas_reach.h"
#include "../botlib/be_aas_sample.h"
#include "../game/be_ea.h"
#include "../botlib/botlib.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"

static int QDECL dummySyscall(int arg, ...){
	return 0;
}

static int ( QDECL * syscall )( int arg, ... ) = dummySyscall;

void dllEntry_G( int ( QDECL *syscallptr )( int arg,... ) ) {
	syscall = syscallptr;
}

static int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

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

int     trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	return FS_Write( buffer, len, f );
}

int     trap_FS_Rename( const char *from, const char *to ) {
	return syscall( G_FS_RENAME, from, to );
}

void    trap_FS_FCloseFile( fileHandle_t f ) {
	FS_FCloseFile( f );
}

void    trap_FS_CopyFile( char *from, char *to ) {  //DAJ
	syscall( G_FS_COPY_FILE, from, to );
}

int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return FS_GetFileList( path, extension, listbuf, bufsize );
}

void    trap_game_SendConsoleCommand( int exec_when, const char *text ) {
	syscall( G_SEND_CONSOLE_COMMAND, exec_when, text );
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

qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	return syscall( G_IN_PVS_IGNORE_PORTALS, p1, p2 );
}

void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	syscall( G_ADJUST_AREA_PORTAL_STATE, ent, open );
}

qboolean trap_AreasConnected( int area1, int area2 ) {
	return syscall( G_AREAS_CONNECTED, area1, area2 );
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
	return syscall( G_DEBUG_POLYGON_CREATE, color, numPoints, points );
}

void trap_DebugPolygonDelete( int id ) {
	syscall( G_DEBUG_POLYGON_DELETE, id );
}

int trap_RealTime( qtime_t *qtime ) {
	return Com_RealTime( qtime );
}

void trap_SnapVector( float *v ) {
	Sys_SnapVector( v );
}

qboolean trap_GetTag( int clientNum, char *tagName, orientation_t *or ) {
	return SV_GetTag(clientNum, tagName, or );
}

// BotLib traps start here
int trap_BotLibSetup( void ) {
	return SV_BotLibSetup();
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

int trap_BotLibDefine( char *string ) {
	return syscall( BOTLIB_PC_ADD_GLOBAL_DEFINE, string );
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

int trap_BotLibUpdateEntity( int ent, void /* struct bot_updateentity_s */ *bue ) {
	return syscall( BOTLIB_UPDATENTITY, ent, bue );
}

int trap_BotLibTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 ) {
	return syscall( BOTLIB_TEST, parm0, parm1, parm2, parm3 );
}

int trap_BotGetSnapshotEntity( int clientNum, int sequence ) {
	return syscall( BOTLIB_GET_SNAPSHOT_ENTITY, clientNum, sequence );
}

int trap_BotGetServerCommand( int clientNum, char *message, int size ) {
	return syscall( BOTLIB_GET_CONSOLE_MESSAGE, clientNum, message, size );
}

void trap_BotUserCommand( int clientNum, usercmd_t *ucmd ) {
	SV_ClientThink( &svs.clients[clientNum], ucmd );
}

void trap_AAS_EntityInfo( int entnum, void /* struct aas_entityinfo_s */ *info ) {
	syscall( BOTLIB_AAS_ENTITY_INFO, entnum, info );
}

int trap_AAS_Initialized( void ) {
	return AAS_Initialized( );
}

void trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs ) {
	syscall( BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX, presencetype, mins, maxs );
}

float trap_AAS_Time( void ) {
	return AAS_Time();
}

// Ridah, multiple AAS files
void trap_AAS_SetCurrentWorld( int index ) {
	AAS_SetCurrentWorld( index );
}
// done.

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
	return syscall( BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA, areanum, origin, goalareanum, travelflags );
}

int trap_AAS_Swimming( vec3_t origin ) {
	return syscall( BOTLIB_AAS_SWIMMING, origin );
}

int trap_AAS_PredictClientMovement( void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize ) {
	return syscall( BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT, move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, PASSFLOAT( frametime ), stopevent, stopareanum, visualize );
}

// Ridah, route-tables
void trap_AAS_RT_ShowRoute( vec3_t srcpos, int srcnum, int destnum ) {
	syscall( BOTLIB_AAS_RT_SHOWROUTE, srcpos, srcnum, destnum );
}

qboolean trap_AAS_RT_GetHidePos( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos ) {
	return syscall( BOTLIB_AAS_RT_GETHIDEPOS, srcpos, srcnum, srcarea, destpos, destnum, destarea, returnPos );
}

int trap_AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos ) {
	return syscall( BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE, srcnum, rangenum, enemynum, PASSFLOAT( rangedist ), travelflags, outpos );
}

qboolean trap_AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos ) {
	return syscall( BOTLIB_AAS_GETROUTEFIRSTVISPOS, srcpos, destpos, travelflags, retpos );
}

void trap_AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking ) {
	syscall( BOTLIB_AAS_SETAASBLOCKINGENTITY, absmin, absmax, blocking );
}
// done.

void trap_EA_Say( int client, char *str ) {
	syscall( BOTLIB_EA_SAY, client, str );
}

void trap_EA_SayTeam( int client, char *str ) {
	syscall( BOTLIB_EA_SAY_TEAM, client, str );
}

void trap_EA_UseItem( int client, char *it ) {
	syscall( BOTLIB_EA_USE_ITEM, client, it );
}

void trap_EA_DropItem( int client, char *it ) {
	syscall( BOTLIB_EA_DROP_ITEM, client, it );
}

void trap_EA_UseInv( int client, char *inv ) {
	syscall( BOTLIB_EA_USE_INV, client, inv );
}

void trap_EA_DropInv( int client, char *inv ) {
	syscall( BOTLIB_EA_DROP_INV, client, inv );
}

void trap_EA_Gesture( int client ) {
	syscall( BOTLIB_EA_GESTURE, client );
}

void trap_EA_Command( int client, char *command ) {
	syscall( BOTLIB_EA_COMMAND, client, command );
}

void trap_EA_SelectWeapon( int client, int weapon ) {
	syscall( BOTLIB_EA_SELECT_WEAPON, client, weapon );
}

void trap_EA_Talk( int client ) {
	syscall( BOTLIB_EA_TALK, client );
}

void trap_EA_Attack( int client ) {
	syscall( BOTLIB_EA_ATTACK, client );
}

void trap_EA_Reload( int client ) {
	syscall( BOTLIB_EA_RELOAD, client );
}

void trap_EA_Use( int client ) {
	syscall( BOTLIB_EA_USE, client );
}

void trap_EA_Respawn( int client ) {
	syscall( BOTLIB_EA_RESPAWN, client );
}

void trap_EA_Jump( int client ) {
	syscall( BOTLIB_EA_JUMP, client );
}

void trap_EA_DelayedJump( int client ) {
	syscall( BOTLIB_EA_DELAYED_JUMP, client );
}

void trap_EA_Crouch( int client ) {
	syscall( BOTLIB_EA_CROUCH, client );
}

void trap_EA_MoveUp( int client ) {
	syscall( BOTLIB_EA_MOVE_UP, client );
}

void trap_EA_MoveDown( int client ) {
	syscall( BOTLIB_EA_MOVE_DOWN, client );
}

void trap_EA_MoveForward( int client ) {
	syscall( BOTLIB_EA_MOVE_FORWARD, client );
}

void trap_EA_MoveBack( int client ) {
	syscall( BOTLIB_EA_MOVE_BACK, client );
}

void trap_EA_MoveLeft( int client ) {
	syscall( BOTLIB_EA_MOVE_LEFT, client );
}

void trap_EA_MoveRight( int client ) {
	syscall( BOTLIB_EA_MOVE_RIGHT, client );
}

void trap_EA_Move( int client, vec3_t dir, float speed ) {
	syscall( BOTLIB_EA_MOVE, client, dir, PASSFLOAT( speed ) );
}

void trap_EA_View( int client, vec3_t viewangles ) {
	EA_View(client, viewangles );
}

void trap_EA_EndRegular( int client, float thinktime ) {
	syscall( BOTLIB_EA_END_REGULAR, client, PASSFLOAT( thinktime ) );
}

void trap_EA_GetInput( int client, float thinktime, void /* struct bot_input_s */ *input ) {
	EA_GetInput(client, thinktime, input );
}

void trap_EA_ResetInput( int client, void *init ) {
	EA_ResetInput( client, init );
}

int trap_BotLoadCharacter( char *charfile, int skill ) {
	return syscall( BOTLIB_AI_LOAD_CHARACTER, charfile, skill );
}

void trap_BotFreeCharacter( int character ) {
	syscall( BOTLIB_AI_FREE_CHARACTER, character );
}

float trap_Characteristic_Float( int character, int index ) {
	int temp;
	temp = syscall( BOTLIB_AI_CHARACTERISTIC_FLOAT, character, index );
	return ( *(float*)&temp );
}

float trap_Characteristic_BFloat( int character, int index, float min, float max ) {
	int temp;
	temp = syscall( BOTLIB_AI_CHARACTERISTIC_BFLOAT, character, index, PASSFLOAT( min ), PASSFLOAT( max ) );
	return ( *(float*)&temp );
}

int trap_Characteristic_Integer( int character, int index ) {
	return syscall( BOTLIB_AI_CHARACTERISTIC_INTEGER, character, index );
}

int trap_Characteristic_BInteger( int character, int index, int min, int max ) {
	return syscall( BOTLIB_AI_CHARACTERISTIC_BINTEGER, character, index, min, max );
}

void trap_Characteristic_String( int character, int index, char *buf, int size ) {
	syscall( BOTLIB_AI_CHARACTERISTIC_STRING, character, index, buf, size );
}

int trap_BotAllocChatState( void ) {
	return syscall( BOTLIB_AI_ALLOC_CHAT_STATE );
}

void trap_BotFreeChatState( int handle ) {
	syscall( BOTLIB_AI_FREE_CHAT_STATE, handle );
}

void trap_BotQueueConsoleMessage( int chatstate, int type, char *message ) {
	syscall( BOTLIB_AI_QUEUE_CONSOLE_MESSAGE, chatstate, type, message );
}

void trap_BotRemoveConsoleMessage( int chatstate, int handle ) {
	syscall( BOTLIB_AI_REMOVE_CONSOLE_MESSAGE, chatstate, handle );
}

int trap_BotNextConsoleMessage( int chatstate, void /* struct bot_consolemessage_s */ *cm ) {
	return syscall( BOTLIB_AI_NEXT_CONSOLE_MESSAGE, chatstate, cm );
}

int trap_BotNumConsoleMessages( int chatstate ) {
	return syscall( BOTLIB_AI_NUM_CONSOLE_MESSAGE, chatstate );
}

void trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	syscall( BOTLIB_AI_INITIAL_CHAT, chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

int trap_BotNumInitialChats( int chatstate, char *type ) {
	return syscall( BOTLIB_AI_NUM_INITIAL_CHATS, chatstate, type );
}

int trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 ) {
	return syscall( BOTLIB_AI_REPLY_CHAT, chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

int trap_BotChatLength( int chatstate ) {
	return syscall( BOTLIB_AI_CHAT_LENGTH, chatstate );
}

void trap_BotEnterChat( int chatstate, int client, int sendto ) {
	syscall( BOTLIB_AI_ENTER_CHAT, chatstate, client, sendto );
}

void trap_BotGetChatMessage( int chatstate, char *buf, int size ) {
	syscall( BOTLIB_AI_GET_CHAT_MESSAGE, chatstate, buf, size );
}

int trap_StringContains( char *str1, char *str2, int casesensitive ) {
	return syscall( BOTLIB_AI_STRING_CONTAINS, str1, str2, casesensitive );
}

int trap_BotFindMatch( char *str, void /* struct bot_match_s */ *match, unsigned long int context ) {
	return syscall( BOTLIB_AI_FIND_MATCH, str, match, context );
}

void trap_BotMatchVariable( void /* struct bot_match_s */ *match, int variable, char *buf, int size ) {
	syscall( BOTLIB_AI_MATCH_VARIABLE, match, variable, buf, size );
}

void trap_UnifyWhiteSpaces( char *string ) {
	syscall( BOTLIB_AI_UNIFY_WHITE_SPACES, string );
}

void trap_BotReplaceSynonyms( char *string, unsigned long int context ) {
	syscall( BOTLIB_AI_REPLACE_SYNONYMS, string, context );
}

int trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname ) {
	return syscall( BOTLIB_AI_LOAD_CHAT_FILE, chatstate, chatfile, chatname );
}

void trap_BotSetChatGender( int chatstate, int gender ) {
	syscall( BOTLIB_AI_SET_CHAT_GENDER, chatstate, gender );
}

void trap_BotSetChatName( int chatstate, char *name ) {
	syscall( BOTLIB_AI_SET_CHAT_NAME, chatstate, name );
}

void trap_BotResetGoalState( int goalstate ) {
	syscall( BOTLIB_AI_RESET_GOAL_STATE, goalstate );
}

void trap_BotResetAvoidGoals( int goalstate ) {
	syscall( BOTLIB_AI_RESET_AVOID_GOALS, goalstate );
}

void trap_BotRemoveFromAvoidGoals( int goalstate, int number ) {
	syscall( BOTLIB_AI_REMOVE_FROM_AVOID_GOALS, goalstate, number );
}

void trap_BotPushGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	syscall( BOTLIB_AI_PUSH_GOAL, goalstate, goal );
}

void trap_BotPopGoal( int goalstate ) {
	syscall( BOTLIB_AI_POP_GOAL, goalstate );
}

void trap_BotEmptyGoalStack( int goalstate ) {
	syscall( BOTLIB_AI_EMPTY_GOAL_STACK, goalstate );
}

void trap_BotDumpAvoidGoals( int goalstate ) {
	syscall( BOTLIB_AI_DUMP_AVOID_GOALS, goalstate );
}

void trap_BotDumpGoalStack( int goalstate ) {
	syscall( BOTLIB_AI_DUMP_GOAL_STACK, goalstate );
}

void trap_BotGoalName( int number, char *name, int size ) {
	syscall( BOTLIB_AI_GOAL_NAME, number, name, size );
}

int trap_BotGetTopGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_GET_TOP_GOAL, goalstate, goal );
}

int trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_GET_SECOND_GOAL, goalstate, goal );
}

int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags ) {
	return syscall( BOTLIB_AI_CHOOSE_LTG_ITEM, goalstate, origin, inventory, travelflags );
}

int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime ) {
	return syscall( BOTLIB_AI_CHOOSE_NBG_ITEM, goalstate, origin, inventory, travelflags, ltg, PASSFLOAT( maxtime ) );
}

int trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_TOUCHING_GOAL, origin, goal );
}

int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE, viewer, eye, viewangles, goal );
}

int trap_BotGetLevelItemGoal( int index, char *classname, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_GET_LEVEL_ITEM_GOAL, index, classname, goal );
}

int trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL, num, goal );
}

int trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal ) {
	return syscall( BOTLIB_AI_GET_MAP_LOCATION_GOAL, name, goal );
}

float trap_BotAvoidGoalTime( int goalstate, int number ) {
	int temp;
	temp = syscall( BOTLIB_AI_AVOID_GOAL_TIME, goalstate, number );
	return ( *(float*)&temp );
}

void trap_BotInitLevelItems( void ) {
	syscall( BOTLIB_AI_INIT_LEVEL_ITEMS );
}

void trap_BotUpdateEntityItems( void ) {
	syscall( BOTLIB_AI_UPDATE_ENTITY_ITEMS );
}

int trap_BotLoadItemWeights( int goalstate, char *filename ) {
	return syscall( BOTLIB_AI_LOAD_ITEM_WEIGHTS, goalstate, filename );
}

void trap_BotFreeItemWeights( int goalstate ) {
	syscall( BOTLIB_AI_FREE_ITEM_WEIGHTS, goalstate );
}

void trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child ) {
	syscall( BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC, parent1, parent2, child );
}

void trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename ) {
	syscall( BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC, goalstate, filename );
}

void trap_BotMutateGoalFuzzyLogic( int goalstate, float range ) {
	syscall( BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC, goalstate, range );
}

int trap_BotAllocGoalState( int state ) {
	return BotAllocGoalState(state );
}

void trap_BotFreeGoalState( int handle ) {
	syscall( BOTLIB_AI_FREE_GOAL_STATE, handle );
}

void trap_BotResetMoveState( int movestate ) {
	syscall( BOTLIB_AI_RESET_MOVE_STATE, movestate );
}

void trap_BotMoveToGoal( void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags ) {
	syscall( BOTLIB_AI_MOVE_TO_GOAL, result, movestate, goal, travelflags );
}

int trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type ) {
	return syscall( BOTLIB_AI_MOVE_IN_DIRECTION, movestate, dir, PASSFLOAT( speed ), type );
}

void trap_BotResetAvoidReach( int movestate ) {
	syscall( BOTLIB_AI_RESET_AVOID_REACH, movestate );
}

void trap_BotResetLastAvoidReach( int movestate ) {
	syscall( BOTLIB_AI_RESET_LAST_AVOID_REACH,movestate  );
}

int trap_BotReachabilityArea( vec3_t origin, int testground ) {
	return syscall( BOTLIB_AI_REACHABILITY_AREA, origin, testground );
}

int trap_BotMovementViewTarget( int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target ) {
	return syscall( BOTLIB_AI_MOVEMENT_VIEW_TARGET, movestate, goal, travelflags, PASSFLOAT( lookahead ), target );
}

int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target ) {
	return syscall( BOTLIB_AI_PREDICT_VISIBLE_POSITION, origin, areanum, goal, travelflags, target );
}

int trap_BotAllocMoveState( void ) {
	return BotAllocMoveState( );
}

void trap_BotFreeMoveState( int handle ) {
	BotFreeMoveState( handle );
}

void trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove ) {
	syscall( BOTLIB_AI_INIT_MOVE_STATE, handle, initmove );
}

// Ridah
void trap_BotInitAvoidReach( int handle ) {
	BotInitAvoidReach( handle );
}
// Done.

int trap_BotChooseBestFightWeapon( int weaponstate, int *inventory ) {
	return syscall( BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON, weaponstate, inventory );
}

void trap_BotGetWeaponInfo( int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo ) {
	syscall( BOTLIB_AI_GET_WEAPON_INFO, weaponstate, weapon, weaponinfo );
}

int trap_BotLoadWeaponWeights( int weaponstate, char *filename ) {
	return syscall( BOTLIB_AI_LOAD_WEAPON_WEIGHTS, weaponstate, filename );
}

int trap_BotAllocWeaponState( void ) {
	return syscall( BOTLIB_AI_ALLOC_WEAPON_STATE );
}

void trap_BotFreeWeaponState( int weaponstate ) {
	syscall( BOTLIB_AI_FREE_WEAPON_STATE, weaponstate );
}

void trap_BotResetWeaponState( int weaponstate ) {
	syscall( BOTLIB_AI_RESET_WEAPON_STATE, weaponstate );
}

int trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child ) {
	return syscall( BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION, numranks, ranks, parent1, parent2, child );
}
