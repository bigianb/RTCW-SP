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

// sv_bot.c

#include "server.h"
#include "../botlib/botlib.h"
#include "../botai/botai.h"
#include "../game/g_local.h"
#include "../game/g_func_decs.h"

#define MAX_DEBUGPOLYS      128

typedef struct bot_debugpoly_s
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
} bot_debugpoly_t;

static bot_debugpoly_t debugpolygons[MAX_DEBUGPOLYS];

extern botlib_export_t  *botlib_export;
int bot_enable;

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient()
{
	int i;
	client_t    *cl;

	// find a client slot
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
		if ( i < 1 ) {
			continue;
		}
		// done.
		if ( cl->state == CS_FREE ) {
			break;
		}
	}

	if ( i == sv_maxclients->integer ) {
		return -1;
	}

	cl->gentity = SV_GentityNum( i );
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient( int clientNum )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum );
        return; // keep the linter happy, ERR_DROP does not return
	}
	client_t* cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if ( cl->gentity ) {
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
BotImport_Print
==================
*/
void  BotImport_Print( int type, char *fmt, ... ) {
	char str[2048];
	va_list ap;

	va_start( ap, fmt );
	vsnprintf( str, 2048, fmt, ap );
	va_end( ap );

	switch ( type ) {
	case PRT_MESSAGE: {
		Com_Printf( "%s", str );
		break;
	}
	case PRT_WARNING: {
		Com_Printf( S_COLOR_YELLOW "Warning: %s", str );
		break;
	}
	case PRT_ERROR: {
		Com_Printf( S_COLOR_RED "Error: %s", str );
		break;
	}
	case PRT_FATAL: {
		Com_Printf( S_COLOR_RED "Fatal: %s", str );
		break;
	}
	case PRT_EXIT: {
		Com_Error( ERR_DROP, S_COLOR_RED "Exit: %s", str );
		break;
	}
	default: {
		Com_Printf( "unknown print type\n" );
		break;
	}
	}
}

/*
==================
BotImport_Trace
==================
*/
void BotImport_Trace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask ) {
	trace_t trace;

	// always use bounding box for bot stuff ?
	SV_Trace( &trace, start, mins, maxs, end, passent, contentmask, qfalse );
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy( trace.endpos, bsptrace->endpos );
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy( trace.plane.normal, bsptrace->plane.normal );
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotImport_EntityTrace
==================
*/
void BotImport_EntityTrace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask ) {
	trace_t trace;

	// always use bounding box for bot stuff ?
	SV_ClipToEntity( &trace, start, mins, maxs, end, entnum, contentmask, qfalse );
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy( trace.endpos, bsptrace->endpos );
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy( trace.plane.normal, bsptrace->plane.normal );
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotImport_BSPModelMinsMaxsOrigin
==================
*/
void BotImport_BSPModelMinsMaxsOrigin( int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin ) {
	clipHandle_t h;
	vec3_t mins, maxs;
	float max;
	int i;

	h = CM_InlineModel( modelnum );
	CM_ModelBounds( h, mins, maxs );
	//if the model is rotated
	if ( ( angles[0] || angles[1] || angles[2] ) ) {
		// expand for rotation

		max = RadiusFromBounds( mins, maxs );
		for ( i = 0; i < 3; i++ ) {
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if ( outmins ) {
		VectorCopy( mins, outmins );
	}
	if ( outmaxs ) {
		VectorCopy( maxs, outmaxs );
	}
	if ( origin ) {
		VectorClear( origin );
	}
}

/*
==================
BotImport_DebugPolygonCreate
==================
*/
int BotImport_DebugPolygonCreate( int color, int numPoints, vec3_t *points )
{
	int i;

	for ( i = 1; i < MAX_DEBUGPOLYS; i++ )    {
		if ( !debugpolygons[i].inuse ) {
			break;
		}
	}
	if ( i >= MAX_DEBUGPOLYS ) {
		return 0;
	}
	bot_debugpoly_t* poly = &debugpolygons[i];
	poly->inuse = qtrue;
	poly->color = color;
	poly->numPoints = numPoints;
	memcpy( poly->points, points, numPoints * sizeof( vec3_t ) );
	//
	return i;
}

/*
==================
BotImport_DebugPolygonShow
==================
*/
void BotImport_DebugPolygonShow( int id, int color, int numPoints, vec3_t *points )
{
	bot_debugpoly_t* poly = &debugpolygons[id];
	poly->inuse = qtrue;
	poly->color = color;
	poly->numPoints = numPoints;
	memcpy( poly->points, points, numPoints * sizeof( vec3_t ) );
}

/*
==================
BotImport_DebugPolygonDelete
==================
*/
void BotImport_DebugPolygonDelete( int id ) {
	debugpolygons[id].inuse = qfalse;
}

/*
==================
BotImport_DebugLineCreate
==================
*/
int BotImport_DebugLineCreate( void ) {
	vec3_t points[1];
	return BotImport_DebugPolygonCreate( 0, 0, points );
}

/*
==================
BotImport_DebugLineDelete
==================
*/
void BotImport_DebugLineDelete( int line ) {
	BotImport_DebugPolygonDelete( line );
}

/*
==================
BotImport_DebugLineShow
==================
*/
void BotImport_DebugLineShow( int line, vec3_t start, vec3_t end, int color ) {
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy( start, points[0] );
	VectorCopy( start, points[1] );
	//points[1][2] -= 2;
	VectorCopy( end, points[2] );
	//points[2][2] -= 2;
	VectorCopy( end, points[3] );


	VectorSubtract( end, start, dir );
	VectorNormalize( dir );
	dot = DotProduct( dir, up );
	if ( dot > 0.99 || dot < -0.99 ) {
		VectorSet( cross, 1, 0, 0 );
	} else { CrossProduct( dir, up, cross );}

	VectorNormalize( cross );

	VectorMA( points[0], 2, cross, points[0] );
	VectorMA( points[1], -2, cross, points[1] );
	VectorMA( points[2], -2, cross, points[2] );
	VectorMA( points[3], 2, cross, points[3] );

	BotImport_DebugPolygonShow( line, color, 4, points );
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand( int client, char *command ) {
	SV_ExecuteClientCommand( &svs.clients[client], command, qtrue );
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame( int time ) {
	if ( !bot_enable ) {
		return;
	}
	BotAIStartFrame( time );
}

/*
===============
SV_BotLibSetup
===============
*/

int SV_BotLibSetup( void ) {
	return Export_BotLibSetup();
}

/*
===============
SV_ShutdownBotLib

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
int SV_BotLibShutdown( void ) {

	if ( !botlib_export ) {
		return -1;
	}

	return botlib_export->BotLibShutdown();
}

/*
==================
SV_BotInitCvars
==================
*/
void SV_BotInitCvars( void ) {

	Cvar_Get( "bot_enable", "1", 0 );               //enable the bot
	Cvar_Get( "bot_developer", "0", 0 );            //bot developer mode
	Cvar_Get( "bot_debug", "0", 0 );                //enable bot debugging
	Cvar_Get( "bot_groundonly", "1", 0 );           //only show ground faces of areas
	Cvar_Get( "bot_reachability", "0", 0 );     //show all reachabilities to other areas
	Cvar_Get( "bot_thinktime", "100", 0 );      //msec the bots thinks
	Cvar_Get( "bot_reloadcharacters", "0", 0 ); //reload the bot characters each time
	Cvar_Get( "bot_testichat", "0", 0 );            //test ichats
	Cvar_Get( "bot_testrchat", "0", 0 );            //test rchats
	Cvar_Get( "bot_fastchat", "0", 0 );         //fast chatting bots
	Cvar_Get( "bot_nochat", "0", 0 );               //disable chats
	Cvar_Get( "bot_grapple", "0", 0 );          //enable grapple
	Cvar_Get( "bot_rocketjump", "1", 0 );           //enable rocket jumping
	Cvar_Get( "bot_miniplayers", "0", 0 );      //minimum players in a team or the game
}

// Ridah, Cast AI
/*
===============
BotImport_AICast_VisibleFromPos
===============
*/
qboolean BotImport_AICast_VisibleFromPos(   vec3_t srcpos, int srcnum,
											vec3_t destpos, int destnum, qboolean updateVisPos ) {
	return AICast_VisibleFromPos( srcpos, srcnum, destpos, destnum, updateVisPos );
}

/*
===============
BotImport_AICast_CheckAttackAtPos
===============
*/
qboolean BotImport_AICast_CheckAttackAtPos( int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld ) {
	return AICast_CheckAttackAtPos( entnum, enemy, pos, ducking, allowHitWorld );
}
// done.

//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage( int client, char *buf, int size )
{
	const char        *msg;

	client_t* cl = &svs.clients[client];
	cl->lastPacketTime = svs.time;

	if ( cl->reliableAcknowledge == cl->reliableSequence ) {
		return qfalse;
	}

	cl->reliableAcknowledge++;
	int index = cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 );

	//if ( !cl->reliableCommands[index][0] ) {
	if ( !( msg = SV_GetReliableCommand( cl, index ) ) || !msg[0] ) {
		return qfalse;
	}

	//Q_strncpyz( buf, cl->reliableCommands[index], size );
	Q_strncpyz( buf, msg, size );
	return qtrue;
}


/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity( int client, int sequence )
{
	client_t* cl = &svs.clients[client];
	clientSnapshot_t* frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	if ( sequence < 0 || sequence >= frame->num_entities ) {
		return -1;
	}
	return svs.snapshotEntities[( frame->first_entity + sequence ) % svs.numSnapshotEntities].number;
}
