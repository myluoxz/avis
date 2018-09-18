#include "Engine.h"
#include "hdr.h"
#include <iomanip>
#include <algorithm>
#include <climits>
#include <ctime>
#include <cstdarg>
#include "res/shddata.h"

#ifdef PLATFORM_WIN
#include <shellapi.h>
#else
void fopen_s(FILE** f, const char* c, const char* m) {
	*f = fopen(c, m);
}
#endif

float Dw(float f) {
	return (f / Display::width);
}
float Dh(float f) {
	return (f / Display::height);
}
Vec3 Ds(Vec3 v) {
	return Vec3(Dw(v.x) * 2 - 1, 1 - Dh(v.y) * 2, 1);
}
Vec2 Ds2(Vec2 v) {
	return Vec2(Dw(v.x) * 2 - 1, 1 - Dh(v.y) * 2);
}


Texture* Engine::fallbackTex = nullptr;

GLuint Engine::defProgram = 0;
GLuint Engine::defProgramW = 0;
GLuint Engine::unlitProgram = 0;
GLuint Engine::unlitProgramA = 0;
GLuint Engine::unlitProgramC = 0;
GLuint Engine::skyProgram = 0;
GLint Engine::defColLoc = 0;
GLint Engine::defWColLoc = 0;
GLint Engine::defWMVPLoc = 0;

PROGDEF(Engine::lineWProg)

std::vector<Rect> Engine::stencilRects;
Rect* Engine::stencilRect = nullptr;

GLuint Engine::quadBuffer;

void Engine::Init() {
	fallbackTex = new Texture(IO::path + "fallback.bmp");
	if (!fallbackTex->loaded)
		std::cout << "cannot load fallback texture!" << std::endl;
	Engine::_mainThreadId = std::this_thread::get_id();
	

	InitShaders();

	Input::RegisterCallbacks();
	MVP::Reset();
	Camera::InitShaders();

	uint d[] = {0, 2, 1, 2, 3, 1};

	glGenBuffers(1, &quadBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint), d, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLint Engine::drawQuadLocs[] = { 0, 0, 0 };
GLint Engine::drawQuadLocsA[] = { 0, 0, 0 };
GLint Engine::drawQuadLocsC[] = { 0 };

void Engine::InitShaders() {
	GLuint vs;
	string err;
	Shader::LoadShader(GL_VERTEX_SHADER, glsl::coreVert, vs, &err);
	if (!vs) {
		 OHNO("Engine", "Cannot load coreVert shader! " + err);
	}

	unlitProgram = Shader::FromF(vs, glsl::coreFrag);
	unlitProgramA = Shader::FromF(vs, glsl::coreFrag2);
	unlitProgramC = Shader::FromF(vs, glsl::coreFrag3);
	defProgram = unlitProgramC;
	defProgramW = Shader::FromVF(glsl::coreVertW, glsl::coreFrag3);
	skyProgram = Shader::FromF(vs, glsl::coreFragSky);
	glDeleteShader(vs);

	drawQuadLocs[0] = glGetUniformLocation(unlitProgram, "sampler");
	drawQuadLocs[1] = glGetUniformLocation(unlitProgram, "col");
	drawQuadLocs[2] = glGetUniformLocation(unlitProgram, "level");

	drawQuadLocsA[0] = glGetUniformLocation(unlitProgramA, "sampler");
	drawQuadLocsA[1] = glGetUniformLocation(unlitProgramA, "col");
	drawQuadLocsA[2] = glGetUniformLocation(unlitProgramA, "level");

	drawQuadLocsC[0] = glGetUniformLocation(unlitProgramC, "col");

	defColLoc = drawQuadLocsC[0];
	defWColLoc = glGetUniformLocation(defProgramW, "col");
	defWMVPLoc = glGetUniformLocation(defProgramW, "MVP");

	lineWProg = Shader::FromVF(glsl::lineWVert, glsl::coreFrag3);
	int i = 0;
#define LC(nm) lineWProgLocs[i++] = glGetUniformLocation(lineWProg, #nm)
	LC(poss); LC(width); LC(MVP); LC(col);
#undef LC
}

std::thread::id Engine::_mainThreadId = std::thread::id();

void Engine::BeginStencil(float x, float y, float w, float h) {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glStencilFunc(GL_NEVER, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP); // draw 1s on test fail (always)
	glStencilMask(0xFF); // draw stencil pattern
	glClear(GL_STENCIL_BUFFER_BIT); // needs mask=0xFF
	UI::Quad(x, y, w, h, white());
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glStencilMask(0x0);
	glStencilFunc(GL_EQUAL, 1, 0xFF);

	if (!stencilRect) stencilRects.resize(1, Rect(x, y, w, h));
	stencilRect = new Rect(x, y, w, h);
}

void Engine::PushStencil(float x, float y, float w, float h) {
	if (!stencilRect) BeginStencil(x, y, w, h);
	else {
		stencilRects.push_back(Rect(x, y, w, h));
		Rect rt = stencilRects[0];
		for (size_t a = 1, ssz = stencilRects.size(); a < ssz; a++)
			rt = rt.Intersection(stencilRects[a]);
		delete(stencilRect);
		BeginStencil(rt.x, rt.y, rt.w, rt.h);
	}
}

void Engine::PopStencil() {
	stencilRects.pop_back();
	auto ssz = stencilRects.size();
	if (!!ssz) {
		Rect rt = stencilRects[0];
		for (size_t a = 1; a < ssz; a++)
			rt = rt.Intersection(stencilRects[a]);
		delete(stencilRect);
		BeginStencil(rt.x, rt.y, rt.w, rt.h);
	}
	else EndStencil();
}

void Engine::EndStencil() {
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);

	delete(stencilRect);
	stencilRects.clear();
	stencilRect = nullptr;
}

