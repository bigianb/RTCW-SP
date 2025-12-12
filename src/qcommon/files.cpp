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

#include "../game/q_shared.h"
#include "qcommon.h"
#include "unzip.h"

/*
=============================================================================

QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created reletive to the base path, so base path should usually be writable.

The "home path" is the path used for all write access. On win32 systems we have "base path"
== "home path", but on *nix systems the base installation is usually readonly, and
"home path" points to ~/.q3a or similar

The user can also install custom mods and content in "home path", so it should be searched
along with "home path" and "cd path" for game content.


The "base game" is the directory under the paths where data comes from by default.

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to FS_AddGameDirectory

Additionaly, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as baseq3 + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("baseq3")

e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.

TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.

When building a pak file, make sure a wolfconfig.cfg isn't present in it,
or configs will never get loaded from disk!

=============================================================================

*/

#define MAX_ZPATH           256
#define MAX_SEARCH_PATHS    4096
#define MAX_FILEHASH_SIZE   1024

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

typedef struct fileInPack_s {
	char                    *name;      // name of the file
	unsigned long pos;                  // file info position in zip
	struct  fileInPack_s*   next;       // next file in the hash
} fileInPack_t;

typedef struct {
	char pakFilename[MAX_OSPATH];               // c:\quake3\baseq3\pak0.pk3
	char pakBasename[MAX_OSPATH];               // pak0
	char pakGamename[MAX_OSPATH];               // baseq3
	unzFile handle;                             // handle to zip file
	int checksum;                               // regular checksum
	int numfiles;                               // number of files in pk3
	int referenced;                             // referenced file flags
	int hashSize;                               // hash table size (power of 2)
	fileInPack_t*   *hashTable;                 // hash table
	fileInPack_t*   buildBuffer;                // buffer with the filenames etc.
} pack_t;

typedef struct {
	char path[MAX_OSPATH];              // c:\quake3
	char gamedir[MAX_OSPATH];           // baseq3
} directory_t;

typedef struct searchpath_s {
	struct searchpath_s *next;

	pack_t      *pack;      // only one of pack / dir will be non nullptr
	directory_t *dir;
} searchpath_t;

static char fs_gamedir[MAX_OSPATH];         // this will be a single file name with no separators
static cvar_t      *fs_debug;
static cvar_t      *fs_homepath;
static cvar_t      *fs_basepath;
static cvar_t      *fs_basegame;

static cvar_t      *fs_gamedirvar;

static searchpath_t    *fs_searchpaths;
static int fs_readCount;                    // total bytes read
static int fs_loadCount;                    // total files read
static int fs_loadStack;                    // total files in memory
static int fs_packFiles;                    // total number of files in packs

static int fs_fakeChkSum;
static int fs_checksumFeed;

typedef union qfile_gus {
	FILE*       o;
	unzFile z;
} qfile_gut;

typedef struct qfile_us {
	qfile_gut file;
	bool unique;
} qfile_ut;

typedef struct {
	qfile_ut handleFiles;
	bool handleSync;
	size_t baseOffset;
	int fileSize;
	size_t zipFilePos;
	bool zipFile;
	bool streamed;
	char name[MAX_ZPATH];
} fileHandleData_t;

static fileHandleData_t fsh[MAX_FILE_HANDLES];

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static int fs_numServerReferencedPaks;
static int fs_serverReferencedPaks[MAX_SEARCH_PATHS];               // checksums
static char     *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];     // pk3 names

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];


/*
==============
FS_Initialized
==============
*/

bool FS_Initialized() {
	return ( fs_searchpaths != nullptr );
}

/*
=================
FS_LoadStack
return load stack
=================
*/
int FS_LoadStack() {
	return fs_loadStack;
}

/*
================
return a hash value for the filename
================
*/
static long FS_HashFileName( const char *fname, int hashSize ) {

	long hash = 0;
	int i = 0;
	while ( fname[i] != '\0' ) {
		char letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;                          // don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                   // damn path names
		}

		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	hash &= ( hashSize - 1 );
	return hash;
}

static fileHandle_t FS_HandleForFile( void ) {

	for (int i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o == nullptr ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "FS_HandleForFile: none free" );
	return 0;
}

static FILE *FS_FileForHandle( fileHandle_t f ) {
	if ( f < 0 || f >= MAX_FILE_HANDLES ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: out of reange" );
        return nullptr; // keep the linter happy, ERR_DROP does not return
	}
	if ( fsh[f].zipFile) {
		Com_Error( ERR_DROP, "FS_FileForHandle: can't get FILE on zip file" );
        return nullptr; // keep the linter happy, ERR_DROP does not return
	}
	if ( !fsh[f].handleFiles.file.o ) {
		Com_Error( ERR_DROP, "FS_FileForHandle: nullptr" );
        return nullptr; // keep the linter happy, ERR_DROP does not return
	}

	return fsh[f].handleFiles.file.o;
}

void    FS_ForceFlush( fileHandle_t f ) {
	FILE* file = FS_FileForHandle( f );
	setvbuf( file, nullptr, _IONBF, 0 );
}

/*
================
FS_filelength

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
size_t FS_filelength( fileHandle_t f ) {
	
	FILE* h = FS_FileForHandle( f );
	size_t pos = ftell( h );
	fseek( h, 0, SEEK_END );
	size_t end = ftell( h );
	fseek( h, pos, SEEK_SET );

	return end;
}

/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
static void FS_ReplaceSeparators( char *path ) {

	for ( char* s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = PATH_SEP;
		}
	}
}

/*
===================
FS_BuildOSPath

Qpath may have either forward or backwards slashes
===================
*/
char *FS_BuildOSPath( const char *base, const char *game, const char *qpath ) {
	char temp[MAX_OSPATH];
	static char ospath[2][MAX_OSPATH];
	static int toggle;

	toggle ^= 1;        // flip-flop to allow two returns without clash

	if ( !game || !game[0] ) {
		game = fs_gamedir;
	}

	snprintf( temp, sizeof( temp ), "/%s/%s", game, qpath );
	FS_ReplaceSeparators( temp );
	snprintf( ospath[toggle], sizeof( ospath[0] ), "%s%s", base, temp );

	return ospath[toggle];
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
static bool FS_CreatePath( const char *OSPath ) {
	
	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return true;
	}

	char* s = strdup(OSPath);
	for (char* ofs = s + 1 ; *ofs ; ofs++ ) {
		if ( *ofs == PATH_SEP ) {
			// create the directory
			*ofs = 0;
			Sys_Mkdir( s );
			*ofs = PATH_SEP;
		}
	}
	free(s);
	return false;
}

