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

//#include "../game/q_shared.h"
#include "math_vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define M_PI        3.14159265358979323846264338327950288

#define LERP_DELTA 1e-6

idVec3 vec_zero( 0.0f, 0.0f, 0.0f );

Bounds boundsZero;

float idVec3::toYaw( void ) {
	float yaw;

	if ( ( y == 0 ) && ( x == 0 ) ) {
		yaw = 0;
	} else {
		yaw = atan2( y, x ) * 180 / M_PI;
		if ( yaw < 0 ) {
			yaw += 360;
		}
	}

	return yaw;
}

float idVec3::toPitch( void ) {
	float forward;
	float pitch;

	if ( ( x == 0 ) && ( y == 0 ) ) {
		if ( z > 0 ) {
			pitch = 90;
		} else {
			pitch = 270;
		}
	} else {
		forward = ( float )idSqrt( x * x + y * y );
		pitch = atan2( z, forward ) * 180 / M_PI;
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	return pitch;
}

/*
angles_t idVec3::toAngles( void ) {
	float forward;
	float yaw;
	float pitch;

	if ( ( x == 0 ) && ( y == 0 ) ) {
		yaw = 0;
		if ( z > 0 ) {
			pitch = 90;
		} else {
			pitch = 270;
		}
	} else {
		yaw = atan2( y, x ) * 180 / M_PI;
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = ( float )idSqrt( x * x + y * y );
		pitch = atan2( z, forward ) * 180 / M_PI;
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	return angles_t( -pitch, yaw, 0 );
}
*/

idVec3 LerpVector( idVec3 &w1, idVec3 &w2, const float t ) {
	float omega, cosom, sinom, scale0, scale1;

	cosom = w1 * w2;
	if ( ( 1.0 - cosom ) > LERP_DELTA ) {
		omega = acos( cosom );
		sinom = sin( omega );
		scale0 = sin( ( 1.0 - t ) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
	} else {
		scale0 = 1.0 - t;
		scale1 = t;
	}

	return ( w1 * scale0 + w2 * scale1 );
}

/*
=============
idVec3::string

This is just a convenience function
for printing vectors
=============
*/
char *idVec3::string( void ) {
	static int index = 0;
	static char str[ 8 ][ 36 ];
	char    *s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = ( index + 1 ) & 7;

	snprintf( s, 36, "%.2f %.2f %.2f", x, y, z );

	return s;
}
