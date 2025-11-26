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

/*
 * name:		cg_consolecmds.c
 *
 * desc:		text commands typed in at the local console, or executed by a key binding
 *
*/


#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "../qcommon/qcommon.h"


void CG_TargetCommand_f( void ) {
	int targetNum;
	char test[4];

	targetNum = CG_CrosshairPlayer();
	if ( !targetNum ) {
		return;
	}

	Cmd_ArgvBuffer( 1, test, 4 );
	Cbuf_AddText( va( "gc %i %i", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f( void ) {
	Cvar_Set( "cg_viewsize", va( "%i",(int)( cg_viewsize.integer + 10 ) ) );
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f( void ) {
	Cvar_Set( "cg_viewsize", va( "%i",(int)( cg_viewsize.integer - 10 ) ) );
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f( void ) {
	Com_Printf( "(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
			   (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
			   (int)cg.refdefViewAngles[YAW] );
}

static void CG_InventoryDown_f( void ) {
	cg.showItems = qtrue;
}

static void CG_InventoryUp_f( void ) {
	cg.showItems = qfalse;
	cg.itemFadeTime = cg.time;
}

static void CG_TellTarget_f( void ) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Cmd_ArgsBuffer( message, 128 );
	snprintf( command, 128, "tell %i %s", clientNum, message );
	CL_AddReliableCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	Cmd_ArgsBuffer( message, 128 );
	snprintf( command, 128, "tell %i %s", clientNum, message );
	CL_AddReliableCommand( command );
}


#define MAX_CAMERAS 64  // matches define in splines.cpp
qboolean cameraInuse[MAX_CAMERAS];

int CG_LoadCamera( const char *name ) {
	int i;
	for ( i = 1; i < MAX_CAMERAS; i++ ) {    // start at '1' since '0' is always taken by the cutscene camera
		if ( !cameraInuse[i] ) {
			if ( trap_loadCamera( i, name ) ) {
				cameraInuse[i] = qtrue;
				return i;
			}
		}
	}
	return -1;
}

void CG_FreeCamera( int camNum ) {
	cameraInuse[camNum] = qfalse;
}

/*
==============
CG_StartCamera
==============
*/
void CG_StartCamera( const char *name, qboolean startBlack ) {
	char lname[MAX_QPATH];

	COM_StripExtension( name, lname );
	strcat( lname, ".camera" );

	if ( trap_loadCamera( CAM_PRIMARY, va( "cameras/%s", lname ) ) ) {
		cg.cameraMode = qtrue;                  // camera on in cgame
		if ( startBlack ) {
			CG_Fade( 0, 0, 0, 255, cg.time, 0 );  // go black
		}
		Cvar_Set( "cg_letterbox", "1" ); // go letterbox
		CL_AddReliableCommand( "startCamera" );   // camera on in game
		trap_startCamera( CAM_PRIMARY, cg.time ); // camera on in client
	} else {
//----(SA)	removed check for cams in main dir
		cg.cameraMode = qfalse;                 // camera off in cgame
		CL_AddReliableCommand( "stopCamera" );    // camera off in game
		trap_stopCamera( CAM_PRIMARY );           // camera off in client
		CG_Fade( 0, 0, 0, 0, cg.time, 0 );        // ensure fadeup
		Cvar_Set( "cg_letterbox", "0" );
		Com_Printf( "Unable to load camera %s\n",lname );
	}
}

/*
==============
CG_SopCamera
==============
*/
void CG_StopCamera( void ) {
	cg.cameraMode = qfalse;                 // camera off in cgame
	CL_AddReliableCommand( "stopCamera" );    // camera off in game
	trap_stopCamera( CAM_PRIMARY );           // camera off in client
	Cvar_Set( "cg_letterbox", "0" );

	// fade back into world
	CG_Fade( 0, 0, 0, 255, 0, 0 );
	CG_Fade( 0, 0, 0, 0, cg.time + 500, 2000 );

}

static void CG_Camera_f( void ) {
	char name[MAX_QPATH];
	Cmd_ArgvBuffer( 1, name, sizeof( name ) );

	CG_StartCamera( name, qfalse );
}

static void CG_Fade_f( void ) {
	int r, g, b, a;
	float duration;

	if ( Cmd_Argc() < 6 ) {
		return;
	}

	r = atof( CG_Argv( 1 ) );
	g = atof( CG_Argv( 2 ) );
	b = atof( CG_Argv( 3 ) );
	a = atof( CG_Argv( 4 ) );

	duration = atof( CG_Argv( 5 ) ) * 1000;

	CG_Fade( r, g, b, a, cg.time, duration );
}


typedef struct {
	const char    *cmd;
	void ( *function )( void );
} consoleCommand_t;

static consoleCommand_t commands[] = {
	{ "viewpos", CG_Viewpos_f },

	{ "+inventory", CG_InventoryDown_f },
	{ "-inventory", CG_InventoryUp_f },
	{ "zoomin", CG_ZoomIn_f },
	{ "zoomout", CG_ZoomOut_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weaplastused", CG_LastWeaponUsed_f },
	{ "weapnextinbank", CG_NextWeaponInBank_f },
	{ "weapprevinbank", CG_PrevWeaponInBank_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapalt", CG_AltWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "weaponbank", CG_WeaponBank_f },
	{ "itemnext", CG_NextItem_f },
	{ "itemprev", CG_PrevItem_f },
	{ "item", CG_Item_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },

	{ "camera", CG_Camera_f },   // duffy
	{ "fade", CG_Fade_f },   // duffy

};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand()
{
	const char* cmd = CG_Argv( 0 );

	for (int i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands()
{
	for (int i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		Cmd_AddCommand( commands[i].cmd, NULL );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	Cmd_AddCommand( "kill", NULL );
	Cmd_AddCommand( "say", NULL );
	Cmd_AddCommand( "say_team", NULL );

	Cmd_AddCommand( "tell", NULL );
	Cmd_AddCommand( "give", NULL );
	Cmd_AddCommand( "god", NULL );
	Cmd_AddCommand( "notarget", NULL );
	Cmd_AddCommand( "noclip", NULL );
	Cmd_AddCommand( "team", NULL );
	Cmd_AddCommand( "follow", NULL );
	Cmd_AddCommand( "levelshot", NULL );
	Cmd_AddCommand( "addbot", NULL );
	Cmd_AddCommand( "setviewpos", NULL );
	Cmd_AddCommand( "callvote", NULL );
	Cmd_AddCommand( "vote", NULL );
	Cmd_AddCommand( "stats", NULL );
	Cmd_AddCommand( "loaddeferred", NULL );        // spelling fixed (SA)

	Cmd_AddCommand( "startCamera", NULL );
	Cmd_AddCommand( "stopCamera", NULL );
	Cmd_AddCommand( "setCameraOrigin", NULL );

	// Rafael
	Cmd_AddCommand( "nofatigue", NULL );

	Cmd_AddCommand( "setspawnpt", NULL );          // NERVE - SMF
}
