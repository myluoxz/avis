#pragma once
#include "Engine.h"

enum class POPUP_TYPE : byte {
    NONE,
	MENU,
    DRAWMODE,
	COLORPICK,
	DROPDOWN,
	RESNM,
	RESID,
	ATOMID,
	SYSMSG
};

class Popups {
public:
	struct MenuItem {
		typedef void(*CBK)();

		GLuint icon = 0;
		std::string label;
		std::vector<MenuItem> child;
		CBK callback;

		void Set(const Texture& tex, const std::string& str, CBK cb) {
			icon = tex ? tex.pointer : 0;
			label = str;
			callback = cb;
		}
	};
	struct DropdownItem {
		DropdownItem(uint* a = 0, std::string* b = 0, bool f = false) : target(a), list(b), flags(f) {}

		uint* target;
		std::string* list;
		bool seld, flags;
	};

    static POPUP_TYPE type;
    static Vec2 pos, pos2;
    static void* data;
	static int selectedMenu;

    static void Draw(), DrawMenu(), DrawDropdown();
	static bool DoDrawMenu(std::vector<MenuItem>* mn, float x, float y, size_t* act);
};