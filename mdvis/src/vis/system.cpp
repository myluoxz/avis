#include "system.h"
#include "py/PyWeb.h"
#include "ui/icons.h"

Vec4 VisSystem::accentColor = Vec4(1, 1, 1, 1);
uint VisSystem::renderMs, VisSystem::uiMs;
Font* VisSystem::font;

VIS_MOUSE_MODE VisSystem::mouseMode = VIS_MOUSE_MODE::ROTATE;

std::unordered_map<uint, float> VisSystem::_bondLengths;
std::unordered_map<ushort, Vec3> VisSystem::_type2Col;
std::unordered_map<ushort, std::array<float, 2>> VisSystem::radii;

void VisSystem::Init() {
	radii.clear();
	std::ifstream strm(IO::path + "/radii.txt");
	if (strm.is_open()) {
		string s;
		while (!strm.eof()) {
			std::getline(strm, s);
			auto p = string_split(s, ' ', true);
			if (p.size() != 5) continue;
			auto i = *(ushort*)&(p[0])[0];
			auto ar = std::stof(p[1]);
			auto vr = std::stof(p[4]);
			radii.emplace(i, std::array<float,2>({{ar, vr}}));
		}
		strm.close();
	}
	strm.open(IO::path + "/bondlengths.txt");
	_bondLengths.clear();
	if (strm.is_open()) {
		string s;
		while (!strm.eof()) {
			std::getline(strm, s);
			auto p = string_split(s, ' ', true);
			if (p.size() != 2) continue;
			auto p2 = string_split(p[0], '-');
			if (p2.size() != 2) continue;
			auto i1 = *(ushort*)&(p2[0])[0];
			auto i2 = *(ushort*)&(p2[1])[0];
			auto ln = pow(std::stof(p[1]) * 0.001f, 2);
			_bondLengths.emplace(i1 + (i2 << 16), ln);
			_bondLengths.emplace(i2 + (i1 << 16), ln);
		}
		strm.close();
	}
	strm.open(IO::path + "/colors.txt");
	_type2Col.clear();
	if (strm.is_open()) {
		string s;
		Vec3 col;
		while (!strm.eof()) {
			std::getline(strm, s);
			auto p = string_split(s, ' ', true);
			if (p.size() != 4) continue;
			auto i = *(ushort*)&p[0];
			col.x = std::stof(p[1]);
			col.y = std::stof(p[2]);
			col.z = std::stof(p[3]);
			_type2Col.emplace(i, col);
		}
		strm.close();
	}
}

bool VisSystem::InMainWin(const Vec2& pos) {
	if (PyWeb::drawFull) return false;
	else return (pos.x > ParMenu::expandPos + 16) && (pos.x < Display::width - PyWeb::expandPos) && (pos.y < Display::height - 18);
}

void VisSystem::DrawBar() {
	Engine::DrawQuad(0, Display::height - 18.0f, (float)Display::width, 18, white(0.9f, 0.1f));
	UI::Label(2, Display::height - 16.0f, 12, "Render: " + std::to_string(renderMs) + "ms  UI: " + std::to_string(uiMs) + "ms", font, white(0.5f));

	byte sel = (byte)mouseMode;
	for (byte b = 0; b < 3; b++) {
		if (Engine::Button(170 + 17 * b, Display::height - 17.0f, 16, 16, (&Icons::toolRot)[b], (sel == b) ? Vec4(1, 0.7f, 0.4f, 1) : white(0.7f), white(), white(0.5f)) == MOUSE_RELEASE) {
			mouseMode = (VIS_MOUSE_MODE)b;
		}
	}
}