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
 * name:		be_ai_chat.h
 *
 * desc:		char AI
 *
 *
 *****************************************************************************/

#define MAX_MESSAGE_SIZE        150         //limit in game dll
#define MAX_CHATTYPE_NAME       32
#define MAX_MATCHVARIABLES      8

#define CHAT_GENDERLESS         0
#define CHAT_GENDERFEMALE       1
#define CHAT_GENDERMALE         2

#define CHAT_ALL                    0
#define CHAT_TEAM                   1

//a console message
typedef struct bot_consolemessage_s
{
	int handle;
	float time;                                         //message time
	int type;                                           //message type
	char message[MAX_MESSAGE_SIZE];             //message
	struct bot_consolemessage_s *prev, *next;   //prev and next in list
} bot_consolemessage_t;

//match variable
typedef struct bot_matchvariable_s
{
	char *ptr;
	int length;
} bot_matchvariable_t;
//returned to AI when a match is found
typedef struct bot_match_s
{
	char string[MAX_MESSAGE_SIZE];
	int type;
	int subtype;
	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
} bot_match_t;