/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
static void FS_CopyFile( char *fromOSPath, char *toOSPath ) {
	
	if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) ) {
		Com_Printf( "Ignoring journal files\n" );
		return;
	}

	FILE* f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	size_t len = ftell( f );
	fseek( f, 0, SEEK_SET );

	// we are using direct malloc instead of calloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	uint8_t* buf = (uint8_t *)malloc( len );
	if ( fread( buf, 1, len, f ) != len ) {
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if ( FS_CreatePath( toOSPath ) ) {
        free( buf );
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
        free( buf );
		return;
	}
	if ( fwrite( buf, 1, len, f ) != len ) {
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );
}

void FS_CopyFileOS( char *from, char *to ) {
		
	const char* fromOSPath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, from );
	const char* toOSPath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, to );

	if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) ) {
		Com_Printf( "Ignoring journal files\n" );
		return;
	}

	FILE* f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	size_t len = ftell( f );
	fseek( f, 0, SEEK_SET );

	// we are using direct malloc instead of calloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	uint8_t* buf = (uint8_t *)malloc( len );
	if ( fread( buf, 1, len, f ) != len ) {
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if ( FS_CreatePath( toOSPath ) ) {
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		return;
	}
	if ( fwrite( buf, 1, len, f ) != len ) {
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );
}
/*
===========
FS_Remove

===========
*/
static void FS_Remove( const char *osPath ) {
	remove( osPath );
}

/*
================
FS_FileExists

Tests if the file exists in the current gamedir, this DOES NOT
search the paths.  This is to determine if opening a file to write
(which always goes into the current gamedir) will cause any overwrites.
NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
================
*/
bool FS_FileExists( const char *file ) {
	
	const char* testpath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, file );

	FILE* f = fopen( testpath, "rb" );
	if ( f ) {
		fclose( f );
		return true;
	}
	return false;
}

/*
================
FS_SV_FileExists

Tests if the file exists
================
*/
bool FS_SV_FileExists( const char *file ) {

	char* testpath = FS_BuildOSPath( fs_homepath->string, file, "" );
	testpath[strlen( testpath ) - 1] = '\0';

	FILE* f = fopen( testpath, "rb" );
	if ( f ) {
		fclose( f );
		return true;
	}
	return false;
}


/*
===========
FS_SV_FOpenFileWrite

===========
*/
fileHandle_t FS_SV_FOpenFileWrite( const char *filename ) {

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	char* ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	ospath[strlen( ospath ) - 1] = '\0';

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileWrite: %s\n", ospath );
	}

	if ( FS_CreatePath( ospath ) ) {
		return 0;
	}

	Com_DPrintf( "writing to: %s\n", ospath );
	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = false;
	if ( !fsh[f].handleFiles.file.o ) {
		f = 0;
	}
	return f;
}

/*
===========
FS_SV_FOpenFileRead
search for a file somewhere below the home path, base path or cd path
we search in that order, matching FS_SV_FOpenFileRead order
 Returns the file length.
===========
*/
size_t FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	// search homepath
	char* ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	// remove trailing slash
	ospath[strlen( ospath ) - 1] = '\0';

	if ( fs_debug->integer ) {
		Com_Printf( "FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath );
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
	fsh[f].handleSync = false;
	if ( !fsh[f].handleFiles.file.o ) {
		// NOTE TTimo on non *nix systems, fs_homepath == fs_basepath, might want to avoid
		if ( Q_stricmp( fs_homepath->string,fs_basepath->string ) ) {
			// search basepath
			ospath = FS_BuildOSPath( fs_basepath->string, filename, "" );
			ospath[strlen( ospath ) - 1] = '\0';

			if ( fs_debug->integer ) {
				Com_Printf( "FS_SV_FOpenFileRead (fs_basepath): %s\n", ospath );
			}

			fsh[f].handleFiles.file.o = fopen( ospath, "rb" );
			fsh[f].handleSync = false;

			if ( !fsh[f].handleFiles.file.o ) {
				f = 0;
			}
		}
	}

	*fp = f;
	if ( f ) {
		return FS_filelength( f );
	}
	return 0;
}

/*
===========
FS_Rename

===========
*/
void FS_Rename( const char *from, const char *to ) {
	
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	char* from_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, from );
	char* to_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, to );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_Rename: %s --> %s\n", from_ospath, to_ospath );
	}

	if ( rename( from_ospath, to_ospath ) ) {
		// Failed first attempt, try deleting destination, and renaming again
		FS_Remove( to_ospath );
		if ( rename( from_ospath, to_ospath ) ) {
			// Failed, try copying it and deleting the original
			FS_CopyFile( from_ospath, to_ospath );
			FS_Remove( from_ospath );
		}
	}
}

/*
==============
FS_FCloseFile

If the FILE pointer is an open pak file, leave it open.

For some reason, other dll's can't just cal fclose()
on files returned by FS_FOpenFile...
==============
*/
void FS_FCloseFile( fileHandle_t f ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( fsh[f].streamed ) {
		Sys_EndStreamedFile( f );
	}
	if ( fsh[f].zipFile) {
		unzCloseCurrentFile( fsh[f].handleFiles.file.z );
		if ( fsh[f].handleFiles.unique ) {
			unzClose( fsh[f].handleFiles.file.z );
		}
		memset( &fsh[f], 0, sizeof( fsh[f] ) );
		return;
	}

	// we didn't find it as a pak, so close it as a unique file
	if ( fsh[f].handleFiles.file.o ) {
		fclose( fsh[f].handleFiles.file.o );
	}
	memset( &fsh[f], 0, sizeof( fsh[f] ) );
}

/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileWrite( const char *filename ) {

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;

	char* ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if ( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = false;
	if ( !fsh[f].handleFiles.file.o ) {
		f = 0;
	}
	return f;
}

/*
===========
FS_FOpenFileAppend

===========
*/
fileHandle_t FS_FOpenFileAppend( const char *filename ) {
	
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	fileHandle_t f = FS_HandleForFile();
	fsh[f].zipFile = false;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	char *ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileAppend: %s\n", ospath );
	}

	if ( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "ab" );
	fsh[f].handleSync = false;
	if ( !fsh[f].handleFiles.file.o ) {
		f = 0;
	}
	return f;
}

/*
===========
FS_FilenameCompare

Ignore case and seprator char distinctions
===========
*/
int FS_FilenameCompare( const char *s1, const char *s2 ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( Q_islower( c1 ) ) {
			c1 -= ( 'a' - 'A' );
		}
		if ( Q_islower( c2 ) ) {
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if ( c1 != c2 ) {
			return -1;      // strings not equal
		}
	} while ( c1 );

	return 0;       // strings are equal
}

