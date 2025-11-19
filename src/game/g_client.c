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

#include "g_local.h"
#include "../qcommon/qcommon.h"
#include "../server/server.h"

// g_client.c -- client functions that don't happen every frame

// Ridah, new bounding box
//static vec3_t	playerMins = {-15, -15, -24};
//static vec3_t	playerMaxs = {15, 15, 32};
vec3_t playerMins = {-18, -18, -24};
vec3_t playerMaxs = {18, 18, 48};
// done.

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
If the start position is targeting an entity, the players camera will start out facing that ent (like an info_notnull)
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int i;
	vec3_t dir;

	G_SpawnInt( "nobots", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->enemy = G_PickTarget( ent->target );
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->shared.s.origin, ent->shared.s.origin, dir );
		vectoangles( dir, ent->shared.s.angles );
	}

}



/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start( gentity_t *ent ) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	vec3_t mins, maxs;

	VectorAdd( spot->shared.s.origin, playerMins, mins );
	VectorAdd( spot->shared.s.origin, playerMaxs, maxs );
	num = SV_AreaEntities( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0 ; i < num ; i++ ) {
		hit = &g_entities[touch[i]];
		if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
			return qtrue;
		}

	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define MAX_SPAWN_POINTS    128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t   *spot;
	vec3_t delta;
	float dist, nearestDist;
	gentity_t   *nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) != NULL ) {

		VectorSubtract( spot->shared.s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_SPAWN_POINTS    128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t   *spot;
	int count;
	int selection;
	gentity_t   *spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) != NULL ) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) { // no spots that won't telefrag
		return G_Find( NULL, FOFS( classname ), "info_player_deathmatch" );
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint( vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t   *spot;
	gentity_t   *nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint();
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint();
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint();
		}
	}

	// find a single player start spot
	if ( !spot ) {
		Com_Error( ERR_DROP, "Couldn't find a spawn point" );
	}

	VectorCopy( spot->shared.s.origin, origin );
	origin[2] += 9;
	VectorCopy( spot->shared.s.angles, angles );

	return spot;
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles ) {
	gentity_t   *spot;

	spot = NULL;
	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) != NULL ) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy( spot->shared.s.origin, origin );
	origin[2] += 9;
	VectorCopy( spot->shared.s.angles, angles );

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue( void ) {
	int i;
	gentity_t   *ent;

	level.bodyQueIndex = 0;
	for ( i = 0; i < BODY_QUEUE_SIZE ; i++ ) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > 6500 ) {
		// the body ques are never actually freed, they are just unlinked
		SV_UnlinkEntity( &ent->shared );
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->shared.s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t       *body;
	int contents, i;

	SV_UnlinkEntity( &ent->shared );

	// if client is in a nodrop area, don't leave the body
	contents = SV_PointContents( ent->shared.s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = ( level.bodyQueIndex + 1 ) % BODY_QUEUE_SIZE;

	SV_UnlinkEntity( body );

	body->shared.s = ent->shared.s;
	body->shared.s.eFlags = EF_DEAD;       // clear EF_TALK, etc

	if ( ent->client->ps.eFlags & EF_HEADSHOT ) {
		body->shared.s.eFlags |= EF_HEADSHOT;          // make sure the dead body draws no head (if killed that way)

	}
	body->shared.s.powerups = 0;   // clear powerups
	body->shared.s.loopSound = 0;  // clear lava burning
	body->shared.s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;        // don't bounce
	if ( body->shared.s.groundEntityNum == ENTITYNUM_NONE ) {
		body->shared.s.pos.trType = TR_GRAVITY;
		body->shared.s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->shared.s.pos.trDelta );
	} else {
		body->shared.s.pos.trType = TR_STATIONARY;
	}
	body->shared.s.event = 0;

	// DHM - Clear out event system
	for ( i = 0; i < MAX_EVENTS; i++ )
		body->shared.s.events[i] = 0;
	body->shared.s.eventSequence = 0;


	body->shared.r.svFlags = ent->shared.r.svFlags;
	VectorCopy( ent->shared.r.mins, body->shared.r.mins );
	VectorCopy( ent->shared.r.maxs, body->shared.r.maxs );
	VectorCopy( ent->shared.r.absmin, body->shared.r.absmin );
	VectorCopy( ent->shared.r.absmax, body->shared.r.absmax );

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->shared.r.contents = CONTENTS_CORPSE;
	body->shared.r.ownerNum = ent->shared.r.ownerNum;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy( body->shared.s.pos.trBase, body->shared.r.currentOrigin );
	SV_LinkEntity( body );
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int i;

	// set the delta angle
	for ( i = 0 ; i < 3 ; i++ ) {
		int cmdAngle;

		cmdAngle = ANGLE2SHORT( angle[i] );
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->shared.s.angles );
	VectorCopy( ent->shared.s.angles, ent->client->ps.viewangles );
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	gentity_t   *tent;

    if ( g_reloading.integer || saveGamePending ) {
        return;
    }

    if ( !( ent->shared.r.svFlags & SVF_CASTAI ) ) {
        // Fast method, just do a map_restart, and then load in the savegame
        // once everything is settled.
        SV_SetConfigstring( CS_SCREENFADE, va( "1 %i 4000", level.time + 2000 ) );

        Cvar_Set( "g_reloading", "1" );

        level.reloadDelayTime = level.time + 6000;

        SV_GameSendServerCommand( -1, va( "snd_fade 0 %d", 6000 ) );  // fade sound out

        return;
    }

	CopyToBodyQue( ent );

	ClientSpawn( ent );

	// add a teleportation effect
	tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
	tent->shared.s.clientNum = ent->shared.s.clientNum;
}


