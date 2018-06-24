#include "Engine.h"
#include "Editor.h"
#include <random>

void CheckGLOK() {
	int err = glGetError();
	if (err != GL_NO_ERROR) {
		std::cout << std::hex << err << std::endl;
		abort();
	}
}

void Camera::DrawSceneObjectsOpaque(std::vector<pSceneObject> oo, GLuint shader) {
	for (auto& sc : oo)
	{
		MVP::Push();
		MVP::Mul(sc->transform.localMatrix());
		auto mrd = sc->GetComponent<MeshRenderer>();
		if (mrd) {
			mrd->DrawDeferred(shader);
		}
		auto smd = sc->GetComponent<SkinnedMeshRenderer>();
		if (smd) {
			smd->DrawDeferred(shader);
		}
		DrawSceneObjectsOpaque(sc->children, shader);
		MVP::Pop();
	}
}

void Camera::DrawSceneObjectsOverlay(std::vector<pSceneObject> oo, GLuint shader) {
	return;
	/*
	for (auto& sc : oo)
	{
		MVP::Push();
		MVP::Mul(sc->transform.localMatrix());
		auto vrd = sc->GetComponent<VoxelRenderer>();
		if (vrd) {
			vrd->Draw();
		}
		DrawSceneObjectsOverlay(sc->children, shader);
		MVP::Pop();
	}
	*/
}

void _InitGBuffer(GLuint* d_fbo, GLuint* d_colfbo, GLuint* d_texs, GLuint* d_depthTex, GLuint* d_idTex, GLuint* d_colTex, float w = Display::width, float h = Display::height) {
	glGenFramebuffers(1, d_fbo);
	glGenFramebuffers(1, d_colfbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *d_fbo);

	// Create the gbuffer textures
	glGenTextures(3, d_texs + 1);
	glGenTextures(1, d_idTex);
	glGenTextures(1, d_colTex);
	glGenTextures(1, d_depthTex);
	d_texs[0] = *d_idTex;

	glBindTexture(GL_TEXTURE_2D, *d_idTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, (int)w, (int)h, 0, GL_RED, GL_UNSIGNED_INT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *d_idTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	for (uint i = 1; i < 4; i++) {
		glBindTexture(GL_TEXTURE_2D, d_texs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, (int)w, (int)h, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, d_texs[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	// depth
	glBindTexture(GL_TEXTURE_2D, *d_depthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (int)w, (int)h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *d_depthTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, DrawBuffers);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera", "FB error 1:" + std::to_string(Status));
	}

	// color
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, *d_colfbo);

	glBindTexture(GL_TEXTURE_2D, *d_colTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)w, (int)h, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *d_colTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDrawBuffers(1, DrawBuffers);

	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("Camera", "FB error 2:" + std::to_string(Status));
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
			glDeleteTextures(4, d_texs2);
		}
		glGenFramebuffers(1, &d_fbo2);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo2);

		// Create the gbuffer textures
		glGenTextures(4, d_texs2);
		glGenTextures(1, &d_depthTex2);

		glBindTexture(GL_TEXTURE_2D, d_texs2[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, d_w2, d_h2, 0, GL_RED, GL_UNSIGNED_INT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d_texs2[0], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		for (uint i = 1; i < 4; i++) {
			glBindTexture(GL_TEXTURE_2D, d_texs2[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, d_w2, d_h2, 0, GL_RGBA, GL_FLOAT, NULL);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, d_texs2[i], 0);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		// depth
		glBindTexture(GL_TEXTURE_2D, d_depthTex2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, d_w2, d_h2, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, d_depthTex2, 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, DrawBuffers);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("Camera", "FB error 1:" + std::to_string(Status));
		}
	}
}


RenderTexture::RenderTexture(uint w, uint h, RT_FLAGS flags, const GLvoid* pixels, GLenum pixelFormat, GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT) : Texture(), depth(!!(flags & RT_FLAG_DEPTH)), stencil(!!(flags & RT_FLAG_STENCIL)), hdr(!!(flags & RT_FLAG_HDR)) {
	width = w;
	height = h;
	_texType = TEX_TYPE_RENDERTEXTURE;

	glGenFramebuffers(1, &d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);

	glGenTextures(1, &pointer);

	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, hdr? GL_RGBA32F : GL_RGBA, w, h, 0, pixelFormat, hdr? GL_FLOAT : GL_UNSIGNED_BYTE, pixels);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pointer, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("RenderTexture", "FB error:" + std::to_string(Status));
	}
	else loaded = true;
}

