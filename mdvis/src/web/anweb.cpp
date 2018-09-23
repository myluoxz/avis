#include "anweb.h"
#ifndef IS_ANSERVER
#include "anops.h"
#include "errorview.h"
#include "ui/icons.h"
#include "md/Particles.h"
#include "vis/pargraphics.h"
#include "vis/system.h"
#endif

//#define NO_REDIR_LOG

bool AnWeb::lazyLoad = true;

AnNode* AnWeb::selConnNode = nullptr;
uint AnWeb::selConnId = 0;
bool AnWeb::selConnIdIsOut = false, AnWeb::selPreClear = false;
AnScript* AnWeb::selScript = nullptr;
byte AnWeb::selSpNode = 0;

std::string AnWeb::activeFile = "";
std::vector<AnNode*> AnWeb::nodes;

bool AnWeb::drawFull = false, AnWeb::expanded = true, AnWeb::executing = false, AnWeb::apply = false;
float AnWeb::maxScroll, AnWeb::scrollPos = 0, AnWeb::expandPos = 0;

std::thread* AnWeb::execThread = nullptr;
AnNode* AnWeb::execNode = nullptr;

bool AnWeb::hasPy = false, AnWeb::hasC = false, AnWeb::hasFt = false;
bool AnWeb::hasPy_s = false, AnWeb::hasC_s = false, AnWeb::hasFt_s = false;

void AnWeb::Init() {
	Insert(new Node_Inputs());
	for (int a = 0; a < 10; a++) {
		AnBrowse::mscFdExpanded[a] = true;
	}
	ChokoLait::focusFuncs.push_back(CheckChanges);
}

void AnWeb::Insert(AnScript* scr, Vec2 pos) {
	AnNode* nd;
	if (scr->type == AN_SCRTYPE::PYTHON)
		nd = new PyNode((PyScript*)scr);
	else {
		Debug::Error("AnWeb::Insert", "Invalid type!");
		return;
	}
	Insert(nd, pos);
}

void AnWeb::Insert(AnNode* node, Vec2 pos) {
	nodes.push_back(node);
	nodes.back()->pos = pos;
}

void AnWeb::Update() {
#ifndef IS_ANSERVER
	if (Input::mouse0) {
		if (Input::mouse0State == MOUSE_DOWN) {
			for (auto n : nodes) n->selected = false;
			for (auto n : nodes) {
				if (n->Select()) break;
			}
		}
		//else {
		//	for (auto n : nodes) {
		//		if (n->selected) n->pos += Input::mouseDelta;
		//	}
		//}
	}
	if (!executing && execThread) { //do I need this?
		if (execThread->joinable()) execThread->join();
	}
	if (apply) {
		apply = false;
		auto frm = Particles::anim.activeFrame;
		Particles::anim.activeFrame = -1;
		Particles::SetFrame(frm);
	}

	if (Input::KeyDown(Key_P)) Save(IO::path + "nodes/test.web");
#endif
}