MOUSE_STATUS Engine::Button(float x, float y, float w, float h) {
	if (!UI::focused || (UI::_layer < UI::_layerMax)) return MOUSE_NONE;
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos)) return MOUSE_NONE;
	}
	return Rect(x, y, w, h).Inside(Input::mousePos) ? MOUSE_STATUS(MOUSE_HOVER_FLAG | Input::mouse0State) : MOUSE_NONE;
}
MOUSE_STATUS Engine::Button(float x, float y, float w, float h, Vec4 normalVec4) {
	return Button(x, y, w, h, normalVec4, Lerp(normalVec4, white(), 0.5f), Lerp(normalVec4, black(), 0.5f));
}
MOUSE_STATUS Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, string label, float labelSize, Vec4 labelVec4, bool labelCenter, Font* labelFont) {
	return Button(x, y, w, h, normalVec4, Lerp(normalVec4, white(), 0.5f), Lerp(normalVec4, black(), 0.5f), label, labelSize, labelFont, labelVec4, labelCenter);
}
MOUSE_STATUS Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4) {
	if (!UI::focused || (UI::_layer < UI::_layerMax) ||(Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos))) {
		UI::Quad(x, y, w, h, normalVec4);
		return MOUSE_NONE;
	}
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos))
			inside = false;
	}
	switch (Input::mouse0State) {
	case 0:
	case MOUSE_UP:
		UI::Quad(x, y, w, h, inside ? highlightVec4 : normalVec4);
		break;
	case MOUSE_DOWN:
	case MOUSE_HOLD:
		UI::Quad(x, y, w, h, inside ? pressVec4 : normalVec4);
		break;
	}
	return (inside && (UI::_layer == UI::_layerMax)) ? MOUSE_STATUS(MOUSE_HOVER_FLAG | Input::mouse0State) : MOUSE_NONE;
}
MOUSE_STATUS Engine::Button(float x, float y, float w, float h, Texture* texture, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, float uvx, float uvy, float uvw, float uvh) {
	if (!UI::focused || (UI::_layer < UI::_layerMax) || (Input::mouse0State != 0 && !Rect(x, y, w, h).Inside(Input::mouseDownPos))) {
		UI::Quad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, normalVec4, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh));
		return MOUSE_NONE;
	}
	bool inside = Rect(x, y, w, h).Inside(Input::mousePos);
	if (stencilRect) {
		if (!stencilRect->Intersection(Rect(x, y, w, h)).Inside(Input::mousePos))
			inside = false;
	}
	switch (Input::mouse0State) {
	case 0:
	case MOUSE_UP:
		UI::Quad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, inside ? highlightVec4 : normalVec4, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh));
		break;
	case MOUSE_DOWN:
	case MOUSE_HOLD:
		UI::Quad(x, y, w, h, (texture->loaded) ? texture->pointer : Engine::fallbackTex->pointer, inside ? pressVec4 : normalVec4, Vec2(uvx, 1 - uvy), Vec2(uvx + uvw, 1 - uvy), Vec2(uvx, 1 - uvy - uvh), Vec2(uvx + uvw, 1 - uvy - uvh));
		break;
	}
	return (inside && (UI::_layer == UI::_layerMax)) ? MOUSE_STATUS(MOUSE_HOVER_FLAG | Input::mouse0State) : MOUSE_NONE;
}
MOUSE_STATUS Engine::Button(float x, float y, float w, float h, Vec4 normalVec4, Vec4 highlightVec4, Vec4 pressVec4, string label, float labelSize, Font* labelFont, Vec4 labelVec4, bool labelCenter) {
	MOUSE_STATUS b = Button(x, y, w, h, normalVec4, highlightVec4, pressVec4);
	ALIGNMENT al = labelFont->alignment;
	labelFont->alignment = labelCenter? ALIGN_MIDCENTER : ALIGN_MIDLEFT;
	UI::Label(round(x + (labelCenter? w*0.5f : 2)), round(y + 0.4f*h), labelSize, label, labelVec4, -1.0f, labelFont);
	labelFont->alignment = al;
	return b;
}

