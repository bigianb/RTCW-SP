/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source
Code (RTCW SP Source Code).

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

In addition, the RTCW SP Source Code is also subject to certain additional
terms. You should have received a copy of these additional terms immediately
following the terms and conditions of the GNU General Public License which
accompanied the RTCW SP Source Code.  If not, please request a copy in writing
from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite
120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "server.h"

/*
=============================================================================

Delta encode a client frame onto the network channel

A normal server packet will look like:

4	sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1	svc_snapshot
4	last client reliable command
4	serverTime
1	lastframe for delta compression
1	snapFlags
1	areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/*
=============
SV_EmitPacketEntities

Writes a delta update of an EntityState list to the message.
=============
*/
static void SV_EmitPacketEntities(clientSnapshot_t *from, clientSnapshot_t *to,
                                  msg_t *msg) {
  int from_num_entities;

  // generate the delta update
  if (!from) {
    from_num_entities = 0;
  } else {
    from_num_entities = from->num_entities;
  }

  EntityState *newent = nullptr;
  EntityState *oldent = nullptr;
  int newindex = 0;
  int oldindex = 0;
  while (newindex < to->num_entities || oldindex < from_num_entities) {
    int newnum;
    if (newindex >= to->num_entities) {
      newnum = 9999;
    } else {
      newent = &svs.snapshotEntities[(to->first_entity + newindex) %
                                     svs.numSnapshotEntities];
      newnum = newent->number;
    }

    int oldnum;
    if (oldindex >= from_num_entities) {
      oldnum = 9999;
    } else {
      oldent = &svs.snapshotEntities[(from->first_entity + oldindex) %
                                     svs.numSnapshotEntities];
      oldnum = oldent->number;
    }

    if (newnum == oldnum) {
      // delta update from old position
      // because the force parm is false, this will not result
      // in any bytes being emited if the entity has not changed at all
      MSG_WriteDeltaEntity(msg, oldent, newent, false);
      oldindex++;
      newindex++;
      continue;
    }

    if (newnum < oldnum) {
      // this is a new entity, send it from the baseline
      MSG_WriteDeltaEntity(msg, &sv.svEntities[newnum].baseline, newent, true);
      newindex++;
      continue;
    }

    if (newnum > oldnum) {
      // the old entity isn't present in the new message
      MSG_WriteDeltaEntity(msg, oldent, nullptr, true);
      oldindex++;
      continue;
    }
  }

  MSG_WriteBits(msg, (MAX_GENTITIES - 1),
                GENTITYNUM_BITS); // end of packetentities
}

/*
==================
SV_WriteSnapshotToClient
==================
*/
static void SV_WriteSnapshotToClient(client_t *client, msg_t *msg) {
  int lastframe;

  // this is the snapshot we are creating
  clientSnapshot_t *frame =
      &client->frames[client->netchan.outgoingSequence & PACKET_MASK];
  clientSnapshot_t *oldframe = nullptr;
  // try to use a previous frame as the source for delta compressing the
  // snapshot
  if (client->deltaMessage <= 0 || client->state != CS_ACTIVE) {
    // client is asking for a retransmit
    oldframe = nullptr;
    lastframe = 0;
  } else if (client->netchan.outgoingSequence - client->deltaMessage >=
             (PACKET_BACKUP - 3)) {
    // client hasn't gotten a good message through in a long time
    Com_DPrintf("%s: Delta request from out of date packet.\n", client->name);
    oldframe = nullptr;
    lastframe = 0;
  } else {
    // we have a valid snapshot to delta from
    oldframe = &client->frames[client->deltaMessage & PACKET_MASK];
    lastframe = client->netchan.outgoingSequence - client->deltaMessage;

    // the snapshot's entities may still have rolled off the buffer, though
    if (oldframe->first_entity <=
        svs.nextSnapshotEntities - svs.numSnapshotEntities) {
      Com_DPrintf("%s: Delta request from out of date entities.\n",
                  client->name);
      oldframe = nullptr;
      lastframe = 0;
    }
  }

  MSG_WriteByte(msg, svc_snapshot);

  // NOTE, MRE: now sent at the start of every message from server to client
  // let the client know which reliable clientCommands we have received
  // MSG_WriteLong( msg, client->lastClientCommand );

  // send over the current server time so the client can drift
  // its view of time to try to match
  MSG_WriteLong(msg, svs.time);

  // what we are delta'ing from
  MSG_WriteByte(msg, lastframe);

  int snapFlags = svs.snapFlagServerBit;
  if (client->rateDelayed) {
    snapFlags |= SNAPFLAG_RATE_DELAYED;
  }
  if (client->state != CS_ACTIVE) {
    snapFlags |= SNAPFLAG_NOT_ACTIVE;
  }

  MSG_WriteByte(msg, snapFlags);

  // send over the areabits
  MSG_WriteByte(msg, frame->areabytes);
  MSG_WriteData(msg, frame->areabits, frame->areabytes);

  // delta encode the playerstate
  if (oldframe) {
    MSG_WriteDeltaPlayerstate(msg, &oldframe->ps, &frame->ps);
  } else {
    MSG_WriteDeltaPlayerstate(msg, nullptr, &frame->ps);
  }

  // delta encode the entities
  SV_EmitPacketEntities(oldframe, frame, msg);

  // padding for rate debugging
  if (sv_padPackets->integer) {
    for (int i = 0; i < sv_padPackets->integer; i++) {
      MSG_WriteByte(msg, svc_nop);
    }
  }
}