void AnWeb::Draw() {
#ifndef IS_ANSERVER
	AnNode::width = 220;
	UI::Quad(AnBrowse::expandPos, 0.0f, Display::width - AnBrowse::expandPos - AnOps::expandPos, Display::height - 18.0f, white(0.8f, 0.05f));
	Engine::BeginStencil(AnBrowse::expandPos, 0.0f, Display::width - AnBrowse::expandPos - AnOps::expandPos, Display::height - 18.0f);
	byte ms = Input::mouse0State;
	if (executing) {
		Input::mouse0 = false;
		Input::mouse0State = 0;
	}
	AnWeb::selPreClear = true;
	Vec2 poss(AnBrowse::expandPos + 10 - scrollPos, 100);
	float maxoff = 220, offy = -5;
	maxScroll = 10;
	int ns = nodes.size(), i = 0, iter = -1;
	bool iterTile = false, iterTileTop = false;
	for (auto n : nodes) {
		if (!n->canTile) {
			poss.x += maxoff + 20;
			maxScroll += maxoff + 20;
			poss.y = 100;
			maxoff = 220;
			if (selScript) {
				if (Engine::Button(poss.x, 100, maxoff, 30, white(0.3f, 0.05f)) == MOUSE_RELEASE) {
					iter = i - 1;
					iterTile = true;
					iterTileTop = true;
				}
				UI::Texture(poss.x + 95, 100, 30, 30, Icons::expand, white(0.2f));
				poss.y = 133;
			}
		}
		else {
			poss.y += offy + 5;
		}
		n->pos = poss;
		auto o = n->DrawConn();
		//maxoff = max(maxoff, o.x);
		offy = o.y;
		if (selScript) {
			if (Engine::Button(poss.x, poss.y + offy + 3, maxoff, 30, white(0.3f, 0.05f)) == MOUSE_RELEASE) {
				iter = i;
				iterTile = true;
				iterTileTop = false;
			}
			UI::Texture(poss.x - 15 + maxoff / 2, poss.y + offy + 3, 30, 30, Icons::expand, white(0.2f));
			offy += 31;
			if ((ns == i + 1) || !nodes[i + 1]->canTile) {
				if (Engine::Button(poss.x + maxoff + 10, 100, 30, 200, white(0.3f, 0.05f)) == MOUSE_RELEASE) {
					iter = i;
					iterTile = false;
				}
				UI::Texture(poss.x + maxoff + 10, 100 + 85, 30, 30, Icons::expand, white(0.2f));
				maxoff += 30;
			}
		}
		i++;
	}
	maxScroll += maxoff + (selScript ? 20 : 10);
	for (auto n : nodes) {
		n->Draw();
	}
	if (Input::mouse0State == MOUSE_UP && selPreClear) selConnNode = nullptr;

	float canScroll = max(maxScroll - (Display::width - AnBrowse::expandPos - AnOps::expandPos), 0.0f);
	//if (Input::KeyHold(Key_RightArrow)) scrollPos += 1000 * Time::delta;
	//if (Input::KeyHold(Key_LeftArrow)) scrollPos -= 1000 * Time::delta;
	scrollPos = Clamp(scrollPos - Input::mouseScroll * 1000 * Time::delta, 0.0f, canScroll);

	Input::mouse0State = ms;
	Input::mouse0 = (Input::mouse0State == 1) || (Input::mouse0State == 2);
	Engine::EndStencil();

	if (Input::mouse0State == MOUSE_UP) {
		if (selScript) {
			if (iter >= 0) {
				AnNode* pn = 0;
				if ((uintptr_t)selScript > 1) {
					switch (selScript->type) {
					case AN_SCRTYPE::PYTHON:
						pn = new PyNode((PyScript*)selScript);
						break;
					case AN_SCRTYPE::C:
						pn = new CNode((CScript*)selScript);
						break;
					case AN_SCRTYPE::FORTRAN:
						pn = new FNode((FScript*)selScript);
						break;
					default:
						Debug::Error("AnWeb::Draw", "Unhandled script type: " + std::to_string((int)selScript->type));
						break;
					}
				}
				else {
#define SW(nm, scr) case (byte)AN_NODE_ ## nm: pn = new scr(); break
					switch (selSpNode) {
						SW(SCN::OCAM, Node_Camera_Out);

						SW(IN::SELPAR, Node_Inputs_SelPar);

						SW(MOD::RECOL, Node_Recolor);
						SW(MOD::RECOLA, Node_Recolor_All);

						SW(MOD::PARAM, Node_SetParam);

						SW(GEN::BOND, Node_AddBond);
						SW(GEN::VOL, Node_Volume);
						SW(GEN::TRJ, Node_TraceTrj);

						SW(MISC::PLOT, Node_Plot);
						SW(MISC::SRNG, Node_ShowRange);
					default:
						Debug::Error("AnWeb::Draw", "Unhandled node type: " + std::to_string((int)selSpNode));
						return;
					}
#undef SW
				}
				if (iterTile) {
					if (iterTileTop) nodes[iter + 1]->canTile = true;
					else pn->canTile = true;
				}
				else pn->canTile = false;
				nodes.insert(nodes.begin() + iter + 1, pn);
			}
			selScript = nullptr;
		}
		else {
			for (auto nn = nodes.begin() + 1; nn != nodes.end(); nn++) {
				auto& n = *nn;
				if (n->op == ANNODE_OP::REMOVE) {
					if ((nn + 1) != nodes.end()) {
						if (!n->canTile && (*(nn + 1))->canTile)
							(*(nn + 1))->canTile = false;
					}
					for (uint i = 0; i < n->inputR.size(); i++) {
						if (n->inputR[i].first) n->inputR[i].first->outputR[n->inputR[i].second].first = nullptr;
					}
					for (uint i = 0; i < n->outputR.size(); i++) {
						if (n->outputR[i].first) n->outputR[i].first->inputR[n->outputR[i].second].first = nullptr;
					}
					delete(n);
					nodes.erase(nn);
					break;
				}
			}
		}
	}

	AnBrowse::Draw();
	AnOps::Draw();

	if (Input::KeyDown(Key_Escape)) {
		if (selScript) selScript = nullptr;
		else {
			drawFull = false;
			AnBrowse::expandPos = AnOps::expandPos = 0;
		}
	}
	if (selScript) {
		Texture* icon = 0;
		if ((uintptr_t)selScript == 1)
			icon = Icons::lightning;
		else if (selScript->type == AN_SCRTYPE::C)
			icon = Icons::lang_c;
		else if (selScript->type == AN_SCRTYPE::PYTHON)
			icon = Icons::lang_py;
		else
			icon = Icons::lang_ft;
		UI::Texture(Input::mousePos.x - 16, Input::mousePos.y - 16, 32, 32, icon, white(0.3f));
	}

	if (Engine::Button(Display::width - 71.0f, 1.0f, 70.0f, 16.0f, white(1, 0.4f), "Done", 12.0f, white(), true) == MOUSE_RELEASE) {
		drawFull = false;
		AnBrowse::expandPos = AnOps::expandPos = 0;
	}
	
	if (Engine::Button(200, 1, 70.0f, 16.0f, white(1, 0.4f), "Save", 12.0f, white(), true) == MOUSE_RELEASE)
		Save(IO::path + "nodes/rdf.anl");

	if (Engine::Button(275, 1, 70, 16, white(1, executing ? 0.2f : 0.4f), "Run", 12, white(), true) == MOUSE_RELEASE) {

	}
	bool canexec = (!AnOps::remote || (AnOps::connectStatus == 255));
	if (Engine::Button(350, 1, 107, 16, white(1, (!canexec || executing) ? 0.2f : 0.4f), "Run All", 12, white(), true) == MOUSE_RELEASE) {
		if (canexec) AnWeb::Execute();
	}
	UI::Texture(275, 1, 16, 16, Icons::play);
	UI::Texture(350, 1, 16, 16, Icons::playall);
#endif
}