RenderTexture::~RenderTexture() {
	glDeleteFramebuffers(1, &d_fbo);
}

void RenderTexture::Blit(Texture* src, RenderTexture* dst, Shader* shd, string texName) {
	if (src == nullptr || dst == nullptr || shd == nullptr) {
		Debug::Warning("Blit", "Parameter is null!");
		return;
	}
	Blit(src->pointer, dst, shd->pointer, texName);
}

void RenderTexture::Blit(GLuint src, RenderTexture* dst, GLuint shd, string texName) {
	glViewport(0, 0, dst->width, dst->height);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->d_fbo);
	float zero[] = { 0,0,1,1 };
	glClearBufferfv(GL_COLOR, 0, zero);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_BLEND);

	glUseProgram(shd);
	GLint loc = glGetUniformLocation(shd, "mainTex");
	glUniform1i(loc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, src);
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, &Camera::fullscreenVerts[0]);
	glBindVertexArray(Camera::emptyVao);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, &Camera::fullscreenVerts[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &Camera::fullscreenIndices[0]);

	glBindVertexArray(0);
	//glDisableVertexAttribArray(0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void RenderTexture::Blit(Texture* src, RenderTexture* dst, Material* mat, string texName) {
	glViewport(0, 0, dst->width, dst->height);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->d_fbo);
	float zero[] = { 0,0,0,0 };
	glClearBufferfv(GL_COLOR, 0, zero);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_BLEND);

	mat->SetTexture(texName, src);
	Mat4x4 dm = Mat4x4();
	mat->ApplyGL(dm, dm);

	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, &Camera::fullscreenVerts[0]);
	glBindVertexArray(Camera::emptyVao);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, &Camera::fullscreenVerts[0]);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, &Camera::fullscreenIndices[0]);

	glBindVertexArray(0);
	//glDisableVertexAttribArray(0);
	glUseProgram(0);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

/*
std::vector<float> RenderTexture::pixels() {
	std::vector<float> v = std::vector<float>(width*height * 3);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, &v[0]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	return v;
}
*/

void RenderTexture::Load(string path) {
	throw std::runtime_error("RT Load (s) not implemented");
}
void RenderTexture::Load(std::istream& strm) {
	throw std::runtime_error("RT Load (i) not implemented");
}

bool RenderTexture::Parse(string path) {
	string ss(path + ".meta");
	std::ofstream str(ss, std::ios::out | std::ios::trunc | std::ios::binary);
	str << "IMR";
	return true;
}


