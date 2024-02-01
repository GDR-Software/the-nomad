
const char *fallbackShader_generic_fp =
"#if !defined(GLSL_LEGACY)\n"
"out vec4 a_Color;\n"
"#endif\n"
"\n"
"in vec2 v_TexCoords;\n"
"in vec3 v_FragPos;\n"
"in vec4 v_Color;\n"
"\n"
"uniform sampler2D u_DiffuseMap;\n"
"\n"
"#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)\n"
"uniform vec4 u_SpecularScale;\n"
"uniform vec4 u_NormalScale;\n"
"\n"
"uniform vec4 u_AmbientColor;\n"
"uniform float u_AmbientIntensity;\n"
"#endif\n"
"\n"
"#if defined(USE_NORMALMAP)\n"
"uniform sampler2D u_NormalMap;\n"
"#endif\n"
"\n"
"#if defined(USE_SPECULARMAP)\n"
"uniform sampler2D u_SpecularMap;\n"
"#endif\n"
"\n"
"#if defined(USE_SHADOWMAP)\n"
"uniform sampler2D u_ShadowMap;\n"
"#endif\n"
"\n"
"uniform int u_AlphaTest;\n"
"\n"
"float CalcLightAttenuation(float point, float normDist)\n"
"{\n"
"	// zero light at 1.0, approximating q3 style\n"
"	// also don't attenuate directional light\n"
"	float attenuation = (0.5 * normDist - 1.5) * point + 1.0;\n"
"\n"
"	// clamp attenuation\n"
"	#if defined(NO_LIGHT_CLAMP)\n"
"	attenuation = max(attenuation, 0.0);\n"
"	#else\n"
"	attenuation = clamp(attenuation, 0.0, 1.0);\n"
"	#endif\n"
"\n"
"	return attenuation;\n"
"}\n"
"\n"
"vec3 CalcDiffuse(vec3 diffuseAlbedo, float NH, float EH, float roughness)\n"
"{\n"
"#if defined(USE_BURLEY)\n"
"	// modified from https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf\n"
"	float fd90 = -0.5 + EH * EH * roughness;\n"
"	float burley = 1.0 + fd90 * 0.04 / NH;\n"
"	burley *= burley;\n"
"	return diffuseAlbedo * burley;\n"
"#else\n"
"	return diffuseAlbedo;\n"
"#endif\n"
"}\n"
"\n"
"#if 0\n"
"void applyLighting() {\n"
"    vec3 ambient = u_AmbientColor + u_AmbientIntensity;\n"
"\n"
"    for (int i = 0; i < u_numLights; i++) {\n"
"        float diffuse = 0.0;\n"
"        mat4 model = mat4(1.0);\n"
"\n"
"        //\n"
"        // using world position will give the lighting a more\n"
"        // retro and pixel-game (8-bit) style look because\n"
"        // individual tiles will be lit up\n"
"        //\n"
"\n"
"        float dist = distance(lights[i].origin, v_WorldPos);\n"
"        vec3 color = lights[i].color + lights[i].intensity.x;\n"
"        float range = lights[i].intensity.y;\n"
"\n"
"        if (dist <= range) {\n"
"            diffuse = 1.0 - abs(dist / lights[i].intensity.x);\n"
"        }\n"
"\n"
"        a_Color *= vec4(min(a_Color.rgb * ((lights[i].color * diffuse) + u_AmbientColor), a_Color.rgb), 1.0);\n"
"    }\n"
"}\n"
"#endif\n"
"\n"
"void AmbientLight() {\n"
"//    vec3 color = u_AmbientColor + u_AmbientIntensity;\n"
"}\n"
"\n"
"void main() {\n"
"    vec4 color = v_Color;\n"
"    color = vec4(1.0);\n"
"    a_Color = texture2D( u_DiffuseMap, v_TexCoords ) * color;\n"
"}\n"
;

