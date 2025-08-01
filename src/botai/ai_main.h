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

#pragma once

/*****************************************************************************
 * name:		ai_main.h
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

#include "../botlib/be_aas_def.h"
#include "../botai/botai.h"
#include "../game/be_ai_goal.h"
#include "../game/g_local.h"

//#define DEBUG
#define CTF

#define MAX_ITEMS                   256
//bot flags
#define BFL_STRAFERIGHT             1   //strafe to the right
#define BFL_ATTACKED                2   //bot has attacked last ai frame
#define BFL_ATTACKJUMPED            4   //bot jumped during attack last frame
#define BFL_AIMATENEMY              8   //bot aimed at the enemy this frame
#define BFL_AVOIDRIGHT              16  //avoid obstacles by going to the right
#define BFL_IDEALVIEWSET            32  //bot has ideal view angles set
//long term goal types
#define LTG_TEAMHELP                1   //help a team mate
#define LTG_TEAMACCOMPANY           2   //accompany a team mate
#define LTG_DEFENDKEYAREA           3   //defend a key area
#define LTG_GETFLAG                 4   //get the enemy flag
#define LTG_RUSHBASE                5   //rush to the base
#define LTG_RETURNFLAG              6   //return the flag
#define LTG_CAMP                    7   //camp somewhere
#define LTG_CAMPORDER               8   //ordered to camp somewhere
#define LTG_PATROL                  9   //patrol
#define LTG_GETITEM                 10  //get an item
#define LTG_KILL                    11  //kill someone
//some goal dedication times
#define TEAM_HELP_TIME              60  //1 minute teamplay help time
#define TEAM_ACCOMPANY_TIME         600 //10 minutes teamplay accompany time
#define TEAM_DEFENDKEYAREA_TIME     240 //4 minutes ctf defend base time
#define TEAM_CAMP_TIME              600 //10 minutes camping time
#define TEAM_PATROL_TIME            600 //10 minutes patrolling time
#define TEAM_LEAD_TIME              600 //10 minutes taking the lead
#define TEAM_GETITEM_TIME           60  //1 minute
#define TEAM_KILL_SOMEONE           180 //3 minute to kill someone
#define CTF_GETFLAG_TIME            240 //4 minutes ctf get flag time
#define CTF_RUSHBASE_TIME           120 //2 minutes ctf rush base time
#define CTF_RETURNFLAG_TIME         180 //3 minutes to return the flag
#define CTF_ROAM_TIME               60  //1 minute ctf roam time
//patrol flags
#define PATROL_LOOP                 1
#define PATROL_REVERSE              2
#define PATROL_BACK                 4
//copied from the aas file header
#define PRESENCE_NONE               1
#define PRESENCE_NORMAL             2
#define PRESENCE_CROUCH             4

//check points
typedef struct bot_waypoint_s
{
	int inuse;
	char name[32];
	bot_goal_t goal;
	struct      bot_waypoint_s *next, *prev;
} bot_waypoint_t;

//bot state
typedef struct bot_state_s
{
	int inuse;                                      //true if this state is used by a bot client
	int botthink_residual;                          //residual for the bot thinks
	int client;                                     //client number of the bot
	int entitynum;                                  //entity number of the bot
	playerState_t cur_ps;                           //current player state
	int last_eFlags;                                //last ps flags
	usercmd_t lastucmd;                             //usercmd from last frame
	int entityeventTime[1024];                      //last entity event time
	//
	bot_settings_t settings;                        //several bot settings
	int ( *ainode )( struct bot_state_s *bs );          //current AI node
	float thinktime;                                //time the bot thinks this frame
	vec3_t origin;                                  //origin of the bot
	vec3_t velocity;                                //velocity of the bot
	int presencetype;                               //presence type of the bot
	vec3_t eye;                                     //eye coordinates of the bot
	int areanum;                                    //the number of the area the bot is in
	int inventory[MAX_ITEMS];                       //string with items amounts the bot has
	int tfl;                                        //the travel flags the bot uses
	int flags;                                      //several flags
	int respawn_wait;                               //wait until respawned
	int lasthealth;                                 //health value previous frame
	int lastkilledplayer;                           //last killed player
	int lastkilledby;                               //player that last killed this bot
	int botdeathtype;                               //the death type of the bot
	int enemydeathtype;                             //the death type of the enemy
	int botsuicide;                                 //true when the bot suicides
	int enemysuicide;                               //true when the enemy of the bot suicides
	int setupcount;                                 //true when the bot has just been setup
	int entergamechat;                              //true when the bot used an enter game chat
	int num_deaths;                                 //number of time this bot died
	int num_kills;                                  //number of kills of this bot
	int revenge_enemy;                              //the revenge enemy
	int revenge_kills;                              //number of kills the enemy made
	int lastframe_health;                           //health value the last frame
	int lasthitcount;                               //number of hits last frame
	int chatto;                                     //chat to all or team
	float walker;                                   //walker charactertic
	float ltime;                                    //local bot time
	float entergame_time;                           //time the bot entered the game
	float ltg_time;                                 //long term goal time
	float nbg_time;                                 //nearby goal time
	float respawn_time;                             //time the bot takes to respawn
	float respawnchat_time;                         //time the bot started a chat during respawn
	float chase_time;                               //time the bot will chase the enemy
	float enemyvisible_time;                        //time the enemy was last visible
	float check_time;                               //time to check for nearby items
	float stand_time;                               //time the bot is standing still
	float lastchat_time;                            //time the bot last selected a chat
	float standfindenemy_time;                      //time to find enemy while standing
	float attackstrafe_time;                        //time the bot is strafing in one dir
	float attackcrouch_time;                        //time the bot will stop crouching
	float attackchase_time;                         //time the bot chases during actual attack
	float attackjump_time;                          //time the bot jumped during attack
	float enemysight_time;                          //time before reacting to enemy
	float enemydeath_time;                          //time the enemy died
	float enemyposition_time;                       //time the position and velocity of the enemy were stored
	float activate_time;                            //time to activate something
	float activatemessage_time;                     //time to show activate message
	float defendaway_time;                          //time away while defending
	float defendaway_range;                         //max travel time away from defend area
	float rushbaseaway_time;                        //time away from rushing to the base
	float ctfroam_time;                             //time the bot is roaming in ctf
	float killedenemy_time;                         //time the bot killed the enemy
	float arrive_time;                              //time arrived (at companion)
	float lastair_time;                             //last time the bot had air
	float teleport_time;                            //last time the bot teleported
	float camp_time;                                //last time camped
	float camp_range;                               //camp range
	float weaponchange_time;                        //time the bot started changing weapons
	float firethrottlewait_time;                    //amount of time to wait
	float firethrottleshoot_time;                   //amount of time to shoot
	vec3_t aimtarget;
	vec3_t enemyvelocity;                           //enemy velocity 0.5 secs ago during battle
	vec3_t enemyorigin;                             //enemy origin 0.5 secs ago during battle
	//
	int character;                                  //the bot character
	int ms;                                         //move state of the bot
	int gs;                                         //goal state of the bot
	int cs;                                         //chat state of the bot
	int ws;                                         //weapon state of the bot
	//
	int enemy;                                      //enemy entity number
	int lastenemyareanum;                           //last reachability area the enemy was in
	vec3_t lastenemyorigin;                         //last origin of the enemy in the reachability area
	int weaponnum;                                  //current weapon number
	vec3_t viewangles;                              //current view angles
	vec3_t ideal_viewangles;                        //ideal view angles
	//
	int ltgtype;                                    //long term goal type
	//
	int teammate;                                   //team mate
	bot_goal_t teamgoal;                            //the team goal
	float teammessage_time;                         //time to message team mates what the bot is doing
	float teamgoal_time;                            //time to stop helping team mate
	float teammatevisible_time;                     //last time the team mate was NOT visible
	//
	int lead_teammate;                              //team mate the bot is leading
	bot_goal_t lead_teamgoal;                       //team goal while leading
	float lead_time;                                //time leading someone
	float leadvisible_time;                         //last time the team mate was visible
	float leadmessage_time;                         //last time a messaged was sent to the team mate
	float leadbackup_time;                          //time backing up towards team mate
	//
	char teamleader[32];                            //netname of the team leader
	float askteamleader_time;                       //time asked for team leader
	float becometeamleader_time;                    //time the bot will become the team leader
	float teamgiveorders_time;                      //time to give team orders
	int numteammates;                               //number of team mates
	int redflagstatus;                              //0 = at base, 1 = not at base
	int blueflagstatus;                             //0 = at base, 1 = not at base
	int flagstatuschanged;                          //flag status changed
	int forceorders;                                //true if forced to give orders
	int flagcarrier;                                //team mate carrying the enemy flag
	char subteam[32];                               //sub team name
	float formation_dist;                           //formation team mate intervening space
	char formation_teammate[16];                    //netname of the team mate the bot uses for relative positioning
	float formation_angle;                          //angle relative to the formation team mate
	vec3_t formation_dir;                           //the direction the formation is moving in
	vec3_t formation_origin;                        //origin the bot uses for relative positioning
	bot_goal_t formation_goal;                      //formation goal
	bot_goal_t activategoal;                        //goal to activate (buttons etc.)
	bot_waypoint_t *checkpoints;                    //check points
	bot_waypoint_t *patrolpoints;                   //patrol points
	bot_waypoint_t *curpatrolpoint;                 //current patrol point the bot is going for
	int patrolflags;                                //patrol flags
} bot_state_t;

//resets the whole bot state
void BotResetState( bot_state_t *bs );
//returns the number of bots in the game
int NumBots( void );
//returns info about the entity
void BotEntityInfo( int entnum, aas_entityinfo_t *info );

// Ridah, defines for AI Cast system
int AICast_ShutdownClient( int client );
void AICast_Init( void );
void AICast_StartFrame( int time );
// done.

// from the game source
void QDECL BotAI_Print( int type, char *fmt, ... );
void QDECL QDECL BotAI_BotInitialChat( bot_state_t *bs, char *type, ... );
void    BotAI_Trace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask );
int     BotAI_GetClientState( int clientNum, playerState_t *state );
int     BotAI_GetEntityState( int entityNum, entityState_t *state );
int     BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state );