void AnWeb::DrawSide() {
#ifndef IS_ANSERVER
	UI::Quad(Display::width - expandPos, 18, 180.0f, Display::height - 18.0f, white(0.9f, 0.15f));
	if (expanded) {
		float w = 180;
		AnNode::width = w - 2;
		UI::Label(Display::width - expandPos + 5, 20, 12, "Analysis", white());

		if (Engine::Button(Display::width - expandPos + 109, 20, 70, 16, white(1, 0.4f), "Edit", 12, white(), true) == MOUSE_RELEASE)
			drawFull = true;

		if (Engine::Button(Display::width - expandPos + 1, 38, 70, 16, white(1, executing ? 0.2f : 0.4f), "Run", 12, white(), true) == MOUSE_RELEASE) {

		}
		if (Engine::Button(Display::width - expandPos + 72, 38, 107, 16, white(1, executing ? 0.2f : 0.4f), "Run All", 12, white(), true) == MOUSE_RELEASE) {
			AnWeb::Execute();
		}
		UI::Texture(Display::width - expandPos + 1, 38, 16, 16, Icons::play);
		UI::Texture(Display::width - expandPos + 72, 38, 16, 16, Icons::playall);

		//Engine::BeginStencil(Display::width - w, 0, 150, Display::height);
		Vec2 poss(Display::width - expandPos + 1, 17 * 3 + 4);
		for (auto n : nodes) {
			n->pos = poss;
			poss.y += n->DrawSide();
		}
		//Engine::EndStencil();
		UI::Quad(Display::width - expandPos - 16.0f, Display::height - 34.0f, 16.0f, 16.0f, white(0.9f, 0.15f));
		if ((!UI::editingText && Input::KeyUp(Key_A)) || Engine::Button(Display::width - expandPos - 16.0f, Display::height - 34.0f, 16.0f, 16.0f, Icons::collapse, white(0.8f), white(), white(0.5f)) == MOUSE_RELEASE)
			expanded = false;
		expandPos = Clamp(expandPos + 1500 * Time::delta, 0.0f, 180.0f);
	}
	else {
		UI::Quad(Display::width - expandPos, 0.0f, expandPos, Display::height - 18.0f, white(0.9f, 0.15f));
		if ((!UI::editingText && Input::KeyUp(Key_A)) || Engine::Button(Display::width - expandPos - 110.0f, Display::height - 34.0f, 110.0f, 16.0f, white(0.9f, 0.15f), white(1, 0.15f), white(1, 0.05f)) == MOUSE_RELEASE)
			expanded = true;
		UI::Texture(Display::width - expandPos - 110.0f, Display::height - 34.0f, 16.0f, 16.0f, Icons::expand);
		UI::Label(Display::width - expandPos - 92.0f, Display::height - 33.0f, 12.0f, "Analysis (A)", white());
		expandPos = Clamp(expandPos - 1500 * Time::delta, 2.0f, 180.0f);
	}
#endif
}