const char *fallbackShader_imgui_fp =
"#if !defined(GLSL_LEGACY)\n"
"layout(location = 0) out vec4 a_Color;\n"
"#endif\n"
"\n"
"in vec2 v_TexCoord;\n"
"in vec4 v_Color;\n"
"\n"
"uniform sampler2D u_DiffuseMap;\n"
"uniform float u_GammaAmount;\n"
"\n"
"#if defined(USE_LIGHT) && !defined(USE_FAST_LIGHT)\n"
"uniform vec4 u_SpecularScale;\n"
"uniform vec4 u_NormalScale;\n"
"\n"
"uniform vec4 u_AmbientColor;\n"
"uniform float u_AmbientIntensity;\n"
"#endif\n"
"\n"
"#if defined(USE_NORMALMAP)\n"
"uniform sampler2D u_NormalMap;\n"
"#endif\n"
"\n"
"#if defined(USE_SPECULARMAP)\n"
"uniform sampler2D u_SpecularMap;\n"
"#endif\n"
"\n"
"#if defined(USE_SHADOWMAP)\n"
"uniform sampler2D u_ShadowMap;\n"
"#endif\n"
"\n"
"uniform int u_AlphaTest;\n"
"\n"
"float CalcLightAttenuation(float point, float normDist)\n"
"{\n"
"	// zero light at 1.0, approximating q3 style\n"
"	// also don't attenuate directional light\n"
"	float attenuation = (0.5 * normDist - 1.5) * point + 1.0;\n"
"\n"
"	// clamp attenuation\n"
"	#if defined(NO_LIGHT_CLAMP)\n"
"	attenuation = max(attenuation, 0.0);\n"
"	#else\n"
"	attenuation = clamp(attenuation, 0.0, 1.0);\n"
"	#endif\n"
"\n"
"	return attenuation;\n"
"}\n"
"\n"
"vec3 CalcDiffuse(vec3 diffuseAlbedo, float NH, float EH, float roughness)\n"
"{\n"
"#if defined(USE_BURLEY)\n"
"	// modified from https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf\n"
"	float fd90 = -0.5 + EH * EH * roughness;\n"
"	float burley = 1.0 + fd90 * 0.04 / NH;\n"
"	burley *= burley;\n"
"	return diffuseAlbedo * burley;\n"
"#else\n"
"	return diffuseAlbedo;\n"
"#endif\n"
"}\n"
"\n"
"void main() {\n"
"    a_Color = v_Color * texture(u_DiffuseMap, v_TexCoord);\n"
"    a_Color.rgb = pow(a_Color.rgb, vec3( 1.0 / u_GammaAmount ));\n"
"}\n"
;

