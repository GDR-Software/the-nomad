#include "../engine/n_shared.h"
#include "../engine/gln_files.h"
#include "../engine/n_common.h"

#if defined(__SSE2__) || defined(_MSC_SSE2_)
#define USING_SSE2
#ifdef _MSC
#include <intrin.h>
#else
#include <immintrin.h>
#include <xmmintrin.h>
#endif
#endif
#include <math.h>

const vec2_t vec2_origin = {0, 0};
const vec3_t vec3_origin = {0, 0};

const vec4_t		colorBlack	= {0, 0, 0, 1};
const vec4_t		colorRed	= {1, 0, 0, 1};
const vec4_t		colorGreen	= {0, 1, 0, 1};
const vec4_t		colorBlue	= {0, 0, 1, 1};
const vec4_t		colorYellow	= {1, 1, 0, 1};
const vec4_t		colorMagenta= {1, 0, 1, 1};
const vec4_t		colorCyan	= {0, 1, 1, 1};
const vec4_t		colorWhite	= {1, 1, 1, 1};
const vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
const vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
const vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

// actually there are 35 colors but we want to use bitmask safely
const vec4_t g_color_table[ 64 ] = {

	{0.0f, 0.0f, 0.0f, 1.0f},
	{1.0f, 0.0f, 0.0f, 1.0f},
	{0.0f, 1.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 0.0f, 1.0f},
	{0.2f, 0.2f, 1.0f, 1.0f}, //{0.0, 0.0, 1.0, 1.0},
	{0.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f},

	// extended color codes from CPMA/CNQ3:
	{ 1.00000f, 0.50000f, 0.00000f, 1.00000f },	// 8
	{ 0.60000f, 0.60000f, 1.00000f, 1.00000f },	// 9

	// CPMA's alphabet rainbow
	{ 1.00000f, 0.00000f, 0.00000f, 1.00000f },	// a
	{ 1.00000f, 0.26795f, 0.00000f, 1.00000f },	// b
	{ 1.00000f, 0.50000f, 0.00000f, 1.00000f },	// c
	{ 1.00000f, 0.73205f, 0.00000f, 1.00000f },	// d
	{ 1.00000f, 1.00000f, 0.00000f, 1.00000f },	// e
	{ 0.73205f, 1.00000f, 0.00000f, 1.00000f },	// f
	{ 0.50000f, 1.00000f, 0.00000f, 1.00000f },	// g
	{ 0.26795f, 1.00000f, 0.00000f, 1.00000f },	// h
	{ 0.00000f, 1.00000f, 0.00000f, 1.00000f },	// i
	{ 0.00000f, 1.00000f, 0.26795f, 1.00000f },	// j
	{ 0.00000f, 1.00000f, 0.50000f, 1.00000f },	// k
	{ 0.00000f, 1.00000f, 0.73205f, 1.00000f },	// l
	{ 0.00000f, 1.00000f, 1.00000f, 1.00000f },	// m
	{ 0.00000f, 0.73205f, 1.00000f, 1.00000f },	// n
	{ 0.00000f, 0.50000f, 1.00000f, 1.00000f },	// o
	{ 0.00000f, 0.26795f, 1.00000f, 1.00000f },	// p
	{ 0.00000f, 0.00000f, 1.00000f, 1.00000f },	// q
	{ 0.26795f, 0.00000f, 1.00000f, 1.00000f },	// r
	{ 0.50000f, 0.00000f, 1.00000f, 1.00000f },	// s
	{ 0.73205f, 0.00000f, 1.00000f, 1.00000f },	// t
	{ 1.00000f, 0.00000f, 1.00000f, 1.00000f },	// u
	{ 1.00000f, 0.00000f, 0.73205f, 1.00000f },	// v
	{ 1.00000f, 0.00000f, 0.50000f, 1.00000f },	// w
	{ 1.00000f, 0.00000f, 0.26795f, 1.00000f },	// x
	{ 1.0, 1.0, 1.0, 1.0 }, // y, white, duped so all colors can be expressed with this palette
	{ 0.5, 0.5, 0.5, 1.0 }, // z, grey
};


int ColorIndexFromChar( char ccode )
{
	if ( ccode >= '0' && ccode <= '9' ) {
		return ( ccode - '0' );
	}
	else if ( ccode >= 'a' && ccode <= 'z' ) {
		return ( ccode - 'a' + 10 );
	}
	else if ( ccode >= 'A' && ccode <= 'Z' ) {
		return ( ccode - 'A' + 10 );
	}
	else {
		return  ColorIndex( S_COLOR_WHITE );
	}
}

vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed char ClampCharMove( int i ) {
	if ( i < -127 ) {
		return -127;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}

// this isn't a real cheap function to call!
int DirToByte( vec3_t dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( dir, vec3_origin );
		return;
	}
	VectorCopy (dir,bytedirs[b]);
}


unsigned ColorBytes3 (float r, float g, float b) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max;
	
	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}

