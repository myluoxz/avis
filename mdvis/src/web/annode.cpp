#include "anweb.h"
#include "anconv.h"
#ifndef IS_ANSERVER
#include "ui/icons.h"
#include "ui/ui_ext.h"
#include "res/resdata.h"
#endif

const string Node_Inputs::sig = ".in";
const string Node_Inputs_ActPar::sig = ".inact";
const string Node_Inputs_SelPar::sig = ".insel";
const string Node_AddBond::sig = ".abnd";
const string Node_Camera_Out::sig = ".camo";
const string Node_Plot::sig = ".plot";
const string Node_Recolor::sig = ".recol";
const string Node_Recolor_All::sig = ".recola";
const string Node_SetParam::sig = ".param";
const string Node_ShowRange::sig = ".srng";
const string Node_TraceTrj::sig = ".trrj";
const string Node_Volume::sig = ".vol";
const string Node_Remap::sig = ".remap";

Vec4 AnNode::bgCol = white(0.7f, 0.25f);

Texture* AnNode::tex_circle_open = nullptr, *AnNode::tex_circle_conn = nullptr;
float AnNode::width = 220;

void AnNode::Init() {
#ifndef IS_ANSERVER
	tex_circle_open = new Texture(res::node_open_png, res::node_open_png_sz);
	tex_circle_conn = new Texture(res::node_conn_png, res::node_conn_png_sz);

	Node_Volume::Init();
#endif
}

bool AnNode::Select() {
#ifndef IS_ANSERVER
	bool in = Rect(pos.x + 16, pos.y, width - 16, 16).Inside(Input::mousePos);
	if (in) selected = true;
	return in;
#else
	return false;
#endif
}

Vec2 AnNode::DrawConn() {
#ifndef IS_ANSERVER
	if (script->ok) {
		auto cnt = (script->invars.size() + script->outvars.size());
		float y = pos.y + 18 + hdrSz;
		for (uint i = 0; i < script->invars.size(); i++, y += 17) {
			auto& ri = inputR[i];
			if (ri.first) {
				Vec2 p2 = Vec2(pos.x, expanded ? y + 8 : pos.y + 8);
				Vec2 p1 = Vec2(ri.first->pos.x + ri.first->width, ri.first->expanded ? ri.first->pos.y + 28 + ri.first->hdrSz + (ri.second + ri.first->inputR.size()) * 17 : ri.first->pos.y + 8);
				Vec2 hx = Vec2((p2.x > p1.x) ? (p2.x - p1.x) / 2 : (p1.x - p2.x) / 3, 0);
				UI2::Bezier(p1, p1 + hx, p2 - hx, p2, white(), 30);
			}
		}
		if (expanded) return Vec2(width, 19 + 17 * cnt + hdrSz + ftrSz + DrawLog(19 + 17 * cnt + hdrSz + ftrSz));
		else return Vec2(width, 16 + DrawLog(16));
	}
	else return Vec2(width, 35);
#else
	return Vec2();
#endif
}