void Camera::InitGBuffer(uint w, uint h) {
	_InitGBuffer(&d_fbo, &d_colfbo, d_texs, &d_depthTex, &d_idTex, &d_colTex, (float)w, (float)h);

	glGenFramebuffers(NUM_EXTRA_TEXS, d_tfbo);
	glGenTextures(NUM_EXTRA_TEXS, d_ttexs);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	for (byte i = 0; i < NUM_EXTRA_TEXS; i++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_tfbo[i]);
		glBindTexture(GL_TEXTURE_2D, d_ttexs[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (int)Display::width, (int)Display::height, 0, GL_RED, GL_UNSIGNED_INT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d_ttexs[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
GLuint Camera::d_probeMaskProgram = 0;
GLuint Camera::d_probeProgram = 0;
GLuint Camera::d_blurProgram = 0;
GLuint Camera::d_blurSBProgram = 0;
GLuint Camera::d_skyProgram = 0;
GLuint Camera::d_pLightProgram = 0;
GLuint Camera::d_sLightProgram = 0;
GLuint Camera::d_sLightCSProgram = 0;
GLuint Camera::d_sLightRSMProgram = 0;
GLuint Camera::d_sLightRSMFluxProgram = 0;
GLuint Camera::d_reflQuadProgram = 0;
GLint Camera::d_skyProgramLocs[9];
std::unordered_map<string, GLuint> Camera::fetchTextures = std::unordered_map<string, GLuint>();
std::vector<string> Camera::fetchTexturesUpdated = std::vector<string>();
const string Camera::_gbufferNames[4] = {"Diffuse", "Normal", "Specular-Gloss", "Emission"};

GLuint Camera::DoFetchTexture(string s) {
	/*
	if (s == "") {
		ushort a = 0;
		s = "temp_fetch_" + a;
		while (fetchTextures[s] != 0)
			s = "temp_fetch_" + (++a);
	}
	if (fetchTextures[s] == 0) {
		glGenTextures(1, &fetchTextures[s]);
		glBindTexture(GL_TEXTURE_2D, fetchTextures[s]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Display::width, Display::height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	*/
	return 0;
}

void Camera::Render(RenderTexture* target, renderFunc func) {
	active = this;
	uint t_w = (uint)roundf((target? target->width : Display::width) * quality);
	uint t_h = (uint)roundf((target? target->height : Display::height) * quality);
	if ((d_w != t_w) || (d_h != t_h)) {
		if (!!d_fbo) {
			glDeleteFramebuffers(1, &d_fbo);
			glDeleteFramebuffers(1, &d_colfbo);
			glDeleteFramebuffers(NUM_EXTRA_TEXS, d_tfbo);
			glDeleteTextures(3, d_texs + 1);
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
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_colfbo);
		//glClearBufferfv(GL_COLOR, 0, zero);
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_tfbo[1]);
		//glClearBufferfv(GL_COLOR, 0, zero);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d_fbo);
		glClearBufferfv(GL_COLOR, 0, zero);
		glClearBufferfv(GL_DEPTH, 0, &one);
		glClearBufferfv(GL_COLOR, 1, zero);
		glClearBufferfv(GL_COLOR, 2, zero);
		glClearBufferfv(GL_COLOR, 3, zero);
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
		DrawSceneObjectsOpaque(Scene::active->objects);
		//MVP::Switch(false);
		//MVP::Clear();

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
	//glClearBufferfv(GL_COLOR, 0, zero);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	//RenderLights();
	
	if (onBlit) onBlit();
	
	if (useGBuffer2 && applyGBuffer2) {
		d_w = _d_w;
		d_h = _d_h;
		d_fbo = _d_fbo;
		memcpy(d_texs, _d_texs, 4 * sizeof(GLuint));
		d_depthTex = _d_depthTex;
		d_idTex = _d_texs[0];
	}

	Scene::dirty = false;
	//_ApplyEmission(d_fbo, d_texs, (float)Display::width, (float)Display::height, 0);
//#endif
}

void Camera::_RenderSky(Mat4x4 ip, GLuint d_texs[], GLuint d_depthTex, float w, float h) {
	if (Scene::active->settings.sky == nullptr || !Scene::active->settings.sky->loaded) return;
	if (d_skyProgram == 0) {
		Debug::Error("SkyLightPass", "Fatal: Shader not initialized!");
		abort();
	}
	GLint _IP = d_skyProgramLocs[0];
	GLint diffLoc = d_skyProgramLocs[1];
	GLint specLoc = d_skyProgramLocs[2];
	GLint normLoc = d_skyProgramLocs[3];
	GLint depthLoc = d_skyProgramLocs[4];
	GLint skyLoc = d_skyProgramLocs[5];
	GLint skyStrLoc = d_skyProgramLocs[6];
	GLint scrSzLoc = d_skyProgramLocs[7];
	GLint skyBrtLoc = d_skyProgramLocs[8];

	glBindVertexArray(emptyVao);
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, fullscreenVerts);
	glUseProgram(d_skyProgram);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, fullscreenVerts);

	glUniformMatrix4fv(_IP, 1, GL_FALSE, glm::value_ptr(ip));

	glUniform2f(scrSzLoc, w, h);
	glUniform1i(diffLoc, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_texs[0]);
	glUniform1i(specLoc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glUniform1i(normLoc, 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d_texs[2]);
	glUniform1i(depthLoc, 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform1i(skyLoc, 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, Scene::active->settings.sky->pointer);
	glUniform1f(skyStrLoc, Scene::active->settings.skyStrength);
	glUniform1f(skyBrtLoc, Scene::active->settings.skyBrightness);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, fullscreenIndices);

	//glDisableVertexAttribArray(0);
	glUseProgram(0);
	//glDisableClientState(GL_VERTEX_ARRAY);
	glBindVertexArray(0);
}

uint Camera::GetIdAt(uint x, uint y) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	float pixel;
	x = uint(x * quality);
	y = uint(y * quality);
	glReadPixels(x, d_h - y, 1, 1, GL_RED, GL_FLOAT, &pixel);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	return (uint)floor(pixel);
}


void Light::ScanParams() {
	paramLocs_Point.clear();
#define PBSL paramLocs_Point.push_back(glGetUniformLocation(Camera::d_pLightProgram,
	PBSL "_IP"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "inSpec"));
	PBSL "inDepth"));
	PBSL "screenSize"));
	PBSL "lightPos"));
	PBSL "lightDir"));
	PBSL "lightColor"));
	PBSL "lightStrength"));
	PBSL "lightMinDist"));
	PBSL "lightMaxDist"));
	PBSL "lightCookie"));
	PBSL "lightCookieStrength"));
	PBSL "lightDepth"));
	PBSL "lightDepthBias"));
	PBSL "lightDepthStrength"));
	PBSL "lightFalloffType"));
	PBSL "useHsvMap"));
	PBSL "hsvMap"));
