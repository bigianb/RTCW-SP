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

// sv_client.c -- server code for dealing with clients

#include "server.h"
#include "../game/g_func_decs.h"

/*
=================
SV_GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

If we are authorizing, a challenge request will cause a packet
to be sent to the authorize server.

When an authorizeip is returned, a challenge response will be
sent to that ip.
=================
*/
void SV_GetChallenge( netadr_t from ) {
	
	return;
	
}

/*
====================
SV_AuthorizeIpPacket

A packet has been returned from the authorize server.
If we have a challenge adr for that ip, send the
challengeResponse to it
====================
*/
void SV_AuthorizeIpPacket( netadr_t from ) {
	int challenge;
	int i;
	char    *s;
	char    *r;
	char ret[1024];

	if ( !NET_CompareBaseAdr( from, svs.authorizeAddress ) ) {
		Com_Printf( "SV_AuthorizeIpPacket: not from authorize server\n" );
		return;
	}

	challenge = atoi( Cmd_Argv( 1 ) );

	for ( i = 0 ; i < MAX_CHALLENGES ; i++ ) {
		if ( svs.challenges[i].challenge == challenge ) {
			break;
		}
	}
	if ( i == MAX_CHALLENGES ) {
		Com_Printf( "SV_AuthorizeIpPacket: challenge not found\n" );
		return;
	}

	// send a packet back to the original client
	svs.challenges[i].pingTime = svs.time;
	s = Cmd_Argv( 2 );
	r = Cmd_Argv( 3 );          // reason

	if ( !Q_stricmp( s, "demo" ) ) {
		if ( Cvar_VariableValue( "fs_restrict" ) ) {
			// a demo client connecting to a demo server
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr,
								"challengeResponse %i", svs.challenges[i].challenge );
			return;
		}
		// they are a demo client trying to connect to a real server
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, "print\nServer is not a demo server\n" );
		// clear the challenge record so it won't timeout and let them through
		memset( &svs.challenges[i], 0, sizeof( svs.challenges[i] ) );
		return;
	}
	if ( !Q_stricmp( s, "accept" ) ) {
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr,
							"challengeResponse %i", svs.challenges[i].challenge );
		return;
	}
	if ( !Q_stricmp( s, "unknown" ) ) {
		if ( !r ) {
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, "print\nAwaiting CD key authorization\n" );
		} else {
			snprintf( ret, 1024, "print\n%s\n", r );
			NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, ret );
		}
		// clear the challenge record so it won't timeout and let them through
		memset( &svs.challenges[i], 0, sizeof( svs.challenges[i] ) );
		return;
	}

	// authorization failed
	if ( !r ) {
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, "print\nSomeone is using this CD Key\n" );
	} else {
		snprintf( ret, 1024, "print\n%s\n", r );
		NET_OutOfBandPrint( NS_SERVER, svs.challenges[i].adr, ret );
	}

	// clear the challenge record so it won't timeout and let them through
	memset( &svs.challenges[i], 0, sizeof( svs.challenges[i] ) );
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/
void SV_DirectConnect( netadr_t from ) {
	char userinfo[MAX_INFO_STRING];
	int i;
	client_t    *cl, *newcl;
    client_t temp;
	sharedEntity_t *ent;
	int clientNum;
	int version;
	int qport;
	int challenge;
	char        *password;
	int startIndex;
	char        *denied;
	int count;

	Com_DPrintf( "SVC_DirectConnect ()\n" );

	Q_strncpyz( userinfo, Cmd_Argv( 1 ), sizeof( userinfo ) );

	version = atoi( Info_ValueForKey( userinfo, "protocol" ) );
	if ( version != PROTOCOL_VERSION ) {
		NET_OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i.\n", PROTOCOL_VERSION );
		Com_DPrintf( "    rejected connect from version %i\n", version );
		return;
	}

	qport = atoi( Info_ValueForKey( userinfo, "qport" ) );
	challenge = atoi( Info_ValueForKey( userinfo, "challenge" ) );

	// quick reject
	for ( i = 0,cl = svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
			 && ( cl->netchan.qport == qport
				  || from.port == cl->netchan.remoteAddress.port ) ) {
			if ( ( svs.time - cl->lastConnectTime )
				 < ( sv_reconnectlimit->integer * 1000 ) ) {
				Com_DPrintf( "%s:reconnect rejected : too soon\n", NET_AdrToString( from ) );
				return;
			}
			break;
		}
	}

	// see if the challenge is valid (LAN clients don't need to challenge)
	if ( !NET_IsLocalAddress( from ) ) {
		int ping;

		for ( i = 0 ; i < MAX_CHALLENGES ; i++ ) {
			if ( NET_CompareAdr( from, svs.challenges[i].adr ) ) {
				if ( challenge == svs.challenges[i].challenge ) {
					break;      // good
				}
				NET_OutOfBandPrint( NS_SERVER, from, "print\nBad challenge.\n" );
				return;
			}
		}
		if ( i == MAX_CHALLENGES ) {
			NET_OutOfBandPrint( NS_SERVER, from, "print\nNo challenge for address.\n" );
			return;
		}
		// force the IP key/value pair so the game can filter based on ip
		Info_SetValueForKey( userinfo, "ip", NET_AdrToString( from ) );

		ping = svs.time - svs.challenges[i].pingTime;
		Com_Printf( "Client %i connecting with %i challenge ping\n", i, ping );

		// never reject a LAN client based on ping
		if ( !Sys_IsLANAddress( from ) ) {
			if ( sv_minPing->value && ping < sv_minPing->value ) {
				// don't let them keep trying until they get a big delay
				NET_OutOfBandPrint( NS_SERVER, from, "print\nServer is for high pings only\n" );
				Com_DPrintf( "Client %i rejected on a too low ping\n", i );
				// reset the address otherwise their ping will keep increasing
				// with each connect message and they'd eventually be able to connect
				svs.challenges[i].adr.port = 0;
				return;
			}
			if ( sv_maxPing->value && ping > sv_maxPing->value ) {
				NET_OutOfBandPrint( NS_SERVER, from, "print\nServer is for low pings only\n" );
				Com_DPrintf( "Client %i rejected on a too high ping\n", i );
				return;
			}
		}
	} else {
		// force the "ip" info key to "localhost"
		Info_SetValueForKey( userinfo, "ip", "localhost" );
	}

	newcl = &temp;
	memset( newcl, 0, sizeof( client_t ) );

	// if there is already a slot for this ip, reuse it
	for ( i = 0,cl = svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
			 && ( cl->netchan.qport == qport
				  || from.port == cl->netchan.remoteAddress.port ) ) {
			Com_Printf( "%s:reconnect\n", NET_AdrToString( from ) );
			newcl = cl;
			// disconnect the client from the game first so any flags the
			// player might have are dropped
			ClientDisconnect(newcl - svs.clients );
			//
			goto gotnewcl;
		}
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey( userinfo, "password" );
	if ( !strcmp( password, sv_privatePassword->string ) ) {
		startIndex = 0;
	} else {
		// skip past the reserved slots
		startIndex = sv_privateClients->integer;
	}

	newcl = NULL;
	for ( i = startIndex; i < sv_maxclients->integer ; i++ ) {
		cl = &svs.clients[i];
		if ( cl->state == CS_FREE ) {
			newcl = cl;
			break;
		}
	}

	if ( !newcl ) {
		if ( NET_IsLocalAddress( from ) ) {
			count = 0;
			for ( i = startIndex; i < sv_maxclients->integer ; i++ ) {
				cl = &svs.clients[i];
				if ( cl->netchan.remoteAddress.type == NA_BOT ) {
					count++;
				}
			}
			// if they're all bots
			if ( count >= sv_maxclients->integer - startIndex ) {
				SV_DropClient( &svs.clients[sv_maxclients->integer - 1], "only bots on server" );
				newcl = &svs.clients[sv_maxclients->integer - 1];
			} else {
				Com_Error( ERR_FATAL, "server is full on local connect\n" );
				return;
			}
		} else {
			NET_OutOfBandPrint( NS_SERVER, from, "print\nServer is full.\n" );
			Com_DPrintf( "Rejected a connection.\n" );
			return;
		}
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->reliableAcknowledge = 0;
	cl->reliableSequence = 0;

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	clientNum = newcl - svs.clients;
	ent = SV_GentityNum( clientNum );
	newcl->gentity = ent;

	// save the challenge
	newcl->challenge = challenge;

	// save the address
	Netchan_Setup( NS_SERVER, &newcl->netchan, from, qport );

	// save the userinfo
	Q_strncpyz( newcl->userinfo, userinfo, sizeof( newcl->userinfo ) );

	// get the game a chance to reject this connection or modify the userinfo
	denied = ClientConnect( clientNum, qtrue, qfalse ); // firstTime = qtrue
	if ( denied ) {
		NET_OutOfBandPrint( NS_SERVER, from, "print\n%s\n", denied );
		Com_DPrintf( "Game rejected a connection: %s.\n", denied );
		return;
	}

	// RF, create the reliable commands
	if ( newcl->netchan.remoteAddress.type != NA_BOT ) {
		SV_InitReliableCommandsForClient( newcl, MAX_RELIABLE_COMMANDS );
	} else {
		SV_InitReliableCommandsForClient( newcl, 0 );
	}

	SV_UserinfoChanged( newcl );

	// send the connect packet to the client
	NET_OutOfBandPrint( NS_SERVER, from, "connectResponse" );

	Com_DPrintf( "Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name );

	newcl->state = CS_CONNECTED;
	newcl->nextSnapshotTime = svs.time;
	newcl->lastPacketTime = svs.time;
	newcl->lastConnectTime = svs.time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	count = 0;
	for ( i = 0,cl = svs.clients ; i < sv_maxclients->integer ; i++,cl++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			count++;
		}
	}
	if ( count == 1 || count == sv_maxclients->integer ) {
		SV_Heartbeat_f();
	}
}


