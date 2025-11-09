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
/*
==================
CheatsOk
==================
*/
qboolean    CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		SV_GameSendServerCommand( ent - g_entities, va( "print \"Cheats are not enabled on this server.\n\"" ) );
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		SV_GameSendServerCommand( ent - g_entities, va( "print \"You must be alive to use this command.\n\"" ) );
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char    *ConcatArgs( int start ) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = Cmd_Argc();
	for ( i = start ; i < c ; i++ ) {
		Cmd_ArgvBuffer( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;        // skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t   *cl;
	int idnum;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			SV_GameSendServerCommand( to - g_entities, va( "print \"Bad client slot: %i\n\"", idnum ) );
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			SV_GameSendServerCommand( to - g_entities, va( "print \"Client %i is not active\n\"", idnum ) );
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum = 0,cl = level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	SV_GameSendServerCommand( to - g_entities, va( "print \"User %s is not on the server\n\"", s ) );
	return -1;
}



//----(SA)	added
/*
==============
G_setfog
==============
*/
void G_setfog( char *fogstring ) {
	SV_SetConfigstring( CS_FOGVARS, fogstring );
}

/*
==============
Cmd_Fogswitch_f
==============
*/
void Cmd_Fogswitch_f( void ) {
	G_setfog( ConcatArgs( 1 ) );
}

//----(SA)	end

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( gentity_t *ent ) {
	char        *name, *amt;
	gitem_t     *it;
	int i;
	qboolean give_all;
	gentity_t       *it_ent;
	trace_t trace;
	int amount;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	//----(SA)	check for an amount (like "give health 30")
	amt = ConcatArgs( 2 );
	amount = atoi( amt );
	//----(SA)	end

	name = ConcatArgs( 1 );

	if ( !name || !strlen( name ) ) {
		return;
	}

	if ( Q_stricmp( name, "all" ) == 0 ) {
		give_all = qtrue;
	} else {
		give_all = qfalse;
	}


	if ( give_all || Q_stricmpn( name, "health", 6 ) == 0 ) {
		//----(SA)	modified
		if ( amount ) {
			ent->health += amount;
		} else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "weapons" ) == 0 ) {
		//ent->client->ps.weapons[0] = (1 << (WP_MONSTER_ATTACK1)) - 1 - (1<<WP_NONE);	//----(SA)	gives the cross now as well

		//(SA) we really don't want to give anything beyond WP_DYNAMITE
		for ( i = 0; i <= WP_DYNAMITE; i++ )
			COM_BitSet( ent->client->ps.weapons, i );

//		for (i=0; i<WP_NUM_WEAPONS; i++) {
//			switch (i) {
//			case WP_MONSTER_ATTACK1:
//			case WP_MONSTER_ATTACK2:
//			case WP_MONSTER_ATTACK3:
//			case WP_NONE:
//				break;
//			default:
//				COM_BitSet( ent->client->ps.weapons, i );
//			}
//		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "holdable" ) == 0 ) {
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = ( 1 << ( HI_BOOK3 - 1 ) ) - 1 - ( 1 << HI_NONE );
		for ( i = 1 ; i <= HI_BOOK3 ; i++ ) {
			ent->client->ps.holdable[i] = 10;
		}

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "ammo", 4 ) == 0 ) {
		if ( amount ) {
			if ( ent->client->ps.weapon ) {
				Add_Ammo( ent, ent->client->ps.weapon, amount, qtrue );
			}
		} else {
			for ( i = 1 ; i < WP_MONSTER_ATTACK1 ; i++ )
				Add_Ammo( ent, i, 999, qtrue );
		}

		if ( !give_all ) {
			return;
		}
	}

	//	"give allammo <n>" allows you to give a specific amount of ammo to /all/ weapons while
	//	allowing "give ammo <n>" to only give to the selected weap.
	if ( Q_stricmpn( name, "allammo", 7 ) == 0 && amount ) {
		for ( i = 1 ; i < WP_MONSTER_ATTACK1 ; i++ )
			Add_Ammo( ent, i, amount, qtrue );

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "armor", 5 ) == 0 ) {
		//----(SA)	modified
		if ( amount ) {
			ent->client->ps.stats[STAT_ARMOR] += amount;
		} else {
			ent->client->ps.stats[STAT_ARMOR] = 100;
		}
		if ( !give_all ) {
			return;
		}
	}

	//---- (SA) Wolf keys
	if ( give_all || Q_stricmp( name, "keys" ) == 0 ) {
		ent->client->ps.stats[STAT_KEYS] = ( 1 << KEY_NUM_KEYS ) - 2;
		if ( !give_all ) {
			return;
		}
	}
	//---- (SA) end

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it ) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		memset( &trace, 0, sizeof( trace ) );
		it_ent->active = qtrue;
		Touch_Item( it_ent, ent, &trace );
		it_ent->active = qfalse;
		if ( it_ent->inuse ) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if ( !( ent->flags & FL_GODMODE ) ) {
		msg = "godmode OFF\n";
	} else {
		msg = "godmode ON\n";
	}

	SV_GameSendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
