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

#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

#if defined( _WIN32 )
#pragma warning(disable : 4244)
#endif

#include <math.h>
#include <assert.h>

#define __VectorMA( v, s, b, o )  ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ) )

#define DotProduct4( x,y )        ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] + ( x )[3] * ( y )[3] )
#define VectorSubtract4( a,b,c )  ( ( c )[0] = ( a )[0] - ( b )[0],( c )[1] = ( a )[1] - ( b )[1],( c )[2] = ( a )[2] - ( b )[2],( c )[3] = ( a )[3] - ( b )[3] )
#define VectorAdd4( a,b,c )       ( ( c )[0] = ( a )[0] + ( b )[0],( c )[1] = ( a )[1] + ( b )[1],( c )[2] = ( a )[2] + ( b )[2],( c )[3] = ( a )[3] + ( b )[3] )
#define VectorCopy4( a,b )        ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2],( b )[3] = ( a )[3] )
#define VectorScale4( v, s, o )   ( ( o )[0] = ( v )[0] * ( s ),( o )[1] = ( v )[1] * ( s ),( o )[2] = ( v )[2] * ( s ),( o )[3] = ( v )[3] * ( s ) )
#define VectorMA4( v, s, b, o )   ( ( o )[0] = ( v )[0] + ( b )[0] * ( s ),( o )[1] = ( v )[1] + ( b )[1] * ( s ),( o )[2] = ( v )[2] + ( b )[2] * ( s ),( o )[3] = ( v )[3] + ( b )[3] * ( s ) )

#define VectorNegate( a,b )       ( ( b )[0] = -( a )[0],( b )[1] = -( a )[1],( b )[2] = -( a )[2] )
#define Vector4Copy( a,b )        ( ( b )[0] = ( a )[0],( b )[1] = ( a )[1],( b )[2] = ( a )[2],( b )[3] = ( a )[3] )

#ifndef SnapVector
#define SnapVector( v ) {v[0] = (int)v[0]; v[1] = (int)v[1]; v[2] = (int)v[2];}
#endif

#ifndef EQUAL_EPSILON
#define EQUAL_EPSILON   0.001
#endif

float Q_fabs( float f );

class angles_t;
static inline double idSqrt( double x ) {
	return sqrt( x );
}

class idVec3 {
public:
	float x,y,z;

	idVec3() {};
	idVec3( const float x, const float y, const float z );

	operator float*();

	float operator[]( const int index ) const;
	float           &operator[]( const int index );

	void            set( const float x, const float y, const float z );

	idVec3 operator-() const;

	idVec3          &operator=( const idVec3 &a );

	float operator*( const idVec3 &a ) const;
	idVec3 operator*( const float a ) const;
	friend idVec3 operator*( float a, idVec3 b );

	idVec3 operator+( const idVec3 &a ) const;
	idVec3 operator-( const idVec3 &a ) const;

	idVec3          &operator+=( const idVec3 &a );
	idVec3          &operator-=( const idVec3 &a );
	idVec3          &operator*=( const float a );

	int operator==( const idVec3 &a ) const;
	int operator!=( const idVec3 &a ) const;

	idVec3          Cross( const idVec3 &a ) const;
	idVec3          &Cross( const idVec3 &a, const idVec3 &b );

	float           Length( void ) const;
	float           Normalize( void );

	void            Zero( void );
	void            Snap( void );
	void            SnapTowards( const idVec3 &to );

	float           toYaw( void );
	float           toPitch( void );
	angles_t        toAngles( void );
	friend idVec3   LerpVector( const idVec3 &w1, const idVec3 &w2, const float t );

	char            *string( void );
};

extern idVec3 vec_zero;

inline idVec3::idVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

inline float idVec3::operator[]( const int index ) const {
	return ( &x )[ index ];
}

inline float &idVec3::operator[]( const int index ) {
	return ( &x )[ index ];
}

inline idVec3::operator float*( void ) {
	return &x;
}

inline idVec3 idVec3::operator-() const {
	return idVec3( -x, -y, -z );
}