#undef PBSL
	paramLocs_Spot.clear();
#define PBSL paramLocs_Spot.push_back(glGetUniformLocation(Camera::d_sLightProgram,
	PBSL "_IP"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "inSpec"));
	PBSL "inDepth"));
	PBSL "screenSize"));
	PBSL "lightPos"));
	PBSL "lightDir"));
	PBSL "lightColor"));
	PBSL "lightStrength"));
	PBSL "lightCosAngle"));
	PBSL "lightMinDist"));
	PBSL "lightMaxDist"));
	PBSL "lightDepth"));
	PBSL "lightDepthBias"));
	PBSL "lightDepthStrength"));
	PBSL "lightCookie"));
	PBSL "lightCookieStrength"));
	PBSL "lightIsSquare"));
	PBSL "_LD"));
	PBSL "lightContShad"));
	PBSL "lightContShadStrength"));
	PBSL "inEmi"));
	PBSL "lightFalloffType"));
	PBSL "useHsvMap"));
	PBSL "hsvMap"));
#undef PBSL
	paramLocs_SpotCS.clear();
#define PBSL paramLocs_SpotCS.push_back(glGetUniformLocation(Camera::d_sLightCSProgram,
	PBSL "_P"));
	PBSL "screenSize"));
	PBSL "inDepth"));
	PBSL "sampleCount"));
	PBSL "sampleLength"));
	PBSL "lightPos"));
#undef PBSL
	paramLocs_SpotFluxer.clear();
#define PBSL paramLocs_SpotFluxer.push_back(glGetUniformLocation(Camera::d_sLightRSMFluxProgram,
	PBSL "screenSize"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "lightDir"));
	PBSL "lightColor"));
	PBSL "lightStrength"));
	PBSL "lightIsSquare"));
#undef PBSL
	paramLocs_SpotRSM.clear();
	paramLocs_SpotRSM.push_back((GLint)glGetUniformBlockIndex(Camera::d_sLightRSMProgram, "SampleBuffer"));
#define PBSL paramLocs_SpotRSM.push_back(glGetUniformLocation(Camera::d_sLightRSMProgram,
	PBSL "_IP"));
	PBSL "_LP"));
	PBSL "_ILP"));
	PBSL "screenSize"));
	PBSL "inColor"));
	PBSL "inNormal"));
	PBSL "inSpec"));
	PBSL "inDepth"));
	PBSL "lightFlux"));
	PBSL "lightNormal"));
	PBSL "lightDepth"));
	PBSL "bufferRadius"));
#undef PBSL
}

void Camera::_ApplyEmission(GLuint d_fbo, GLuint d_texs[], float w, float h, GLuint targetFbo) {
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_EMISSION_AO);
	glBlitFramebuffer(0, 0, (int)w, (int)h, 0, 0, (int)w, (int)h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}


