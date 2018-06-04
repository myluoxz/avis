#include "../anweb.h"
#include "utils/plot.h"
#include "ui/icons.h"

Node_Plot::Node_Plot() : AnNode(new DmScript()) {
	//width = 200;
	canTile = true;
	inputR.resize(1);
	script->invars.push_back(std::pair<string, string>("array", "list(float)"));
}

void Node_Plot::Draw() {
	auto cnt = 1;
	this->pos = pos;
	Engine::DrawQuad(pos.x, pos.y, width, 16, white(selected ? 1.0f : 0.7f, 0.35f));
	if (Engine::Button(pos.x, pos.y, 16, 16, expanded ? Icons::expand : Icons::collapse, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) expanded = !expanded;
	UI::Label(pos.x + 20, pos.y + 1, 12, "Plot list(float)", font, white());
	DrawToolbar();
	if (expanded) {
		Engine::DrawQuad(pos.x, pos.y + 16, width, 3 + 17 * cnt + width, white(0.7f, 0.25f));
		float y = pos.y + 18;
		//for (uint i = 0; i < 1; i++, y += 17) {
		const uint i = 0;
		if (!AnWeb::selConnNode || (AnWeb::selConnIdIsOut && AnWeb::selConnNode != this)) {
			if (Engine::Button(pos.x - 5, y + 3, 10, 10, inputR[i].first ? tex_circle_conn : tex_circle_open, white(), white(), white()) == MOUSE_RELEASE) {
				if (!AnWeb::selConnNode) {
					AnWeb::selConnNode = this;
					AnWeb::selConnId = i;
					AnWeb::selConnIdIsOut = false;
					AnWeb::selPreClear = false;
				}
				else {
					AnWeb::selConnNode->ConnectTo(AnWeb::selConnId, this, i);
					AnWeb::selConnNode = nullptr;
				}
			}
		}
		else if (AnWeb::selConnNode == this && AnWeb::selConnId == i && !AnWeb::selConnIdIsOut) {
			Engine::DrawLine(Vec2(pos.x, y + 8), Input::mousePos, white(), 1);
			UI::Texture(pos.x - 5, y + 3, 10, 10, inputR[i].first ? tex_circle_conn : tex_circle_open);
		}
		else {
			UI::Texture(pos.x - 5, y + 3, 10, 10, inputR[i].first ? tex_circle_conn : tex_circle_open, red(0.3f));
		}
		//UI::Texture(pos.x - 5, y + 3, 10, 10, inputR[0].first ? tex_circle_conn : tex_circle_open);
		UI::Label(pos.x + 10, y, 12, "value 1", font, white());
		//}
		if (valXs.size()) {
			plt::plot(pos.x + 2, pos.y + 18 + 17 * cnt, width - 4, width - 4, &valXs[0], &valYs[0], valXs.size());
		}
	}
}

float Node_Plot::DrawSide() {
	Engine::DrawQuad(pos.x, pos.y, width, 16, white(selected ? 1.0f : 0.7f, 0.35f));
	if (Engine::Button(pos.x, pos.y, 16, 16, expanded ? Icons::expand : Icons::collapse, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) expanded = !expanded;
	UI::Label(pos.x + 20, pos.y + 1, 12, "Plot list(float)", font, white());
	if (expanded) {
		Engine::DrawQuad(pos.x, pos.y + 16, width, 4 + width, white(0.7f, 0.25f));
		if (valXs.size()) {
			plt::plot(pos.x + 2, pos.y + 18, width - 4, width - 4, &valXs[0], &valYs[0], valXs.size());
		}
		return width + 21;
	}
	else return 17;
}

Vec2 Node_Plot::DrawConn() {
	auto cnt = 1;
	float y = pos.y + 18;
	//for (uint i = 0; i < script->invarCnt; i++, y += 17) {
		if (inputR[0].first) Engine::DrawLine(Vec2(pos.x, expanded ? y + 8 : pos.y + 8), Vec2(inputR[0].first->pos.x + inputR[0].first->width, inputR[0].first->expanded ? inputR[0].first->pos.y + 20 + 8 + (inputR[0].second + inputR[0].first->script->invars.size()) * 17 : inputR[0].first->pos.y + 8), white(), 2);
	//}
	if (expanded) return Vec2(width, 19 + 17 * cnt + width);
	else return Vec2(width, 16);
}

void Node_Plot::Execute() {
	/*
	if (!inputR[0].first) return;
	auto ref = inputR[0].first->script[inputV[0].second].first.value;
	auto sz = PyList_Size(ref);
	valXs.resize(sz);
	valYs.resize(sz);
	for (Py_ssize_t a = 0; a < sz; a++) {
		valXs[a] = (float)a;
		auto obj = PyList_GetItem(ref, a);
		valYs[a] = (float)PyFloat_AsDouble(obj);
	}
	*/
}