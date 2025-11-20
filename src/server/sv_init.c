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

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring( int index, const char *val )
{
	int maxChunkSize = MAX_STRING_CHARS - 24;
	
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "SV_SetConfigstring: bad index %i\n", index );
        return; // keep the linter happy, ERR_DROP does not return
	}

	if ( !val ) {
		val = "";
	}

	// don't bother broadcasting an update if no change
	if ( !strcmp( val, sv.configstrings[ index ] ) ) {
		return;
	}

	// change the string in sv
	free( sv.configstrings[index] );
	sv.configstrings[index] = CopyString( val );

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevent clients
        client_t    *client = svs.clients;
		for (int i = 0; i < sv_maxclients->integer ; i++, client++ ) {
			if ( client->state < CS_PRIMED ) {
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && ( client->gentity->r.svFlags & SVF_NOSERVERINFO ) ) {
				continue;
			}

			// RF, don't send to bot/AI
			if ( client->gentity && ( client->gentity->r.svFlags & SVF_CASTAI ) ) {
				continue;
			}

			size_t len = strlen( val );
			if ( len >= maxChunkSize ) {
				int sent = 0;
				size_t remaining = len;
				char    *cmd;
				char buf[MAX_STRING_CHARS];

				while ( remaining > 0 ) {
					if ( sent == 0 ) {
						cmd = "bcs0";
					} else if ( remaining < maxChunkSize )    {
						cmd = "bcs2";
					} else {
						cmd = "bcs1";
					}
					Q_strncpyz( buf, &val[sent], maxChunkSize );

					SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd, index, buf );

					sent += ( maxChunkSize - 1 );
					remaining -= ( maxChunkSize - 1 );
				}
			} else {
				// standard cs, just send it
				SV_SendServerCommand( client, "cs %i \"%s\"\n", index, val );
			}
		}
	}
}



/*
===============
SV_GetConfigstring

===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize )
{
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
        return; // keep the linter happy, ERR_DROP does not return
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bad index %i\n", index );
        return; // keep the linter happy, ERR_DROP does not return
	}
	if ( !sv.configstrings[index] ) {
		buffer[0] = 0;
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index], bufferSize );
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val )
{
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_SetUserinfo: bad index %i\n", index );
        return; // keep the linter happy, ERR_DROP does not return
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof( svs.clients[index].name ) );
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize )
{
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
        return; // keep the linter happy, ERR_DROP does not return
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bad index %i\n", index );
        return; // keep the linter happy, ERR_DROP does not return
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress non-delta messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline()
{
	for (int entnum = 1; entnum < sv.num_entities ; entnum++ ) {
		sharedEntity_t* svent = SV_GentityNum( entnum );
		if ( !svent->r.linked ) {
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		sv.svEntities[entnum].baseline = svent->s;
	}
}


/*
===============
SV_BoundMaxClients

===============
*/
void SV_BoundMaxClients( int minimum )
{
	// get the current maxclients value
	Cvar_Get( "sv_maxclients", "8", 0 );

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set( "sv_maxclients", va( "%i", minimum ) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set( "sv_maxclients", va( "%i", MAX_CLIENTS ) );
	}
}

/*
===============
SV_InitReliableCommandsForClient
===============
*/
void SV_InitReliableCommandsForClient( client_t *cl, int commands )
{
	if ( !commands ) {
		Com_Memset( &cl->reliableCommands, 0, sizeof( cl->reliableCommands ) );
	}
	//
	cl->reliableCommands.bufSize = commands * RELIABLE_COMMANDS_CHARS;
	cl->reliableCommands.buf = calloc(1, cl->reliableCommands.bufSize );
	cl->reliableCommands.commandLengths = calloc(1, commands * sizeof( *cl->reliableCommands.commandLengths ) );
	cl->reliableCommands.commands = calloc(1, commands * sizeof( *cl->reliableCommands.commands ) );
	//
	cl->reliableCommands.rover = cl->reliableCommands.buf;
}

