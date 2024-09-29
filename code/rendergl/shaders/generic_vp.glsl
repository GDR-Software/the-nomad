in vec3 a_Position;
in uvec2 a_WorldPos;
in vec2 a_TexCoords;
in vec4 a_Color;

out vec2 v_TexCoords;
out vec3 v_FragPos;
out vec4 v_Color;
out vec3 v_WorldPos;

uniform mat4 u_ModelViewProjection;
uniform vec4 u_BaseColor;
uniform vec4 u_VertColor;

#if defined(USE_RGBAGEN)
uniform int u_ColorGen;
uniform int u_AlphaGen;
uniform vec3 u_DirectedLight;
#endif

#if defined(USE_TCGEN)
uniform vec4 u_DiffuseTexMatrix;
uniform vec4 u_DiffuseTexOffTurb;
#endif

#if defined(USE_TCGEN)
uniform int u_TCGen0;
uniform vec3 u_TCGen0Vector0;
uniform vec3 u_TCGen0Vector1;
uniform vec3 u_WorldPos;
#endif

#if defined(USE_TCMOD)
vec2 ModTexCoords( vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb )
{
	float amplitude = offTurb.z;
	float phase = offTurb.w * 2.0 * M_PI;
	vec2 st2;

	st2.x = st.x * texMatrix.x + ( st.y * texMatrix.z + offTurb.x );
	st2.y = st.x * texMatrix.y + ( st.y * texMatrix.w + offTurb.y );

	vec2 offsetPos = vec2( position.x + position.z, position.y );

	vec2 texOffset = sin( offsetPos * ( 2.0 * M_PI / 1024.0 ) + vec2( phase ) );

	return st2 + texOffset * amplitude;	
}
#endif

#if defined(USE_TCGEN)
vec2 GenTexCoords( int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1 )
{
	vec2 tex = a_TexCoords;

	if ( TCGen == TCGEN_LIGHTMAP ) {
		tex = a_TexCoords.st;
	}
	else if ( TCGen == TCGEN_ENVIRONMENT_MAPPED ) {
		vec3 viewer = normalize( vec3( 0.0 ) - position );
		vec2 ref = reflect( viewer, normal ).yz;
		tex.s = ref.x * -0.5 + 0.5;
		tex.t = ref.y *  0.5 + 0.5;
	}
	else if ( TCGen == TCGEN_VECTOR ) {
		tex = vec2( dot( position, TCGenVector0 ), dot( position, TCGenVector1 ) );
	}

	return tex;
}
#endif

void main() {
	vec3 position = vec3( a_Position.xy, 0.0 );

	if ( u_ColorGen == CGEN_VERTEX ) {
		v_Color = vec4( 1.0 );
	} else {
		v_Color = u_VertColor * a_Color + u_BaseColor;
	}

#if defined(USE_TCGEN)
	vec2 texCoords = GenTexCoords( u_TCGen0, position, vec3( 0.0 ), u_TCGen0Vector0, u_TCGen0Vector1 );
#else
	vec2 texCoords = a_TexCoords;
#endif

#if defined(USE_TCMOD)
	v_TexCoords = ModTexCoords( texCoords, position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb );
#else
	v_TexCoords = texCoords;
#endif

	v_WorldPos = vec3( a_WorldPos.xy, 0.0 );

    gl_Position = u_ModelViewProjection * vec4( position, 1.0 );
}