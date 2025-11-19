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


/*****************************************************************************
 * name:		ai_chat.c
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

#include "../game/g_local.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
//
#include "chars.h"               //characteristics
#include "inv.h"             //indexes into the inventory
#include "syn.h"             //synonyms
#include "match.h"               //string matching types and vars

#include "../qcommon/qcommon.h"
#include "../server/server.h"

/*
==================
BotNumActivePlayers
==================
*/
int BotNumActivePlayers( void ) {
	int i, num;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	num = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		SV_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}

		//
		num++;
	}
	return num;
}


/*
==================
BotRandomOpponentName
==================
*/
char *BotRandomOpponentName( bot_state_t *bs ) {
	int i, count;
	char buf[MAX_INFO_STRING];
	int opponents[MAX_CLIENTS], numopponents;
	static int maxclients;
	static char name[32];

	if ( !maxclients ) {
		maxclients = Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	numopponents = 0;
	opponents[0] = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		if ( i == bs->client ) {
			continue;
		}
		//
		SV_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}

		//skip team mates
		if ( BotSameTeam( bs, i ) ) {
			continue;
		}
		//
		opponents[numopponents] = i;
		numopponents++;
	}
	count = random() * numopponents;
	for ( i = 0; i < numopponents; i++ ) {
		count--;
		if ( count <= 0 ) {
			EasyClientName( opponents[i], name, sizeof( name ) );
			return name;
		}
	}
	EasyClientName( opponents[0], name, sizeof( name ) );
	return name;
}

/*
==================
BotMapTitle
==================
*/

char *BotMapTitle( void ) {
	char info[1024];
	static char mapname[128];

	SV_GetServerinfo( info, sizeof( info ) );

	strncpy( mapname, Info_ValueForKey( info, "mapname" ), sizeof( mapname ) - 1 );
	mapname[sizeof( mapname ) - 1] = '\0';

	return mapname;
}


/*
==================
BotWeaponNameForMeansOfDeath
==================
*/

char *BotWeaponNameForMeansOfDeath( int mod ) {
	switch ( mod ) {
	case MOD_SHOTGUN: return "Shotgun";
	case MOD_GAUNTLET: return "Gauntlet";
	case MOD_MACHINEGUN: return "Machinegun";
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH: return "Grenade Launcher";
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH: return "Rocket Launcher";
	case MOD_RAILGUN: return "Railgun";
	case MOD_LIGHTNING: return "Lightning Gun";
	case MOD_BFG:
	case MOD_BFG_SPLASH: return "BFG10K";
	case MOD_GRAPPLE: return "Grapple";
	default: return "[unknown weapon]";
	}
}

/*
==================
BotRandomWeaponName
==================
*/
char *BotRandomWeaponName( void ) {
	int rnd;

	rnd = random() * 8.9;
	switch ( rnd ) {
	case 0: return "Gauntlet";
	case 1: return "Shotgun";
	case 2: return "Machinegun";
	case 3: return "Grenade Launcher";
	case 4: return "Rocket Launcher";
	case 5: return "Plasmagun";
	case 6: return "Railgun";
	case 7: return "Lightning Gun";
	default: return "BFG10K";
	}
}

/*
==================
BotValidChatPosition
==================
*/
int BotValidChatPosition( bot_state_t *bs ) {
    return qfalse;
}

/*
==================
BotChat_EnterGame
==================
*/
int BotChat_EnterGame( bot_state_t *bs ) {
	
    return qfalse;
	
}

/*
==================
BotChat_ExitGame
==================
*/
int BotChat_ExitGame( bot_state_t *bs ) {
	
    return qfalse;
	
}

/*
==================
BotChat_StartLevel
==================
*/
int BotChat_StartLevel( bot_state_t *bs ) {
	
		return qfalse;
	
}


/*
==================
BotChat_Kill
==================
*/
int BotChat_Kill( bot_state_t *bs ) {
	
		return qfalse;
	
}

/*
==================
BotChat_EnemySuicide
==================
*/
int BotChat_EnemySuicide( bot_state_t *bs ) {
	
		return qfalse;
	
}

/*
==================
BotChat_HitTalking
==================
*/
int BotChat_HitTalking( bot_state_t *bs ) {
	
		return qfalse;
	
}

/*
==================
BotChat_HitNoDeath
==================
*/
int BotChat_HitNoDeath( bot_state_t *bs ) {
	
		return qfalse;
	
}

/*
==================
BotChat_HitNoKill
==================
*/
int BotChat_HitNoKill( bot_state_t *bs ) {
	
		return qfalse;
	
}

/*
==================
BotChat_Random
==================
*/
int BotChat_Random( bot_state_t *bs ) {
	
		return qfalse;
	
}

/*
==================
BotChatTime
==================
*/
float BotChatTime( bot_state_t *bs ) {


	return 2.0; 
}