==================
Cmd_Nofatigue_f

Sets client to nofatigue

argv(0) nofatigue
==================
*/

void Cmd_Nofatigue_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOFATIGUE;
	if ( !( ent->flags & FL_NOFATIGUE ) ) {
		msg = "nofatigue OFF\n";
	} else {
		msg = "nofatigue ON\n";
	}

	SV_GameSendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if ( !( ent->flags & FL_NOTARGET ) ) {
		msg = "notarget OFF\n";
	} else {
		msg = "notarget ON\n";
	}

	SV_GameSendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	SV_GameSendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	
	if ( g_reloading.integer ) {
		return;
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	player_die( ent, ent, ent, 100000, MOD_SUICIDE );
}

/*
=================
SetWolfData
=================
*/
void SetWolfData( gentity_t *ent, char *ptype, char *weap, char *pistol, char *grenade, char *skinnum ) {   // DHM - Nerve
	gclient_t   *client;

	client = ent->client;

	client->sess.playerType = atoi( ptype );
	client->sess.playerWeapon = atoi( weap );
	client->sess.playerPistol = atoi( pistol );
	client->sess.playerItem = atoi( grenade );
	client->sess.playerSkin = atoi( skinnum );
}
// dhm - end

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}


/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int i;
	char arg[MAX_TOKEN_CHARS];

	if ( Cmd_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	Cmd_ArgvBuffer( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum;
	int original;

	if ( dir != 1 && dir != -1 ) {
		Com_Error( ERR_DROP, "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}


		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	SV_GameSendServerCommand( ent - g_entities, va( "print \"%s\n\"", vtos( ent->s.origin ) ) );
}



qboolean G_canPickupMelee( gentity_t *ent ) {

	if ( !( ent->client ) ) {
		return qfalse;  // hmm, shouldn't be too likely...

	}
	if ( !( ent->s.weapon ) ) {  // no weap, go ahead
		return qtrue;
	}

	if ( ent->client->ps.weaponstate == WEAPON_RELOADING ) {
		return qfalse;
	}

	if ( WEAPS_ONE_HANDED & ( 1 << ( ent->client->pers.cmd.weapon ) ) ) {
		return qtrue;
	}

	return qfalse;
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( !g_cheats.integer ) {
		SV_GameSendServerCommand( ent - g_entities, va( "print \"Cheats are not enabled on this server.\n\"" ) );
		return;
	}
	if ( Cmd_Argc() != 5 ) {
		SV_GameSendServerCommand( ent - g_entities, va( "print \"usage: setviewpos x y z yaw\n\"" ) );
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		Cmd_ArgvBuffer( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	Cmd_ArgvBuffer( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f( gentity_t *ent ) {
	g_camEnt->r.svFlags |= SVF_PORTAL;
	g_camEnt->r.svFlags &= ~SVF_NOCLIENT;
	ent->client->cameraPortal = g_camEnt;
	ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
	ent->s.eFlags |= EF_VIEWING_CAMERA;

}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f( gentity_t *ent ) {
	gentity_t *sp;

	if ( ent->client->cameraPortal ) {
		// send a script event
		G_Script_ScriptEvent( ent->client->cameraPortal, "stopcam", "" );
		// go back into noclient mode
		ent->client->cameraPortal->r.svFlags |= SVF_NOCLIENT;
		ent->client->cameraPortal = NULL;
		ent->s.eFlags &= ~EF_VIEWING_CAMERA;
		ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;

		// RF, if we are near the spawn point, save the "current" game, for reloading after death
		sp = NULL;
		// gcc: suggests () around assignment used as truth value
		while ( ( sp = G_Find( sp, FOFS( classname ), "info_player_deathmatch" ) ) ) { // info_player_start becomes info_player_deathmatch in it's spawn functon
			if ( Distance( ent->s.pos.trBase, sp->s.origin ) < 256 && SV_inPVS( ent->s.pos.trBase, sp->s.origin ) ) {
				G_SaveGame( NULL );
				break;
			}
		}
	}
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( Cmd_Argc() != 4 ) {
		return;
	}

	VectorClear( ent->client->cameraOrigin );
	for ( i = 0 ; i < 3 ; i++ ) {
		Cmd_ArgvBuffer( i + 1, buffer, sizeof( buffer ) );
		ent->client->cameraOrigin[i] = atof( buffer );
	}
}


/*
==============
Cmd_InterruptCamera_f
==============
*/
void Cmd_InterruptCamera_f( gentity_t *ent ) {
	AICast_ScriptEvent( AICast_GetCastState( ent->s.number ), "trigger", "cameraInterrupt" );
}

/*
==============
G_ThrowChair
==============
*/
qboolean G_ThrowChair( gentity_t *ent, vec3_t dir, qboolean force ) {
	trace_t trace;
	vec3_t mins, maxs;
//	vec3_t		forward;
	vec3_t start, end;
	qboolean isthrown = qtrue;
	gentity_t   *traceEnt;

	if ( !ent->active || !ent->melee ) {
		return qfalse;
	}

	VectorCopy( ent->r.mins, mins );
	VectorCopy( ent->r.maxs, maxs );

//	AngleVectors (ent->r.currentAngles, forward, NULL, NULL);
	VectorCopy( ent->r.currentOrigin, start );

	start[2] += 24;
	VectorMA( start, 17, dir, start );
//	start[2] += 24;

	VectorCopy( start, end );
	VectorMA( end, 32, dir, end );

	SV_Trace( &trace, start, mins, maxs, end, ent->s.number, MASK_SOLID | MASK_MISSILESHOT, qfalse );

	traceEnt = &g_entities[ trace.entityNum ];

	if ( trace.startsolid ) {
		isthrown = qfalse;
	}

	if ( trace.fraction != 1 ) {
		isthrown = qfalse;
	}

	if ( isthrown || force ) {
		// successful drop
		traceEnt->active = qfalse;

		ent->melee = NULL;
		ent->active = qfalse;
		ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
//		ent->s.eFlags &= ~EF_MELEE_ACTIVE;
	}

	if ( !isthrown && force ) {    // was not successfully thrown, but you /need/ to drop it.  break it.
		G_Damage( traceEnt, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );    // Die!
	}

	return ( isthrown || force );
}


// Rafael
/*
==================
Cmd_Activate_f
==================
*/
void Cmd_Activate_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
	static int oldactivatetime = 0;
	int activatetime = level.time;
	qboolean walking = qfalse;

	if ( ent->client->pers.cmd.buttons & BUTTON_WALKING ) {
		walking = qtrue;
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePointForActivate( ent, forward, right, up, offset );

	VectorMA( offset, 96, forward, end );

	SV_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ), qfalse );

	//----(SA)	removed erroneous code

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// Com_Printf( "%s activate %s\n", ent->classname, traceEnt->classname);

	// Ridah, check for using a friendly AI
	if ( traceEnt->aiCharacter ) {
		AICast_Activate( ent->s.number, traceEnt->s.number );
		return;
	}

	if ( traceEnt->classname ) {
		traceEnt->flags &= ~FL_SOFTACTIVATE;    // FL_SOFTACTIVATE will be set if the user is holding his 'walk' key down when activating things

		if ( ( ( Q_stricmp( traceEnt->classname, "func_door" ) == 0 ) || ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 ) ) ) {
//----(SA)	modified
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_TryDoor( traceEnt, ent, ent );      // (door,other,activator)
//----(SA)	end
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 )
					&& ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
					&& traceEnt->active == qfalse ) {
			G_TryDoor( traceEnt, ent, ent );      // (door,other,activator)
//			Use_BinaryMover (traceEnt, ent, ent);
//			traceEnt->active = qtrue;
		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) )     {
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "props_footlocker" ) )     {
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "script_mover" ) )     {
			G_Script_ScriptEvent( traceEnt, "activate", ent->aiName );
		} else if ( traceEnt->s.eType == ET_ALARMBOX )     {
			trace_t trace;
			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->use ) {
				traceEnt->use( traceEnt, ent, 0 );
			}
		} else if ( traceEnt->s.eType == ET_ITEM )     {
			trace_t trace;

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->touch ) {
				if ( ent->client->pers.autoActivate == PICKUP_ACTIVATE ) {
					ent->client->pers.autoActivate = PICKUP_FORCE;      //----(SA) force the pickup of a normally autoactivate only item
				}
				traceEnt->active = qtrue;
				traceEnt->touch( traceEnt, ent, &trace );
			}

		} else if ( ( Q_stricmp( traceEnt->classname, "misc_mg42" ) == 0 ) /*&& activatetime > oldactivatetime + 1000*/ && traceEnt->active == qfalse )         {
			if ( !ent->active && traceEnt->takedamage ) {  // not a dead gun
				// RF, dont allow activating MG42 if crouching
				if ( !( ent->client->ps.pm_flags & PMF_DUCKED ) && !infront( traceEnt, ent ) ) {
					gclient_t   *cl;
					cl = &level.clients[ ent->s.clientNum ];

					// no mounting while using a scoped weap
					switch ( cl->ps.weapon ) {
					case WP_SNIPERRIFLE:
					case WP_SNOOPERSCOPE:
					case WP_FG42SCOPE:
						return;

					default:
						break;
					}

					if ( !( cl->ps.grenadeTimeLeft ) ) { // make sure the client isn't holding a hot potato
						traceEnt->active = qtrue;
						ent->active = qtrue;
						traceEnt->r.ownerNum = ent->s.number;
						VectorCopy( traceEnt->s.angles, traceEnt->TargetAngles );

						if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
							G_UseTargets( traceEnt, ent );   //----(SA)	added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)

						}
						return; // avoid dropping down to below, where we get thrown straight off again (AI)
					}
				}
			}
		} else if ( ( Q_stricmp( traceEnt->classname, "misc_flak" ) == 0 ) /*&& activatetime > oldactivatetime + 1000*/ && traceEnt->active == qfalse )         {
			if ( !infront( traceEnt, ent ) ) {     // make sure the client isn't holding a hot potato
				gclient_t   *cl;
				cl = &level.clients[ ent->s.clientNum ];
				if ( !( cl->ps.grenadeTimeLeft ) ) {
					traceEnt->active = qtrue;
					ent->active = qtrue;
					traceEnt->r.ownerNum = ent->s.number;
					// Rafael fix for wierd mg42 movement
					VectorCopy( traceEnt->s.angles, traceEnt->TargetAngles );
				}
			}
		}
		// chairs
		else if ( traceEnt->isProp && traceEnt->takedamage && traceEnt->s.pos.trType == TR_STATIONARY && !traceEnt->nopickup ) {
			if ( !ent->active ) {
				if ( traceEnt->active ) {
					// ?
					traceEnt->active = qfalse;
				} else

				// pickup item
				{
					// only allow if using a 'one-handed' weapon
					if ( G_canPickupMelee( ent ) ) {
						traceEnt->active = qtrue;
						traceEnt->r.ownerNum = ent->s.number;
						ent->active = qtrue;
						ent->melee = traceEnt;
						ent->client->ps.eFlags |= EF_MELEE_ACTIVE;
//						ent->s.eFlags |= EF_MELEE_ACTIVE;
					}
				}
			}
		}

	}

	if ( ent->active ) {

		if ( ent->client->ps.persistant[PERS_HWEAPON_USE] ) {
			// we wish to dismount mg42
			ent->active = 2;

		} else if ( ent->melee ) {
			// throw chair
			if ( ( tr.fraction == 1 ) || ( !( traceEnt->r.contents & CONTENTS_SOLID ) ) ) {
				G_ThrowChair( ent, forward, qfalse );
			}

		} else {
			ent->active = qfalse;
		}
	}


	if ( activatetime > oldactivatetime + 1000 ) {
		oldactivatetime = activatetime;
	}
}