void Camera::_DoDrawLight_Point(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w, float h, GLuint tar) {
	if (l->maxDist <= 0 || l->angle <= 0 || l->intensity <= 0) return;
	if (l->minDist <= 0) l->minDist = 0.00001f;

	if (d_pLightProgram == 0) {
		Debug::Error("SpotLightPass", "Fatal: Shader not initialized!");
		abort();
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
#define sloc l->paramLocs_Point
	glBindVertexArray(Camera::emptyVao);
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, fullscreenVerts);
	glUseProgram(d_pLightProgram);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, fullscreenVerts);

	glUniformMatrix4fv(sloc[0], 1, GL_FALSE, glm::value_ptr(ip));

	glUniform1i(sloc[1], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_texs[0]);
	glUniform1i(sloc[2], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glUniform1i(sloc[3], 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d_texs[2]);
	glUniform1i(sloc[4], 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform2f(sloc[5], w, h);
	Vec3 wpos = l->object->transform.position();
	glUniform3f(sloc[6], wpos.x, wpos.y, wpos.z);
	Vec3 dir = l->object->transform.forward();
	glUniform3f(sloc[7], dir.x, dir.y, dir.z);
	glUniform3f(sloc[8], l->color.x, l->color.y, l->color.z);
	glUniform1f(sloc[9], l->intensity);
	glUniform1f(sloc[10], l->minDist);
	glUniform1f(sloc[11], l->maxDist);
	/*
	if (l->cookie) {
		glUniform1i(sloc[16], 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, l->cookie->pointer);
	}
	glUniform1f(sloc[17], l->cookie ? l->cookieStrength : 0);
	*/
	glUniform1i(sloc[17], (int)l->falloff);

	glUniform1f(sloc[18], l->hsvMap? 1.0f : 0.0f);
	if (l->hsvMap) {
		glUniform1i(sloc[19], 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, l->hsvMap->pointer);
	}
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, fullscreenIndices);

	glBindVertexArray(0);
	//glDisableVertexAttribArray(0);
	glUseProgram(0);
	//glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DoDrawLight_Spot_Contact(Light* l, Mat4x4& p, GLuint d_depthTex, float w, float h, GLuint src, GLuint tar) {
	if (d_sLightCSProgram == 0) {
		Debug::Error("SpotLightCSPass", "Fatal: Shader not initialized!");
		abort();
	}

	//glDisable(GL_BLEND);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);

	float zero[] = { 0, 0, 0, 0 };
	glClearBufferfv(GL_COLOR, 0, zero);

#define sloc l->paramLocs_SpotCS
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, fullscreenVerts);
	glBindVertexArray(emptyVao);
	glUseProgram(d_sLightCSProgram);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, fullscreenVerts);

	glUniformMatrix4fv(sloc[0], 1, GL_FALSE, glm::value_ptr(p));

	glUniform2f(sloc[1], w, h);
	glUniform1i(sloc[2], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform1i(sloc[3], (GLint)l->contactShadowSamples);
	glUniform1f(sloc[4], l->contactShadowDistance);
	Vec3 wpos = l->object->transform.position();
	Vec4 wpos2 = p*Vec4(wpos.x, wpos.y, wpos.z, 1);
	wpos2 /= wpos2.w;
	glUniform3f(sloc[5], wpos2.x, wpos2.y, wpos2.z);
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, fullscreenIndices);

	//glEnable(GL_BLEND);
	//glDisableVertexAttribArray(0);
	glUseProgram(0);
	//glDisableClientState(GL_VERTEX_ARRAY);
	glBindVertexArray(0);
}

void Camera::_DoDrawLight_Spot(Light* l, Mat4x4& ip, GLuint d_fbo, GLuint d_texs[], GLuint d_depthTex, GLuint ctar, GLuint c_tex, float w, float h, GLuint tar) {
	if (l->maxDist <= 0 || l->angle <= 0 || l->intensity <= 0) return;
	if (l->minDist <= 0) l->minDist = 0.00001f;
	l->CalcShadowMatrix();
	if (l->drawShadow) {
		l->DrawShadowMap(tar);
		glViewport(0, 0, (int)w, (int)h); //shadow map modifies viewport
	}
	Mat4x4 lp = MVP::projection();
	
	if (d_sLightProgram == 0) {
		Debug::Error("SpotLightPass", "Fatal: Shader not initialized!");
		abort();
	}

	/*
	glBindFramebuffer(GL_READ_FRAMEBUFFER, l->_fluxFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, 512, 512, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	return;
	*/

	if (l->drawShadow && l->contactShadows) {
		auto iip = glm::inverse(ip);
		_DoDrawLight_Spot_Contact(l, iip, d_depthTex, w, h, d_fbo, ctar);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
#define sloc l->paramLocs_Spot
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, fullscreenVerts);
	glBindVertexArray(emptyVao);
	glUseProgram(d_sLightProgram);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, fullscreenVerts);

	glUniformMatrix4fv(sloc[0], 1, GL_FALSE, glm::value_ptr(ip));

	glUniform1i(sloc[1], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, d_texs[0]);
	glUniform1i(sloc[2], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, d_texs[1]);
	glUniform1i(sloc[3], 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, d_texs[2]);
	glUniform1i(sloc[4], 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, d_depthTex);
	glUniform2f(sloc[5], w, h);
	Vec3 wpos = l->object->transform.position();
	glUniform3f(sloc[6], wpos.x, wpos.y, wpos.z);
	Vec3 dir = l->object->transform.forward();
	glUniform3f(sloc[7], dir.x, dir.y, dir.z);
	glUniform3f(sloc[8], l->color.x, l->color.y, l->color.z);
	glUniform1f(sloc[9], l->intensity);
	glUniform1f(sloc[10], cos(deg2rad*0.5f*l->angle));
	glUniform1f(sloc[11], l->minDist);
	glUniform1f(sloc[12], l->maxDist);
	if (l->drawShadow) {
		glUniform1i(sloc[13], 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, l->_shadowMap);
		glUniform1f(sloc[14], l->shadowBias);
	}
	glUniform1f(sloc[15], l->drawShadow? l->shadowStrength : 0);
	if (l->cookie) {
		glUniform1i(sloc[16], 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, l->cookie->pointer);
	}
	glUniform1f(sloc[17], l->cookie ? l->cookieStrength : 0);
	glUniform1f(sloc[18], l->square ? 1.0f : 0.0f);
	glUniformMatrix4fv(sloc[19], 1, GL_FALSE, glm::value_ptr(lp));

	if (l->drawShadow && l->contactShadows) {
		glUniform1i(sloc[20], 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, c_tex);
	}
	glUniform1f(sloc[21], (l->drawShadow && l->contactShadows) ? 1.0f : 0.0f);
	glUniform1i(sloc[22], 7);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, d_texs[3]);

	glUniform1f(sloc[24], l->hsvMap ? 1.0f : 0.0f);
	if (l->hsvMap) {
		glUniform1i(sloc[25], 8);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, l->hsvMap->pointer);
	}
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, fullscreenIndices);

	if (Scene::active->settings.GIType == GITYPE_RSM && Scene::active->settings.rsmRadius > 0)
		l->DrawRSM(ip, lp, w, h, d_texs, d_depthTex);

	//glDisableVertexAttribArray(0);
	glUseProgram(0);
	glBindVertexArray(0);
	//glDisableClientState(GL_VERTEX_ARRAY);
}

