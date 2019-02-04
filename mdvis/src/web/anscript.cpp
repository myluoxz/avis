#include "anweb.h"

bool AnScript::Clear() {
	inputs.clear();
	inputs.clear();
	desc = "";
	descLines = 0;
	ok = false;
	return true;
}

AnScript_I::~AnScript_I() {}

void* AnScript_I::Resolve(uintptr_t o) {
	return (void*)((uintptr_t)instance + o);
}

int AnScript_I::GetDimValue(CVar::szItem i) {
	return i.useOffset ? *(int*)Resolve(i.offset) : i.size;
}

void AnScript_I::Execute() {
	parent->caller(instance);
}

float AnScript_I::GetProgress() {
	return 0.1f;
}