/*
===========
FS_FileCompare

Do a binary check of the two files, return false if they are different, otherwise true
===========
*/
bool FS_FileCompare( const char *s1, const char *s2 ) {
	
	FILE* f1 = fopen( s1, "rb" );
	if ( !f1 ) {
		Com_Error( ERR_FATAL, "FS_FileCompare: %s does not exist\n", s1 );
	}

	FILE* f2 = fopen( s2, "rb" );
	if ( !f2 ) {  // this file is allowed to not be there, since it might not exist in the previous build
		fclose( f1 );
		return false;
	}

	// first do a length test
	size_t pos = ftell( f1 );
	fseek( f1, 0, SEEK_END );
	size_t len1 = ftell( f1 );
	fseek( f1, pos, SEEK_SET );

	pos = ftell( f2 );
	fseek( f2, 0, SEEK_END );
	size_t len2 = ftell( f2 );
	fseek( f2, pos, SEEK_SET );

	if ( len1 != len2 ) {
		fclose( f1 );
		fclose( f2 );
		return false;
	}

	// now do a binary compare
	uint8_t* b1 = (uint8_t *)malloc( len1 );
	if ( fread( b1, 1, len1, f1 ) != len1 ) {
		Com_Error( ERR_FATAL, "Short read in FS_FileCompare()\n" );
	}
	fclose( f1 );

	uint8_t* b2 = (uint8_t *)malloc( len2 );
	if ( fread( b2, 1, len2, f2 ) != len2 ) {
		Com_Error( ERR_FATAL, "Short read in FS_FileCompare()\n" );
	}
	fclose( f2 );

	uint8_t* p1 = b1;
	uint8_t* p2 = b2;
	for ( pos = 0; pos < len1; pos++, p1++, p2++ )
	{
		if ( *p1 != *p2 ) {
			free( b1 );
			free( b2 );
			return false;
		}
	}

	// they are identical
	free( b1 );
	free( b2 );
	return true;
}

/*
===========
FS_ShiftedStrStr
===========
*/
 const char *FS_ShiftedStrStr( const char *string, const char *substring, int shift ) {
	char buf[MAX_STRING_TOKENS];
	int i=0;
	for ( ; substring[i]; i++ ) {
		buf[i] = substring[i] + shift;
	}
	buf[i] = '\0';
	return strstr( string, buf );
}

/*
===========
FS_FOpenFileRead

Finds the file in the search path.
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
extern bool com_fullyInitialized;

size_t FS_FOpenFileRead( const char *filename, fileHandle_t *file, bool uniqueFILE ) {
	searchpath_t    *search;
	char            *netpath;
	pack_t          *pak;
	fileInPack_t    *pakFile;
	directory_t     *dir;
	long hash;
	unz_s           *zfi;
	FILE            *temp;
	size_t l;

	hash = 0;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( file == nullptr ) {
		// just wants to see if file is there
		for ( search = fs_searchpaths ; search ; search = search->next ) {
			//
			if ( search->pack ) {
				hash = FS_HashFileName( filename, search->pack->hashSize );
			}
			// is the element a pak file?
			if ( search->pack && search->pack->hashTable[hash] ) {
				// look through all the pak file elements
				pak = search->pack;
				pakFile = pak->hashTable[hash];
				do {
					// case and separator insensitive comparisons
					if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
						// found it!
						return true;
					}
					pakFile = pakFile->next;
				} while ( pakFile != nullptr );
			} else if ( search->dir ) {
				dir = search->dir;

				netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
				temp = fopen( netpath, "rb" );
				if ( !temp ) {
					continue;
				}
				fclose( temp );
				return true;
			}
		}
		return false;
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: nullptr 'filename' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		*file = 0;
		return -1;
	}

	// make sure the q3key file is only readable by the quake3.exe at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if ( com_fullyInitialized && strstr( filename, "rtcwkey" ) ) {
		*file = 0;
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		//
		if ( search->pack ) {
			hash = FS_HashFileName( filename, search->pack->hashSize );
		}
		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[hash] ) {

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
					// found it!

					// mark the pak as having been referenced and mark specifics on cgame and ui
					// shaders, txt, arena files  by themselves do not count as a reference as
					// these are loaded from all pk3s
					// from every pk3 file..
					l = strlen( filename );
					if ( !( pak->referenced & FS_GENERAL_REF ) ) {
						if ( Q_stricmp( filename + l - 7, ".shader" ) != 0 &&
							 Q_stricmp( filename + l - 4, ".txt" ) != 0 &&
							 Q_stricmp( filename + l - 4, ".cfg" ) != 0 &&
							 Q_stricmp( filename + l - 7, ".config" ) != 0 &&
							 strstr( filename, "levelshots" ) == nullptr &&
							 Q_stricmp( filename + l - 4, ".bot" ) != 0 &&
							 Q_stricmp( filename + l - 6, ".arena" ) != 0 &&
							 Q_stricmp( filename + l - 5, ".menu" ) != 0 ) {
							pak->referenced |= FS_GENERAL_REF;
						}
					}

					// qagame.qvm	- 13
					// dTZT`X!di`
					if ( !( pak->referenced & FS_QAGAME_REF ) && FS_ShiftedStrStr( filename, "dTZT`X!di`", 13 ) ) {
						pak->referenced |= FS_QAGAME_REF;
					}
					// cgame.qvm	- 7
					// \`Zf^'jof
					if ( !( pak->referenced & FS_CGAME_REF ) && FS_ShiftedStrStr( filename, "\\`Zf^'jof", 7 ) ) {
						pak->referenced |= FS_CGAME_REF;
					}
					// ui.qvm		- 5
					// pd)lqh
					if ( !( pak->referenced & FS_UI_REF ) && FS_ShiftedStrStr( filename, "pd)lqh", 5 ) ) {
						pak->referenced |= FS_UI_REF;
					}

					if ( uniqueFILE ) {
						// open a new file on the pakfile
						fsh[*file].handleFiles.file.z = unzReOpen( pak->pakFilename, pak->handle );
						if ( fsh[*file].handleFiles.file.z == nullptr ) {
							Com_Error( ERR_FATAL, "Couldn't reopen %s", pak->pakFilename );
						}
					} else {
						fsh[*file].handleFiles.file.z = pak->handle;
					}
					Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
					fsh[*file].zipFile = true;
					zfi = (unz_s *)fsh[*file].handleFiles.file.z;
					// in case the file was new
					temp = zfi->file;
					// set the file position in the zip file (also sets the current file info)
					unzSetCurrentFileInfoPosition( pak->handle, pakFile->pos );
					// copy the file info into the unzip structure
					Com_Memcpy( zfi, pak->handle, sizeof( unz_s ) );
					// we copy this back into the structure
					zfi->file = temp;
					// open the file in the zip
					unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
					fsh[*file].zipFilePos = pakFile->pos;

					if ( fs_debug->integer ) {
						Com_Printf( "FS_FOpenFileRead: %s (found in '%s')\n",
									filename, pak->pakFilename );
					}
					return zfi->cur_file_info.uncompressed_size;
				}
				pakFile = pakFile->next;
			} while ( pakFile != nullptr );
		} else if ( search->dir ) {
			// check a file in the directory tree

			// if we are running restricted, the only files we
			// will allow to come from the directory are .cfg files
			l = strlen( filename );

			dir = search->dir;

			netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
			fsh[*file].handleFiles.file.o = fopen( netpath, "rb" );
			if ( !fsh[*file].handleFiles.file.o ) {
				continue;
			}

			if ( Q_stricmp( filename + l - 4, ".cfg" )       // for config files
				 && Q_stricmp( filename + l - 5, ".menu" )  // menu files
				 && Q_stricmp( filename + l - 5, ".game" )  // menu files
				 && Q_stricmp( filename + l - 4, ".dat" ) ) { // for journal files
				fs_fakeChkSum = random();
			}

			Q_strncpyz( fsh[*file].name, filename, sizeof( fsh[*file].name ) );
			fsh[*file].zipFile = false;
			if ( fs_debug->integer ) {
				Com_Printf( "FS_FOpenFileRead: %s (found in '%s/%s')\n", filename,
							dir->path, dir->gamedir );
			}

			return FS_filelength( *file );
		}
	}

	*file = 0;
	return -1;
}


/*
==============
FS_Delete
TTimo - this was not in the 1.30 filesystem code
using fs_homepath for the file to remove
==============
*/
int FS_Delete( const char *filename ) {
	char *ospath;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filename || filename[0] == 0 ) {
		return 0;
	}

	// for safety, only allow deletion from the save directory
	if ( Q_strncmp( filename, "save/", 5 ) != 0 ) {
		return 0;
	}

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( remove( ospath ) != -1 ) {  // success
		return 1;
	}


	return 0;
}