/*
==================
SV_UpdateServerCommandsToClient

(re)send all server commands the client hasn't acknowledged yet
==================
*/
void SV_UpdateServerCommandsToClient(client_t *client, msg_t *msg) {
  // write any unacknowledged serverCommands
  for (int i = client->reliableAcknowledge + 1; i <= client->reliableSequence;
       i++) {
    MSG_WriteByte(msg, svc_serverCommand);
    MSG_WriteLong(msg, i);
    MSG_WriteString(
        msg, SV_GetReliableCommand(client, i & (MAX_RELIABLE_COMMANDS - 1)));
  }
  client->reliableSent = client->reliableSequence;
}

/*
=============================================================================

Build a client snapshot structure

=============================================================================
*/

#define MAX_SNAPSHOT_ENTITIES 2048

typedef struct {
  int numSnapshotEntities;
  int snapshotEntities[MAX_SNAPSHOT_ENTITIES];
} snapshotEntityNumbers_t;

/*
=======================
SV_QsortEntityNumbers
=======================
*/
static int SV_QsortEntityNumbers(const void *a, const void *b) {
  int *ea = (int *)a;
  int *eb = (int *)b;

  if (*ea == *eb) {
    Com_Error(ERR_DROP, "SV_QsortEntityStates: duplicated entity");
  }

  if (*ea < *eb) {
    return -1;
  }

  return 1;
}

/*
===============
SV_AddEntToSnapshot
===============
*/
static void SV_AddEntToSnapshot(ServerEntity *svEnt, sharedEntity_t *gEnt,
                                snapshotEntityNumbers_t *eNums) {
  // if we have already added this entity to this snapshot, don't add again
  if (svEnt->snapshotCounter == sv.snapshotCounter) {
    return;
  }
  svEnt->snapshotCounter = sv.snapshotCounter;

  // if we are full, silently discard entities
  if (eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES) {
    return;
  }

  eNums->snapshotEntities[eNums->numSnapshotEntities] = gEnt->s.number;
  eNums->numSnapshotEntities++;
}

