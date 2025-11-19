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
 * name:		ai_cmd.c
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
BotGetItemTeamGoal

FIXME: add stuff like "upper rocket launcher"
"the rl near the railgun", "lower grenade launcher" etc.
==================
*/
int BotGetItemTeamGoal( char *goalname, bot_goal_t *goal ) {
	int i;

	if ( !strlen( goalname ) ) {
		return qfalse;
	}
	i = -1;
	do {
		i = trap_BotGetLevelItemGoal( i, goalname, goal );
		if ( i > 0 ) { // && !AvoidGoalTime(&bs->gs, goal.number))
			return qtrue;
		}
	} while ( i > 0 );
	return qfalse;
}

/*
==================
BotGetMessageTeamGoal
==================
*/
int BotGetMessageTeamGoal( bot_state_t *bs, char *goalname, bot_goal_t *goal ) {
	bot_waypoint_t *cp;

	if ( BotGetItemTeamGoal( goalname, goal ) ) {
		return qtrue;
	}

	cp = BotFindWayPoint( bs->checkpoints, goalname );
	if ( cp ) {
		memcpy( goal, &cp->goal, sizeof( bot_goal_t ) );
		return qtrue;
	}
	return qfalse;
}

/*
==================
BotGetTime
==================
*/
float BotGetTime( bot_match_t *match ) {
	bot_match_t timematch;
	char timestring[MAX_MESSAGE_SIZE];
	float t;
/* IJB
	//if the matched string has a time
	if ( match->subtype & ST_TIME ) {
		//get the time string
		trap_BotMatchVariable( match, TIME, timestring, MAX_MESSAGE_SIZE );
		//match it to find out if the time is in seconds or minutes
		if ( trap_BotFindMatch( timestring, &timematch, MTCONTEXT_TIME ) ) {
			if ( timematch.type == MSG_FOREVER ) {
				t = 99999999;
			} else {
				trap_BotMatchVariable( &timematch, TIME, timestring, MAX_MESSAGE_SIZE );
				if ( timematch.type == MSG_MINUTES ) {
					t = atof( timestring ) * 60;
				} else if ( timematch.type == MSG_SECONDS ) {
					t = atof( timestring );
				} else { t = 0;}
			}
			//if there's a valid time
			if ( t > 0 ) {
				return trap_AAS_Time() + t;
			}
		}
	}
 */
	return 0;
}

/*
==================
FindClientByName
==================
*/
int FindClientByName( char *name ) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = Cvar_VariableIntegerValue( "sv_maxclients" );
	}
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		ClientName( i, buf, sizeof( buf ) );
		if ( !Q_stricmp( buf, name ) ) {
			return i;
		}
	}
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		ClientName( i, buf, sizeof( buf ) );
		if ( stristr( buf, name ) ) {
			return i;
		}
	}
	return -1;
}

/*
==================
FindEnemyByName
==================
*/
int FindEnemyByName( bot_state_t *bs, char *name ) {
	int i;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = Cvar_VariableIntegerValue( "sv_maxclients" );
	}
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		if ( BotSameTeam( bs, i ) ) {
			continue;
		}
		ClientName( i, buf, sizeof( buf ) );
		if ( !Q_stricmp( buf, name ) ) {
			return i;
		}
	}
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		if ( BotSameTeam( bs, i ) ) {
			continue;
		}
		ClientName( i, buf, sizeof( buf ) );
		if ( stristr( buf, name ) ) {
			return i;
		}
	}
	return -1;
}

/*
==================
NumPlayersOnSameTeam
==================
*/
int NumPlayersOnSameTeam( bot_state_t *bs ) {
	int i, num;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	num = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		SV_GetConfigstring( CS_PLAYERS + i, buf, MAX_INFO_STRING );
		if ( strlen( buf ) ) {
			if ( BotSameTeam( bs, i + 1 ) ) {
				num++;
			}
		}
	}
	return num;
}


