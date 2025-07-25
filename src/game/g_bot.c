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

// g_bot.c

#include "g_local.h"
#include "../botai/botai.h"
#include "../qcommon/qcommon.h"

static int g_numBots;
static char g_botInfos[MAX_BOTS][MAX_INFO_STRING];


int g_numArenas;
static char g_arenaInfos[MAX_ARENAS][MAX_INFO_STRING];


#define BOT_BEGIN_DELAY_BASE        2000
#define BOT_BEGIN_DELAY_INCREMENT   1500

#define BOT_SPAWN_QUEUE_DEPTH   16

typedef struct {
	int clientNum;
	int spawnTime;
} botSpawnQueue_t;

static int botBeginDelay;
static botSpawnQueue_t botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

extern gentity_t    *podium1;
extern gentity_t    *podium2;
extern gentity_t    *podium3;

/*
===============
G_LoadArenas
===============
*/
/*
static void G_LoadArenas( void ) {
#ifdef QUAKESTUFF
	int			len;
	char		*filename;
	vmCvar_t	arenasFile;
	fileHandle_t	f;
	int			n;
	char		buf[MAX_ARENAS_TEXT];

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM );
	if( *arenasFile.string ) {
		filename = arenasFile.string;
	}
	else {
		filename = "scripts/arenas.txt";
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numArenas = COM_ParseInfos( buf, MAX_ARENAS, g_arenaInfos );
	trap_Printf( va( "%i arenas parsed\n", g_numArenas ) );

	for( n = 0; n < g_numArenas; n++ ) {
		Info_SetValueForKey( g_arenaInfos[n], "num", va( "%i", n ) );
	}
#endif
}
*/


/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int n;

	for ( n = 0; n < g_numArenas; n++ ) {
		if ( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}


/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char model[MAX_QPATH];
	char    *skin;

	Q_strncpyz( model, modelAndSkin, sizeof( model ) );
	skin = Q_strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	} else {
		skin = model;
	}

	if ( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}

	trap_game_SendConsoleCommand( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team ) {
	int i, n, num, skill;
	char    *value, netname[36], *teamstr;
	gclient_t   *cl;

	num = 0;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		//
		for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( !( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) ) {
				continue;
			}
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if ( i >= g_maxclients.integer ) {
			num++;
		}
	}
	num = random() * num;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		//
		for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( !( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) ) {
				continue;
			}
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if ( i >= g_maxclients.integer ) {
			num--;
			if ( num <= 0 ) {
				skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );
				if ( team == TEAM_RED ) {
					teamstr = "red";
				} else if ( team == TEAM_BLUE ) {
					teamstr = "blue";
				} else { teamstr = "";}
				strncpy( netname, value, sizeof( netname ) - 1 );
				netname[sizeof( netname ) - 1] = '\0';
				Q_CleanStr( netname );
				trap_game_SendConsoleCommand( EXEC_INSERT, va( "addbot %s %i %s %i\n", netname, skill, teamstr, 0 ) );
				return;
			}
		}
	}
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	char netname[36];
	gclient_t   *cl;

	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		strcpy( netname, cl->pers.netname );
		Q_CleanStr( netname );
		trap_game_SendConsoleCommand( EXEC_INSERT, va( "kick %s\n", netname ) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team ) {
	int i, num;
	gclient_t   *cl;

	num = 0;
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int G_CountBotPlayers( int team ) {
	int i, n, num;
	gclient_t   *cl;

	num = 0;
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) ) {
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}
		num++;
	}
	for ( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if ( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	//only check once each 10 seconds
	if ( checkminimumplayers_time > level.time - 10000 ) {
		return;
	}
	checkminimumplayers_time = level.time;
	Cvar_Update( &bot_minplayers );
	minplayers = bot_minplayers.integer;
	if ( minplayers <= 0 ) {
		return;
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		if ( minplayers >= g_maxclients.integer / 2 ) {
			minplayers = ( g_maxclients.integer / 2 ) - 1;
		}

		humanplayers = G_CountHumanPlayers( TEAM_RED );
		botplayers = G_CountBotPlayers( TEAM_RED );
		//
		if ( humanplayers + botplayers < minplayers ) {
			G_AddRandomBot( TEAM_RED );
		} else if ( humanplayers + botplayers > minplayers && botplayers ) {
			G_RemoveRandomBot( TEAM_RED );
		}
		//
		humanplayers = G_CountHumanPlayers( TEAM_BLUE );
		botplayers = G_CountBotPlayers( TEAM_BLUE );
		//
		if ( humanplayers + botplayers < minplayers ) {
			G_AddRandomBot( TEAM_BLUE );
		} else if ( humanplayers + botplayers > minplayers && botplayers ) {
			G_RemoveRandomBot( TEAM_BLUE );
		}
	} else if ( g_gametype.integer == GT_TOURNAMENT )     {
		if ( minplayers >= g_maxclients.integer ) {
			minplayers = g_maxclients.integer - 1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if ( humanplayers + botplayers < minplayers ) {
			G_AddRandomBot( TEAM_FREE );
		} else if ( humanplayers + botplayers > minplayers && botplayers ) {
			// try to remove spectators first
			if ( !G_RemoveRandomBot( TEAM_SPECTATOR ) ) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
	} else if ( g_gametype.integer == GT_FFA )     {
		if ( minplayers >= g_maxclients.integer ) {
			minplayers = g_maxclients.integer - 1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( TEAM_FREE );
		//
		if ( humanplayers + botplayers < minplayers ) {
			G_AddRandomBot( TEAM_FREE );
		} else if ( humanplayers + botplayers > minplayers && botplayers ) {
			G_RemoveRandomBot( TEAM_FREE );
		}
	}
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int n;
	char userinfo[MAX_INFO_VALUE];

	G_CheckMinimumPlayers();

	for ( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if ( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum );
		botSpawnQueue[n].spawnTime = 0;

		if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
			trap_GetUserinfo( botSpawnQueue[n].clientNum, userinfo, sizeof( userinfo ) );
			PlayerIntroSound( Info_ValueForKey( userinfo, "model" ) );
		}
	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int n;

	for ( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if ( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum );
}


/*
===============
G_QueueBotBegin
===============
*/
void G_QueueBotBegin( int clientNum ) {
	AddBotToSpawnQueue( clientNum, botBeginDelay );
	botBeginDelay += BOT_BEGIN_DELAY_INCREMENT;
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t settings;
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof( settings.characterfile ) );
	settings.skill = atoi( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof( settings.team ) );

	if ( !BotAISetupClient( clientNum, &settings ) ) {
		trap_DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	if ( restart && g_gametype.integer == GT_SINGLE_PLAYER ) {
		g_entities[clientNum].botDelayBegin = qtrue;
	} else {
		g_entities[clientNum].botDelayBegin = qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, int skill, const char *team, int delay ) {
	int clientNum;
	char            *botinfo;
	gentity_t       *bot;
	char            *key;
	char            *s;
	char            *botname;
	char            *model;
	char userinfo[MAX_INFO_STRING];

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( name );
	if ( !botinfo ) {
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if ( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "name" );
	}
	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );

	if ( skill == 1 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	} else if ( skill == 2 )   {
		Info_SetValueForKey( userinfo, "handicap", "70" );
	} else if ( skill == 3 )   {
		Info_SetValueForKey( userinfo, "handicap", "90" );
	}

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model ) {
		model = "visor/default";
	}
	Info_SetValueForKey( userinfo, key, model );

	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );

	key = "color";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey( botinfo, "aifile" );
	if ( !*s ) {
		trap_Printf( S_COLOR_RED "Error: bot has no aifile specified\n" );
		return;
	}

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
		G_Printf( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}

	// initialize the bot settings
	if ( !team || !*team ) {
		if ( g_gametype.integer == GT_TEAM || g_gametype.integer == GT_CTF ) {
			if ( PickTeam( clientNum ) == TEAM_RED ) {
				team = "red";
			} else {
				team = "blue";
			}
		} else {
			team = "red";
		}
	}
	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );
	Info_SetValueForKey( userinfo, "team", team );

	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if ( delay == 0 ) {
		ClientBegin( clientNum );
		return;
	}

	AddBotToSpawnQueue( clientNum, delay );
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	int skill;
	int delay;
	char name[MAX_TOKEN_CHARS];
	char string[MAX_TOKEN_CHARS];
	char team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap_Printf( "Usage: Addbot <botname> [skill 1-4] [team] [msec delay]\n" );
		return;
	}

	// skill
	trap_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	} else {
		skill = atoi( string );
	}

	// team
	trap_Argv( 3, team, sizeof( team ) );

	// delay
	trap_Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	} else {
		delay = atoi( string );
	}

	G_AddBot( name, skill, team, delay );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		 trap_Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap_SendServerCommand( -1, "loaddeferred\n" );   // spelling fixed (SA)
	}
}

