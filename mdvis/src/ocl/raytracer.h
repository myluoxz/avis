#pragma once
#include "Engine.h"
#include "utils/BVH.h"
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#ifdef PLATFORM_OSX
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#ifdef PLATFORM_WIN
#pragma comment(lib, "OpenCL.lib")
#endif

class RayTracer {
public:
	struct mat_st {
		cl_float rough;
		cl_float specular;
		cl_float gloss;
		cl_float metallic;
		cl_float ior;
	};
	static struct info_st {
		cl_int w;
		cl_int h;
		cl_float IP[16];
		cl_int rand;
		cl_float str;
		cl_int bg_w;
		cl_int bg_h;
		mat_st mat;
	} info;

	static bool Init();
	
	static bool live, expDirty;
	static BVH::Node* bvh;
	static uint bvhSz;
	static Mat4x4 IP;
	static string bgName;
	static float prvRes;
	static uint resw, resh;
	static uint prvSmp;

	static void SetScene();
	static void CheckRes();
	static void SetTex(uint w, uint h), SetBg(string fl);
	static void Render();
	static void Clear();

	static void DrawMenu();

	static GLuint resTex;
	
	static cl_context _ctx;
	static cl_command_queue _que;
	static cl_kernel _kernel;
};