/*
===============
SV_AddEntitiesVisibleFromPoint
===============
*/
static void SV_AddEntitiesVisibleFromPoint(vec3_t origin,
                                           clientSnapshot_t *frame,
                                           snapshotEntityNumbers_t *eNums,
                                           bool portal) {
  // during an error shutdown message we may need to transmit
  // the shutdown message after the server has shutdown, so
  // specfically check for it
  if (!sv.state) {
    return;
  }

  int leafnum = CM_PointLeafnum(origin);
  int clientarea = CM_LeafArea(leafnum);
  int clientcluster = CM_LeafCluster(leafnum);

  // calculate the visible areas
  frame->areabytes = CM_WriteAreaBits(frame->areabits, clientarea);

  uint8_t *clientpvs = CM_ClusterPVS(clientcluster);

  int c_fullsend = 0;

  sharedEntity_t *playerEnt = SV_GentityNum(frame->ps.clientNum);

  for (int e = 0; e < sv.num_entities; e++) {
    sharedEntity_t *ent = SV_GentityNum(e);

    // never send entities that aren't linked in
    if (!ent->r.linked) {
      continue;
    }

    if (ent->s.number != e) {
      Com_DPrintf("FIXING ENT->S.NUMBER!!!\n");
      ent->s.number = e;
    }

    // entities can be flagged to explicitly not be sent to the client
    if (ent->r.svFlags & SVF_NOCLIENT) {
      continue;
    }

    // entities can be flagged to be sent to only one client
    if (ent->r.svFlags & SVF_SINGLECLIENT) {
      if (ent->r.singleClient != frame->ps.clientNum) {
        continue;
      }
    }
    // entities can be flagged to be sent to everyone but one client
    if (ent->r.svFlags & SVF_NOTSINGLECLIENT) {
      if (ent->r.singleClient == frame->ps.clientNum) {
        continue;
      }
    }

    ServerEntity *svEnt = SV_SvEntityForGentity(ent);

    // don't double add an entity through portals
    if (svEnt->snapshotCounter == sv.snapshotCounter) {
      continue;
    }

    // if this client is viewing from a camera, only add ents visible from
    // portal ents
    if ((playerEnt->s.eFlags & EF_VIEWING_CAMERA) && !portal) {
      if (ent->r.svFlags & SVF_PORTAL) {
        SV_AddEntToSnapshot(svEnt, ent, eNums);
        SV_AddEntitiesVisibleFromPoint(ent->s.origin2, frame, eNums, true);
      }
      continue;
    }

    // broadcast entities are always sent
    if (ent->r.svFlags & SVF_BROADCAST) {
      SV_AddEntToSnapshot(svEnt, ent, eNums);
      continue;
    }

	uint8_t *bitvector = clientpvs;
	
    // ignore if not touching a PV leaf
    // check area
    if (!CM_AreasConnected(clientarea, svEnt->areanum)) {
      // doors can legally straddle two areas, so
      // we may need to check another one
      if (!CM_AreasConnected(clientarea, svEnt->areanum2)) {
        goto notVisible; // blocked by a door
      }
    }

    

    // check individual leafs
    if (!svEnt->numClusters) {
      goto notVisible;
    }
    {
      int l = 0;
      int i;
      for (i = 0; i < svEnt->numClusters; i++) {
        l = svEnt->clusternums[i];
        if (bitvector[l >> 3] & (1 << (l & 7))) {
          break;
        }
      }

      // if we haven't found it to be visible,
      // check overflow clusters that coudln't be stored
      if (i == svEnt->numClusters) {
        if (svEnt->lastCluster) {
          for (; l <= svEnt->lastCluster; l++) {
            if (bitvector[l >> 3] & (1 << (l & 7))) {
              break;
            }
          }
          if (l == svEnt->lastCluster) {
            goto notVisible; // not visible
          }
        } else {
          goto notVisible;
        }
      }
    }
    if (ent->r.svFlags & SVF_VISDUMMY) {
      // find master;
      sharedEntity_t *ment = SV_GentityNum(ent->s.otherEntityNum);

      if (ment) {
        ServerEntity *master = 0;
        master = SV_SvEntityForGentity(ment);

        if (master->snapshotCounter == sv.snapshotCounter || !ment->r.linked) {
          goto notVisible;
          // continue;
        }

        SV_AddEntToSnapshot(master, ment, eNums);
      }
      goto notVisible;
    } else if (ent->r.svFlags & SVF_VISDUMMY_MULTIPLE) {

      ServerEntity *master = 0;

      for (int h = 0; h < sv.num_entities; h++) {
        sharedEntity_t *ment = SV_GentityNum(h);

        if (ment == ent) {
          continue;
        }

        if (ment) {
          master = SV_SvEntityForGentity(ment);
        } else {
          continue;
        }

        if (!(ment->r.linked)) {
          continue;
        }

        if (ment->s.number != h) {
          Com_DPrintf("FIXING vis dummy multiple ment->S.NUMBER!!!\n");
          ment->s.number = h;
        }

        if (ment->r.svFlags & SVF_NOCLIENT) {
          continue;
        }

        if (master->snapshotCounter == sv.snapshotCounter) {
          continue;
        }

        if (ment->s.otherEntityNum == ent->s.number) {
          SV_AddEntToSnapshot(master, ment, eNums);
        }
      }
      goto notVisible;
    }

    // add it
    SV_AddEntToSnapshot(svEnt, ent, eNums);

    // if its a portal entity, add everything visible from its camera position
    if (ent->r.svFlags & SVF_PORTAL) {
      SV_AddEntitiesVisibleFromPoint(ent->s.origin2, frame, eNums, true);
    }

    continue;

  notVisible:

    // Ridah, if this entity has changed events, then send it regardless of
    // whether we can see it or not
    if (ent->r.eventTime == svs.time) {
      ent->s.eFlags |= EF_NODRAW; // don't draw, just process event
      SV_AddEntToSnapshot(svEnt, ent, eNums);
    } else if (ent->s.eType == ET_PLAYER) {
      // keep players around if they are alive and active (so sounds dont get
      // messed up)
      if (!(ent->s.eFlags & EF_DEAD)) {
        ent->s.eFlags |=
            EF_NODRAW; // don't draw, just process events and sounds
        SV_AddEntToSnapshot(svEnt, ent, eNums);
      }
    }
  }
}

