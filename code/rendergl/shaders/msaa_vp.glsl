in vec3 a_Position;
in vec2 a_TexCoords;

out vec2 v_TexCoords;

void main() {
	v_TexCoords = a_TexCoords;
	gl_Position = vec4( a_Position, 1.0 );
}