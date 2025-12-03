/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../renderer/tr_local.h"
//#include "sdl_icon.h"

float displayAspect = 1.333f; // default to 4:3 aspect ratio

typedef enum
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

SDL_Window *SDL_window = nullptr;
static SDL_GLContext SDL_glContext = nullptr;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;

int qglMajorVersion, qglMinorVersion;
int qglesMajorVersion, qglesMinorVersion;

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	IN_Shutdown();

	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize( void )
{
	SDL_MinimizeWindow( SDL_window );
}


/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( const char *comment )
{
}

/*
===============
GLimp_CompareModes
===============
*/
static int GLimp_CompareModes( const void *a, const void *b )
{
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = (SDL_Rect *)a;
	SDL_Rect *modeB = (SDL_Rect *)b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabs( aspectA - displayAspect );
	float aspectDiffB = fabs( aspectB - displayAspect );
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if( aspectDiffsDiff > ASPECT_EPSILON )
		return 1;
	else if( aspectDiffsDiff < -ASPECT_EPSILON )
		return -1;
	else
		return areaA - areaB;
}

/*
===============
GLimp_SetMode
===============
*/
static int GLimp_SetMode(int mode, bool fullscreen, bool noborder, bool fixedFunction)
{
	const char *glstring;
	int perChannelColorBits;
	int colorBits, depthBits, stencilBits;
	int samples;
	int i = 0;
	SDL_Surface *icon = nullptr;
	Uint32 flags = SDL_WINDOW_OPENGL;
	
	int display = 0;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;

	ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");

	if ( r_allowResize->integer ){
		flags |= SDL_WINDOW_RESIZABLE;
	}
	// If a window exists, note its display index
	if( SDL_window != nullptr )
	{
		display = SDL_GetDisplayForWindow( SDL_window );
		if( display < 0 )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError() );
		}
	}

	const SDL_DisplayMode* desktopMode = SDL_GetDesktopDisplayMode( display );
	if (desktopMode)
	{
		displayAspect = (float)desktopMode->w / (float)desktopMode->h;
		ri.Printf( PRINT_ALL, "Display aspect: %.3f\n", displayAspect );
	}
	else
	{
		ri.Printf( PRINT_ALL, "Cannot determine display aspect, assuming 1.333\n" );
	}

	ri.Printf (PRINT_ALL, "...setting mode %d:", mode );

	if (mode == -2)
	{
		// use desktop video resolution
		if( desktopMode != nullptr && desktopMode->h > 0 )
		{
			glConfig.vidWidth = desktopMode->w;
			glConfig.vidHeight = desktopMode->h;
		}
		else
		{
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			ri.Printf( PRINT_ALL,
					"Cannot determine display resolution, assuming 640x480\n" );
		}

		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	}
	else if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
	{
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

	// Center window
	if( desktopMode && r_centerWindow->integer && !fullscreen )
	{
		x = ( desktopMode->w / 2 ) - ( glConfig.vidWidth / 2 );
		y = ( desktopMode->h / 2 ) - ( glConfig.vidHeight / 2 );
	}

	// Destroy existing state if it exists
	if( SDL_glContext != nullptr )
	{
		SDL_GL_DestroyContext( SDL_glContext );
		SDL_glContext = nullptr;
	}

	if( SDL_window != nullptr )
	{
		SDL_GetWindowPosition( SDL_window, &x, &y );
		ri.Printf( PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y );
		SDL_DestroyWindow( SDL_window );
		SDL_window = nullptr;
	}

	if( fullscreen )
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = true;
	}
	else
	{
		if( noborder )
			flags |= SDL_WINDOW_BORDERLESS;

		glConfig.isFullscreen = false;
	}

	colorBits = r_colorbits->value;
	if ((!colorBits) || (colorBits >= 32))
		colorBits = 24;

	if (!r_depthbits->value)
		depthBits = 24;
	else
		depthBits = r_depthbits->value;

	stencilBits = r_stencilbits->value;
	//samples = r_ext_multisample->value;

	for (i = 0; i < 16; i++)
	{
		int testColorBits, testDepthBits, testStencilBits;
		int realColorBits[3];

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i)
		{
			// one pass, reduce
			switch (i / 4)
			{
				case 2 :
					if (colorBits == 24)
						colorBits = 16;
					break;
				case 1 :
					if (depthBits == 24)
						depthBits = 16;
					else if (depthBits == 16)
						depthBits = 8;
				case 3 :
					if (stencilBits == 24)
						stencilBits = 16;
					else if (stencilBits == 16)
						stencilBits = 8;
			}
		}

		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3)
		{ // reduce colorBits
			if (testColorBits == 24)
				testColorBits = 16;
		}

		if ((i % 4) == 2)
		{ // reduce depthBits
			if (testDepthBits == 24)
				testDepthBits = 16;
			else if (testDepthBits == 16)
				testDepthBits = 8;
		}

		if ((i % 4) == 1)
		{ // reduce stencilBits
			if (testStencilBits == 24)
				testStencilBits = 16;
			else if (testStencilBits == 16)
				testStencilBits = 8;
			else
				testStencilBits = 0;
		}

		if (testColorBits == 24)
			perChannelColorBits = 8;
		else
			perChannelColorBits = 4;

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, perChannelColorBits );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, testDepthBits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, testStencilBits );

		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, samples );

		glConfig.stereoEnabled = false;
		SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		if( ( SDL_window = SDL_CreateWindow( "Wolf",
				glConfig.vidWidth, glConfig.vidHeight, flags ) ) == nullptr )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
			continue;
		}

		if( fullscreen )
		{
			SDL_DisplayMode mode;

			switch( testColorBits )
			{
				case 16: mode.format = SDL_PIXELFORMAT_RGB565; break;
				case 24: mode.format = SDL_PIXELFORMAT_RGB24;  break;
				default: ri.Printf( PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits ); continue;
			}

			mode.w = glConfig.vidWidth;
			mode.h = glConfig.vidHeight;
			//mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue( "r_displayRefresh" );
	

			if( !SDL_SetWindowFullscreenMode( SDL_window, &mode ))
			{
				ri.Printf( PRINT_DEVELOPER, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError( ) );
				continue;
			}
		}

		SDL_SetWindowIcon( SDL_window, icon );

		if (!fixedFunction)
		{
			int profileMask, majorVersion, minorVersion;
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profileMask);
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
			SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

			ri.Printf(PRINT_ALL, "Trying to get an OpenGL 3.2 core context\n");
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
			if ((SDL_glContext = SDL_GL_CreateContext(SDL_window)) == nullptr)
			{
				ri.Printf(PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
				ri.Printf(PRINT_ALL, "Reverting to default context\n");

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
			}
			else
			{
				const char *renderer;

				ri.Printf(PRINT_ALL, "SDL_GL_CreateContext succeeded.\n");

			}
		}
		else
		{
			SDL_glContext = nullptr;
		}

		if ( !SDL_glContext )
		{
			if( ( SDL_glContext = SDL_GL_CreateContext( SDL_window ) ) == nullptr )
			{
				ri.Printf( PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError( ) );
				SDL_DestroyWindow( SDL_window );
				SDL_window = nullptr;
				continue;
			}

		}

		qglClearColor( 0, 0, 0, 1 );
		qglClear( GL_COLOR_BUFFER_BIT );
		SDL_GL_SwapWindow( SDL_window );

		if( !SDL_GL_SetSwapInterval( r_swapInterval->integer ) )
		{
			ri.Printf( PRINT_DEVELOPER, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError( ) );
		}

		SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &realColorBits[0] );
		SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &realColorBits[1] );
		SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &realColorBits[2] );
		SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &glConfig.depthBits );
		SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &glConfig.stencilBits );

		glConfig.colorBits = realColorBits[0] + realColorBits[1] + realColorBits[2];

		ri.Printf( PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
				glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
		break;
	}

	SDL_DestroySurface( icon );

	if( !SDL_window )
	{
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	glstring = (char *) qglGetString (GL_RENDERER);
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

	return RSERR_OK;
}