/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int len, colorlessLen;
	char ch;
	char    *p;
	int spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while ( 1 ) {
		ch = *in++;
		if ( !ch ) {
			break;
		}

		// don't allow leading spaces
		if ( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if ( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if ( !*in ) {
				break;
			}

			// don't allow black in a name, period
			if ( ColorIndex( *in ) == 0 ) {
				in++;
				continue;
			}

			// make sure room in dest for both chars
			if ( len == outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if ( ch == ' ' ) {
			spaces++;
			if ( spaces > 3 ) {
				continue;
			}
		} else {
			spaces = 0;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
		if ( len == outSize - 1 ) {
			break;
		}
	}
	*out = 0;

	// don't allow empty names
	if ( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( out, "UnnamedPlayer", outSize );
	}
}

/*
==================
G_CheckForExistingModelInfo

  If this player model has already been parsed, then use the existing information.
  Otherwise, set the modelInfo pointer to the first free slot.

  returns qtrue if existing model found, qfalse otherwise
==================
*/
qboolean G_CheckForExistingModelInfo( gclient_t *cl, char *modelName, animModelInfo_t **modelInfo ) {
	int i;
	animModelInfo_t *trav;

	for ( i = 0; i < MAX_ANIMSCRIPT_MODELS; i++ ) {
		trav = level.animScriptData.modelInfo[i];
		if ( trav && trav->modelname[0] ) {
			if ( !Q_stricmp( trav->modelname, modelName ) ) {
				// found a match, use this modelinfo
				*modelInfo = trav;
				level.animScriptData.clientModels[cl->ps.clientNum] = i + 1;
				return qtrue;
			}
		} else {
			level.animScriptData.modelInfo[i] = G_Alloc( sizeof( animModelInfo_t ) );
			*modelInfo = level.animScriptData.modelInfo[i];
			// clear the structure out ready for use
			memset( *modelInfo, 0, sizeof( **modelInfo ) );
			level.animScriptData.clientModels[cl->ps.clientNum] = i + 1;
			return qfalse;
		}
	}

	Com_Error( ERR_DROP, "unable to find a free modelinfo slot, cannot continue\n" );
	// qfalse signifies that we need to parse the information from the script files
	return qfalse;
}

/*
==============
G_GetModelInfo
==============
*/
qboolean G_ParseAnimationFiles( char *modelname, gclient_t *cl );
qboolean G_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo ) {

	if ( !G_CheckForExistingModelInfo( &level.clients[clientNum], modelName, modelInfo ) ) {
		level.clients[clientNum].modelInfo = *modelInfo;
		if ( !G_ParseAnimationFiles( modelName, &level.clients[clientNum] ) ) {
			Com_Error( ERR_DROP, "Failed to load animation scripts for model %s\n", modelName );
		}
	}

	return qtrue;
}

/*
=============
G_ParseAnimationFiles
=============
*/
qboolean G_ParseAnimationFiles( char *modelname, gclient_t *cl ) {
	char text[100000];
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len;

	// set the name of the model in the modelinfo structure
	Q_strncpyz( cl->modelInfo->modelname, modelname, sizeof( cl->modelInfo->modelname ) );

	// load the cfg file
	snprintf( filename, sizeof( filename ), "models/players/%s/wolfanim.cfg", modelname );
	len = FS_FOpenFileByMode( filename, &f, FS_READ );
	if ( len <= 0 ) {
		Com_Printf( "G_ParseAnimationFiles(): file '%s' not found\n", filename );       //----(SA)	added
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		Com_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	FS_Read( text, len, f );
	text[len] = 0;
	FS_FCloseFile( f );

	// parse the text
	BG_AnimParseAnimConfig( cl->modelInfo, filename, text );

	// load the script file
	snprintf( filename, sizeof( filename ), "models/players/%s/wolfanim.script", modelname );
	len = FS_FOpenFileByMode( filename, &f, FS_READ );
	if ( len <= 0 ) {
		if ( cl->modelInfo->version > 1 ) {
			return qfalse;
		}
		// try loading the default script for old legacy models
		snprintf( filename, sizeof( filename ), "models/players/default.script", modelname );
		len = FS_FOpenFileByMode( filename, &f, FS_READ );
		if ( len <= 0 ) {
			return qfalse;
		}
	}
	if ( len >= sizeof( text ) - 1 ) {
		Com_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	FS_Read( text, len, f );
	text[len] = 0;
	FS_FCloseFile( f );

	// parse the text
	BG_AnimParseAnimScript( cl->modelInfo, &level.animScriptData, cl->ps.clientNum, filename, text );

	// ask the client to send us the movespeeds if available
	if ( g_entities[0].client && g_entities[0].client->pers.connected == CON_CONNECTED ) {
		SV_GameSendServerCommand( 0, va( "mvspd %s", modelname ) );
	}

	return qtrue;
}


/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call SV_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	char    *s;
	char model[MAX_QPATH], modelname[MAX_QPATH];

//----(SA) added this for head separation
	char head[MAX_QPATH];

	char oldname[MAX_STRING_CHARS];
	gclient_t   *client;
	char    *c1;
	char userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	client->ps.clientNum = clientNum;

	SV_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate( userinfo ) ) {
		strcpy( userinfo, "\\name\\badinfo" );
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// check the auto activation
	s = Info_ValueForKey( userinfo, "cg_autoactivate" );
	if ( !atoi( s ) ) {
		client->pers.autoActivate = PICKUP_ACTIVATE;
	} else {
		client->pers.autoActivate = PICKUP_TOUCH;
	}

	// check the auto empty weapon switching
	s = Info_ValueForKey( userinfo, "cg_emptyswitch" );
	if ( !atoi( s ) ) {
		client->pers.emptySwitch = 0;
	} else {
		client->pers.emptySwitch = 1;
	}

	// set name
	Q_strncpyz( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey( userinfo, "name" );
	ClientCleanName( s, client->pers.netname, sizeof( client->pers.netname ) );

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) ) {
			SV_GameSendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname,
											client->pers.netname ) );
		}
	}

	// set max health
	client->pers.maxHealth = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
		client->pers.maxHealth = 100;
	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	if ( g_forceModel.integer ) {
		Q_strncpyz( model, DEFAULT_MODEL, sizeof( model ) );
		Q_strcat( model, sizeof( model ), "/default" );
	} else {
		Q_strncpyz( model, Info_ValueForKey( userinfo, "model" ), sizeof( model ) );
	}

	// RF, reset anims so client's dont freak out
	client->ps.legsAnim = 0;
	client->ps.torsoAnim = 0;


	// strip the skin name
	Q_strncpyz( modelname, model, sizeof( modelname ) );
	if ( strstr( modelname, "/" ) ) {
		modelname[ strstr( modelname, "/" ) - modelname ] = 0;
	} else if ( strstr( modelname, "\\" ) ) {
		modelname[ strstr( modelname, "\\" ) - modelname ] = 0;
	}

	if ( !G_CheckForExistingModelInfo( client, modelname, &client->modelInfo ) ) {
		if ( !G_ParseAnimationFiles( modelname, client ) ) {
			Com_Error( ERR_DROP, "Failed to load animation scripts for model %s\n", modelname );
		}
	}


