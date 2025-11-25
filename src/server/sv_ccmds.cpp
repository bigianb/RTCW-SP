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
==================
SV_Map_f

Restart the server on a different map
==================
*/
static void SV_Map_f( void ) {
	const char        *cmd;

	char smapname[MAX_QPATH];
	char mapname[MAX_QPATH];
	qboolean killBots, cheat, buildScript;
	char expanded[MAX_QPATH];
	int savegameTime = -1;

	const char* map = Cmd_Argv( 1 );
	if ( !map ) {
		return;
	}

	buildScript = Cvar_VariableIntegerValue( "com_buildScript" ) != 0 ? qtrue : qfalse;

	if ( !buildScript && sv_reloading->integer && sv_reloading->integer != RELOAD_NEXTMAP ) {  // game is in 'reload' mode, don't allow starting new maps yet.
		return;
	}

	// Ridah: trap a savegame load
	if ( strstr( map, ".svg" ) ) {
		// open the savegame, read the mapname, and copy it to the map string
		char savemap[MAX_QPATH];
		byte *buffer;

		if ( !( strstr( map, "save/" ) == map ) ) {
			snprintf( savemap, sizeof( savemap ), "save/%s", map );
		} else {
			strcpy( savemap, map );
		}

		size_t size = FS_ReadFile( savemap, NULL );
		if ( size < 0 ) {
			Com_Printf( "Can't find savegame %s\n", savemap );
			return;
		}

		FS_ReadFile( savemap, (void **)&buffer );

		if ( Q_stricmp( savemap, "save/current.svg" ) != 0 ) {
			// copy it to the current savegame file
			FS_WriteFile( "save/current.svg", buffer, size );
			// make sure it is the correct size
			size_t csize = FS_ReadFile( "save/current.svg", NULL );
			if ( csize != size ) {
				Hunk_FreeTempMemory( buffer );
				FS_Delete( "save/current.svg" );
				Com_Error( ERR_DROP, "Insufficient free disk space.\n\nPlease free at least 5mb of free space on game drive." );
                return; // keep the linter happy, ERR_DROP does not return
			}
		}

		// set the cvar, so the game knows it needs to load the savegame once the clients have connected
		Cvar_Set( "savegame_loading", "1" );
		// set the filename
		Cvar_Set( "savegame_filename", savemap );

		// the mapname is at the very start of the savegame file
		snprintf( savemap, sizeof( savemap ), ( char * )( buffer + sizeof( int ) ) );  // skip the version
		Q_strncpyz( smapname, savemap, sizeof( smapname ) );
		map = smapname;

		savegameTime = *( int * )( buffer + sizeof( int ) + MAX_QPATH );

		if ( savegameTime >= 0 ) {
			svs.time = savegameTime;
		}

		Hunk_FreeTempMemory( buffer );
	} else {
		Cvar_Set( "savegame_loading", "0" );  // make sure it's turned off
		// set the filename
		Cvar_Set( "savegame_filename", "" );
	}
	// done.

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	snprintf( expanded, sizeof( expanded ), "maps/%s.bsp", map );
	if ( FS_ReadFile( expanded, NULL ) == -1 ) {
		Com_Printf( "Can't find map %s\n", expanded );
		return;
	}

	Cvar_Set( "r_mapFogColor", "0" );
	Cvar_Set( "r_waterFogColor", "0" );
	Cvar_Set( "r_savegameFogColor", "0" );

	// Rafael gameskill
	Cvar_Get( "g_gameskill", "1", CVAR_SERVERINFO | CVAR_LATCH );
	// done

	Cvar_SetValue( "g_episode", 0 );

	cmd = Cmd_Argv( 0 );
	//Cvar_SetValue( "g_gametype", GT_SINGLE_PLAYER );
	Cvar_SetValue( "g_doWarmup", 0 );
	// may not set sv_maxclients directly, always set latched
	Cvar_SetLatched( "sv_maxclients", "32" ); // Ridah, modified this
	cmd += 2;
	killBots = qtrue;
	if ( !Q_stricmp( cmd, "devmap" ) ) {
		cheat = qtrue;
	} else {
		cheat = qfalse;
	}

	// save the map name here cause on a map restart we reload the q3config.cfg
	// and thus nuke the arguments of the map command
	Q_strncpyz( mapname, map, sizeof( mapname ) );

	// start up the map
	SV_SpawnServer( mapname, killBots );

	// set the cheat value
	// if the level was started with "map <levelname>", then
	// cheats will not be allowed.  If started with "devmap <levelname>"
	// then cheats will be allowed
	if ( cheat ) {
		Cvar_Set( "sv_cheats", "1" );
	} else {
		Cvar_Set( "sv_cheats", "0" );
	}

}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f( void ) {
	int i;
	client_t    *client;
	char        *denied;
	qboolean isBot;
	int delay;

	// make sure we aren't restarting twice in the same frame
	if ( com_frameTime == sv.serverId ) {
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer ) {
		Com_Printf( "Server is not running.\n" );
		return;
	}

	if ( sv.restartTime ) {
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		delay = atoi( Cmd_Argv( 1 ) );
	} else {
		delay = 0;
	}
	if ( delay && !Cvar_VariableValue( "g_doWarmup" ) ) {
		sv.restartTime = svs.time + delay * 1000;
		SV_SetConfigstring( CS_WARMUP, va( "%i", sv.restartTime ) );
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if ( sv_maxclients->modified) {
		char mapname[MAX_QPATH];

		Com_Printf( "variable change -- restarting.\n" );
		// restart the map the slow way
		Q_strncpyz( mapname, Cvar_VariableString( "mapname" ), sizeof( mapname ) );

		SV_SpawnServer( mapname, qfalse );
		return;
	}

	// Ridah, check for loading a saved game
	if ( Cvar_VariableIntegerValue( "savegame_loading" ) ) {
		// open the current savegame, and find out what the time is, everything else we can ignore
		const char *savemap = "save/current.svg";
		byte *buffer;
		int size, savegameTime;

		size = FS_ReadFile( savemap, NULL );
		if ( size < 0 ) {
			Com_Printf( "Can't find savegame %s\n", savemap );
			return;
		}

		//buffer = Hunk_AllocateTempMemory(size);
		FS_ReadFile( savemap, (void **)&buffer );

		// the mapname is at the very start of the savegame file
		savegameTime = *( int * )( buffer + sizeof( int ) + MAX_QPATH );

		if ( savegameTime >= 0 ) {
			svs.time = savegameTime;
		}

		Hunk_FreeTempMemory( buffer );
	}
	// done.

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	sv.restartedServerId = sv.serverId;
	sv.serverId = com_frameTime;
	Cvar_Set( "sv_serverid", va( "%i", sv.serverId ) );

	// reset all the vm data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.restarting = qtrue;

	SV_RestartGameProgs();

	// run a few frames to allow everything to settle
	for ( i = 0 ; i < 3 ; i++ ) {
		G_RunFrame( svs.time );
		svs.time += 100;
	}

	sv.state = SS_GAME;
	sv.restarting = qfalse;

	// connect and begin all the clients
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		client = &svs.clients[i];

		// send the new gamestate to all connected clients
		if ( client->state < CS_CONNECTED ) {
			continue;
		}

		if ( client->netchan.remoteAddress.type == NA_BOT ) {
			isBot = qtrue;
		} else {
			isBot = qfalse;
		}

		// add the map_restart command
		SV_AddServerCommand( client, "map_restart\n" );

		// connect the client again, without the firstTime flag
		denied = ClientConnect( i, qfalse, isBot );
		if ( denied ) {
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient( client, denied );
			Com_Printf( "SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i ); // bk010125
			continue;
		}

		client->state = CS_ACTIVE;

		SV_ClientEnterWorld( client, &client->lastUsercmd );
	}

	// run another frame to allow things to look at all the players
	G_RunFrame( svs.time );
	svs.time += 100;
}

/*
=================
SV_LoadGame_f
=================
*/
void    SV_LoadGame_f( void ) {
	char filename[MAX_QPATH], mapname[MAX_QPATH];
	byte *buffer;
	int size;

	// dont allow command if another loadgame is pending
	if ( Cvar_VariableIntegerValue( "savegame_loading" ) ) {
		return;
	}
	if ( sv_reloading->integer ) {
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	if ( !filename[0] ) {
		Com_Printf( "You must specify a savegame to load\n" );
		return;
	}
	if ( Q_strncmp( filename, "save/", 5 ) && Q_strncmp( filename, "save\\", 5 ) ) {
		Q_strncpyz( filename, va( "save/%s", filename ), sizeof( filename ) );
	}
	// enforce .svg extension
	if ( !strstr( filename, "." ) || Q_strncmp( strstr( filename, "." ) + 1, "svg", 3 ) ) {
		Q_strcat( filename, sizeof( filename ), ".svg" );
	}
	// use '/' instead of '\' for directories
	while ( strstr( filename, "\\" ) ) {
		*(char *)strstr( filename, "\\" ) = '/';
	}

	size = FS_ReadFile( filename, NULL );
	if ( size < 0 ) {
		Com_Printf( "Can't find savegame %s\n", filename );
		return;
	}

	//buffer = Hunk_AllocateTempMemory(size);
	FS_ReadFile( filename, (void **)&buffer );

	// read the mapname, if it is the same as the current map, then do a fast load
	snprintf( mapname, sizeof( mapname ), (const char*)( buffer + sizeof( int ) ) );

	if ( com_sv_running->integer && ( com_frameTime != sv.serverId ) ) {
		// check mapname
		if ( !Q_stricmp( mapname, sv_mapname->string ) ) {    // same

			if ( Q_stricmp( filename, "save/current.svg" ) != 0 ) {
				// copy it to the current savegame file
				FS_WriteFile( "save/current.svg", buffer, size );
			}

			Hunk_FreeTempMemory( buffer );

			Cvar_Set( "savegame_loading", "2" );  // 2 means it's a restart, so stop rendering until we are loaded
			// set the filename
			Cvar_Set( "savegame_filename", filename );
			// quick-restart the server
			SV_MapRestart_f();  // savegame will be loaded after restart

			return;
		}
	}

	Hunk_FreeTempMemory( buffer );

	// otherwise, do a slow load
	if ( Cvar_VariableIntegerValue( "sv_cheats" ) ) {
		Cbuf_ExecuteText( EXEC_APPEND, va( "spdevmap %s", filename ) );
	} else {    // no cheats
		Cbuf_ExecuteText( EXEC_APPEND, va( "spmap %s", filename ) );
	}
}

//===============================================================

/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f( void ) {
	svs.nextHeartbeatTime = -9999999;
}


/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f( void ) {
	Com_Printf( "Server info settings:\n" );
	Info_Print( Cvar_InfoString( CVAR_SERVERINFO ) );
}


/*
===========
SV_Systeminfo_f

Examine or change the serverinfo string
===========
*/
static void SV_Systeminfo_f( void ) {
	Com_Printf( "System info settings:\n" );
	Info_Print( Cvar_InfoString( CVAR_SYSTEMINFO ) );
}

/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f( void ) {
	SV_Shutdown( "killserver" );
}

//===========================================================

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands( void ) {
	static qboolean initialized;

	if ( initialized ) {
		return;
	}
	initialized = qtrue;

	Cmd_AddCommand( "heartbeat", SV_Heartbeat_f );

	Cmd_AddCommand( "serverinfo", SV_Serverinfo_f );
	Cmd_AddCommand( "systeminfo", SV_Systeminfo_f );

	Cmd_AddCommand( "map_restart", SV_MapRestart_f );
	Cmd_AddCommand( "sectorlist", SV_SectorList_f );
	Cmd_AddCommand( "spmap", SV_Map_f );

	Cmd_AddCommand( "map", SV_Map_f );
	Cmd_AddCommand( "devmap", SV_Map_f );
	Cmd_AddCommand( "spdevmap", SV_Map_f );

	Cmd_AddCommand( "loadgame", SV_LoadGame_f );
	Cmd_AddCommand( "killserver", SV_KillServer_f );

}


