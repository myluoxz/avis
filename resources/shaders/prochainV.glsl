#version 330 core

uniform mat4 _MV, _P;
uniform samplerBuffer poss;
uniform usamplerBuffer ids;
uniform int chainSz;
uniform int chainReso;
uniform int loopReso;
uniform float beziery;

out vec3 v2f_normal;
out float v2f_chpos;

vec3 bezier(vec3 a, vec3 b, vec3 c, vec3 d, float t) {
    vec3 q = mix(a, b, t);
    vec3 r = mix(b, c, t);
    vec3 s = mix(c, d, t);
    vec3 u = mix(q, r, t);
    vec3 v = mix(r, s, t);
    return mix(u, v, t);
}

void main() {

    int loop = gl_VertexID / (6 * loopReso);
    int par = loop / chainReso;
    vec3 p1 = texelFetch(poss, int(texelFetch(ids, par*2).r)).rgb;
    vec3 p2 = texelFetch(poss, int(texelFetch(ids, (par + 1)*2).r)).rgb;
    vec3 h1 = mix(p1, p2, 0.33);
    vec3 h2 = mix(p1, p2, 0.67);
    vec3 n1 = p2 - p1;
    vec3 n2 = n1;

    vec3 p1o = p1;
    if (par > 0) {
        vec3 p0 = texelFetch(poss, int(texelFetch(ids, (par - 1)*2).r)).rgb;
        //p1 = mix(p1, mix(p0, p2, 0.5), 0.33);
        p1 = mix(h1, mix(p1, p0, 0.33), 0.5);
        n1 = p2 - p0;
    }
    if (par < chainSz - 2) {
        vec3 p3 = texelFetch(poss, int(texelFetch(ids, (par + 2)*2).r)).rgb;
        //p2 = mix(p2, mix(p1o, p3, 0.5), 0.33);
        p2 = mix(h2, mix(p2, p3, 0.33), 0.5);
        n2 = p3 - p1o;
    }

    float t = mod(float(loop), chainReso) / chainReso;
    if (mod(gl_VertexID, 2) > 0) t += 1.0/chainReso;

    vec3 pt = texelFetch(poss, int(texelFetch(ids, (par + 11)*2).r)).rgb;
    vec3 pt2 = texelFetch(poss, int(texelFetch(ids, (par + 12)*2).r)).rgb;
    pt = mix(pt, pt2, t);


    //angle of loop
    int li = int(mod(gl_VertexID, 6 * loopReso));
    float ds = 2 * 3.14159 / loopReso;
    float s = (li / 6) * ds;
    float lt = mod(li, 6.0);
    if (lt > 1.5 && lt < 4.5) s += ds;

    vec3 pos0 = bezier(p1, h1, h2, p2, t);

    //get the base tangents
    vec3 c1 = texelFetch(poss, int(texelFetch(ids, int(par)*2 + 1).r)).rgb;
    vec3 c2 = texelFetch(poss, int(texelFetch(ids, int(par)*2 + 3).r)).rgb;
    vec3 td1 = c1 - p1;
    vec3 td2 = c2 - p2;
    vec3 d1 = normalize(cross(n1, td1));
    vec3 db1 = normalize(cross(d1, n1));

    vec3 d2r = normalize(cross(n2, td2));
    vec3 db2r = normalize(cross(d2r, n2));
    vec3 d1t2 = cross(d1, n2);
    vec3 d12 = normalize(cross(n2, d1t2));
    float da = acos(dot(d12, d2r));
    float da2 = round(loopReso * da / (2 * 3.14159)) * 2 * 3.14159 / loopReso;
    vec3 d2 = cos(da2) * d2r + sin(da2) * db2r;
    if (dot(d2, d12) < 0) d2 *= -1.0;
    vec3 db2 = normalize(cross(d2, n2));

    vec3 myTan = normalize(mix(d1, d2, t));
    vec3 myTanB = normalize(mix(db1, db2, t));

    vec3 pos = pos0 + 0.02 * (cos(s) * myTan + sin(s) * myTanB);
    vec3 nrm = normalize(pos - pos0);
    vec3 dpt = vec3(0,1,0);//normalize(pt-pos);
    float css = dot(dpt, nrm);
    float cssa = abs(css);
    vec3 posi = pos - dpt * sign(css) * (1 - exp(-log(cssa + 1))) * 0.02;
    pos = posi;
    pos += dpt * css * 0.04;
    nrm = normalize(posi - pos0);
	v2f_normal = ((_MV*vec4(nrm, 0)).xyz);
    gl_Position = _P * _MV * vec4(pos, 1);
    
	v2f_chpos = float(loop) / ((chainSz-1)*chainReso);
}