void AnWeb::DrawScene() {
#ifndef IS_ANSERVER
	for (auto& n : nodes) n->DrawScene();
#endif
}

void AnWeb::Execute() {
	if (!executing) {
		executing = true;
		if (execThread) {
			if (execThread->joinable()) execThread->join();
			delete(execThread);
		}
#ifndef IS_ANSERVER
		if (AnOps::remote) {
			Save(activeFile);
			execThread = new std::thread(DoExecute_Srv);
		}
		else
#endif
			execThread = new std::thread(DoExecute);
	}
}

void AnWeb::DoExecute() {
	for (auto n : nodes) {
		n->log.clear();
	}
	ErrorView::execMsgs.clear();
	char* err = 0;
	static std::string pylog;
	for (auto n : nodes) {
#ifndef NO_REDIR_LOG
		if (n->script->type == AN_SCRTYPE::PYTHON)
			PyScript::ClearLog();
		else
			IO::RedirectStdio2(IO::path + "nodes/__tmpstd");
#endif
		execNode = n;
		n->executing = true;
		std::exception_ptr cxp;
		try {
			n->Execute();
		}
		catch (char* e) {
			err = e;
		}
		catch (...) {
			cxp = std::current_exception();
		}
		n->executing = false;

		if (cxp) {
			try {
				std::rethrow_exception(cxp);
			}
			catch (std::exception& e) {
				static std::string serr = e.what();
				serr += " thrown!\x01";
				err = &serr[0];
			}
		}

#ifndef NO_REDIR_LOG
		if (n->script->type == AN_SCRTYPE::PYTHON) {
			pylog = PyScript::GetLog();
		}
		else {
			IO::RestoreStdio2();
			auto f = std::ifstream(IO::path + "nodes/__tmpstd");
			std::string s;
			while (std::getline(f, s)) {
				n->log.push_back(std::pair<byte, std::string>(0, s));
			}
		}
#endif
		if (n->script->type == AN_SCRTYPE::PYTHON) {
			if (err)
				n->CatchExp(&pylog[0]);
			else {
				auto ss = string_split(pylog, '\n');
				for (auto& s : ss) {
					n->log.push_back(std::pair<byte, std::string>(0, s));
				}
			}
		}
		else if (err) {
			n->CatchExp(err);
			break;
		}
	}
	execNode = nullptr;
	executing = false;
	apply = true;
#ifndef NO_REDIR_LOG
	remove((IO::path + "nodes/__tmpstd").c_str());
#endif
}

void AnWeb::OnExecLog(std::string s, bool e) {
	if (execNode) {
		if (s == "___123___") {
			if (execNode->log.back().second == "")
				execNode->log.pop_back();
			execNode = nullptr;
		}
		else if ((uintptr_t)execNode <= 2) {
			(*(uintptr_t*)&execNode)--;
		}
		else execNode->log.push_back(std::pair<byte, std::string>(e ? 1 : 0, s));
	}
}