/*
=============
SV_BuildClientSnapshot

Decides which entities are going to be visible to the client, and
copies off the playerstate and areabits.

This properly handles multiple recursive portals, but the render
currently doesn't.

For viewing through other player's eyes, clent can be something other than
client->gentity
=============
*/
static void SV_BuildClientSnapshot(client_t *client) {
  // bump the counter used to prevent double adding
  sv.snapshotCounter++;

  // this is the frame we are creating
  clientSnapshot_t *frame =
      &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

  // clear everything in this snapshot
  snapshotEntityNumbers_t entityNumbers;
  entityNumbers.numSnapshotEntities = 0;
  memset(frame->areabits, 0, sizeof(frame->areabits));

  sharedEntity_t *clent = client->gentity;
  if (!clent || client->state == CS_ZOMBIE) {
    return;
  }

  // grab the current PlayerState
  PlayerState *ps = SV_GameClientNum((int)(client - svs.clients));
  frame->ps = *ps;

  // never send client's own entity, because it can
  // be regenerated from the playerstate
  int clientNum = frame->ps.clientNum;
  if (clientNum < 0 || clientNum >= MAX_GENTITIES) {
    Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
    return; // keep the linter happy, ERR_DROP does not return
  }
  ServerEntity *svEnt = &sv.svEntities[clientNum];

  svEnt->snapshotCounter = sv.snapshotCounter;

  // find the client's viewpoint
  vec3_t org;
  VectorCopy(ps->origin, org);
  org[2] += ps->viewheight;

  if (frame->ps.leanf != 0) {
    vec3_t right, v3ViewAngles;
    VectorCopy(ps->viewangles, v3ViewAngles);
    v3ViewAngles[2] += frame->ps.leanf / 2.0f;
    AngleVectors(v3ViewAngles, nullptr, right, nullptr);
    VectorMA(org, frame->ps.leanf, right, org);
  }

  // add all the entities directly visible to the eye, which
  // may include portal entities that merge other viewpoints
  SV_AddEntitiesVisibleFromPoint(org, frame, &entityNumbers, false);

  // if there were portals visible, there may be out of order entities
  // in the list which will need to be resorted for the delta compression
  // to work correctly.  This also catches the error condition
  // of an entity being included twice.
  qsort(entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
        sizeof(entityNumbers.snapshotEntities[0]), SV_QsortEntityNumbers);

  // now that all viewpoint's areabits have been OR'd together, invert
  // all of them to make it a mask vector, which is what the renderer wants
  for (int i = 0; i < MAX_MAP_AREA_BYTES / 4; i++) {
    ((int *)frame->areabits)[i] = ((int *)frame->areabits)[i] ^ -1;
  }

  // copy the entity states out
  frame->num_entities = 0;
  frame->first_entity = svs.nextSnapshotEntities;
  for (int i = 0; i < entityNumbers.numSnapshotEntities; i++) {
    sharedEntity_t *ent = SV_GentityNum(entityNumbers.snapshotEntities[i]);
    EntityState *state = &svs.snapshotEntities[svs.nextSnapshotEntities %
                                                 svs.numSnapshotEntities];
    *state = ent->s;
    svs.nextSnapshotEntities++;
    // this should never hit, map should always be restarted first in SV_Frame
    if (svs.nextSnapshotEntities >= 0x7FFFFFFE) {
      Com_Error(ERR_FATAL, "svs.nextSnapshotEntities wrapped");
    }
    frame->num_entities++;
  }
}

