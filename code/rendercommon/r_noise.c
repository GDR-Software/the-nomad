#include "../engine/n_shared.h"
#include "../rendercommon/r_public.h"

#define NOISE_SIZE 256
#define NOISE_MASK ( NOISE_SIZE - 1 )

#define VAL( a ) s_noise_perm[ ( a ) & ( NOISE_MASK )]
#define INDEX( x, y, z, t ) VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )

static float s_noise_table[NOISE_SIZE];
static int32_t s_noise_perm[NOISE_SIZE];

static float GetNoiseValue( int32_t x, int32_t y, int32_t z, int32_t t )
{
	int32_t index = INDEX( ( int32_t ) x, ( int32_t ) y, ( int32_t ) z, ( int32_t ) t );

	return s_noise_table[index];
}

void R_NoiseInit( void )
{
	uint32_t i;

	for ( i = 0; i < NOISE_SIZE; i++ ) {
		s_noise_table[i] = ( float ) ( ( ( rand() / ( float ) RAND_MAX ) * 2.0 - 1.0 ) );
		s_noise_perm[i] = ( unsigned char ) ( rand() / ( float ) RAND_MAX * 255 );
	}
}

float R_NoiseGet4f( float x, float y, float z, double t )
{
	int32_t i;
	int32_t ix, iy, iz, it;
	float fx, fy, fz, ft;
	float front[4];
	float back[4];
	float fvalue, bvalue, value[2], finalvalue;

	ix = ( int32_t ) floor( x );
	fx = x - ix;
	iy = ( int32_t ) floor( y );
	fy = y - iy;
	iz = ( int32_t ) floor( z );
	fz = z - iz;
	it = ( int32_t ) floor( t );
	ft = t - it;

	for ( i = 0; i < 2; i++ ) {
		front[0] = GetNoiseValue( ix, iy, iz, it + i );
		front[1] = GetNoiseValue( ix+1, iy, iz, it + i );
		front[2] = GetNoiseValue( ix, iy+1, iz, it + i );
		front[3] = GetNoiseValue( ix+1, iy+1, iz, it + i );

		back[0] = GetNoiseValue( ix, iy, iz + 1, it + i );
		back[1] = GetNoiseValue( ix+1, iy, iz + 1, it + i );
		back[2] = GetNoiseValue( ix, iy+1, iz + 1, it + i );
		back[3] = GetNoiseValue( ix+1, iy+1, iz + 1, it + i );

		fvalue = LERP( LERP( front[0], front[1], fx ), LERP( front[2], front[3], fx ), fy );
		bvalue = LERP( LERP( back[0], back[1], fx ), LERP( back[2], back[3], fx ), fy );

		value[i] = LERP( fvalue, bvalue, fz );
	}

	finalvalue = LERP( value[0], value[1], ft );

	return finalvalue;
}