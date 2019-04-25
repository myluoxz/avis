#pragma once
#include "anweb.h"
#include "ui/localizer.h"

#define INODE_DEF_NOREG_H \
	static const std::string sig;\
	static const char* _name;\
	static std::unique_ptr<DmScript> scr;\
	static std::shared_ptr<AnNode> _Spawn();

#define INODE_DEF_H INODE_DEF_NOREG_H\
	static AnNode_Internal_Reg _reg;

#define INODE_DEF_NOREG(tt, nm) \
	const std::string Node_ ## nm::sig = "." #nm;\
	const char* Node_ ## nm::_name = tt;\
	std::unique_ptr<DmScript> Node_ ## nm::scr = std::unique_ptr<DmScript>(new DmScript(sig));\
	std::shared_ptr<AnNode> Node_ ## nm::_Spawn() { return std::make_shared<Node_ ## nm>(); } 

#define INODE_DEF(tt, nm, gp) INODE_DEF_NOREG(tt, nm) \
	AnNode_Internal_Reg Node_ ## nm::_reg = AnNode_Internal_Reg(\
		Node_ ## nm::sig, Node_ ## nm::_name, ANNODE_GROUP::gp, &Node_ ## nm::_Spawn);

#define INODE_INIT AnNode(scr->CreateInstance(), 0)
#define INODE_INITF(f) AnNode(scr->CreateInstance(), f)

#define INODE_TITLE(col) \
	title = _(_name);\
	titleCol = col;

#define INODE_SINIT(cmd) if (!scr->ok) {\
		cmd\
		scr->ok = true;\
		script->Init(scr.get());\
	}\
	ResizeIO(scr.get());\
	conV.clear();

enum class ANNODE_GROUP {
	GET,
	SET,
	GEN,
	CONV,
	MISC
};
#define ANNODE_GROUP_COUNT 5

class AnNode_Internal {
public:
	static void Init();

	typedef pAnNode (*spawnFunc)();
	struct noderef {
		noderef(std::string s, std::string n, spawnFunc f)
			: sig(s), name(n), spawner(f) {}
		const std::string sig;
		std::string name;
		const spawnFunc spawner;
	};
	static std::array<std::vector<noderef>, ANNODE_GROUP_COUNT> scrs;
	static std::array<std::string, ANNODE_GROUP_COUNT> groupNms;
};

class AnNode_Internal_Reg {
	typedef pAnNode (*spawnFunc)();
public:
	AnNode_Internal_Reg(const std::string sig, const std::string nm, ANNODE_GROUP grp, spawnFunc func);
};