inline idVec3 &idVec3::operator=( const idVec3 &a ) {
	x = a.x;
	y = a.y;
	z = a.z;

	return *this;
}

inline void idVec3::set( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

inline idVec3 idVec3::operator-( const idVec3 &a ) const {
	return idVec3( x - a.x, y - a.y, z - a.z );
}

inline float idVec3::operator*( const idVec3 &a ) const {
	return x * a.x + y * a.y + z * a.z;
}

inline idVec3 idVec3::operator*( const float a ) const {
	return idVec3( x * a, y * a, z * a );
}

inline idVec3 operator*( const float a, const idVec3 b ) {
	return idVec3( b.x * a, b.y * a, b.z * a );
}

inline idVec3 idVec3::operator+( const idVec3 &a ) const {
	return idVec3( x + a.x, y + a.y, z + a.z );
}

inline idVec3 &idVec3::operator+=( const idVec3 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;

	return *this;
}

inline idVec3 &idVec3::operator-=( const idVec3 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;

	return *this;
}

inline idVec3 &idVec3::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;

	return *this;
}

inline int idVec3::operator==( const idVec3 &a ) const {
	if ( Q_fabs( x - a.x ) > EQUAL_EPSILON ) {
		return false;
	}

	if ( Q_fabs( y - a.y ) > EQUAL_EPSILON ) {
		return false;
	}

	if ( Q_fabs( z - a.z ) > EQUAL_EPSILON ) {
		return false;
	}

	return true;
}

inline int idVec3::operator!=( const idVec3 &a ) const {
	if ( Q_fabs( x - a.x ) > EQUAL_EPSILON ) {
		return true;
	}

	if ( Q_fabs( y - a.y ) > EQUAL_EPSILON ) {
		return true;
	}

	if ( Q_fabs( z - a.z ) > EQUAL_EPSILON ) {
		return true;
	}

	return false;
}

