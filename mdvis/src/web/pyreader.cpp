#include "anweb.h"
#include "anscript.h"
#include "anconv.h"
#include "vis/system.h"
#ifndef PLATFORM_WIN
#include <dlfcn.h>
#endif

void PyReader::Init() {
#ifdef PLATFORM_OSX
	if (dlsym(RTLD_DEFAULT, "Py_Initialize")) {
#endif
	static string pyenv = VisSystem::envs["PYENV"];
#ifdef PLATFORM_WIN
	size_t psz;
	getenv_s(&psz, 0, 0, "PATH");
	char* pbuf = new char[psz];
	getenv_s(&psz, pbuf, psz, "PATH");
	_putenv_s("PATH", &(pyenv + "\\;" + string(pbuf))[0]);
	delete[](pbuf);

	static std::wstring pyenvw = IO::_tow(pyenv);
	Py_SetPythonHome(&pyenvw[0]);
	static std::wstring pynmw = IO::_tow(pyenv + "\\python");
	Py_SetProgramName(&pynmw[0]);
#else
#error set python path
	Py_SetPythonHome(&pyenv[0]);
	Py_SetProgramName("python3");
#endif
	try {
		Py_Initialize();
		AnWeb::hasPy = true;
		PyScript::InitLog();
	} catch (char*) {
		string env = "";
#ifdef PLATFORM_WIN
		char* buf;
		size_t sz = 0;
		if (_dupenv_s(&buf, &sz, "EnvVarName") == 0 && buf)
		{
			env = buf;
			free(buf);
		}
#else
		env = getenv("PYTHONPATH");
#endif
		Debug::Warning("Python", "Cannot initialize python! PYTHONPATH env is: " + env);
	}
#ifdef PLATFORM_OSX
	} else {
		Debug::Warning("Python", "Python3.6 framework not loaded!");
	}
#endif
}

bool PyReader::Read(string path, PyScript* scr) { //"path/to/script[no .py]"
	PyScript::ClearLog();
	string mdn = scr->name = path;
	std::replace(mdn.begin(), mdn.end(), '/', '.');
	string spath = IO::path + "/nodes/" + scr->path + ".py";
	scr->chgtime = IO::ModTime(spath);
	//auto pName = PyUnicode_FromString(path.c_str());
	//scr->pModule = PyImport_Import(pName);
	if (AnWeb::hasPy) {
		auto mdl = scr->pModule ? PyImport_ReloadModule(scr->pModule) : PyImport_ImportModule(mdn.c_str());
		//Py_DECREF(pName);
		if (!mdl) {
			Debug::Warning("PyReader", "Failed to read python file " + path + "!");
			PyErr_Print();
			//
			std::cout << PyScript::GetLog() << std::endl;
			return false;
		}
		scr->pModule = mdl;
		scr->pFunc = PyObject_GetAttrString(scr->pModule, "Execute");
		if (!scr->pFunc || !PyCallable_Check(scr->pFunc)) {
			Debug::Warning("PyReader", "Failed to find \"Execute\" function in " + path + "!");
			Py_XDECREF(scr->pFunc);
			Py_DECREF(scr->pModule);
			return false;
		}
		Py_INCREF(scr->pFunc);
	}
	//extract io variables
	std::ifstream strm(spath);
	string ln;
	while (!strm.eof()) {
		std::getline(strm, ln);
		while (ln[0] == '#' && ln[1] == '#' && ln[2] == ' ') {
			scr->desc += ln.substr(3) + "\n";
			scr->descLines++;
			std::getline(strm, ln);
		}

		string ln2 = ln.substr(0, 4);
		if (ln2 == "#in ") {
			auto ss = string_split(ln, ' ');
			auto sz = ss.size() - 1;
			for (uint i = 0; i < sz; i++) {
				scr->_invars.push_back(PyVar());
				auto& bk = scr->_invars.back();
				bk.typeName = ss[i + 1];
				if (!ParseType(bk.typeName, &bk)) {
					Debug::Warning("PyReader::ParseType", "input arg type \"" + bk.typeName + "\" not recognized!");
					return false;
				}
			}
			std::getline(strm, ln);
			auto c1 = ln.find_first_of('('), c2 = ln.find_first_of(')');
			if (c1 == string::npos || c2 == string::npos || c2 <= c1) {
				Debug::Warning("PyReader::ParseType", "braces for input function not found!");
				return false;
			}
			ss = string_split(ln.substr(c1 + 1, c2 - c1 - 1), ',');
			if (ss.size() != sz) {
				Debug::Warning("PyReader::ParseType", "input function args count not consistent!");
				return false;
			}
			scr->pArgl = (AnWeb::hasPy) ? PyTuple_New(sz) : nullptr;
			scr->pArgs.resize(sz, 0);
			for (uint i = 0; i < sz; i++) {
				auto ns = ss[i].find_first_not_of(' ');
				auto ss2 = (ns == string::npos) ? ss[i] : ss[i].substr(ns);
				auto tn = scr->_invars[i].typeName;
				scr->_invars[i].name = ss2;
				scr->invars.push_back(std::pair<string, string>(scr->_invars[i].name, tn));
				if (*((int32_t*)&tn[0]) == *((int32_t*)"list")) {
					//scr->pArgs[i] = AnConv::PyArr(1, tn[6]);
				}
			}
		}
		else if (ln2 == "#out") {
			scr->_outvars.push_back(PyVar());
			auto& bk = scr->_outvars.back();
			bk.typeName = ln.substr(5);
			if (!ParseType(bk.typeName, &bk)) {
				Debug::Warning("PyReader::ParseType", "output arg type \"" + bk.typeName + "\" not recognized!");
				return false;
			}
			std::getline(strm, ln);
			bk.name = ln.substr(0, ln.find_first_of(' '));
			scr->outvars.push_back(std::pair<string, string>(bk.name, bk.typeName));
			if (AnWeb::hasPy) scr->pRets.push_back(PyObject_GetAttrString(scr->pModule, bk.name.c_str()));
			else scr->pRets.push_back(nullptr);
		}
	}

	if (!scr->invars.size()) {
		Debug::Warning("PyReader", "Script has no input parameters!");
	}
	if (!scr->outvars.size()) {
		Debug::Warning("PyReader", "Script has no output parameters!");
	}

	return true;
}

void PyReader::Refresh(PyScript* scr) {
	auto mt = IO::ModTime(IO::path + "/nodes/" + scr->path + ".py");
	if (mt > scr->chgtime) {
		Debug::Message("CReader", "Reloading " + scr->path + ".py");
		scr->Clear();
		scr->ok = Read(scr->path, scr);
	}
}

bool PyReader::ParseType(string s, PyVar* var) {
	if (s.substr(0, 3) == "int") var->type = AN_VARTYPE::INT;
	else if (s.substr(0, 6) == "double") var->type = AN_VARTYPE::DOUBLE;
	else if (s.substr(0, 4) == "list") {
		var->type = AN_VARTYPE::LIST;
		var->dim = s[5] - '1' + 1;
		var->stride = AnScript::StrideOf(s[6]);
	}
	else return false;
	return true;
}