void Camera::_DrawLights(std::vector<pSceneObject>& oo, Mat4x4& ip, GLuint targetFbo) {
	for (auto& o : oo) {
		if (!o->active)
			continue;
		for (auto c : o->_components) {
			if (c->componentType == COMP_LHT) {
				Light* l = (Light*)c.get();
				switch (l->lightType) {
				case LIGHTTYPE_POINT:
					_DoDrawLight_Point(l, ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
					break;
				case LIGHTTYPE_SPOT:
					_DoDrawLight_Spot(l, ip, d_fbo, d_texs, d_depthTex, 0, 0, (float)Display::width, (float)Display::height, targetFbo);
					break;
				default:
					break;
				}
			}
		}

		_DrawLights(o->children, ip);
	}
}

void Camera::RenderLights(GLuint targetFbo) {
	/*
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat matrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	Mat4x4 mat = glm::inverse(Mat4x4(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11], matrix[12], matrix[13], matrix[14], matrix[15]));

	//clear alpha here
	//std::vector<ReflectionProbe*> probes;
	//_RenderProbesMask(Scene::active->objects, mat, probes);
	//_RenderProbes(probes, mat);
	_RenderSky(mat, d_texs, d_depthTex);
	_DrawLights(Scene::active->objects, mat, targetFbo);
	/*/


	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFbo);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, targetFbo);

	//_ApplyEmission(d_fbo, d_texs, (float)Display::width, (float)Display::height, targetFbo);
	Mat4x4 mat = glm::inverse(MVP::projection());
	//_RenderSky(mat, d_texs, d_depthTex); //wont work well on ortho, will it?
	//glViewport(v.r, Display::height - v.g - v.a, v.b, v.a - EB_HEADER_SIZE - 2);
	_DrawLights(Scene::active->objects, mat);
	//glViewport(0, 0, Display::width, Display::height);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//*/
}