inline idVec3 idVec3::Cross( const idVec3 &a ) const {
	return idVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

inline idVec3 &idVec3::Cross( const idVec3 &a, const idVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;

	return *this;
}

inline float idVec3::Length( void ) const {
	float length;

	length = x * x + y * y + z * z;
	return ( float )idSqrt( length );
}

inline float idVec3::Normalize( void ) {
	float length;
	float ilength;

	length = this->Length();
	if ( length ) {
		ilength = 1.0f / length;
		x *= ilength;
		y *= ilength;
		z *= ilength;
	}

	return length;
}

inline void idVec3::Zero( void ) {
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

inline void idVec3::Snap( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
	z = float( int( z ) );
}

/*
======================
SnapTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
inline void idVec3::SnapTowards( const idVec3 &to ) {
	if ( to.x <= x ) {
		x = float( int( x ) );
	} else {
		x = float( int( x ) + 1 );
	}

	if ( to.y <= y ) {
		y = float( int( y ) );
	} else {
		y = float( int( y ) + 1 );
	}

	if ( to.z <= z ) {
		z = float( int( z ) );
	} else {
		z = float( int( z ) + 1 );
	}
}

//===============================================================

class Bounds {
public:
	idVec3 b[2];

	Bounds();
	Bounds( const idVec3 &mins, const idVec3 &maxs );

	void    Clear();
	void    Zero();
	float   Radius();       // radius from origin, not from center
	idVec3  Center();
	void    AddPoint( const idVec3 &v );
	void    AddBounds( const Bounds &bb );
	bool    IsCleared();
	bool    ContainsPoint( const idVec3 &p );
	bool    IntersectsBounds( const Bounds &b2 );   // touching is NOT intersecting
};

extern Bounds boundsZero;

inline Bounds::Bounds() {
}

inline bool Bounds::IsCleared() {
	return b[0][0] > b[1][0];
}

inline bool Bounds::ContainsPoint( const idVec3 &p ) {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2]
		 || p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2] ) {
		return false;
	}
	return true;
}

inline bool Bounds::IntersectsBounds( const Bounds &b2 ) {
	if ( b2.b[1][0] < b[0][0] || b2.b[1][1] < b[0][1] || b2.b[1][2] < b[0][2]
		 || b2.b[0][0] > b[1][0] || b2.b[0][1] > b[1][1] || b2.b[0][2] > b[1][2] ) {
		return false;
	}
	return true;
}

inline Bounds::Bounds( const idVec3 &mins, const idVec3 &maxs ) {
	b[0] = mins;
	b[1] = maxs;
}

inline idVec3 Bounds::Center() {
	return idVec3( ( b[1][0] + b[0][0] ) * 0.5f, ( b[1][1] + b[0][1] ) * 0.5f, ( b[1][2] + b[0][2] ) * 0.5f );
}

inline void Bounds::Clear() {
	b[0][0] = b[0][1] = b[0][2] = 99999;
	b[1][0] = b[1][1] = b[1][2] = -99999;
}

inline void Bounds::Zero() {
	b[0][0] = b[0][1] = b[0][2] =
							b[1][0] = b[1][1] = b[1][2] = 0;
}

inline void Bounds::AddPoint( const idVec3 &v ) {
	if ( v[0] < b[0][0] ) {
		b[0][0] = v[0];
	}
	if ( v[0] > b[1][0] ) {
		b[1][0] = v[0];
	}
	if ( v[1] < b[0][1] ) {
		b[0][1] = v[1];
	}
	if ( v[1] > b[1][1] ) {
		b[1][1] = v[1];
	}
	if ( v[2] < b[0][2] ) {
		b[0][2] = v[2];
	}
	if ( v[2] > b[1][2] ) {
		b[1][2] = v[2];
	}
}


inline void Bounds::AddBounds( const Bounds &bb ) {
	if ( bb.b[0][0] < b[0][0] ) {
		b[0][0] = bb.b[0][0];
	}
	if ( bb.b[0][1] < b[0][1] ) {
		b[0][1] = bb.b[0][1];
	}
	if ( bb.b[0][2] < b[0][2] ) {
		b[0][2] = bb.b[0][2];
	}

	if ( bb.b[1][0] > b[1][0] ) {
		b[1][0] = bb.b[1][0];
	}
	if ( bb.b[1][1] > b[1][1] ) {
		b[1][1] = bb.b[1][1];
	}
	if ( bb.b[1][2] > b[1][2] ) {
		b[1][2] = bb.b[1][2];
	}
}

inline float Bounds::Radius() {
	int i;
	float total;
	float a, aa;

	total = 0;
	for ( i = 0 ; i < 3 ; i++ ) {
		a = (float)fabs( b[0][i] );
		aa = (float)fabs( b[1][i] );
		if ( aa > a ) {
			a = aa;
		}
		total += a * a;
	}

	return (float)idSqrt( total );
}

//===============================================================


class idVec2 {
public:
	float x;
	float y;

	operator float*();
	float operator[]( int index ) const;
	float           &operator[]( int index );
};

inline float idVec2::operator[]( int index ) const {
	return ( &x )[ index ];
}

inline float& idVec2::operator[]( int index ) {
	return ( &x )[ index ];
}

inline idVec2::operator float*( void ) {
	return &x;
}

class idVec4 : public idVec3 {
public:
	float dist;
	idVec4();
	~idVec4() {
	};

	idVec4( float x, float y, float z, float dist );
	float operator[]( int index ) const;
	float           &operator[]( int index );
};

inline idVec4::idVec4() {
}
inline idVec4::idVec4( float x, float y, float z, float dist ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->dist = dist;
}

inline float idVec4::operator[]( int index ) const {
	return ( &x )[ index ];
}

inline float& idVec4::operator[]( int index ) {
	return ( &x )[ index ];
}


class idVec5_t : public idVec3 {
public:
	float s;
	float t;
	float operator[]( int index ) const;
	float           &operator[]( int index );
};


inline float idVec5_t::operator[]( int index ) const {
	return ( &x )[ index ];
}

inline float& idVec5_t::operator[]( int index ) {
	return ( &x )[ index ];
}

#endif /* !__MATH_VECTOR_H__ */
