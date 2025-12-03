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

//===========================================================================
//
/*****************************************************************************
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 *
 *****************************************************************************/

#define BOTLIB_API_VERSION      2

struct aas_clientmove_s;
struct aas_entityinfo_s;
struct bot_consolemessage_s;
struct bot_match_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;
struct weaponinfo_s;


//debug line colors
#define LINECOLOR_NONE          -1
#define LINECOLOR_RED           1 //0xf2f2f0f0L
#define LINECOLOR_GREEN         2 //0xd0d1d2d3L
#define LINECOLOR_BLUE          3 //0xf3f3f1f1L
#define LINECOLOR_YELLOW        4 //0xdcdddedfL
#define LINECOLOR_ORANGE        5 //0xe0e1e2e3L

//Print types
#define PRT_MESSAGE             1
#define PRT_WARNING             2
#define PRT_ERROR               3
#define PRT_FATAL               4
#define PRT_EXIT                5

//console message types
#define CMS_NORMAL              0
#define CMS_CHAT                1

//botlib error codes
#define BLERR_NOERROR                   0   //no error
#define BLERR_LIBRARYNOTSETUP           1   //library not setup
#define BLERR_LIBRARYALREADYSETUP       2   //BotSetupLibrary: library already setup
#define BLERR_INVALIDCLIENTNUMBER       3   //invalid client number
#define BLERR_INVALIDENTITYNUMBER       4   //invalid entity number
#define BLERR_NOAASFILE                 5   //BotLoadMap: no AAS file available
#define BLERR_CANNOTOPENAASFILE         6   //BotLoadMap: cannot open AAS file
#define BLERR_CANNOTSEEKTOAASFILE       7   //BotLoadMap: cannot seek to AAS file
#define BLERR_CANNOTREADAASHEADER       8   //BotLoadMap: cannot read AAS header
#define BLERR_WRONGAASFILEID            9   //BotLoadMap: incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION       10  //BotLoadMap: incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP         11  //BotLoadMap: cannot read AAS file lump
#define BLERR_NOBSPFILE                 12  //BotLoadMap: no BSP file available
#define BLERR_CANNOTOPENBSPFILE         13  //BotLoadMap: cannot open BSP file
#define BLERR_CANNOTSEEKTOBSPFILE       14  //BotLoadMap: cannot seek to BSP file
#define BLERR_CANNOTREADBSPHEADER       15  //BotLoadMap: cannot read BSP header
#define BLERR_WRONGBSPFILEID            16  //BotLoadMap: incorrect BSP file id
#define BLERR_WRONGBSPFILEVERSION       17  //BotLoadMap: incorrect BSP file version
#define BLERR_CANNOTREADBSPLUMP         18  //BotLoadMap: cannot read BSP file lump
#define BLERR_AICLIENTNOTSETUP          19  //BotAI: client not setup
#define BLERR_AICLIENTALREADYSETUP      20  //BotSetupClient: client already setup
#define BLERR_AIMOVEINACTIVECLIENT      21  //BotMoveClient: cannot move inactive client
#define BLERR_AIMOVETOACTIVECLIENT      22  //BotMoveClient: cannot move to active client
#define BLERR_AICLIENTALREADYSHUTDOWN   23  //BotShutdownClient: client not setup
#define BLERR_AIUPDATEINACTIVECLIENT    24  //BotUpdateClient: called for inactive client
#define BLERR_AICMFORINACTIVECLIENT     25  //BotConsoleMessage: called for inactive client
#define BLERR_SETTINGSINACTIVECLIENT    26  //BotClientSettings: called for inactive client
#define BLERR_CANNOTLOADICHAT           27  //BotSetupClient: cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS     28  //BotSetupClient: cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG      29  //BotSetupLibrary: cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS   30  //BotSetupClient: cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG    31  //BotSetupLibrary: cannot load weapon config
#define BLERR_INVALIDSOUNDINDEX         32  //BotAddSound: invalid sound index value

//action flags
#define ACTION_ATTACK           1
#define ACTION_USE              2
#define ACTION_RESPAWN          4
#define ACTION_JUMP             8
#define ACTION_MOVEUP           8
#define ACTION_CROUCH           16
#define ACTION_MOVEDOWN         16
#define ACTION_MOVEFORWARD      32
#define ACTION_MOVEBACK         64
#define ACTION_MOVELEFT         128
#define ACTION_MOVERIGHT        256
#define ACTION_DELAYEDJUMP      512
#define ACTION_TALK             1024
#define ACTION_GESTURE          2048
#define ACTION_WALK             4096
#define ACTION_RELOAD           8192

//the bot input, will be converted to an UserCmd
/*
typedef struct bot_input_s
{
	float thinktime;        //time since last output (in seconds)
	vec3_t dir;             //movement direction
	float speed;            //speed in the range [0, 400]
	vec3_t viewangles;      //the view angles
	int actionflags;        //one of the ACTION_? flags
	int weapon;             //weapon to use
} bot_input_t;
*/
#ifndef BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	bool allsolid;          // if true, plane is not valid
	bool startsolid;        // if true, the initial point was in a solid area
	float fraction;             // time completed, 1.0 = didn't hit anything
	vec3_t endpos;              // final position
	cplane_t plane;             // surface normal at impact
	float exp_dist;             // expanded plane distance
	int sidenum;                // number of the brush side hit
	bsp_surface_t surface;      // the hit point surface
	int contents;               // contents on other side of surface hit
	int ent;                    // number of entity hit
} bsp_trace_t;

#define BSPTRACE
#endif  // BSPTRACE


