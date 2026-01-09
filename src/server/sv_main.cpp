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
#include "../game/g_local.h"
#include "../game/g_func_decs.h"

serverStatic_t svs;                 // persistant server info
server_t sv;                        // local server

cvar_t  *sv_fps;                // time rate for running non-clients
cvar_t  *sv_timeout;            // seconds without any message
cvar_t  *sv_zombietime;         // seconds to sink messages after disconnect
cvar_t  *sv_rconPassword;       // password for remote server commands
cvar_t  *sv_privatePassword;    // password for the privateClient slots
cvar_t  *sv_allowDownload;
cvar_t  *sv_maxclients;
cvar_t  *sv_privateClients;     // number of clients reserved for password
cvar_t  *sv_hostname;
cvar_t  *sv_master[MAX_MASTER_SERVERS];     // master server ip address
cvar_t  *sv_reconnectlimit;     // minimum seconds between connect messages
cvar_t  *sv_showloss;           // report when usercmds are lost
cvar_t  *sv_padPackets;         // add nop bytes to messages
cvar_t  *sv_killserver;         // menu system can set to 1 to shut server down
cvar_t  *sv_mapname;

cvar_t  *sv_serverid;
cvar_t  *sv_maxRate;
cvar_t  *sv_minPing;
cvar_t  *sv_maxPing;

cvar_t  *sv_floodProtect;
cvar_t  *sv_allowAnonymous;

cvar_t  *sv_gameskill;
cvar_t  *sv_reloading;

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
======================
SV_AddServerCommand

The given command will be transmitted to the client, and is guaranteed to
not have future snapshot_t executed before it is executed
======================
*/
void SV_AddServerCommand( client_t *client, const char *cmd )
{
	client->reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SV_DropClient()
	// doesn't cause a recursive drop client
	if ( client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1 ) {
		Com_Printf( "===== pending server commands =====\n" );
		for (int i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
			Com_Printf( "cmd %5d: %s\n", i, SV_GetReliableCommand( client, i & ( MAX_RELIABLE_COMMANDS - 1 ) ) );
		}
		SV_DropClient( client, "Server command overflow" );
		return;
	}
	int index = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	SV_AddReliableCommand( client, index, cmd );
}


