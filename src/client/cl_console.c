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

// console.c

#include "client.h"
#include "cgame/cg_local.h"

int g_console_field_width = 78;


#define COLNSOLE_COLOR  COLOR_WHITE //COLOR_BLACK

#define NUM_CON_TIMES 4

//#define		CON_TEXTSIZE	32768
#define     CON_TEXTSIZE    65536   // (SA) DM want's more console...

typedef struct {
	qboolean initialized;

	short text[CON_TEXTSIZE];
	int current;            // line where next message will be printed
	int x;                  // offset in current line for next print
	int display;            // bottom of console displays this line

	int linewidth;          // characters across screen
	int totallines;         // total lines in console scrollback

	float xadjust;          // for wide aspect screens

	float displayFrac;      // aproaches finalFrac at scr_conspeed
	float finalFrac;        // 0.0 to 1.0 lines of console to display

	int vislines;           // in scanlines

	int times[NUM_CON_TIMES];       // cls.realtime time the line was generated
	// for transparent notify lines
	vec4_t color;
} console_t;

extern console_t con;

console_t con;

cvar_t      *con_debug;
cvar_t      *con_conspeed;
cvar_t      *con_notifytime;

#define DEFAULT_CONSOLE_WIDTH   78

vec4_t console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void ) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();
	cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f( void ) {
	chat_playerNum = -1;
	chat_team = qfalse;
//	chat_limbo = qfalse;		// NERVE - SMF
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f( void ) {
	chat_playerNum = -1;
	chat_team = qtrue;
//	chat_limbo = qfalse;		// NERVE - SMF
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f( void ) {
	chat_playerNum = CG_CrosshairPlayer();
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
//	chat_limbo = qfalse;		// NERVE - SMF
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f( void ) {
	chat_playerNum = CG_LastAttacker();
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
//	chat_limbo = qfalse;		// NERVE - SMF
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

// NERVE - SMF
/*
================
Con_StartLimboMode_f
================
*/
void Con_StartLimboMode_f( void ) {
//	chat_playerNum = -1;
//	chat_team = qfalse;
	chat_limbo = qtrue;     // NERVE - SMF
//	Field_Clear( &chatField );
//	chatField.widthInChars = 30;

//	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_StopLimboMode_f
================
*/
void Con_StopLimboMode_f( void ) {
//	chat_playerNum = -1;
//	chat_team = qfalse;
	chat_limbo = qfalse;        // NERVE - SMF
//	Field_Clear( &chatField );
//	chatField.widthInChars = 30;

//	cls.keyCatchers &= ~KEYCATCH_MESSAGE;
}
// -NERVE - SMF

/*
================
Con_Clear_f
================
*/
void Con_Clear_f( void ) {
	int i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';
	}

	Con_Bottom();       // go to end
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f( void ) {
	int l, x, i;
	short   *line;
	fileHandle_t f;
	char buffer[1024];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "usage: condump <filename>\n" );
		return;
	}

	Com_Printf( "Dumped console text to %s.\n", Cmd_Argv( 1 ) );

#ifdef __MACOS__    //DAJ MacOS file typing
	{
		extern _MSL_IMP_EXP_C long _fcreator, _ftype;
		_ftype = 'TEXT';
		_fcreator = 'R*ch';
	}
#endif
	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if ( !f ) {
		Com_Printf( "ERROR: couldn't open.\n" );
		return;
	}

	// skip empty lines
	for ( l = con.current - con.totallines + 1 ; l <= con.current ; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for ( x = 0 ; x < con.linewidth ; x++ )
			if ( ( line[x] & 0xff ) != ' ' ) {
				break;
			}
		if ( x != con.linewidth ) {
			break;
		}
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for ( i = 0; i < con.linewidth; i++ )
			buffer[i] = line[i] & 0xff;
		for ( x = con.linewidth - 1 ; x >= 0 ; x-- )
		{
			if ( buffer[x] == ' ' ) {
				buffer[x] = 0;
			} else {
				break;
			}
		}
		strcat( buffer, "\n" );
		FS_Write( buffer, strlen( buffer ), f );
	}

	FS_FCloseFile( f );
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int i;

	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}



/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void ) {
	int i, j, width, oldwidth, oldtotallines, numlines, numchars;
	MAC_STATIC short tbuf[CON_TEXTSIZE];

	width = ( SCREEN_WIDTH / SMALLCHAR_WIDTH ) - 2;

	if ( width == con.linewidth ) {
		return;
	}

	if ( width < 1 ) {        // video hasn't been initialized yet
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for ( i = 0; i < CON_TEXTSIZE; i++ )

			con.text[i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';
	} else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if ( con.totallines < numlines ) {
			numlines = con.totallines;
		}

		numchars = oldwidth;

		if ( con.linewidth < numchars ) {
			numchars = con.linewidth;
		}

		memcpy( tbuf, con.text, CON_TEXTSIZE * sizeof( short ) );
		for ( i = 0; i < CON_TEXTSIZE; i++ )

			con.text[i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';


		for ( i = 0 ; i < numlines ; i++ )
		{
			for ( j = 0 ; j < numchars ; j++ )
			{
				con.text[( con.totallines - 1 - i ) * con.linewidth + j] =
					tbuf[( ( con.current - i + oldtotallines ) %
						   oldtotallines ) * oldwidth + j];
			}
		}

		Con_ClearNotify();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init( void ) {
	int i;

	con_notifytime = Cvar_Get( "con_notifytime", "3", 0 );
	con_conspeed = Cvar_Get( "scr_conspeed", "3", 0 );
	con_debug = Cvar_Get( "con_debug", "0", CVAR_ARCHIVE ); //----(SA)	added

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "messagemode", Con_MessageMode_f );
	Cmd_AddCommand( "messagemode2", Con_MessageMode2_f );
	Cmd_AddCommand( "messagemode3", Con_MessageMode3_f );
	Cmd_AddCommand( "messagemode4", Con_MessageMode4_f );
	Cmd_AddCommand( "startLimboMode", Con_StartLimboMode_f );     // NERVE - SMF
	Cmd_AddCommand( "stopLimboMode", Con_StopLimboMode_f );           // NERVE - SMF
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed( void ) {
	int i;

	// mark time for transparent overlay
	if ( con.current >= 0 ) {
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if ( con.display == con.current ) {
		con.display++;
	}
	con.current++;
	for ( i = 0; i < con.linewidth; i++ )
		con.text[( con.current % con.totallines ) * con.linewidth + i] = ( ColorIndex( COLNSOLE_COLOR ) << 8 ) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char *txt ) {
	int y;
	int c, l;
	int color;

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	if ( !con.initialized ) {
		con.color[0] =
			con.color[1] =
				con.color[2] =
					con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize();
		con.initialized = qtrue;
	}

	color = ColorIndex( COLNSOLE_COLOR );

	while ( ( c = *txt ) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *( txt + 1 ) );
			txt += 2;
			continue;
		}

		// count word length
		for ( l = 0 ; l < con.linewidth ; l++ ) {
			if ( txt[l] <= ' ' ) {
				break;
			}

		}

		// word wrap
		if ( l != con.linewidth && ( con.x + l >= con.linewidth ) ) {
			Con_Linefeed();

		}

		txt++;

		switch ( c )
		{
		case '\n':
			Con_Linefeed();
			break;
		case '\r':
			con.x = 0;
			break;
		default:    // display character and advance
			y = con.current % con.totallines;
			con.text[y * con.linewidth + con.x] = ( color << 8 ) | c;
			con.x++;
			if ( con.x >= con.linewidth ) {

				Con_Linefeed();
				con.x = 0;
			}
			break;
		}
	}


	// mark time for transparent overlay

	if ( con.current >= 0 ) {
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput( void ) {
	int y;

	if ( cls.state != CA_DISCONNECTED && !( cls.keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * 2 );

	re.SetColor( con.color );

	SCR_DrawSmallChar( con.xadjust + 1 * SMALLCHAR_WIDTH, y, ']' );

	Field_Draw( &g_consoleField, con.xadjust + 2 * SMALLCHAR_WIDTH, y,
				SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify( void ) {
	int x, v;
	short   *text;
	int i;
	int time;
	int skip;
	int currentColor;

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	v = 0;
	for ( i = con.current - NUM_CON_TIMES + 1 ; i <= con.current ; i++ )
	{
		if ( i < 0 ) {
			continue;
		}
		time = con.times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = cls.realtime - time;
		if ( time > con_notifytime->value * 1000 ) {
			continue;
		}
		text = con.text + ( i % con.totallines ) * con.linewidth;

		if ( cl.snap.ps.pm_type != PM_INTERMISSION && cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) {
			continue;
		}

		for ( x = 0 ; x < con.linewidth ; x++ ) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( ( text[x] >> 8 ) & 7 ) != currentColor ) {
				currentColor = ( text[x] >> 8 ) & 7;
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor( NULL );

	if ( cls.keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) {
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE ) {
		if ( chat_team ) {
			SCR_DrawBigString( 8, v, "say_team:", 1.0f );
			skip = 11;
		} else
		{
			SCR_DrawBigString( 8, v, "say:", 1.0f );
			skip = 5;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
					   SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/

void Con_DrawSolidConsole( float frac ) {
	int i, x, y;
	int rows;
	short           *text;
	int row;
	int lines;
//	qhandle_t		conShader;
	int currentColor;
	vec4_t color;

	lines = cls.glconfig.vidHeight * frac;
	if ( lines <= 0 ) {
		return;
	}

	if ( lines > cls.glconfig.vidHeight ) {
		lines = cls.glconfig.vidHeight;
	}

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1 ) {
		y = 0;
	} else {
		SCR_DrawPic( 0, 0, SCREEN_WIDTH, y, cls.consoleShader );

		if ( frac >= 0.5f ) {  // only draw when the console is down all the way (for now)
			color[0] = color[1] = color[2] = frac * 2.0f;
			color[3] = 1.0f;
			re.SetColor( color );

			// draw the logo
			SCR_DrawPic( 192, 70, 256, 128, cls.consoleShader2 );
			re.SetColor( NULL );
		}
	}

	color[0] = 0;
	color[1] = 0;
	color[2] = 0;
	color[3] = 0.6f;
	SCR_FillRect( 0, y, SCREEN_WIDTH, 2, color );

	// draw the version number

	re.SetColor( g_color_table[ColorIndex( COLNSOLE_COLOR )] );

	i = strlen( Q3_VERSION );

	for ( x = 0 ; x < i ; x++ ) {

		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH,

						   ( lines - ( SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2 ) ), Q3_VERSION[x] );

	}


	// draw the text
	con.vislines = lines;
	rows = ( lines - SMALLCHAR_WIDTH ) / SMALLCHAR_WIDTH;     // rows of text to draw

	y = lines - ( SMALLCHAR_HEIGHT * 3 );

	// draw from the bottom up
	if ( con.display != con.current ) {
		// draw arrows to show the buffer is backscrolled
		re.SetColor( g_color_table[ColorIndex( COLOR_WHITE )] );
		for ( x = 0 ; x < con.linewidth ; x += 4 )
			SCR_DrawSmallChar( con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	for ( i = 0 ; i < rows ; i++, y -= SMALLCHAR_HEIGHT, row-- )
	{
		if ( row < 0 ) {
			break;
		}
		if ( con.current - row >= con.totallines ) {
			// past scrollback wrap point
			continue;
		}

		text = con.text + ( row % con.totallines ) * con.linewidth;

		for ( x = 0 ; x < con.linewidth ; x++ ) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( ( text[x] >> 8 ) & 7 ) != currentColor ) {
				currentColor = ( text[x] >> 8 ) & 7;
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(  con.xadjust + ( x + 1 ) * SMALLCHAR_WIDTH, y, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput();

	re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize();

	// if disconnected, render console full screen
	switch ( cls.state ) {
	case CA_UNINITIALIZED:
	case CA_CONNECTING:         // sending request packets to the server
	case CA_CHALLENGING:        // sending challenge packets to the server
	case CA_CONNECTED:          // netchan_t established, getting gamestate
	case CA_PRIMED:             // got gamestate, waiting for first frame
	case CA_LOADING:            // only during cgame initialization, never during main loop
		if ( !con_debug->integer ) { // these are all 'no console at all' when con_debug is not set
			return;
		}

		if ( cls.keyCatchers & KEYCATCH_UI ) {
			return;
		}

		Con_DrawSolidConsole( 1.0 );
		return;

	case CA_DISCONNECTED:       // not talking to a server
		if ( !( cls.keyCatchers & KEYCATCH_UI ) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
		break;

	case CA_ACTIVE:             // game views should be displayed
		if ( con.displayFrac ) {
			if ( con_debug->integer == 2 ) {    // 2 means draw full screen console at '~'
//					Con_DrawSolidConsole( 1.0f );
				Con_DrawSolidConsole( con.displayFrac * 2.0f );
				return;
			}
		}

		break;


	case CA_CINEMATIC:          // playing a cinematic or a static pic, not connected to a server
	default:
		break;
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		Con_DrawNotify();       // draw notify lines
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole( void ) {
	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		con.finalFrac = 0.5;        // half screen
	} else {
		con.finalFrac = 0;              // none visible

	}
	// scroll towards the destination height
	if ( con.finalFrac < con.displayFrac ) {
		con.displayFrac -= con_conspeed->value * cls.realFrametime * 0.001;
		if ( con.finalFrac > con.displayFrac ) {
			con.displayFrac = con.finalFrac;
		}

	} else if ( con.finalFrac > con.displayFrac )     {
		con.displayFrac += con_conspeed->value * cls.realFrametime * 0.001;
		if ( con.finalFrac < con.displayFrac ) {
			con.displayFrac = con.finalFrac;
		}
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if ( con.display > con.current ) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;              // none visible
	con.displayFrac = 0;
}