/*
===============
SV_InitReliableCommands
===============
*/
void SV_InitReliableCommands( client_t *clients )
{
    for (int i = 0; i < sv_maxclients->integer; i++ ) {
        SV_InitReliableCommandsForClient( &clients[i], MAX_RELIABLE_COMMANDS );
    }
}

/*
===============
SV_FreeReliableCommandsForClient
===============
*/
void SV_FreeReliableCommandsForClient( client_t *cl )
{
	if ( !cl->reliableCommands.bufSize ) {
		return;
	}
	free( cl->reliableCommands.buf );
	free( cl->reliableCommands.commandLengths );
	free( cl->reliableCommands.commands );
	//
	Com_Memset( &cl->reliableCommands, 0, sizeof( cl->reliableCommands.bufSize ) );
}

/*
===============
SV_GetReliableCommand
===============
*/
const char *SV_GetReliableCommand( client_t *cl, int index )
{
	if ( !cl->reliableCommands.bufSize ) {
		return "";
	}

	if ( !cl->reliableCommands.commandLengths[index] ) {
		return "";
	}

	return cl->reliableCommands.commands[index];
}

/*
===============
SV_AddReliableCommand
===============
*/
qboolean SV_AddReliableCommand( client_t *cl, int index, const char *cmd )
{
	size_t i, j;
	char    *ch, *ch2;
	//
	if ( !cl->reliableCommands.bufSize ) {
		return qfalse;
	}
	//
	size_t length = strlen( cmd );
	//
	if ( ( cl->reliableCommands.rover - cl->reliableCommands.buf ) + length + 1 >= cl->reliableCommands.bufSize ) {
		// go back to the start
		cl->reliableCommands.rover = cl->reliableCommands.buf;
	}
	//
	// make sure this position won't overwrite another command
	for ( i = length, ch = cl->reliableCommands.rover; i && !*ch; i--, ch++ ) {
		// keep going until we find a bad character, or enough space is found
	}
	// if the test failed
	if ( i ) {
		// find a valid spot to place the new string
		// start at the beginning (keep it simple)
		for ( i = 0, ch = cl->reliableCommands.buf; i < cl->reliableCommands.bufSize; i++, ch++ ) {
			if ( !*ch && ( !i || !*( ch - 1 ) ) ) { // make sure we dont start at the terminator of another string
				// see if this is the start of a valid segment
				for ( ch2 = ch, j = 0; i < cl->reliableCommands.bufSize - 1 && j < length + 1 && !*ch2; i++, ch2++, j++ ) {
					// loop
				}
				//
				if ( j == length + 1 ) {
					// valid segment found
					cl->reliableCommands.rover = ch;
					break;
				}
				//
				if ( i == cl->reliableCommands.bufSize - 1 ) {
					// ran out of room, not enough space for string
					return qfalse;
				}
				//
				ch = &cl->reliableCommands.buf[i];  // continue where ch2 left off
			}
		}
	}
	//
	// insert the command at the rover
	cl->reliableCommands.commands[index] = cl->reliableCommands.rover;
	Q_strncpyz( cl->reliableCommands.commands[index], cmd, length + 1 );
	cl->reliableCommands.commandLengths[index] = length;
	//
	// move the rover along
	cl->reliableCommands.rover += length + 1;
	//
	return qtrue;
}

/*
===============
SV_FreeAcknowledgedReliableCommands
===============
*/
void SV_FreeAcknowledgedReliableCommands( client_t *cl )
{
	if ( !cl->reliableCommands.bufSize ) {
		return;
	}
	//
	int realAck = ( cl->reliableAcknowledge ) & ( MAX_RELIABLE_COMMANDS - 1 );
	// move backwards one command, since we need the most recently acknowledged
	// command for netchan decoding
	int ack = ( cl->reliableAcknowledge - 1 ) & ( MAX_RELIABLE_COMMANDS - 1 );
	//
	if ( !cl->reliableCommands.commands[ack] ) {
		return; // no new commands acknowledged
	}
	//
	while ( cl->reliableCommands.commands[ack] ) {
		// clear the string
		memset( cl->reliableCommands.commands[ack], 0, cl->reliableCommands.commandLengths[ack] );
		// clear the pointer
		cl->reliableCommands.commands[ack] = NULL;
		cl->reliableCommands.commandLengths[ack] = 0;
		// move the the previous command
		ack--;
		if ( ack < 0 ) {
			ack = ( MAX_RELIABLE_COMMANDS - 1 );
		}
		if ( ack == realAck ) {
			// never free the actual most recently acknowledged command
			break;
		}
	}
}

