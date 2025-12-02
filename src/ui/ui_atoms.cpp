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

#include "ui_local.h"
#include "../renderer/tr_local.h"

uiStatic_t uis;
bool m_entersound;              // after a frame, so caching won't disrupt the sound

float UI_ClampCvar( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

char *UI_Argv( int arg ) {
	static char buffer[MAX_STRING_CHARS];

	Cmd_ArgvBuffer( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char buffer[MAX_STRING_CHARS];

	Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}


bool UI_ConsoleCommand( int realTime ) {
	char    *cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	cmd = UI_Argv( 0 );


	if ( Q_stricmp( cmd, "ui_load" ) == 0 ) {
		UI_Load();
		return true;
	}

	if ( Q_stricmp( cmd, "remapShader" ) == 0 ) {
		if ( Cmd_Argc() == 4 ) {
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			Q_strncpyz( shader1, UI_Argv( 1 ), sizeof( shader1 ) );
			Q_strncpyz( shader2, UI_Argv( 2 ), sizeof( shader2 ) );
			trap_R_RemapShader( shader1, shader2, UI_Argv( 3 ) );
			return true;
		}
	}

	return false;
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {

	*x *= uiInfo.uiDC.xscale;
	*y *= uiInfo.uiDC.yscale;
	*w *= uiInfo.uiDC.xscale;
	*h *= uiInfo.uiDC.yscale;

}

void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t hShader;

	hShader = RE_RegisterShaderNoMip( picname );
	UI_AdjustFrom640( &x, &y, &width, &height );
	RE_StretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float s0;
	float s1;
	float t0;
	float t1;

	if ( w < 0 ) {   // flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1;
	}

	if ( h < 0 ) {   // flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1;
	}

	UI_AdjustFrom640( &x, &y, &w, &h );
	RE_StretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	RE_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	RE_StretchPic( x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );

	RE_SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	RE_SetColor( rgba );
}

void UI_UpdateScreen( void ) {
	SCR_UpdateScreen();
}


void UI_DrawTextBox( int x, int y, int width, int lines ) {
	UI_FillRect( x + BIGCHAR_WIDTH / 2, y + BIGCHAR_HEIGHT / 2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorBlack );
	UI_DrawRect( x + BIGCHAR_WIDTH / 2, y + BIGCHAR_HEIGHT / 2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, 1.0f, colorWhite );
}

bool UI_CursorInRect( int x, int y, int width, int height ) {
	if ( uiInfo.uiDC.cursorx < x ||
		 uiInfo.uiDC.cursory < y ||
		 uiInfo.uiDC.cursorx > x + width ||
		 uiInfo.uiDC.cursory > y + height ) {
		return false;
	}

	return true;
}