void AnWeb::DoExecute_Srv() {
#ifndef IS_ANSERVER
	AnOps::message = "checking for lock";
	if (AnOps::ssh.HasFile(AnOps::path + "/.lock")) {
		Debug::Warning("AnWeb", "An existing session is running!");
		executing = false;
		return;
	}
	AnOps::message = "cleaning old files";
	AnOps::ssh.Write("cd ser; rm -f ./*; cd in; rm -f ./*; cd ../out; rm -f ./*; cd " + AnOps::path);
	AnOps::message = "syncing files";
	AnOps::ssh.SendFile(activeFile, AnOps::path + "/ser/web.anl");
	AnOps::SendIn();
	AnOps::ssh.Write("chmod +r ser/web.anl");
	AnOps::message = "running";
	AnOps::ssh.Write("./mdvis_ansrv; echo '$%''%$'");
	AnOps::ssh.WaitFor("$%%$", 200);
	AnOps::RecvOut();
	LoadOut();
	AnOps::message = "finished";
	executing = false;
	apply = true;
#endif
}

#define sp << " "
#define nl << "\n"
#define wrs(s) _StreamWrite(s.c_str(), &strm, s.size() + 1);
void AnWeb::Save(const std::string& s) {
#define SVS(nm, vl) n->addchild(#nm, vl)
#define SV(nm, vl) SVS(nm, std::to_string(vl))
	XmlNode head = {};
	head.name = "MDVis_Graph_File";
	for (auto nd : nodes) {
		auto n = head.addchild("node");
		SV(type, (int)nd->script->type);
		SVS(name, nd->script->name);
		SV(tile, nd->canTile);
		nd->SaveConn();
		auto nc = n->addchild("conns");
		for (size_t a = 0; a < nd->_connInfo.size(); a++) {
			auto& c = nd->_connInfo[a];
			auto n = nc->addchild("item");
			SV(connd, c.cond);
			if (c.cond) {
				SVS(name, c.mynm);
				SVS(type, c.mytp);
				SV(tarid, c.tar->id);
				SVS(tarname, c.tarnm);
				SVS(tartype, c.tartp);
			}
			else {
				auto& tp = nd->script->invars[a].second;
				if (tp == "int") SV(value, nd->inputVDef[a].i);
				else if (tp == "double") SV(value, nd->inputVDef[a].d);
			}
		}
	}
	Xml::Write(&head, s);
#undef SVS
#undef SV
}

void AnWeb::SaveIn() {
	std::string path = IO::path + "nodes/__tmp__/";
	if (!IO::HasDirectory(path)) IO::MakeDirectory(path);
	path += "in/";
	if (!IO::HasDirectory(path)) IO::MakeDirectory(path);
	else {
		auto fls = IO::GetFiles(path);
		for (auto& f : fls) {
			remove((path + "/" + f).c_str());
		}
	}
	for (auto n : nodes) {
		n->SaveIn(path);
	}
}

void AnWeb::SaveOut() {
#ifdef IS_ANSERVER
	std::string path = IO::path + "ser/";
#else
	std::string path = IO::path + "nodes/__tmp__/";
#endif
	if (!IO::HasDirectory(path)) IO::MakeDirectory(path);
	path += "out/";
	if (!IO::HasDirectory(path)) IO::MakeDirectory(path);
	else {
		auto fls = IO::GetFiles(path);
		for (auto& f : fls) {
			remove((path + "/" + f).c_str());
		}
	}
	for (auto n : nodes) {
		n->SaveOut(path);
	}
}

byte GB(std::istream& strm) {
	byte b;
	strm.read((char*)&b, 1);
	return b;
}