// Rafael WolfKick
//===================
//	Cmd_WolfKick
//===================

#define WOLFKICKDISTANCE    96
int Cmd_WolfKick_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
	gentity_t   *tent;
	static int oldkicktime = 0;
	int kicktime = level.time;
	qboolean solidKick = qfalse;    // don't play "hit" sound on a trigger unless it's an func_invisible_user

	int damage = 15;

	if ( ent->client->ps.leanf ) {
		return 0;   // no kick when leaning

	}
	if ( oldkicktime > kicktime ) {
		return ( 0 );
	} else {
		oldkicktime = kicktime + 1000;
	}

	// play the anim
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_KICK, qfalse, qtrue );

	ent->client->ps.persistant[PERS_WOLFKICK] = 1;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePointForActivate( ent, forward, right, up, offset );

	// note to self: we need to determine the usable distance for wolf
	VectorMA( offset, WOLFKICKDISTANCE, forward, end );

	SV_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ), qfalse );

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.fraction == 1.0 ) {
		tent = G_TempEntity( tr.endpos, EV_WOLFKICK_MISS );
		tent->s.eventParm = ent->s.number;
		return ( 1 );
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !ent->melee ) { // because we dont want you to open a door with a prop
		if ( ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 )
			 && ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
			 && traceEnt->active == qfalse ) {
//			if(traceEnt->key < 0) {	// door force locked
			if ( traceEnt->key >= KEY_LOCKED_TARGET ) {    // door force locked

				//----(SA)	play kick "hit" sound
				tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
				tent->s.otherEntityNum = ent->s.number;	\
				//----(SA)	end

				AICast_AudibleEvent( ent->s.clientNum, tr.endpos, HEAR_RANGE_DOOR_KICKLOCKED ); // "someone kicked a locked door near me!"

				G_AddEvent( traceEnt, EV_GENERAL_SOUND, traceEnt->soundPos3 );

				return 1;   //----(SA)	changed.  shows boot for locked doors
			}

//			if(traceEnt->key > 0) {	// door requires key
			if ( traceEnt->key > KEY_NONE && traceEnt->key < KEY_NUM_KEYS ) {
				gitem_t *item = BG_FindItemForKey( traceEnt->key, 0 );
				if ( !( ent->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
					//----(SA)	play kick "hit" sound
					tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
					tent->s.otherEntityNum = ent->s.number;	\
					//----(SA)	end

					AICast_AudibleEvent( ent->s.clientNum, tr.endpos, HEAR_RANGE_DOOR_KICKLOCKED ); // "someone kicked a locked door near me!"

					// player does not have key
					G_AddEvent( traceEnt, EV_GENERAL_SOUND, traceEnt->soundPos3 );

					return 1;   //----(SA)	changed.  shows boot animation for locked doors
				}
			}

			if ( traceEnt->teammaster && traceEnt->team && traceEnt != traceEnt->teammaster ) {
				traceEnt->teammaster->active = qtrue;
				traceEnt->teammaster->flags |= FL_KICKACTIVATE;
				Use_BinaryMover( traceEnt->teammaster, ent, ent );
				G_UseTargets( traceEnt->teammaster, ent );
			} else
			{
				traceEnt->active = qtrue;
				traceEnt->flags |= FL_KICKACTIVATE;
				Use_BinaryMover( traceEnt, ent, ent );
				G_UseTargets( traceEnt, ent );
			}
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 )
					&& ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
					&& traceEnt->active == qfalse ) {
			Use_BinaryMover( traceEnt, ent, ent );
			traceEnt->active = qtrue;

		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) )     {
			traceEnt->flags |= FL_KICKACTIVATE;     // so cell doors know they were kicked
													// It doesn't hurt to pass this along since only ent use() funcs who care about it will check.
													// However, it may become handy to put a "KICKABLE" or "NOTKICKABLE" flag on the invisible_user
			traceEnt->use( traceEnt, ent, ent );
			traceEnt->flags &= ~FL_KICKACTIVATE;    // reset

			solidKick = qtrue;  //----(SA)
		} else if ( !Q_stricmp( traceEnt->classname, "props_flippy_table" ) && traceEnt->use )       {
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "misc_mg42" ) )     {
			solidKick = qtrue;  //----(SA)	play kick hit sound
		}
	}

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, offset );

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
		if ( LogAccuracyHit( traceEnt, ent ) ) {
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		if ( traceEnt->r.contents >= 0 && ( traceEnt->r.contents & CONTENTS_TRIGGER ) && !solidKick ) {
			tent = G_TempEntity( tr.endpos, EV_WOLFKICK_MISS ); // (SA) don't play the "hit" sound if you kick most triggers
		} else {
			tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
		}


		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->s.eventParm = DirToByte( reflect );
		// done.

		// (SA) should break...
		if ( ent->melee ) {
			ent->active = qfalse;
			ent->melee->health = 0;
			ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE; // whoops, missed this one
		}
	}

	tent->s.otherEntityNum = ent->s.number;

	// try to swing chair
	if ( traceEnt->takedamage ) {

		if ( ent->melee ) {
			ent->active = qfalse;
			ent->melee->health = 0;
			ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE;

		}

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_KICKED );   //----(SA)	modified
	}

	return ( 1 );
}
// done

