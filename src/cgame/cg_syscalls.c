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

// cg_syscalls.c -- this file is only included when building a dll
// cg_syscalls.asm is included instead when building a qvm
#include "cg_local.h"
#include "client.h"
#include "botlib/l_script.h"
#include "botlib/l_precomp.h"
#include "../renderer/tr_local.h"
#include "../game/g_func_decs.h"

static int QDECL dummySyscall(int arg, ...){
	return 0;
}

static int ( QDECL * syscall )( int arg, ... ) = dummySyscall;


void dllEntry_CG( int ( QDECL  *syscallptr )( int arg,... ) ) {
	syscall = syscallptr;
}

static
int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void    trap_Print( const char *fmt ) {
	Com_Printf( "%s", fmt );
}
/*
void    trap_Error( const char *fmt ) {
	syscall( CG_ERROR, fmt );
}

int     trap_Milliseconds( void ) {
	return syscall( CG_MILLISECONDS );
}

void    trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	syscall( CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags );
}
*/
void    trap_Cvar_Update( vmCvar_t *vmCvar ) {
	Cvar_Update(vmCvar );
}
/*
void    trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( CG_CVAR_SET, var_name, value );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( CG_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

int     trap_Argc( void ) {
	return syscall( CG_ARGC );
}

void    trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( CG_ARGV, n, buffer, bufferLength );
}
*/
void    trap_Args( char *buffer, int bufferLength ) {
	Cmd_ArgsBuffer(buffer, bufferLength );
}
/*
int     trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( CG_FS_FOPENFILE, qpath, f, mode );
}

void    trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( CG_FS_READ, buffer, len, f );
}

void    trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( CG_FS_WRITE, buffer, len, f );
}

void    trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( CG_FS_FCLOSEFILE, f );
}
*/
void    trap_SendConsoleCommand( const char *text ) {
	Cbuf_AddText( text );
}

void    trap_AddCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}

void    trap_SendClientCommand( const char *s ) {
	CL_AddReliableCommand( s );
}

void    trap_UpdateScreen( void ) {
	SCR_UpdateScreen();
}

void    trap_CM_LoadMap( const char *mapname ) {
	int checksum;

	CM_LoadMap( mapname, qtrue, &checksum );
}

int     trap_CM_NumInlineModels( void ) {
	return CM_NumInlineModels();
}

clipHandle_t trap_CM_InlineModel( int index ) {
	return CM_InlineModel(index );
}

clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {
	return CM_TempBoxModel( mins, maxs, qfalse );
}

clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs ) {
	return CM_TempBoxModel( mins, maxs, qtrue );
}

int     trap_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return CM_PointContents( p, model );
}

int     trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return CM_TransformedPointContents( p, model, origin, angles );
}

void    trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask ) {
	CM_BoxTrace(results, start, end, mins, maxs, model, brushmask, qfalse );
}

void    trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
									 const vec3_t mins, const vec3_t maxs,
									 clipHandle_t model, int brushmask,
									 const vec3_t origin, const vec3_t angles ) {
	CM_TransformedBoxTrace(results, start, end, mins, maxs, model, brushmask, origin, angles, qfalse );
}

void    trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask ) {
	CM_BoxTrace(results, start, end, mins, maxs, model, brushmask, qtrue );
}

void    trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
										 const vec3_t mins, const vec3_t maxs,
										 clipHandle_t model, int brushmask,
										 const vec3_t origin, const vec3_t angles ) {
	CM_TransformedBoxTrace(results, start, end, mins, maxs, model, brushmask, origin, angles, qtrue );
}

int     trap_CM_MarkFragments( int numPoints, const vec3_t *points,
							   const vec3_t projection,
							   int maxPoints, vec3_t pointBuffer,
							   int maxFragments, markFragment_t *fragmentBuffer ) {
	return R_MarkFragments(numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

void    trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	S_StartSound(origin, entityNum, entchannel, sfx );
}

void    trap_S_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags ) {
	S_StartSoundEx(origin, entityNum, entchannel, sfx, flags );
}

void    trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	S_StartLocalSound(sfx, channelNum );
}