/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
void SV_Startup()
{
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

	svs.clients = calloc( sizeof( client_t ) * sv_maxclients->integer, 1 );
	if ( !svs.clients ) {
		Com_Error( ERR_FATAL, "SV_Startup: unable to allocate svs.clients" );
	}


	svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
	
	svs.initialized = qtrue;

	Cvar_Set( "sv_running", "1" );
}


/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients()
{
	// get the highest client number in use
	int count = 0;
	for (int i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if ( i > count ) {
				count = i;
			}
		}
	}
	count++;

	int oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	// RF, free reliable commands for clients outside the NEW maxclients limit
	if ( oldMaxClients > sv_maxclients->integer ) {
		for (int i = sv_maxclients->integer ; i < oldMaxClients ; i++ ) {
			SV_FreeReliableCommandsForClient( &svs.clients[i] );
		}
	}

	client_t* oldClients = Hunk_AllocateTempMemory( count * sizeof( client_t ) );
	// copy the clients to hunk memory
	for (int i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		} else {
			Com_Memset( &oldClients[i], 0, sizeof( client_t ) );
		}
	}

	// free old clients arrays

	free( svs.clients );    // RF, avoid trying to allocate large chunk on a fragmented zone


	// allocate new clients

	// RF, avoid trying to allocate large chunk on a fragmented zone
	svs.clients = calloc( sizeof( client_t ) * sv_maxclients->integer, 1 );
	if ( !svs.clients ) {
		Com_Error( ERR_FATAL, "SV_Startup: unable to allocate svs.clients" );
	}


	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof( client_t ) );

	// copy the clients over
	for (int i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Hunk_FreeTempMemory( oldClients );

	// allocate new snapshot entities
	svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;

	// RF, allocate reliable commands for newly created client slots
	if ( oldMaxClients < sv_maxclients->integer ) {
		for (int i = oldMaxClients ; i < sv_maxclients->integer ; i++ ) {
			// must be an AI slot
			SV_InitReliableCommandsForClient( &svs.clients[i], 0 );
		}
	}
}


/*
====================
SV_SetExpectedHunkUsage

  Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void SV_SetExpectedHunkUsage( char *mapname )
{
	int handle;
	int len = FS_FOpenFileByMode( "hunkusage.dat", &handle, FS_READ );
	if ( len >= 0 ) {
		// the file exists, so read it in, strip out the current entry for this map
		// and save it out, so we can append the new value

		char* buf = (char *)calloc(1, len + 1 );
		memset( buf, 0, len + 1 );

		FS_Read( (void *)buf, len, handle );
		FS_FCloseFile( handle );

		// now parse the file, filtering out the current map
		char* buftrav = buf;
		char* token;
		while ( ( token = COM_Parse( &buftrav ) ) && token[0] ) {
			if ( !Q_strcasecmp( token, mapname ) ) {
				// found a match
				token = COM_Parse( &buftrav );  // read the size
				if ( token && token[0] ) {
					// this is the usage
					Cvar_Set( "com_expectedhunkusage", token );
					free( buf );
					return;
				}
			}
		}

		free( buf );
	}
	// just set it to a negative number,so the cgame knows not to draw the percent bar
	Cvar_Set( "com_expectedhunkusage", "-1" );
}

/*
================
SV_ClearServer
================
*/
void SV_ClearServer()
{
	for (int i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			free( sv.configstrings[i] );
		}
	}
	Com_Memset( &sv, 0, sizeof( sv ) );
}