void Camera::DumpBuffers() {
	glViewport(0, 0, Display::width, Display::height);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d_fbo);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLsizei HalfWidth = (GLsizei)(Display::width / 2.0f);
	GLsizei HalfHeight = (GLsizei)(Display::height / 2.0f);
	
	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_DIFFUSE);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, HalfHeight, HalfWidth, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	
	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_NORMAL);
	glBlitFramebuffer(0, 0, Display::width, Display::height, HalfWidth, HalfHeight, Display::width, Display::height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_SPEC_GLOSS);
	glBlitFramebuffer(0, 0, Display::width, Display::height, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	
	glReadBuffer(GL_COLOR_ATTACHMENT0 + GBUFFER_EMISSION_AO);
	glBlitFramebuffer(0, 0, Display::width, Display::height, HalfWidth, 0, Display::width, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

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
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	// depth
	glBindTexture(GL_TEXTURE_2D, _shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMap, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
	/*
	//depth cube
	glGenFramebuffers(6, _shadowCubeFbos);
	glGenTextures(1, &_shadowCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowCubeMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (uint a = 0; a < 6; a++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, 0, GL_DEPTH_COMPONENT32F, 512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	for (uint a = 0; a < 6; a++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowCubeFbos[a]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, _shadowCubeMap, 0);

		Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("ShadowMap", "FB cube error: " + std::to_string(Status));
			abort();
		}
	}

	Debug::Message("ShadowMap", "FB cube ok");

	InitRSM();
	*/

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Light::InitRSM() {
	return;
	glGenFramebuffers(1, &_rsmFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _rsmFbo);

	glGenTextures(1, &_rsmTex);

	glBindTexture(GL_TEXTURE_2D, _rsmTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _rsmTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	GLint Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("GI_RsmMap", "FB error:" + std::to_string(Status));
		abort();
	}
	else {
		Debug::Message("GI_RsmMap", "FB ok");
	}

	glGenFramebuffers(1, &_fluxFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fluxFbo);

	glGenTextures(1, &_fluxTex);

	glBindTexture(GL_TEXTURE_2D, _fluxTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fluxTex, 0);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum DrawBuffers2[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers2);

	Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		Debug::Error("GI_FluxMap", "FB error:" + std::to_string(Status));
		abort();
	}
	else {
		Debug::Message("GI_FluxMap", "FB ok");
	}

	std::default_random_engine generator;
	std::normal_distribution<double> distri(5, 2);
	for (uint a = 0; a < 1024; a++) {
		_rsmBuffer.xPos[a] = (float)(distri(generator)*10.0);
		_rsmBuffer.yPos[a] = (float)(distri(generator)*10.0);
		float sz = sqrt(pow(_rsmBuffer.xPos[a], 2) + pow(_rsmBuffer.yPos[a], 2))*0.001f;
		_rsmBuffer.size[a] = 0.5f;// sz;
	}
	glGenBuffers(1, &_rsmUBO);
	//glBindBufferBase(GL_UNIFORM_BUFFER, BUFFERLOC_LIGHT_RSM, _rsmUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, _rsmUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(_rsmBuffer), &_rsmBuffer, GL_STATIC_DRAW);
	//glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//glBindBuffer(GL_UNIFORM_BUFFER, gbo);
	//GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	//memcpy(p, &_rsmBuffer, sizeof(_rsmBuffer));
	//glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(_rsmBuffer), &_rsmBuffer, GL_WRITE_ONLY);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Light::DrawShadowMap(GLuint tar) {
	if (_shadowFbo == 0) {
		Debug::Error("RenderShadow", "Fatal: Fbo not set up!");
		abort();
	}
	glViewport(0, 0, 1024, 1024);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tar);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shadowFbo);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	//glClearColor(1, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT);
	//glClearDepth(1);
	//glClear(GL_DEPTH_BUFFER_BIT);
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	//switch (_lightType) {
	//case LIGHTTYPE_SPOT:
	//case LIGHTTYPE_DIRECTIONAL:
	//CalcShadowMatrix();
	MVP::Switch(false);
	MVP::Clear();
	Camera::DrawSceneObjectsOpaque(Scene::active->objects);
	//	break;           
	//}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);

	if (Scene::active->settings.GIType == GITYPE_RSM) {
		BlitRSMFlux();
	}

	glEnable(GL_BLEND);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, tar);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar);
	//glViewport(0, 0, Display::width, Display::height);
}

void Light::BlitRSMFlux() {
	glViewport(0, 0, 512, 512);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _shadowFbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fluxFbo);
#define sloc paramLocs_SpotFluxer
	glBindVertexArray(Camera::emptyVao);
	//glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_FLOAT, 0, Camera::fullscreenVerts);
	glUseProgram(Camera::d_sLightRSMFluxProgram);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_TRUE, 0, Camera::fullscreenVerts);

	glUniform2f(sloc[0], 512.0f, 512.0f);
	glUniform1i(sloc[1], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _shadowGITexs[0]);
	glUniform1i(sloc[2], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _shadowGITexs[1]);
	Vec3 fwd = object->transform.forward();
	glUniform3f(sloc[3], fwd.x, fwd.y, fwd.z);
	glUniform4f(sloc[4], color.r, color.g, color.b, color.a);
	glUniform1f(sloc[5], intensity);
	glUniform1f(sloc[6], square? 1.0f : 0.0f);
#undef sloc
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::fullscreenIndices);

	//glDisableVertexAttribArray(0);
	glUseProgram(0);
	//glDisableClientState(GL_VERTEX_ARRAY);
}