size_t FS_Read( void *buffer, size_t len, fileHandle_t f ) {

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !f ) {
		return 0;
	}

	uint8_t* buf = (uint8_t *)buffer;
	fs_readCount += len;

	if ( !fsh[f].zipFile) {
		size_t remaining = len;
		int tries = 0;
		while ( remaining ) {
			size_t block = remaining;
			size_t read = fread( buf, 1, block, fsh[f].handleFiles.file.o );
			if ( read == 0 ) {
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if ( !tries ) {
					tries = 1;
				} else {
					return len - remaining;   //Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if ( read == -1 ) {
				Com_Error( ERR_FATAL, "FS_Read: -1 bytes read" );
			}

			remaining -= read;
			buf += read;
		}
		return len;
	} else {
		return unzReadCurrentFile( fsh[f].handleFiles.file.z, buffer, len );
	}
}

/*
=================
FS_Write

Properly handles partial writes
=================
*/
size_t FS_Write( const void *buffer, size_t len, fileHandle_t h ) {
	
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !h ) {
		return 0;
	}

	FILE* f = FS_FileForHandle( h );
	uint8_t* buf = (uint8_t *)buffer;

	size_t remaining = len;
	int tries = 0;
	while ( remaining ) {
		size_t block = remaining;
		size_t written = fwrite( buf, 1, block, f );
		if ( written == 0 ) {
			if ( !tries ) {
				tries = 1;
			} else {
				Com_Printf( "FS_Write: 0 bytes written\n" );
				return 0;
			}
		}

		if ( written == -1 ) {
			Com_Printf( "FS_Write: -1 bytes written\n" );
			return 0;
		}

		remaining -= written;
		buf += written;
	}
	if ( fsh[h].handleSync ) {
		fflush( f );
	}
	return len;
}

#define MAXPRINTMSG 4096
void  FS_Printf( fileHandle_t h, const char *fmt, ... ) {
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start( argptr,fmt );
	vsnprintf( msg,MAXPRINTMSG,fmt,argptr );
	va_end( argptr );

	FS_Write( msg, strlen( msg ), h );
}

/*
=================
FS_Seek

=================
*/
size_t FS_Seek( fileHandle_t f, size_t offset, int origin ) {
	int _origin;
	char foo[65536];

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
		return -1;
	}

	if ( fsh[f].streamed ) {
		fsh[f].streamed = false;
		Sys_StreamSeek( f, offset, origin );
		fsh[f].streamed = true;
	}

	if ( fsh[f].zipFile) {
		if ( offset == 0 && origin == FS_SEEK_SET ) {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
			return unzOpenCurrentFile( fsh[f].handleFiles.file.z );
		} else if ( offset < 65536 ) {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
			unzOpenCurrentFile( fsh[f].handleFiles.file.z );
			return FS_Read( foo, offset, f );
		} else {
			Com_Error( ERR_FATAL, "ZIP FILE FSEEK NOT YET IMPLEMENTED\n" );
			return -1;
		}
	} else {
		FILE *file;
		file = FS_FileForHandle( f );
		switch ( origin ) {
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			Com_Error( ERR_FATAL, "Bad origin in FS_Seek\n" );
			break;
		}

		return fseek( file, offset, _origin );
	}
}


/*
======================================================================================

CONVENIENCE FUNCTIONS FOR ENTIRE FILES

======================================================================================
*/

int FS_FileIsInPAK( const char *filename, int *pChecksum ) {
	searchpath_t    *search;
	pack_t          *pak;
	fileInPack_t    *pakFile;
	long hash = 0;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filename ) {
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: nullptr 'filename' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[0] == '/' || filename[0] == '\\' ) {
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) ) {
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		//
		if ( search->pack ) {
			hash = FS_HashFileName( filename, search->pack->hashSize );
		}
		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[hash] ) {
			
			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
					return 1;
				}
				pakFile = pakFile->next;
			} while ( pakFile != nullptr );
		}
	}
	return -1;
}

/*
============
FS_ReadFile

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
size_t FS_ReadFile( const char *qpath, void **buffer ) {
	fileHandle_t h;

	bool isConfig;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name\n" );
	}

	uint8_t* buf = nullptr; // quiet compiler warning
	size_t len = 0;
	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( qpath, ".cfg" ) ) {
		isConfig = true;
		if ( com_journal && com_journal->integer == 2 ) {
			size_t r;

			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			r = FS_Read( &len, sizeof( len ), com_journalDataFile );
			if ( r != sizeof( len ) ) {
				if ( buffer != nullptr ) {
					*buffer = nullptr;
				}
				return -1;
			}
			// if the file didn't exist when the journal was created
			if ( !len ) {
				if ( buffer == nullptr ) {
					return 1;           // hack for old journal files
				}
				*buffer = nullptr;
				return -1;
			}
			if ( buffer == nullptr ) {
				return len;
			}

			buf = (uint8_t *)Hunk_AllocateTempMemory( len + 1 );
			*buffer = buf;

			r = FS_Read( buf, len, com_journalDataFile );
			if ( r != len ) {
				Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
			}

			fs_loadCount++;
			fs_loadStack++;

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	} else {
		isConfig = false;
	}

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead( qpath, &h, false );
	if ( h == 0 ) {
		if ( buffer ) {
			*buffer = nullptr;
		}
		// if we are journalling and it is a config file, write a zero to the journal file
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
			len = 0;
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		return -1;
	}

	if ( !buffer ) {
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}
		FS_FCloseFile( h );
		return len;
	}

	fs_loadCount++;
	fs_loadStack++;

	buf = (uint8_t *)Hunk_AllocateTempMemory( len + 1 );
	*buffer = buf;

	FS_Read( buf, len, h );

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	FS_FCloseFile( h );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && com_journal && com_journal->integer == 1 ) {
		Com_DPrintf( "Writing %s to journal file.\n", qpath );
		FS_Write( &len, sizeof( len ), com_journalDataFile );
		FS_Write( buf, len, com_journalDataFile );
		FS_Flush( com_journalDataFile );
	}
	return len;
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile( void *buffer ) {
	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}
	if ( !buffer ) {
		Com_Error( ERR_FATAL, "FS_FreeFile( nullptr )" );
	}
	fs_loadStack--;

	Hunk_FreeTempMemory( buffer );

}

/*
============
FS_WriteFile

Filename are reletive to the quake search path
============
*/
void FS_WriteFile( const char *qpath, const void *buffer, size_t size ) {
	fileHandle_t f;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !qpath || !buffer ) {
		Com_Error( ERR_FATAL, "FS_WriteFile: nullptr parameter" );
	}

	f = FS_FOpenFileWrite( qpath );
	if ( !f ) {
		Com_Printf( "Failed to open %s\n", qpath );
		return;
	}

	FS_Write( buffer, size, f );

	FS_FCloseFile( f );
}



