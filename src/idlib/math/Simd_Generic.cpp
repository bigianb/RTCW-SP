/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <cassert>

#include "Simd_Generic.h"
#include "Math.h"
#include "Vector.h"

//===============================================================
//
//	Generic implementation of idSIMDProcessor
//
//===============================================================

#define UNROLL1(Y) { int _IX; for (_IX=0;_IX<count;_IX++) {Y(_IX);} }
#define UNROLL2(Y) { int _IX, _NM = count&0xfffffffe; for (_IX=0;_IX<_NM;_IX+=2){Y(_IX+0);Y(_IX+1);} if (_IX < count) {Y(_IX);}}
#define UNROLL4(Y) { int _IX, _NM = count&0xfffffffc; for (_IX=0;_IX<_NM;_IX+=4){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);}for(;_IX<count;_IX++){Y(_IX);}}
#define UNROLL8(Y) { int _IX, _NM = count&0xfffffff8; for (_IX=0;_IX<_NM;_IX+=8){Y(_IX+0);Y(_IX+1);Y(_IX+2);Y(_IX+3);Y(_IX+4);Y(_IX+5);Y(_IX+6);Y(_IX+7);} _NM = count&0xfffffffe; for(;_IX<_NM;_IX+=2){Y(_IX); Y(_IX+1);} if (_IX < count) {Y(_IX);} }

#undef NODEFAULT
#ifdef _DEBUG
	#define NODEFAULT	default: assert( 0 )
#else
	#define NODEFAULT	default: __assume( 0 )
#endif


/*
============
idSIMD_Generic::GetName
============
*/
const char* idSIMD_Generic::GetName() const
{
	return "generic code";
}

/*
============
idSIMD_Generic::MinMax
============
*/
void VPCALL idSIMD_Generic::MinMax( float& min, float& max, const float* src, const int count )
{
	min = idMath::INFINITUM;
	max = -idMath::INFINITUM;
#define OPER(X) if ( src[(X)] < min ) {min = src[(X)];} if ( src[(X)] > max ) {max = src[(X)];}
	UNROLL1( OPER )
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void VPCALL idSIMD_Generic::MinMax( idVec2& min, idVec2& max, const idVec2* src, const int count )
{
	min[0] = min[1] = idMath::INFINITUM;
	max[0] = max[1] = -idMath::INFINITUM;
#define OPER(X) const idVec2 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; }
	UNROLL1( OPER )
#undef OPER
}

/*
============
idSIMD_Generic::MinMax
============
*/
void VPCALL idSIMD_Generic::MinMax( idVec3& min, idVec3& max, const idVec3* src, const int count )
{
	min[0] = min[1] = min[2] = idMath::INFINITUM;
	max[0] = max[1] = max[2] = -idMath::INFINITUM;
#define OPER(X) const idVec3 &v = src[(X)]; if ( v[0] < min[0] ) { min[0] = v[0]; } if ( v[0] > max[0] ) { max[0] = v[0]; } if ( v[1] < min[1] ) { min[1] = v[1]; } if ( v[1] > max[1] ) { max[1] = v[1]; } if ( v[2] < min[2] ) { min[2] = v[2]; } if ( v[2] > max[2] ) { max[2] = v[2]; }
	UNROLL1( OPER )
#undef OPER
}

/*
================
idSIMD_Generic::Memcpy
================
*/
void VPCALL idSIMD_Generic::Memcpy( void* dst, const void* src, const int count )
{
	memcpy( dst, src, count );
}

/*
================
idSIMD_Generic::Memset
================
*/
void VPCALL idSIMD_Generic::Memset( void* dst, const int val, const int count )
{
	memset( dst, val, count );
}

