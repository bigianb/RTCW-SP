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

#include "cg_local.h"
#include "client.h"
#include "botlib/l_script.h"
#include "botlib/l_precomp.h"
#include "../game/g_func_decs.h"
#include "../splines/splines_camera.h"
#include "../src/ui/ui_shared.h"

qhandle_t RegisterModelAndDrawInfo( const char *name ) {
	CG_DrawInformation();
	return RE_RegisterModel( name );
}

qhandle_t RegisterSkinAndDrawInfo( const char *name ) {
	CG_DrawInformation();
	return RE_RegisterSkin( name );
}

qhandle_t RegisterShaderAndDrawInfo( const char *name ) {
	CG_DrawInformation();
	return RE_RegisterShader( name );
}

extern void startCamera( int camNum, int time );
void trap_startCamera( int camNum, int time ) {
	if (camNum  == 0 ) {
		cl.cameraMode = true;
	}
	startCamera( camNum, time );
}

void trap_stopCamera( int camNum ) {
	if ( camNum == 0 ) {
		cl.cameraMode = false;
	}
}
