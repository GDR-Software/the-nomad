#if !defined(GLSL_LEGACY)
layout( location = 0 ) out vec4 a_Color;
#endif

#define mad( a, b, c ) ( a * b + c )

uniform sampler2D u_DiffuseMap;
uniform sampler2D u_BlendTexture;

uniform vec2 u_ScreenSize;

in vec2 v_TexCoords;
in vec4 v_Offset;

vec4 SMAA_RT_METRICS = vec4( 1.0 / u_ScreenSize.x, 1.0 / u_ScreenSize.y, u_ScreenSize.x, u_ScreenSize.y );

/**
 * Conditional move:
 */
void SMAAMovc( bvec2 cond, inout vec2 variable, vec2 value ) {
	if ( cond.x ) {
		variable.x = value.x;
	}
	if (  cond.y ) {
		variable.y = value.y;
	}
}

void SMAAMovc( bvec4 cond, inout vec4 variable, vec4 value ) {
	SMAAMovc( cond.xy, variable.xy, value.xy );
	SMAAMovc( cond.zw, variable.zw, value.zw );
}

void main() {
	vec4 color;

	// Fetch the blending weights for current pixel:
	vec4 a;
	a.x = texture2D( u_BlendTexture, v_Offset.xy ).a; // Right
	a.y = texture2D( u_BlendTexture, v_Offset.zw ).g; // Top
	a.wz = texture2D( u_BlendTexture, v_TexCoords ).xz; // Bottom / Left

	// Is there any blending weight with a value greater than 0.0?
	if ( dot( a, vec4( 1.0, 1.0, 1.0, 1.0 ) ) <= 1e-5 ) {
		color = texture2D( u_DiffuseMap, v_TexCoords ); // LinearSampler
	} else {
		bool h = max( a.x, a.z ) > max( a.y, a.w ); // max(horizontal) > max(vertical)

		// Calculate the blending offsets:
		vec4 blendingOffset = vec4( 0.0, a.y, 0.0, a.w );
		vec2 blendingWeight = a.yw;
		SMAAMovc( bvec4( h, h, h, h ), blendingOffset, vec4( a.x, 0.0, a.z, 0.0 ) );
		SMAAMovc( bvec2( h, h ), blendingWeight, a.xz);
		blendingWeight /= dot( blendingWeight, vec2( 1.0, 1.0 ) );

		// Calculate the texture coordinates:
		vec4 blendingCoord = mad( blendingOffset, vec4( SMAA_RT_METRICS.xy, -SMAA_RT_METRICS.xy ), v_TexCoords.xyxy );

		// We exploit bilinear filtering to mix current pixel with the chosen
		// neighbor:
		color = blendingWeight.x * texture2D( u_DiffuseMap, blendingCoord.xy ); // LinearSampler
		color += blendingWeight.y * texture2D( u_DiffuseMap, blendingCoord.zw ); // LinearSampler
	}

	a_Color = color;
}