void AnWeb::Load(const std::string& s) {
	XmlNode* head = Xml::Parse(s);
	if (!head) {
		Debug::Warning("AnWeb", "Cannot open save file!");
		return;
	}
	nodes.clear(); //memory leaking!
	AnNode* n = nullptr;
	AN_SCRTYPE tp;
	std::string nm;
	int cnt = 0;
	for (auto& nd : head->children[0].children) {
		if (nd.name != "node") continue;
		for (auto& c : nd.children) {
			if (c.name == "type") tp = (AN_SCRTYPE)TryParse(c.value, 0);
			else if (c.name == "name") {
				nm = c.value;
				switch (tp) {
				case AN_SCRTYPE::NONE:
		#define ND(scr) if (nm == scr::sig) n = new scr(); else
					ND(Node_Inputs)
					ND(Node_Inputs_ActPar)
					ND(Node_Inputs_SelPar)
					ND(Node_AddBond)
					//ND(Node_AddVolume)
					ND(Node_TraceTrj)
					ND(Node_Camera_Out)
					ND(Node_Recolor)
					ND(Node_Recolor_All)
					ND(Node_SetParam)
					ND(Node_Volume)
					ND(Node_Plot)
					ND(Node_ShowRange)
					Debug::Warning("AnWeb::Load", "Unknown node name: " + nm);
		#undef ND
					break;
				case AN_SCRTYPE::C:
					n = new CNode(CScript::allScrs[nm]);
					break;
				case AN_SCRTYPE::PYTHON:
					n = new PyNode(PyScript::allScrs[nm]);
					break;
				case AN_SCRTYPE::FORTRAN:
					n = new FNode(FScript::allScrs[nm]);
					break;
				default:
					Debug::Warning("AnWeb::Load", "Unknown node type: " + std::to_string((byte)tp));
					continue;
				}
				n->id = cnt++;
				nodes.push_back(n);
			}
			else if (c.name == "tile") n->canTile = (c.value == "1");
			
#define GTS(nm, vl) if (cc.name == #nm) vl = cc.value
#define GT(nm, vl) if (cc.name == #nm) vl = TryParse(cc.value, vl)
			else if (c.name == "conns") {
				for (auto& c1 : c.children) {
					if (c1.name != "item") continue;
					AnNode::ConnInfo ci;
					for (auto& cc : c1.children) {
						if (cc.name == "connd") ci.cond = (cc.value == "1");
						else if (ci.cond) {
							GTS(name, ci.mynm);
							else GTS(type, ci.mytp);
							else if (cc.name == "tarid") {
								auto i = TryParse(cc.value, 0xffff);
								if (i < cnt - 1) ci.tar = nodes[i];
							}
							else GTS(tarname, ci.tarnm);
							else GTS(tartype, ci.tartp);
						}
						else {
							//
						}
					}
					n->_connInfo.push_back(ci);
				}
			}
#undef GTS
#undef GT
		}
		n->Reconn();
		activeFile = s;
	}
}

void AnWeb::LoadIn() {
#ifdef IS_ANSERVER
	std::string path = IO::path + "ser/in/";
#else
	std::string path = IO::path + "nodes/__tmp__/in/";
#endif
	for (auto n : nodes) {
		n->LoadIn(path);
	}
}

void AnWeb::LoadOut() {
	std::string path = IO::path + "nodes/__tmp__/out/";
	for (auto n : nodes) {
		n->LoadOut(path);
	}
}

void AnWeb::CheckChanges() {
	SaveConn();
	AnBrowse::Refresh();
	ClearConn();
	Reconn();
}

void AnWeb::SaveConn() {
	for (auto n : nodes) {
		n->SaveConn();
	}
}

void AnWeb::ClearConn() {
	for (auto n : nodes) {
		n->ClearConn();
	}
}

void AnWeb::Reconn() {
	for (auto n : nodes) {
		n->Reconn();
	}
}

void AnWeb::Serialize(XmlNode* n) {
	n->name = "Analysis";
	n->addchild("graph", "graph.anl");
	Save(VisSystem::currentSavePath + "_data/graph.anl");
#define SVS(nm, vl) n->addchild(#nm, vl)
#define SV(nm, vl) SVS(nm, std::to_string(vl))
	SV(focus, drawFull);
#undef SVS
#undef SV
}

void AnWeb::Deserialize(XmlNode* nd) {
#define GT(nm, vl) if (n.name == #nm) vl = TryParse(n.value, vl)
#define GTV(nm, vl) if (n.name == #nm) Xml::ToVec(&n, vl)
	for (auto& n2 : nd->children) {
		if (n2.name == "Analysis") {
			for (auto& n : n2.children) {
				if (n.name == "graph") {
					Load(VisSystem::currentSavePath + "_data/" + n.value);
				}
				else if (n.name == "focus") drawFull = (n.value == "1");
			}
			return;
		}
	}
#undef GT
#undef GTV
}

void AnWeb::OnSceneUpdate() {
	for (auto n : nodes) {
		n->OnSceneUpdate();
	}
}

void AnWeb::OnAnimFrame() {
	for (auto n : nodes) {
		n->OnAnimFrame();
	}
}