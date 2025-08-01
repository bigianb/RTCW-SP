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

#import "../client/client.h"
#import "macosx_local.h"

#import "dlfcn.h"
#import "Q3Controller.h"

#import <AppKit/AppKit.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOBSD.h>
#import <IOKit/storage/IOCDMedia.h>
#import <mach/mach_error.h>

#import <sys/types.h>
#import <unistd.h>
#import <sys/param.h>
#import <sys/mount.h>
#import <sys/sysctl.h>

#ifdef OMNI_TIMER
#import "macosx_timers.h"
#endif

qboolean stdin_active = qfalse;

//===========================================================================

int main( int argc, const char *argv[] ) {
#ifdef DEDICATED
	Q3Controller *controller;

	stdin_active = qtrue;
	controller = [[Q3Controller alloc] init];
	[controller quakeMain];
	return 0;
#else
	return NSApplicationMain( argc, argv );
#endif
}

//===========================================================================

/*
=================
Sys_UnloadDll

=================
*/
void Sys_UnloadDll( void *dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	dlclose( dllHandle );
}

/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
=================
*/
extern char     *FS_BuildOSPath( const char *base, const char *game, const char *qpath );

void *Sys_LoadDll( const char *name, int( **entryPoint ) ( int, ...),
				   int ( *systemcalls )( int, ...) ) {
	void *libHandle;
	void ( *dllEntry )( int ( *syscallptr )( int, ...) );
	NSString *bundlePath, *libraryPath;
	const char *path;

	bundlePath = [[NSBundle mainBundle] pathForResource: [NSString stringWithCString: name] ofType: @"bundle"];
//    libraryPath = [NSString stringWithFormat: @"%@/Contents/MacOS/%s", bundlePath, name];
	libraryPath = [NSString stringWithFormat: @"%s.bundle/Contents/MacOS/%s", name, name];
	if ( !libraryPath ) {
		return NULL;
	}

	path = [libraryPath cString];
	Com_Printf( "Loading '%s'.\n", path );
	libHandle = dlopen( [libraryPath cString], RTLD_LAZY );
	if ( !libHandle ) {
		libHandle = dlopen( name, RTLD_LAZY );
		if ( !libHandle ) {
			Com_Printf( "Error loading dll: %s\n", dlerror() );
			return NULL;
		}
	}

	dllEntry = dlsym( libHandle, "_dllEntry" );
	if ( !dllEntry ) {
		Com_Printf( "Error loading dll:  No dllEntry symbol.\n" );
		dlclose( libHandle );
		return NULL;
	}

	*entryPoint = dlsym( libHandle, "_vmMain" );
	if ( !*entryPoint ) {
		Com_Printf( "Error loading dll:  No vmMain symbol.\n" );
		dlclose( libHandle );
		return NULL;
	}

	dllEntry( systemcalls );
	return libHandle;
}

//===========================================================================

char *Sys_GetClipboardData( void ) { // FIXME
	NSPasteboard *pasteboard;
	NSArray *pasteboardTypes;

	pasteboard = [NSPasteboard generalPasteboard];
	pasteboardTypes = [pasteboard types];
	if ([pasteboardTypes containsObject : NSStringPboardType] ) {
		NSString *clipboardString;

		clipboardString = [pasteboard stringForType:NSStringPboardType];
		if ( clipboardString && [clipboardString length] > 0 ) {
			return strdup([clipboardString cString] );
		}
	}
	return NULL;
}

char *Sys_GetWholeClipboard( void ) {
	return NULL;
}

void Sys_SetClipboard( const char *contents ) {
}


//===========================================================================

void Sys_BeginProfiling( void ) {
}

void Sys_EndProfiling( void ) {
}

//===========================================================================

/*
================
Sys_Init

The cvar and file system has been setup, so configurations are loaded
================
*/
void Sys_Init( void ) {
#ifdef OMNI_TIMER
	InitializeTimers();
	OTStackPushRoot( rootNode );
#endif

	NET_Init();
	Sys_InitInput();
}

/*
=================
Sys_Shutdown
=================
*/
void Sys_Shutdown( void ) {
	Com_Printf( "----- Sys_Shutdown -----\n" );
	Sys_EndProfiling();
	Sys_ShutdownInput();
	Com_Printf( "------------------------\n" );
}

void Sys_Error( const char *error, ...) {
	va_list argptr;
	NSString *formattedString;

	Sys_Shutdown();

	va_start( argptr,error );
	formattedString = [[NSString alloc] initWithFormat:[NSString stringWithCString:error] arguments:argptr];
	va_end( argptr );

	NSLog( @"Sys_Error: %@", formattedString );
	NSRunAlertPanel( @"Wolfenstein Error", formattedString, nil, nil, nil );

	Sys_Quit();
}

void Sys_Quit( void ) {
	Sys_Shutdown();
	[NSApp terminate : nil];
}

/*
================
Sys_Print

This is called for all console output, even if the game is running
full screen and the dedicated console window is hidden.
================
*/

char *ansiColors[8] =
{ "\033[30m",       /* ANSI Black */
  "\033[31m",       /* ANSI Red */
  "\033[32m",       /* ANSI Green */
  "\033[33m",     /* ANSI Yellow */
  "\033[34m",       /* ANSI Blue */
  "\033[36m",     /* ANSI Cyan */
  "\033[35m",       /* ANSI Magenta */
  "\033[37m" };   /* ANSI White */