/*
===============
GLimp_StartDriverAndSetMode
===============
*/
static bool GLimp_StartDriverAndSetMode(int mode, bool fullscreen, bool noborder, bool gl3Core)
{
	if (SDL_WasInit(SDL_INIT_VIDEO) != SDL_INIT_VIDEO)
	{
		const char *driverName;
	
		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			ri.Printf( PRINT_ALL, "SDL_Init( SDL_INIT_VIDEO ) FAILED (%s)\n", SDL_GetError());
			return false;
		}
	}
	
	rserr_t err = (rserr_t)GLimp_SetMode(mode, fullscreen, noborder, gl3Core);

	switch ( err )
	{
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
			return false;
		case RSERR_INVALID_MODE:
			ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
			return false;
		default:
			break;
	}

	return true;
}


/*
===============
GLimp_InitExtensions
===============
*/
static void GLimp_InitExtensions( bool fixedFunction )
{
	if ( !r_allowExtensions->integer )
	{
		ri.Printf( PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	glConfig.textureCompression = TC_NONE;

	// OpenGL 1 fixed function pipeline
	if ( fixedFunction )
	{
		glConfig.textureEnvAddAvailable = false;
		if ( SDL_GL_ExtensionSupported( "GL_EXT_texture_env_add" ) )
		{
			if ( r_ext_texture_env_add->integer )
			{
				glConfig.textureEnvAddAvailable = true;
				ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
			}
			else
			{
				glConfig.textureEnvAddAvailable = false;
				ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
		}

		// GL_ARB_multitexture
		qglMultiTexCoord2fARB = nullptr;
		qglActiveTextureARB = nullptr;
		qglClientActiveTextureARB = nullptr;

		// GL_EXT_compiled_vertex_array
		if ( SDL_GL_ExtensionSupported( "GL_EXT_compiled_vertex_array" ) )
		{
			if ( r_ext_compiled_vertex_array->value )
			{
				ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
				qglLockArraysEXT = ( void ( APIENTRY * )( GLint, GLint ) ) SDL_GL_GetProcAddress( "glLockArraysEXT" );
				qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDL_GL_GetProcAddress( "glUnlockArraysEXT" );
				if (!qglLockArraysEXT || !qglUnlockArraysEXT)
				{
					ri.Error (ERR_FATAL, "bad getprocaddress");
				}
			}
			else
			{
				ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
		}
	}
}

#define R_MODE_FALLBACK 3 // 640 * 480

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( bool fixedFunction )
{
	ri.Printf( PRINT_DEVELOPER, "Glimp_Init( )\n" );

	r_allowSoftwareGL = Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_sdlDriver = Cvar_Get( "r_sdlDriver", "", CVAR_ROM );
	r_allowResize = Cvar_Get( "r_allowResize", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_centerWindow = Cvar_Get( "r_centerWindow", "0", CVAR_ARCHIVE | CVAR_LATCH );

	if( Cvar_VariableIntegerValue( "com_abnormalExit" ) )
	{
		Cvar_Set( "r_mode", va( "%d", R_MODE_FALLBACK ) );
		Cvar_Set( "r_fullscreen", "0" );
		Cvar_Set( "r_centerWindow", "0" );
		Cvar_Set( "com_abnormalExit", "0" );
	}

	// IJB Sys_GLimpInit( );

	// Create the window and set up the context
	if(!GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, false, fixedFunction))
	{
		/*
	
	// Try again, this time in a platform specific "safe mode"
	ri.Sys_GLimpSafeInit( );

	if(GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, false, fixedFunction))
		goto success;

	// Finally, try the default screen resolution
	if( r_mode->integer != R_MODE_FALLBACK )
	{
		ri.Printf( PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n",
				r_mode->integer, R_MODE_FALLBACK );

		if(GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, false, false, fixedFunction))
			goto success;
	}

	// Nothing worked, give up
	ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem" );
*/
}

	// These values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	// glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer && SDL_SetWindowBrightness( SDL_window, 1.0f ) >= 0;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n'){
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	}
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );

	// initialize extensions
	GLimp_InitExtensions( fixedFunction );

	ri.Cvar_Get( "r_availableModes", "", CVAR_ROM );

	// This depends on SDL_INIT_VIDEO, hence having it here
	IN_Init( SDL_window );
}


/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{
		SDL_GL_SwapWindow( SDL_window );
	}

	if( r_fullscreen->modified )
	{
		bool         fullscreen;
		bool    needToToggle;

		// Find out the current state
		fullscreen = !!( SDL_GetWindowFlags( SDL_window ) & SDL_WINDOW_FULLSCREEN );

		if( r_fullscreen->integer && Cvar_VariableIntegerValue( "in_nograb" ) )
		{
			ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
			ri.Cvar_Set( "r_fullscreen", "0" );
			r_fullscreen->modified = false;
		}

		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if( needToToggle )
		{
			// Need the vid_restart here since r_fullscreen is only latched
			if( fullscreen )
			{
				Com_Printf( "Switching to windowed rendering\n" );
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
			}
			else
			{
				Com_Printf( "Switching to fullscreen rendering\n" );
				Cbuf_ExecuteText(EXEC_APPEND, "vid_restart\n");
			}

			IN_Restart( );
		}

		r_fullscreen->modified = false;
	}
}