/*
==========================================================================

ZIP FILE LOADING

==========================================================================
*/

/*
=================
FS_LoadZipFile

Creates a new pak_t in the search chain for the contents
of a zip file.
=================
*/
static pack_t *FS_LoadZipFile( char *zipfile, const char *basename ) {
	fileInPack_t    *buildBuffer;
	pack_t          *pack;
	unzFile uf;
	int err;
	unz_global_info gi;
	char filename_inzip[MAX_ZPATH];
	unz_file_info file_info;
	int i, len;
	long hash;
	int fs_numHeaderLongs;
	int             *fs_headerLongs;
	char            *namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen( zipfile );
	err = unzGetGlobalInfo( uf,&gi );

	if ( err != UNZ_OK ) {
		return nullptr;
	}

	fs_packFiles += gi.number_entry;

	len = 0;
	unzGoToFirstFile( uf );
	for ( i = 0; i < gi.number_entry; i++ )
	{
		err = unzGetCurrentFileInfo( uf, &file_info, filename_inzip, sizeof( filename_inzip ), nullptr, 0, nullptr, 0 );
		if ( err != UNZ_OK ) {
			break;
		}
		len += strlen( filename_inzip ) + 1;
		unzGoToNextFile( uf );
	}

	buildBuffer = (fileInPack_t *)calloc(1,  ( gi.number_entry * sizeof( fileInPack_t ) ) + len );
	namePtr = ( (char *) buildBuffer ) + gi.number_entry * sizeof( fileInPack_t );
	fs_headerLongs = (int *)calloc(1,  gi.number_entry * sizeof( int ) );

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for ( i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1 ) {
		if ( i > gi.number_entry ) {
			break;
		}
	}

	pack = (pack_t *)calloc(1,  sizeof( pack_t ) + i * sizeof( fileInPack_t * ) );
	pack->hashSize = i;
	pack->hashTable = ( fileInPack_t ** )( ( (char *) pack ) + sizeof( pack_t ) );
	for ( i = 0; i < pack->hashSize; i++ ) {
		pack->hashTable[i] = nullptr;
	}

	Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );
	Q_strncpyz( pack->pakBasename, basename, sizeof( pack->pakBasename ) );

	// strip .pk3 if needed
	if ( strlen( pack->pakBasename ) > 4 && !Q_stricmp( pack->pakBasename + strlen( pack->pakBasename ) - 4, ".pk3" ) ) {
		pack->pakBasename[strlen( pack->pakBasename ) - 4] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile( uf );

	for ( i = 0; i < gi.number_entry; i++ )
	{
		err = unzGetCurrentFileInfo( uf, &file_info, filename_inzip, sizeof( filename_inzip ), nullptr, 0, nullptr, 0 );
		if ( err != UNZ_OK ) {
			break;
		}
		if ( file_info.uncompressed_size > 0 ) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong( file_info.crc );
		}
		Q_strlwr( filename_inzip );
		hash = FS_HashFileName( filename_inzip, pack->hashSize );
		buildBuffer[i].name = namePtr;
		strcpy( buildBuffer[i].name, filename_inzip );
		namePtr += strlen( filename_inzip ) + 1;
		// store the file position in the zip
		unzGetCurrentFileInfoPosition( uf, &buildBuffer[i].pos );
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile( uf );
	}

	pack->checksum = Com_BlockChecksum( &fs_headerLongs[ 1 ], sizeof(*fs_headerLongs) * ( fs_numHeaderLongs - 1 ) );
	pack->checksum = LittleLong( pack->checksum );

	free( fs_headerLongs );

	pack->buildBuffer = buildBuffer;
	return pack;
}

/*
=================================================================================

DIRECTORY SCANNING FUNCTIONS

=================================================================================
*/

#define MAX_FOUND_FILES 0x1000