void Sys_Print( const char *text ) {
#if 0
	/* Okay, this is a stupid hack, but what the hell, I was bored. ;) */
	const char *scan = text;
	int index;

	/* Make sure terminal mode is reset at the start of the line... */
	fputs( "\033[0m", stdout );

	while ( *scan ) {
		/* See if we have a color control code.  If so, snarf the character,
		print what we have so far, print the ANSI Terminal color code,
		skip over the color control code and continue */
		if ( Q_IsColorString( scan ) ) {
			index = ColorIndex( scan[1] );

			/* Flush current message */
			if ( scan != text ) {
				fwrite( text, scan - text, 1, stdout );
			}

			/* Write ANSI color code */
			fputs( ansiColors[index], stdout );

			/* Reset search */
			text = scan + 2;
			scan = text;
			continue;
		}
		scan++;
	}

	/* Flush whatever's left */
	fputs( text, stdout );

	/* Make sure terminal mode is reset at the end of the line too... */
	fputs( "\033[0m", stdout );

#else
	fputs( text, stdout );
#endif
}



/*
================
Sys_CheckCD

Return true if the proper CD is in the drive
================
*/

qboolean Sys_ObjectIsCDRomDevice( io_object_t object ) {
	CFStringRef value;
	kern_return_t krc;
	CFDictionaryRef properties;
	qboolean isCDROM = qfalse;
	io_iterator_t parentIterator;
	io_object_t parent;

	krc = IORegistryEntryCreateCFProperties( object, &properties, kCFAllocatorDefault, (IOOptionBits)0 );
	if ( krc != KERN_SUCCESS ) {
		fprintf( stderr, "IORegistryEntryCreateCFProperties returned 0x%08x -- %s\n", krc, mach_error_string( krc ) );
		return qfalse;
	}

	//NSLog(@"properties = %@", properties);

	// See if this is a CD-ROM
	value = CFDictionaryGetValue( properties, CFSTR( kIOCDMediaTypeKey ) );
	if ( value && CFStringCompare( value, CFSTR( "CD-ROM" ), 0 ) == kCFCompareEqualTo ) {
		isCDROM = qtrue;
	}
	CFRelease( properties );

	// If it isn't check each of its parents.  It seems that the parent enumerator only returns the immediate parent.  Maybe the plural indicates that an object can have multiple direct parents.  So, we'll call ourselves recursively for each parent.
	if ( !isCDROM ) {
		krc = IORegistryEntryGetParentIterator( object, kIOServicePlane, &parentIterator );
		if ( krc != KERN_SUCCESS ) {
			fprintf( stderr, "IOServiceGetMatchingServices returned 0x%08x -- %s\n",
					 krc, mach_error_string( krc ) );
		} else {
			while ( !isCDROM && ( parent = IOIteratorNext( parentIterator ) ) ) {
				if ( Sys_ObjectIsCDRomDevice( parent ) ) {
					isCDROM = qtrue;
				}
				IOObjectRelease( parent );
			}

			IOObjectRelease( parentIterator );
		}
	}

	//NSLog(@"Sys_ObjectIsCDRomDevice -> %d", isCDROM);
	return isCDROM;
}

qboolean Sys_IsCDROMDevice( const char *deviceName ) {
	kern_return_t krc;
	io_iterator_t deviceIterator;
	mach_port_t masterPort;
	io_object_t object;
	qboolean isCDROM = qfalse;

	krc = IOMasterPort( bootstrap_port, &masterPort );
	if ( krc != KERN_SUCCESS ) {
		fprintf( stderr, "IOMasterPort returned 0x%08x -- %s\n", krc, mach_error_string( krc ) );
		return qfalse;
	}

	// Get an iterator for this BSD device.  If it is a CD, it will likely only be one partition of the larger CD-ROM device.
	krc = IOServiceGetMatchingServices( masterPort,
										IOBSDNameMatching( masterPort, 0, deviceName ),
										&deviceIterator );
	if ( krc != KERN_SUCCESS ) {
		fprintf( stderr, "IOServiceGetMatchingServices returned 0x%08x -- %s\n",
				 krc, mach_error_string( krc ) );
		return qfalse;
	}

	while ( !isCDROM && ( object = IOIteratorNext( deviceIterator ) ) ) {
		if ( Sys_ObjectIsCDRomDevice( object ) ) {
			isCDROM = qtrue;
		}
		IOObjectRelease( object );
	}

	IOObjectRelease( deviceIterator );

	//NSLog(@"Sys_IsCDROMDevice -> %d", isCDROM);
	return isCDROM;
}

qboolean Sys_CheckCD( void ) {
	return qtrue;
}


//===================================================================

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead ) {
}

void Sys_EndStreamedFile( fileHandle_t f ) {
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f ) {
	return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin ) {
	FS_Seek( f, offset, origin );
}


void OutputDebugString( char * s ) {
#ifdef DEBUG
	fprintf( stderr, "%s", s );
#endif
}

/*
==================
Sys_LowPhysicalMemory()
==================
*/
#define MEM_THRESHOLD 96 * 1024 * 1024

qboolean Sys_LowPhysicalMemory() {
	return NSRealMemoryAvailable() <= MEM_THRESHOLD;
}

static unsigned int _Sys_ProcessorCount = 0;

unsigned int Sys_ProcessorCount() {
	if ( !_Sys_ProcessorCount ) {
		int name[] = {CTL_HW, HW_NCPU};
		size_t size;

		size = sizeof( _Sys_ProcessorCount );
		if ( sysctl( name, 2, &_Sys_ProcessorCount, &size, NULL, 0 ) < 0 ) {
			perror( "sysctl" );
			_Sys_ProcessorCount = 1;
		} else {
			Com_Printf( "System processor count is %d\n", _Sys_ProcessorCount );
		}
	}

	return _Sys_ProcessorCount;
}