void    trap_S_ClearLoopingSounds( int killall ) {
	S_ClearLoopingSounds(); 

	if ( killall == 1 ) {
		//qboolean clearStreaming, qboolean clearMusic
		S_ClearSounds( qtrue, qfalse );
	} else if ( killall == 2 ) {
		S_ClearSounds( qtrue, qtrue );
	}
}

void    trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume ) {
	S_AddLoopingSound(entityNum, origin, velocity, 1250, sfx, volume );     // volume was previously removed from CG_S_ADDLOOPINGSOUND.  I added 'range'
}

void    trap_S_AddRangedLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range ) {
	S_AddLoopingSound(entityNum, origin, velocity, range, sfx, 255 );   // RF, assume full volume, since thats how it worked before
}

void    trap_S_StopStreamingSound( int entityNum ) {
	S_StopEntStreamingSound(entityNum );
}


void    trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	S_UpdateEntityPosition(entityNum, origin );
}

// Ridah, talking animations
int     trap_S_GetVoiceAmplitude( int entityNum ) {
	return S_GetVoiceAmplitude(entityNum );
}
// done.

void    trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater ) {
	S_Respatialize(entityNum, origin, axis, inwater );
}

sfxHandle_t trap_S_RegisterSound( const char *sample ) {
	CG_DrawInformation();
	return S_RegisterSound(sample, qfalse );
}

void    trap_S_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime ) {
	S_StartBackgroundTrack(intro, loop, fadeupTime );
}

void    trap_S_FadeBackgroundTrack( float targetvol, int time, int num ) {   // yes, i know.  fadebackground coming in, fadestreaming going out.  will have to see where functionality leads...
	S_FadeStreamingSound(targetvol, time, num ); // 'num' is '0' if it's music, '1' if it's "all streaming sounds"
}

void    trap_S_FadeAllSound( float targetvol, int time ) {
	S_FadeAllSounds(targetvol, time );
}

void    trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation ) {
	S_StartStreamingSound( intro, loop, entnum, channel, attenuation );
}

void    trap_R_LoadWorldMap( const char *mapname ) {
	RE_LoadWorldMap( mapname );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	CG_DrawInformation();
	return RE_RegisterModel( name );
}

qboolean trap_R_GetSkinModel( qhandle_t skinid, const char *type, char *name ) {
	return RE_GetSkinModel( skinid, type, name );
}

qhandle_t trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap ) {
	return RE_GetShaderFromModel(modelid, surfnum, withlightmap );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	CG_DrawInformation();
	return RE_RegisterSkin( name );
}

qhandle_t trap_R_RegisterShader( const char *name ) {
	CG_DrawInformation();
	return RE_RegisterShader( name );
}

void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	RE_RegisterFont(fontName, pointSize, font );
}

void    trap_R_ClearScene( void ) {
	RE_ClearScene();
}

void    trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	RE_AddRefEntityToScene(re );
}

void    trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	syscall( CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

void    trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	syscall( CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys );
}

void    trap_RB_ZombieFXAddNewHit( int entityNum, const vec3_t hitPos, const vec3_t hitDir ) {
	RB_ZombieFXAddNewHit(entityNum, hitPos, hitDir );
}

void    trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw ) {
	syscall( CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), overdraw );
}

//----(SA)
void    trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags ) {
	syscall( CG_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, flags );
}
//----(SA)

//----(SA)
void    trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density ) {
	syscall( CG_R_SETFOG, fogvar, var1, var2, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( density ) );
}
//----(SA)
void    trap_R_RenderScene( const refdef_t *fd ) {
	RE_RenderScene(fd );
}

void    trap_R_SetColor( const float *rgba ) {
	RE_SetColor(rgba );
}

void    trap_R_DrawStretchPic( float x, float y, float w, float h,
							   float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	RE_StretchPic( x , y , w , h , s1 , t1 ,  s2 , t2 , hShader );
}

void    trap_R_DrawStretchPicGradient(  float x, float y, float w, float h,
										float s1, float t1, float s2, float t2, qhandle_t hShader,
										const float *gradientColor, int gradientType ) {
	RE_StretchPicGradient(x, y, w, h, s1, t1, s2, t2, hShader, gradientColor, gradientType  );
}