bool Engine::Toggle(float x, float y, float s, Vec4 col, bool t) {
	byte b = Button(x, y, s, s, col);
	if (b == MOUSE_RELEASE)
		t = !t;
	return t;
}
bool Engine::Toggle(float x, float y, float s, Texture* texture, bool t, Vec4 col, ORIENTATION o) {
	byte b;
	if (o == ORIENT_NONE)
		b = Button(x, y, s, s, texture, col, Lerp(col, white(), 0.5f), Lerp(col, black(), 0.5f));
	else if (o == ORIENT_HORIZONTAL)
		b = Button(x, y, s, s, texture, col, Lerp(col, white(), 0.5f), Lerp(col, black(), 0.5f), t ? 0.5f : 0, 0, 0.5f, 1);
	else
		b = Button(x, y, s, s, texture, col, Lerp(col, white(), 0.5f), Lerp(col, black(), 0.5f), 0, t ? 0.5f : 0, 1, 0.5f);
	if (b == MOUSE_RELEASE)
		t = !t;
	return t;
}

float Engine::DrawSliderFill(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground) {
	UI::Quad(x, y, w, h, background);
	val = Clamp(val, min, max);
	float v = val, vv = (val - min)/(max-min);
	if (Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		if (Input::mouse0 && (UI::_layer == UI::_layerMax)) {
			vv = Clamp((Input::mousePos.x - (x+1)) / (w-2), 0.0f, 1.0f);
			v = vv*(max - min) + min;
			UI::Quad(x + 1, y + 1, (w - 2)*vv, h - 2, foreground*white(1, 0.4f));
			return v;
		}
	}
	else UI::Quad(x + 1, y + 1, (w - 2)*vv, h - 2, foreground*(Rect(x, y, w, h).Inside(Input::mousePos)? 1 : 0.8f));
	return v;
}
float Engine::DrawSliderFillY(float x, float y, float w, float h, float min, float max, float val, Vec4 background, Vec4 foreground) {
	UI::Quad(x, y, w, h, background);
	val = Clamp(val, min, max);
	float v = val, vv = (val - min) / (max - min);
	if (Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		if (Input::mouse0 && (UI::_layer == UI::_layerMax)) {
			vv = Clamp((Input::mousePos.y - (y + 1)) / (h - 2), 0.0f, 1.0f);
			v = vv*(max - min) + min;
			UI::Quad(x + 1, y + 1 + (h-2)*(1-vv), w - 2, (h - 2)*vv, foreground*white(1, 0.4f));
			return v;
		}
	}
	else UI::Quad(x + 1, y + 1 + (h - 2)*(1 - vv), w - 2, (h - 2)*vv, foreground*(Rect(x, y, w, h).Inside(Input::mousePos) ? 1 : 0.8f));
	return v;
}

Vec2 Engine::DrawSliderFill2D(float x, float y, float w, float h, Vec2 min, Vec2 max, Vec2 val, Vec4 background, Vec4 foreground) {
	UI::Quad(x, y, w, h, background);
	val = clamp(val, min, max);
	Vec2 v = val, vv = (val - min) / (max - min);
	if (Rect(x, y, w, h).Inside(Input::mouseDownPos)) {
		if (Input::mouse0 && (UI::_layer == UI::_layerMax)) {
			vv = Clamp<Vec2>((Input::mousePos - Vec2(x + 1, y + 1)) / Vec2(w - 2, h - 2), Vec2(0, 0), Vec2(1, 1));
			vv.y = 1 - vv.y;
			v = vv*(max - min) + min;
			UI::Quad(x + 1, y + 1 + (h - 2)*(1 - vv.y), (w - 2)*vv.x, (h - 2)*vv.y, foreground*white(1, 0.4f));
			return v;
		}
	}
	else UI::Quad(x + 1, y + 1 + (h - 2)*(1 - vv.y), (w - 2)*vv.x, (h - 2)*vv.y, foreground*(Rect(x, y, w, h).Inside(Input::mousePos) ? 1 : 0.8f));
	return v;
}

