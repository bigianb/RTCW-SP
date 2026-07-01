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

#pragma once

typedef enum {
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_NEED_CD,
	UIMENU_ENDGAME,
	UIMENU_BAD_CD_KEY,
	UIMENU_TEAM,
	UIMENU_PREGAME,
	UIMENU_POSTGAME,
	UIMENU_NOTEBOOK,
	UIMENU_CLIPBOARD,
	UIMENU_HELP,
	UIMENU_BOOK1, 
	UIMENU_BOOK2, 
	UIMENU_BOOK3, 
	UIMENU_BRIEFING, 
	UIMENU_WM_QUICKMESSAGE
} uiMenuCommand_t;

void UI_Init( void );
void UI_Shutdown( void );
void UI_KeyEvent( int key, bool down );
void UI_MouseEvent( int dx, int dy );
void UI_Refresh( int realtime );
uiMenuCommand_t UI_GetActiveMenu( void );
void	UI_SetActiveMenu( uiMenuCommand_t menu );
bool UI_IsFullscreen( void );
bool UI_ConsoleCommand( int realTime );

void UI_FeederAddItem( float feederID, const char *name, int index );
int UI_FeederCount( float feederID );
void UI_FeederSelection( float feederID, int index );
qhandle_t UI_FeederItemImage( float feederID, int index );
const char *UI_FeederItemText( float feederID, int index, int column, qhandle_t *handle );
void UI_RunMenuScript( const char **args );
void UI_DrawTopBottom( float x, float y, float w, float h, float size );

int UI_PlayCinematic( const char *name, float x, float y, float w, float h );
void UI_StopCinematic( int handle );
void UI_DrawCinematic( int handle, float x, float y, float w, float h );
void UI_RunCinematicFrame( int handle );
const char *UI_translateString( const char *inString );
const char *UI_FileText( char *fileName );
bool UI_OwnerDrawVisible( int flags );
int UI_OwnerDrawWidth( int ownerDraw, int font, float scale );
void UI_Pause( bool b );

int Text_Height( const char *text, int font, float scale, int limit );

#define SORT_SAVENAME       0
#define SORT_SAVETIME       1
