#pragma once
namespace glsl {
	const char fontVert[] = R"(
#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in int c;

uniform vec2 off;
uniform int mask;

out vec2 UV;
void main() {
	int cc = c & 0x00ff;
	int mk = c & 0xff00;
	if (mk == mask) {
		gl_Position.xyz = (pos + vec3(off, 0))*2 - vec3(1,1,0);
		vec2 uv1 = vec2(mod(cc, 16), (cc/16));
		UV = (uv1 + uv)/16;
	}
	else {
		gl_Position.xyz = vec3(-2, -2, 0);
		UV = vec2(0, 0);
	}
	gl_Position.w = 1;
}
)";
}