#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 col;

uniform mat4 _MV, _P;
uniform vec3 camPos;
uniform vec3 camFwd;

uniform float orthoSz;
uniform vec2 screenSize;
uniform samplerBuffer radTex;
uniform float radScl;
uniform float spriteScl;
uniform bool oriented;
uniform float orienScl;

uniform float tubesize;

layout (std140) uniform clipping {
    vec4 clip_planes[6];
};
bool clipped(vec3 pos) {
    for (int a = 0; a < 6; ++a) {
        if (dot(pos, clip_planes[a].xyz) > clip_planes[a].w)
            return true;
    }
    return false;
}

float _getquadsize(float z, float w, float r) {
	float th = atan(w / z);
	float a = sqrt(z*z + w*w);
	float ph = asin(r / a);
	return z * tan(th + ph) - w;
}

float getquadsize(vec3 wpos, vec3 spos, float r) {
	const float fov = 60 * 3.14159 / 180;
	const float tf = tan(fov/2);
	float tf2 = screenSize.x * tf / screenSize.y;

	vec3 c2o = wpos - camPos;
	float lc2o = length(c2o);
	float cth = dot(camFwd, (c2o / lc2o));
	float z = lc2o * cth;
	//if (z < r) return 0;
	vec2 wm = z * vec2(tf2, tf);
	vec2 w = spos.xy * wm;

	float sx = _getquadsize(z, abs(w.x), r);
	sx *= screenSize.x / wm.x;
	float sy = _getquadsize(z, abs(w.y), r);
	sy *= screenSize.y / wm.y;

	return max(sx, sy);
}

flat out int v2f_id;
out vec3 v2f_pos;
out float v2f_scl;
out float v2f_rad;

void main(){
	float radTexel = texelFetch(radTex,gl_VertexID).r;
	if (radTexel < 0) {
		gl_PointSize = 0;
		return;
	}
	vec4 wpos = _MV*vec4(pos, 1);
	wpos /= wpos.w;
	if (clipped(wpos.xyz)) {
		gl_PointSize = 0;
		return;
	}

	gl_Position = _P*wpos;
	v2f_pos = wpos.xyz;
	v2f_id = gl_VertexID + 1;
	if (radScl <= 0) v2f_rad = tubesize * 2;
	else if (radTexel == 0) v2f_rad = 0.17 * radScl;
	else v2f_rad = 0.1 * radTexel * radScl;

	vec4 unitVec = _MV*vec4(1, 0, 0, 0);
	v2f_scl = length(unitVec);

	if (radScl == 0) gl_PointSize = 1;
	else {
		if (orthoSz < 0) {
			//gl_PointSize = getquadsize(wpos.xyz, gl_Position.xyz / gl_Position.w, spriteScl * v2f_rad) * 2;

			vec3 wdir = wpos.xyz - camPos;
			float z = length(wdir);
			float ca = dot(normalize(wpos.xyz - camPos), camFwd);
			float z2 = z * ca;
			if (z2 < 0.1) gl_PointSize = 0;
			else gl_PointSize = spriteScl * v2f_scl * v2f_rad * 2 * screenSize.y / z2;
		}
		else {
			gl_PointSize = spriteScl * screenSize.y * v2f_scl * v2f_rad * 2 / orthoSz;
		}
		if (oriented) {
			gl_PointSize *= orienScl;
		}
	}
}