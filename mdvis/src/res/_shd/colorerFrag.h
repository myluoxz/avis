namespace glsl {
    const char colererFrag[] = R"(
#version 330

uniform usampler2D idTex;
uniform vec2 screenSize;
uniform samplerBuffer id2col;
uniform sampler2D colList;
uniform int usegrad;

layout (location = 0) out vec4 fragCol;

vec3 gradfill(float f) {
	vec3 v;
	v.b = clamp(2 - 4 * f, 0, 1);
	v.g = clamp(2 - abs(2 - 4 * f), 0, 1);
	v.r = clamp(4 * f - 2, 0, 1);
	return v;
}

void main () {
	//float idf = floor(texture(idTex, gl_FragCoord.xy / screenSize).x);
	//int id = int(idf);
	int id = int(texture(idTex, gl_FragCoord.xy / screenSize).x);
	if (id == 0) {
		fragCol = vec4(0.5, 0.5, 0.5, 1);
		return;
	}
	float cdf = texelFetch(id2col, id-1).r;
	int cd = int(cdf * 255);
	if (usegrad == 1) fragCol.rgb = gradfill(cd / 255.0);
	else fragCol = texture(colList, vec2((mod(cd, 16) + 0.5) / 16.0, ((cd / 16) + 0.5) / 16.0));
}
)";
}