static int FS_ReturnPath( const char *zname, char *zpath, int *depth ) {
	int len, at, newdep;

	newdep = 0;
	zpath[0] = 0;
	len = 0;
	at = 0;

	while ( zname[at] != 0 )
	{
		if ( zname[at] == '/' || zname[at] == '\\' ) {
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy( zpath, zname );
	zpath[len] = 0;
	*depth = newdep;

	return len;
}

/*
==================
FS_AddFileToList
==================
*/
static int FS_AddFileToList( char *name, char *list[MAX_FOUND_FILES], int nfiles ) {
	int i;

	if ( nfiles == MAX_FOUND_FILES - 1 ) {
		return nfiles;
	}
	for ( i = 0 ; i < nfiles ; i++ ) {
		if ( !Q_stricmp( name, list[i] ) ) {
			return nfiles;      // allready in list
		}
	}
	list[nfiles] = CopyString( name );
	nfiles++;

	return nfiles;
}

/*
===============
FS_ListFilteredFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
char **FS_ListFilteredFiles( const char *path, const char *extension, const char *filter, int *numfiles ) {

	char            **listCopy;
	char            *list[MAX_FOUND_FILES];

	char zpath[MAX_ZPATH];

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !path ) {
		*numfiles = 0;
		return nullptr;
	}
	if ( !extension ) {
		extension = "";
	}

	size_t pathLength = strlen( path );
	if ( path[pathLength - 1] == '\\' || path[pathLength - 1] == '/' ) {
		pathLength--;
	}
	size_t extensionLength = strlen( extension );
	int nfiles = 0;
	int pathDepth;
	FS_ReturnPath( path, zpath, &pathDepth );

	//
	// search through the path, one element at a time, adding to list
	//
	for (searchpath_t *search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack ) {

			// look through all the pak file elements
			pack_t* pak = search->pack;
			fileInPack_t* buildBuffer = pak->buildBuffer;
			for (int i = 0; i < pak->numfiles; i++ ) {
				char    *name;
				int zpathLen, depth;

				// check for directory match
				name = buildBuffer[i].name;
				//
				if ( filter ) {
					// case insensitive
					if ( !Com_FilterPath( filter, name, false ) ) {
						continue;
					}
					// unique the match
					nfiles = FS_AddFileToList( name, list, nfiles );
				} else {

					zpathLen = FS_ReturnPath( name, zpath, &depth );

					if ( ( depth - pathDepth ) > 2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) ) {
						continue;
					}

					// check for extension match
					size_t length = strlen( name );
					if ( length < extensionLength ) {
						continue;
					}

					if ( Q_stricmp( name + length - extensionLength, extension ) ) {
						continue;
					}
					// unique the match

					size_t temp = pathLength;
					if ( pathLength ) {
						temp++;     // include the '/'
					}
					nfiles = FS_AddFileToList( name + temp, list, nfiles );
				}
			}
		} else if ( search->dir ) { // scan for files in the filesystem
			int numSysFiles;

			char* netpath = FS_BuildOSPath( search->dir->path, search->dir->gamedir, path );
			char **sysFiles = Sys_ListFiles( netpath, extension, filter, &numSysFiles, false );
			for (int i = 0 ; i < numSysFiles ; i++ ) {
				// unique the match
				char* name = sysFiles[i];
				nfiles = FS_AddFileToList( name, list, nfiles );
			}
			Sys_FreeFileList( sysFiles );
			
		}
	}

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return nullptr;
	}

	listCopy = (char **)calloc(1,  ( nfiles + 1 ) * sizeof( *listCopy ) );
	for (int i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[nfiles] = nullptr;

	return listCopy;
}

/*
=================
FS_ListFiles
=================
*/
char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) {
	return FS_ListFilteredFiles( path, extension, nullptr, numfiles );
}

/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList( char **list ) {
	int i;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		free( list[i] );
	}

	free( list );
}