void Engine::DrawProgressBar(float x, float y, float w, float h, float progress, Vec4 background, Texture* foreground, Vec4 tint, int padding, byte clip) {
	UI::Quad(x, y, w, h, background);
	progress = Clamp(progress, 0.0f, 100.0f)*0.01f;
	float tx = (clip == 0) ? 1 : ((clip == 1) ? progress : w*progress / h);
	UI::Quad(x + padding, y + padding, w*progress - 2 * padding, h - 2 * padding, foreground->pointer, tint, Vec2(0, 1), Vec2(tx, 1), Vec2(0, 0), Vec2(tx, 0));
}

void Engine::RotateUI(float aa, Vec2 point) {
	float a = -aa * deg2rad;
	Display::uiMatrix = glm::mat3(1, 0, 0, 0, 1, 0, point.x, point.y, 1)*glm::mat3(cos(a), -sin(a), 0, sin(a), cos(a), 0, 0, 0, 1)*glm::mat3(1, 0, 0, 0, 1, 0, -point.x, -point.y, 1)*Display::uiMatrix;
	Display::uiMatrixIsI = false;
}
void Engine::ResetUIMatrix() {
	Display::uiMatrix = glm::mat3();
	Display::uiMatrixIsI = true;
}

void Engine::Sleep(uint ms) {
#ifdef PLATFORM_WIN
	::Sleep(ms);
#else
	usleep(ms * 1000);
#endif
}

void Engine::DrawCube(Vec3 pos, float dx, float dy, float dz, Vec4 Vec4) {
	Vec3 quadPoss[8];
	quadPoss[0] = pos + Vec3(-dx, -dy, -dz);
	quadPoss[1] = pos + Vec3(dx, -dy, -dz);
	quadPoss[2] = pos + Vec3(-dx, dy, -dz);
	quadPoss[3] = pos + Vec3(dx, dy, -dz);
	quadPoss[4] = pos + Vec3(-dx, -dy, dz);
	quadPoss[5] = pos + Vec3(dx, -dy, dz);
	quadPoss[6] = pos + Vec3(-dx, dy, dz);
	quadPoss[7] = pos + Vec3(dx, dy, dz);
	static const uint quadIndexes[36] = {
		0, 1, 2, 1, 3, 2,		4, 5, 6, 5, 7, 6, 
		0, 2, 4, 2, 6, 4,		1, 3, 5, 3, 7, 5, 
		0, 1, 4, 1, 5, 4,		2, 3, 6, 3, 7, 6 };

	UI::SetVao(8, quadPoss);

	glUseProgram(Engine::defProgram);
	glUniform4f(Engine::defColLoc, Vec4.r, Vec4.g, Vec4.b, Vec4.a * UI::alpha);

	glBindVertexArray(UI::_vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, &quadIndexes[0]);
	glBindVertexArray(0);
	glUseProgram(0);
}

void Engine::DrawLine(Vec2 v1, Vec2 v2, Vec4 col, float width) {
	DrawLine(Vec3(v1.x, v1.y, 1), Vec3(v2.x, v2.y, 1), col, width);
}
void Engine::DrawLine(Vec3 v1, Vec3 v2, Vec4 col, float width) {
	Vec3 quadPoss[2];
	quadPoss[0] = v1;
	quadPoss[1] = v2;
	for (int y = 0; y < 2; y++) {
		quadPoss[y] = Ds(Display::uiMatrix*quadPoss[y]);
	}
	UI::SetVao(2, quadPoss);

	glUseProgram(Engine::defProgram);
	glUniform4f(Engine::defColLoc, col.r, col.g, col.b, col.a * UI::alpha);
	glBindVertexArray(UI::_vao);
	glDrawArrays(GL_LINES, 0, 2);
	glBindVertexArray(0);
	glUseProgram(0);
}
void Engine::DrawLineW(Vec3 v1, Vec3 v2, Vec4 col, float width) {
	Vec3 pts[2]{ v1, v2 };
	DrawLinesW(pts, 2, col, width);
}

void Engine::DrawLinesW(Vec3* pts, int num, Vec4 col, float width) {
	UI::SetVao(num, pts);
	auto mvp = MVP::projection() * MVP::modelview();
	
	glUseProgram(lineWProg);
	glUniform1i(lineWProgLocs[0], 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, UI::_tvbo);
	glUniform2f(lineWProgLocs[1], width / Display::width, width / Display::height);
	glUniformMatrix4fv(lineWProgLocs[2], 1, GL_FALSE, glm::value_ptr(mvp));
	glUniform4f(lineWProgLocs[3], col.r, col.g, col.b, col.a);
	glBindVertexArray(Camera::emptyVao);
	glDrawArrays(GL_TRIANGLES, 0, 6 * (num-1));
	glBindVertexArray(0);
	glUseProgram(0);
}