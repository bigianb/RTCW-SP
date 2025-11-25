#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>

#include <SDL3/SDL.h>
#include "../qcommon/qcommon.h"

char *Sys_Cwd( void )
{
	return SDL_GetCurrentDirectory();
}

const char *Sys_DefaultCDPath( void ) {
	return "";
}

const char    *Sys_DefaultHomePath( void ) {
	return NULL;
}

void *Sys_InitializeCriticalSection() {
	//Com_DPrintf( "WARNING: Sys_InitializeCriticalSection not implemented\n" );
	return (void *)-1;
}

void Sys_EnterCriticalSection( void *ptr ) {
}

void Sys_LeaveCriticalSection( void *ptr ) {
}

#define MAX_FOUND_FILES 0x1000

void Sys_ListFilteredFiles( const char *basedir, const char *subdirs, const char *filter, char **list, int *numfiles ) {
	char search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char filename[MAX_OSPATH];
	DIR         *fdir;
	struct dirent *d;
	struct stat st;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if ( strlen( subdirs ) ) {
		snprintf( search, sizeof( search ), "%s/%s", basedir, subdirs );
	} else {
		snprintf( search, sizeof( search ), "%s", basedir );
	}

	if ( ( fdir = opendir( search ) ) == NULL ) {
		return;
	}

	while ( ( d = readdir( fdir ) ) != NULL ) {
		snprintf( filename, sizeof( filename ), "%s/%s", search, d->d_name );
		if ( stat( filename, &st ) == -1 ) {
			continue;
		}

		if ( st.st_mode & S_IFDIR ) {
			if ( Q_stricmp( d->d_name, "." ) && Q_stricmp( d->d_name, ".." ) ) {
				if ( strlen( subdirs ) ) {
					snprintf( newsubdirs, sizeof( newsubdirs ), "%s/%s", subdirs, d->d_name );
				} else {
					snprintf( newsubdirs, sizeof( newsubdirs ), "%s", d->d_name );
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		snprintf( filename, sizeof( filename ), "%s/%s", subdirs, d->d_name );
		if ( !Com_FilterPath( filter, filename, qfalse ) ) {
			continue;
		}
		list[ *numfiles ] = CopyString( filename );
		( *numfiles )++;
	}

	closedir( fdir );
}


char **Sys_ListFiles( const char *directory, const char *extension, const char *filter, int *numfiles, qboolean wantsubs ) {
	struct dirent *d;
	// char *p; // bk001204 - unused
	DIR     *fdir;
	qboolean dironly = wantsubs;
	char search[MAX_OSPATH];
	int nfiles;
	char        **listCopy;
	char        *list[MAX_FOUND_FILES];
	//int			flag; // bk001204 - unused
	int i;
	struct stat st;

	int extLen;

	if ( filter ) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if ( !nfiles ) {
			return NULL;
		}

		listCopy = (char **)calloc(1, ( nfiles + 1 ) * sizeof( *listCopy ) );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension ) {
		extension = "";
	}

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = qtrue;
	}

	extLen = strlen( extension );

	// search
	nfiles = 0;

	if ( ( fdir = opendir( directory ) ) == NULL ) {
		*numfiles = 0;
		return NULL;
	}

	while ( ( d = readdir( fdir ) ) != NULL ) {
		snprintf( search, sizeof( search ), "%s/%s", directory, d->d_name );
		if ( stat( search, &st ) == -1 ) {
			continue;
		}
		if ( ( dironly && !( st.st_mode & S_IFDIR ) ) ||
			 ( !dironly && ( st.st_mode & S_IFDIR ) ) ) {
			continue;
		}

		if ( *extension ) {
			if ( strlen( d->d_name ) < strlen( extension ) ||
				 Q_stricmp(
					 d->d_name + strlen( d->d_name ) - strlen( extension ),
					 extension ) ) {
				continue; // didn't match
			}
		}

		if ( nfiles == MAX_FOUND_FILES - 1 ) {
			break;
		}
		list[ nfiles ] = CopyString( d->d_name );
		nfiles++;
	}

	list[ nfiles ] = 0;

	closedir( fdir );

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char **)calloc(1, ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void    Sys_FreeFileList( char **list ) {
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		free( list[i] );
	}

	free( list );
}

const char *Sys_GetCurrentUser( void ) {
	return "";
}

int Sys_GetProcessorId( void ) {
	// TODO TTimo add better CPU identification?
	// see Sys_GetHighQualityCPU
	return CPUID_GENERIC;
}

int Sys_GetHighQualityCPU() {
	// TODO TTimo see win_shared.c IsP3 || IsAthlon
	return 0;
}

qboolean Sys_LowPhysicalMemory() {
	return qfalse; 
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose ) {
}

time_t sys_timeBase = 0;
int Sys_Milliseconds( void ) {
	struct timeval tp;
	struct timezone tzp;

	gettimeofday( &tp, &tzp );

	if ( !sys_timeBase ) {
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	return (int)( tp.tv_sec - sys_timeBase ) * 1000 + tp.tv_usec / 1000;
}

void    Sys_Mkdir( const char *path ) {
	mkdir( path, 0777 );
}
