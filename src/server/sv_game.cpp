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

#include "server.h"

#include "../botlib/botlib.h"
#include "../game/g_func_decs.h"
#include "../qcommon/clip_model.h"

botlib_export_t *botlib_export;

sharedEntity_t *SV_GentityNum( size_t num )
{
	sharedEntity_t* ent = ( sharedEntity_t * )( (uint8_t *)sv.gentities + sv.gentitySize * ( num ) );
	return ent;
}

PlayerState *SV_GameClientNum( int num )
{
	PlayerState* ps = ( PlayerState * )( (uint8_t *)sv.gameClients + sv.gameClientSize * ( num ) );
	return ps;
}

ServerEntity  *SV_SvEntityForGentity( sharedEntity_t *gEnt )
{
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
		return nullptr;	// not required as ERR_DROP will do a longjump
	}
	return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity( ServerEntity *svEnt )
{
	size_t num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, const char *text )
{
	if ( clientNum == -1 ) {
		SV_SendServerCommand( nullptr, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", text );
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient( int clientNum, const char *reason )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel( sharedEntity_t *ent, const char *name )
{
	if ( !name ) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: nullptr" );
	}

	if ( name[0] != '*' ) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}


	ent->s.modelindex = atoi( name + 1 );

	clipHandle_t h = TheClipModel::get().inlineModel( ent->s.modelindex );
	idVec3 mins, maxs;
	TheClipModel::get().modelBounds( h, mins, maxs );
	VectorCopy( mins, ent->r.mins );
	VectorCopy( maxs, ent->r.maxs );
	ent->r.bmodel = true;

	ent->r.contents = -1;       // we don't know exactly what is in the brushes

	SV_LinkEntity( ent );       // FIXME: remove
}

/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
bool SV_inPVS( const vec3_t p1, const vec3_t p2 )
{
	ClipModel& clipModel = TheClipModel::get();
	int leafnum = CM_PointLeafnum( p1 );
	int cluster = clipModel.leafCluster( leafnum );
	int area1 = clipModel.leafArea( leafnum );
	uint8_t* mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = clipModel.leafCluster( leafnum );
	int area2 = clipModel.leafArea( leafnum );
	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
		return false;
	}
	if ( !CM_AreasConnected( area1, area2 ) ) {
		return false;      // a door blocks sight
	}
	return true;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
bool SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 )
{
	ClipModel& clipModel = TheClipModel::get();
	int leafnum = CM_PointLeafnum( p1 );
	int cluster = clipModel.leafCluster( leafnum );
	int area1 = clipModel.leafArea( leafnum );
	uint8_t* mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = clipModel.leafCluster( leafnum );
	int area2 = clipModel.leafArea( leafnum );

	if ( mask && ( !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) ) {
		return false;
	}

	return true;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState( sharedEntity_t *ent, bool open )
{
	ServerEntity* svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}


/*
==================
SV_GameAreaEntities
==================
*/
bool    SV_EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, const int capsule )
{
	// check for exact collision
	const float* origin = gEnt->r.currentOrigin;
	const float* angles = gEnt->r.currentAngles;

	clipHandle_t ch = SV_ClipHandleForEntity( gEnt );
	trace_t trace;
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs,
							ch, -1, origin, angles, capsule );

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize )
{
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
        return; // keep the linter happy, ERR_DROP does not return
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t,
						PlayerState *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd( int clientNum, UserCmd *cmd )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
        return; // keep the linter happy, ERR_DROP does not return
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs( void )
{
	G_ShutdownGame(false);
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM( bool restart )
{
	// start the entity parsing at the beginning
	sv.entityParsePoint = TheClipModel::get().entityString;

	// use the current msec count for a random seed
	// init for this gamestate
	G_InitGame( svs.time, Com_Milliseconds(), restart );

	// clear all gentity pointers that might still be set from
	// a previous level
	for (int i = 0 ; i < sv_maxclients->integer ; i++ ) {
		svs.clients[i].gentity = nullptr;
	}
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs()
{
	G_ShutdownGame(true);
	SV_InitGameVM( true );
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs()
{
	//FIXME these are temp while I make bots run in vm
	extern int bot_enable;

	cvar_t *var = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	if ( var ) {
		bot_enable = var->integer;
	} else {
		bot_enable = 0;
	}

	SV_InitGameVM( false );
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
bool SV_GameCommand()
{
	if ( sv.state != SS_GAME ) {
		return false;
	}
	return ConsoleCommand();
}