const char *fallbackShader_generic_vp =
"in vec3 a_Position;\n"
"in vec2 a_TexCoords;\n"
"in vec4 a_Color;\n"
"\n"
"out vec2 v_TexCoords;\n"
"out vec3 v_FragPos;\n"
"out vec4 v_Color;\n"
"\n"
"uniform mat4 u_ModelViewProjection;\n"
"uniform vec4 u_BaseColor;\n"
"uniform vec4 u_VertColor;\n"
"\n"
"uniform mat4 u_ModelMatrix;\n"
"\n"
"#if defined(USE_RGBAGEN)\n"
"uniform int u_ColorGen;\n"
"uniform int u_AlphaGen;\n"
"uniform vec3 u_DirectedLight;\n"
"#endif\n"
"\n"
"#if defined(USE_TCGEN)\n"
"uniform vec4 u_DiffuseTexMatrix;\n"
"uniform vec4 u_DiffuseTexOffTurb;\n"
"#endif\n"
"\n"
"#if defined(USE_TCGEN)\n"
"uniform int u_TCGen0;\n"
"uniform vec3 u_TCGen0Vector0;\n"
"uniform vec3 u_TCGen0Vector1;\n"
"uniform vec3 u_WorldPos;\n"
"#endif\n"
"\n"
"#if defined(USE_RGBAGEN)\n"
"vec4 CalcColor(vec3 position, vec3 normal)\n"
"{\n"
"	vec4 color = u_VertColor * a_Color + u_BaseColor;\n"
"\n"
"	if (u_ColorGen == CGEN_LIGHTING_DIFFUSE)\n"
"	{\n"
"		float incoming = clamp(dot(normal, u_ModelLightDir), 0.0, 1.0);\n"
"\n"
"//		color.rgb = clamp(u_DirectedLight * incoming + u_AmbientLight, 0.0, 1.0);\n"
"	}\n"
"\n"
"	vec3 viewer = u_LocalViewOrigin - position;\n"
"\n"
"	if (u_AlphaGen == AGEN_LIGHTING_SPECULAR)\n"
"	{\n"
"		vec3 lightDir = normalize(vec3(-960.0, 1980.0, 96.0) - position);\n"
"		vec3 reflected = -reflect(lightDir, normal);\n"
"\n"
"		color.a = clamp(dot(reflected, normalize(viewer)), 0.0, 1.0);\n"
"		color.a *= color.a;\n"
"		color.a *= color.a;\n"
"	}\n"
"	else if (u_AlphaGen == AGEN_PORTAL)\n"
"	{\n"
"		color.a = clamp(length(viewer) / u_PortalRange, 0.0, 1.0);\n"
"	}\n"
"\n"
"	return color;\n"
"}\n"
"#endif\n"
"\n"
"#if defined(USE_TCMOD)\n"
"vec2 ModTexCoords( vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb )\n"
"{\n"
"	float amplitude = offTurb.z;\n"
"	float phase = offTurb.w * 2.0 * M_PI;\n"
"	vec2 st2;\n"
"\n"
"	st2.x = st.x * texMatrix.x + ( st.y * texMatrix.z + offTurb.x );\n"
"	st2.y = st.x * texMatrix.y + ( st.y * texMatrix.w + offTurb.y );\n"
"\n"
"	vec2 offsetPos = vec2( position.x + position.z, position.y );\n"
"\n"
"	vec2 texOffset = sin( offsetPos * ( 2.0 * M_PI / 1024.0 ) + vec2( phase ) );\n"
"\n"
"	return st2 + texOffset * amplitude;\n"
"}\n"
"#endif\n"
"\n"
"float CalcLightAttenuation( float point, float normDist )\n"
"{\n"
"	// zero light at 1.0, approximating q3 style\n"
"	// also don't attenuate directional light\n"
"	float attenuation = ( 0.5 * normDist - 1.5 ) * point + 1.0;\n"
"\n"
"	// clamp attenuation\n"
"#if defined(NO_LIGHT_CLAMP)\n"
"	attenuation = max( attenuation, 0.0 );\n"
"#else\n"
"	attenuation = clamp( attenuation, 0.0, 1.0 );\n"
"#endif\n"
"\n"
"	return attenuation;\n"
"}\n"
"\n"
"#if defined(USE_TCGEN)\n"
"vec2 GenTexCoords( int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1 )\n"
"{\n"
"	vec2 tex = a_TexCoords;\n"
"\n"
"	if ( TCGen == TCGEN_LIGHTMAP ) {\n"
"	//	tex = a_TexCoord1.st;\n"
"	}\n"
"	else if ( TCGen == TCGEN_ENVIRONMENT_MAPPED ) {\n"
"		vec3 viewer = normalize( u_WorldPos - position );\n"
"		vec2 ref = reflect( viewer, normal ).yz;\n"
"		tex.s = ref.x * -0.5 + 0.5;\n"
"		tex.t = ref.y *  0.5 + 0.5;\n"
"	}\n"
"	else if ( TCGen == TCGEN_VECTOR ) {\n"
"		tex = vec2( dot( position, TCGenVector0 ), dot( position, TCGenVector1 ) );\n"
"	}\n"
"\n"
"	return tex;\n"
"}\n"
"#endif\n"
"\n"
"void main()\n"
"{\n"
"	v_Color = a_Color;\n"
"//\n"
"//#if defined(USE_TCGEN)\n"
"//	vec2 texCoords = GenTexCoords( u_TCGen0, a_Position, vec3( 0.0 ), u_TCGen0Vector0, u_TCGen0Vector1 );\n"
"//#else\n"
"//	vec2 texCoords = a_TexCoords;\n"
"//#endif\n"
"//\n"
"//#if defined(USE_TCMOD)\n"
"//	v_TexCoords = ModTexCoords( texCoords, a_Position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb );\n"
"//#else\n"
"//	v_TexCoords = texCoords;\n"
"//#endif\n"
"	v_TexCoords = a_TexCoords;\n"
"\n"
"	v_FragPos = vec4( u_ModelViewProjection * vec4( a_Position, 1.0 ) ).xyz;\n"
"\n"
"    gl_Position = u_ModelViewProjection * vec4( a_Position, 1.0 );\n"
"}\n"
;