// NERVE - SMF
/*
============
ClientDamage
============
*/
void ClientDamage( gentity_t *clent, int entnum, int enemynum, int id ) {
	gentity_t *enemy, *ent;
	vec3_t vec;

	ent = &g_entities[entnum];

	enemy = &g_entities[enemynum];

	switch ( id ) {
	case CLDMG_DEBRIS:
		
		G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 3 + rand() % 3, DAMAGE_NO_KNOCKBACK, MOD_EXPLOSIVE );
		
		break;
	case CLDMG_SPIRIT:

		if ( enemy->aiCharacter == AICHAR_ZOMBIE ) {
			G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 6, DAMAGE_NO_KNOCKBACK, MOD_ZOMBIESPIRIT );
		} else {
			G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 8 + rand() % 4, DAMAGE_NO_KNOCKBACK, MOD_ZOMBIESPIRIT );
		}
		
		break;
	case CLDMG_BOSS1LIGHTNING:
		
		if ( ent->takedamage ) {
			VectorSubtract( ent->r.currentOrigin, enemy->r.currentOrigin, vec );
			VectorNormalize( vec );
			G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 6 + rand() % 3, 0, MOD_LIGHTNING );
		}
		break;
	case CLDMG_TESLA:
		
		if (    ( ent->aiCharacter == AICHAR_PROTOSOLDIER ) ||
				( ent->aiCharacter == AICHAR_SUPERSOLDIER ) ||
				( ent->aiCharacter == AICHAR_LOPER ) ||
				( ent->aiCharacter >= AICHAR_STIMSOLDIER1 && ent->aiCharacter <= AICHAR_STIMSOLDIER3 ) ) {
			break;
		}

		if ( ent->takedamage /*&& !AICast_NoFlameDamage(ent->s.number)*/ ) {
			VectorSubtract( ent->r.currentOrigin, enemy->r.currentOrigin, vec );
			VectorNormalize( vec );
			if ( !( enemy->r.svFlags & SVF_CASTAI ) ) {
				G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 8, 0, MOD_LIGHTNING );
			} else {
				G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 4, 0, MOD_LIGHTNING );
			}
		}
		break;
	case CLDMG_FLAMETHROWER:
		
		if ( ent->takedamage && !AICast_NoFlameDamage( ent->s.number ) ) {
			#define FLAME_THRESHOLD 50
			int damage = 5;

			// RF, only do damage once they start burning
			//if (ent->health > 0)	// don't explode from flamethrower
			//	G_Damage( traceEnt, ent, ent, forward, tr.endpos, 1, 0, MOD_LIGHTNING);

			// now check the damageQuota to see if we should play a pain animation
			// first reduce the current damageQuota with time
			if ( ent->flameQuotaTime && ent->flameQuota > 0 ) {
				ent->flameQuota -= (int)( ( (float)( level.time - ent->flameQuotaTime ) / 1000 ) * (float)damage / 2.0 );
				if ( ent->flameQuota < 0 ) {
					ent->flameQuota = 0;
				}
			}

			// add the new damage
			ent->flameQuota += damage;
			ent->flameQuotaTime = level.time;

			// Ridah, make em burn
			if ( ent->client && ( !( ent->r.svFlags & SVF_CASTAI ) || ent->health <= 0 || ent->flameQuota > FLAME_THRESHOLD ) ) {
				if ( ent->s.onFireEnd < level.time ) {
					ent->s.onFireStart = level.time;
				}
				if ( ent->health <= 0 || !( ent->r.svFlags & SVF_CASTAI ) ) {
					if ( ent->r.svFlags & SVF_CASTAI ) {
						ent->s.onFireEnd = level.time + 6000;
					} else {
						ent->s.onFireEnd = level.time + FIRE_FLASH_TIME;
					}
				} else {
					ent->s.onFireEnd = level.time + 99999;  // make sure it goes for longer than they need to die
				}
				ent->flameBurnEnt = enemy->s.number;
				// add to playerState for client-side effect
				ent->client->ps.onFireStart = level.time;
			}
		}
		break;
	}
}
// -NERVE - SMF