/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalMessage() will handle that
=====================
*/
void SV_DropClient( client_t *drop, const char *reason ) {
	int i;

	if ( drop->state == CS_ZOMBIE ) {
		return;     // already dropped
	}

	// Ridah, no need to tell the player if an AI drops
	if ( !( drop->gentity && drop->gentity->r.svFlags & SVF_CASTAI ) ) {
		// tell everyone why they got dropped
		SV_SendServerCommand( NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason );
	}

	Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
	drop->state = CS_ZOMBIE;        // become free in a few seconds

	// call the prog function for removing a client
	// this will remove the body, among other things
	ClientDisconnect(drop - svs.clients );

	// Ridah, no need to tell the player if an AI drops
	if ( !( drop->gentity && drop->gentity->r.svFlags & SVF_CASTAI ) ) {
		// add the disconnect command
		SV_SendServerCommand( drop, "disconnect" );
	}
	// done.

	if ( drop->netchan.remoteAddress.type == NA_BOT ) {
		SV_BotFreeClient( drop - svs.clients );
	}

	// nuke user info
	SV_SetUserinfo( drop - svs.clients, "" );

	// RF, nuke reliable commands
	SV_FreeReliableCommandsForClient( drop );

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			break;
		}
	}
	if ( i == sv_maxclients->integer ) {
		SV_Heartbeat_f();
	}
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void SV_SendClientGameState( client_t *client ) {
	int start;
	entityState_t   *base, nullstate;
	msg_t msg;
	byte msgBuffer[MAX_MSGLEN];

	Com_DPrintf( "SV_SendClientGameState() for %s\n", client->name );
	Com_DPrintf( "Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name );
	client->state = CS_PRIMED;
	client->pureAuthentic = 0;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient( client, &msg );

	// send the gamestate
	MSG_WriteByte( &msg, svc_gamestate );
	MSG_WriteLong( &msg, client->reliableSequence );

	// write the configstrings
	for ( start = 0 ; start < MAX_CONFIGSTRINGS ; start++ ) {
		if ( sv.configstrings[start][0] ) {
			MSG_WriteByte( &msg, svc_configstring );
			MSG_WriteShort( &msg, start );
			MSG_WriteBigString( &msg, sv.configstrings[start] );
		}
	}

	// write the baselines
	memset( &nullstate, 0, sizeof( nullstate ) );
	for ( start = 0 ; start < MAX_GENTITIES; start++ ) {
		base = &sv.svEntities[start].baseline;
		if ( !base->number ) {
			continue;
		}
		MSG_WriteByte( &msg, svc_baseline );
		MSG_WriteDeltaEntity( &msg, &nullstate, base, qtrue );
	}

	MSG_WriteByte( &msg, svc_EOF );

	MSG_WriteLong( &msg, client - svs.clients );

	// write the checksum feed
	MSG_WriteLong( &msg, sv.checksumFeed );

	// deliver this to the client
	SV_SendMessageToClient( &msg, client );
}


/*
==================
SV_ClientEnterWorld
==================
*/
void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd ) {
	int clientNum;
	sharedEntity_t *ent;

	Com_DPrintf( "Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name );
	client->state = CS_ACTIVE;

	// set up the entity for the client
	clientNum = client - svs.clients;
	ent = SV_GentityNum( clientNum );
	ent->s.number = clientNum;
	client->gentity = ent;

	client->deltaMessage = -1;
	client->nextSnapshotTime = svs.time;    // generate a snapshot immediately
	client->lastUsercmd = *cmd;

	// call the game begin function
	ClientBegin( client - svs.clients );
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/


/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
static void SV_Disconnect_f( client_t *cl ) {
	SV_DropClient( cl, "disconnected" );
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but i abstained

=================
*/
static void SV_VerifyPaks_f( client_t *cl ) {
	int nChkSum1, nChkSum2, nClientPaks, nServerPaks, i, j, nCurArg;
	int nClientChkSum[1024];
	int nServerChkSum[1024];
	const char *pPaks, *pArg;
	qboolean bGood = qtrue;

	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	//
	if ( sv_pure->integer != 0 ) {

		bGood = qtrue;
		nChkSum1 = nChkSum2 = 0;
		// we run the game, so determine which cgame and ui the client "should" be running
		bGood = ( FS_FileIsInPAK( "vm/cgame.qvm", &nChkSum1 ) == 1 );
		if ( bGood ) {
			bGood = ( FS_FileIsInPAK( "vm/ui.qvm", &nChkSum2 ) == 1 );
		}

		nClientPaks = Cmd_Argc();

		// start at arg 1 ( skip cl_paks )
		nCurArg = 1;

		// we basically use this while loop to avoid using 'goto' :)
		while ( bGood ) {

			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if ( nClientPaks < 6 ) {
				bGood = qfalse;
				break;
			}
			// verify first to be the cgame checksum
			pArg = Cmd_Argv( nCurArg++ );
			if ( !pArg || *pArg == '@' || atoi( pArg ) != nChkSum1 ) {
				bGood = qfalse;
				break;
			}
			// verify the second to be the ui checksum
			pArg = Cmd_Argv( nCurArg++ );
			if ( !pArg || *pArg == '@' || atoi( pArg ) != nChkSum2 ) {
				bGood = qfalse;
				break;
			}
			// should be sitting at the delimeter now
			pArg = Cmd_Argv( nCurArg++ );
			if ( *pArg != '@' ) {
				bGood = qfalse;
				break;
			}
			// store checksums since tokenization is not re-entrant
			for ( i = 0; nCurArg < nClientPaks; i++ ) {
				nClientChkSum[i] = atoi( Cmd_Argv( nCurArg++ ) );
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for ( i = 0; i < nClientPaks; i++ ) {
				for ( j = 0; j < nClientPaks; j++ ) {
					if ( i == j ) {
						continue;
					}
					if ( nClientChkSum[i] == nClientChkSum[j] ) {
						bGood = qfalse;
						break;
					}
				}
				if ( bGood == qfalse ) {
					break;
				}
			}
			if ( bGood == qfalse ) {
				break;
			}

			// get the pure checksums of the pk3 files loaded by the server
			pPaks = FS_LoadedPakPureChecksums();
			Cmd_TokenizeString( pPaks );
			nServerPaks = Cmd_Argc();
			if ( nServerPaks > 1024 ) {
				nServerPaks = 1024;
			}

			for ( i = 0; i < nServerPaks; i++ ) {
				nServerChkSum[i] = atoi( Cmd_Argv( i ) );
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for ( i = 0; i < nClientPaks; i++ ) {
				for ( j = 0; j < nServerPaks; j++ ) {
					if ( nClientChkSum[i] == nServerChkSum[j] ) {
						break;
					}
				}
				if ( j >= nServerPaks ) {
					bGood = qfalse;
					break;
				}
			}
			if ( bGood == qfalse ) {
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = sv.checksumFeed;
			for ( i = 0; i < nClientPaks; i++ ) {
				nChkSum1 ^= nClientChkSum[i];
			}
			nChkSum1 ^= nClientPaks;
			if ( nChkSum1 != nClientChkSum[nClientPaks] ) {
				bGood = qfalse;
				break;
			}

			// break out
			break;
		}

		if ( bGood ) {
			cl->pureAuthentic = 1;
		} else {
			cl->pureAuthentic = 0;
			cl->nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			SV_SendClientSnapshot( cl );
			SV_DropClient( cl, "Unpure client detected. Invalid .PK3 files referenced!" );
		}
	}
}

/*
=================
SV_ResetPureClient_f
=================
*/
static void SV_ResetPureClient_f( client_t *cl ) {
	cl->pureAuthentic = 0;
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged( client_t *cl ) {
	char    *val;
	int i;

	// name for C code
	Q_strncpyz( cl->name, Info_ValueForKey( cl->userinfo, "name" ), sizeof( cl->name ) );

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// internet public server, assume they don't need a rate choke
	if ( Sys_IsLANAddress( cl->netchan.remoteAddress ) && com_dedicated->integer != 2 ) {
		cl->rate = 99999;   // lans should not rate limit
	} else {
		val = Info_ValueForKey( cl->userinfo, "rate" );
		if ( strlen( val ) ) {
			i = atoi( val );
			cl->rate = i;
			if ( cl->rate < 1000 ) {
				cl->rate = 1000;
			} else if ( cl->rate > 90000 ) {
				cl->rate = 90000;
			}
		} else {
			cl->rate = 3000;
		}
	}
	val = Info_ValueForKey( cl->userinfo, "handicap" );
	if ( strlen( val ) ) {
		i = atoi( val );
		if ( i <= 0 || i > 100 || strlen( val ) > 4 ) {
			Info_SetValueForKey( cl->userinfo, "handicap", "100" );
		}
	}

	// snaps command
	val = Info_ValueForKey( cl->userinfo, "snaps" );
	if ( strlen( val ) ) {
		i = atoi( val );
		if ( i < 1 ) {
			i = 1;
		} else if ( i > 30 ) {
			i = 30;
		}
		cl->snapshotMsec = 1000 / i;
	} else {
		cl->snapshotMsec = 50;
	}
}


/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( client_t *cl ) {
	Q_strncpyz( cl->userinfo, Cmd_Argv( 1 ), sizeof( cl->userinfo ) );

	SV_UserinfoChanged( cl );
	// call prog code to allow overrides
	ClientUserinfoChanged(cl - svs.clients );
}



typedef struct {
	char    *name;
	void ( *func )( client_t *cl );
} ucmd_t;

static ucmd_t ucmds[] = {
	{"userinfo", SV_UpdateUserinfo_f},
	{"disconnect", SV_Disconnect_f},
	{"cp", SV_VerifyPaks_f},
	{"vdr", SV_ResetPureClient_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand( client_t *cl, const char *s, qboolean clientOK ) {
	ucmd_t  *u;

	Cmd_TokenizeString( s );

	// see if it is a server level command
	for ( u = ucmds ; u->name ; u++ ) {
		if ( !strcmp( Cmd_Argv( 0 ), u->name ) ) {
			u->func( cl );
			break;
		}
	}

	if ( clientOK ) {
		// pass unknown strings to the game
		if ( !u->name && sv.state == SS_GAME ) {
			ClientCommand( cl - svs.clients );
		}
	}
}

/*
===============
SV_ClientCommand
===============
*/
static qboolean SV_ClientCommand( client_t *cl, msg_t *msg ) {
	int seq;
	const char  *s;
	qboolean clientOk = qtrue;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed it
	if ( cl->lastClientCommand >= seq ) {
		return qtrue;
	}

	//Com_DPrintf( "clientCommand: %s : %i : %s\n", cl->name, seq, s );

	// drop the connection if we have somehow lost commands
	if ( seq > cl->lastClientCommand + 1 ) {
		Com_Printf( "Client %s lost %i clientCommands\n", cl->name,
					seq - cl->lastClientCommand + 1 );
		SV_DropClient( cl, "Lost reliable commands" );
		return qfalse;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	if ( !com_cl_running->integer &&
		 cl->state >= CS_ACTIVE &&      // (SA) this was commented out in Wolf.  Did we do that?
		 sv_floodProtect->integer &&
		 svs.time < cl->nextReliableTime ) {
		// ignore any other text messages from this client but let them keep playing
		clientOk = qfalse;
		Com_DPrintf( "client text ignored for %s\n", cl->name );
		//return qfalse;	// stop processing
	}

	// don't allow another command for one second
	cl->nextReliableTime = svs.time + 1000;

	SV_ExecuteClientCommand( cl, s, clientOk );

	cl->lastClientCommand = seq;
	snprintf( cl->lastClientCommandString, sizeof( cl->lastClientCommandString ), "%s", s );

	return qtrue;       // continue procesing
}


//==================================================================================


/*
==================
SV_ClientThink

Also called by bot code
==================
*/
void SV_ClientThink( client_t *cl, usercmd_t *cmd ) {
	cl->lastUsercmd = *cmd;

	if ( cl->state != CS_ACTIVE ) {
		return;     // may have been kicked during the last usercmd
	}

	ClientThink(cl - svs.clients );
}

/*
==================
SV_UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove( client_t *cl, msg_t *msg, qboolean delta ) {
	int i, key;
	int cmdCount;
	usercmd_t nullcmd;
	usercmd_t cmds[MAX_PACKET_USERCMDS];
	usercmd_t   *cmd, *oldcmd;

	if ( delta ) {
		cl->deltaMessage = cl->messageAcknowledge;
	} else {
		cl->deltaMessage = -1;
	}

	cmdCount = MSG_ReadByte( msg );

	if ( cmdCount < 1 ) {
		Com_Printf( "cmdCount < 1\n" );
		return;
	}

	if ( cmdCount > MAX_PACKET_USERCMDS ) {
		Com_Printf( "cmdCount > MAX_PACKET_USERCMDS\n" );
		return;
	}

	// use the checksum feed in the key
	key = sv.checksumFeed;
	// also use the message acknowledge
	key ^= cl->messageAcknowledge;
	// also use the last acknowledged server command in the key
	//key ^= Com_HashKey(cl->reliableCommands[ cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ], 32);
	key ^= Com_HashKey( SV_GetReliableCommand( cl, cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 ) ), 32 );

	memset( &nullcmd, 0, sizeof( nullcmd ) );
	oldcmd = &nullcmd;
	for ( i = 0 ; i < cmdCount ; i++ ) {
		cmd = &cmds[i];
		MSG_ReadDeltaUsercmdKey( msg, key, oldcmd, cmd );
//		MSG_ReadDeltaUsercmd( msg, oldcmd, cmd );
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if ( cl->state == CS_PRIMED ) {
		SV_ClientEnterWorld( cl, &cmds[0] );
		// the moves can be processed normaly
	}
	//
	if ( sv_pure->integer != 0 && cl->pureAuthentic == 0 ) {
		SV_DropClient( cl, "Cannot validate pure client!" );
		return;
	}

	if ( cl->state != CS_ACTIVE ) {
		cl->deltaMessage = -1;
		return;
	}

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for ( i =  0 ; i < cmdCount ; i++ ) {
		// if this is a cmd from before a map_restart ignore it
		if ( cmds[i].serverTime > cmds[cmdCount - 1].serverTime ) {
			continue;
		}
		
		SV_ClientThink( cl, &cmds[ i ] );
	}
}


/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg ) {
	int c;
	int serverId;

	MSG_Bitstream( msg );

	serverId = MSG_ReadLong( msg );
	cl->messageAcknowledge = MSG_ReadLong( msg );

	if ( cl->messageAcknowledge < 0 ) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//SV_DropClient( cl, "illegible client message" );
		return;
	}

	cl->reliableAcknowledge = MSG_ReadLong( msg );

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if ( cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS ) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//SV_DropClient( cl, "illegible client message" );
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//

	if ( serverId != sv.serverId  ) {
		if ( serverId == sv.restartedServerId ) {
			// they just haven't caught the map_restart yet
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if ( cl->messageAcknowledge > cl->gamestateMessageNum ) {
			Com_DPrintf( "%s : dropped gamestate, resending\n", cl->name );
			SV_SendClientGameState( cl );
		}
		return;
	}

	// RF, kill any reliableCommands that have been acknowledged
	SV_FreeAcknowledgedReliableCommands( cl );

	// read optional clientCommand strings
	do {
		c = MSG_ReadByte( msg );
		if ( c == clc_EOF ) {
			break;
		}
		if ( c != clc_clientCommand ) {
			break;
		}
		if ( !SV_ClientCommand( cl, msg ) ) {
			return; // we couldn't execute it because of the flood protection
		}
		if ( cl->state == CS_ZOMBIE ) {
			return; // disconnect command
		}
	} while ( 1 );

	// read the usercmd_t
	if ( c == clc_move ) {
		SV_UserMove( cl, msg, qtrue );
	} else if ( c == clc_moveNoDelta ) {
		SV_UserMove( cl, msg, qfalse );
	} else if ( c != clc_EOF ) {
		Com_Printf( "WARNING: bad command byte for client %i\n", cl - svs.clients );
	}
//	if ( msg->readcount != msg->cursize ) {
//		Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
//	}
}

