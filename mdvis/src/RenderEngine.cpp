#include "Engine.h"
#include <random>

void _InitGBuffer(GLuint* d_fbo, GLuint* d_colfbo, GLuint* d_texs, GLuint* d_depthTex, GLuint* d_idTex, GLuint* d_colTex, float w = Display::width, float h = Display::height) {
	glGenFramebuffers(1, d_fbo);
	glGenFramebuffers(1, d_colfbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *d_fbo);

	// Create the gbuffer textures
	glGenTextures(1, d_texs + 1);
	glGenTextures(1, d_idTex);
	glGenTextures(1, d_colTex);
	glGenTextures(1, d_depthTex);
	d_texs[0] = *d_idTex;

	//id
	glBindTexture(GL_TEXTURE_2D, *d_idTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, (int)w, (int)h, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *d_idTex, 0);
	SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
	//normal
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)w, (int)h, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, d_texs[1], 0);
	SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
	// depth
	glBindTexture(GL_TEXTURE_2D, *d_depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (int)w, (int)h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *d_depthTex, 0);
	SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, DrawBuffers);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera", "FB error 1:" + std::to_string(status));
	}

	// color
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *d_colfbo);
	glBindTexture(GL_TEXTURE_2D, *d_colTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)w, (int)h, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *d_colTex, 0);
	SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDrawBuffers(1, DrawBuffers);
	status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera", "FB error 2:" + std::to_string(status));
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Camera::GenGBuffer2() {
	useGBuffer2 = true;
	uint dw2 = uint(d_w * quality2);
	uint dh2 = uint(d_h * quality2);

	if ((dw2 != d_w2) || (dh2 != d_h2)) {
		d_w2 = dw2;
		d_h2 = dh2;

		if (!!d_fbo2) {
			glDeleteFramebuffers(1, &d_fbo2);
			glDeleteTextures(2, d_texs2);
		}
		glGenFramebuffers(1, &d_fbo2);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo2);

		// Create the gbuffer textures
		glGenTextures(2, d_texs2);
		glGenTextures(1, &d_depthTex2);

		//id
		glBindTexture(GL_TEXTURE_2D, d_texs2[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI, d_w2, d_h2, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d_texs2[0], 0);
		SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
		//normal
		glBindTexture(GL_TEXTURE_2D, d_texs2[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, d_w2, d_h2, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, d_texs2[1], 0);
		SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
		// depth
		glBindTexture(GL_TEXTURE_2D, d_depthTex2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, d_w2, d_h2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, d_depthTex2, 0);
		SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, };
		glDrawBuffers(2, DrawBuffers);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("Camera", "FB error 1:" + std::to_string(Status));
		}
	}
}

void Camera::InitGBuffer(uint w, uint h) {
	_InitGBuffer(&d_fbo, &d_colfbo, d_texs, &d_depthTex, &d_idTex, &d_colTex, (float)w, (float)h);

	glGenFramebuffers(NUM_EXTRA_TEXS, d_tfbo);
	glGenTextures(NUM_EXTRA_TEXS, d_ttexs);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	for (byte i = 0; i < NUM_EXTRA_TEXS; i++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_tfbo[i]);
		glBindTexture(GL_TEXTURE_2D, d_ttexs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)Display::width, (int)Display::height, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d_ttexs[i], 0);
		SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDrawBuffers(1, DrawBuffers);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("Camera", "FB error t:" + std::to_string(Status));
		}
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}


GLuint Camera::emptyVao = 0;
//const float Camera::fullscreenVerts[] = { -1, -1, 1,  -1, 1, 1,  1, -1, 1,  1, 1, 1 };
const float Camera::fullscreenVerts[] = { -1, -1,  -1, 1,  1, -1,  1, 1 };
const int Camera::fullscreenIndices[] = { 0, 1, 2, 1, 3, 2 };
GLuint Camera::rectIdBuf = 0;

uint Camera::GetIdAt(uint x, uint y) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	uint pixel[2];
	x = uint(x * quality);
	y = uint(y * quality);
	glReadPixels(x, d_h - y - 1, 1, 1, GL_RG_INTEGER, GL_UNSIGNED_INT, pixel);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	if (pixel[1] != 0) return 0;
	return pixel[0];
}