/*
============
Cmd_ClientDamage_f
============
*/
void Cmd_ClientDamage_f( gentity_t *clent ) {
	char s[MAX_STRING_CHARS];
	int entnum, id, enemynum;

	if ( Cmd_Argc() != 4 ) {
		Com_Printf( "ClientDamage command issued with incorrect number of args\n" );
	}

	Cmd_ArgvBuffer( 1, s, sizeof( s ) );
	entnum = atoi( s );

	Cmd_ArgvBuffer( 2, s, sizeof( s ) );
	enemynum = atoi( s );

	Cmd_ArgvBuffer( 3, s, sizeof( s ) );
	id = atoi( s );

	ClientDamage( clent, entnum, enemynum, id );
}

/*
==============
Cmd_EntityCount_f
==============
*/
#define AITEAM_NAZI     0
#define AITEAM_ALLIES   1
#define AITEAM_MONSTER  2
void Cmd_EntityCount_f( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		return;
	}

	Com_Printf( "entity count = %i\n", level.num_entities );

	{
		int kills[2];
		int nazis[2];
		int monsters[2];
		int i;
		gentity_t *ent;

		// count kills
		kills[0] = kills[1] = 0;
		nazis[0] = nazis[1] = 0;
		monsters[0] = monsters[1] = 0;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			ent = &g_entities[i];

			if ( !ent->inuse ) {
				continue;
			}

			if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
				continue;
			}

			if ( ent->aiTeam == AITEAM_ALLIES ) {
				continue;
			}

			kills[1]++;

			if ( ent->health <= 0 ) {
				kills[0]++;
			}

			if ( ent->aiTeam == AITEAM_NAZI ) {
				nazis[1]++;
				if ( ent->health <= 0 ) {
					nazis[0]++;
				}
			} else {
				monsters[1]++;
				if ( ent->health <= 0 ) {
					monsters[0]++;
				}
			}
		}
		Com_Printf( "kills %i/%i nazis %i/%i monsters %i/%i \n",kills[0], kills[1], nazis[0], nazis[1], monsters[0], monsters[1] );

	}
}

