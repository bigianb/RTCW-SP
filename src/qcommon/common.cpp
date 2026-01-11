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

// common.c -- misc functions used in client and server

#include "../game/q_shared.h"
#include "qcommon.h"
#include <setjmp.h>

#define MAXPRINTMSG 4096

#define MAX_NUM_ARGVS   50

#define MIN_DEDICATED_COMHUNKMEGS 1
#define MIN_COMHUNKMEGS 54      // RF, optimizing
#define DEF_COMHUNKMEGS "72"
#define DEF_COMZONEMEGS "30"

int com_argc;
char    *com_argv[MAX_NUM_ARGVS + 1];

extern char cl_cdkey[34];

jmp_buf abortframe;     // an ERR_DROP occured, exit the entire frame


FILE *debuglogfile;
static fileHandle_t logfile;
fileHandle_t com_journalFile;               // events are written here
fileHandle_t com_journalDataFile;           // config files are written here

cvar_t  *com_viewlog;
cvar_t  *com_speeds;
cvar_t  *com_developer;

cvar_t  *com_timescale;
cvar_t  *com_fixedtime;
cvar_t  *com_dropsim;       // 0.0 to 1.0, simulated packet drops
cvar_t  *com_journal;
cvar_t  *com_maxfps;

cvar_t  *com_sv_running;
cvar_t  *com_cl_running;
cvar_t  *com_logfile;       // 1 = buffer log, 2 = flush after each print
cvar_t  *com_showtrace;
cvar_t  *com_version;
cvar_t  *com_blood;
cvar_t  *com_buildScript;   // for automated data building scripts
cvar_t  *com_introPlayed;
cvar_t  *cl_paused;
cvar_t  *sv_paused;
cvar_t  *com_cameraMode;
#if defined( _WIN32 ) && defined( _DEBUG )
cvar_t  *com_noErrorInterrupt;
#endif
cvar_t  *com_recommendedSet;

// Rafael Notebook
cvar_t  *cl_notebook;

cvar_t  *com_hunkused;      // Ridah

// com_speeds times
int time_game;
int time_frontend;          // renderer frontend time
int time_backend;           // renderer backend time

int com_frameTime;
int com_frameMsec;
int com_frameNumber;

bool com_errorEntered;
bool com_fullyInitialized;

char com_errorMessage[MAXPRINTMSG];

void Com_WriteConfig_f( void );
void CIN_CloseAllVideos();

//============================================================================

static char *rd_buffer;
static int rd_buffersize;
static void ( *rd_flush )( char *buffer );