dirtype_t Angle2Dir( float angle ) {
	if ( ( angle >= 337.5f && angle <= 360.0f ) || ( angle >= 0.0f && angle <= 22.5f ) ) {
		return DIR_NORTH;
	} else if ( angle >= 22.5f && angle <= 67.5f ) {
		return DIR_NORTH_EAST;
	} else if ( angle >= 67.5f && angle <= 112.5f ) {
		return DIR_EAST;
	} else if ( angle >= 112.5f && angle <= 157.5f ) {
		return DIR_SOUTH_EAST;
	} else if ( angle >= 157.5f && angle <= 202.5f ) {
		return DIR_SOUTH;
	} else if ( angle >= 202.5f && angle <= 247.5f ) {
		return DIR_SOUTH_WEST;
	} else if ( angle >= 247.5f && angle <= 292.5f ) {
		return DIR_WEST;
	} else if ( angle >= 292.5f && angle <= 337.5f ) {
		return DIR_NORTH_WEST;
	} else {
		Con_Printf( COLOR_YELLOW "WARNING: Angle2Dir: funny angle %f\n", angle );
	}
	return DIR_NORTH;
}

float Dir2Angle( dirtype_t dir ) {
	switch ( dir ) {
	case DIR_NORTH:
		return 0.0f;
	case DIR_NORTH_EAST:
		return 45.0f;
	case DIR_EAST:
		return 90.0f;
	case DIR_SOUTH_EAST:
		return 135.0f;
	case DIR_SOUTH:
		return 180.0f;
	case DIR_SOUTH_WEST:
		return 225.0f;
	case DIR_WEST:
		return 270.0f;
	case DIR_NORTH_WEST:
		return 315.0f;
	default:
		Con_Printf( COLOR_YELLOW "WARNING: Dir2Angle: invalid dir %i", (int)dir );
	};
	return -1.0f;
}