int BotGetPatrolWaypoints( bot_state_t *bs, bot_match_t *match ) {
    
    return 0;
/*
	char keyarea[MAX_MESSAGE_SIZE];
	int patrolflags;
	bot_waypoint_t *wp, *newwp, *newpatrolpoints;
	bot_match_t keyareamatch;
	bot_goal_t goal;

	newpatrolpoints = NULL;
	patrolflags = 0;
	//
	trap_BotMatchVariable( match, KEYAREA, keyarea, MAX_MESSAGE_SIZE );
	//
	while ( 1 ) {
		if ( !trap_BotFindMatch( keyarea, &keyareamatch, MTCONTEXT_PATROLKEYAREA ) ) {
			trap_EA_SayTeam( bs->client, "what do you say?" );
			BotFreeWaypoints( newpatrolpoints );
			bs->patrolpoints = NULL;
			return qfalse;
		}
		trap_BotMatchVariable( &keyareamatch, KEYAREA, keyarea, MAX_MESSAGE_SIZE );
		if ( !BotGetMessageTeamGoal( bs, keyarea, &goal ) ) {
			//BotAI_BotInitialChat(bs, "cannotfind", keyarea, NULL);
			//trap_BotEnterChat(bs->cs, bs->client, CHAT_TEAM);
			BotFreeWaypoints( newpatrolpoints );
			bs->patrolpoints = NULL;
			return qfalse;
		}
		//create a new waypoint
		newwp = BotCreateWayPoint( keyarea, goal.origin, goal.areanum );
		//add the waypoint to the patrol points
		newwp->next = NULL;
		for ( wp = newpatrolpoints; wp && wp->next; wp = wp->next ) ;
		if ( !wp ) {
			newpatrolpoints = newwp;
			newwp->prev = NULL;
		} else {
			wp->next = newwp;
			newwp->prev = wp;
		}
		//
		if ( keyareamatch.subtype & ST_BACK ) {
			patrolflags = PATROL_LOOP;
			break;
		} else if ( keyareamatch.subtype & ST_REVERSE )     {
			patrolflags = PATROL_REVERSE;
			break;
		} else if ( keyareamatch.subtype & ST_MORE )     {
			trap_BotMatchVariable( &keyareamatch, MORE, keyarea, MAX_MESSAGE_SIZE );
		} else {
			break;
		}
	}
	//
	if ( !newpatrolpoints || !newpatrolpoints->next ) {
		trap_EA_SayTeam( bs->client, "I need more key points to patrol\n" );
		BotFreeWaypoints( newpatrolpoints );
		newpatrolpoints = NULL;
		return qfalse;
	}
	//
	BotFreeWaypoints( bs->patrolpoints );
	bs->patrolpoints = newpatrolpoints;
	//
	bs->curpatrolpoint = bs->patrolpoints;
	bs->patrolflags = patrolflags;
	//
	return qtrue;
 */
}

/*
==================
BotAddressedToBot
==================
*/
int BotAddressedToBot( bot_state_t *bs, bot_match_t *match ) {
    return qfalse;
    
/* IJB - no chat
	char addressedto[MAX_MESSAGE_SIZE];
	char netname[MAX_MESSAGE_SIZE];
	char name[MAX_MESSAGE_SIZE];
	char botname[128];
	int client;
	bot_match_t addresseematch;

	trap_BotMatchVariable( match, NETNAME, netname, sizeof( netname ) );
	client = ClientFromName( netname );
	if ( client < 0 ) {
		return qfalse;
	}
	if ( !BotSameTeam( bs, client ) ) {
		return qfalse;
	}
	//if the message is addressed to someone
	if ( match->subtype & ST_ADDRESSED ) {
		trap_BotMatchVariable( match, ADDRESSEE, addressedto, sizeof( addressedto ) );
		//the name of this bot
		ClientName( bs->client, botname, 128 );
		//
		while ( trap_BotFindMatch( addressedto, &addresseematch, MTCONTEXT_ADDRESSEE ) ) {
			if ( addresseematch.type == MSG_EVERYONE ) {
				return qtrue;
			} else if ( addresseematch.type == MSG_MULTIPLENAMES )     {
				trap_BotMatchVariable( &addresseematch, TEAMMATE, name, sizeof( name ) );
				if ( strlen( name ) ) {
					if ( stristr( botname, name ) ) {
						return qtrue;
					}
					if ( stristr( bs->subteam, name ) ) {
						return qtrue;
					}
				}
				trap_BotMatchVariable( &addresseematch, MORE, addressedto, MAX_MESSAGE_SIZE );
			} else {
				trap_BotMatchVariable( &addresseematch, TEAMMATE, name, MAX_MESSAGE_SIZE );
				if ( strlen( name ) ) {
					if ( stristr( botname, name ) ) {
						return qtrue;
					}
					if ( stristr( bs->subteam, name ) ) {
						return qtrue;
					}
				}
				break;
			}
		}
		//snprintf(buf, sizeof(buf), "not addressed to me but %s", addressedto);
		//trap_EA_Say(bs->client, buf);
		return qfalse;
	} else {
		//make sure not everyone reacts to this message
		if ( random() > (float ) 1.0 / ( NumPlayersOnSameTeam( bs ) - 1 ) ) {
			return qfalse;
		}
	}
	return qtrue;
 */
}