void AnNode::Draw() {
#ifndef IS_ANSERVER
	const float connrad = 6;

	auto cnt = (script->invars.size() + script->outvars.size());
	Engine::DrawQuad(pos.x, pos.y, width, 16, Vec4(titleCol, selected ? 1.0f : 0.7f));
	if (Engine::Button(pos.x, pos.y, 16, 16, expanded ? Icons::expand : Icons::collapse, white(0.8f), white(), white(1, 0.5f)) == MOUSE_RELEASE) expanded = !expanded;
	UI::Label(pos.x + 18, pos.y + 1, 12, title, white());
	DrawToolbar();
	if (!script->ok) {
		Engine::DrawQuad(pos.x, pos.y + 16, width, 19, white(0.7f, 0.25f));
		UI::Label(pos.x + 5, pos.y + 17, 12, std::to_string(script->errorCount) + " errors", red());
	}
	else {
		if (expanded) {
			float y = pos.y + 16, yy = y;
			DrawHeader(y);
			hdrSz = y-yy - setSz;
			Engine::DrawQuad(pos.x, y, width, 4.0f + 17 * cnt, bgCol);
			y += 2;
			for (uint i = 0; i < script->invars.size(); i++, y += 17) {
				if (!AnWeb::selConnNode || (AnWeb::selConnIdIsOut && AnWeb::selConnNode != this)) {
					if (Engine::Button(pos.x - connrad, y + 8-connrad, connrad*2, connrad*2, inputR[i].first ? tex_circle_conn : tex_circle_open, white(), Vec4(1, 0.8f, 0.8f, 1), white(1, 0.5f)) == MOUSE_RELEASE) {
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
					Vec2 p2(pos.x, y + 8);
					Vec2 p1 = Input::mousePos;
					Vec2 hx = Vec2((p2.x > p1.x) ? (p2.x - p1.x)/2 : (p1.x - p2.x)/3, 0);
					UI2::Bezier(p1, p1 + hx, p2 - hx, p2, white(), 30);
					//Engine::DrawLine(Vec2(pos.x, y + 8), Input::mousePos, white(), 1);
					UI::Texture(pos.x - connrad, y + 8-connrad, connrad*2, connrad*2, inputR[i].first ? tex_circle_conn : tex_circle_open);
				}
				else {
					UI::Texture(pos.x - connrad, y + 8-connrad, connrad*2, connrad*2, inputR[i].first ? tex_circle_conn : tex_circle_open, red(0.3f));
				}
				UI::Label(pos.x + 10, y, 12, script->invars[i].first, white());
				if (!inputR[i].first) {
					auto& vr = script->invars[i].second;
					auto isi = (vr == "int");
					if (isi || (vr == "float")) {
						string s = std::to_string(isi ? inputVDef[i].i : inputVDef[i].f);
						s = UI::EditText(pos.x + width * 0.33f, y, width * 0.67f - 6, 16, 12, white(1, 0.5f), s, true, white());
						if (isi) inputVDef[i].i = TryParse(s, 0);
						else inputVDef[i].f = TryParse(s, 0.0f);
					}
					else {
						UI::Label(pos.x + width * 0.33f, y, 12, script->invars[i].second, white(0.3f));
					}
				}
				else {
					UI::Label(pos.x + width * 0.33f, y, 12, "<connected>", yellow());
				}
			}
			y += 2;

			for (uint i = 0; i < script->outvars.size(); i++, y += 17) {
				if (!AnWeb::selConnNode || ((!AnWeb::selConnIdIsOut) && (AnWeb::selConnNode != this))) {
					if (Engine::Button(pos.x + width - connrad, y + 8-connrad, connrad*2, connrad*2, outputR[i].first ? tex_circle_conn : tex_circle_open, white(), white(), white()) == MOUSE_RELEASE) {
						if (!AnWeb::selConnNode) {
							AnWeb::selConnNode = this;
							AnWeb::selConnId = i;
							AnWeb::selConnIdIsOut = true;
							AnWeb::selPreClear = false;
						}
						else {
							ConnectTo(i, AnWeb::selConnNode, AnWeb::selConnId);
							AnWeb::selConnNode = nullptr;
						}
					}
				}
				else if (AnWeb::selConnNode == this && AnWeb::selConnId == i && AnWeb::selConnIdIsOut) {
					Vec2 p2 = Input::mousePos;
					Vec2 p1(pos.x + width, y + 8);
					Vec2 hx = Vec2((p2.x > p1.x) ? (p2.x - p1.x) / 2 : (p1.x - p2.x) / 3, 0);
					UI2::Bezier(p1, p1 + hx, p2 - hx, p2, white(), 30);
					//Engine::DrawLine(Vec2(pos.x + width, y + 8), Input::mousePos, white(), 1);
					UI::Texture(pos.x + width - connrad, y + 8-connrad, connrad*2, connrad*2, outputR[i].first ? tex_circle_conn : tex_circle_open);
				}
				else {
					UI::Texture(pos.x + width - connrad, y + 8-connrad, connrad*2, connrad*2, outputR[i].first ? tex_circle_conn : tex_circle_open, red(0.3f));
				}


				UI::font->Align(ALIGN_TOPRIGHT);
				UI::Label(pos.x + width - 10, y, 12, script->outvars[i].first, white());
				UI::font->Align(ALIGN_TOPLEFT);
				UI::Label(pos.x + 2, y, 12, script->outvars[i].second, white(0.3f), width * 0.67f - 6);
			}
			auto yo = y;
			DrawFooter(y);
			ftrSz = y - yo;
			if (AnWeb::executing) Engine::DrawQuad(pos.x, pos.y + 16, width, 3.0f + 17 * cnt, white(0.5f, 0.25f));
		}
	}
#endif
}

float AnNode::GetHeaderSz() {
	return (showDesc? (17 * script->descLines) : 0) + (showSett? setSz : 0) + hdrSz;
}

void AnNode::DrawHeader(float& off) {
	if (showDesc) {
		Engine::DrawQuad(pos.x, off, width, 17.0f * script->descLines, bgCol);
		UI::alpha = 0.7f;
		UI2::LabelMul(pos.x + 5, off + 1, 12, script->desc);
		UI::alpha = 1;
		off += 17 * script->descLines;
	}
	if (showSett) {
		float offo = off;
		DrawSettings(off);
		setSz = off - offo;
	}
	else setSz = 0;
}

float AnNode::DrawSide() {
#ifndef IS_ANSERVER
	auto cnt = (script->invars.size());
	Engine::DrawQuad(pos.x, pos.y, width, 16, white(selected ? 1.0f : 0.7f, 0.35f));
	if (Engine::Button(pos.x, pos.y, 16, 16, expanded ? Icons::expand : Icons::collapse, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) expanded = !expanded;
	UI::Label(pos.x + 20, pos.y + 1, 12, title, white());
	if (this->log.size() && Engine::Button(pos.x + width - 17, pos.y, 16, 16, Icons::log, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) {
		logExpanded = !logExpanded;
	}
	if (executing) {
		Engine::DrawQuad(pos.x, pos.y + 16, width, 2, white(0.7f, 0.25f));
		if (script->progress)
			Engine::DrawQuad(pos.x, pos.y + 16, 2 + (width - 2) * *((float*)script->progress), 2, red());
		else
			Engine::DrawQuad(pos.x, pos.y + 16, width * 0.1f, 2, red());
	}
	float y = pos.y + (executing ? 18 : 16);
	if (expanded) {
		DrawHeader(y);
		if (cnt > 0) {
			Engine::DrawQuad(pos.x, y, width, 2.0f + 17 * cnt, bgCol);
			for (uint i = 0; i < script->invars.size(); i++, y += 17) {
				UI::Label(pos.x + 2, y, 12, script->invars[i].first, white());
				if (!inputR[i].first) {
					auto& vr = script->invars[i].second;
					auto isi = (vr == "int");
					if (isi || (vr == "float")) {
						string s = std::to_string(isi ? inputVDef[i].i : inputVDef[i].f);
						s = UI::EditText(pos.x + width * 0.4f, y, width * 0.6f - 3, 16, 12, white(1, 0.5f), s, true, white());
						if (isi) inputVDef[i].i = TryParse(s, 0);
						else inputVDef[i].f = TryParse(s, 0.0f);
					}
					else {
						UI::Label(pos.x + width * 0.4f, y, 12, script->invars[i].second, white(0.3f));
					}
				}
				else {
					UI::Label(pos.x + width * 0.4f, y, 12, "<connected>", yellow());
				}
			}
		}
		if (AnWeb::executing) Engine::DrawQuad(pos.x, pos.y + 16, width, 3.0f + 17 * cnt, white(0.5f, 0.25f));
		
		DrawFooter(y);
		
		return y - pos.y + 1 + DrawLog(y);
	}
	else return y - pos.y + 1 + DrawLog(y);
#else
	return 0;
#endif
}

float AnNode::DrawLog(float off) {
	auto sz = this->log.size();
	if (!sz) return 0;
	auto sz2 = min<int>(sz, 10);
	if (logExpanded) {
		Engine::DrawQuad(pos.x, pos.y + off, width, 15.0f * sz2 + 2, black(0.9f));
		Engine::PushStencil(pos.x + 1, pos.y + off, width - 2, 15.0f * sz2);
		for (int i = 0; i < sz2; i++) {
			auto& l = log[i + logOffset];
			Vec4 col = (!l.first) ? white() : ((l.first == 1) ? yellow() : red());
			UI::Label(pos.x + 4, pos.y + off + 1 + 15 * i, 12, l.second, col);
		}
		Engine::PopStencil();
		if (sz > 10) {
			float mw = 115;
			float of = logOffset * mw / sz;
			float w = 10 * mw / sz;
			Engine::DrawQuad(pos.x + width - 3, pos.y + off + 1 + of, 2, w, white(0.3f));
			if (Engine::Button(pos.x + width - 17, pos.y + off + 150 - 33, 16, 16, Icons::up, white(0.8f), white(), white(1, 0.5f)) == MOUSE_RELEASE) {
				logOffset = max<int>(logOffset - 10, 0);
			}
			if (Engine::Button(pos.x + width - 17, pos.y + off + 150 - 16, 16, 16, Icons::down, white(0.8f), white(), white(1, 0.5f)) == MOUSE_RELEASE) {
				logOffset = min<int>(logOffset + 10, sz - 10);
			}
		}
		return 15 * sz2 + 2.0f;
	}
	else {
		Engine::DrawQuad(pos.x, pos.y + off, width, 2, black(0.9f));
		return 2;
	}
}

void AnNode::DrawToolbar() {
#ifndef IS_ANSERVER
	if (this->log.size() && Engine::Button(pos.x + width - 67, pos.y, 16, 16, Icons::log, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) {
		logExpanded = !logExpanded;
	}
	if (!!script->descLines) {
		if (Engine::Button(pos.x + width - 33, pos.y, 16, 16, Icons::details, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) {
			showDesc = !showDesc;
		}
	}
	if (Engine::Button(pos.x + width - 16, pos.y, 16, 16, Icons::cross, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE) {
		op = ANNODE_OP::REMOVE;
	}
#endif
}

void AnNode::ConnectTo(uint id, AnNode* tar, uint tarId) {
	if (CanConn(script->outvars[id].second, tar->script->invars[tarId].second)) {
		if (tar->inputR[tarId].first) tar->inputR[tarId].first->outputR[tar->inputR[tarId].second].first = nullptr;
		tar->inputR[tarId].first = this;
		tar->inputR[tarId].second = id;
		outputR[id].first = tar;
		outputR[id].second = tarId;
		OnConn(true, id);
		tar->OnConn(false, tarId);
	}
}

AnNode::AnNode(AnScript* scr) : script(scr), canTile(false) {
	if (!scr) return;
	title = scr->name;
	inputR.resize(scr->invars.size(), std::pair<AnNode*, uint>());
	outputR.resize(scr->outvars.size(), std::pair<AnNode*, uint>());
	conV.resize(outputR.size());
}

#define sp << " "
#define nl << "\n"
void AnNode::Save(std::ofstream& strm) {
	
}

void AnNode::Load(std::ifstream& strm) {
	int ic, cc, ct;
	strm >> ic >> cc >> ct;
	string vn, tp;
	for (auto i = 0; i < ic; i++) {
		strm >> vn >> tp;
		for (uint b = 0; b < script->invars.size(); b++) {
			string nn = script->invars[b].first;
			if (nn == vn) {
				auto t = ((CScript*)script)->_invars[b].type;
				if (t == AN_VARTYPE::DOUBLE)
					inputVDef[b].d = TryParse(tp, 0.0);
				else if (t == AN_VARTYPE::INT)
					inputVDef[b].i = TryParse(tp, 0);
			}
		}
	}
	for (auto i = 0; i < cc; i++) {
		strm >> ic;
		if (ic < 0) continue;
		strm >> vn >> tp;
		AnNode* n2 = AnWeb::nodes[ic];
		uint i1 = -1, i2 = -1;
		for (uint b = 0; b < n2->script->outvars.size(); b++) {
			if (n2->script->outvars[b].first == vn) {
				i2 = b;
				break;
			}
		}
		for (uint b = 0; b < script->invars.size(); b++) {
			if (script->invars[b].first == tp) {
				i1 = b;
				break;
			}
		}
		if (i1 == -1 || i2 == -1) continue;
		n2->ConnectTo(i2, this, i1);
	}
	canTile = !!ct;
}

void AnNode::SaveOut(const string& path) {
	string nm = script->name;
	//std::replace(nm.begin(), nm.end(), '/', '_');
	std::ofstream strm(path + std::to_string(id) + nm, std::ios::binary);
	if (strm.is_open()) {
		auto cs = conV.size();
		_StreamWrite(&cs, &strm, 1);
		for (auto& c : conV) {
			c.Write(strm);
		}
		strm.close();
	}
}

void AnNode::LoadOut(const string& path) {
	string nm = script->name;
	std::ifstream strm(path + std::to_string(id) + nm, std::ios::binary);
	if (strm.is_open()) {
		byte sz = 0;
		_Strm2Val(strm, sz);
		if (sz != conV.size()) {
			Debug::Warning("Node", "output data corrupted!");
			return;
		}
		for (auto& c : conV) {
			c.Read(strm);
		}
	}
}

void AnNode::SaveConn() {
	auto sz = inputR.size();
	_connInfo.resize(sz);
	for (size_t a = 0; a < sz; a++) {
		ConnInfo& cn = _connInfo[a];
		cn.mynm = script->invars[a].first;
		cn.mytp = script->invars[a].second;
		auto ra = inputR[a].first;
		cn.cond = cn.tar = ra;
		if (cn.cond) {
			cn.tarnm = ra->script->outvars[inputR[a].second].first;
			cn.tartp = ra->script->outvars[inputR[a].second].second;
		}
	}
}

void AnNode::ClearConn() {
	inputR.clear();
	inputR.resize(script->invars.size(), std::pair<AnNode*, uint>());
	outputR.clear();
	outputR.resize(script->outvars.size(), std::pair<AnNode*, uint>());
	conV.resize(outputR.size());
}

void AnNode::Reconn() {
	for (auto& cn : _connInfo) {
		for (size_t a = 0, sz = inputR.size(); a < sz; a++) {
			if (script->invars[a].first == cn.mynm) {
				if (script->invars[a].second == cn.mytp) {
					if (cn.cond) {
						auto& ra = cn.tar;
						for (size_t b = 0, sz2 = ra->outputR.size(); b < sz2; b++) {
							if (ra->script->outvars[b].first == cn.tarnm) {
								if (ra->script->outvars[b].second == cn.tartp) {
									ra->ConnectTo(b, this, a);
									break;
								}
							}
						}
					}
					break;
				}
			}
		}
	}
}

bool AnNode::CanConn(string lhs, string rhs) {
	if (rhs[0] == '*' && lhs.substr(0, 4) != "list") return true;
	auto s = lhs.size();
	if (s != rhs.size()) return false;
	for (int a = 0; a < s; a++) {
		if (lhs[a] != rhs[a] && rhs[a] != '*') return false;
	}
	return true;
}

void AnNode::CatchExp(char* c) {
	auto ss = string_split(c, '\n');
	for (auto& s : ss) {
		log.push_back(std::pair<byte, string>(2, s));
	}

	ErrorView::Message msg{};
	msg.name = script->name;
	msg.msg = ss;
	ErrorView::execMsgs.push_back(msg);
}


PyNode::PyNode(PyScript* scr) : AnNode(scr) {
	if (!scr) return;
	title += " (python)";
	inputV.resize(scr->invars.size());
	outputV.resize(scr->outvars.size());
	outputVC.resize(scr->outvars.size());
	inputVDef.resize(scr->invars.size());
	for (uint i = 0; i < scr->invars.size(); i++) {
		inputV[i] = scr->_invars[i];
		if (scr->_invars[i].type == AN_VARTYPE::DOUBLE) inputVDef[i].d = 0;
		else inputVDef[i].i = 0;
	}
	for (uint i = 0; i < scr->outvars.size(); i++) {
		auto& ovi = outputV[i] = scr->_outvars[i];
		conV[i].type = ovi.type;
		conV[i].typeName = ovi.typeName;
		switch (ovi.type) {
		case AN_VARTYPE::INT:
			conV[i].value = &outputVC[i].val.i;
			break;
		case AN_VARTYPE::DOUBLE:
			conV[i].value = &outputVC[i].val.d;
			break;
		case AN_VARTYPE::LIST:
			conV[i].dimVals.resize(ovi.dim);
			outputVC[i].dims.resize(ovi.dim);
			for (int j = 0; j < outputV[i].dim; j++)
				conV[i].dimVals[j] = &outputVC[i].dims[j];
			conV[i].value = &outputVC[i].val.arr.p;
			break;
		default:
			Debug::Warning("PyNode", "case not handled!");
			break;
		}
	}
}

void PyNode::Execute() {
	auto scr = (PyScript*)script;
	for (uint i = 0; i < script->invars.size(); i++) {
		auto& mv = scr->_invars[i];
		if (inputR[i].first) {
			auto& cv = inputR[i].first->conV[inputR[i].second];
			switch (mv.type) {
			case AN_VARTYPE::INT:
				scr->Set(i, *((int*)cv.value));
				break;
			case AN_VARTYPE::DOUBLE:
				scr->Set(i, *((double*)cv.value));
				break;
			case AN_VARTYPE::LIST:
				auto sz = cv.dimVals.size();
				std::vector<int> dims(sz);
				for (size_t a = 0; a < sz; a++)
					dims[a] = *cv.dimVals[a];
				scr->Set(i, AnConv::PyArr(inputR[i].first->script->outvars[inputR[i].second].second[6], (int)sz, &dims[0], *(void**)cv.value));
				break;
			}
		}
		else {
			switch (mv.type) {
			case AN_VARTYPE::INT:
				scr->Set(i, inputVDef[i].i);
				break;
			case AN_VARTYPE::DOUBLE:
				scr->Set(i, inputVDef[i].f);
				break;
			default:
				Debug::Error("PyNode", "Value not handled!");
				throw "";
				break;
			}
		}
	}
	script->Exec();
	for (uint i = 0; i < script->outvars.size(); i++) {
		auto& mv = outputV[i];
		mv.value = scr->pRets[i];
		Py_INCREF(mv.value);
		switch (mv.type) {
		case AN_VARTYPE::DOUBLE:
			outputVC[i].val.d = PyFloat_AsDouble(mv.value);
			break;
		case AN_VARTYPE::INT:
			outputVC[i].val.i = (int)PyLong_AsLong(mv.value);
			break;
		case AN_VARTYPE::LIST:
		{
			auto& ar = outputVC[i].val.arr;
			int n;
			ar.p = AnConv::FromPy(mv.value, conV[i].dimVals.size(), &conV[i].dimVals[0], n);
			n *= scr->_outvars[i].stride;
			ar.data.resize(n);
			memcpy(&ar.data[0], ar.p, n);
			ar.p = &ar.data[0];
			break;
		}
		default:
			OHNO("AnVar", "Unexpected scr_vartype " + std::to_string((int)(mv.type)));
			break;
		}
	}
}

void PyNode::CatchExp(char* c) {
	if (!c[0]) return;
	auto ss = string_split(c, '\n');
	if (ss.size() == 1 && ss[0] == "") return;

	int i = 0;
	for (auto& s : ss) {
		if (s == "Traceback (most recent call last):") {
			break;
		}
		else {
			log.push_back(std::pair<byte, string>(0, s));
			i++;
		}
	}
	auto n = ss.size() - 1;
	log.push_back(std::pair<byte, string>(2, ss[n-1]));
	ErrorView::Message msg{};
	msg.name = script->name;
	msg.path = script->path;
	msg.severe = true;
	msg.msg.resize(n - i - 1);
	msg.msg[0] = ss[n-1];
	for (int a = i + 1; a < n - 1; a++) {
		msg.msg[a - i] = ss[a];
	}
	auto loc = string_find(ss[n - 3], ", line ");
	msg.linenum = atoi(&ss[n - 3][loc + 7]);
	ErrorView::execMsgs.push_back(msg);
}


CNode::CNode(CScript* scr) : AnNode(scr) {
	if (!scr) return;
	title += " (c++)";
	inputV.resize(scr->invars.size());
	outputV.resize(scr->outvars.size());
	inputVDef.resize(scr->invars.size());
	for (uint i = 0; i < scr->invars.size(); i++) {
		inputV[i] = scr->_invars[i].value;
		if (scr->_invars[i].type == AN_VARTYPE::DOUBLE) inputVDef[i].d = 0;
		else inputVDef[i].i = 0;
	}
	for (uint i = 0; i < scr->outvars.size(); i++) {
		outputV[i] = scr->_outvars[i].value;
		conV[i] = scr->_outvars[i];
	}
}

void CNode::Execute() {
	auto scr = (CScript*)script;
	for (uint i = 0; i < scr->invars.size(); i++) {
		auto& mv = scr->_invars[i];
		if (inputR[i].first) {
			auto& cv = inputR[i].first->conV[inputR[i].second];
			switch (mv.type) {
			case AN_VARTYPE::INT:
				scr->Set(i, *(int*)cv.value);
				break;
			case AN_VARTYPE::DOUBLE:
				scr->Set(i, *(double*)cv.value);
				break;
			case AN_VARTYPE::LIST:
				scr->Set(i, *(float**)cv.value);
				
				for (uint j = 0; j < mv.dimVals.size(); j++) {
					*mv.dimVals[j] = *cv.dimVals[j];
				}
				break;
			default:
				OHNO("CNode", "Unexpected scr_vartype " + std::to_string((int)(mv.type)));
				break;
			}
		}
		else {
			switch (mv.type) {
			case AN_VARTYPE::INT:
				scr->Set(i, inputVDef[i].i);
				break;
			case AN_VARTYPE::DOUBLE:
				scr->Set(i, inputVDef[i].d);
				break;
			default:
				Debug::Error("CNode", "Value not handled!");
				break;
			}
		}
	}

	script->Exec();
}

void CNode::Reconn() {
	AnNode::Reconn();
	auto scr = (CScript*)script;
	inputV.resize(scr->invars.size());
	outputV.resize(scr->outvars.size());
	inputVDef.resize(scr->invars.size());
	for (uint i = 0; i < scr->invars.size(); i++) {
		inputV[i] = scr->_invars[i].value;
		if (scr->_invars[i].type == AN_VARTYPE::DOUBLE) inputVDef[i].d = 0;
		else inputVDef[i].i = 0;
	}
	for (uint i = 0; i < scr->outvars.size(); i++) {
		outputV[i] = scr->_outvars[i].value;
		conV[i] = scr->_outvars[i];
	}
}

void CNode::CatchExp(char* c) {
	ErrorView::Message msg{};
	msg.name = script->name;
	msg.path = script->path;
	msg.severe = true;
	
}


FNode::FNode(FScript* scr) : AnNode(scr) {
	if (!scr) return;
	title += " (fortran)";
	inputV.resize(scr->invars.size());
	outputV.resize(scr->outvars.size());
	inputVDef.resize(scr->invars.size());
	for (uint i = 0; i < scr->invars.size(); i++) {
		inputV[i] = scr->_invars[i].value;
		if (scr->_invars[i].type == AN_VARTYPE::DOUBLE) inputVDef[i].d = 0;
		else inputVDef[i].i = 0;
	}
	for (uint i = 0; i < scr->outvars.size(); i++) {
		outputV[i] = scr->_outvars[i].value;
		conV[i] = scr->_outvars[i];
	}
}

void FNode::Execute() {
	auto scr = (FScript*)script;
	for (uint i = 0; i < scr->invars.size(); i++) {
		scr->pre = i;
		auto& mv = scr->_invars[i];
		if (inputR[i].first) {
			auto& cv = inputR[i].first->conV[inputR[i].second];
			switch (mv.type) {
			case AN_VARTYPE::INT:
				scr->Set(i, *(int*)cv.value);
				break;
			case AN_VARTYPE::DOUBLE:
				scr->Set(i, *(double*)cv.value);
				break;
			default:
				Debug::Warning("CNode", "var not handled!");
				throw("");
			}
		}
		else {
			switch (mv.type) {
			case AN_VARTYPE::INT:
				scr->Set(i, inputVDef[i].i);
				break;
			case AN_VARTYPE::DOUBLE:
				scr->Set(i, inputVDef[i].d);
				break;
			default:
				Debug::Error("CNode", "Value not handled!");
				throw("");
			}
		}
	}
	scr->pre = -1;
	scr->post = -1;
	
	scr->Exec();

	for (uint i = 0; i < scr->outvars.size(); i++) {
		scr->post = i;
		if (scr->_outvars[i].type == AN_VARTYPE::LIST) {
			scr->_outarr_post[i]();
			auto& cv = conV[i];
			size_t dn = cv.dimVals.size();
			cv.data.dims.resize(dn);
			int sz = 1;
			for (size_t a = 0; a < dn; a++) {
				cv.data.dims[a] = (*scr->arr_shapeloc)[a];
				cv.dimVals[a] = &cv.data.dims[a];
				sz *= cv.data.dims[a];
			}
			cv.data.val.arr.data.resize(cv.stride * sz);
			cv.data.val.arr.p = &cv.data.val.arr.data[0];
			memcpy(cv.data.val.arr.p, *scr->arr_dataloc, cv.stride * sz);
			cv.value = &cv.data.val.arr.p;
		}
	}
	scr->post = -1;
}

void FNode::CatchExp(char* c) {
	ErrorView::Message msg{};
	msg.name = script->name;
	msg.path = script->path;
	msg.severe = true;
	string s = c;
	if (string_find(s, "At line ") == 0) {
		if ((msg.linenum = atoi(c + 8)) > 0) {
			auto lc = strchr(c, '\n');
			log.push_back(std::pair<byte, string>(2, lc + 1));
			string s(c + 9, (size_t)lc - (size_t)c - 20); //_temp__.f90\n
			msg.msg.resize(1, lc + 1);
			msg.msg.push_back("Fortran runtime error caught by handler");
			ErrorView::execMsgs.push_back(msg);
			return;
		}
	}
	else if (s.back() == 1) {
		s.pop_back();
		log.push_back(std::pair<byte, string>(2, s));
		msg.msg.resize(1, s);
		auto scr = (FScript*)script;
		if (scr->pre > -1) {
			msg.msg.push_back("While handling input variable " + scr->invars[scr->pre].first);
		}
		else if (scr->post > -1) {
			msg.msg.push_back("While handling output variable " + scr->outvars[scr->post].first);
		}
		ErrorView::execMsgs.push_back(msg);
		return;
	}
	AnNode::CatchExp(c);
}