//----(SA) added this for head separation
	// set head
	if ( g_forceModel.integer ) {
		Q_strncpyz( head, DEFAULT_HEAD, sizeof( head ) );
	} else {
		Q_strncpyz( head, Info_ValueForKey( userinfo, "head" ), sizeof( head ) );
	}

//----(SA) end

	// colors
	c1 = Info_ValueForKey( userinfo, "color" );

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds

//----(SA) modified these for head separation

	if ( ent->shared.r.svFlags & SVF_BOT ) {

		s = va( "n\\%s\\t\\%i\\model\\%s\\head\\%s\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s",
				client->pers.netname, client->sess.sessionTeam, model, head, c1,
				client->pers.maxHealth, client->sess.wins, client->sess.losses,
				Info_ValueForKey( userinfo, "skill" ) );
	} else {
		s = va( "n\\%s\\t\\%i\\model\\%s\\head\\%s\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i",
				client->pers.netname, client->sess.sessionTeam, model, head, c1,
				client->pers.maxHealth, client->sess.wins, client->sess.losses );
	}

//----(SA) end

	SV_SetConfigstring( CS_PLAYERS + clientNum, s );

	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
}

void calcLevelClients()
{
	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;

	for (int i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;
			level.numNonSpectatorClients++;
			if ( level.clients[i].pers.connected == CON_CONNECTED ) {
				level.numPlayingClients++;
			}
		}
	}
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char        *value;
	gclient_t   *client;
	char userinfo[MAX_INFO_STRING];
	gentity_t   *ent;

	ent = &g_entities[ clientNum ];

	SV_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check to see if they are on the banned IP list
	value = Info_ValueForKey( userinfo, "ip" );

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset( client, 0, sizeof( *client ) );

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime ) {
		G_InitSessionData( client, userinfo );
	}
	G_ReadSessionData( client );

	if ( isBot ) {
		ent->shared.r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if ( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		// Ridah
		if ( !(ent->shared.r.svFlags & SVF_CASTAI) ) {
			// done.
			SV_GameSendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname ) );
		}
	}

	calcLevelClients();
	
	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum ) {
	gentity_t   *ent;
	gclient_t   *client;
	gentity_t   *tent;
	int flags;
	int spawn_count;                // DHM - Nerve

	ent = g_entities + clientNum;

	if ( ent->botDelayBegin ) {
		G_QueueBotBegin( clientNum );
		ent->botDelayBegin = qfalse;
		return;
	}

	client = level.clients + clientNum;

	if ( ent->shared.r.linked ) {
		SV_UnlinkEntity( &ent->shared );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	// DHM - Nerve :: Also save PERS_SPAWN_COUNT, so that CG_Respawn happens
	spawn_count = client->ps.persistant[PERS_SPAWN_COUNT];
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWN_COUNT] = spawn_count;

	// MrE: use capsule for collision
	client->ps.eFlags |= EF_CAPSULE;
	ent->shared.r.svFlags |= SVF_CAPSULE;

	// locate ent at a spawn point
	ClientSpawn( ent );

	// Ridah, trigger a spawn event
	if ( !( ent->shared.r.svFlags & SVF_CASTAI ) ) {
		AICast_ScriptEvent( AICast_GetCastState( clientNum ), "spawn", "" );
	}

	{
		// send event
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->shared.s.clientNum = ent->shared.s.clientNum;

		// Ridah
		if ( !(ent->shared.r.svFlags & SVF_CASTAI) ) {
			// done.
			SV_GameSendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname ) );
		}
		
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn( gentity_t *ent ) {
	int index;
	vec3_t spawn_origin, spawn_angles;
	gclient_t   *client;
	int i;
	clientPersistant_t saved;
	clientSession_t savedSess;
	int persistant[MAX_PERSISTANT];
	gentity_t   *spawnPoint;
	int flags;
	int savedPing;
	int savedTeam;

	index = ent - g_entities;
	client = ent->client;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client

	// Ridah
	if ( ent->shared.r.svFlags & SVF_CASTAI ) {
		spawnPoint = ent;
		VectorCopy( ent->shared.s.origin, spawn_origin );
		spawn_origin[2] += 9;   // spawns seem to be sunk into ground?
		VectorCopy( ent->shared.s.angles, spawn_angles );
	} else
	{
		ent->aiName = "player";  // needed for script AI
		ent->aiTeam = 1;        // member of allies
		ent->client->ps.teamNum = ent->aiTeam;
		AICast_ScriptParse( AICast_GetCastState( ent->shared.s.number ) );
		// done.

		
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint(
					client->ps.origin,
					spawn_origin, spawn_angles );
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->shared.r.svFlags & SVF_BOT ) ) {
				continue;   // try again
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->shared.r.svFlags & SVF_BOT ) ) {
				continue;   // try again
			}

			break;

		} while ( 1 );
		

		// Ridah
	}
	// done.

	client->pers.teamState.state = TEAM_ACTIVE;


	// toggle the teleport bit so the client knows to not lerp
	flags = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;

	savedTeam = client->ps.teamNum;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	memset( client, 0, sizeof( *client ) );

	client->pers = saved;
	client->sess = savedSess;

	client->ps.teamNum = savedTeam;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;
	// MrE: use capsules for AI and player
	client->ps.eFlags |= EF_CAPSULE;

	ent->shared.s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	if ( !( ent->shared.r.svFlags & SVF_CASTAI ) ) {
		ent->classname = "player";
	}
	ent->shared.r.contents = CONTENTS_BODY;

	// RF, AI should be clipped by monsterclip brushes
	if ( ent->shared.r.svFlags & SVF_CASTAI ) {
		ent->clipmask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
	} else {
		ent->clipmask = MASK_PLAYERSOLID;
	}

	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy( playerMins, ent->shared.r.mins );
	VectorCopy( playerMaxs, ent->shared.r.maxs );

	// Ridah, setup the bounding boxes and viewheights for prediction
	VectorCopy( ent->shared.r.mins, client->ps.mins );
	VectorCopy( ent->shared.r.maxs, client->ps.maxs );

	client->ps.crouchViewHeight = CROUCH_VIEWHEIGHT;
	client->ps.standViewHeight = DEFAULT_VIEWHEIGHT;
	client->ps.deadViewHeight = DEAD_VIEWHEIGHT;

	client->ps.crouchMaxZ = client->ps.maxs[2] - ( client->ps.standViewHeight - client->ps.crouchViewHeight );

	client->ps.runSpeedScale = 0.8;
	client->ps.sprintSpeedScale = 1.1;
	client->ps.crouchSpeedScale = 0.25;
	client->ps.sprintTime = 20000;
	client->ps.sprintExertTime = 0;
	client->ps.friction = 1.0;
	client->ps.clientNum = index;

    ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	SV_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	G_KillBox( ent );
	SV_LinkEntity( &ent->shared );

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;
	client->latched_wbuttons = 0;


	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );

	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent - g_entities );

	// positively link the client, even if the command times are weird
	{
		BG_PlayerStateToEntityState( &client->ps, &ent->shared.s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->shared.r.currentOrigin );
		SV_LinkEntity( &ent->shared );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->shared.s, qtrue );
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call SV_GameDropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t   *ent;
	gentity_t   *tent;
	int i;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	// Ridah
	if ( !( ent->shared.r.svFlags & SVF_CASTAI ) ) {
		// done.

		// send effect if they were completely connected
		if ( ent->client->pers.connected == CON_CONNECTED) {
			tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
			tent->shared.s.clientNum = ent->shared.s.clientNum;

			// They don't get to take powerups with them!
			// Especially important for stuff like CTF flags
			TossClientItems( ent );
		}

		G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

		// Ridah
	}
	// done.

	SV_UnlinkEntity( &ent->shared );
	ent->shared.s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	SV_SetConfigstring( CS_PLAYERS + clientNum, "" );

	if ( ent->shared.r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum );
	}
}


