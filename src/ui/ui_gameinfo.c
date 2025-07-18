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
//
// gameinfo.c
//

#include "ui_local.h"


//
// arena and bot info
//


int ui_numBots;
static char     *ui_botInfos[MAX_BOTS];

static int ui_numArenas;
static char     *ui_arenaInfos[MAX_ARENAS];

// TTimo: unused
//static int		ui_numSinglePlayerArenas;
//static int		ui_numSpecialSinglePlayerArenas;

/*
===============
UI_ParseInfos
===============
*/
int UI_ParseInfos( char *buf, int max, char *infos[] ) {
	char    *token;
	int count;
	char key[MAX_TOKEN_CHARS];
	char info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = UI_Alloc( strlen( info ) + strlen( "\\num\\" ) + strlen( va( "%d", MAX_ARENAS ) ) + 1 );
		if ( infos[count] ) {
			strcpy( infos[count], info );
			count++;
		}
	}
	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile( char *filename ) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_UI_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_UI_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas] );
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas( void ) {
	int numdirs;
	vmCvar_t arenasFile;
	char filename[128];
	char dirlist[1024];
	char*       dirptr;
	int i, n;
	int dirlen;
	char        *type;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM );
	if ( *arenasFile.string ) {
		UI_LoadArenasFromFile( arenasFile.string );
	} else {
		UI_LoadArenasFromFile( "scripts/arenas.txt" );
	}

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, 1024 );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 ) {
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		UI_LoadArenasFromFile( filename );
	}
	trap_UI_Print( va( "%i arenas parsed\n", ui_numArenas ) );
	if ( UI_OutOfMemory() ) {
		trap_UI_Print( S_COLOR_YELLOW "WARNING: not anough memory in pool to load all arenas\n" );
	}

	for ( n = 0; n < ui_numArenas; n++ ) {
		// determine type

		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
		uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc( Info_ValueForKey( ui_arenaInfos[n], "map" ) );
		uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc( Info_ValueForKey( ui_arenaInfos[n], "longname" ) );
		uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
		uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc( va( "levelshots/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName ) );
		uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

		type = Info_ValueForKey( ui_arenaInfos[n], "type" );
		// if no type specified, it will be treated as "ffa"
		if ( *type ) {
			if ( strstr( type, "ffa" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_FFA );
			}
			if ( strstr( type, "tourney" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_TOURNAMENT );
			}
			if ( strstr( type, "ctf" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_CTF );
			}
#ifdef MISSIONPACK
			if ( strstr( type, "oneflag" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_1FCTF );
			}
			if ( strstr( type, "overload" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_OBELISK );
			}
			if ( strstr( type, "harvester" ) ) {
				uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_HARVESTER );
			}
#endif  // #ifdef MISSIONPACK
		} else {
			uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << GT_FFA );
		}

		uiInfo.mapCount++;
		if ( uiInfo.mapCount >= MAX_MAPS ) {
			break;
		}
	}
}


/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile( char *filename ) {
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_UI_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_UI_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	COM_Compress( buf );

	ui_numBots += UI_ParseInfos( buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots] );
}

/*
===============
UI_LoadBots
===============
*/
void UI_LoadBots( void ) {
	vmCvar_t botsFile;
	int numdirs;
	char filename[128];
	char dirlist[1024];
	char*       dirptr;
	int i;
	int dirlen;

	ui_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM );
	if ( *botsFile.string ) {
		UI_LoadBotsFromFile( botsFile.string );
	} else {
		UI_LoadBotsFromFile( "scripts/bots.txt" );
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList( "scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 ) {
		dirlen = strlen( dirptr );
		strcpy( filename, "scripts/" );
		strcat( filename, dirptr );
		UI_LoadBotsFromFile( filename );
	}
	trap_UI_Print( va( "%i bots parsed\n", ui_numBots ) );
}


/*
===============
UI_GetBotInfoByNumber
===============
*/
char *UI_GetBotInfoByNumber( int num ) {
	if ( num < 0 || num >= ui_numBots ) {
		trap_UI_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return ui_botInfos[num];
}


/*
===============
UI_GetBotInfoByName
===============
*/
char *UI_GetBotInfoByName( const char *name ) {
	int n;
	char    *value;

	for ( n = 0; n < ui_numBots ; n++ ) {
		value = Info_ValueForKey( ui_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

int UI_GetNumBots() {
	return ui_numBots;
}


char *UI_GetBotNameByNumber( int num ) {
	char *info = UI_GetBotInfoByNumber( num );
	if ( info ) {
		return Info_ValueForKey( info, "name" );
	}
	return "Sarge";
}