/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if ( num < 0 || num >= g_numBots ) {
		trap_Printf( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return g_botInfos[num];
}


/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int n;
	char    *value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

/*
===============
G_InitBots
===============
*/
/*
void G_InitBots( qboolean restart ) {

	// Ridah, we don't need this anymore
	return;
	// done.
	int			fragLimit;
	int			timeLimit;
	const char	*arenainfo;
	char		*strValue;
	int			basedelay;
	char		map[MAX_QPATH];
	char		serverinfo[MAX_INFO_STRING];

	G_LoadBots();
	G_LoadArenas();

	trap_Cvar_Register( &bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO );

	if( g_gametype.integer == GT_SINGLE_PLAYER ) {
		trap_GetServerinfo( serverinfo, sizeof(serverinfo) );
		Q_strncpyz( map, Info_ValueForKey( serverinfo, "mapname" ), sizeof(map) );
		arenainfo = G_GetArenaInfoByMap( map );
		if ( !arenainfo ) {
			return;
		}

		strValue = Info_ValueForKey( arenainfo, "fraglimit" );
		fragLimit = atoi( strValue );
		if ( fragLimit ) {
			trap_Cvar_Set( "fraglimit", strValue );
		}
		else {
			trap_Cvar_Set( "fraglimit", "0" );
		}

		strValue = Info_ValueForKey( arenainfo, "timelimit" );
		timeLimit = atoi( strValue );
		if ( timeLimit ) {
			trap_Cvar_Set( "timelimit", strValue );
		}
		else {
			trap_Cvar_Set( "timelimit", "0" );
		}

		if ( !fragLimit && !timeLimit ) {
			trap_Cvar_Set( "fraglimit", "10" );
			trap_Cvar_Set( "timelimit", "0" );
		}

		basedelay = BOT_BEGIN_DELAY_BASE;
		strValue = Info_ValueForKey( arenainfo, "special" );
		if( Q_stricmp( strValue, "training" ) == 0 ) {
			basedelay += 10000;
		}

		if( !restart ) {
			G_SpawnBots( Info_ValueForKey( arenainfo, "bots" ), basedelay );
		}
	}
}
*/