void Com_BeginRedirect( char *buffer, int buffersize, void ( *flush )( char *) ) {
	if ( !buffer || !buffersize || !flush ) {
		return;
	}
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect( void ) {
	if ( rd_flush ) {
		rd_flush( rd_buffer );
	}

	rd_buffer = nullptr;
	rd_buffersize = 0;
	rd_flush = nullptr;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
void  Com_Printf( const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];
	static bool opening_qconsole = false;

	va_start( argptr,fmt );
	vsnprintf( msg, MAXPRINTMSG, fmt,argptr );
	va_end( argptr );

	if ( rd_buffer ) {
		if ( ( strlen( msg ) + strlen( rd_buffer ) ) > ( rd_buffersize - 1 ) ) {
			rd_flush( rd_buffer );
			*rd_buffer = 0;
		}
		Q_strcat( rd_buffer, rd_buffersize, msg );
		return;
	}

	CL_ConsolePrint( msg );

	// echo to dedicated console and early console
	Sys_Print( msg );

	// logfile
	if ( com_logfile && com_logfile->integer ) {
		// TTimo: only open the qconsole.log if the filesystem is in an initialized state
		//   also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		if ( !logfile && FS_Initialized() && !opening_qconsole ) {
			struct tm *newtime;
			time_t aclock;

			opening_qconsole = true;

			time( &aclock );
			newtime = localtime( &aclock );

			logfile = FS_FOpenFileWrite( "rtcwconsole.log" );    //----(SA)	changed name for Wolf
			Com_Printf( "logfile opened on %s\n", asctime( newtime ) );
			if ( com_logfile->integer > 1 ) {
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush( logfile );
			}

			opening_qconsole = false;
		}
		if ( logfile && FS_Initialized() ) {
			FS_Write( msg, strlen( msg ), logfile );
		}
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/
void  Com_DPrintf( const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];

	if ( !com_developer || !com_developer->integer ) {
		return;         // don't confuse non-developers with techie stuff...
	}

	va_start( argptr,fmt );
	vsnprintf( msg, MAXPRINTMSG, fmt,argptr );
	va_end( argptr );

	Com_Printf( "%s", msg );
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
[[noreturn]]
void  Com_Error( int code, const char *fmt, ... ) {
	va_list argptr;
	static int lastErrorTime;
	static int errorCount;
	int currentTime;

	// when we are running automated scripts, make sure we
	// know if anything failed
	if ( com_buildScript && com_buildScript->integer ) {

		// ERR_ENDGAME is not really an error, don't die if building a script
		if ( code != ERR_ENDGAME ) {
			code = ERR_FATAL;
		}
	}

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	currentTime = Sys_Milliseconds();
	if ( currentTime - lastErrorTime < 100 ) {
		if ( ++errorCount > 3 ) {
			code = ERR_FATAL;
		}
	} else {
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	if ( com_errorEntered ) {
		Sys_Error( "recursive error after: %s", com_errorMessage );
	}
	com_errorEntered = true;

	va_start( argptr,fmt );
	vsnprintf( com_errorMessage, MAXPRINTMSG, fmt,argptr );
	va_end( argptr );

	if ( code != ERR_DISCONNECT && code != ERR_ENDGAME ) {
		Cvar_Set( "com_errorMessage", com_errorMessage );
	}

	if ( code == ERR_SERVERDISCONNECT ) {
		CL_Disconnect( true );
		CL_FlushMemory();
		com_errorEntered = false;
		longjmp( abortframe, -1 );
	} else if ( code == ERR_ENDGAME ) {
		SV_Shutdown( "endgame" );
		if ( com_cl_running && com_cl_running->integer ) {
			CL_Disconnect( true );
			CL_FlushMemory();
			com_errorEntered = false;
			CL_EndgameMenu();
		}
		longjmp( abortframe, -1 );
	} else if ( code == ERR_DROP || code == ERR_DISCONNECT ) {
		Com_Printf( "********************\nERROR: %s\n********************\n", com_errorMessage );
		SV_Shutdown( va( "Server crashed: %s\n",  com_errorMessage ) );
		CL_Disconnect( true );
		CL_FlushMemory();
		com_errorEntered = false;
		longjmp( abortframe, -1 );

	} else {
		CL_Shutdown();
		SV_Shutdown( va( "Server fatal crashed: %s\n", com_errorMessage ) );
	}

	Com_Shutdown();

	Sys_Error( "%s", com_errorMessage );
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f( void ) {
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		SV_Shutdown( "Server quit\n" );
		CL_Shutdown();
		Com_Shutdown();
		FS_Shutdown( true );
	}
	Sys_Quit();
}



/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters seperate the commandLine string into multiple console
command lines.

All of these are valid:

quake3 +set test blah +map test
quake3 set test blah+map test
quake3 set test blah + map test

============================================================================
*/

#define MAX_CONSOLE_LINES   32
int com_numConsoleLines;
char    *com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
Com_ParseCommandLine

Break it up into multiple console lines
==================
*/
void Com_ParseCommandLine( char *commandLine ) {
	com_consoleLines[0] = commandLine;
	com_numConsoleLines = 1;

	while ( *commandLine ) {
		// look for a + seperating character
		// if commandLine came from a file, we might have real line seperators
		if ( *commandLine == '+' || *commandLine == '\n' ) {
			if ( com_numConsoleLines == MAX_CONSOLE_LINES ) {
				return;
			}
			com_consoleLines[com_numConsoleLines] = commandLine + 1;
			com_numConsoleLines++;
			*commandLine = 0;
		}
		commandLine++;
	}
}


/*
===================
Com_SafeMode

Check for "safe" on the command line, which will
skip loading of wolfconfig.cfg
===================
*/
bool Com_SafeMode( void ) {
	int i;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( !Q_stricmp( Cmd_Argv( 0 ), "safe" )
			 || !Q_stricmp( Cmd_Argv( 0 ), "cvar_restart" ) ) {
			com_consoleLines[i][0] = 0;
			return true;
		}
	}
	return false;
}


/*
===============
Com_StartupVariable

Searches for command line parameters that are set commands.
If match is not nullptr, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets shouls
be after execing the config and default.
===============
*/
void Com_StartupVariable( const char *match ) {
	int i;
	const char    *s;
	cvar_t  *cv;

	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		Cmd_TokenizeString( com_consoleLines[i] );
		if ( strcmp( Cmd_Argv( 0 ), "set" ) ) {
			continue;
		}

		s = Cmd_Argv( 1 );
		if ( !match || !strcmp( s, match ) ) {
			Cvar_Set( s, Cmd_Argv( 2 ) );
			cv = Cvar_Get( s, "", 0 );
			cv->flags |= CVAR_USER_CREATED;
//			com_consoleLines[i] = 0;
		}
	}
}


/*
=================
Com_AddStartupCommands

Adds command line parameters as script statements
Commands are seperated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
bool Com_AddStartupCommands( void ) {
	int i;
	bool added;

	added = false;
	// quote every token, so args with semicolons can work
	for ( i = 0 ; i < com_numConsoleLines ; i++ ) {
		if ( !com_consoleLines[i] || !com_consoleLines[i][0] ) {
			continue;
		}

		// set commands won't override menu startup
		if ( Q_stricmpn( com_consoleLines[i], "set", 3 ) ) {
			added = true;
		}
		Cbuf_AddText( com_consoleLines[i] );
		Cbuf_AddText( "\n" );
	}

	return added;
}


//============================================================================

void Info_Print( const char *s ) {
	char key[512];
	char value[512];

	if ( *s == '\\' ) {
		s++;
	}
	while ( *s )
	{
		char* o = key;
		while ( *s && *s != '\\' )
			*o++ = *s++;

        size_t l = o - key;
		if ( l < 20 ) {
			memset( o, ' ', 20 - l );
			key[20] = 0;
		} else {
			*o = 0;
		}
		Com_Printf( "%s", key );

		if ( !*s ) {
			Com_Printf( "MISSING VALUE\n" );
			return;
		}

		o = value;
		s++;
		while ( *s && *s != '\\' )
			*o++ = *s++;
		*o = 0;

		if ( *s ) {
			s++;
		}
		Com_Printf( "%s\n", value );
	}
}

/*
============
Com_StringContains
============
*/
char *Com_StringContains( char *str1, char *str2, int casesensitive ) {

	size_t len = strlen( str1 ) - strlen( str2 );
	for (int i = 0; i <= len; i++, str1++ ) {
        int j;
		for (j = 0; str2[j]; j++ ) {
			if ( casesensitive ) {
				if ( str1[j] != str2[j] ) {
					break;
				}
			} else {
				if ( toupper( str1[j] ) != toupper( str2[j] ) ) {
					break;
				}
			}
		}
		if ( !str2[j] ) {
			return str1;
		}
	}
	return nullptr;
}

/*
============
Com_Filter
============
*/
int Com_Filter( const char *filter, char *name, int casesensitive ) {
	char buf[MAX_TOKEN_CHARS];
	char *ptr;
	int i, found;

	while ( *filter ) {
		if ( *filter == '*' ) {
			filter++;
			for ( i = 0; *filter; i++ ) {
				if ( *filter == '*' || *filter == '?' ) {
					break;
				}
				buf[i] = *filter;
				filter++;
			}
			buf[i] = '\0';
			if ( strlen( buf ) ) {
				ptr = Com_StringContains( name, buf, casesensitive );
				if ( !ptr ) {
					return false;
				}
				name = ptr + strlen( buf );
			}
		} else if ( *filter == '?' )      {
			filter++;
			name++;
		} else if ( *filter == '[' && *( filter + 1 ) == '[' )           {
			filter++;
		} else if ( *filter == '[' )      {
			filter++;
			found = false;
			while ( *filter && !found ) {
				if ( *filter == ']' && *( filter + 1 ) != ']' ) {
					break;
				}
				if ( *( filter + 1 ) == '-' && *( filter + 2 ) && ( *( filter + 2 ) != ']' || *( filter + 3 ) == ']' ) ) {
					if ( casesensitive ) {
						if ( *name >= *filter && *name <= *( filter + 2 ) ) {
							found = true;
						}
					} else {
						if ( toupper( *name ) >= toupper( *filter ) &&
							 toupper( *name ) <= toupper( *( filter + 2 ) ) ) {
							found = true;
						}
					}
					filter += 3;
				} else {
					if ( casesensitive ) {
						if ( *filter == *name ) {
							found = true;
						}
					} else {
						if ( toupper( *filter ) == toupper( *name ) ) {
							found = true;
						}
					}
					filter++;
				}
			}
			if ( !found ) {
				return false;
			}
			while ( *filter ) {
				if ( *filter == ']' && *( filter + 1 ) != ']' ) {
					break;
				}
				filter++;
			}
			filter++;
			name++;
		} else {
			if ( casesensitive ) {
				if ( *filter != *name ) {
					return false;
				}
			} else {
				if ( toupper( *filter ) != toupper( *name ) ) {
					return false;
				}
			}
			filter++;
			name++;
		}
	}
	return true;
}

/*
============
Com_FilterPath
============
*/
int Com_FilterPath(const char *filter,const char *name, int casesensitive ) {
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for ( i = 0; i < MAX_QPATH - 1 && filter[i]; i++ ) {
		if ( filter[i] == '\\' || filter[i] == ':' ) {
			new_filter[i] = '/';
		} else {
			new_filter[i] = filter[i];
		}
	}
	new_filter[i] = '\0';
	for ( i = 0; i < MAX_QPATH - 1 && name[i]; i++ ) {
		if ( name[i] == '\\' || name[i] == ':' ) {
			new_name[i] = '/';
		} else {
			new_name[i] = name[i];
		}
	}
	new_name[i] = '\0';
	return Com_Filter( new_filter, new_name, casesensitive );
}

/*
============
Com_HashKey
============
*/
int Com_HashKey( const char *string, int maxlen )
{
	int hash = 0;
	for (int i = 0; i < maxlen && string[i] != '\0'; i++ ) {
		hash += string[i] * ( 119 + i );
	}
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	return hash;
}

/*
================
Com_RealTime
================
*/
time_t Com_RealTime( qtime_t *qtime ) {
	time_t t;
	struct tm *tms;

	t = time( nullptr );
	if ( !qtime ) {
		return t;
	}
	tms = localtime( &t );
	if ( tms ) {
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return t;
}

/*
========================
CopyString

 NOTE:	never write over the memory CopyString returns because
		memory from a memstatic_t might be returned
========================
*/
char *CopyString( const char *in ) {
	char    *out;

	out = (char *)calloc(1,  strlen( in ) + 1 );
	strcpy( out, in );
	return out;
}

/*
==============================================================================

Goals:
	reproducable without history effects -- no out of memory errors on weird map to map changes
	allow restarting of the client without fragmentation
	minimize total pages in use at run time
	minimize total pages needed during load time

  Single block of memory with stack allocators coming from both ends towards the middle.

  One side is designated the temporary memory allocator.

  Temporary memory can be allocated and freed in any order.

  A highwater mark is kept of the most in use at any time.

  When there is no temporary memory allocated, the permanent and temp sides
  can be switched, allowing the already touched temp memory to be used for
  permanent storage.

  Temp memory must never be allocated on two ends at once, or fragmentation
  could occur.

  If we have any in-use temp memory, additional temp allocations must come from
  that side.

  If not, we can choose to make either side the new temp side and push future
  permanent allocations to the other side.  Permanent allocations should be
  kept on the side that has the current greatest wasted highwater mark.

==============================================================================
*/


#define HUNK_MAGIC  0x89537892
#define HUNK_FREE_MAGIC 0x89537893

typedef struct {
	int magic;
	int size;
} hunkHeader_t;

typedef struct {
	int mark;
	int permanent;
	int temp;
	int tempHighwater;
} hunkUsed_t;

typedef struct hunkblock_s {
	int size;
	uint8_t printed;
	struct hunkblock_s *next;
	char *label;
	char *file;
	int line;
} hunkblock_t;

static hunkblock_t *hunkblocks;

static hunkUsed_t hunk_low, hunk_high;
static hunkUsed_t  *hunk_permanent, *hunk_temp;

static uint8_t    *s_hunkData = nullptr;
static int s_hunkTotal;

static int s_zoneTotal;


void Com_InitZoneMemory( void ) {

}


/*
=================
Com_InitZoneMemory
=================
*/
void Com_InitHunkMemory( void ) {


}


void CL_ShutdownCGame( void );
void CL_ShutdownUI( void );
void SV_ShutdownGameProgs( void );


void *Hunk_Alloc( int size, ha_pref preference ) {

    void* buf = malloc(size);

	memset( buf, 0, size );

	return buf;
}

/*
=================
Hunk_AllocateTempMemory

This is used by the file loading system.
Multiple files can be loaded in temporary memory.
When the files-in-use count reaches zero, all temp memory will be deleted
=================
*/
void *Hunk_AllocateTempMemory( size_t size ) {
    return malloc(size);
}


/*
==================
Hunk_FreeTempMemory
==================
*/
void Hunk_FreeTempMemory( void *buf ) {
    free(buf);
}



/*
===================================================================

EVENTS AND JOURNALING

In addition to these events, .cfg files are also copied to the
journaled file
===================================================================
*/

// bk001129 - here we go again: upped from 64
#define MAX_PUSHED_EVENTS               256
// bk001129 - init, also static
static int com_pushedEventsHead = 0;
static int com_pushedEventsTail = 0;
// bk001129 - static
static sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

/*
=================
Com_InitJournaling
=================
*/
void Com_InitJournaling( void ) {
	Com_StartupVariable( "journal" );
	com_journal = Cvar_Get( "journal", "0", CVAR_INIT );
	if ( !com_journal->integer ) {
		return;
	}

	if ( com_journal->integer == 1 ) {
		Com_Printf( "Journaling events\n" );

		com_journalFile = FS_FOpenFileWrite( "journal.dat" );
		com_journalDataFile = FS_FOpenFileWrite( "journaldata.dat" );
	} else if ( com_journal->integer == 2 ) {
		Com_Printf( "Replaying journaled events\n" );
		FS_FOpenFileRead( "journal.dat", &com_journalFile, true );
		FS_FOpenFileRead( "journaldata.dat", &com_journalDataFile, true );
	}

	if ( !com_journalFile || !com_journalDataFile ) {
		Cvar_Set( "com_journal", "0" );
		com_journalFile = 0;
		com_journalDataFile = 0;
		Com_Printf( "Couldn't open journal files\n" );
	}
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define MAX_QUEUED_EVENTS  256
#define MASK_QUEUED_EVENTS ( MAX_QUEUED_EVENTS - 1 )

static sysEvent_t  eventQueue[ MAX_QUEUED_EVENTS ];
static int         eventHead = 0;
static int         eventTail = 0;

/*
================
Com_QueueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Com_QueueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr )
{
	sysEvent_t  *ev;

	// combine mouse movement with previous mouse event
	if ( type == SE_MOUSE && eventHead != eventTail )
	{
		ev = &eventQueue[ ( eventHead + MAX_QUEUED_EVENTS - 1 ) & MASK_QUEUED_EVENTS ];

		if ( ev->evType == SE_MOUSE )
		{
			ev->evValue += value;
			ev->evValue2 += value2;
			return;
		}
	}

	ev = &eventQueue[ eventHead & MASK_QUEUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUEUED_EVENTS )
	{
		Com_Printf("Com_QueueEvent: overflow\n");
		// we are discarding an event, but don't leak memory
		if ( ev->evPtr )
		{
			free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 )
	{
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Com_GetSystemEvent

================
*/
sysEvent_t Com_GetSystemEvent( void )
{
	sysEvent_t  ev;
	char        *s;

	// return if we have data
	if ( eventHead > eventTail )
	{
		eventTail++;
		return eventQueue[ ( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
	}


	// return if we have data
	if ( eventHead > eventTail )
	{
		eventTail++;
		return eventQueue[ ( eventTail - 1 ) & MASK_QUEUED_EVENTS ];
	}

	// create an empty event to return
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

/*
=================
Com_GetRealEvent
=================
*/
sysEvent_t  Com_GetRealEvent( void ) {
	size_t r;
	sysEvent_t ev;

	// either get an event from the system or the journal file
	if ( com_journal->integer == 2 ) {
		r = FS_Read( &ev, sizeof( ev ), com_journalFile );
		if ( r != sizeof( ev ) ) {
			Com_Error( ERR_FATAL, "Error reading from journal file" );
		}
		if ( ev.evPtrLength ) {
			ev.evPtr = calloc(1,  ev.evPtrLength );
			r = FS_Read( ev.evPtr, ev.evPtrLength, com_journalFile );
			if ( r != ev.evPtrLength ) {
				Com_Error( ERR_FATAL, "Error reading from journal file" );
			}
		}
	} else {
		ev = Com_GetSystemEvent();

		// write the journal value out if needed
		if ( com_journal->integer == 1 ) {
			r = FS_Write( &ev, sizeof( ev ), com_journalFile );
			if ( r != sizeof( ev ) ) {
				Com_Error( ERR_FATAL, "Error writing to journal file" );
			}
			if ( ev.evPtrLength ) {
				r = FS_Write( ev.evPtr, ev.evPtrLength, com_journalFile );
				if ( r != ev.evPtrLength ) {
					Com_Error( ERR_FATAL, "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}


/*
=================
Com_InitPushEvent
=================
*/
// bk001129 - added
void Com_InitPushEvent( void ) {
	// clear the static buffer array
	// this requires EVT_NONE to be accepted as a valid but NOP event
	memset( com_pushedEvents, 0, sizeof( com_pushedEvents ) );
	// reset counters while we are at it
	// beware: GetEvent might still return an EVT_NONE from the buffer
	com_pushedEventsHead = 0;
	com_pushedEventsTail = 0;
}


/*
=================
Com_PushEvent
=================
*/
void Com_PushEvent( sysEvent_t *event ) {
	sysEvent_t      *ev;
	static int printedWarning = 0; // bk001129 - init, bk001204 - explicit int

	ev = &com_pushedEvents[ com_pushedEventsHead & ( MAX_PUSHED_EVENTS - 1 ) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = true;
			Com_Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = false;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
Com_GetEvent
=================
*/
sysEvent_t  Com_GetEvent( void ) {
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ ( com_pushedEventsTail - 1 ) & ( MAX_PUSHED_EVENTS - 1 ) ];
	}
	return Com_GetRealEvent();
}

/*
=================
Com_RunAndTimeServerPacket
=================
*/
void Com_RunAndTimeServerPacket( netadr_t *evFrom, msg_t *buf ) {
	int t1, t2, msec;

	t1 = 0;

	if ( com_speeds->integer ) {
		t1 = Sys_Milliseconds();
	}

	SV_PacketEvent( *evFrom, buf );

	if ( com_speeds->integer ) {
		t2 = Sys_Milliseconds();
		msec = t2 - t1;
		if ( com_speeds->integer == 3 ) {
			Com_Printf( "SV_PacketEvent time: %i\n", msec );
		}
	}
}

/*
=================
Com_EventLoop

Returns last event time
=================
*/
int Com_EventLoop( void ) {
	sysEvent_t ev;
	netadr_t evFrom;
	uint8_t bufData[MAX_MSGLEN];
	msg_t buf;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	while ( 1 ) {
		ev = Com_GetEvent();

		// if no more events are available
		if ( ev.evType == EVT_NONE ) {
			// manually send packet events for the loopback channel
			while ( NET_GetLoopPacket( NS_CLIENT, &evFrom, &buf ) ) {
				CL_PacketEvent( evFrom, &buf );
			}

			while ( NET_GetLoopPacket( NS_SERVER, &evFrom, &buf ) ) {
				// if the server just shut down, flush the events
				if ( com_sv_running->integer ) {
					Com_RunAndTimeServerPacket( &evFrom, &buf );
				}
			}

			return ev.evTime;
		}


		switch ( ev.evType ) {
		default:
			// bk001129 - was ev.evTime
			Com_Error( ERR_FATAL, "Com_EventLoop: bad event type %i", ev.evType );
			break;
		case EVT_NONE:
			break;
		case SE_KEY:
			CL_KeyEvent( ev.evValue, ev.evValue2 != 0, ev.evTime );
			break;
		case SE_CHAR:
			CL_CharEvent( ev.evValue );
			break;
		case SE_MOUSE:
			CL_MouseEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_JOYSTICK_AXIS:
			CL_JoystickEvent( ev.evValue, ev.evValue2, ev.evTime );
			break;
		case SE_CONSOLE:
			Cbuf_AddText( (char *)ev.evPtr );
			Cbuf_AddText( "\n" );
			break;

		}

		// free any block data
		if ( ev.evPtr ) {
			free( ev.evPtr );
		}
	}

	return 0;   // never reached
}

/*
================
Com_Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int Com_Milliseconds( void ) {
	sysEvent_t ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != EVT_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != EVT_NONE );

	return ev.evTime;
}

//============================================================================

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
static void Com_Error_f( void ) {
	if ( Cmd_Argc() > 1 ) {
		Com_Error( ERR_DROP, "Testing drop error" );
	} else {
		Com_Error( ERR_FATAL, "Testing fatal error" );
	}
}


/*
=============
Com_Freeze_f

Just freeze in place for a given number of seconds to test
error recovery
=============
*/
static void Com_Freeze_f( void ) {
	float s;
	int start, now;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "freeze <seconds>\n" );
		return;
	}
	s = atof( Cmd_Argv( 1 ) );

	start = Com_Milliseconds();

	while ( 1 ) {
		now = Com_Milliseconds();
		if ( ( now - start ) * 0.001 > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
static void Com_Crash_f( void ) {
	__builtin_trap();
}

void Com_SetRecommended( bool vidrestart ) {
	cvar_t *cv;
	bool goodVideo;
	bool goodCPU;
	bool lowMemory;
	// will use this for recommended settings as well.. do i outside the lower check so it gets done even with command line stuff
	cv = Cvar_Get( "r_highQualityVideo", "1", CVAR_ARCHIVE );
	goodVideo = ( cv && cv->integer );
	goodCPU = Sys_GetHighQualityCPU();
	lowMemory = Sys_LowPhysicalMemory();

	if ( goodVideo && goodCPU ) {
		Com_Printf( "Found high quality video and CPU\n" );
		Cbuf_AddText( "exec highVidhighCPU.cfg\n" );
	} else if ( goodVideo && !goodCPU ) {
		Cbuf_AddText( "exec highVidlowCPU.cfg\n" );
		Com_Printf( "Found high quality video and low quality CPU\n" );
	} else if ( !goodVideo && goodCPU ) {
		Cbuf_AddText( "exec lowVidhighCPU.cfg\n" );
		Com_Printf( "Found low quality video and high quality CPU\n" );
	} else {
		Cbuf_AddText( "exec lowVidlowCPU.cfg\n" );
		Com_Printf( "Found low quality video and low quality CPU\n" );
	}

// (SA) set the cvar so the menu will reflect this on first run
	Cvar_Set( "ui_glCustom", "999" );   // 'recommended'


	if ( lowMemory ) {
		Com_Printf( "Found minimum memory requirement\n" );
		Cvar_Set( "s_khz", "11" );
		if ( !goodVideo ) {
			Cvar_Set( "r_lowMemTextureSize", "256" );
			Cvar_Set( "r_lowMemTextureThreshold", "40.0" );
		}
	}
	if ( vidrestart ) {
		Cbuf_AddText( "vid_restart\n" );
	}
}
/*
=================
Com_Init
=================
*/
void Com_Init( char *commandLine ) {
	char    *s;

	Com_Printf( "%s %s\n", Q3_VERSION, __DATE__ );

	if ( setjmp( abortframe ) ) {
		Sys_Error( "Error during initialization" );
	}

	// bk001129 - do this before anything else decides to push events
	Com_InitPushEvent();

	Cvar_Init();

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	Com_ParseCommandLine( commandLine );

	Swap_Init();
	Cbuf_Init();

	Com_InitZoneMemory();
	Cmd_Init();

	// override anything from the config files with command line args
	Com_StartupVariable( nullptr );

	// get the developer cvar set as early as possible
	Com_StartupVariable( "developer" );
	com_developer = Cvar_Get( "developer", "1", CVAR_TEMP );

	// done early so bind command exists
	CL_InitKeyCommands();

	FS_InitFilesystem();

	Com_InitJournaling();

	Cbuf_AddText( "exec default.cfg\n" );

	Cbuf_AddText( "exec language.cfg\n" ); //----(SA)	added

	// skip the q3config.cfg if "safe" is on the command line
	if ( !Com_SafeMode() ) {
		Cbuf_AddText( "exec wolfconfig.cfg\n" );
	}

	Cbuf_AddText( "exec autoexec.cfg\n" );

	Cbuf_Execute();

	// override anything from the config files with command line args
	Com_StartupVariable( nullptr );

	// allocate the stack based hunk allocator
	Com_InitHunkMemory();

	// if any archived cvars are modified after this, we will trigger a writing
	// of the config file
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	//
	// init commands and vars
	//
	com_maxfps = Cvar_Get( "com_maxfps", "85", CVAR_ARCHIVE );
	com_blood = Cvar_Get( "com_blood", "1", CVAR_ARCHIVE );

	
	com_logfile = Cvar_Get( "logfile", "0", CVAR_TEMP );

	com_timescale = Cvar_Get( "timescale", "1", CVAR_CHEAT | CVAR_SYSTEMINFO );
	com_fixedtime = Cvar_Get( "fixedtime", "0", CVAR_CHEAT );
	com_showtrace = Cvar_Get( "com_showtrace", "0", CVAR_CHEAT );
	com_dropsim = Cvar_Get( "com_dropsim", "0", CVAR_CHEAT );
	com_viewlog = Cvar_Get( "viewlog", "0", CVAR_CHEAT );
	com_speeds = Cvar_Get( "com_speeds", "0", 0 );

	com_cameraMode = Cvar_Get( "com_cameraMode", "0", CVAR_CHEAT );

	cl_paused = Cvar_Get( "cl_paused", "0", CVAR_ROM );
	sv_paused = Cvar_Get( "sv_paused", "0", CVAR_ROM );
	com_sv_running = Cvar_Get( "sv_running", "0", CVAR_ROM );
	com_cl_running = Cvar_Get( "cl_running", "0", CVAR_ROM );
	com_buildScript = Cvar_Get( "com_buildScript", "0", 0 );

	com_introPlayed = Cvar_Get( "com_introplayed", "0", CVAR_ARCHIVE );
	com_recommendedSet = Cvar_Get( "com_recommendedSet", "0", CVAR_ARCHIVE );

	Cvar_Get( "savegame_loading", "0", CVAR_ROM );

	com_hunkused = Cvar_Get( "com_hunkused", "0", 0 );

	if ( com_developer && com_developer->integer ) {
		Cmd_AddCommand( "error", Com_Error_f );
		Cmd_AddCommand( "crash", Com_Crash_f );
		Cmd_AddCommand( "freeze", Com_Freeze_f );
	}
	Cmd_AddCommand( "quit", Com_Quit_f );
	Cmd_AddCommand( "changeVectors", MSG_ReportChangeVectors_f );
	Cmd_AddCommand( "writeconfig", Com_WriteConfig_f );

	s = va( "%s %s", Q3_VERSION, __DATE__ );
	com_version = Cvar_Get( "version", s, CVAR_ROM | CVAR_SERVERINFO );

	Sys_Init();
	Netchan_Init( Com_Milliseconds() & 0xffff );    // pick a port value that should be nice and random

	SV_Init();


	CL_Init();
	Sys_ShowConsole( com_viewlog->integer, false );

	// set com_frameTime so that if a map is started on the
	// command line it will still be able to count on com_frameTime
	// being random enough for a serverid
	com_frameTime = Com_Milliseconds();

	// add + commands from command line
	if ( !Com_AddStartupCommands() ) {
		// if the user didn't give any commands, run default action
	}

	// start in full screen ui mode
	Cvar_Set( "r_uiFullScreen", "1" );

	CL_StartHunkUsers();

	Sys_ShowConsole( com_viewlog->integer, false );

	if ( !com_recommendedSet->integer ) {
		Com_SetRecommended( true );
		Cvar_Set( "com_recommendedSet", "1" );
	}


    if ( !com_introPlayed->integer ) {
    #ifdef __MACOS__
        extern void PlayIntroMovies( void );
        PlayIntroMovies();
    #endif
        Cbuf_AddText( "cinematic wolfintro.RoQ 3\n" );
    }

	com_fullyInitialized = true;
	Com_Printf( "--- Common Initialization Complete ---\n" );
}

//==================================================================

void Com_WriteConfigToFile( const char *filename ) {

    fileHandle_t f = FS_FOpenFileWrite( filename );
	if ( !f ) {
		Com_Printf( "Couldn't write %s.\n", filename );
		return;
	}

	FS_Printf( f, "// generated by RTCW, do not modify\n" );
	Key_WriteBindings( f );
	Cvar_WriteVariables( f );
	FS_FCloseFile( f );
}


/*
===============
Com_WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void Com_WriteConfiguration( void ) {

	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !( cvar_modifiedFlags & CVAR_ARCHIVE ) ) {
		return;
	}
	cvar_modifiedFlags &= ~CVAR_ARCHIVE;

	Com_WriteConfigToFile( "wolfconfig.cfg" );
}


/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
void Com_WriteConfig_f( void ) {
	char filename[MAX_QPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	Com_Printf( "Writing %s.\n", filename );
	Com_WriteConfigToFile( filename );
}

/*
================
Com_ModifyMsec
================
*/
int Com_ModifyMsec( int msec ) {
	int clampTime;

	//
	// modify time for debugging values
	//
	if ( com_fixedtime->integer ) {
		msec = com_fixedtime->integer;
	} else if ( com_timescale->value ) {
		msec *= com_timescale->value;
	}

	// don't let it scale below 1 msec
	if ( msec < 1 && com_timescale->value ) {
		msec = 1;
	}

	// for local single player gaming
	// we may want to clamp the time to prevent players from
	// flying off edges when something hitches.
	clampTime = 200;

	if ( msec > clampTime ) {
		msec = clampTime;
	}

	return msec;
}


void Com_Frame()
{
	int minMsec;
	static int lastTime = 0;
	int key;

	if ( setjmp( abortframe ) ) {
		return;         // an ERR_DROP was thrown
	}

	int timeBeforeFirstEvents = 0;
	int timeBeforeServer = 0;
	int timeBeforeEvents = 0;
	int timeBeforeClient = 0;
	int timeAfter = 0;

	// write config file if anything changed
	Com_WriteConfiguration();

	//
	// main event loop
	//
	if ( com_speeds->integer ) {
		timeBeforeFirstEvents = Sys_Milliseconds();
	}

	// we may want to spin here if things are going too fast
	if ( com_maxfps->integer > 0) {
		minMsec = 1000 / com_maxfps->integer;
	} else {
		minMsec = 1;
	}

	IN_Frame();

	lastTime = com_frameTime;
	com_frameTime = Com_EventLoop();
	int msec = com_frameTime - lastTime;

	Cbuf_Execute();

	lastTime = com_frameTime;

	// mess with msec if needed
	com_frameMsec = msec;
	msec = Com_ModifyMsec( msec );

	//
	// server side
	//
	if ( com_speeds->integer ) {
		timeBeforeServer = Sys_Milliseconds();
	}

	SV_Frame( msec );

	//
	// client system
	//

	//
	// run event loop a second time to get server to client packets
	// without a frame of latency
	//
	if ( com_speeds->integer ) {
		timeBeforeEvents = Sys_Milliseconds();
	}
	Com_EventLoop();
	Cbuf_Execute();


	//
	// client side
	//
	if ( com_speeds->integer ) {
		timeBeforeClient = Sys_Milliseconds();
	}

	CL_Frame( msec );

	if ( com_speeds->integer ) {
		timeAfter = Sys_Milliseconds();
	}
	

	//
	// report timing information
	//
	if ( com_speeds->integer ) {
		int all = timeAfter - timeBeforeServer;
		int sv = timeBeforeEvents - timeBeforeServer;
		int ev = timeBeforeServer - timeBeforeFirstEvents + timeBeforeClient - timeBeforeEvents;
		int cl = timeAfter - timeBeforeClient;
		sv -= time_game;
		cl -= time_frontend + time_backend;

		Com_Printf( "frame:%i all:%3i sv:%3i ev:%3i cl:%3i gm:%3i rf:%3i bk:%3i\n",
					com_frameNumber, all, sv, ev, cl, time_game, time_frontend, time_backend );
	}

	com_frameNumber++;
}

/*
=================
Com_Shutdown
=================
*/
void Com_Shutdown( void ) {
	if ( logfile ) {
		FS_FCloseFile( logfile );
		logfile = 0;
	}

	if ( com_journalFile ) {
		FS_FCloseFile( com_journalFile );
		com_journalFile = 0;
	}

}

void Com_Memcpy( void* dest, const void* src, const size_t count ) {
	memcpy( dest, src, count );
}

void Com_Memset( void* dest, const int val, const size_t count ) {
	memset( dest, val, count );
}

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

	This should go in q_math but it is too late to add new traps
	to game and ui
=====================
*/
float Q_acos( float c ) {
	float angle;

	angle = acos( c );

	if ( angle > M_PI ) {
		return (float)M_PI;
	}
	if ( angle < -M_PI ) {
		return (float)M_PI;
	}
	return angle;
}

/*
===========================================
command line completion
===========================================
*/

/*
==================
Field_Clear
==================
*/
void Field_Clear( field_t *edit ) {
	memset( edit->buffer, 0, MAX_EDIT_LINE );
	edit->cursor = 0;
	edit->scroll = 0;
}

static const char *completionString;
static char shortestMatch[MAX_TOKEN_CHARS];
static int matchCount;
// field we are working on, passed to Field_CompleteCommand (&g_consoleCommand for instance)
static field_t *completionField;

/*
===============
FindMatches

===============
*/
static void FindMatches( const char *s ) {
	int i;

	if ( Q_stricmpn( s, completionString, strlen( completionString ) ) ) {
		return;
	}
	matchCount++;
	if ( matchCount == 1 ) {
		Q_strncpyz( shortestMatch, s, sizeof( shortestMatch ) );
		return;
	}

	// cut shortestMatch to the amount common with s
	for ( i = 0 ; s[i] ; i++ ) {
		if ( tolower( shortestMatch[i] ) != tolower( s[i] ) ) {
			shortestMatch[i] = 0;
		}
	}
}

/*
===============
PrintMatches

===============
*/
static void PrintMatches( const char *s ) {
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( "    %s\n", s );
	}
}

static void keyConcatArgs( void ) {

	for (int i = 1 ; i < Cmd_Argc() ; i++ ) {
		Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
		const char* arg = Cmd_Argv( i );
		while ( *arg ) {
			if ( *arg == ' ' ) {
				Q_strcat( completionField->buffer, sizeof( completionField->buffer ),  "\"" );
				break;
			}
			arg++;
		}
		Q_strcat( completionField->buffer, sizeof( completionField->buffer ),  Cmd_Argv( i ) );
		if ( *arg == ' ' ) {
			Q_strcat( completionField->buffer, sizeof( completionField->buffer ),  "\"" );
		}
	}
}

static void ConcatRemaining( const char *src, const char *start ) {
	

	const char *str = strstr( src, start );
	if ( !str ) {
		keyConcatArgs();
		return;
	}

	str += strlen( start );
	Q_strcat( completionField->buffer, sizeof( completionField->buffer ), str );
}

/*
===============
Field_CompleteCommand

perform Tab expansion
NOTE TTimo this was originally client code only
  moved to common code when writing tty console for *nix dedicated server
===============
*/
void Field_CompleteCommand( field_t *field ) {
	field_t temp;
	completionField = field;

	// only look at the first token for completion purposes
	Cmd_TokenizeString( completionField->buffer );

	completionString = Cmd_Argv( 0 );
	if ( completionString[0] == '\\' || completionString[0] == '/' ) {
		completionString++;
	}
	matchCount = 0;
	shortestMatch[0] = 0;

	if ( strlen( completionString ) == 0 ) {
		return;
	}

	Cmd_CommandCompletion( FindMatches );
	Cvar_CommandCompletion( FindMatches );

	if ( matchCount == 0 ) {
		return; // no matches
	}

	Com_Memcpy( &temp, completionField, sizeof( field_t ) );

	if ( matchCount == 1 ) {
		snprintf( completionField->buffer, sizeof( completionField->buffer ), "\\%s", shortestMatch );
		if ( Cmd_Argc() == 1 ) {
			Q_strcat( completionField->buffer, sizeof( completionField->buffer ), " " );
		} else {
			ConcatRemaining( temp.buffer, completionString );
		}
		completionField->cursor = strlen( completionField->buffer );
		return;
	}

	// multiple matches, complete to shortest
	snprintf( completionField->buffer, sizeof( completionField->buffer ), "\\%s", shortestMatch );
	completionField->cursor = strlen( completionField->buffer );
	ConcatRemaining( temp.buffer, completionString );

	Com_Printf( "]%s\n", completionField->buffer );

	// run through again, printing matches
	Cmd_CommandCompletion( PrintMatches );
	Cvar_CommandCompletion( PrintMatches );
}