void    trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	R_ModelBounds(model, mins, maxs );
}

int     trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex ) {
	return R_LerpTag( tag, refent, tagName, startIndex );
}

void    trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	R_RemapShader(oldShader, newShader, timeOffset );
}

extern void CL_GetGameState( gameState_t *gs );
void        trap_GetGameState( gameState_t *gamestate ) {
	CL_GetGameState( gamestate );
}

extern void    CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );
void        trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	CL_GetCurrentSnapshotNumber( snapshotNumber, serverTime );
}

extern qboolean    CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
qboolean    trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return CL_GetSnapshot( snapshotNumber, snapshot );
}

extern qboolean CL_GetServerCommand( int serverCommandNumber );
qboolean    trap_GetServerCommand( int serverCommandNumber ) {
	return CL_GetServerCommand( serverCommandNumber );
}

extern int CL_GetCurrentCmdNumber( void );
int         trap_GetCurrentCmdNumber( void ) {
	return CL_GetCurrentCmdNumber();
}

extern qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd );
qboolean    trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return CL_GetUserCmd(cmdNumber, ucmd );
}

extern void CL_SetUserCmdValue( int userCmdValue, int holdableValue, float sensitivityScale, int cld );
void        trap_SetUserCmdValue( int stateValue, int holdableValue, float sensitivityScale, int cld ) {    //----(SA)	// NERVE - SMF - added cld
	CL_SetUserCmdValue(stateValue, holdableValue, sensitivityScale, cld );
}

int trap_MemoryRemaining( void ) {
	return syscall( CG_MEMORY_REMAINING );
}

qboolean trap_loadCamera( int camNum, const char *name ) {
	return syscall( CG_LOADCAMERA, camNum, name );
}

void trap_startCamera( int camNum, int time ) {
	syscall( CG_STARTCAMERA, camNum, time );
}

void trap_stopCamera( int camNum ) {
	if ( camNum == 0 ) {  // CAM_PRIMARY
		cl.cameraMode = qfalse;
	}
}

extern qboolean getCameraInfo( int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov );

qboolean trap_getCameraInfo( int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov ) {
	return getCameraInfo(camNum, time, origin, angles, fov );
}


qboolean trap_Key_IsDown( int keynum ) {
	return Key_IsDown( keynum );
}

int trap_Key_GetCatcher( void ) {
	return Key_GetCatcher();
}

void trap_Key_SetCatcher( int catcher ) {
	Key_SetCatcher( catcher );
}

int trap_Key_GetKey( const char *binding ) {
	return Key_GetKey(binding );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return PC_AddGlobalDefine(define );
}

int trap_PC_LoadSource( const char *filename ) {
	return PC_LoadSourceHandle(filename);
}

int trap_PC_FreeSource( int handle ) {
	return PC_FreeSourceHandle( handle );
}

void  trap_S_StopBackgroundTrack( void ) {
	S_StopBackgroundTrack();
}

void trap_SendMoveSpeedsToGame( int entnum, char *movespeeds ) {
	G_RetrieveMoveSpeedsFromClient(entnum, movespeeds );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return CIN_PlayCinematic(arg0, xpos, ypos, width, height, bits );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic( int handle ) {
	return CIN_StopCinematic(handle );
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic( int handle ) {
	return CIN_RunCinematic( handle );
}


// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	CIN_DrawCinematic( handle );
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	CIN_SetExtents( handle, x, y, w, h );
}


// bring up a popup menu
extern void Menus_OpenByName( const char *p );

extern void IngamePopup(const char *arg0);
void trap_UI_Popup( const char *arg0 ) {
	IngamePopup(arg0 );
}

// NERVE - SMF
void trap_UI_LimboChat( const char *arg0 ) {
	if (arg0){
		CL_AddToLimboChat(arg0 );
	}
}

void trap_UI_ClosePopup( const char *arg0 ) {
	UI_KeyEvent(K_ESCAPE, qtrue );
}
// -NERVE - SMF

qboolean trap_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo ) {
	return G_GetModelInfo(clientNum, modelName, modelInfo );
}
