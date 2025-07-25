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
#include "qcommon.h"
#include "client.h"
#include "renderer/tr_local.h"

static int QDECL dummySyscall(int arg, ...){
	return 0;
}

static int ( QDECL * syscall )( int arg, ... ) = dummySyscall;

void dllEntry_UI( int ( QDECL *syscallptr )( int arg,... ) ) {
	syscall = syscallptr;
}

static int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void trap_UI_Print( const char *string ) {
	Com_Printf( "%s", string);
}

void trap_UI_Error( const char *string ) {
	Com_Error( ERR_DROP, "%s", string );
}

float trap_Cvar_VariableValue( const char *var_name ) {
	return Cvar_VariableValue(var_name );
}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	syscall( UI_CVAR_SETVALUE, var_name, PASSFLOAT( value ) );
}

void trap_Cvar_Reset( const char *name ) {
	syscall( UI_CVAR_RESET, name );
}

void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	syscall( UI_CVAR_CREATE, var_name, var_value, flags );
}

void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	syscall( UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize );
}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	Cbuf_ExecuteText( exec_when, text );
}

void trap_FS_Seek( fileHandle_t f, long offset, int origin  ) {
	syscall( UI_FS_SEEK, f, offset, origin );
}

int trap_FS_Delete( const char *filename ) {
	return syscall( UI_FS_DELETEFILE, filename );
}

qhandle_t trap_UI_RegisterModel( const char *name ) {
	return RE_RegisterModel(name );
}

qhandle_t trap_UI_RegisterSkin( const char *name ) {
	return RE_RegisterSkin(name );
}

void trap_UI_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	syscall( UI_R_REGISTERFONT, fontName, pointSize, font );
}

qhandle_t trap_UI_RegisterShaderNoMip( const char *name ) {
	return syscall( UI_R_REGISTERSHADERNOMIP, name );
}

void trap_UI_ClearScene( void ) {
	syscall( UI_R_CLEARSCENE );
}

void trap_UI_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( UI_R_ADDREFENTITYTOSCENE, re );
}

void trap_UI_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	syscall( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

void trap_UI_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw ) {
	syscall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), overdraw );
}

void trap_UI_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags ) {
	syscall( UI_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, flags );
}

void trap_UI_RenderScene( const refdef_t *fd ) {
	syscall( UI_R_RENDERSCENE, fd );
}

void trap_UI_SetColor( const float *rgba ) {
	RE_SetColor(rgba );
}

void trap_UI_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( UI_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

void    trap_UI_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( UI_R_MODELBOUNDS, model, mins, maxs );
}
/*
void trap_UpdateScreen( void ) {
	syscall( UI_UPDATESCREEN );
}
*/
int trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex ) {
	return syscall( UI_CM_LERPTAG, tag, refent, tagName, 0 );           // NEFVE - SMF - fixed
}
/*
void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall( UI_S_STARTLOCALSOUND, sfx, channelNum );
}
*/
sfxHandle_t trap_UI_S_RegisterSound( const char *sample ) {
	return S_RegisterSound( sample, qfalse );
}

/*
void    trap_S_FadeBackgroundTrack( float targetvol, int time, int num ) {   // yes, i know.  fadebackground coming in, fadestreaming going out.  will have to see where functionality leads...
	syscall( UI_S_FADESTREAMINGSOUND, PASSFLOAT( targetvol ), time, num ); // 'num' is '0' if it's music, '1' if it's "all streaming sounds"
}

void    trap_S_FadeAllSound( float targetvol, int time ) {
	syscall( UI_S_FADEALLSOUNDS, PASSFLOAT( targetvol ), time );
}
*/


void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	Key_GetBindingBuf( keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	Key_SetBinding( keynum, binding );
}
/*
qboolean trap_Key_IsDown( int keynum ) {
	return syscall( UI_KEY_ISDOWN, keynum );
}
*/
qboolean trap_Key_GetOverstrikeMode( void ) {
	return syscall( UI_KEY_GETOVERSTRIKEMODE );
}

void trap_Key_SetOverstrikeMode( qboolean state ) {
	syscall( UI_KEY_SETOVERSTRIKEMODE, state );
}

void trap_Key_ClearStates( void ) {
	Key_ClearStates();
}

void trap_GetClipboardData( char *buf, int bufsize ) {
	syscall( UI_GETCLIPBOARDDATA, buf, bufsize );
}

void trap_GetClientState( uiClientState_t *state ) {
	GetClientState( state );
}

int trap_GetConfigString( int index, char* buff, int buffsize ) {
	return GetConfigString(index, buff, buffsize );
}

int trap_LAN_GetLocalServerCount( void ) {
	return syscall( UI_LAN_GETLOCALSERVERCOUNT );
}

void trap_LAN_GetLocalServerAddressString( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETLOCALSERVERADDRESSSTRING, n, buf, buflen );
}

int trap_LAN_GetGlobalServerCount( void ) {
	return syscall( UI_LAN_GETGLOBALSERVERCOUNT );
}

void trap_LAN_GetGlobalServerAddressString( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETGLOBALSERVERADDRESSSTRING, n, buf, buflen );
}

int trap_LAN_GetPingQueueCount( void ) {
	return syscall( UI_LAN_GETPINGQUEUECOUNT );
}

void trap_LAN_ClearPing( int n ) {
	syscall( UI_LAN_CLEARPING, n );
}

void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	syscall( UI_LAN_GETPING, n, buf, buflen, pingtime );
}

void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETPINGINFO, n, buf, buflen );
}

// NERVE - SMF
qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return syscall( UI_LAN_UPDATEVISIBLEPINGS, source );
}

int trap_LAN_GetServerCount( int source ) {
	return syscall( UI_LAN_GETSERVERCOUNT, source );
}

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return syscall( UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}

void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETSERVERINFO, source, n, buf, buflen );
}

int trap_LAN_AddServer( int source, const char *name, const char *addr ) {
	return syscall( UI_LAN_ADDSERVER, source, name, addr );
}

void trap_LAN_RemoveServer( int source, const char *addr ) {
	syscall( UI_LAN_REMOVESERVER, source, addr );
}

int trap_LAN_GetServerPing( int source, int n ) {
	return syscall( UI_LAN_GETSERVERPING, source, n );
}

int trap_LAN_ServerIsVisible( int source, int n ) {
	return syscall( UI_LAN_SERVERISVISIBLE, source, n );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return syscall( UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}

void trap_LAN_SaveCachedServers() {
	//syscall( UI_LAN_SAVECACHEDSERVERS );
}

void trap_LAN_LoadCachedServers() {
	//syscall( UI_LAN_LOADCACHEDSERVERS );
}

void trap_LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	//syscall( UI_LAN_MARKSERVERVISIBLE, source, n, visible );
}

void trap_LAN_ResetPings( int n ) {
	//syscall( UI_LAN_RESETPINGS, n );
}

void trap_GetCDKey( char *buf, int buflen ) {
	syscall( UI_GET_CDKEY, buf, buflen );
}

void trap_SetCDKey( char *buf ) {
	syscall( UI_SET_CDKEY, buf );
}
// allows you to resize the animation dynamically
void trap_UI_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	CIN_SetExtents( handle, x, y, w, h );
}

qboolean trap_VerifyCDKey( const char *key, const char *chksum ) {
	return syscall( UI_VERIFY_CDKEY, key, chksum );
}

// NERVE - SMF
qboolean trap_GetLimboString( int index, char *buf ) {
	return syscall( UI_CL_GETLIMBOSTRING, index, buf );
}
// -NERVE - SMF