extern int Export_BotLibShutdown( void );
/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( char *server, qboolean killBots )
{
	static cvar_t   *bot_enable;

	// Rafael gameskill
	static cvar_t   *g_gameskill;

	if ( !g_gameskill ) {
		g_gameskill = Cvar_Get( "g_gameskill", "2", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE );     // (SA) new default '2' (was '1')
	}
	// done

	if ( !bot_enable ) {
		bot_enable = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	}
	
	if ( sv_maxclients->latchedString ) {
		// it's been modified, so grab the new value
		Cvar_Get( "sv_maxclients", "8", 0 );
	}
	if ( sv_maxclients->integer < MAX_CLIENTS ) {
		Cvar_SetValue( "sv_maxclients", MAX_SP_CLIENTS );
	}
	if ( !bot_enable->integer ) {
		Cvar_Set( "bot_enable", "1" );
	}
	
	// done.

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf( "------ Server Initialization ------\n" );
	Com_Printf( "Server: %s\n",server );

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

	// make sure all the client stuff is unloaded
	CL_ShutdownAll();

	// clear the whole hunk because we're (re)loading the server
	// IJB: shared hunk so need to free the AAS first.
	Export_BotLibShutdown();

	CM_ClearMap();

	// init client structures and svs.numSnapshotEntities
	if ( !Cvar_VariableValue( "sv_running" ) ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	// clear pak references
	FS_ClearPakReferences( 0 );

	// allocate the snapshot entities on the hunk
	svs.snapshotEntities = Hunk_Alloc( sizeof( entityState_t ) * svs.numSnapshotEntities, h_high );
	svs.nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// set nextmap to the same map, but it may be overriden
	// by the game startup or another console command
	Cvar_Set( "nextmap", "map_restart 0" );

	// wipe the entire per-level structure
	SV_ClearServer();

	// allocate empty config strings
	for (int i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyString( "" );
	}

	SV_SetExpectedHunkUsage( va( "maps/%s.bsp", server ) );

	// make sure we are not paused
	Cvar_Set( "cl_paused", "0" );

	// get a new checksum feed and restart the file system
	srand( Sys_Milliseconds() );
	sv.checksumFeed = ( ( (int) rand() << 16 ) ^ rand() ) ^ Sys_Milliseconds();
	FS_Restart( sv.checksumFeed );

	int checksum;
	CM_LoadMap( va( "maps/%s.bsp", server ), qfalse, &checksum );

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va( "%i",checksum ) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId;
	Cvar_Set( "sv_serverid", va( "%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// run a few frames to allow everything to settle
	for (int i = 0 ; i < 3 ; i++ ) {
		G_RunFrame( svs.time );
		SV_BotFrame( svs.time );
		svs.time += 100;
	}

	// create a baseline for more efficient communications
	SV_CreateBaseline();

	qboolean isBot = qfalse;	// IJB: what should the default be?
	for (int i = 0 ; i < sv_maxclients->integer ; i++ ) {
		// send the new gamestate to all connected clients
		if ( svs.clients[i].state >= CS_CONNECTED ) {

			if ( svs.clients[i].netchan.remoteAddress.type == NA_BOT ) {
				 
				SV_DropClient( &svs.clients[i], " gametype is Single Player" );      //DAJ added message
				continue;
	
			} else {
				isBot = qfalse;
			}

			// connect the client again
			const char* denied = ClientConnect( i, qfalse, isBot ); // firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				if ( !isBot ) {
					// when we get the next packet from a connected client,
					// the new gamestate will be sent
					svs.clients[i].state = CS_CONNECTED;
				} else {
					client_t        *client;
					sharedEntity_t  *ent;

					client = &svs.clients[i];
					client->state = CS_ACTIVE;
					ent = SV_GentityNum( i );
					ent->s.number = i;
					client->gentity = ent;

					client->deltaMessage = -1;
					client->nextSnapshotTime = svs.time;    // generate a snapshot immediately

					ClientBegin( i );
				}
			}
		}
	}

	// run another frame to allow things to look at all the players
	G_RunFrame( svs.time );
	SV_BotFrame( svs.time );
	svs.time += 100;

	Cvar_Set( "sv_paks", "" );
	Cvar_Set( "sv_pakNames", "" );
	
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	Cvar_Set( "sv_referencedPaks", FS_ReferencedPakChecksums());
	Cvar_Set( "sv_referencedPakNames", FS_ReferencedPakNames());

	// save systeminfo and serverinfo strings
	char systemInfo[MAX_INFO_STRING];
	Q_strncpyz( systemInfo, Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	Com_Printf( "-----------------------------------\n" );
}


/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/

void SV_Init()
{
	SV_AddOperatorCommands();

	// serverinfo vars
	Cvar_Get( "dmflags", "0", CVAR_SERVERINFO );
	Cvar_Get( "fraglimit", "20", CVAR_SERVERINFO );
	Cvar_Get( "timelimit", "0", CVAR_SERVERINFO );

	// Rafael gameskill
	sv_gameskill = Cvar_Get( "g_gameskill", "1", CVAR_SERVERINFO | CVAR_LATCH );
	// done

	Cvar_Get( "sv_keywords", "", CVAR_SERVERINFO );
	Cvar_Get( "protocol", va( "%i", PROTOCOL_VERSION ), CVAR_SERVERINFO | CVAR_ROM );
	sv_mapname = Cvar_Get( "mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM );
	sv_privateClients = Cvar_Get( "sv_privateClients", "0", CVAR_SERVERINFO );
	sv_hostname = Cvar_Get( "sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE );
	sv_maxclients = Cvar_Get( "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH );
	sv_maxRate = Cvar_Get( "sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_minPing = Cvar_Get( "sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get( "sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_floodProtect = Cvar_Get( "sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_allowAnonymous = Cvar_Get( "sv_allowAnonymous", "0", CVAR_SERVERINFO );

	// systeminfo
	Cvar_Get( "sv_cheats", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_serverid = Cvar_Get( "sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );

	Cvar_Get( "sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get( "sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get( "sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get( "sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get( "rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get( "sv_privatePassword", "", CVAR_TEMP );
	sv_fps = Cvar_Get( "sv_fps", "20", CVAR_TEMP );
	sv_timeout = Cvar_Get( "sv_timeout", "120", CVAR_TEMP );
	sv_zombietime = Cvar_Get( "sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get( "nextmap", "", CVAR_TEMP );

	sv_master[0] = Cvar_Get( "sv_master1", "master.gmistudios.com", 0 );
	sv_master[1] = Cvar_Get( "sv_master2", "", CVAR_ARCHIVE );
	sv_master[2] = Cvar_Get( "sv_master3", "", CVAR_ARCHIVE );
	sv_master[3] = Cvar_Get( "sv_master4", "", CVAR_ARCHIVE );
	sv_master[4] = Cvar_Get( "sv_master5", "", CVAR_ARCHIVE );
	sv_reconnectlimit = Cvar_Get( "sv_reconnectlimit", "3", 0 );
	sv_showloss = Cvar_Get( "sv_showloss", "0", 0 );
	sv_padPackets = Cvar_Get( "sv_padPackets", "0", 0 );
	sv_killserver = Cvar_Get( "sv_killserver", "0", 0 );
	sv_mapChecksum = Cvar_Get( "sv_mapChecksum", "", CVAR_ROM );

	sv_reloading = Cvar_Get( "g_reloading", "0", CVAR_ROM ); 

	// initialize bot cvars so they are listed and can be set before loading the botlib
	SV_BotInitCvars();
}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message )
{
	// send it twice, ignoring rate
	for (int j = 0 ; j < 2 ; j++ ) {
		for (int i = 0; i < sv_maxclients->integer ; i++ ) {
			client_t    *cl = &svs.clients[i];
			if ( cl->state >= CS_CONNECTED ) {
				// force a snapshot to be sent
				cl->nextSnapshotTime = -1;
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( char *finalmsg )
{
	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown -----\n" );

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	SV_ShutdownGameProgs();

	// free current level
	SV_ClearServer();

	// free server static data
	if ( svs.clients ) {
		//free( svs.clients );
		free( svs.clients );    // RF, avoid trying to allocate large chunk on a fragmented zone
	}
	memset( &svs, 0, sizeof( svs ) );

	Cvar_Set( "sv_running", "0" );

	Com_Printf( "---------------------------\n" );

	// disconnect any local clients
	CL_Disconnect( qfalse );
}