/*
====================
SV_RateMsec

Return the number of msec a given size message is supposed
to take to clear, based on the current rate
====================
*/
#define HEADER_RATE_BYTES 48 // include our header, IP header, and some overhead
static int SV_RateMsec(client_t *client, int messageSize) {
  // individual messages will never be larger than fragment size
  if (messageSize > 1500) {
    messageSize = 1500;
  }
  int rate = client->rate;
  if (sv_maxRate->integer) {
    if (sv_maxRate->integer < 1000) {
      Cvar_Set("sv_MaxRate", "1000");
    }
    if (sv_maxRate->integer < rate) {
      rate = sv_maxRate->integer;
    }
  }
  int rateMsec = (messageSize + HEADER_RATE_BYTES) * 1000 / rate;

  return rateMsec;
}

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
void SV_SendMessageToClient(msg_t *msg, client_t *client) {
  // record information about the message
  client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize =
      msg->cursize;
  client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent =
      svs.time;
  client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked =
      -1;

  // send the datagram
  SV_Netchan_Transmit(client, msg); // msg->cursize, msg->data );

  // set nextSnapshotTime based on rate and requested number of updates

  // local clients get snapshots every frame
  client->nextSnapshotTime = svs.time - 1;
  return;
}

/*
=======================
SV_SendClientSnapshot

Also called by SV_FinalMessage

=======================
*/
void SV_SendClientSnapshot(client_t *client) {
  // RF, AI don't need snapshots built
  if (client->gentity && client->gentity->r.svFlags & SVF_CASTAI) {
    return;
  }

  // build the snapshot
  SV_BuildClientSnapshot(client);

  // bots need to have their snapshots build, but
  // the query them directly without needing to be sent
  if (client->gentity && client->gentity->r.svFlags & SVF_BOT) {
    return;
  }

  uint8_t msg_buf[MAX_MSGLEN];
  msg_t msg;
  MSG_Init(&msg, msg_buf, sizeof(msg_buf));
  msg.allowoverflow = true;

  // NOTE, MRE: all server->client messages now acknowledge
  // let the client know which reliable clientCommands we have received
  MSG_WriteLong(&msg, client->lastClientCommand);

  // (re)send any reliable server commands
  SV_UpdateServerCommandsToClient(client, &msg);

  // send over all the relevant EntityState
  // and the PlayerState
  SV_WriteSnapshotToClient(client, &msg);

  // check for overflow
  if (msg.overflowed) {
    Com_Printf("WARNING: msg overflowed for %s\n", client->name);
    MSG_Clear(&msg);
  }

  SV_SendMessageToClient(&msg, client);
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages() {
  // send a message to each connected client
  for (int i = 0; i < sv_maxclients->integer; i++) {
    client_t *c = &svs.clients[i];
    if (!c->state) {
      continue; // not connected
    }

    if (svs.time < c->nextSnapshotTime) {
      continue; // not time yet
    }

    // send additional message fragments if the last message
    // was too large to send at once
    if (c->netchan.unsentFragments) {
      c->nextSnapshotTime =
          svs.time + SV_RateMsec(c, c->netchan.unsentLength -
                                        c->netchan.unsentFragmentStart);
      SV_Netchan_TransmitNextFragment(&c->netchan);
      continue;
    }

    // generate and send a new message
    SV_SendClientSnapshot(c);
  }
}
