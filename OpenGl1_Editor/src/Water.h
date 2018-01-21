#pragma once
#define _WATERY

#include "Engine.h"

const float _si_l = 1e-9f;
const float _si_m = 1.66e-27f;
const float _si_t = 1e-12f;
const float _si_q = 1.602e-19f;

const float _ast = 0.1f;

const float BOND_LENGTH = 1 * _ast;
const float BOND_ANGLE = 109.47f * deg2rad;
const float K_LINEAR = 345000; //kJ / mol nm^2
const float K_RADIAL = 383; //kJ / mol rad^2
const float dt = 1e-4f; //0.1fs

const float MASS_H = 1.0f;
const float MASS_O = 15.99f;

/*
class WaterParticle {
public:
	bool is_oxygen;

	float charge;
	dVec3 position, velocity, force;
	uint p1, p2; //h1, h2 if oxygen, o, h2 if hydrogen
	double energy;

	void repos(), f_bond();
};
*/

class Water {
public:
	static Water* me;

	Water(string path, uint cnt, float d, float t);

	//std::vector<WaterParticle*> particles;

	//static const WaterParticle* Get(uint i);
	void Update(), Draw();

	IComputeBuffer* frb, *psb, *vlb, *iob, *prb;
	ComputeShader* shader;

	float dens, temp, wall;

	const uint particlecount, threads;
	GLuint forSSBO, posSSBO, velSSBO, prmSSBO, rdfSSBO;
	GLuint computeprog;
	Vec4* colors;
};