// NERVE - SMF
/*
============
Cmd_SetSpawnPoint_f
============
*/
void Cmd_SetSpawnPoint_f( gentity_t *clent ) {
	char arg[MAX_TOKEN_CHARS];
	int spawnIndex;

	if ( Cmd_Argc() != 2 ) {
		return;
	}

	Cmd_ArgvBuffer( 1, arg, sizeof( arg ) );
	spawnIndex = atoi( arg );
}
// -NERVE - SMF

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;     // not fully in game yet
	}


	Cmd_ArgvBuffer( 0, cmd, sizeof( cmd ) );

	// Ridah, AI Cast debugging
	if ( Q_stricmp( cmd, "aicast" ) == 0 ) {
		extern void AICast_DBG_Cmd_f( int clientNum );
		//
		AICast_DBG_Cmd_f( clientNum );
		return;
	}
	// done.

	// RF, client damage commands
	if ( Q_stricmp( cmd, "cld" ) == 0 ) {
		Cmd_ClientDamage_f( ent );
		return;
	}
	// done.


//----(SA)	added
	if ( Q_stricmp( cmd, "fogswitch" ) == 0 ) {
		Cmd_Fogswitch_f();
		return;
	}
//----(SA)	end


	if ( Q_stricmp( cmd, "give" ) == 0 ) {
		Cmd_Give_f( ent );
	} else if ( Q_stricmp( cmd, "god" ) == 0 )  {
		Cmd_God_f( ent );
	} else if ( Q_stricmp( cmd, "nofatigue" ) == 0 )  {
		Cmd_Nofatigue_f( ent );
	} else if ( Q_stricmp( cmd, "notarget" ) == 0 )  {
		Cmd_Notarget_f( ent );
	} else if ( Q_stricmp( cmd, "noclip" ) == 0 )  {
		Cmd_Noclip_f( ent );
	} else if ( Q_stricmp( cmd, "kill" ) == 0 )  {
		Cmd_Kill_f( ent );

	} else if ( Q_stricmp( cmd, "follow" ) == 0 )  {
		Cmd_Follow_f( ent );
	} else if ( Q_stricmp( cmd, "follownext" ) == 0 )  {
		Cmd_FollowCycle_f( ent, 1 );
	} else if ( Q_stricmp( cmd, "followprev" ) == 0 )  {
		Cmd_FollowCycle_f( ent, -1 );
	} else if ( Q_stricmp( cmd, "where" ) == 0 )  {
		Cmd_Where_f( ent );

	} else if ( Q_stricmp( cmd, "startCamera" ) == 0 )  {
		Cmd_StartCamera_f( ent );
	} else if ( Q_stricmp( cmd, "stopCamera" ) == 0 )  {
		Cmd_StopCamera_f( ent );
	} else if ( Q_stricmp( cmd, "setCameraOrigin" ) == 0 )  {
		Cmd_SetCameraOrigin_f( ent );
	} else if ( Q_stricmp( cmd, "cameraInterrupt" ) == 0 )  {
		Cmd_InterruptCamera_f( ent );
	} else if ( Q_stricmp( cmd, "setviewpos" ) == 0 )  {
		Cmd_SetViewpos_f( ent );
	} else if ( Q_stricmp( cmd, "entitycount" ) == 0 )  {
		Cmd_EntityCount_f( ent );
	} else if ( Q_stricmp( cmd, "setspawnpt" ) == 0 )  {
		Cmd_SetSpawnPoint_f( ent );
	} else {
		SV_GameSendServerCommand( clientNum, va( "print \"unknown cmd %s\n\"", cmd ) );
	}
}
