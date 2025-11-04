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
#include "../server/server.h"

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

	Com_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
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

	SV_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof( settings.characterfile ) );
	settings.skill = atoi( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof( settings.team ) );

	if ( !BotAISetupClient( clientNum, &settings ) ) {
		SV_GameDropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	if ( restart  ) {
		g_entities[clientNum].botDelayBegin = qtrue;
	}

	return qtrue;
}