void Camera::Render(onBlitFunc func) {
	active = this;
	uint t_w = (uint)roundf(Display::width * quality);
	uint t_h = (uint)roundf(Display::height * quality);
	if ((d_w != t_w) || (d_h != t_h)) {
		if (!!d_fbo) {
			glDeleteFramebuffers(1, &d_fbo);
			glDeleteFramebuffers(1, &d_colfbo);
			glDeleteFramebuffers(NUM_EXTRA_TEXS, d_tfbo);
			glDeleteTextures(1, d_texs + 1);
			glDeleteTextures(1, &d_idTex);
			glDeleteTextures(1, &d_colTex);
			glDeleteTextures(1, &d_depthTex);
			glDeleteTextures(NUM_EXTRA_TEXS, d_ttexs);
		}
		InitGBuffer(t_w, t_h);
		_d_fbo = d_fbo;
		memcpy(_d_texs, d_texs, 4 * sizeof(GLuint));
		_d_depthTex = d_depthTex;
		d_w = t_w;
		d_h = t_h;
		_d_w = d_w;
		_d_h = d_h;
		Scene::dirty = true;
		_w = Display::width;
		_h = Display::height;
	}
	if (useGBuffer2) {
		GenGBuffer2();
		if (applyGBuffer2) {
			d_w = d_w2;
			d_h = d_h2;
			d_fbo = d_fbo2;
			memcpy(d_texs, d_texs2, 4 * sizeof(GLuint));
			d_idTex = d_texs2[0];
			d_depthTex = d_depthTex2;
			Display::width = (uint)(_w * quality2);
			Display::height = (uint)(_h * quality2);
		}
	}

	float zero[] = { 0,0,0,0 };
	float one = 1;
	if (Scene::dirty) {
		d_texs[0] = d_idTex;


		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
		glClearBufferfv(GL_COLOR, 0, zero);
		glClearBufferfv(GL_COLOR, 1, zero);
		glClearBufferfv(GL_DEPTH, 0, &one);
		//glClearBufferfv(GL_COLOR, 2, zero);
		//glClearBufferfv(GL_COLOR, 3, zero);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(true);
		glDisable(GL_BLEND);
		MVP::Switch(true);
		MVP::Clear();
		ApplyGL();
		//MVP::Push();
		//MVP::Scale(2, 2, 1);
		MVP::Switch(false);
		MVP::Clear();
		glViewport(0, 0, d_w, d_h);

		if (func) func();

		d_texs[0] = d_colTex;
		glViewport(0, 0, Display::actualWidth, Display::actualHeight);
	}
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	if (useGBuffer2 && applyGBuffer2) {
		Display::width = _w;
		Display::height = _h;
	}

	//DumpBuffers();
	//return;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *d_tfbo);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	if (onBlit) onBlit();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	
	if (useGBuffer2 && applyGBuffer2) {
		d_w = _d_w;
		d_h = _d_h;
		d_fbo = _d_fbo;
		memcpy(d_texs, _d_texs, 4 * sizeof(GLuint));
		d_depthTex = _d_depthTex;
		d_idTex = _d_texs[0];
	}

	Scene::dirty = false;
}
/*
void Camera::_ApplyEmission(GLuint d_fbo, GLuint d_texs[], float w, float h, GLuint targetFbo) {
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_EMISSION_AO);
	glBlitFramebuffer(0, 0, (int)w, (int)h, 0, 0, (int)w, (int)h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}
*/
GLuint Light::_shadowFbo = 0;
GLuint Light::_shadowGITexs[] = {0, 0, 0};
GLuint Light::_shadowMap = 0;
GLuint Light::_shadowCubeFbos[] = { 0, 0, 0, 0, 0, 0 };
GLuint Light::_shadowCubeMap = 0;
GLuint Light::_fluxFbo = 0;
GLuint Light::_fluxTex = 0;
GLuint Light::_rsmFbo = 0;
GLuint Light::_rsmTex = 0;
GLuint Light::_rsmUBO = 0;
RSM_RANDOM_BUFFER Light::_rsmBuffer = RSM_RANDOM_BUFFER();
std::vector<GLint> Light::paramLocs_Point = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_Spot = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotCS = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotFluxer = std::vector<GLint>();
std::vector<GLint> Light::paramLocs_SpotRSM = std::vector<GLint>();

void Light::InitShadow() {
	glGenFramebuffers(1, &_shadowFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowFbo);

	//glGenTextures(3, _shadowGITexs);
	glGenTextures(1, &_shadowMap);

	for (uint i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, _shadowGITexs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _shadowGITexs[i], 0);
		SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);
	}

	// depth
	glBindTexture(GL_TEXTURE_2D, _shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMap, 0);
	SetTexParams<>(0, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, DrawBuffers);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("ShadowMap", "FB error:" + std::to_string(Status));
		abort();
	}
	else {
		Debug::Message("ShadowMap", "FB ok");
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Light::DrawShadowMap(GLuint tar) {
	if (_shadowFbo == 0) {
		Debug::Error("RenderShadow", "Fatal: Fbo not set up!");
	}
	glViewport(0, 0, 1024, 1024);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tar);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowFbo);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	MVP::Switch(false);
	MVP::Clear();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	glEnable(GL_BLEND);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tar);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
}