/*
==================
G_RetrieveMoveSpeedsFromClient
==================
*/
void G_RetrieveMoveSpeedsFromClient( int entnum, char *text ) {
	char *text_p, *token;
	animation_t *anim;
	animModelInfo_t *modelInfo;

	text_p = text;

	// get the model name
	token = COM_Parse( &text_p );
	if ( !token || !token[0] ) {
		Com_Error( ERR_DROP, "G_RetrieveMoveSpeedsFromClient: internal error" );
	}

	modelInfo = BG_ModelInfoForModelname( token );

	if ( !modelInfo ) {
		// ignore it
		return;
	}

	while ( 1 ) {
		token = COM_Parse( &text_p );
		if ( !token || !token[0] ) {
			break;
		}

		// this is a name
		anim = BG_AnimationForString( token, modelInfo );
		if ( anim->moveSpeed == 0 ) {
			Com_Error( ERR_DROP, "G_RetrieveMoveSpeedsFromClient: trying to set movespeed for non-moving animation" );
		}

		// get the movespeed
		token = COM_Parse( &text_p );
		if ( !token || !token[0] ) {
			Com_Error( ERR_DROP, "G_RetrieveMoveSpeedsFromClient: missing movespeed" );
		}
		anim->moveSpeed = atoi( token );

		// get the stepgap
		token = COM_Parse( &text_p );
		if ( !token || !token[0] ) {
			Com_Error( ERR_DROP, "G_RetrieveMoveSpeedsFromClient: missing stepGap" );
		}
		anim->stepGap = atoi( token );
	}
}