/*
================
FS_GetFileList
================
*/
int FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {

	*listbuf = 0;
	int nFiles = 0;
	int nTotal = 0;

	if ( Q_stricmp( path, "$modlist" ) == 0 ) {
		return FS_GetModList( listbuf, bufsize );
	}

	char** pFiles = FS_ListFiles( path, extension, &nFiles );

	for (int i = 0; i < nFiles; i++ ) {
		int nLen = (int)strlen( pFiles[i] ) + 1;
		if ( nTotal + nLen + 1 < bufsize ) {
			strcpy( listbuf, pFiles[i] );
			listbuf += nLen;
			nTotal += nLen;
		} else {
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList( pFiles );

	return nFiles;
}

/*
=======================
Sys_ConcatenateFileLists

mkv: Naive implementation. Concatenates three lists into a
	 new list, and frees the old lists from the heap.
bk001129 - from cvs1.17 (mkv)

FIXME TTimo those two should move to common.c next to Sys_ListFiles
=======================
 */
static unsigned int Sys_CountFileList( char **list ) {
	int i = 0;

	if ( list ) {
		while ( *list )
		{
			list++;
			i++;
		}
	}
	return i;
}

static char** Sys_ConcatenateFileLists( char **list0, char **list1, char **list2 ) {
	int totalLength = 0;
	char** cat = nullptr, **dst, **src;

	totalLength += Sys_CountFileList( list0 );
	totalLength += Sys_CountFileList( list1 );
	totalLength += Sys_CountFileList( list2 );

	/* Create new list. */
	dst = cat = (char **)calloc(1, ( totalLength + 1 ) * sizeof( char* ) );

	/* Copy over lists. */
	if ( list0 ) {
		for ( src = list0; *src; src++, dst++ )
			*dst = *src;
	}
	if ( list1 ) {
		for ( src = list1; *src; src++, dst++ )
			*dst = *src;
	}
	if ( list2 ) {
		for ( src = list2; *src; src++, dst++ )
			*dst = *src;
	}

	// Terminate the list
	*dst = nullptr;

	// Free our old lists.
	// NOTE: not freeing their content, it's been merged in dst and still being used
	if ( list0 ) {
		free( list0 );
	}
	if ( list1 ) {
		free( list1 );
	}
	if ( list2 ) {
		free( list2 );
	}

	return cat;
}

/*
================
FS_GetModList

Returns a list of mod directory names
A mod directory is a peer to baseq3 with a pk3 in it
The directories are searched in base path, cd path and home path
================
*/
int FS_GetModList( char *listbuf, int bufsize ) {
	int nMods, i, j, nTotal, nPaks, nPotential;
	char **pFiles = nullptr;
	char **pPaks = nullptr;
	char *name, *path;
	char descPath[MAX_OSPATH];
	fileHandle_t descHandle;

	int dummy;
	char **pFiles0 = nullptr;
	char **pFiles1 = nullptr;
	char **pFiles2 = nullptr;
	bool bDrop = false;

	*listbuf = 0;
	nMods = nPotential = nTotal = 0;

	pFiles0 = Sys_ListFiles( fs_homepath->string, nullptr, nullptr, &dummy, true );
	pFiles1 = Sys_ListFiles( fs_basepath->string, nullptr, nullptr, &dummy, true );

	// we searched for mods in the two paths
	// it is likely that we have duplicate names now, which we will cleanup below
	pFiles = Sys_ConcatenateFileLists( pFiles0, pFiles1, pFiles2 );
	nPotential = Sys_CountFileList( pFiles );

	for ( i = 0 ; i < nPotential ; i++ ) {
		name = pFiles[i];
		// NOTE: cleaner would involve more changes
		// ignore duplicate mod directories
		if ( i != 0 ) {
			bDrop = false;
			for ( j = 0; j < i; j++ )
			{
				if ( Q_stricmp( pFiles[j],name ) == 0 ) {
					// this one can be dropped
					bDrop = true;
					break;
				}
			}
		}
		if ( bDrop ) {
			continue;
		}
		// we drop the basegame, "." and ".."
		if ( Q_stricmp( name, BASEGAME ) && Q_stricmpn( name, ".", 1 ) ) {
			// now we need to find some .pk3 files to validate the mod
			// NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			// we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			//   so it could be in base path, cd path or home path
			//   we will try each three of them here (yes, it's a bit messy)
			path = FS_BuildOSPath( fs_basepath->string, name, "" );
			nPaks = 0;
			pPaks = Sys_ListFiles( path, ".pk3", nullptr, &nPaks, false );
			Sys_FreeFileList( pPaks ); // we only use Sys_ListFiles to check wether .pk3 files are present

			/* try on home path */
			if ( nPaks <= 0 ) {
				path = FS_BuildOSPath( fs_homepath->string, name, "" );
				nPaks = 0;
				pPaks = Sys_ListFiles( path, ".pk3", nullptr, &nPaks, false );
				Sys_FreeFileList( pPaks );
			}

			if ( nPaks > 0 ) {
				size_t nLen = strlen( name ) + 1;
				// nLen is the length of the mod path
				// we need to see if there is a description available
				descPath[0] = '\0';
				strcpy( descPath, name );
				strcat( descPath, "/description.txt" );
				size_t nDescLen = FS_SV_FOpenFileRead( descPath, &descHandle );
				if ( nDescLen > 0 && descHandle ) {
					FILE *file;
					file = FS_FileForHandle( descHandle );
					Com_Memset( descPath, 0, sizeof( descPath ) );
					nDescLen = fread( descPath, 1, 48, file );
					if ( nDescLen >= 0 ) {
						descPath[nDescLen] = '\0';
					}
					FS_FCloseFile( descHandle );
				} else {
					strcpy( descPath, name );
				}
				nDescLen = strlen( descPath ) + 1;

				if ( nTotal + nLen + 1 + nDescLen + 1 < bufsize ) {
					strcpy( listbuf, name );
					listbuf += nLen;
					strcpy( listbuf, descPath );
					listbuf += nDescLen;
					nTotal += nLen + nDescLen;
					nMods++;
				} else {
					break;
				}
			}
		}
	}
	Sys_FreeFileList( pFiles );

	return nMods;
}




//============================================================================

/*
================
FS_Dir_f
================
*/
void FS_Dir_f( void ) {
	const char    *path;
	const char    *extension;
	char    **dirnames;
	int ndirs;
	int i;

	if ( Cmd_Argc() < 2 || Cmd_Argc() > 3 ) {
		Com_Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( Cmd_Argc() == 2 ) {
		path = Cmd_Argv( 1 );
		extension = "";
	} else {
		path = Cmd_Argv( 1 );
		extension = Cmd_Argv( 2 );
	}

	Com_Printf( "Directory of %s %s\n", path, extension );
	Com_Printf( "---------------\n" );

	dirnames = FS_ListFiles( path, extension, &ndirs );

	for ( i = 0; i < ndirs; i++ ) {
		Com_Printf( "%s\n", dirnames[i] );
	}
	FS_FreeFileList( dirnames );
}

/*
===========
FS_ConvertPath
===========
*/
void FS_ConvertPath( char *s ) {
	while ( *s ) {
		if ( *s == '\\' || *s == ':' ) {
			*s = '/';
		}
		s++;
	}
}

/*
===========
FS_PathCmp

Ignore case and seprator char distinctions
===========
*/
int FS_PathCmp( const char *s1, const char *s2 ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( Q_islower( c1 ) ) {
			c1 -= ( 'a' - 'A' );
		}
		if ( Q_islower( c2 ) ) {
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if ( c1 < c2 ) {
			return -1;      // strings not equal
		}
		if ( c1 > c2 ) {
			return 1;
		}
	} while ( c1 );

	return 0;       // strings are equal
}

/*
================
FS_SortFileList
================
*/
void FS_SortFileList( char **filelist, int numfiles ) {
	int i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist = (char **)calloc(1, ( numfiles + 1 ) * sizeof( *sortedlist ) );
	sortedlist[0] = nullptr;
	numsortedfiles = 0;
	for ( i = 0; i < numfiles; i++ ) {
		for ( j = 0; j < numsortedfiles; j++ ) {
			if ( FS_PathCmp( filelist[i], sortedlist[j] ) < 0 ) {
				break;
			}
		}
		for ( k = numsortedfiles; k > j; k-- ) {
			sortedlist[k] = sortedlist[k - 1];
		}
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Com_Memcpy( filelist, sortedlist, numfiles * sizeof( *filelist ) );
	free( sortedlist );
}

/*
================
FS_NewDir_f
================
*/
void FS_NewDir_f( void ) {

	char    **dirnames;
	int ndirs;
	int i;

	if ( Cmd_Argc() < 2 ) {
		Com_Printf( "usage: fdir <filter>\n" );
		Com_Printf( "example: fdir *q3dm*.bsp\n" );
		return;
	}

	const char *filter = Cmd_Argv( 1 );

	Com_Printf( "---------------\n" );

	dirnames = FS_ListFilteredFiles( "", "", filter, &ndirs );

	FS_SortFileList( dirnames, ndirs );

	for ( i = 0; i < ndirs; i++ ) {
		FS_ConvertPath( dirnames[i] );
		Com_Printf( "%s\n", dirnames[i] );
	}
	Com_Printf( "%d files listed\n", ndirs );
	FS_FreeFileList( dirnames );
}

/*
============
FS_Path_f

============
*/
void FS_Path_f( void ) {
	searchpath_t    *s;
	int i;

	Com_Printf( "Current search path:\n" );
	for ( s = fs_searchpaths; s; s = s->next ) {
		if ( s->pack ) {
			Com_Printf( "%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles );
		} else {
			Com_Printf( "%s/%s\n", s->dir->path, s->dir->gamedir );
		}
	}


	Com_Printf( "\n" );
	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o ) {
			Com_Printf( "handle %i: %s\n", i, fsh[i].name );
		}
	}
}


//===========================================================================


static int  paksort( const void *a, const void *b ) {
	char    *aa, *bb;

	aa = *(char **)a;
	bb = *(char **)b;

	return FS_PathCmp( aa, bb );
}

/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads the zip headers
================
*/
#define MAX_PAKFILES    1024
static void FS_AddGameDirectory( const char *path, const char *dir ) {
	searchpath_t    *sp;
	int i;
	searchpath_t    *search;
	pack_t          *pak;
	char            *pakfile;
	int numfiles;
	char            **pakfiles;
	char            *sorted[MAX_PAKFILES];


	for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
		if ( sp->dir && !Q_stricmp( sp->dir->path, path ) && !Q_stricmp( sp->dir->gamedir, dir ) ) {
			return;         // we've already got this one
		}
	}

	Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );

	//
	// add the directory to the search path
	//
	search = (searchpath_t *)calloc(1, sizeof( searchpath_t ) );
	search->dir = (directory_t *)calloc(1, sizeof( *search->dir ) );

	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	// find all pak files in this directory
	pakfile = FS_BuildOSPath( path, dir, "" );
	pakfile[ strlen( pakfile ) - 1 ] = 0; // strip the trailing slash

	pakfiles = Sys_ListFiles( pakfile, ".pk3", nullptr, &numfiles, false );

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override pak0.pk3
	if ( numfiles > MAX_PAKFILES ) {
		numfiles = MAX_PAKFILES;
	}
	for ( i = 0 ; i < numfiles ; i++ ) {
		sorted[i] = pakfiles[i];

		if ( !Q_strncmp( sorted[i],"sp_",3 ) ) { //	sort sp first
			memcpy( sorted[i],"zz",2 );
		}

	}

	qsort( sorted, numfiles, sizeof(char*), paksort );

	for ( i = 0 ; i < numfiles ; i++ ) {

		if ( Q_strncmp( sorted[i],"mp_",3 ) ) { 

			if ( !Q_strncmp( sorted[i],"zz_",3 ) ) {
				memcpy( sorted[i],"sp",2 );
			}

			pakfile = FS_BuildOSPath( path, dir, sorted[i] );
			if ( ( pak = FS_LoadZipFile( pakfile, sorted[i] ) ) == 0 ) {
				continue;
			}
			// store the game name for downloading
			strcpy( pak->pakGamename, dir );

			search = (searchpath_t *)calloc(1, sizeof( searchpath_t ) );
			search->pack = pak;
			search->next = fs_searchpaths;
			fs_searchpaths = search;
		}
	}

	// done
	Sys_FreeFileList( pakfiles );
}

/*
================
FS_Shutdown

Frees all resources and closes all files
================
*/
void FS_Shutdown( bool closemfp ) {
	searchpath_t    *p, *next;
	int i;

	for ( i = 0; i < MAX_FILE_HANDLES; i++ ) {
		if ( fsh[i].fileSize ) {
			FS_FCloseFile( i );
		}
	}

	// free everything
	for ( p = fs_searchpaths ; p ; p = next ) {
		next = p->next;

		if ( p->pack ) {
			unzClose( p->pack->handle );
			free( p->pack->buildBuffer );
			free( p->pack );
		}
		if ( p->dir ) {
			free( p->dir );
		}
		free( p );
	}

	// any FS_ calls will now be an error until reinitialized
	fs_searchpaths = nullptr;

	Cmd_RemoveCommand( "path" );
	Cmd_RemoveCommand( "dir" );
	Cmd_RemoveCommand( "fdir" );
	Cmd_RemoveCommand( "touchFile" );
}

/*
================
FS_Startup
================
*/
static void FS_Startup( const char *gameName ) {
	const char *homePath;
	cvar_t  *fs;

	Com_Printf( "----- FS_Startup -----\n" );

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );

	fs_basepath = Cvar_Get( "fs_basepath", Sys_DefaultInstallPath(), CVAR_INIT );
	fs_basegame = Cvar_Get( "fs_basegame", "", CVAR_INIT );
	homePath = Sys_DefaultHomePath();
	if ( !homePath || !homePath[0] ) {
		homePath = fs_basepath->string;
	}
	fs_homepath = Cvar_Get( "fs_homepath", homePath, CVAR_INIT );
	fs_gamedirvar = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );

	// add search path elements in reverse priority order
	if ( fs_basepath->string[0] ) {
		FS_AddGameDirectory( fs_basepath->string, gameName );
	}
	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if ( fs_basepath->string[0] && Q_stricmp( fs_homepath->string,fs_basepath->string ) ) {
		FS_AddGameDirectory( fs_homepath->string, gameName );
	}

	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[0] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_basegame->string, gameName ) ) {
		if ( fs_basepath->string[0] ) {
			FS_AddGameDirectory( fs_basepath->string, fs_basegame->string );
		}
		if ( fs_homepath->string[0] && Q_stricmp( fs_homepath->string,fs_basepath->string ) ) {
			FS_AddGameDirectory( fs_homepath->string, fs_basegame->string );
		}
	}

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[0] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_gamedirvar->string, gameName ) ) {
		if ( fs_basepath->string[0] ) {
			FS_AddGameDirectory( fs_basepath->string, fs_gamedirvar->string );
		}
		if ( fs_homepath->string[0] && Q_stricmp( fs_homepath->string,fs_basepath->string ) ) {
			FS_AddGameDirectory( fs_homepath->string, fs_gamedirvar->string );
		}
	}

	// add our commands
	Cmd_AddCommand( "path", FS_Path_f );
	Cmd_AddCommand( "dir", FS_Dir_f );
	Cmd_AddCommand( "fdir", FS_NewDir_f );

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = false; // We just loaded, it's not modified

	Com_Printf( "----------------------\n" );

	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}

