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

// q_shared.c -- stateless support routines that are included in each code dll
#include "q_splineshared.h"

/*
============================================================================

GROWLISTS

============================================================================
*/

void Com_InitGrowList( growList_t *list, int maxElements ) {
	list->maxElements = maxElements;
	list->currentElements = 0;
	list->elements = (void **)malloc( list->maxElements * sizeof( void * ) );
}

int Com_AddToGrowList( growList_t *list, void *data ) {
	void    **old;

	if ( list->currentElements != list->maxElements ) {
		list->elements[list->currentElements] = data;
		return list->currentElements++;
	}

	// grow, reallocate and move
	old = list->elements;

	if ( list->maxElements < 0 ) {
		Com_Error( ERR_FATAL, "Com_AddToGrowList: maxElements = %i", list->maxElements );
	}

	if ( list->maxElements == 0 ) {
		// initialize the list to hold 100 elements
		Com_InitGrowList( list, 100 );
		return Com_AddToGrowList( list, data );
	}

	list->maxElements *= 2;

	Com_DPrintf( "Resizing growlist to %i maxElements\n", list->maxElements );

	list->elements = (void **)malloc( list->maxElements * sizeof( void * ) );

	if ( !list->elements ) {
		Com_Error( ERR_DROP, "Growlist alloc failed" );
        return 0; // keep the linter happy, ERR_DROP does not return
	}

	memcpy( list->elements, old, list->currentElements * sizeof( void * ) );

	free( old );

	return Com_AddToGrowList( list, data );
}

void *Com_GrowListElement( const growList_t *list, int index ) {
	if ( index < 0 || index >= list->currentElements ) {
		Com_Error( ERR_DROP, "Com_GrowListElement: %i out of range of %i",
				   index, list->currentElements );
        return nullptr; // keep the linter happy, ERR_DROP does not return
	}
	return list->elements[index];
}

int Com_IndexForGrowListElement( const growList_t *list, const void *element ) {
	int i;

	for ( i = 0 ; i < list->currentElements ; i++ ) {
		if ( list->elements[i] == element ) {
			return i;
		}
	}
	return -1;
}

/*
============
Com_SkipPath
============
*/
char *Com_SkipPath( char *pathname ) {
	char    *last;

	last = pathname;
	while ( *pathname )
	{
		if ( *pathname == '/' ) {
			last = pathname + 1;
		}
		pathname++;
	}
	return last;
}

/*
============
Com_StripExtension
============
*/
void Com_StripExtension( const char *in, char *out ) {
	while ( *in && *in != '.' ) {
		*out++ = *in++;
	}
	*out = 0;
}


/*
==================
Com_DefaultExtension
==================
*/
void Com_DefaultExtension( char *path, int maxSize, const char *extension ) {
	char oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path ) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	snprintf( path, maxSize, "%s%s", oldPath, extension );
}

void Info_SetValueForKey( char *s, const char *key, const char *value );

/*
===============
Com_ParseInfos
===============
*/
int Com_ParseInfos( const char *buf, int max, char infos[][MAX_INFO_STRING] ) {
	const char  *token;
	int count;
	char key[MAX_TOKEN_CHARS];

	count = 0;

	while ( 1 ) {
		token = Com_Parse( &buf );
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

		infos[count][0] = 0;
		while ( 1 ) {
			token = Com_Parse( &buf );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = Com_ParseOnLine( &buf );
			if ( !token[0] ) {
				token = "<nullptr>";
			}
			Info_SetValueForKey( infos[count], key, token );
		}
		count++;
	}

	return count;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
ParseHex
===============
*/
int ParseHex( const char *text ) {
	int value;
	int c;

	value = 0;
	while ( ( c = *text++ ) != 0 ) {
		if ( c >= '0' && c <= '9' ) {
			value = value * 16 + c - '0';
			continue;
		}
		if ( c >= 'a' && c <= 'f' ) {
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if ( c >= 'A' && c <= 'F' ) {
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}
