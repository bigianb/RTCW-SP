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

// Copyright (C) 1999-2000 Id Software, Inc.
//

// g_public.h -- game module information visible to server

#pragma once

#define GAME_API_VERSION    8

// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define SVF_NOCLIENT            0x00000001  // don't send entity to clients, even if it has effects
#define SVF_VISDUMMY            0x00000004  // this ent is a "visibility dummy" and needs it's master to be sent to clients that can see it even if they can't see the master ent
#define SVF_BOT                 0x00000008
// Wolfenstein
#define SVF_CASTAI              0x00000010
// done.
#define SVF_BROADCAST           0x00000020  // send to all connected clients
#define SVF_PORTAL              0x00000040  // merge a second pvs at origin2 into snapshots
#define SVF_USE_CURRENT_ORIGIN  0x00000080  // entity->r.currentOrigin instead of entity->s.origin
											// for link position (missiles and movers)
// Ridah
#define SVF_NOFOOTSTEPS         0x00000100
// done.
// MrE:
#define SVF_CAPSULE             0x00000200  // use capsule for collision detection

#define SVF_VISDUMMY_MULTIPLE   0x00000400  // so that one vis dummy can add to snapshot multiple speakers

// recent id changes
#define SVF_SINGLECLIENT        0x00000800  // only send to a single client (entityShared_t->singleClient)
#define SVF_NOSERVERINFO        0x00001000  // don't send CS_SERVERINFO updates to this client
											// so that it can be updated for ping tools without
											// lagging clients
#define SVF_NOTSINGLECLIENT     0x00002000  // send entity to everyone but one client
											// (entityShared_t->singleClient)

//===============================================================


typedef struct {
	EntityState s;                // communicated by server to clients

	bool linked;                // false if not in any good cluster
	int linkcount;

	int svFlags;                    // SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;               // only send to this client when SVF_SINGLECLIENT is set

	bool bmodel;                // if false, assume an explicit mins / maxs bounding box
									// only set by SV_SetBrushModel
	vec3_t mins, maxs;
	int contents;                   // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;          // derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;
} entityShared_t;



// the server looks at a sharedEntity, which is the start of the game's GameEntity structure
typedef struct {
	EntityState s;                // communicated by server to clients
	entityShared_t r;               // shared by both the server system and game
} sharedEntity_t;


//
// functions exported by the game subsystem
//
typedef enum {
	GAME_INIT,  // ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	GAME_SHUTDOWN,  // (void);

	GAME_CLIENT_CONNECT,    // ( int clientNum, bool firstTime, bool isBot );
	// return nullptr if the client is allowed to connect, otherwise return
	// a text string with the reason for denial

	GAME_CLIENT_BEGIN,              // ( int clientNum );

	GAME_CLIENT_USERINFO_CHANGED,   // ( int clientNum );

	GAME_CLIENT_DISCONNECT,         // ( int clientNum );

	GAME_CLIENT_COMMAND,            // ( int clientNum );

	GAME_CLIENT_THINK,              // ( int clientNum );

	GAME_RUN_FRAME,                 // ( int levelTime );

	GAME_CONSOLE_COMMAND,           // ( void );
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return false if the game doesn't recognize it as a command.

	BOTAI_START_FRAME,              // ( int time );

	// Ridah, Cast AI
	AICAST_VISIBLEFROMPOS,
	AICAST_CHECKATTACKATPOS,
	// done.

	GAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT,
	GAME_GETMODELINFO

} gameExport_t;

