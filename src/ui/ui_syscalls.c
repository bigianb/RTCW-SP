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


void trap_UI_Print( const char *string ) {
	Com_Printf( "%s", string);
}

void trap_UI_Error( const char *string ) {
	Com_Error( ERR_DROP, "%s", string );
}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	Cvar_SetValue(var_name, value );
}

void trap_Cvar_Reset( const char *name ) {
	Cvar_Reset(name );
}

void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	Cvar_Get(var_name, var_value, flags );
}

void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	Cvar_InfoStringBuffer(bit, buffer, bufsize );
}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	Cbuf_ExecuteText( exec_when, text );
}

void trap_FS_Seek( fileHandle_t f, long offset, int origin  ) {
	FS_Seek(f, offset, origin );
}

int trap_FS_Delete( const char *filename ) {
	return FS_Delete(filename );
}

qhandle_t trap_UI_RegisterModel( const char *name ) {
	return RE_RegisterModel(name );
}

qhandle_t trap_UI_RegisterSkin( const char *name ) {
	return RE_RegisterSkin(name );
}

void trap_UI_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	RE_RegisterFont(fontName, pointSize, font );
}

qhandle_t trap_UI_RegisterShaderNoMip( const char *name ) {
	return RE_RegisterShaderNoMip( name );
}

void trap_UI_ClearScene( void ) {
	RE_ClearScene();
}

void trap_UI_AddRefEntityToScene( const refEntity_t *re ) {
	RE_AddRefEntityToScene(re );
}

void trap_UI_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	RE_AddPolyToScene( hShader, numVerts, verts );
}

void trap_UI_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags ) {
	RE_AddCoronaToScene(org, r, g, b, scale, id, flags );
}

void trap_UI_RenderScene( const refdef_t *fd ) {
	RE_RenderScene(fd);
}

void trap_UI_SetColor( const float *rgba ) {
	RE_SetColor(rgba );
}

void trap_UI_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	RE_StretchPic(x, y, w, h, s1, t1, s2, t1, hShader );
}

void    trap_UI_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	R_ModelBounds(model, mins, maxs );
}

int trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex ) {
	return R_LerpTag(tag, refent, tagName, 0 );           // NEFVE - SMF - fixed
}

sfxHandle_t trap_UI_S_RegisterSound( const char *sample ) {
	return S_RegisterSound( sample, qfalse );
}

extern void Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	Key_KeynumToStringBuf(keynum, buf, buflen );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	Key_GetBindingBuf( keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	Key_SetBinding( keynum, binding );
}

qboolean trap_Key_GetOverstrikeMode( void ) {
	return Key_GetOverstrikeMode();
}

void trap_Key_SetOverstrikeMode( qboolean state ) {
	Key_SetOverstrikeMode(state );
}

void trap_Key_ClearStates( void ) {
	Key_ClearStates();
}

extern void GetClipboardData( char *buf, int buflen );
void trap_GetClipboardData( char *buf, int bufsize ) {
	GetClipboardData( buf, bufsize );
}

void trap_GetClientState( uiClientState_t *state ) {
	GetClientState( state );
}

int trap_GetConfigString( int index, char* buff, int buffsize ) {
	return GetConfigString(index, buff, buffsize );
}

extern int LAN_GetServerCount( int source );
int trap_LAN_GetServerCount( int source ) {
	return LAN_GetServerCount(source );
}

extern int LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return LAN_CompareServers(source, sortKey, sortDir, s1, s2 );
}

extern void LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	LAN_GetServerAddressString(source, n, buf, buflen );
}

extern void LAN_GetServerInfo( int source, int n, char *buf, int buflen );
void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	LAN_GetServerInfo(source, n, buf, buflen );
}

extern int LAN_AddServer( int source, const char *name, const char *address );
int trap_LAN_AddServer( int source, const char *name, const char *addr ) {
	return LAN_AddServer(source, name, addr );
}

extern void LAN_RemoveServer( int source, const char *addr );
void trap_LAN_RemoveServer( int source, const char *addr ) {
	LAN_RemoveServer(source, addr );
}

extern int LAN_GetServerPing( int source, int n );
int trap_LAN_GetServerPing( int source, int n ) {
	return LAN_GetServerPing(source, n );
}

extern int LAN_ServerIsVisible( int source, int n );
int trap_LAN_ServerIsVisible( int source, int n ) {
	return LAN_ServerIsVisible(source, n );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return CL_ServerStatus(serverAddress, serverStatus, maxLen );
}

void trap_LAN_SaveCachedServers() {
	LAN_SaveServersToCache();
}

void trap_LAN_LoadCachedServers() {
	LAN_LoadCachedServers();
}

extern void LAN_MarkServerVisible( int source, int n, qboolean visible );
void trap_LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	LAN_MarkServerVisible(source, n, visible );
}

extern void LAN_ResetPings( int source );
void trap_LAN_ResetPings( int n ) {
	LAN_ResetPings( n );
}

// allows you to resize the animation dynamically
void trap_UI_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	CIN_SetExtents( handle, x, y, w, h );
}