/*
==================
BotGPSToPosition
==================
*/
int BotGPSToPosition( char *buf, vec3_t position ) {
	int i, j = 0;
	int num, sign;

	for ( i = 0; i < 3; i++ ) {
		num = 0;
		while ( buf[j] == ' ' ) j++;
		if ( buf[j] == '-' ) {
			j++;
			sign = -1;
		} else {
			sign = 1;
		}
		while ( buf[j] ) {
			if ( buf[j] >= '0' && buf[j] <= '9' ) {
				num = num * 10 + buf[j] - '0';
				j++;
			} else {
				j++;
				break;
			}
		}
		BotAI_Print( PRT_MESSAGE, "%d\n", sign * num );
		position[i] = (float) sign * num;
	}
	return qtrue;
}

/*
==================
BotMatch_HelpAccompany
==================
*/
void BotMatch_HelpAccompany( bot_state_t *bs, bot_match_t *match ) {
    
    return;
    

}

/*
==================
BotMatch_DefendKeyArea
==================
*/
void BotMatch_DefendKeyArea( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_GetItem
==================
*/
void BotMatch_GetItem( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_Camp
==================
*/
void BotMatch_Camp( bot_state_t *bs, bot_match_t *match ) {
    return;
	
}

/*
==================
BotMatch_Patrol
==================
*/
void BotMatch_Patrol( bot_state_t *bs, bot_match_t *match ) {
    return;
	
}

/*
==================
BotMatch_JoinSubteam
==================
*/
void BotMatch_JoinSubteam( bot_state_t *bs, bot_match_t *match ) {
    return;

}

/*
==================
BotMatch_LeaveSubteam
==================
*/
void BotMatch_LeaveSubteam( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_LeaveSubteam
==================
*/
void BotMatch_WhichTeam( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_CheckPoint
==================
*/
void BotMatch_CheckPoint( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_FormationSpace
==================
*/
void BotMatch_FormationSpace( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_Dismiss
==================
*/
void BotMatch_Dismiss( bot_state_t *bs, bot_match_t *match ) {
    return;

}

/*
==================
BotMatch_StartTeamLeaderShip
==================
*/
void BotMatch_StartTeamLeaderShip( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_StopTeamLeaderShip
==================
*/
void BotMatch_StopTeamLeaderShip( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_WhoIsTeamLeader
==================
*/
void BotMatch_WhoIsTeamLeader( bot_state_t *bs, bot_match_t *match ) {
    return;
}


/*
==================
BotMatch_WhatIsMyCommand
==================
*/
void BotMatch_WhatIsMyCommand( bot_state_t *bs, bot_match_t *match ) {
	char netname[MAX_NETNAME];

	ClientName( bs->client, netname, sizeof( netname ) );
	if ( Q_stricmp( netname, bs->teamleader ) != 0 ) {
		return;
	}
	bs->forceorders = qtrue;
}

/*
==================
BotNearestVisibleItem
==================
*/
float BotNearestVisibleItem( bot_state_t *bs, char *itemname, bot_goal_t *goal ) {
	int i;
	char name[64];
	bot_goal_t tmpgoal;
	float dist, bestdist;
	vec3_t dir;
	bsp_trace_t trace;

	bestdist = 999999;
	i = -1;
	do {
		i = trap_BotGetLevelItemGoal( i, itemname, &tmpgoal );
		trap_BotGoalName( tmpgoal.number, name, sizeof( name ) );
		if ( Q_stricmp( itemname, name ) != 0 ) {
			continue;
		}
		VectorSubtract( tmpgoal.origin, bs->origin, dir );
		dist = VectorLength( dir );
		if ( dist < bestdist ) {
			//trace from start to end
			BotAI_Trace( &trace, bs->eye, NULL, NULL, tmpgoal.origin, bs->client, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );
			if ( trace.fraction >= 1.0 ) {
				bestdist = dist;
				memcpy( goal, &tmpgoal, sizeof( bot_goal_t ) );
			}
		}
	} while ( i > 0 );
	return bestdist;
}

/*
==================
BotMatch_WhereAreYou
==================
*/
void BotMatch_WhereAreYou( bot_state_t *bs, bot_match_t *match ) {
    return;

}

/*
==================
BotMatch_LeadTheWay
==================
*/
void BotMatch_LeadTheWay( bot_state_t *bs, bot_match_t *match ) {
    return;
}

/*
==================
BotMatch_Kill
==================
*/
void BotMatch_Kill( bot_state_t *bs, bot_match_t *match ) {
    return;

}
