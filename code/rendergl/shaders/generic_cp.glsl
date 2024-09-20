layout( local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;
layout( rgba32f, binding = 0 ) uniform image2D screen;

void main() {
	ivec2 texCoord = ivec2( gl_GlobalInvocationID.xy );

	imageStore( screen, texCoord, vec4( 1.0 ) );
}