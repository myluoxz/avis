#pragma once
#include "vis/system.h"

class Preferences {
	struct Pref {
		std::string sig;
		std::string name;
		std::string desc;
		
		enum class TYPE {
			BOOL, INT, FLOAT, STRING
		} type;

		bool minmax, slide;
		int   min_i, max_i;
		float min_f, max_f;

		bool        dval_b, *val_b = nullptr;
		int         dval_i, *val_i = nullptr;
		float       dval_f, *val_f = nullptr;
		std::string dval_s, *val_s = nullptr;
	};
	static std::vector<std::pair<std::string, std::vector<Pref>>> prefs;
	static bool show;

public:
	static void Init();

	static void Draw();

	static void Link(const std::string sig, bool* b);
	static void Link(const std::string sig, int* i);
	static void Link(const std::string sig, float* f);
	static void Link(const std::string sig, std::string* s);
};