const char *fallbackShader_imgui_vp =
"in vec3 a_Position;\n"
"in vec2 a_TexCoord;\n"
"in vec4 a_Color;\n"
"\n"
"uniform mat4 u_ModelViewProjection;\n"
"\n"
"out vec2 v_TexCoord;\n"
"out vec4 v_Color;\n"
"\n"
"uniform vec4 u_BaseColor;\n"
"uniform vec4 u_VertColor;\n"
"\n"
"uniform mat4 u_ModelMatrix;\n"
"\n"
"#if defined(USE_RGBAGEN)\n"
"uniform int u_ColorGen;\n"
"uniform int u_AlphaGen;\n"
"uniform vec3 u_DirectedLight;\n"
"#endif\n"
"\n"
"#if defined(USE_TCGEN)\n"
"uniform vec4 u_DiffuseTexMatrix;\n"
"uniform vec4 u_DiffuseTexOffTurb;\n"
"#endif\n"
"\n"
"#if defined(USE_TCGEN)\n"
"uniform int u_TCGen0;\n"
"uniform vec3 u_TCGen0Vector0;\n"
"uniform vec3 u_TCGen0Vector1;\n"
"uniform vec3 u_WorldPos;\n"
"#endif\n"
"\n"
"#if defined(USE_RGBAGEN)\n"
"vec4 CalcColor(vec3 position, vec3 normal)\n"
"{\n"
"	vec4 color = u_VertColor * a_Color + u_BaseColor;\n"
"\n"
"	if (u_ColorGen == CGEN_LIGHTING_DIFFUSE)\n"
"	{\n"
"		float incoming = clamp(dot(normal, u_ModelLightDir), 0.0, 1.0);\n"
"\n"
"//		color.rgb = clamp(u_DirectedLight * incoming + u_AmbientLight, 0.0, 1.0);\n"
"	}\n"
"\n"
"	vec3 viewer = u_LocalViewOrigin - position;\n"
"\n"
"	if (u_AlphaGen == AGEN_LIGHTING_SPECULAR)\n"
"	{\n"
"		vec3 lightDir = normalize(vec3(-960.0, 1980.0, 96.0) - position);\n"
"		vec3 reflected = -reflect(lightDir, normal);\n"
"\n"
"		color.a = clamp(dot(reflected, normalize(viewer)), 0.0, 1.0);\n"
"		color.a *= color.a;\n"
"		color.a *= color.a;\n"
"	}\n"
"	else if (u_AlphaGen == AGEN_PORTAL)\n"
"	{\n"
"		color.a = clamp(length(viewer) / u_PortalRange, 0.0, 1.0);\n"
"	}\n"
"\n"
"	return color;\n"
"}\n"
"#endif\n"
"\n"
"#if defined(USE_TCMOD)\n"
"vec2 ModTexCoords( vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb )\n"
"{\n"
"	float amplitude = offTurb.z;\n"
"	float phase = offTurb.w * 2.0 * M_PI;\n"
"	vec2 st2;\n"
"\n"
"	st2.x = st.x * texMatrix.x + ( st.y * texMatrix.z + offTurb.x );\n"
"	st2.y = st.x * texMatrix.y + ( st.y * texMatrix.w + offTurb.y );\n"
"\n"
"	vec2 offsetPos = vec2( position.x + position.z, position.y );\n"
"\n"
"	vec2 texOffset = sin( offsetPos * ( 2.0 * M_PI / 1024.0 ) + vec2( phase ) );\n"
"\n"
"	return st2 + texOffset * amplitude;\n"
"}\n"
"#endif\n"
"\n"
"float CalcLightAttenuation( float point, float normDist )\n"
"{\n"
"	// zero light at 1.0, approximating q3 style\n"
"	// also don't attenuate directional light\n"
"	float attenuation = ( 0.5 * normDist - 1.5 ) * point + 1.0;\n"
"\n"
"	// clamp attenuation\n"
"#if defined(NO_LIGHT_CLAMP)\n"
"	attenuation = max( attenuation, 0.0 );\n"
"#else\n"
"	attenuation = clamp( attenuation, 0.0, 1.0 );\n"
"#endif\n"
"\n"
"	return attenuation;\n"
"}\n"
"\n"
"#if defined(USE_TCGEN)\n"
"vec2 GenTexCoords( int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1 )\n"
"{\n"
"	vec2 tex = a_TexCoord;\n"
"\n"
"	if ( TCGen == TCGEN_LIGHTMAP ) {\n"
"	//	tex = a_TexCoord1.st;\n"
"	}\n"
"	else if ( TCGen == TCGEN_ENVIRONMENT_MAPPED ) {\n"
"		vec3 viewer = normalize( u_WorldPos - position );\n"
"		vec2 ref = reflect( viewer, normal ).yz;\n"
"		tex.s = ref.x * -0.5 + 0.5;\n"
"		tex.t = ref.y *  0.5 + 0.5;\n"
"	}\n"
"	else if ( TCGen == TCGEN_VECTOR ) {\n"
"		tex = vec2( dot( position, TCGenVector0 ), dot( position, TCGenVector1 ) );\n"
"	}\n"
"\n"
"	return tex;\n"
"}\n"
"#endif\n"
"\n"
"void main()\n"
"{\n"
"#if defined(USE_RGBAGEN)\n"
"    v_Color = CalcColor( a_Position, a_Normal );\n"
"#else\n"
"    v_Color = u_VertColor * a_Color + u_BaseColor;\n"
"#endif\n"
"\n"
"#if defined(USE_TCGEN)\n"
"	vec2 texCoords = GenTexCoords( u_TCGen0, a_Position, vec3( 0.0 ), u_TCGen0Vector0, u_TCGen0Vector1 );\n"
"#else\n"
"	vec2 texCoords = a_TexCoord;\n"
"#endif\n"
"\n"
"#if defined(USE_TCMOD)\n"
"	v_TexCoord = ModTexCoords( texCoords, position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb );\n"
"#else\n"
"	v_TexCoord = texCoords;\n"
"#endif\n"
"    v_Color = a_Color;\n"
"    gl_Position = u_ModelViewProjection * vec4( a_Position.xy, 0, 1 );\n"
"}\n"
;