dirtype_t DirFromPoint( const vec3_t v ) {
//	float angle;

//	angle = atan2( v[0], v[1] );
	return (dirtype_t)( (int)round( 8.0f * atan2( v[0], v[1] ) / ( 2.0f * M_PI ) + (int)NUMDIRS ) % (int)NUMDIRS );
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenerate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;
	float	rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD( degrees );
	zrot[0][0] = cos( rad );
	zrot[0][1] = sin( rad );
	zrot[1][0] = -sin( rad );
	zrot[1][1] = cos( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( temp, axis[1] );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( out[0], in[0] );
	VectorCopy( out[1], in[1] );
	VectorCopy( out[2], in[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
#ifndef Q3_VM
	assert( N_fabs(inv_denom) != 0.0f ); // zero vectors get here
#endif
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up ) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( const vec3_t in, const vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

float Q_rsqrt(float number)
{
#ifdef USING_SSE2
	// does this cpu actually support sse2?
	if ( !( CPU_flags & CPU_SSE2 ) ) {
		long x;
    	float x2, y;
		const float threehalfs = 1.5F;

    	x2 = number * 0.5F;
    	x = *(long *)&number;                    // evil floating point bit level hacking
    	x = 0x5f3759df - (x >> 1);               // what the fuck?
    	y = *(float *)&x;
    	y = y * ( threehalfs - ( x2 * y * y ) ); // 1st iteration
  	//	y = y * ( threehalfs - ( x2 * y * y ) ); // 2nd iteration, this can be removed

    	return y;
	}

	float ret;
	_mm_store_ss( &ret, _mm_rsqrt_ss( _mm_load_ss( &number ) ) );
	return ret;
#else
    long x;
    float x2, y;
	const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    x = *(long *)&number;                    // evil floating point bit level hacking
    x = 0x5f3759df - (x >> 1);               // what the fuck?
    y = *(float *)&x;
    y = y * ( threehalfs - ( x2 * y * y ) ); // 1st iteration
//  y = y * ( threehalfs - ( x2 * y * y ) ); // 2nd iteration, this can be removed

    return y;
#endif
}

#ifdef __cplusplus
float disBetweenOBJ( const glm::vec2& src, const glm::vec2& tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

unsigned disBetweenOBJ( const glm::uvec2& src, const glm::uvec2& tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

int disBetweenOBJ( const glm::ivec2& src, const glm::ivec2& tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

float disBetweenOBJ( const glm::vec3& src, const glm::vec3& tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

unsigned disBetweenOBJ( const glm::uvec3& src, const glm::uvec3& tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

int disBetweenOBJ( const glm::ivec3& src, const glm::ivec3& tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

unsigned disBetweenOBJ( const uvec3_t src, const uvec3_t tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

int disBetweenOBJ( const ivec3_t src, const ivec3_t tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}
#endif

float disBetweenOBJ( const vec3_t src, const vec3_t tar )
{
	if ( src[1] == tar[1] ) { // horizontal
		return src[0] > tar[0] ? ( src[0] - tar[0] ) : ( tar[0] - src[0] );
	} else if ( src[0] == tar[0] ) { // vertical
		return src[1] > tar[1] ? ( src[1] - tar[1] ) : ( tar[1] - src[1] );
	} else { // diagonal
		return sqrtf( ( pow( ( src[0] - tar[0] ), 2 ) + pow( ( src[1] - tar[1] ), 2 ) ) );
	}
}

#if defined(Q3_VM) && !defined(__Q3_VM_MATH)
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorLength(const vec3_t v) {
	return (vec_t)sqrtf (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}
#endif

//============================================================

/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist[2];
	int		sides, b, i;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (i=0 ; i<3 ; i++)
		{
			b = (p->signbits >> i) & 1;
			dist[ b] += p->normal[i]*emaxs[i];
			dist[!b] += p->normal[i]*emins[i];
		}
	}

	sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)
		sides |= 2;

	return sides;
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const bbox_t *bounds ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabs( bounds->mins[i] );
		b = fabs( bounds->maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}

#if defined( Q3_VM ) || defined( __Q3_VM_MATH )
int VectorCompare( const vec3_t a, const vec3_t b ) {
	if ( a[0] != b[0] || a[1] != b[1] || a[2] != b[2] ) {
		return 0;
	}
	return 1;
}
#endif

void ClearBounds( bbox_t *bounds ) {
	bounds->mins[0] = bounds->mins[1] = bounds->mins[2] = 99999;
	bounds->maxs[0] = bounds->maxs[1] = bounds->maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, bbox_t *bounds ) {
	if ( v[0] < bounds->mins[0] ) {
		bounds->mins[0] = v[0];
	}
	if ( v[0] > bounds->maxs[0]) {
		bounds->maxs[0] = v[0];
	}

	if ( v[1] < bounds->mins[1] ) {
		bounds->mins[1] = v[1];
	}
	if ( v[1] > bounds->maxs[1]) {
		bounds->maxs[1] = v[1];
	}

	if ( v[2] < bounds->mins[2] ) {
		bounds->mins[2] = v[2];
	}
	if ( v[2] > bounds->maxs[2]) {
		bounds->maxs[2] = v[2];
	}
}

qboolean BoundsIntersect( const bbox_t *a, const bbox_t *b )
{
	if ( a->maxs[0] < b->mins[0] ||
		a->maxs[1] < b->mins[1] ||
		a->maxs[2] < b->mins[2] ||
		a->mins[0] > b->maxs[0] ||
		a->mins[1] > b->maxs[1] ||
		a->mins[2] > b->maxs[2] )
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectSphere( const bbox_t *bounds,
		const vec3_t origin, vec_t radius )
{
	if ( origin[0] - radius > bounds->maxs[0] ||
		origin[0] + radius < bounds->mins[0] ||
		origin[1] - radius > bounds->maxs[1] ||
		origin[1] + radius < bounds->mins[1] ||
		origin[2] - radius > bounds->maxs[2] ||
		origin[2] + radius < bounds->mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectPoint( const bbox_t *bounds,
		const vec3_t origin )
{
	if ( origin[0] > bounds->maxs[0] ||
		origin[0] < bounds->mins[0] ||
		origin[1] > bounds->maxs[1] ||
		origin[1] < bounds->mins[1] ||
		origin[2] > bounds->maxs[2] ||
		origin[2] < bounds->mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

vec_t VectorNormalize( vec3_t v ) {
	// NOTE: TTimo - Apple G4 altivec source uses double?
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if ( length ) {
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out ) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (length)
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
		VectorClear( out );
	}
		
	return length;

}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}



/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}
*/


/*
================
MatrixMultiply
================
*/
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}


/*
================
Q_isnan

Don't pass doubles to this
================
*/
int N_isnan( float x )
{
	floatint_t fi;

	fi.f = x;
	fi.u &= 0x7FFFFFFF;
	fi.u = 0x7F800000 - fi.u;

	return (int)( fi.u >> 31 );
}
//------------------------------------------------------------------------


/*
================
Q_isfinite
================
*/
static int N_isfinite( float f )
{
	floatint_t fi;
	fi.f = f;

	if ( fi.u == 0xFF800000 || fi.u == 0x7F800000 )
		return 0; // -INF or +INF

	fi.u = 0x7F800000 - (fi.u & 0x7FFFFFFF);
	if ( (int)( fi.u >> 31 ) )
		return 0; // -NAN or +NAN

	return 1;
}


/*
================
Q_atof
================
*/
float N_atof( const char *str )
{
	float f;

	f = atof( str );

	// modern C11-like implementations of atof() may return INF or NAN
	// which breaks all FP code where such values getting passed
	// and effectively corrupts range checks for cvars as well
	if ( !N_isfinite( f ) )
		return 0.0f;

	return f;
}


float N_fabs( float f ) {
	floatint_t fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}


/*
================
Q_log2f
================
*/
float N_log2f( float f )
{
	const float v = logf( f );
	return v / M_LN2;
}


/*
================
Q_exp2f
================
*/
float N_exp2f( float f )
{
	return powf( 2.0f, f );
}

/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

=====================
*/
float N_acos(float c) {
	float angle;

	angle = acos(c);

	if (angle > M_PI) {
		return (float)M_PI;
	}
	if (angle < -M_PI) {
		return (float)M_PI;
	}
	return angle;
}
