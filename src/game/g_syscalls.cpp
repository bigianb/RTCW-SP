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
#include "g_local.h"
#include "qcommon.h"
#include "../server/server.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_aas_bsp.h"
#include "../botlib/be_aas_entity.h"
#include "../botlib/be_aas_main.h"
#include "../botlib/be_aas_move.h"
#include "../botlib/be_aas_reach.h"
#include "../botlib/be_aas_route.h"
#include "../botlib/be_aas_routetable.h"
#include "../botlib/be_aas_sample.h"
#include "../game/be_ea.h"
#include "../botlib/botlib.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"


void trap_BotUserCommand( int clientNum, UserCmd *ucmd ) {
	SV_ClientThink( &svs.clients[clientNum], ucmd );
}

