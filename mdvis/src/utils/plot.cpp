#include "plot.h"
#include "ui/icons.h"

string to_string_scientific(float f) {
	std::stringstream strm;
	strm.precision(3);
	strm << std::scientific << f;
	return strm.str();
}

float plt::remapdata::Eval(float f) {
	return f;
}

void plt::plot(float x, float y, float w, float h, float* dx, float* dy, uint cnt, Font* font, uint sz, Vec4 col) {
	Engine::DrawQuad(x, y, w, h, black());
	x += 2;
	y += 2;
	w -= 4;
	h -= 4;
	Engine::DrawQuad(x, y, w, h, white());

	Vec3* poss = new Vec3[cnt];
	float x1 = *dx, x2 = *dx, y1 = *dy, y2 = *dy;
	for (uint i = 0; i < cnt; i++) {
		if (x1 > dx[i]) x1 = dx[i];
		if (x2 < dx[i]) x2 = dx[i];
		if (y1 > dy[i]) y1 = dy[i];
		if (y2 < dy[i]) y2 = dy[i];
	}
	for (uint i = 0; i < cnt; i++) {
		poss[i].x = ((x + ((dx[i] - x1) / (x2 - x1)) * w) / Display::width) * 2 - 1;
		poss[i].y = 1 - ((y + (1 - (dy[i] - y1) / (y2 - y1)) * h) / Display::height) * 2;
	}
	UI::SetVao(cnt, poss);

	glUseProgram(Engine::defProgram);
	glUniform4f(Engine::defColLoc, 0.0f, 0.0f, 0.0f, 1.0f);
	glBindVertexArray(UI::_vao);
	glDrawArrays(GL_LINE_STRIP, 0, cnt);
	glBindVertexArray(0);
	glUseProgram(0);

	delete[](poss);

	if (font) {
		font->Align(ALIGN_TOPLEFT);
		UI::Label(x, y + h + 2, (float)sz, to_string_scientific(x1), col);
		font->Align(ALIGN_TOPRIGHT);
		UI::Label(x + w, y + h + 2, (float)sz, to_string_scientific(x2), col);
		Engine::RotateUI(-90.0f, Vec2(x, y + w));
		font->Align(ALIGN_TOPLEFT);
		UI::Label(x, y + h - 4 - sz, (float)sz, to_string_scientific(y1), col);
		font->Align(ALIGN_TOPRIGHT);
		UI::Label(x + w, y + h - 4 - sz, (float)sz, to_string_scientific(y2), col);
		Engine::ResetUIMatrix();
		font->Align(ALIGN_TOPLEFT);
	}
}

void plt::remap(float x, float y, float w, float h, plt::remapdata& data) {
	auto ps = data.pts.size();
	for (int a = 0; a < 3; a++) {
		if (Engine::Button(x + w - 21 * (2-a) - 20, y, 20, 16, white(1, 0.4f)) == MOUSE_RELEASE) {
			data.type = a;
		}
	}
	UI::Label(x, y + h - 17, 12, "anchor", white());
	Engine::DrawQuad(x, y + 17, w, h - 34, white(1, 0.1f));
	x += 2;
	y += 19;
	w -= 4;
	h -= 38;
	Engine::DrawQuad(x, y, w, h, white(1, 0.3f));
	
	if (!!ps){
		float y0, x1;
		y0 = y + (1-data.pts[0].y) * h;
		if (ps > 1) {
			switch (data.type) {
			case 0:
				x1 = x + data.pts[1].x * w;
				Engine::DrawLine(Vec2(x, y0), Vec2(x1, y0), white(), 1);
				Engine::DrawLine(Vec2(x1, y0), Vec2(x1, y + (1-data.pts[1].y) * h), white(), 1);
				for (size_t a = 1; a < ps - 1; a++) {
					y0 = y + (1-data.pts[a].y) * h;
					x1 = x + data.pts[a+1].x * w;
					Engine::DrawLine(Vec2(x + data.pts[a].x * w, y0), Vec2(x1, y0), white(), 1);
					Engine::DrawLine(Vec2(x1, y0), Vec2(x1, y + (1-data.pts[a+1].y) * h), white(), 1);
				}
				y0 = y + (1-data.pts[ps-1].y) * h;
				Engine::DrawLine(Vec2(x + data.pts[ps-1].x * w, y0), Vec2(x + w, y0), white(), 1);
				break;
			case 1:
				Engine::DrawLine(Vec2(x, y0), Vec2(x + data.pts[0].x * w, y0), white(), 1);
				for (size_t a = 0; a < ps - 1; a++) {
					y0 = y + (1-data.pts[a].y) * h;
					Engine::DrawLine(Vec2(x + data.pts[a].x * w, y + (1-data.pts[a].y) * h), 
						Vec2(x + data.pts[a+1].x * w, y + (1-data.pts[a+1].y) * h), white(), 1);
				}
				y0 = y + (1-data.pts[ps-1].y) * h;
				Engine::DrawLine(Vec2(x + data.pts[ps-1].x * w, y0), Vec2(x + w, y0), white(), 1);
				break;
			}
		}
		else Engine::DrawLine(Vec2(x, y0), Vec2(x + w, y0), white(), 1);
	}
	else Engine::DrawLine(Vec2(x, y + h), Vec2(x + w, y), white(), 1);
	
	if (Input::mouse0State == 1) data.selId = -1;
	for (size_t i = 0; i < ps; i++) {
		auto& pt = data.pts[i];
		if (Engine::Button(x + w*pt.x - 4, y + h*(1-pt.y) - 4, 8, 8, Icons::circle, (i == data.selId)? yellow() : white()) == MOUSE_PRESS) {
			data.selId = i;
		}
	}
	if (data.selId > -1 && Input::mouse0) {
		data.pts[data.selId] = Vec2(Clamp((Input::mousePos.x - x)/w, 0.0f, 1.0f), 
			1 - Clamp((Input::mousePos.y - y)/h, 0.0f, 1.0f));
		
		Vec2 me = data.pts[data.selId];
		std::sort(data.pts.begin(), data.pts.end(), [](Vec2 v1, Vec2 v2) { return v1.x < v2.x; });
		data.selId = std::find(data.pts.begin(), data.pts.end(), me) - data.pts.begin();
	}
}