void Light::DrawRSM(Mat4x4& ip, Mat4x4& lp, float w, float h, GLuint gtexs[], GLuint gdepth) {
	glUseProgram(Camera::d_sLightRSMProgram);
#define sloc paramLocs_SpotRSM
	//glUniformBlockBinding(Camera::d_sLightRSMProgram, (GLuint)sloc[0], _rsmUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, (GLuint)sloc[0], _rsmUBO);
	glUniformMatrix4fv(sloc[1], 1, GL_FALSE, glm::value_ptr(ip));
	glUniformMatrix4fv(sloc[2], 1, GL_FALSE, glm::value_ptr(lp));
	glUniformMatrix4fv(sloc[3], 1, GL_FALSE, glm::value_ptr(glm::inverse(lp)));
	glUniform2f(sloc[4], w, h);
	glUniform1i(sloc[5], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gtexs[0]);
	glUniform1i(sloc[6], 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gtexs[1]);
	glUniform1i(sloc[7], 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gtexs[2]);
	glUniform1i(sloc[8], 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gdepth);
	glUniform1i(sloc[9], 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, _fluxTex);
	glUniform1i(sloc[10], 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, _shadowGITexs[1]);
	glUniform1i(sloc[11], 6);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, _shadowMap);
	glUniform1f(sloc[12], Scene::active->settings.rsmRadius);
#undef sloc
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, Camera::fullscreenIndices);
}

CubeMap::CubeMap(ushort size, bool mips, GLenum type, byte dataSize, GLenum format, GLenum dataType) : size(size), AssetObject(ASSETTYPE_HDRI), loaded(false) {
	if (size != 64 && size != 128 && size != 256 && size != 512 && size != 1024 && size != 2048) {
		Debug::Error("Cubemap", "CubeMaps must be POT-sized between 64 and 2048! (" + std::to_string(size) + ")");
		abort();
	}
	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pointer);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mips? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (uint a = 0; a < 6; a++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, 0, type, size, size, 0, format, dataType, NULL);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	loaded = true;
}

void CubeMap::_RenderCube(Vec3 pos, Vec3 xdir, GLuint fbos[], uint size, GLuint shader) {
	glViewport(0, 0, size, size);
	glUseProgram(shader);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	glDisable(GL_BLEND);
	
	MVP::Switch(false);
	MVP::Clear();
	MVP::Switch(true);
	MVP::Clear();
	MVP::Mul(glm::perspectiveFov(PI, (float)size, (float)size, 0.001f, 1000.0f));
	MVP::Scale(1, -1, -1);
	//MVP::Mul(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI))));
	MVP::Translate(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[4]);
	MVP::Translate(-pos.x, -pos.y, -pos.z);
	MVP::Mul(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI)));
	MVP::Translate(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[0]);
	MVP::Translate(-pos.x, -pos.y, -pos.z);
	MVP::Mul(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI)));
	MVP::Translate(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[5]);
	MVP::Translate(-pos.x, -pos.y, -pos.z);
	MVP::Mul(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 1, 0), PI)));
	MVP::Translate(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[1]);
	MVP::Translate(-pos.x, -pos.y, -pos.z);
	MVP::Mul(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 0, 1), PI)));
	MVP::Translate(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[2]);
	MVP::Translate(-pos.x, -pos.y, -pos.z);
	MVP::Mul(QuatFunc::ToMatrix(QuatFunc::FromAxisAngle(Vec3(0, 0, 1), 2*PI)));
	MVP::Translate(pos.x, pos.y, pos.z);
	_DoRenderCubeFace(fbos[3]);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void CubeMap::_DoRenderCubeFace(GLuint fbo) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
	float zero[] = { 0,0,0,0 };
	float one = 1;
	glClearBufferfv(GL_COLOR, 0, zero);
	glClearBufferfv(GL_DEPTH, 0, &one);
	glClearBufferfv(GL_COLOR, 1, zero);
	glClearBufferfv(GL_COLOR, 2, zero);
	glClearBufferfv(GL_COLOR, 3, zero);
	MVP::Switch(false);
	Camera::DrawSceneObjectsOpaque(Scene::active->objects);
}

RenderCubeMap::RenderCubeMap() : map(256, true) {
	for (uint a = 0; a < 6; a++) {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[a][0]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + a, map.pointer, 0);

		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
		GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (Status != GL_FRAMEBUFFER_COMPLETE) {
			Debug::Error("CubeMap", "FBO error: " + std::to_string(Status));
			abort();

		}
	}
}