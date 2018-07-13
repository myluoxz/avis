namespace glsl {
	const char voxelVert[] = R"(
#version 330

layout(location=0) in vec3 pos;

uniform mat4 _MV;
uniform mat4 _P;
uniform float size;

out vec3 v2f_uvw;
out vec3 v2f_wpos;

void main(){
	vec4 wp = _MV*vec4(pos * size, 1);
	gl_Position = _P*wp;
	v2f_wpos = (wp / wp.w).xyz;
	v2f_uvw = pos * 0.5 + vec3(0.5, 0.5, 0.5);
	//v2f_uvw = pos * 0.5 / size + vec3(1.0, 1.0, 1.0) / 2;
}
)"; }