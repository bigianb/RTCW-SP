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




// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"
#include "../server/server.h"
#include "../qcommon/qcommon.h"

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/





typedef struct ipFilter_s
{
	unsigned mask;
	unsigned compare;
} ipFilter_t;

#define MAX_IPFILTERS   1024

static ipFilter_t ipFilters[MAX_IPFILTERS];
static int numIPFilters;

/*
=================
StringToFilter
=================
*/
static bool StringToFilter( char *s, ipFilter_t *f ) {
	char num[128];
	int i, j;
	uint8_t b[4];
	uint8_t m[4];

	for ( i = 0 ; i < 4 ; i++ )
	{
		b[i] = 0;
		m[i] = 0;
	}

	for ( i = 0 ; i < 4 ; i++ )
	{
		if ( *s < '0' || *s > '9' ) {
			Com_Printf( "Bad filter address: %s\n", s );
			return false;
		}

		j = 0;
		while ( *s >= '0' && *s <= '9' )
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi( num );
		if ( b[i] != 0 ) {
			m[i] = 255;
		}

		if ( !*s ) {
			break;
		}
		s++;
	}

	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return true;
}

/*
===================
Svcmd_EntityList_f
===================
*/
void    Svcmd_EntityList_f( void ) {
	int e;
	GameEntity       *check;

	check = g_entities + 1;
	for ( e = 1; e < level.num_entities ; e++, check++ ) {
		if ( !check->inuse ) {
			continue;
		}
		Com_Printf( "%3i:", e );
		switch ( check->shared.s.eType ) {
		case ET_GENERAL:
			Com_Printf( "ET_GENERAL          " );
			break;
		case ET_PLAYER:
			Com_Printf( "ET_PLAYER           " );
			break;
		case ET_ITEM:
			Com_Printf( "ET_ITEM             " );
			break;
		case ET_MISSILE:
			Com_Printf( "ET_MISSILE          " );
			break;
		case ET_MOVER:
			Com_Printf( "ET_MOVER            " );
			break;
		case ET_BEAM:
			Com_Printf( "ET_BEAM             " );
			break;
		case ET_PORTAL:
			Com_Printf( "ET_PORTAL           " );
			break;
		case ET_SPEAKER:
			Com_Printf( "ET_SPEAKER          " );
			break;
		case ET_PUSH_TRIGGER:
			Com_Printf( "ET_PUSH_TRIGGER     " );
			break;
		case ET_TELEPORT_TRIGGER:
			Com_Printf( "ET_TELEPORT_TRIGGER " );
			break;
		case ET_INVISIBLE:
			Com_Printf( "ET_INVISIBLE        " );
			break;
		case ET_GRAPPLE:
			Com_Printf( "ET_GRAPPLE          " );
			break;
		case ET_EXPLOSIVE:
			Com_Printf( "ET_EXPLOSIVE        " );
			break;
		case ET_TESLA_EF:
			Com_Printf( "ET_TESLA_EF         " );
			break;
		case ET_SPOTLIGHT_EF:
			Com_Printf( "ET_SPOTLIGHT_EF     " );
			break;
		case ET_EFFECT3:
			Com_Printf( "ET_EFFECT3          " );
			break;
		case ET_ALARMBOX:
			Com_Printf( "ET_ALARMBOX          " );
			break;
		default:
			Com_Printf( "%3i                 ", check->shared.s.eType );
			break;
		}

		if ( check->classname ) {
			Com_Printf( "%s", check->classname );
		}
		Com_Printf( "\n" );
	}
}

GameClient   *ClientForString( const char *s ) {
	GameClient   *cl;
	int i;
	int idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return nullptr;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			Com_Printf( "Client %i is not connected\n", idnum );
			return nullptr;
		}
		return cl;
	}

	// check for a name match
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	Com_Printf( "User %s is not on the server\n", s );

	return nullptr;
}


char    *ConcatArgs( int start );

/*
=================
ConsoleCommand

=================
*/
bool    ConsoleCommand( void ) {
	char cmd[MAX_TOKEN_CHARS];

	Cmd_ArgvBuffer( 0, cmd, sizeof( cmd ) );

	// Ridah, savegame
	if ( Q_stricmp( cmd, "savegame" ) == 0 ) {

		// don't allow a manual savegame command while we are waiting for the game to start/exit
		if ( g_reloading.integer ) {
			return true;
		}
		if ( saveGamePending ) {
			return true;
		}

		Cmd_ArgvBuffer( 1, cmd, sizeof( cmd ) );
		if ( strlen( cmd ) > 0 ) {
			// strip the extension if provided
			if ( strrchr( cmd, '.' ) ) {
				cmd[strrchr( cmd,'.' ) - cmd] = '\0';
			}
			if ( !Q_stricmp( cmd, "current" ) ) {     // beginning of map
				Com_Printf( "sorry, '%s' is a reserved savegame name.  please use another name.\n", cmd );
				return true;
			}

			if ( G_SaveGame( cmd ) ) {
				SV_GameSendServerCommand( -1, "cp gamesaved" );  // deletedgame
			} else {
				Com_Printf( "Unable to save game.\n" );
			}

		} else {    // need a name
			Com_Printf( "syntax: savegame <name>\n" );
		}

		return true;
	}
	// done.

	if ( Q_stricmp( cmd, "entitylist" ) == 0 ) {
		Svcmd_EntityList_f();
		return true;
	}

	if ( Q_stricmp( cmd, "game_memory" ) == 0 ) {
		Svcmd_GameMem_f();
		return true;
	}

	return false;
}

