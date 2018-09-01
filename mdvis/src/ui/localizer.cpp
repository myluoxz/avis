#include "localizer.h"

bool Localizer::useDict = false;

void Localizer::Init(const string& nm) {
	if (nm == "EN") return;
    auto path = IO::path + "/locale/" + nm + "/locale.txt";
    std::ifstream strm(path);
    if (!strm.is_open()) {
        Debug::Warning("System", "locale not found! defaulting to english...");
		return;
    }
    char buf[500]{};
    dict.clear();
    std::pair<uint32_t, string> pr;
    int l = 1;
    while (strm.getline(buf, 20)) {
        if (!buf[0]) {
            l++;
            continue;
        }
        pr.first = TryParse(string(buf), ~0U);
        if (pr.first == ~0U) {
            Debug::Warning("System", "Locale syntax error at line " + std::to_string(l));
			return;
        }
        strm.getline(buf, 500);
        if (buf[0] != '<') {
            Debug::Warning("System", "Locale syntax error at line " + std::to_string(l + 1));
			return;
        }
        strm.getline(buf, 500);
        if (buf[0] != '>') {
            Debug::Warning("System", "Locale syntax error at line " + std::to_string(l + 2));
			return;
        }
        pr.second = buf + 1;
        dict.emplace(pr);
        l += 3;
    }
	useDict = true;
}

void Localizer::MakeMap(string path) {
	std::cout << "Scanning keywords..." << std::endl;
    path = IO::path + "/" + path;
    std::map<uint32_t, string> strs;
    _MakeMap(path, strs);
	std::cout << "Writing " << strs.size() << " keywords..." << std::endl;
    std::ofstream strm(IO::path + "/locale/template.txt");
    std::ofstream strm2(IO::path + "/locale/EN/locale.txt");
    for (auto& a : strs) {
        strm << a.first << "\n<" << a.second << "\n>\n\n";
        strm2 << a.first << "\n<" << a.second << "\n>" + a.second + "\n\n";
    }
	std::vector<string> dirs;
	IO::GetFolders(IO::path + "/locale/", &dirs);
	for (auto& d : dirs) {
		if (d != "EN") {
			std::cout << "Updating locale: " << d << std::endl;
			Merge(d, strs);
		}
	}
	std::cout << "Done!" << std::endl;
	std::getline(std::cin, path);
	return;
}

void Localizer::_MakeMap(string path, std::map<uint32_t, string>& strs) {
    std::vector<string> nms = IO::GetFiles(path, ".cpp");
    for (auto& n : nms) {
        string cd = IO::GetText(path + "/" + n);
        auto cds = cd.size();
        int p0 = 0, p1 = 0;
        for(;;) {
            p0 = string_find(cd, "_(\"", p1);
            if (p0 == -1) break;
            p1 = string_find(cd, "\")", p0);
            auto str = cd.substr(p0 + 3, p1 - p0 - 3);
            strs[HASH(str.c_str())] = str;
        }
    }
    nms.clear();
    IO::GetFolders(path, &nms);
    for (auto& n : nms)
        _MakeMap(path + "/" + n, strs);
}

void Localizer::Merge(string path, std::map<uint32_t, string> strs) {
	std::ifstream strm(IO::path + "/locale/" + path + "/locale.txt", std::ios::binary);
	if (strm.is_open()) {
		std::ofstream ostrm(IO::path + "/locale/" + path + "/locale_new.txt", std::ios::binary);
		char buf[500];
		int l = 0;
		while (strm.getline(buf, 500)) {
			if (!buf[0]) {
				ostrm << "\n";
				l++;
				continue;
			}
			uint32_t i = TryParse(string(buf), ~0U);
			if (i == ~0U) {
				Debug::Warning("Localizer", path + "Locale syntax error at line " + std::to_string(l));
				return;
			}
			strm.getline(buf, 500);
			if (buf[0] != '<') {
				Debug::Warning("Localizer", path + "Locale syntax error at line " + std::to_string(l + 1));
				return;
			}
			strm.getline(buf, 500);
			if (buf[0] != '>') {
				Debug::Warning("Localizer", path + "Locale syntax error at line " + std::to_string(l + 2));
				return;
			}
			if (strs.count(i) == 1) {
				auto s = strs[i];
				ostrm << i << "\n<" << s << "\n" << string(buf) << "\n";
				strs.erase(i);
			}
			l += 3;
		}
		for (auto& a : strs) {
			ostrm << a.first << "\n<" << a.second << "\n>\n\n";
		}
	}
}

std::unordered_map<uint32_t, string> Localizer::dict;