/*
=================
SV_SendServerCommand

Sends a reliable command string to be interpreted by
the client game module: "cp", "print", "chat", etc
A nullptr client will broadcast to all clients
=================
*/
void  SV_SendServerCommand( client_t *cl, const char *fmt, ... )
{
	va_list argptr;
	uint8_t message[MAX_MSGLEN];
	
	va_start( argptr,fmt );
	vsnprintf( (char *)message, MAX_MSGLEN, fmt,argptr );
	va_end( argptr );

	if ( cl != nullptr ) {
		SV_AddServerCommand( cl, (char *)message );
		return;
	}

	// send the data to all relevent clients
	for (int j = 0; j < sv_maxclients->integer; j++ ) {
		client_t *client = &svs.clients[j];
		if ( client->state < CS_PRIMED ) {
			continue;
		}
		// Ridah, don't need to send messages to AI
		if ( client->gentity && client->gentity->r.svFlags & SVF_CASTAI ) {
			continue;
		}
		// done.
		SV_AddServerCommand( client, (char *)message );
	}
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status( netadr_t from ) {
	
	return;
}

/*
================
SVC_Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info( netadr_t from ) {
	
	return;
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket( netadr_t from, msg_t *msg )
{
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );        // skip the -1 marker

	const char* s = MSG_ReadStringLine( msg );

	Cmd_TokenizeString( s );

	const char* c = Cmd_Argv( 0 );
	Com_DPrintf( "SV packet %s : %s\n", NET_AdrToString( from ), c );

	if ( !Q_stricmp( c,"connect" ) ) {
		SV_DirectConnect( from );
	} else if ( !Q_stricmp( c,"disconnect" ) ) {
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	} else {
		Com_DPrintf( "bad connectionless packet from %s:\n%s\n"
					 , NET_AdrToString( from ), s );
	}
}


//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent( netadr_t from, msg_t *msg )
{
	// check for connectionless packet (0xffffffff) first
	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		SV_ConnectionlessPacket( from, msg );
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	MSG_BeginReadingOOB( msg );
	MSG_ReadLong( msg );                // sequence number
	int qport = MSG_ReadShort( msg ) & 0xffff;

	// find which client the message is from
	for (int i = 0; i < sv_maxclients->integer; i++ ) {
		client_t *cl = &svs.clients[i];
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( !NET_CompareBaseAdr( from, cl->netchan.remoteAddress ) ) {
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if ( cl->netchan.qport != qport ) {
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if ( cl->netchan.remoteAddress.port != from.port ) {
			Com_Printf( "SV_ReadPackets: fixing up a translated port\n" );
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if ( SV_Netchan_Process( cl, msg ) ) {
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if ( cl->state != CS_ZOMBIE ) {
				cl->lastPacketTime = svs.time;  // don't timeout
				SV_ExecuteClientMessage( cl, msg );
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't reckognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint( NS_SERVER, from, "disconnect" );
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->integer
seconds, drop the conneciton.  Server time is used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts()
{
	int droppoint = svs.time - 1000 * sv_timeout->integer;
	int zombiepoint = svs.time - 1000 * sv_zombietime->integer;

	for (int i = 0; i < sv_maxclients->integer; i++ ) {
		client_t *cl = &svs.clients[i];
		// message times may be wrong across a changelevel
		if ( cl->lastPacketTime > svs.time ) {
			cl->lastPacketTime = svs.time;
		}

		if ( cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint ) {
			Com_DPrintf( "Going from CS_ZOMBIE to CS_FREE for %s\n", cl->name );
			cl->state = CS_FREE;    // can now be reused
			continue;
		}
		// Ridah, AI's don't time out
		if ( cl->gentity && !( cl->gentity->r.svFlags & SVF_CASTAI ) ) {
			if ( cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint ) {
				// wait several frames so a debugger session doesn't
				// cause a timeout
				if ( ++cl->timeoutCount > 5 ) {
					SV_DropClient( cl, "timed out" );
					cl->state = CS_FREE;    // don't bother with zombie state
				}
			} else {
				cl->timeoutCount = 0;
			}
		}
	}
}


/*
==================
SV_CheckPaused
==================
*/
bool SV_CheckPaused()
{
	if ( !cl_paused->integer ) {
		return false;
	}

	// only pause if there is just a single client connected
	int count = 0;
	for (int i = 0; i < sv_maxclients->integer; i++ ) {
		client_t *cl = &svs.clients[i];
		if ( cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT ) {
			count++;
		}
	}

	if ( count > 1 ) {
		// don't pause
		sv_paused->integer = 0;
		return false;
	}

	sv_paused->integer = 1;
	return true;
}

/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame( int msec )
{
	// the menu kills the server with this cvar
	if ( sv_killserver->integer ) {
		SV_Shutdown( "Server was killed.\n" );
		Cvar_Set( "sv_killserver", "0" );
		return;
	}

	if ( !com_sv_running->integer ) {
		return;
	}

	// allow pause if only the local client is connected
	if ( SV_CheckPaused() ) {
		return;
	}

	// if it isn't time for the next frame, do nothing
	if ( sv_fps->integer < 1 ) {
		Cvar_Set( "sv_fps", "10" );
	}
	int frameMsec = 1000 / sv_fps->integer ;

	sv.timeResidual += msec;


	SV_BotFrame( svs.time + sv.timeResidual );

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if ( svs.time > 0x70000000 ) {
		SV_Shutdown( "Restarting server due to time wrapping" );
		Cbuf_AddText( "vstr nextmap\n" );
		return;
	}
	
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if ( svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities ) {
		SV_Shutdown( "Restarting server due to numSnapshotEntities wrapping" );
		Cbuf_AddText( "vstr nextmap\n" );
		return;
	}

	if ( sv.restartTime && svs.time >= sv.restartTime ) {
		sv.restartTime = 0;
		Cbuf_AddText( "map_restart 0\n" );
		return;
	}

	// update infostrings if anything has been changed
	if ( cvar_modifiedFlags & CVAR_SERVERINFO ) {
		SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if ( cvar_modifiedFlags & CVAR_SYSTEMINFO ) {
		SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString_Big( CVAR_SYSTEMINFO ) );
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}
	
	int startTime = 0;
	if ( com_speeds->integer ) {
		startTime = Sys_Milliseconds();
	}

	// run the game simulation in chunks
	while ( sv.timeResidual >= frameMsec ) {
		sv.timeResidual -= frameMsec;
		svs.time += frameMsec;

		// let everything in the world think and move
		G_RunFrame( svs.time );
	}

	if ( com_speeds->integer ) {
		time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SV_CheckTimeouts();

	// send messages back to the clients
	SV_SendClientMessages();
}