/*
=====================
FS_ClearPakReferences
=====================
*/
void FS_ClearPakReferences( int flags ) {
	searchpath_t *search;

	if ( !flags ) {
		flags = -1;
	}
	for ( search = fs_searchpaths; search; search = search->next ) {
		// is the element a pak file and has it been referenced?
		if ( search->pack ) {
			search->pack->referenced &= ~flags;
		}
	}
}

/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem( void ) {
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );

	// try to start up normally
	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", nullptr ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}


/*
================
FS_Restart
================
*/
void FS_Restart( int checksumFeed ) {

	// free anything we currently have loaded
	FS_Shutdown( false );

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences( 0 );

	// try to start up normally
	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", nullptr ) <= 0 ) {
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if ( lastValidBase[0] ) {

			Cvar_Set( "fs_basepath", lastValidBase );
			Cvar_Set( "fs_gamedirvar", lastValidGame );
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart( checksumFeed );
			Com_Error( ERR_DROP, "Invalid game folder\n" );
            return; // keep the linter happy, ERR_DROP does not return
		}
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	// bk010116 - new check before safeMode
	if ( Q_stricmp( fs_gamedirvar->string, lastValidGame ) ) {
		// skip the wolfconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText( "exec wolfconfig.cfg\n" );
		}
	}

	Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );

}

/*
=================
FS_ConditionalRestart
restart if necessary
=================
*/
bool FS_ConditionalRestart( int checksumFeed ) {
	if ( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed ) {
		FS_Restart( checksumFeed );
		return true;
	}
	return false;
}

int     FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	int r;
	bool sync;

	sync = false;

	switch ( mode ) {
	case FS_READ:
		r = FS_FOpenFileRead( qpath, f, true );
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite( qpath );
		r = 0;
		if ( *f == 0 ) {
			r = -1;
		}
		break;
	case FS_APPEND_SYNC:
		sync = true;
	case FS_APPEND:
		*f = FS_FOpenFileAppend( qpath );
		r = 0;
		if ( *f == 0 ) {
			r = -1;
		}
		break;
	default:
		Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
		return -1;
	}

	if ( !f ) {
		return r;
	}

	if ( *f ) {
		if ( fsh[*f].zipFile) {
			fsh[*f].baseOffset = unztell( fsh[*f].handleFiles.file.z );
		} else {
			fsh[*f].baseOffset = ftell( fsh[*f].handleFiles.file.o );
		}
		fsh[*f].fileSize = r;
		fsh[*f].streamed = false;
	}
	fsh[*f].handleSync = sync;

	return r;
}

size_t     FS_FTell( fileHandle_t f ) {
	size_t pos;
	if ( fsh[f].zipFile) {
		pos = unztell( fsh[f].handleFiles.file.z );
	} else {
		pos = ftell( fsh[f].handleFiles.file.o );
	}
	return pos;
}

void    FS_Flush( fileHandle_t f ) {
	fflush( fsh[f].handleFiles.file.o );
}

